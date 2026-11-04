#include <algorithm>
#include "../atel.hpp"
#include "../navigation.hpp"
#include "../navigation_cancel.hpp"
#include "visibility_reference.hpp"
#include <cstdio>
#include <cstdlib>
#include <stdexcept>
using namespace ffx;
void check(bool value, const char* message) {
    if (!value) { std::fprintf(stderr,"FAIL: %s\n",message); std::exit(1); }
}
void op(AtelScript& s, unsigned code, unsigned arg=0) {
    s.code.push_back(static_cast<uint8_t>(code));
    if (code&0x80) { s.code.push_back(static_cast<uint8_t>(arg)); s.code.push_back(static_cast<uint8_t>(arg>>8)); }
}
AtelScript basic() { AtelScript s; s.workers.push_back({0,{0},{},{}}); return s; }
Triangle triangle(Point a, Point b, Point c) {
    Triangle t; t.valid=true; t.points[0]=a; t.points[1]=b; t.points[2]=c;
    t.center={(a.x+b.x+c.x)/3,(a.y+b.y+c.y)/3,(a.z+b.z+c.z)/3};
    for(auto& n:t.neighbours)n=-1;
    return t;
}
void visibilityFixtures() {
    const auto parity=[](const Mesh& mesh,Point a,Point b) {
        const std::vector<Point> points{a,{(a.x+b.x)/2,(a.y+b.y)/2,(a.z+b.z)/2},b};
        const auto old=visibility_reference::smooth(mesh,points),current=smooth(mesh,points);
        check(old.size()==current.size(),"indexed visibility retains original shortcut decision");
        for(size_t i=0;i<old.size();++i)
            check(old[i].x==current[i].x && old[i].y==current[i].y && old[i].z==current[i].z,"indexed visibility retains exact route coordinates");
    };
    // Parallel edges exercise the original barycentric epsilon just outside
    // the polygon; candidate boxes must not turn that tolerance into a wall.
    for(double scale:{1.0,10000.0,1000000.0}) {
        Mesh edge;edge.triangles={triangle({0,0,0},{scale,0,0},{0,0,1})};
        parity(edge,{scale*.1,0,-.0000005},{scale*.8,0,-.0000005});
        parity(edge,{scale*.1,0,-.000002},{scale*.8,0,-.000002});
        parity(edge,{scale*.1,8,0},{scale*.8,8,0});
        parity(edge,{scale*.1,8.000001,0},{scale*.8,8.000001,0});
        edge.fixedHeight=true;parity(edge,{scale*.1,900,0},{scale*.8,900,0});
    }
    Mesh floors;
    for(unsigned i=0;i<32;++i) {
        // Disjoint strips plus overlapping high floors exercise heap entry
        // order, true narrow gaps, early coverage and native height rejection.
        const double x=i*12.0;
        for(double y:{0.0,100.0}) {
            floors.triangles.push_back(triangle({x,y,0},{x+10,y,0},{x,y,10}));
            floors.triangles.push_back(triangle({x+10,y,0},{x+10,y,10},{x,y,10}));
        }
    }
    unsigned seed=12419;
    for(unsigned i=0;i<256;++i) {
        seed=seed*1664525u+1013904223u;const auto a=floors.triangles[seed%floors.triangles.size()].center;
        seed=seed*1664525u+1013904223u;const auto b=floors.triangles[seed%floors.triangles.size()].center;
        parity(floors,a,b);floors.fixedHeight=!floors.fixedHeight;parity(floors,a,b);
    }
    // Ill-conditioned projected triangles are deliberately tested by the old
    // clipper rather than trusted to a numerically narrow hierarchy box.
    Mesh thin;thin.triangles={triangle({1000000,0,1000000},{2000000,0,2000000},{2000000,0,2000000.000001})};
    parity(thin,thin.triangles[0].points[0],thin.triangles[0].points[1]);
    floors.events.count=1;floors.events.rows[0]={{4,0,-1},{4,0,11},{6,0,5},20};
    parity(floors,{1,0,5},{9,0,5});parity(floors,{9,0,5},{1,0,5});
}
#include "navigation_zone_followups.inc"
#include "navigation_exit_followups.inc"
int main() {
    auto s=basic(); op(s,0xae,123); op(s,0xae,4); op(s,0xb5,0x11); op(s,0x34);
    auto r=scanWorker(s,0); check(r.exit&&r.room==123&&r.spawn==4,"direct warp");
    s=basic(); s.workers[0].ints={456,2}; op(s,0xad,0); op(s,0xad,1); op(s,0xd8,0x10b);
    r=scanWorker(s,0); check(r.room==456&&r.spawn==2,"worker-local integer table");
    s=basic(); op(s,0xae,123); op(s,0xae,4); op(s,0xb5,99); op(s,0xb5,0x11);
    r=scanWorker(s,0); check(r.exit&&r.room==UnknownLiteral,"unrelated call consumes literals");
    s=basic(); op(s,0xae,123); op(s,0xae,4); op(s,0x12); op(s,0xb5,0x11);
    r=scanWorker(s,0); check(r.room==UnknownLiteral,"expression cannot become destination");
    s=basic(); op(s,0xae,123); op(s,0xb5,0x11);
    r=scanWorker(s,0); check(r.exit&&r.room==UnknownLiteral,"missing spawn remains unresolved");
    s=basic(); op(s,0xb3,1); op(s,0x34);
    s.workers.push_back({0,{4},{7},{}}); op(s,0xb0,0);
    op(s,0xae,789); op(s,0xae,6); op(s,0xb5,0x11); op(s,0x34);
    r=scanWorker(s,0); check(r.room==789&&r.spawn==6&&r.warps[0].worker==1,"JSR uses callee jump table");
    s=basic(); s.workers[0].jumps={13}; op(s,0xb1,0);
    op(s,0xae,123); op(s,0xae,1); op(s,0xb5,0x11); op(s,0x34);
    op(s,0xae,456); op(s,0xae,1); op(s,0xb5,0x11); op(s,0x34);
    r=scanWorker(s,0); check(r.warps.size()==2&&r.room==UnknownLiteral,"conflicting branch destinations");
    s=basic(); s.workers[0].jumps={0}; op(s,0xb0,0);
    r=scanWorker(s,0); check(!r.exit&&!r.truncated&&r.steps<5,"backward jump terminates");
    s=basic(); op(s,0xb0,10);
    r=scanWorker(s,0); check(r.unresolved,"out-of-range jump fails closed");
    s=basic(); op(s,0xae,0); op(s,0xae,7); op(s,0xae,9); op(s,0x36);
    r=scanWorker(s,0); check(r.request&&r.targets.size()==1&&r.targets[0].worker==7&&r.targets[0].fn==9,"request target ownership");
    s=basic(); s.code={0xae,1}; bool threw=false;
    try{scanWorker(s,0);}catch(const std::runtime_error&){threw=true;}
    check(threw,"truncated bytecode rejected");
    s=basic();op(s,0xae,0);op(s,0xae,7);op(s,0xae,9);op(s,0x36);op(s,0x34);
    s.workers[0].entries={0,0,11};op(s,0x34);
    const std::vector<unsigned> playerEntries{2};
    r=scanWorker(s,0,&playerEntries);
    check(!r.request&&!r.interactive,"ambient initialization request is not a player interaction");
    s=basic();op(s,0xae,0);op(s,0xae,1);op(s,0xb5,0x64);op(s,0x34);
    r=scanWorker(s,0);check(r.interactive,"native text outcome identifies a useful interaction");
    Mesh mesh;
    mesh.triangles.push_back(triangle({0,0,0},{20,0,0},{0,0,20}));
    mesh.triangles.push_back({}); // Invalid native index must remain in place.
    mesh.triangles.push_back(triangle({20,0,0},{20,0,20},{0,0,20}));
    mesh.triangles[0].neighbours[0]=2; mesh.triangles[2].neighbours[0]=0;
    check(findTriangle(mesh,{2,0,2})==0&&findTriangle(mesh,{18,0,18})==2,"native triangle indices retained");
    check(path(mesh,0,2)==std::vector<int>({0,2}),"native adjacency path");
    check(nearWalkable(mesh,{10,0,10})&&!nearWalkable(mesh,{100,0,100}),"walkable actor proximity");
    std::vector<Point> points{{2,0,2},{10,0,10},{18,0,18}};
    check(smooth(mesh,points).size()==2,"visible route smoothing");
    mesh.triangles[2].collision=1;
    check(path(mesh,0,2).empty()&&!nearWalkable(mesh,{18,0,18}),"blocked triangle excluded");
    check(path(mesh,2,2).empty(),"blocked floor cannot produce an already-here route");
    check(smooth(mesh,points).size()==3,"blocked shortcut preserved");
    check(distanceXZ({0,100,0},{3,0,4})==5,"XZ distance ignores height");
    Mesh levels; levels.triangles={triangle({0,0,0},{20,0,0},{0,0,20}),triangle({0,100,0},{20,100,0},{0,100,20})};
    check(findTriangle(levels,{2,100,2})==1&&findTriangle(levels,{2,0,2})==0,"overlapping platforms select their actual floor");
    check(findTriangle(levels,{2,50,2})==-1,"rail gap does not invent a walking floor");
    check(path(levels,0,1).empty(),"disconnected platforms require a native ride");
    for(unsigned i=0;i<3;++i) mesh.triangles[0].vertices[i]=i;
    mesh.triangles[2].vertices[0]=1; mesh.triangles[2].vertices[1]=3; mesh.triangles[2].vertices[2]=2;
    mesh.triangles[2].collision=0;
    const auto route=routePoints(mesh,{2,0,2},{18,0,18});
    check(route.size()==2,"shared native vertex portal produces a continuous route");
    const auto outside=routePoints(mesh,{2,0,2},{23,0,18});
    check(!outside.empty() && outside.back().x==20,"event target ends at actual floor boundary");
    Mesh large; large.triangles={triangle({0,0,0},{1000,0,0},{0,0,1000})};
    check(findTriangle(large,{-2,0,500})==0 && findTriangle(large,{-100,0,500})==-1,"bounded nearest-edge search works on large triangles");
    Mesh gap; gap.triangles={triangle({0,0,0},{8.45,0,0},{0,0,10}),triangle({8.45,0,0},{8.45,0,10},{0,0,10}),
        triangle({8.55,0,0},{20,0,0},{8.55,0,10}),triangle({20,0,0},{20,0,10},{8.55,0,10})};
    check(smooth(gap,{{1,0,1},{5,0,1},{19,0,1}}).size()==3,"continuous route check detects narrow gaps between sample points");
    check(nearbyWalls(large,{2,0,2},{1,0,0},{0,0,-1},3)==12,"wall rays distinguish camera right and behind boundaries");
    std::atomic<unsigned> revision{2};
    unsigned cancelled=0;
    {
        nav::ComputationScope computation(revision,1);
        const auto rejects=[&](auto compute) {
            try {compute();} catch(const nav::ComputationCancelled&) {++cancelled;}
        };
        rejects([&] {findTriangle(large,{900,0,900});});
        rejects([&] {path(mesh,0,2);});
        rejects([&] {smooth(gap,{{1,0,1},{5,0,1},{19,0,1}});});
        rejects([&] {nearbyWalls(large,{2,0,2},{1,0,0},{0,0,-1},3);});
        InteractionZone cancelZone;cancelZone.valid=true;cancelZone.shape=1;
        cancelZone.center={18,0,18};cancelZone.range=1;cancelZone.height=-1;
        rejects([&] {interactionRoute(mesh,{2,0,2},cancelZone);});
        {
            nav::ComputationScope nested(revision,2);
            check(findTriangle(large,{2,0,2})==0,"nested current request remains usable");
        }
        rejects([&] {findTriangle(large,{2,0,2});});
    }
    check(cancelled==6 && findTriangle(large,{2,0,2})==0,"canceled copied computations stop and restore prior TLS predicate");
    zoneFixtures();
    exitValidationFixtures();
    visibilityFixtures();
    std::puts("PASS: native ATEL ownership, availability, counter zones, crossing exits, navmesh indices, A* and smoothing fixtures.");
}
