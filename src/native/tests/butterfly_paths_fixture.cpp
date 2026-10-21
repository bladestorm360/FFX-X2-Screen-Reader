#include "../butterfly_paths.hpp"
#include <cmath>
#include <chrono>
#include <cstdio>
#include <cstdlib>
#include <limits>
using namespace ffx;
namespace {
void check(bool v,const char* message) { if(!v) { std::fprintf(stderr,"FAIL: %s\n",message); std::exit(1); } }
Entity body(Point p,double radius) {
    Entity e; e.zone.valid=true; e.zone.shape=1; e.zone.center=p; e.zone.range=radius;
    e.zone.contactMinY=-1; e.zone.contactMaxY=1; return e;
}
void rectangle(Mesh& m,double x,double z,double width,double depth) {
    const int v=static_cast<int>(m.triangles.size()*3),n=static_cast<int>(m.triangles.size());
    Triangle a,b; a.valid=b.valid=true;
    for(unsigned j=0;j<3;++j) a.neighbours[j]=b.neighbours[j]=-1;
    a.points[0]={x,0,z}; a.points[1]={x+width,0,z}; a.points[2]={x,0,z+depth};
    b.points[0]=a.points[1]; b.points[1]={x+width,0,z+depth}; b.points[2]=a.points[2];
    a.vertices[0]=v; a.vertices[1]=b.vertices[0]=v+1; a.vertices[2]=b.vertices[2]=v+2; b.vertices[1]=v+3;
    a.neighbours[0]=n+1; b.neighbours[0]=n;
    for(auto* t:{&a,&b}) for(auto p:t->points) { t->center.x+=p.x/3; t->center.z+=p.z/3; }
    m.triangles.push_back(a); m.triangles.push_back(b);
}
// Weld matching corners and establish only genuinely shared native edges.
void connect(Mesh& m) {
    int id=0;
    for(size_t i=0;i<m.triangles.size();++i) for(unsigned a=0;a<3;++a) {
        auto& t=m.triangles[i]; t.vertices[a]=id++;
        for(size_t j=0;j<i;++j) for(unsigned b=0;b<3;++b) {
            const auto p=t.points[a],q=m.triangles[j].points[b];
            if(p.x==q.x && p.y==q.y && p.z==q.z) t.vertices[a]=m.triangles[j].vertices[b];
        }
    }
    for(auto& t:m.triangles) for(auto& n:t.neighbours) n=-1;
    for(size_t i=0;i<m.triangles.size();++i) for(size_t j=0;j<m.triangles.size();++j) if(i!=j) {
        unsigned shared=0;
        for(int a:m.triangles[i].vertices) for(int b:m.triangles[j].vertices) if(a==b) ++shared;
        if(shared==2) for(auto& n:m.triangles[i].neighbours) if(n==-1) { n=static_cast<int>(j); break; }
    }
}
}
int main() {
    auto red=body({0,0,0},2);
    check(!butterflyRouteClear({{-10,0,0},{10,0,0}},{red}),"finite edge crosses red body");
    check(butterflyRouteClear({{3,0,0},{10,0,0}},{red}),"line extension is not finite segment");
    check(!butterflyRouteClear({{-10,0,2},{10,0,2}},{red}),"tangent rejected");
    check(!butterflyRouteClear({{0,0,0},{10,0,0}},{red}),"start in red rejected");
    check(!butterflyRouteClear({{-10,0,0},{0,0,0}},{red}),"goal in red rejected");
    check(butterflyRouteClear({{-10,3,0},{10,3,0}},{red}),"separate native height interval");
    check(!butterflyRouteClear({{-10,10,0},{10,-10,0}},{red}),"sloped finite edge enters Y and XZ together");
    check(butterflyRouteClear({{-10,0,0},{10,20,0}},{red}),"clip Y before minimizing XZ");
    red.zone.valid=false;
    check(!butterflyRouteClear({{-10,0,0},{10,0,0}},{red}),"unknown live hazard fails closed");
    red.inactive=true;
    check(butterflyRouteClear({{-10,0,0},{10,0,0}},{red}),"disabled live hazard ignored");
    red=body({50,0,50},10);
    Mesh square; rectangle(square,0,0,100,100);
    const Point origin{10,0,50}; auto blue=body({90,0,50},4);
    const auto route=butterflyRoute(square,origin,blue,{red});
    check(!route.empty() && butterflyInTarget(blue,route.back()),"red detour reaches blue native body");
    check(butterflyRouteClear(route,{red}),"all detour finite edges clear");
    auto moved=red; moved.zone.center=route.back();
    check(!butterflyRouteClear(route,{moved}),"fresh hazard delta invalidates stale route");
    check(butterflyRoute({},origin,blue,{red}).empty(),"missing floor geometry fails closed");
    auto malformed=square; malformed.triangles[0].valid=false; malformed.triangles[1].valid=false;
    check(butterflyRoute(malformed,origin,blue,{}).empty(),"invalid mesh fails closed");
    auto disconnected=square;
    for(auto& t:disconnected.triangles) for(auto& n:t.neighbours) n=-1;
    check(butterflyRoute(disconnected,origin,blue,{}).empty(),"touching but disconnected floors cannot bridge");
    check(!butterflyRoute(square,{10,0.000001,50},blue,{}).empty(),"native float floor rounding accepted");
    check(butterflyRoute(square,{10,10,50},blue,{}).empty(),"separate plane cannot become floor route");
    check(butterflyRoute(square,origin,blue,{body(blue.zone.center,50)}).empty(),"hazard covering every goal fails closed");
    Mesh large; rectangle(large,0,0,1000000,1000);
    check(!butterflyRoute(large,{10,0,500},body({999990,0,500},4),{}).empty(),"no arbitrary path distance cap");
    Mesh u;
    for(unsigned x=0;x<3;++x) rectangle(u,x*100,0,100,100);
    rectangle(u,0,100,100,100); rectangle(u,200,100,100,100); connect(u);
    const auto bent=butterflyRoute(u,{50,0,180},body({250,0,180},4),{});
    check(!bent.empty(),"connected U floor path exists");
    bool wentAround=false;
    for(auto p:bent) if(p.z<=100) wentAround=true;
    check(wentAround,"U mesh route does not shortcut missing floor");
    blue.zone.range=std::numeric_limits<double>::quiet_NaN();
    check(butterflyRoute(square,origin,blue,{}).empty(),"malformed blue geometry fails closed");
    Mesh many;
    for(unsigned x=0;x<12;++x) for(unsigned z=0;z<12;++z) rectangle(many,x*100,z*100,100,100);
    connect(many);
    std::vector<Entity> hazards;
    for(unsigned i=0;i<7;++i) hazards.push_back(body({150.+i*140,0,150.+i*140},25));
    const auto before=std::chrono::steady_clock::now();
    const auto longer=butterflyRoute(many,{10,0,10},body({1190,0,1190},4),hazards);
    const auto elapsed=std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::steady_clock::now()-before).count();
    check(!longer.empty() && butterflyRouteClear(longer,hazards),"288-triangle seven-hazard synthetic path");
    std::printf("Synthetic routing timing: 288 triangles, 7 hazards, %lld ms\n",static_cast<long long>(elapsed));
    std::puts("PASS: butterfly finite hazards, live deltas, mesh connectivity and large routes");
}
