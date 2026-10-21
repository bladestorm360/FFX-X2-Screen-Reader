// Offline investigation adapter: execute production routing over separately
// retained Sea of Sorrow collision and script facts. No game process, installed
// archive read, native pointer or game implementation is part of this host.
#include "external_fixture.hpp"
#include "../owned_mesh_decoder.hpp"
#include <cstdio>
#include <fstream>
#include <cstring>
#include <chrono>
using namespace ffx;
namespace {
void check(bool ok,const char* message) {if(!ok){std::fprintf(stderr,"FAIL: %s\n",message);std::exit(1);}}
void point(Point p) {std::printf("[%.10f,%.10f,%.10f]",p.x,p.y,p.z);}
void route(const Mesh& mesh,Point start,Point goal,const char* name,const InteractionZone* zone=nullptr) {
    const auto points=zone?interactionRoute(mesh,start,*zone):routePoints(mesh,start,goal);
    check(!points.empty(),"critical original Sin route retains a legal detour");
    for(size_t i=1;i<points.size();++i) check(routeSegmentAllowed(mesh,points[i-1],points[i]),"no returned route crosses a returning native event in reverse");
    const auto indices=path(mesh,findTriangle(mesh,start),findTriangle(mesh,goal),true);
    std::printf("{\"name\":\"%s\",\"start_triangle\":%d,\"end_triangle\":%d,\"points\":[",name,findTriangle(mesh,start),findTriangle(mesh,goal));
    for(size_t i=0;i<points.size();++i) {if(i)std::printf(",");point(points[i]);}
    std::printf("],\"native_triangle_path\":[");
    for(size_t i=0;i<indices.size();++i) std::printf("%s%d",i?",":"",indices[i]);
    std::printf("]}");
}
}
int main() {
    const auto triangleBytes=ffx_fixture::load<19520>("offline-output/sin_routes_20261020/triangles.bin",0x380693f553224c0full);
    const auto vertexBytes=ffx_fixture::load<9432>("offline-output/sin_routes_20261020/vertices.bin",0x7ca76c1d7dec7119ull);
    std::vector<RawNavigationTriangle> triangles(1220);std::vector<RawNavigationVertex> vertices(1179);
    std::memcpy(triangles.data(),triangleBytes.bytes,sizeof(triangleBytes.bytes));
    std::memcpy(vertices.data(),vertexBytes.bytes,sizeof(vertexBytes.bytes));
    std::ifstream facts(ffx_fixture::evidenceRoot()/"offline-output/sin_routes_20261020/facts.txt");
    float scale=0;Point start,sphere,a,b;
    facts>>scale>>start.x>>start.y>>start.z>>sphere.x>>sphere.y>>sphere.z>>a.x>>a.y>>a.z>>b.x>>b.y>>b.z;
    if(!facts || !std::isfinite(scale) || scale<=0) return 2;
    nav::CollisionSnapshot collision;collision.actorIsControlled=true;
    std::ifstream events(ffx_fixture::evidenceRoot()/"offline-output/sin_routes_20261020/events.txt");
    RouteEvents rules;rules.kind=1;rules.count=4;
    for(auto& e:rules.rows) events>>e.a.x>>e.a.y>>e.a.z>>e.b.x>>e.b.y>>e.b.z>>e.landing.x>>e.landing.y>>e.landing.z>>e.height;
    if(!events) return 3;
    const auto began=std::chrono::steady_clock::now();
    const auto mesh=decodeOwnedNavigationMesh(triangles,vertices,scale,collision,false,-1,start,0,0,rules);
    check(mesh.events.ready,"exact original floor accepts the script event constraints");
    const auto missing=decodeOwnedNavigationMesh({}, {},scale,collision,true,-1,start,0,0,rules);
    check(!missing.events.ready && routePoints(missing,start,sphere).empty(),"missing Sin floor cannot become a generic swimming shortcut");
    for(const auto& e:rules.rows) {
        const double x=e.b.x-e.a.x,z=e.b.z-e.a.z,length=std::hypot(x,z);
        const Point center{(e.a.x+e.b.x)/2,(e.a.y+e.b.y)/2,(e.a.z+e.b.z)/2};
        const double sign=(x*(e.landing.z-e.a.z)-z*(e.landing.x-e.a.x))>=0?1:-1;
        Point from{center.x-sign*z/length*2,center.y,center.z+sign*x/length*2},to{center.x+sign*z/length*2,center.y,center.z-sign*x/length*2};
        check(!routeSegmentAllowed(mesh,from,to) && routeSegmentAllowed(mesh,to,from),"all four ledges retain only the native landing direction");
        check(smooth(mesh,{from,to}).empty(),"two-point smoothing cannot restore a reverse shortcut");
        from.y+=100;to.y+=100;check(routeSegmentAllowed(mesh,from,to),"a separate floor does not inherit a height-limited event");
    }
    const Point end{(a.x+b.x)/2,(a.y+b.y)/2,(a.z+b.z)/2};
    InteractionZone exit;exit.valid=true;exit.crossing=true;exit.shape=2;exit.center=a;exit.end=b;exit.range=10;exit.height=10;
    std::printf("{\"routes\":[");
    route(mesh,start,sphere,"entrance-to-save-default");std::printf(",");
    route(mesh,start,end,"entrance-to-exit-line",&exit);std::printf(",");
    route(mesh,sphere,end,"save-default-to-exit-line",&exit);std::printf(",");
    route(mesh,sphere,start,"save-default-to-entrance");
    // The retained original W0 entries8/9 both return to this landing. The
    // log's rounded two-exit distances independently locate the player nearby.
    const Point landing{91.16837310791016,14.116618156433105,-40.34036636352539};
    std::printf(",");route(mesh,landing,end,"scripted-landing-to-exit-line",&exit);
    std::printf(",");route(mesh,landing,sphere,"scripted-landing-to-save-default");
    Entity target;target.category=L"Exit";target.zone=exit;target.position=end;
    check(!destinationUnreachable(mesh,landing,target),"hide-unreachable uses the same corrected native detour");
    auto disabled=rules;disabled.count=0;const auto ordinary=decodeOwnedNavigationMesh(triangles,vertices,scale,collision,false,-1,landing,0,0,disabled);
    const auto old=routePoints(ordinary,landing,sphere);check(!old.empty() && !routeSegmentAllowed(mesh,old[0],old[1]),"disabled callbacks retire the constraint and reproduce the formerly accepted shortcut");
    // Comparable same-floor/player/goal work. The old result is shorter because
    // it is invalid; these are preparation costs, never game/audible latency.
    for(unsigned mode=0;mode<2;++mode) {
        const auto& measured=mode?mesh:ordinary;double cold=0,total=0,peak=0;
        for(unsigned i=0;i<6;++i) {
            const auto before=std::chrono::steady_clock::now();
            const auto answer=interactionRoute(measured,landing,exit);check(!answer.empty(),"benchmark retains its route");
            const auto cost=std::chrono::duration<double,std::milli>(std::chrono::steady_clock::now()-before).count();
            if(!i) cold=cost;else {total+=cost;peak=std::max(peak,cost);}
        }
        std::fprintf(stderr,"METRIC same original floor/landing/exit %s: first %.3fms, warm5 mean %.3fms, max %.3fms; owned computation only.\n",mode?"directed repair":"disabled-event baseline",cold,total/5,peak);
    }
    for(unsigned subset=0;subset<16;++subset) {
        auto active=rules;active.count=0;for(unsigned i=0;i<4;++i) if(subset&(1u<<i)) active.rows[active.count++]=rules.rows[i];
        const auto variant=decodeOwnedNavigationMesh(triangles,vertices,scale,collision,false,-1,landing,0,0,active);
        const auto points=interactionRoute(variant,landing,exit);check(!points.empty(),"every active ledge subset retains an actual onward route");
        for(size_t i=1;i<points.size();++i) check(routeSegmentAllowed(variant,points[i-1],points[i]),"every live subset returns valid directed legs");
    }
    auto changed=vertices;changed[0].coordinates[0]++;
    const auto foreign=decodeOwnedNavigationMesh(triangles,changed,scale,collision,false,-1,landing,0,0,rules);
    check(!foreign.events.ready && routePoints(foreign,landing,sphere).empty() && !destinationUnreachable(foreign,landing,target),"foreign floor stays unresolved without hiding the exit or inventing a shortcut");
    std::fprintf(stderr,"PASS: real Sin floor, six critical routes, four directions, all16 event states, smoothing/reachability/geometry guards. Controlled host total %.3fms; no game/audible latency claim.\n",std::chrono::duration<double,std::milli>(std::chrono::steady_clock::now()-began).count());
    std::printf("]}\n");
}
