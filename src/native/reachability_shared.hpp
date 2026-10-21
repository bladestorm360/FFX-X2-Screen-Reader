#pragma once
#include "owned_mesh_decoder.hpp"
#include "navigation_capture.hpp"
#include "nonblocking_snapshots.hpp"
#include <atomic>
#include <cstring>
namespace ffx::reach {
constexpr unsigned MaxTargets=512,MaxCompetitors=32,PartBytes=8192;
using Geometry=nav::GeometryFacts;
using Context=nav::NativeGeometryContext;
struct Target {
    InteractionShape shape;
    Point position,approach,obstacle;
    double contactRadius=0,minY=0,maxY=0,obstacleRadius=0;
    std::array<InteractionShape,MaxCompetitors> competing{};
    unsigned count=0,id=0,pushControl=0;int actor=-1,worker=-1;
    bool exit=false,swimApproach=false,swimDepth=false,inactive=false,unsupported=false;
};
constexpr unsigned SnapshotSlots=4,NoSnapshot=SnapshotSlots,MaxTriangles=32767,MaxVertices=32767;
struct Snapshot {
    unsigned request=0,count=0;Context context;bool geometryCopied=false;
    ULONGLONG capturedAt=0;
    std::array<Target,MaxTargets> targets;
    std::array<RawNavigationTriangle,MaxTriangles> triangles;
    std::array<RawNavigationVertex,MaxVertices> vertices;
};
// Fixed mod-owned buffers constructed on bootstrap. Claim/copy/publish never
// waits or allocates; the preparation worker releases each consumed slot.
void prepareSnapshots();
unsigned claimSnapshot();
Snapshot& snapshot(unsigned);
void releaseSnapshot(unsigned);
extern async::AtomicSnapshot<Geometry> cachedGeometryReady;
struct Reply { unsigned request=0,mode=0,vertices=0; }; // mode 1=capture, 2=raw cache, 3=vertices ready
struct ResultBatch {unsigned request=0,count=0;std::array<unsigned,MaxTargets> unreachable{};};
extern async::TryCaptureQueue<unsigned,SnapshotSlots> captures;
extern async::AtomicSnapshot<ResultBatch> results;
extern async::AtomicSnapshot<Reply> replies;
extern std::atomic<unsigned> latest;
bool sameGeometry(const Geometry&,const Geometry&);
bool sameContext(const Context&,const Context&);
bool sameTarget(const Target&,const Target&);
void startWorker();
void wakeWorker();
void drainWorker();
}
