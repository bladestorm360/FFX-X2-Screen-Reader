#include "reachability.hpp"
#include "reachability_shared.hpp"
#include "navigation_scene.hpp"
#include "process_storage.hpp"
#include "settings.hpp"
#include <cmath>
#include <limits>
namespace ffx {
namespace {
enum class Stage { Idle,Results };
Stage stage=Stage::Idle;
std::atomic<unsigned> revision{1};
unsigned request=0,committed=0;
reach::Context context;
std::vector<Entity>* owner=nullptr; // mod-owned, native owner only
std::array<reach::Target,reach::MaxTargets> capturedTargets;
struct OwnedRequest {unsigned epoch=0,request=0,committed=0,count=0;reach::Context context;std::array<reach::Target,reach::MaxTargets> targets;std::array<bool,reach::MaxTargets> unreachable{};};
ProcessStorage<OwnedRequest> ownedRequest;
bool validPoint(Point p) { return std::isfinite(p.x) && std::isfinite(p.y) && std::isfinite(p.z); }
bool pointer(uintptr_t p,unsigned bytes) { return p>=0x10000 && p!=0xcccccccc && p!=0xcdcdcdcd && p<=UINTPTR_MAX-bytes; }
bool currentContext(reach::Context& c) {
    return setting(Setting::HideUnreachable) && nav::captureNativeGeometry(c,revision.load(std::memory_order_acquire));
}

bool actorStillCurrent(const Entity& e) {
    if(e.actor<0) return true;
    const auto actors=read<uintptr_t>(address(0x1fc44e4));
    const unsigned count=read<unsigned>(address(0x1fc44e0));
    if(!actors || !count || count>128 || static_cast<unsigned>(e.actor)>=count || !pointer(actors,count*0x880)) return false;
    const auto actor=actors+e.actor*0x880;
    const auto enabled=read<uint16_t>(actor+2);
    if(read<uint16_t>(actor)!=e.id || !enabled || enabled>255 || read<unsigned>(actor+0x824)==0xcdcdcdcd) return false;
    const Point live{read<float>(actor+0xc),read<float>(actor+0x10),read<float>(actor+0x14)};
    return validPoint(live) && live.x==e.position.x && live.y==e.position.y && live.z==e.position.z;
}
reach::Target target(const Entity& e,bool ownedFacts=false) {
    reach::Target t{};t.shape=static_cast<const InteractionShape&>(e.zone);t.position=e.position;
    t.approach=e.zone.approach;t.obstacle=e.zone.obstacle;t.contactRadius=e.zone.contactRadius;
    t.minY=e.zone.contactMinY;t.maxY=e.zone.contactMaxY;t.obstacleRadius=e.zone.obstacleRadius;
    t.id=e.id;t.actor=e.actor;t.worker=e.worker;t.pushControl=e.pushControl;t.exit=e.category==L"Exit";
    t.swimApproach=e.swimApproach;t.swimDepth=e.swimDepth;t.inactive=e.inactive;
    t.unsupported=e.butterfly || e.zone.competing.size()>reach::MaxCompetitors || !validPoint(e.position) || (!ownedFacts && !actorStillCurrent(e));
    if(!t.unsupported) { t.count=static_cast<unsigned>(e.zone.competing.size());
        std::copy(e.zone.competing.begin(),e.zone.competing.end(),t.competing.begin()); }
    return t;
}
void cancel(bool clear=true) {
    reach::latest.fetch_add(1,std::memory_order_release);stage=Stage::Idle;request=committed=0;
    if(clear && owner) {
        bool changed=false;for(auto& e:*owner) { changed=changed || e.unreachable;e.unreachable=false; }
        if(changed) navigationReachabilityChanged();
    }
    owner=nullptr;
}
void advance(Event&) {
    if(!nativeGameThread() || stage==Stage::Idle || !owner) return;
    reach::Context now;if(!currentContext(now) || !reach::sameContext(now,context)) {cancel();return;}
    bool changed=false;
    for(unsigned i=0;i<committed && i<owner->size();++i) {
        auto& e=(*owner)[i];
        if(e.unreachable && !reach::sameTarget(capturedTargets[i],target(e))) {e.unreachable=false;changed=true;}
    }
    reach::ResultBatch batch;
    if(!reach::results.tryRead(batch) || batch.request!=request ||
        batch.count>owner->size() || batch.count>reach::MaxTargets || batch.count<=committed) {
        if(changed) navigationReachabilityChanged();return;
    }
    // Worker progress contains the complete prefix, so queue pressure cannot
    // discard a target verdict. Only newly completed indices are applied.
    for(;committed<batch.count;++committed) {
        auto& e=(*owner)[committed];
        const bool hidden=batch.unreachable[committed] && reach::sameTarget(capturedTargets[committed],target(e));
        changed=changed || e.unreachable!=hidden;e.unreachable=hidden;
    }
    if(changed) navigationReachabilityChanged();
}
void retireGeometry() {
    revision.fetch_add(1,std::memory_order_acq_rel);
    reach::latest.fetch_add(1,std::memory_order_release);
}
void meshChanged(Event&) {
    // Loader callers only retire owned worker results. Capture state/containers
    // remain native-owner-only; the next owner event observes changed geometry.
    retireGeometry();
    if(nativeGameThread()) cancel();
}
}
void requestOwnedReachability(const nav::Scene& observation,std::vector<Entity>& entities,bool preserve) {
    if(nativeGameThread() || !ownedRequest.ready()) return;
    auto& previous=ownedRequest.get();
    // A command refreshes the entity vector before selecting from it. Reuse
    // verdicts for the exact same copied context/targets, including an in-flight
    // prefix; clearing them on every refresh made the filter appear disabled.
    bool same=setting(Setting::HideUnreachable) && observation.geometryValid && previous.request &&
        previous.epoch==observation.epoch && previous.count==entities.size() &&
        previous.request==reach::latest.load(std::memory_order_acquire) && reach::sameContext(previous.context,observation.geometry);
    if(same) for(unsigned i=0;i<previous.count;++i) if(!reach::sameTarget(previous.targets[i],target(entities[i],true))) {same=false;break;}
    if(same) {
        for(unsigned i=0;i<previous.count;++i) entities[i].unreachable=i<previous.committed && previous.unreachable[i];
        (void)applyOwnedReachability(observation,entities);return;
    }
    reach::latest.fetch_add(1,std::memory_order_acq_rel);previous.epoch=previous.request=previous.committed=previous.count=0;
    if(!preserve) for(auto& entity:entities) entity.unreachable=false;
    if(!setting(Setting::HideUnreachable) || !observation.geometryValid || entities.empty() || entities.size()>reach::MaxTargets) return;
    const auto slot=reach::claimSnapshot();if(slot==reach::NoSnapshot) return;
    auto& copied=reach::snapshot(slot);copied.context=observation.geometry;copied.count=static_cast<unsigned>(entities.size());
    copied.request=reach::latest.load(std::memory_order_acquire);copied.capturedAt=GetTickCount64();
    reach::Geometry cached;copied.geometryCopied=!reach::cachedGeometryReady.tryRead(cached) || !reach::sameGeometry(cached,copied.context.geometry);
    for(unsigned i=0;i<copied.count;++i) {copied.targets[i]=target(entities[i],true);previous.targets[i]=copied.targets[i];}
    if(copied.geometryCopied) {
        std::copy(observation.triangles.begin(),observation.triangles.begin()+copied.context.geometry.count,copied.triangles.begin());
        std::copy(observation.vertices.begin(),observation.vertices.begin()+copied.context.geometry.vertexCount,copied.vertices.begin());
    }
    if(!reach::captures.tryPush(slot)) {reach::releaseSnapshot(slot);return;}
    previous.epoch=observation.epoch;previous.request=copied.request;previous.count=copied.count;previous.context=copied.context;reach::wakeWorker();
}
bool applyOwnedReachability(const nav::Scene& observation,std::vector<Entity>& entities) {
    if(nativeGameThread() || !ownedRequest.ready()) return false;
    auto& pendingRequest=ownedRequest.get();if(!pendingRequest.request || pendingRequest.epoch!=generation() || !reach::sameContext(observation.geometry,pendingRequest.context)) return false;
    reach::ResultBatch batch;if(!reach::results.tryRead(batch) || batch.request!=pendingRequest.request || batch.count>pendingRequest.count || batch.count>entities.size() || batch.count<=pendingRequest.committed) return false;
    bool changed=false;
    for(;pendingRequest.committed<batch.count;++pendingRequest.committed) {
        auto& entity=entities[pendingRequest.committed];const bool hide=batch.unreachable[pendingRequest.committed] && reach::sameTarget(pendingRequest.targets[pendingRequest.committed],target(entity,true));
        changed=changed || hide!=entity.unreachable;entity.unreachable=hide;pendingRequest.unreachable[pendingRequest.committed]=hide;
    }
    return changed;
}
bool ownedReachabilityChecking() {return ownedRequest.ready() && ownedRequest.get().request && ownedRequest.get().committed<ownedRequest.get().count;}
void retireOwnedReachability() {if(nativeGameThread() || !ownedRequest.ready()) return;reach::latest.fetch_add(1,std::memory_order_release);auto& requestState=ownedRequest.get();requestState.epoch=requestState.request=requestState.committed=requestState.count=0;}
bool reachabilityChecking() { return stage!=Stage::Idle && owner && committed<owner->size(); }
bool reachabilityContextValid() {
    reach::Context now;return stage!=Stage::Idle && currentContext(now) && reach::sameContext(context,now);
}
bool sameReachabilityTarget(const Entity& before,const Entity& after) { return reach::sameTarget(target(before),target(after)); }
void requestReachability(std::vector<Entity>& entities,Point,bool preserve) {
    if(!nativeGameThread()) return;
    cancel(!preserve);
    if(entities.empty() || entities.size()>reach::MaxTargets || !currentContext(context)) return;
    const auto slot=reach::claimSnapshot();
    if(slot==reach::NoSnapshot) {recordLog("reachability snapshot pressure; destinations retained");return;}
    auto& copied=reach::snapshot(slot);copied.context=context;copied.count=static_cast<unsigned>(entities.size());
    copied.request=reach::latest.load(std::memory_order_acquire);copied.capturedAt=GetTickCount64();
    reach::Geometry readyGeometry;
    copied.geometryCopied=!reach::cachedGeometryReady.tryRead(readyGeometry) || !reach::sameGeometry(readyGeometry,context.geometry);
    for(unsigned i=0;i<copied.count;++i) {capturedTargets[i]=target(entities[i]);copied.targets[i]=capturedTargets[i];}
    if(copied.geometryCopied &&
        ((context.geometry.count && !copyReadyRegion(context.geometry.triangles,copied.triangles.data(),context.geometry.count*16,context.geometry.count*16)) ||
         (context.geometry.vertexCount && !copyReadyRegion(context.geometry.vertices,copied.vertices.data(),context.geometry.vertexCount*8,context.geometry.vertexCount*8)))) {
        reach::releaseSnapshot(slot);return;
    }
    if(!reach::captures.tryPush(slot)) {reach::releaseSnapshot(slot);return;}
    owner=&entities;request=copied.request;stage=Stage::Results;reach::wakeWorker();
}
void refreshReachabilityAfterMovement(std::vector<Entity>& entities) {
    if(!nativeGameThread() || !setting(Setting::HideUnreachable) || entities.empty()) return;
    reach::Context now;
    if(!currentContext(now)) {if(owner) cancel();return;}
    if(stage!=Stage::Idle && owner==&entities && reach::sameContext(context,now)) return;
    requestReachability(entities,now.player);
}
void resetReachability() { retireGeometry();if(nativeGameThread()) cancel(); }
void installReachability() {
    ownedRequest.prepare();
    // 43EA00 replaces the collision resource before readers may use its pages.
    hookSnapshot(0x43ea00,meshChanged,nullptr);
    hook(0x225930,nullptr,advance);
    reach::prepareSnapshots();reach::startWorker();
}
#ifdef FFX_READER_TEST
void advanceReachabilityForTest() { Event e;advance(e); }
bool reachabilityCompleteForTest() { reach::Reply r;return stage==Stage::Results && reach::replies.tryRead(r) && r.request==request && r.mode==4; }
#endif
}
