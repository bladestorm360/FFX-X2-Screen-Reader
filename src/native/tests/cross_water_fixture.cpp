#include "external_fixture.hpp"
#include "../owned_mesh_decoder.hpp"
#include <fstream>
#include <cstdio>
#include <cstdlib>
#include <chrono>
#include <algorithm>
using namespace ffx;
namespace {
void check(bool ok,const char* message) {if(!ok){std::fprintf(stderr,"FAIL: %s\n",message);std::exit(1);}}
template<class T> std::vector<T> load(const char* name,unsigned count) {
    const auto path=(ffx_fixture::evidenceRoot()/"offline-output/gagazet_cross_water_20261016").string()+"/"+name;
    std::ifstream f(path,std::ios::binary);check(f.good(),"approved original geometry exists");std::vector<T> rows(count);
    f.read(reinterpret_cast<char*>(rows.data()),count*sizeof(T));check(f.gcount()==count*sizeof(T),"complete original geometry");return rows;
}
Entity waterExit(unsigned i) {
    const auto& c=nav::gagazetCrossWater.crossings[i];Entity e;e.category=L"Exit";e.worker=28+i;
    e.zone.shape=2;e.zone.center=c.exitA;e.zone.end=c.exitB;e.zone.range=10;e.zone.height=50;e.zone.crossing=true;e.zone.valid=true;
    e.position=c.exitA;return e;
}
bool contains(const std::vector<Point>& route,Point p) {return std::any_of(route.begin(),route.end(),[&](Point a){return distanceXZ(a,p)<.001&&std::abs(a.y-p.y)<.001;});}
}
int main() {
    const auto raw=load<RawNavigationTriangle>("mtgz06_0_triangles.bin",1406);
    const auto vertices=load<RawNavigationVertex>("mtgz06_0_vertices.bin",1039);
    constexpr float scale=18.835050964355467f;
    const Point seeds[]={{178.5891,88.16030883789062,-351.6188},{232.9568,-198.8135986328125,-130.0165},{569.6378,-127.19363403320312,-950.2076}};
    const int floors[]={45,1342,284};nav::CollisionSnapshot collision;collision.actorIsControlled=true;
    auto old=decodeOwnedNavigationMesh(raw,vertices,scale,collision,true,floors[0],seeds[0]);
    check(interactionRoute(old,seeds[0],waterExit(1).zone).empty(),"original flattened decoder reproduces first-to-second swimming failure");
    const auto started=std::chrono::steady_clock::now();unsigned routes=0;
    for(unsigned source=0;source<3;++source) for(unsigned destination=0;destination<3;++destination) {
        const auto mesh=decodeOwnedNavigationMesh(raw,vertices,scale,collision,true,floors[source],seeds[source],1,7);
        check(mesh.crossWater && mesh.terrain.size()==1406,"exact original geometry receives qualified cross-water bake");
        const auto route=interactionRoute(mesh,seeds[source],waterExit(destination).zone);
        if(route.empty()) {
            const auto& a=nav::gagazetCrossWater.crossings[source];const auto& b=nav::gagazetCrossWater.crossings[destination];
            auto land=mesh;land.crossWater=nullptr;land.triangles=mesh.terrain;land.fixedHeight=false;land.player=a.land;land.playerTriangle=a.landTriangle;
            auto water=land;water.fixedHeight=true;water.player=seeds[source];water.playerTriangle=floors[source];
            for(unsigned i=0;i<water.triangles.size();++i) {auto& t=water.triangles[i];t.valid=t.valid&&std::binary_search(a.triangles,a.triangles+a.count,static_cast<uint16_t>(i));for(auto& p:t.points)p.y=seeds[source].y;t.center.y=seeds[source].y;}
            const auto swim=routePoints(water,seeds[source],a.land,a.landTriangle);
            const auto walking=routePoints(land,a.land,b.water,b.waterTriangle);
            std::fprintf(stderr,"route source%u destination%u empty; swim%zu walk%zu topology%zu portals%zu\n",source,destination,swim.size(),walking.size(),path(land,a.landTriangle,b.waterTriangle).size(),path(land,a.landTriangle,b.waterTriangle,true).size());
            auto aw=a.water;aw.y=seeds[source].y;
            const auto first=routePoints(water,seeds[source],aw,a.waterTriangle),across=routePoints(water,aw,a.land,a.landTriangle);
            const auto middle=routePoints(land,a.land,b.land,b.landTriangle),entering=routePoints(land,b.land,b.water,b.waterTriangle);
            auto destinationWater=land;destinationWater.fixedHeight=true;destinationWater.player=b.water;destinationWater.playerTriangle=b.waterTriangle;
            for(unsigned i=0;i<destinationWater.triangles.size();++i) {auto& t=destinationWater.triangles[i];t.valid=t.valid&&std::binary_search(b.triangles,b.triangles+b.count,static_cast<uint16_t>(i));for(auto& p:t.points)p.y=b.water.y;t.center.y=b.water.y;}
            const auto last=interactionRoute(destinationWater,b.water,waterExit(destination).zone,true);
            std::fprintf(stderr,"stages first%zu across%zu middle%zu entering%zu last%zu\n",first.size(),across.size(),middle.size(),entering.size(),last.size());
            for(const auto& segment:{across,entering}) for(auto p:segment) std::fprintf(stderr,"cross %.6f %.6f %.6f\n",p.x,p.y,p.z);
        }
        check(!route.empty(),"all three native cave water sections route through their proper shorelines");++routes;
        check(!destinationUnreachable(mesh,seeds[source],waterExit(destination)),"filter agrees with actual cross-water route");
        if(source!=destination) {
            check(contains(route,nav::gagazetCrossWater.crossings[source].land),"source shoreline/land height retained in route");
            check(contains(route,nav::gagazetCrossWater.crossings[destination].water),"destination shoreline/water height retained in route");
        }
    }
    const auto ms=std::chrono::duration<double,std::milli>(std::chrono::steady_clock::now()-started).count();
    for(const auto boundary:std::array<std::pair<unsigned,int>,2>{{{0,85},{1,862}}}) {
        const auto& c=nav::gagazetCrossWater.crossings[boundary.first];
        const auto land=decodeOwnedNavigationMesh(raw,vertices,scale,collision,false,-1,{},1,7);
        const auto& t=land.triangles[boundary.second];const auto side=[&](Point p){return (c.b.x-c.a.x)*(p.z-c.a.z)-(c.b.z-c.a.z)*(p.x-c.a.x);};
        Point waterVertex=t.points[0];for(auto p:t.points) if(side(p)*side(c.water)>side(waterVertex)*side(c.water)) waterVertex=p;
        Point p{t.center.x*.2+waterVertex.x*.8,c.water.y,t.center.z*.2+waterVertex.z*.8};
        const auto edge=decodeOwnedNavigationMesh(raw,vertices,scale,collision,true,boundary.second,p,1,7);
        check(!interactionRoute(edge,p,waterExit((boundary.first+1)%3).zone).empty(),"shore-straddling native source triangle retains cross-water routes");
    }
    auto firstComplete=collision;firstComplete.remaps[1]=firstComplete.remaps[2]=firstComplete.remaps[3]=1;
    auto firstMesh=decodeOwnedNavigationMesh(raw,vertices,scale,firstComplete,true,floors[0],seeds[0],1,7);
    check(!interactionRoute(firstMesh,seeds[0],waterExit(1).zone).empty(),"first trial alone opens first-to-second water route");
    check(interactionRoute(firstMesh,seeds[0],waterExit(2).zone).empty(),"unfinished second trial still blocks the next native water route");
    check(destinationUnreachable(firstMesh,seeds[0],waterExit(2)),"filter retains the second-trial gate across mixed movement modes");
    auto closed=collision;closed.remaps[52-48]=1;
    auto mesh=decodeOwnedNavigationMesh(raw,vertices,scale,closed,true,floors[0],seeds[0],1,7);
    check(interactionRoute(mesh,seeds[0],waterExit(1).zone).empty(),"closed first-trial native gate blocks cross-water route");
    check(destinationUnreachable(mesh,seeds[0],waterExit(1)),"filter proves closed native gate instead of exposing an inaccessible crossing");
    mesh=decodeOwnedNavigationMesh(raw,vertices,scale,collision,true,floors[0],seeds[0],1,6);
    check(interactionRoute(mesh,seeds[0],waterExit(1).zone).empty(),"inactive departure shoreline cannot create a movement-mode transition");
    mesh=decodeOwnedNavigationMesh(raw,vertices,scale,collision,true,floors[0],seeds[0],1,5);
    check(interactionRoute(mesh,seeds[0],waterExit(1).zone).empty(),"inactive arrival shoreline cannot create a movement-mode transition");
    for(unsigned destination=0;destination<3;++destination) {
        const Point land={26,78,-539};
        mesh=decodeOwnedNavigationMesh(raw,vertices,scale,collision,false,-1,land,1,7);
        check(!interactionRoute(mesh,land,waterExit(destination).zone).empty(),"land-to-water route includes native entry crossing");
    }
    auto changed=raw;changed[0].collision^=1;
    check(!decodeOwnedNavigationMesh(changed,vertices,scale,collision,true,floors[0],seeds[0],1,7).crossWater,"changed native geometry refuses old shoreline bake");
    check(!decodeOwnedNavigationMesh(raw,vertices,scale,collision,true,floors[0],seeds[0],0,7).crossWater,"unverified script cannot use baked water transitions");
    std::atomic<unsigned> revision{1};bool cancelled=false;
    try {nav::ComputationScope scope(revision,1);revision=2;interactionRoute(mesh,seeds[0],waterExit(1).zone);} catch(const nav::ComputationCancelled&) {cancelled=true;}
    check(cancelled,"cross-water computation cancels stale work");
    const auto waterRaw=load<RawNavigationTriangle>("mtgz07_0_triangles.bin",594);
    const auto waterVertices=load<RawNavigationVertex>("mtgz07_0_vertices.bin",408);
    const auto separate=decodeOwnedNavigationMesh(waterRaw,waterVertices,44.097573852539064f,collision,true,0,{-72,126,-458},1,7);
    check(!separate.crossWater,"separate submerged map cannot adopt cave shoreline connections");
    std::printf("PASS: original1406-triangle Gagazet cross-water bake: %u swim/swim pairs, land/water, original failure replay, native gates, inactive shores, script/geometry guards and cancellation; %0.3fms for nine cold decodes/routes plus filter checks (offline host only).\n",routes,ms);
}
