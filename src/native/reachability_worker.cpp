#include "reachability_shared.hpp"
#include "reachability.hpp"
#include "process_storage.hpp"
#include <algorithm>
#include <limits>
#include <map>
#include <tuple>
namespace ffx::reach {
async::TryCaptureQueue<unsigned,SnapshotSlots> captures;
async::AtomicSnapshot<Geometry> cachedGeometryReady;
namespace {
ProcessStorage<std::array<Snapshot,SnapshotSlots>> snapshotStorage;
std::atomic<unsigned> claimed{0};
}
void prepareSnapshots() {snapshotStorage.prepare();}
unsigned claimSnapshot() {
    if(!snapshotStorage.ready()) return NoSnapshot;
    auto observed=claimed.load(std::memory_order_relaxed);
    for(unsigned i=0;i<SnapshotSlots;++i) if(!(observed&(1u<<i))) {
        if(claimed.compare_exchange_strong(observed,observed|(1u<<i),std::memory_order_acquire,std::memory_order_relaxed)) return i;
    }
    return NoSnapshot;
}
Snapshot& snapshot(unsigned slot) {return snapshotStorage.get()[slot];}
void releaseSnapshot(unsigned slot) {if(slot<SnapshotSlots) claimed.fetch_and(~(1u<<slot),std::memory_order_release);}
async::AtomicSnapshot<ResultBatch> results;
async::AtomicSnapshot<Reply> replies;
std::atomic<unsigned> latest{1};
std::atomic<ReachabilityPrepared> preparedHandler{nullptr};
bool sameGeometry(const Geometry& a,const Geometry& b) {
    return a.epoch==b.epoch && a.revision==b.revision && a.count==b.count && a.vertexCount==b.vertexCount && a.root==b.root && a.triangles==b.triangles && a.vertices==b.vertices && a.scale==b.scale;
}
bool sameContext(const Context& a,const Context& b) {
    return a.actor==b.actor && sameGeometry(a.geometry,b.geometry) && a.floor==b.floor && a.swimming==b.swimming && a.waterBake==b.waterBake && a.waterCrossings==b.waterCrossings && nav::sameRouteEvents(a.events,b.events) &&
        a.player.x==b.player.x && a.player.y==b.player.y && a.player.z==b.player.z &&
        a.collision.actorFlags==b.collision.actorFlags && a.collision.actorIsControlled==b.collision.actorIsControlled && a.collision.remaps==b.collision.remaps;
}
namespace {
bool samePoint(Point a,Point b) { return a.x==b.x && a.y==b.y && a.z==b.z; }
bool sameShape(const InteractionShape& a,const InteractionShape& b) {
    return samePoint(a.center,b.center) && samePoint(a.end,b.end) && a.angle==b.angle && a.width==b.width &&
        a.depth==b.depth && a.range==b.range && a.height==b.height && a.viewAngle==b.viewAngle &&
        a.shape==b.shape && a.worker==b.worker && a.valid==b.valid && a.crossing==b.crossing && a.talk==b.talk;
}
}
bool sameTarget(const Target& a,const Target& b) {
    if(!sameShape(a.shape,b.shape) || !samePoint(a.position,b.position) || !samePoint(a.approach,b.approach) ||
        !samePoint(a.obstacle,b.obstacle) || a.contactRadius!=b.contactRadius || a.minY!=b.minY || a.maxY!=b.maxY ||
        a.obstacleRadius!=b.obstacleRadius || a.count!=b.count || a.id!=b.id || a.pushControl!=b.pushControl ||
        a.actor!=b.actor || a.worker!=b.worker || a.exit!=b.exit || a.swimApproach!=b.swimApproach || a.swimDepth!=b.swimDepth ||
        a.inactive!=b.inactive || a.unsupported!=b.unsupported) return false;
    for(unsigned i=0;i<a.count;++i) if(!sameShape(a.competing[i],b.competing[i])) return false;
    return true;
}
namespace {
struct Job { unsigned request=0,received=0,triangles=0,vertices=0; Context context; std::vector<Target> targets;
    std::vector<RawNavigationTriangle> raw; std::vector<RawNavigationVertex> points; bool reuse=false; };
struct WorkerStorage { Job job; std::vector<RawNavigationTriangle> cachedRaw; std::vector<RawNavigationVertex> cachedPoints; Mesh ownedMesh; };
ProcessStorage<WorkerStorage> workerStorage;
Job& job() noexcept {return workerStorage.get().job;}
std::vector<RawNavigationTriangle>& cachedRaw() noexcept {return workerStorage.get().cachedRaw;}
std::vector<RawNavigationVertex>& cachedPoints() noexcept {return workerStorage.get().cachedPoints;}
Mesh& ownedMesh() noexcept {return workerStorage.get().ownedMesh;}
Geometry cachedGeometry; bool haveGeometry=false;
std::atomic<HANDLE> wake{nullptr};
Context checkedContext; bool haveChecks=false;
struct Check { Target target; bool unreachable=false; };
using Key=std::tuple<int,int,unsigned,unsigned,bool,bool>;
using Checked=std::map<Key,Check>;
ProcessStorage<Checked> checkedStorage;
Checked& checked() noexcept {return checkedStorage.get();}
Context meshContext;bool haveMesh=false;
bool alive() { return job().request && job().request==latest.load(std::memory_order_acquire); }
Entity entity(const Target& t) {
    Entity e; e.category=t.exit?L"Exit":L"Object"; e.position=t.position;e.swimApproach=t.swimApproach;e.swimDepth=t.swimDepth;e.inactive=t.inactive;
    static_cast<InteractionShape&>(e.zone)=t.shape;
    e.zone.contactRadius=t.contactRadius;e.zone.contactMinY=t.minY;e.zone.contactMaxY=t.maxY;
    e.zone.approach=t.approach;e.zone.obstacle=t.obstacle;e.zone.obstacleRadius=t.obstacleRadius;
    e.zone.competing.assign(t.competing.begin(),t.competing.begin()+t.count);
    return e;
}
void finish() {
    if(!alive() || job().received!=job().targets.size()) return;
    nav::ComputationScope computation(latest,job().request);
    if(!job().reuse) {
        if(job().triangles!=job().context.geometry.count || job().vertices!=job().points.size()) return;
        cachedGeometry=job().context.geometry;cachedRaw()=std::move(job().raw);cachedPoints()=std::move(job().points);haveGeometry=true;
    }
    if(!haveChecks || !sameContext(checkedContext,job().context)) { checked().clear();checkedContext=job().context;haveChecks=true; }
    // Geometry, decoding, allocation, graph searches and counterfactual meshes
    // are worker-owned. Geometry addresses are opaque comparison tokens only.
    if(!haveMesh || !sameContext(meshContext,job().context)) {
        ownedMesh()=decodeOwnedNavigationMesh(cachedRaw(),cachedPoints(),job().context.geometry.scale,job().context.collision,
            job().context.swimming,job().context.floor,job().context.player,job().context.waterBake,job().context.waterCrossings,job().context.events);meshContext=job().context;haveMesh=true;
    }
    ResultBatch batch;batch.request=job().request;
    for(unsigned i=0;i<job().targets.size() && alive();++i) {
        const auto& t=job().targets[i]; bool unreachable=false;
        if(!t.unsupported) {
            const Key key{t.actor,t.worker,t.id,t.pushControl,t.exit,t.swimDepth};
            const auto previous=checked().find(key);
            if(previous!=checked().end() && sameTarget(previous->second.target,t)) unreachable=previous->second.unreachable;
            else { unreachable=destinationUnreachable(ownedMesh(),job().context.player,entity(t));
                if(alive() && checked().size()<MaxTargets*2) checked()[key]={t,unreachable}; }
        }
        nav::checkComputation();batch.unreachable[i]=unreachable?1u:0u;batch.count=i+1;
        // First usable result is immediate; later bounded prefixes amortize
        // publication without needing one game/input event per destination.
        if(i==0 || batch.count%16==0 || batch.count==job().targets.size()) {results.publish(batch);if(const auto notify=preparedHandler.load(std::memory_order_acquire)) notify();}
    }
    if(alive()) replies.publish({job().request,4,0});
}
void consume(unsigned slot) {
    const auto& captured=snapshot(slot);
    if(captured.context.geometry.count>MaxTriangles || captured.context.geometry.vertexCount>MaxVertices || captured.count>MaxTargets) return;
    // A canceled request may still carry the resource's only ready owned copy.
    // Cache its bytes first; no engine address is ever read by the worker.
    if(captured.geometryCopied) {
        cachedRaw().assign(captured.triangles.begin(),captured.triangles.begin()+captured.context.geometry.count);
        cachedPoints().assign(captured.vertices.begin(),captured.vertices.begin()+captured.context.geometry.vertexCount);
        cachedGeometry=captured.context.geometry;haveGeometry=true;cachedGeometryReady.publish(cachedGeometry);
        haveChecks=haveMesh=false;
    }
    if(captured.request!=latest.load(std::memory_order_acquire)) return;
    if(!haveGeometry || !sameGeometry(cachedGeometry,captured.context.geometry)) {cachedGeometryReady.publish({});return;}
    job()={};job().request=captured.request;job().context=captured.context;
    job().targets.assign(captured.targets.begin(),captured.targets.begin()+captured.count);
    job().received=captured.count;job().reuse=true;
}
DWORD WINAPI worker(void*) {
    SetThreadPriority(GetCurrentThread(),THREAD_PRIORITY_BELOW_NORMAL);
    for(;;) { WaitForSingleObject(wake.load(),INFINITE);drainWorker(); }
}
}
void drainWorker() {
    try {workerStorage.prepare();checkedStorage.prepare();} // Only the dedicated worker/isolated fixture.
    catch(...) {recordLog("reachability cache initialization unavailable");return;}
    unsigned slot=NoSnapshot;
    while(captures.tryPop(slot)) {
        try {consume(slot);}
        catch(...) {job().request=0;haveGeometry=haveChecks=haveMesh=false;cachedGeometryReady.publish({});checked().clear();}
        releaseSnapshot(slot);
        if(alive()) {
            try {finish();}
            catch(const nav::ComputationCancelled&) {job().request=0;}
            catch(...) {job().request=0;haveGeometry=haveChecks=haveMesh=false;cachedGeometryReady.publish({});checked().clear();}
        }
    }
}
void wakeWorker() { if(const auto event=wake.load()) SetEvent(event); }
void startWorker() {
#ifndef FFX_READER_TEST
    HMODULE pin=nullptr;
    if(!GetModuleHandleExW(GET_MODULE_HANDLE_EX_FLAG_FROM_ADDRESS|GET_MODULE_HANDLE_EX_FLAG_PIN,reinterpret_cast<LPCWSTR>(&worker),&pin)) return;
    const auto event=CreateEventW(nullptr,FALSE,FALSE,nullptr);if(!event) return;
    // Once published, this handle stays alive for process lifetime.
    wake.store(event);
    const auto thread=CreateThread(nullptr,0,worker,nullptr,0,nullptr);
    if(thread) CloseHandle(thread);else recordLog("reachability worker unavailable error=%lu",GetLastError());
#endif
}
}
namespace ffx {void onReachabilityPrepared(ReachabilityPrepared handler) {reach::preparedHandler.store(handler,std::memory_order_release);}}
#ifdef FFX_READER_TEST
namespace ffx { void drainReachabilityForTest() { reach::drainWorker(); } }
#endif
