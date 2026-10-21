#include "wall_cues.hpp"
#include "airship_map.hpp"
#include "navigation_capture.hpp"
#include "nonblocking_snapshots.hpp"
#include "process_storage.hpp"
#include "settings.hpp"
#include <atomic>
namespace ffx {
namespace {
constexpr unsigned Slots=4,MaxRows=32767;
struct Snapshot {
    nav::NativeGeometryContext context;
    Point forward,right;double range=0;
    unsigned serial=0,cancellation=0;bool copied=false;
    std::array<RawNavigationTriangle,MaxRows> triangles;
    std::array<RawNavigationVertex,MaxRows> vertices;
};
struct Result {unsigned epoch=0,serial=0,cancellation=0,walls=0;};
ProcessStorage<std::array<Snapshot,Slots>> snapshots;
async::TryCaptureQueue<unsigned,Slots> pending;
async::AtomicSnapshot<nav::GeometryFacts> cacheReady;
async::AtomicSnapshot<Result> publishedWallResult;
std::atomic<unsigned> claimed{0},latest{0},cancellation{1},geometryRevision{1},pressure{0};
std::atomic<HANDLE> wake{nullptr};
std::atomic<bool> ownerReady{false},installed{false};
bool pressureEpisode=false; // Native owner only.
bool unavailableEpisode=false;
struct WorkerState {
    std::vector<RawNavigationTriangle> raw;std::vector<RawNavigationVertex> vertices;
    nav::GeometryFacts geometry;nav::NativeGeometryContext context;Mesh mesh;
    bool haveRaw=false,haveMesh=false;
};
ProcessStorage<WorkerState> workerState;
bool sameGeometry(const nav::GeometryFacts& a,const nav::GeometryFacts& b) {
    return a.epoch==b.epoch && a.revision==b.revision && a.count==b.count && a.vertexCount==b.vertexCount &&
        a.root==b.root && a.triangles==b.triangles && a.vertices==b.vertices && a.scale==b.scale;
}
unsigned claim() {
    auto observed=claimed.load(std::memory_order_relaxed);
    for(unsigned i=0;i<Slots;++i) if(!(observed&(1u<<i)) &&
        claimed.compare_exchange_strong(observed,observed|(1u<<i),std::memory_order_acquire,std::memory_order_relaxed)) return i;
    return Slots;
}
void release(unsigned slot) {claimed.fetch_and(~(1u<<slot),std::memory_order_release);}
void consume(const Snapshot& frame) {
    auto& state=workerState.get();const auto& c=frame.context;
    if(c.geometry.count>MaxRows || c.geometry.vertexCount>MaxRows) return;
    // Even an obsolete movement may contain the only cold physical copy.
    if(frame.copied) {
        state.raw.assign(frame.triangles.begin(),frame.triangles.begin()+c.geometry.count);
        state.vertices.assign(frame.vertices.begin(),frame.vertices.begin()+c.geometry.vertexCount);
        state.geometry=c.geometry;state.haveRaw=true;state.haveMesh=false;cacheReady.publish(c.geometry);
    }
    if(frame.serial!=latest.load(std::memory_order_acquire) || frame.cancellation!=cancellation.load(std::memory_order_acquire) ||
        c.geometry.epoch!=generation() || !state.haveRaw || !sameGeometry(state.geometry,c.geometry)) return;
    nav::ComputationScope scope(latest,frame.serial);
    const auto& previous=state.context;
    const bool collisionSame=previous.collision.actorFlags==c.collision.actorFlags &&
        previous.collision.actorIsControlled==c.collision.actorIsControlled && previous.collision.remaps==c.collision.remaps;
    const bool componentSame=!c.swimming || (c.floor>=0 && static_cast<size_t>(c.floor)<state.mesh.triangles.size() && state.mesh.triangles[c.floor].valid);
    if(!state.haveMesh || !collisionSame || previous.swimming!=c.swimming || previous.waterBake!=c.waterBake || previous.waterCrossings!=c.waterCrossings || !nav::sameRouteEvents(previous.events,c.events) || !componentSame) {
        state.mesh=decodeOwnedNavigationMesh(state.raw,state.vertices,c.geometry.scale,c.collision,c.swimming,c.floor,c.player,c.waterBake,c.waterCrossings,c.events);
        state.haveMesh=true;
    } else if(c.swimming && state.mesh.planeY!=c.player.y) {
        state.mesh.planeY=c.player.y;
        for(auto& triangle:state.mesh.triangles) {nav::checkComputation();for(auto& p:triangle.points) p.y=c.player.y;triangle.center.y=c.player.y;}
    }
    state.context=c;state.mesh.player=c.player;state.mesh.playerTriangle=c.floor;
    const auto walls=nearbyWalls(state.mesh,c.player,frame.forward,frame.right,frame.range);
    nav::checkComputation();
    if(frame.cancellation==cancellation.load(std::memory_order_acquire) && c.geometry.epoch==generation())
        publishedWallResult.publish({c.geometry.epoch,frame.serial,frame.cancellation,walls});
}
void drain() {
    workerState.prepare();unsigned slot=Slots;
    while(pending.tryPop(slot)) {
        try {consume(snapshots.get()[slot]);}
        catch(const nav::ComputationCancelled&) {}
        catch(...) {workerState.get().haveRaw=workerState.get().haveMesh=false;cacheReady.publish({});}
        release(slot);
    }
}
void changed(Event&) {geometryRevision.fetch_add(1,std::memory_order_acq_rel);cancelWallCues();}
#ifndef FFX_READER_TEST
DWORD WINAPI worker(void*) {
    SetThreadPriority(GetCurrentThread(),THREAD_PRIORITY_BELOW_NORMAL);
    for(;;) {WaitForSingleObject(wake.load(std::memory_order_acquire),INFINITE);drain();}
}
#endif
}
void cancelWallCues() noexcept {cancellation.fetch_add(1,std::memory_order_acq_rel);latest.fetch_add(1,std::memory_order_release);}
unsigned copiedWallCues() noexcept {
    Result copied;
    return publishedWallResult.tryRead(copied) && copied.epoch==generation() && copied.serial==latest.load(std::memory_order_acquire) &&
        copied.cancellation==cancellation.load(std::memory_order_acquire)?copied.walls:0;
}
void captureWallCues() {
    if(!nativeGameThread() || !snapshots.ready()) return;
    if(!setting(Setting::Walls) || airshipNavigationUiActive()) {cancelWallCues();return;}
    if(!ownerReady.load(std::memory_order_acquire)) {
        cancelWallCues();if(!unavailableEpisode) {unavailableEpisode=true;output(L"Wall cues are unavailable.",false,"wall-cue-unavailable");}return;
    }
    Point forward,right;nav::NativeGeometryContext context;
    if(!cameraBasis(forward,right) ||
        !nav::captureNativeGeometry(context,geometryRevision.load(std::memory_order_acquire),false)) {cancelWallCues();return;}
    const auto serial=latest.fetch_add(1,std::memory_order_acq_rel)+1;
    const auto slot=claim();
    if(slot==Slots) {
        ++pressure;if(!pressureEpisode) {pressureEpisode=true;output(L"Wall cues are busy. Move again when they recover.",false,"wall-cue-pressure");}return;
    }
    pressureEpisode=false;auto& frame=snapshots.get()[slot];frame.context=context;frame.forward=forward;frame.right=right;
    frame.range=setting(Setting::WallRange)*11.13;frame.serial=serial;frame.cancellation=cancellation.load(std::memory_order_acquire);
    nav::GeometryFacts ready;frame.copied=!cacheReady.tryRead(ready) || !sameGeometry(ready,context.geometry);
    if(frame.copied && ((context.geometry.count && !copyReadyRegion(context.geometry.triangles,frame.triangles.data(),context.geometry.count*16,context.geometry.count*16)) ||
        (context.geometry.vertexCount && !copyReadyRegion(context.geometry.vertices,frame.vertices.data(),context.geometry.vertexCount*8,context.geometry.vertexCount*8)))) {release(slot);return;}
    if(!pending.tryPush(slot)) {release(slot);++pressure;return;}
    if(const auto event=wake.load(std::memory_order_acquire)) SetEvent(event);
}
void installWallCues() {
    if(installed.exchange(true,std::memory_order_acq_rel)) return;
    snapshots.prepare();hookSnapshot(0x43ea00,changed,nullptr);
#ifndef FFX_READER_TEST
    HMODULE pin=nullptr;
    if(!GetModuleHandleExW(GET_MODULE_HANDLE_EX_FLAG_FROM_ADDRESS|GET_MODULE_HANDLE_EX_FLAG_PIN,reinterpret_cast<LPCWSTR>(&worker),&pin)) return;
    const auto event=CreateEventW(nullptr,FALSE,FALSE,nullptr);if(!event) return;wake.store(event,std::memory_order_release);
    const auto thread=CreateThread(nullptr,0,worker,nullptr,0,nullptr);if(thread) {ownerReady.store(true,std::memory_order_release);CloseHandle(thread);}else recordLog("wall cue worker unavailable error=%lu",GetLastError());
#else
    ownerReady.store(true,std::memory_order_release);
#endif
}
#ifdef FFX_READER_TEST
void drainWallCuesForTest() {drain();}
unsigned wallCuePressureForTest() {return pressure.load();}
bool wallCuesReadyForTest() {Result copied;return publishedWallResult.tryRead(copied) && copied.epoch==generation() && copied.serial==latest.load() && copied.cancellation==cancellation.load();}
void setWallCueWakeForTest(HANDLE event) {wake.store(event,std::memory_order_release);}
#endif
}
