// Actual approved City/Nucleus floors through the production owned decoder and
// route solver. Native collision83E4E0 is executed from external retained bytes;
// controlled globals replace only actor ownership and the16 mutable remaps.
#include "external_fixture.hpp"
#include "../owned_mesh_decoder.hpp"
#include "collision_replay_bytes.inc"
#include <cstdio>
#include <fstream>
#include <cstring>
#include <queue>
#include <chrono>
using namespace ffx;
namespace {
void check(bool ok,const char* message) {if(!ok) {std::fprintf(stderr,"FAIL: %s\n",message);std::exit(1);}}
uintptr_t nativeTriangles=0;
std::array<unsigned,65> nativeRemaps{};
std::array<uint8_t,0x880> controlledActor{};
uintptr_t __cdecl controlled() {return reinterpret_cast<uintptr_t>(controlledActor.data());}
using Native=unsigned(__cdecl*)(uintptr_t,int);
Native originalCollision() {
    auto* code=static_cast<uint8_t*>(VirtualAlloc(nullptr,4096,MEM_COMMIT|MEM_RESERVE,PAGE_EXECUTE_READWRITE));
    check(code!=nullptr,"isolated original collision allocation");std::memcpy(code,collisionReplayBody,sizeof(collisionReplayBody));
    for(const auto& fix:collisionReplayFixups) {
        uintptr_t target=0;
        if(fix.va>=0x83e4e0 && fix.va<0x83e5b3) target=reinterpret_cast<uintptr_t>(code)+fix.va-0x83e4e0;
        else if(fix.va==0x1301a44) target=reinterpret_cast<uintptr_t>(&nativeTriangles);
        else if(fix.va==0x1301998) target=reinterpret_cast<uintptr_t>(nativeRemaps.data());
        else if(fix.va==0x82d8e0) target=reinterpret_cast<uintptr_t>(controlled);
        check(target!=0,"no unresolved native collision dependency");
        const auto patched=target-(fix.relative?reinterpret_cast<uintptr_t>(code)+fix.offset+4:0);std::memcpy(code+fix.offset,&patched,4);
    }
    FlushInstructionCache(GetCurrentProcess(),code,sizeof(collisionReplayBody));return reinterpret_cast<Native>(code);
}
struct Exit {unsigned worker=0;InteractionZone zone;};
struct Goal {unsigned worker=0;Point position;};
struct Facts {float scale=0;std::vector<Point> entrances;std::vector<Exit> exits,controls;std::vector<Goal> chests;};
Facts readFacts(const char* stem) {
    Facts f;std::ifstream input(ffx_fixture::evidenceRoot()/"offline-output/sin_later_maps_20261020"/(std::string(stem)+"_facts_v2.txt"));
    unsigned count=0;input>>f.scale>>count;check(count>0 && count<=8,"bounded original entrance table");f.entrances.resize(count);
    for(auto& p:f.entrances) input>>p.x>>p.y>>p.z;
    input>>count;check(count<=3,"bounded original exit table");f.exits.resize(count);
    for(auto& e:f.exits) {auto& z=e.zone;input>>e.worker>>z.center.x>>z.center.y>>z.center.z>>z.end.x>>z.end.y>>z.end.z>>z.range>>z.height;z.valid=z.crossing=true;z.shape=2;}
    input>>count;check(count<=8,"bounded original type-only chest positions");f.chests.resize(count);
    for(auto& g:f.chests) input>>g.worker>>g.position.x>>g.position.y>>g.position.z;
    input>>count;check(count<=4,"bounded original platform stops");f.controls.resize(count);
    for(auto& e:f.controls) {auto& z=e.zone;input>>e.worker>>z.center.x>>z.center.y>>z.center.z>>z.width>>z.depth>>z.height>>z.angle;z.valid=z.crossing=true;z.shape=3;}
    check(input.good() && std::isfinite(f.scale) && f.scale>0,"ready original script/geometry facts");return f;
}
bool onTriangle(const Triangle& t,Point p,double heightLimit=8) {
    const auto& a=t.points[0];const auto& b=t.points[1];const auto& c=t.points[2];
    const double determinant=(b.z-c.z)*(a.x-c.x)+(c.x-b.x)*(a.z-c.z);
    if(std::abs(determinant)<1e-8) return false;
    const auto u=((b.z-c.z)*(p.x-c.x)+(c.x-b.x)*(p.z-c.z))/determinant;
    const auto v=((c.z-a.z)*(p.x-c.x)+(a.x-c.x)*(p.z-c.z))/determinant;
    return u>=-.00001 && v>=-.00001 && u+v<=1.00001 && std::abs(u*a.y+v*b.y+(1-u-v)*c.y-p.y)<=heightLimit;
}
bool covered(const Mesh& mesh,Point p) {
    for(const auto& t:mesh.triangles) if(t.valid && !t.collision && onTriangle(t,p)) return true;
    return false;
}
unsigned verifySegments(const Mesh& mesh,const std::vector<Point>& route) {
    unsigned samples=0;
    for(size_t i=1;i<route.size();++i) {
        const auto a=route[i-1],b=route[i];const unsigned steps=static_cast<unsigned>(std::ceil(distanceXZ(a,b)))+1;
        for(unsigned j=0;j<=steps;++j) {
            const double ratio=double(j)/steps;
            const Point p{a.x+(b.x-a.x)*ratio,a.y+(b.y-a.y)*ratio,a.z+(b.z-a.z)*ratio};
            if(!covered(mesh,p)) {
                std::fprintf(stderr,"UNCOVERED [%.8f,%.8f,%.8f] leg%zu/%zu\n",p.x,p.y,p.z,i,route.size());return 0;
            }
            ++samples;
        }
    }
    return samples;
}
double length(const std::vector<Point>& points) {double sum=0;for(size_t i=1;i<points.size();++i) sum+=distanceXZ(points[i-1],points[i]);return sum;}
template<size_t T,size_t V> void inspect(const char* stem,const ffx_fixture::Bytes<T>& tb,const ffx_fixture::Bytes<V>& vb,Native native) {
    std::vector<RawNavigationTriangle> triangles(T/16);std::vector<RawNavigationVertex> vertices(V/8);
    std::memcpy(triangles.data(),tb.bytes,T);std::memcpy(vertices.data(),vb.bytes,V);nativeTriangles=reinterpret_cast<uintptr_t>(triangles.data());
    const auto facts=readFacts(stem);const bool city=std::strcmp(stem,"city")==0;unsigned comparisons=0,routes=0,unavailable=0,samples=0;
    std::printf("{\"area\":\"%s\",\"cases\":[",stem);
    for(unsigned state=0;state<(city?64u:1u);++state) {
        nav::CollisionSnapshot snapshot;snapshot.actorIsControlled=true;
        for(unsigned i=0;i<16;++i) nativeRemaps[48+i]=snapshot.remaps[i]=i<6 && (state&(1u<<i))?1u:0u;
        const auto mesh=decodeOwnedNavigationMesh(triangles,vertices,facts.scale,snapshot,false,-1,facts.entrances[0]);
        for(unsigned i=0;i<triangles.size();++i) {check((native(controlled(),i)!=0)==(mesh.triangles[i].collision!=0),"real-floor decode agrees with original native collision predicate");++comparisons;}
        if(state) std::printf(",");std::printf("{\"closed_mask\":%u,\"exits\":[",state);
        bool comma=false;
        for(const auto& exit:facts.exits) {
            const auto points=interactionRoute(mesh,facts.entrances[0],exit.zone);
            check(city?!points.empty():points.empty(),"City physical exits stay reachable; off-floor Nucleus script exits stay unavailable");
            if(!points.empty()) {const auto checked=verifySegments(mesh,points);check(checked>0,"smoothed exit route stays on original passable floor");samples+=checked;++routes;}
            else ++unavailable;
            if(comma) std::printf(",");comma=true;
            std::printf("{\"worker\":%u,\"points\":%zu,\"length\":%.6f,\"route\":[",exit.worker,points.size(),length(points));
            for(size_t i=0;i<points.size();++i) {if(i)std::printf(",");std::printf("[%.9f,%.9f,%.9f]",points[i].x,points[i].y,points[i].z);}std::printf("]}");
        }
        std::printf("]}");
        if(!state) for(const auto& goal:facts.chests) {
            const auto points=routePoints(mesh,facts.entrances[0],goal.position);
            std::fprintf(stderr,"FACT City chest W%u all-open points%zu length%.3f\n",goal.worker,points.size(),length(points));
        }
        if(!state) for(const auto& platform:facts.controls) {
            bool reachable=false;
            std::vector<Point> starts=facts.entrances;for(const auto& chest:facts.chests) starts.push_back(chest.position);
            for(auto start:starts) {const auto points=interactionRoute(mesh,start,platform.zone);if(points.empty()) continue;
                check(inInteractionZone(platform.zone,points.back()),"real floor endpoint strictly enters native boarding rectangle");
                check(verifySegments(mesh,points)>0,"boarding approach stays on original floor");reachable=true;break;}
            check(reachable,"each platform stop has a route from its actual floor component");
        }
        if(!city) {
            unsigned inside=0;
            // Include both possible integer endpoints around the original
            // GetRandomInRange(200)-100 candidates; this is coverage, not RNG.
            for(int x=-100;x<=100;++x) for(int z=-100;z<=100;++z) if(covered(mesh,{double(x),0,double(z)})) ++inside;
            std::fprintf(stderr,"FACT Nucleus original candidate envelope: %u/40401 integer points covered.\n",inside);
            check(inside==40401,"entire original pickup coordinate envelope lies on native floor");
            for(unsigned i=0;i<mesh.triangles.size();++i) {
                const auto points=routePoints(mesh,facts.entrances[0],mesh.triangles[i].center);check(!points.empty(),"Nucleus entire native floor connected to entrance");
                const auto checked=verifySegments(mesh,points);check(checked>0,"Nucleus route samples remain on original floor");samples+=checked;
            }
        }
    }
    std::printf("]}");
    std::fprintf(stderr,"PASS %s: original native predicate%u comparisons, exit routes%u/unavailable%u, floor samples%u. Controlled remaps/owner; no full VM/gameplay.\n",stem,comparisons,routes,unavailable,samples);
}
}
int main() {
    const auto before=std::chrono::steady_clock::now();const auto native=originalCollision();
    std::printf("{\"areas\":[");
    inspect("city",ffx_fixture::load<14016>("offline-output/sin_later_maps_20261020/city_triangles.bin",0xc930bffad5ce62cdull),
        ffx_fixture::load<6600>("offline-output/sin_later_maps_20261020/city_vertices.bin",0x7c3bf41cdb4ce448ull),native);
    std::printf(",");
    inspect("nucleus",ffx_fixture::load<2336>("offline-output/sin_later_maps_20261020/nucleus_triangles.bin",0x92f86e16ebb79e20ull),
        ffx_fixture::load<728>("offline-output/sin_later_maps_20261020/nucleus_vertices.bin",0x92cf4e93b2c52727ull),native);
    std::printf("]}\n");
    std::fprintf(stderr,"METRIC both real-floor/64-state native route audits %.3fms; offline batch only.\n",std::chrono::duration<double,std::milli>(std::chrono::steady_clock::now()-before).count());
}
