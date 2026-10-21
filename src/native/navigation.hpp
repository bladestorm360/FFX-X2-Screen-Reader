#pragma once
#include "events.hpp"
#include "nav_entries.hpp"
#include <type_traits>
#include "atel_resources.hpp"

namespace ffx {
namespace nav {struct CrossWaterBake;}
struct Point { double x = 0, y = 0, z = 0; };
// Copied finite scripted crossings, not extra floor or injected movement.
// landing identifies the native movement's permitted side of each line.
struct DirectedCrossing {Point a,b,landing;double height=0;};
struct RouteEvents {std::array<DirectedCrossing,4> rows{};unsigned kind=0,count=0;bool ready=true;};
struct Triangle { int vertices[3]{}, neighbours[3]{}; unsigned collision = 0, collisionGroup = 0;
    Point points[3]{}, center{}; bool valid = false; };
struct Mesh { std::vector<Triangle> triangles; bool openPlane=false,fixedHeight=false; double planeY=0;
    int playerTriangle=-1; Point player; bool swimmingFloor=false; std::vector<std::array<double,3>> floorY;
    const nav::CrossWaterBake* crossWater=nullptr;unsigned waterCrossings=0;std::vector<Triangle> terrain;RouteEvents events; };
bool swimmingPlane();
struct InteractionShape { Point center,end; double angle=0,width=0,depth=0,range=0,height=-1,viewAngle=3.141592653589793;
    unsigned shape=0; int worker=-1; bool valid=false,crossing=false,talk=false; };
struct InteractionZone : InteractionShape { std::vector<InteractionShape> competing;
    double contactRadius=0,contactMinY=-1e30,contactMaxY=1e30; Point approach;
    Point obstacle; double obstacleRadius=0; };
struct Entity { std::wstring name, category; unsigned id = 0; int actor = -1, worker = -1;
    Point position; double distance = 0; InteractionZone zone; bool unreachable=false, inactive=false; unsigned pushControl=0,pushLocal=0;
    EntryMask usefulEntries; bool authoredTrialControl=false,butterfly=false,hazardous=false,butterflyStart=false;
    unsigned butterflyEpoch=0; bool swimApproach=false,swimDepth=false,swimReady=true; double swimClearance=0; uintptr_t swimChunk=0; };
// Native steering carries copied facts, never an allocating Entity/vector/string.
// At most500 loaded workers supply competitors;512 includes authored recess facts.
struct NativeInteractionRows {
    std::array<InteractionShape,512> rows;
    unsigned count=0;
    const InteractionShape* begin() const {return rows.data();}
    const InteractionShape* end() const {return rows.data()+count;}
    const InteractionShape& operator[](size_t index) const {return rows[index];}
};
struct NativeInteractionZone : InteractionShape {
    NativeInteractionRows competing;
    double contactRadius=0,contactMinY=-1e30,contactMaxY=1e30;
    Point approach,obstacle;double obstacleRadius=0;
};
struct NativeRouteTarget {
    std::array<wchar_t,2049> name{};
    unsigned id=0,pushControl=0,pushLocal=0;int actor=-1,worker=-1;
    Point position;NativeInteractionZone zone;
    bool inactive=false,butterfly=false,swimDepth=false;uintptr_t swimChunk=0;
};
static_assert(std::is_trivially_copyable_v<NativeRouteTarget> && std::is_trivially_destructible_v<NativeRouteTarget>);
std::vector<unsigned> interactionEntries(uintptr_t worker,EntryMask entryMask,unsigned* channels=nullptr);
struct NativeInteractionFacts {unsigned channels=0;std::array<unsigned,7> entries{};};
// Native captures only the original fixed seven-channel getters/flags. Saved
// callback proofs run later over owned script tables and copied saved facts.
NativeInteractionFacts captureNativeInteractionFacts(uintptr_t worker,EntryMask entryMask={});
NativeInteractionFacts captureNativeInteractionFacts(uintptr_t worker,EntryMask,const AtelSpan& observedSpan);
InteractionZone interactionZone(uintptr_t worker,unsigned channels);
void constrainInteractionZone(InteractionZone&,uintptr_t worker);
void configurePedestalTarget(Entity&,const char* verifiedScript,unsigned localWorker);
bool pedestalRecessTarget(const Entity&,uintptr_t worker,Entity& recess);
void refreshTargetZone(Entity&,uintptr_t worker,unsigned channels);
void refreshNavigationTarget(Entity&);
double actorContactRadius(uintptr_t actor,uintptr_t player);
std::wstring pedestalInstruction(const Entity&);
bool inInteractionZone(const InteractionZone&,Point);
bool inInteractionZone(const NativeInteractionZone&,Point);
std::vector<Point> interactionRoute(const Mesh&,Point,const InteractionZone&,bool horizontalApproach=false);
void refreshNavigation();
bool requestNavigationAssistRoute();
void requestNavigationAssistInstruction();
void resetNavigationScripts();
void announceCurrentMap();
void announceMapArrival(Event&);
void finishMapArrival();
bool activeField();
bool destinationUnreachable(const Mesh&,Point,const Entity&);
Mesh readMesh();
Point playerPosition();
uintptr_t playerActor();
bool faceNavigationTarget(const Entity&);
bool faceNavigationTarget(const NativeRouteTarget&);
double distanceXZ(Point a, Point b);
int findTriangle(const Mesh& mesh, Point point);
bool nearWalkable(const Mesh& mesh, Point point, double distanceSquared = 9);
bool floorPortal(const Triangle&,const Triangle&,Point& midpoint);
// Shared graph/smoothing/consumer guard for actual native event direction.
bool routeSegmentAllowed(const Mesh&,Point from,Point to);
bool routeEdgeAllowed(const Mesh&,int from,int to,Point portal,const Point* origin=nullptr);
std::vector<int> path(const Mesh& mesh, int start, int end,bool requirePortal=false,const Point* origin=nullptr);
std::vector<Point> smooth(const Mesh& mesh, const std::vector<Point>& points);
std::wstring direction(Point delta);
bool cameraBasis(Point& forward,Point& right);
bool relativeBearing(Point delta,double& pan,double& front);
unsigned nearbyWalls(const Mesh&,Point,Point forward,Point right,double range);
std::vector<Point> routePoints(const Mesh&,Point start,Point end);
std::vector<Point> routePoints(const Mesh&,Point start,Point end,int endTriangle);
void scanExits(Point player, std::vector<Entity>& entities);
#ifdef FFX_READER_TEST
bool navigationSelectedForTest(Entity&); // Preparation-owner fixture observation only.
unsigned navigationPendingForTest();
#endif
}
