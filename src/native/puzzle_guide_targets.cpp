// Four ticketed owned slots transfer bounded target text and script identity.
// Capture invalidates previous movement; worker take never follows native tokens.
// See ../../Docs/features/puzzles/index.md#other-progression-puzzles.
#include "puzzle_guide_targets.hpp"
#include "process_storage.hpp"
#include "navigation_observations.hpp"
#include "navigation_intents.hpp"
#include "puzzle_guide_owned.hpp"
#include "auto_walk.hpp"
#include "nav_audio.hpp"
#include "utility_memory.hpp"
#include <atomic>
#include <cstring>
namespace ffx {
namespace {
constexpr unsigned Slots=4,Writing=0x80000000u;
struct Frame {bool talk=false;int local=-1;Point position;UtilityText prefix;std::array<wchar_t,1025> finish;unsigned length=0,epoch=0,revision=0,serial=0;uintptr_t chunk=0;};
ProcessStorage<std::array<Frame,Slots>> storage;
std::array<std::atomic<unsigned>,Slots> tickets{};std::atomic<unsigned> nextTicket{0};
void release(unsigned slot,unsigned ticket) {if(slot<Slots) tickets[slot].compare_exchange_strong(ticket,0,std::memory_order_release);}
}
void startPuzzleGuideTargets() {storage.prepare();}
bool queuePuzzleGuidePoint(Point point,int local,bool talk,const wchar_t* finish,const UtilityText& prefix) {
    if(!nativeGameThread() || !storage.ready() || !prefix.valid || !finish) return false;
    unsigned length=0;while(length<=1024 && finish[length]) ++length;if(length>1024) return false;
    UtilityScript script;if(!captureUtilityScript(script)) return false;
    unsigned slot=Slots,token=(nextTicket.fetch_add(1)+1)&~Writing;if(!token) token=(nextTicket.fetch_add(1)+1)&~Writing;
    for(unsigned i=0;i<Slots;++i) {unsigned empty=0;if(tickets[i].compare_exchange_strong(empty,token|Writing,std::memory_order_acquire)) {slot=i;break;}}
    if(slot==Slots) return false;auto& frame=storage.get()[slot];frame.talk=talk;frame.local=local;frame.position=point;frame.prefix=prefix;
    std::wmemcpy(frame.finish.data(),finish,length);frame.finish[length]=0;frame.length=length;
    frame.epoch=generation();frame.revision=nav::navigationObservationRevision();frame.chunk=script.chunk;frame.serial=script.allocationSerial;
    nav::invalidateNavigationIntents();cancelPedestal();clearNavRoute();stopAutoWalk();tickets[slot].store(token,std::memory_order_release);
    if(!nav::submitNavigationQuery(nav::Query::GuideTarget,false,ownedPuzzleGuideRevision(),slot,script.chunk,token)) {release(slot,token);return false;}return true;
}
bool takePuzzleGuideTarget(unsigned slot,unsigned token,QueuedPuzzleTarget& result) {
    if(nativeGameThread() || !storage.ready() || slot>=Slots || tickets[slot].load(std::memory_order_acquire)!=token) return false;
    struct Return {unsigned slot,token;~Return(){release(slot,token);}} returned{slot,token};const auto& frame=storage.get()[slot];
    if(frame.epoch!=generation() || frame.revision!=nav::navigationObservationRevision() || frame.length>1024) return false;
    result.target={};result.target.name=frame.local<0?L"Warp panel approach":L"Via Purifico guide target";result.target.position=frame.position;result.target.worker=frame.local;result.target.zone.talk=frame.talk;
    result.prefix=renderUtilityText(frame.prefix);result.finish.assign(frame.finish.data(),frame.length);result.chunk=frame.chunk;result.serial=frame.serial;return true;
}
void retirePuzzleGuideTargets() {
    if(nativeGameThread() || !storage.ready()) return;
    for(unsigned i=0;i<Slots;++i) {const auto token=tickets[i].load(std::memory_order_acquire);if(!token || (token&Writing)) continue;const auto& frame=storage.get()[i];if(frame.epoch!=generation() || frame.revision!=nav::navigationObservationRevision()) release(i,token);}
}
}
