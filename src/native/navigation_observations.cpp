#include "navigation_observations.hpp"
#include "process_storage.hpp"
#include "nonblocking_snapshots.hpp"
#include "auto_walk.hpp"
#include "navigation_intents.hpp"
#include <atomic>
#include <cstring>
namespace ffx::nav {
namespace {
constexpr unsigned Domain=10,Slots=4,Writing=0x80000000u;
struct Frame {Scene scene;unsigned requested=0,revision=0;ULONGLONG capturedAt=0;};
ProcessStorage<std::array<Frame,Slots>> frames;
async::AtomicSnapshot<AreaNameDemand> areaNames;
std::array<std::atomic<unsigned>,Slots> tickets{};
std::atomic<unsigned> nextTicket{0},nextQuery{0},requested{0},revision{1};
std::atomic<QueryConsumer> queryConsumer{nullptr};std::atomic<SceneConsumer> sceneConsumer{nullptr};std::atomic<ObservationReset> resetConsumer{nullptr};
std::atomic<bool> started{false},waiting{false};
// This field belongs to the native capture owner, never the preparation owner.
unsigned lastCaptured=0,lastRevision=0;
bool pressure=false,captureUnavailable=false;
struct Packet {unsigned kind=0,slot=0,ticket=0;QueryObservation query;};
unsigned nonzero(std::atomic<unsigned>& counter) {const auto next=counter.fetch_add(1,std::memory_order_acq_rel)+1;return next?next:counter.fetch_add(1,std::memory_order_acq_rel)+1;}
void release(unsigned slot,unsigned ticket) {if(slot<Slots) tickets[slot].compare_exchange_strong(ticket,0,std::memory_order_release,std::memory_order_relaxed);}
void process(const speechq::Header& header,const std::wstring& owned) {
    if(owned.size()*sizeof(wchar_t)!=sizeof(Packet)) return;
    Packet p;std::memcpy(&p,owned.data(),sizeof(p));
    if(!p.kind) {
        if(p.query.epoch==generation() && p.query.revision==revision.load(std::memory_order_acquire))
            if(const auto consume=queryConsumer.load(std::memory_order_acquire)) consume(header,p.query);
        return;
    }
    if(p.kind!=1 || p.slot>=Slots || tickets[p.slot].load(std::memory_order_acquire)!=p.ticket) return;
    // Every exit, including a canceled/throwing consumer, returns the slot.
    struct Return {unsigned slot,ticket;~Return(){release(slot,ticket);}} returned{p.slot,p.ticket};
    const auto& frame=frames.get()[p.slot];
    if(frame.scene.epoch!=generation() || frame.revision!=revision.load(std::memory_order_acquire)) return;
    if(const auto consume=sceneConsumer.load(std::memory_order_acquire)) consume(header,{frame.scene,frame.requested,frame.capturedAt});
}
void retireCanceled() {
    for(unsigned slot=0;slot<Slots;++slot) {
        const auto token=tickets[slot].load(std::memory_order_acquire);if(!token || (token&Writing)) continue;
        const auto& frame=frames.get()[slot];
        if(frame.scene.epoch!=generation() || frame.revision!=revision.load(std::memory_order_acquire)) release(slot,token);
    }
}
void reset(unsigned epoch) {retireCanceled();if(const auto consume=resetConsumer.load(std::memory_order_acquire)) consume(epoch);}
void prepared() noexcept {if(waiting.load(std::memory_order_acquire)) requestNavigationScene();speechq::notifyPreparation();}
}
void startNavigationObservations(QueryConsumer queries,SceneConsumer scenes,ObservationReset resets) {
    if(started.load(std::memory_order_acquire)) return;
    frames.prepare();prepareNavigationSceneStorage();queryConsumer.store(queries,std::memory_order_release);sceneConsumer.store(scenes,std::memory_order_release);resetConsumer.store(resets,std::memory_order_release);
    if(!onAtelPrepared(prepared)) throw std::runtime_error("Navigation preparation notification unavailable");
    speechq::registerDomain(Domain,process,reset);speechq::registerDomainMaintenance(Domain,retireCanceled);started.store(true,std::memory_order_release);
}
bool submitNavigationQuery(Query query,bool shift,unsigned policyRevision,unsigned local,uintptr_t chunk,unsigned resourceSerial) {
    if(!started.load(std::memory_order_acquire)) return false;
    Packet p;p.query={generation(),revision.load(std::memory_order_acquire),nonzero(nextQuery),nonzero(requested),query,shift};
    p.query.manual=autoWalkInputRevision();p.query.intentRevision=navigationIntentRevision();
    p.query.policyRevision=policyRevision;p.query.local=local;p.query.chunk=chunk;p.query.resourceSerial=resourceSerial;
    // The demand stamp identifies observations captured after this request.
    // Sharing one observation across a burst never merges its independent actions.
    if(!speechq::captureDomain(Domain,p,p.query.epoch)) {output(L"Navigation is busy. Try the command again after it recovers.",false,"navigation-pressure");return false;}
    waiting.store(true,std::memory_order_release);return true;
}
bool captureRequestedNavigationScene(bool geometry,bool changed) {
    if(!nativeGameThread() || !started.load(std::memory_order_acquire)) return false;
    const auto want=requested.load(std::memory_order_acquire),currentRevision=revision.load(std::memory_order_acquire);
    if(!want && !lastRevision) return false;
    if(!changed && want==lastCaptured && currentRevision==lastRevision) return false;
    // Field facts/getters are unavailable while a native battle/menu/resource
    // reset owns the scene. A genuine field/control event supplies the retry.
    if(scene || inBattle() || !nativeReaderResourcesReady()) {lastCaptured=want;lastRevision=currentRevision;return false;}
    unsigned slot=Slots,token=nonzero(nextTicket)&~Writing;if(!token) token=nonzero(nextTicket)&~Writing;
    for(unsigned i=0;i<Slots;++i) {unsigned empty=0;if(tickets[i].compare_exchange_strong(empty,token|Writing,std::memory_order_acquire,std::memory_order_relaxed)) {slot=i;break;}}
    if(slot==Slots) {if(!pressure) output(L"Navigation data is busy. Updates resume when it recovers.",false,"navigation-pressure");pressure=true;return false;}
    auto& frame=frames.get()[slot];frame.requested=want;frame.revision=currentRevision;frame.capturedAt=GetTickCount64();
    if(!captureNavigationScene(frame.scene,currentRevision,geometry) || frame.scene.epoch!=generation() || currentRevision!=revision.load(std::memory_order_acquire)) {
        release(slot,token|Writing);
        // A refused observation is retried only on a new request or a real
        // owner event/resource completion, never by scanning every frame.
        lastCaptured=want;lastRevision=currentRevision;
        const bool pendingCommand=waiting.load(std::memory_order_acquire);
        if(pendingCommand && !captureUnavailable) output(L"Navigation data is unavailable. Waiting for the field to become ready.",false,"navigation-data-unavailable");
        captureUnavailable=pendingCommand;return false;
    }
    AreaNameDemand names;if(areaNames.tryRead(names)) captureNavigationAreaNames(frame.scene,names);
    tickets[slot].store(token,std::memory_order_release);
    Packet p;p.kind=1;p.slot=slot;p.ticket=token;
    if(!speechq::captureDomain(Domain,p,frame.scene.epoch)) {release(slot,token);return false;}
    lastCaptured=want;lastRevision=currentRevision;pressure=captureUnavailable=false;return true;
}
void navigationWaitingForData(bool value) {waiting.store(value,std::memory_order_release);}
bool navigationDataPending() {return waiting.load(std::memory_order_acquire);}
void requestNavigationScene() {requested.fetch_add(1,std::memory_order_release);}
void requestNavigationAreaNames(const AreaNameDemand& names) {areaNames.publish(names);requestNavigationScene();}
void resetNavigationObservations() {waiting=false;revision.fetch_add(1,std::memory_order_acq_rel);requestNavigationScene();}
unsigned navigationObservationRevision() {return revision.load(std::memory_order_acquire);}
const std::atomic<unsigned>& navigationRevisionCounter() {return revision;}
#ifdef FFX_READER_TEST
unsigned navigationSceneSlotsForTest() {unsigned count=0;for(auto& slot:tickets) if(slot.load(std::memory_order_acquire)) ++count;return count;}
#endif
}
