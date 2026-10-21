#include "../owned_mesh_decoder.hpp"
#include <cassert>
#include <limits>

using namespace ffx;

static RawNavigationTriangle triangle(int a,int b,int c,int n0=-1,int n1=-1,int n2=-1,unsigned group=0) {
    RawNavigationTriangle t{};
    t.vertices[0]=static_cast<std::int16_t>(a); t.vertices[1]=static_cast<std::int16_t>(b); t.vertices[2]=static_cast<std::int16_t>(c);
    t.neighbours[0]=static_cast<std::int16_t>(n0); t.neighbours[1]=static_cast<std::int16_t>(n1); t.neighbours[2]=static_cast<std::int16_t>(n2); t.collision=group; return t;
}

int main() {
    std::vector<RawNavigationVertex> vertices(9);
    const std::int16_t xyz[9][3]={{0,10,0},{30,20,0},{0,30,30},{0,110,0},{30,120,0},{0,130,30},{0,210,0},{30,220,0},{0,230,30}};
    for(int i=0;i<9;++i) for(int j=0;j<3;++j) vertices[i].coordinates[j]=xyz[i][j];
    std::vector<RawNavigationTriangle> raw{triangle(0,1,2,1,-1,-1,48),triangle(3,4,5,0),triangle(6,7,8)};
    nav::CollisionSnapshot collision{}; collision.remaps[0]=1;
    auto mesh=decodeOwnedNavigationMesh(raw,vertices,10.0f,collision,false,1,{4,5,6});
    assert(mesh.triangles.size()==3 && mesh.triangles[0].valid);
    assert(mesh.triangles[0].vertices[1]==1 && mesh.triangles[0].neighbours[0]==1);
    assert(mesh.triangles[0].collisionGroup==48 && mesh.triangles[0].collision==1);
    assert(mesh.triangles[0].points[1].x==3 && mesh.triangles[0].points[1].y==2);
    assert(mesh.triangles[0].center.x==1 && mesh.triangles[0].center.y==2);
    collision.remaps[0]=0; raw[0].collision=48;
    auto aliasOpen=decodeOwnedNavigationMesh(raw,vertices,10.0f,collision,false,-1,{1,2,3});
    assert(aliasOpen.triangles[0].collision==0 && aliasOpen.playerTriangle==-1 && aliasOpen.player.y==2);
    auto bad=raw; bad[0].vertices[1]=-1;
    assert(!decodeOwnedNavigationMesh(bad,vertices,10.0f,collision,false,0,{}).triangles[0].valid);
    assert(decodeOwnedNavigationMesh(raw,vertices,std::numeric_limits<float>::quiet_NaN(),collision,false,0,{}).triangles.empty());
    assert(decodeOwnedNavigationMesh(raw,vertices,0.0f,collision,false,0,{}).triangles.empty());
    std::vector<RawNavigationTriangle> tooMany(100001);
    assert(decodeOwnedNavigationMesh(tooMany,vertices,1.0f,collision,false,0,{}).triangles.empty());
    std::vector<RawNavigationVertex> tooManyVertices(32769);
    assert(decodeOwnedNavigationMesh(raw,tooManyVertices,1.0f,collision,false,0,{}).triangles.empty());

    raw={triangle(0,1,2,1),triangle(3,4,5,0,-1,-1,2),triangle(6,7,8)};
    auto swim=decodeOwnedNavigationMesh(raw,vertices,10.0f,collision,true,0,{1,77,2});
    assert(swim.swimmingFloor && swim.fixedHeight && swim.playerTriangle==0 && swim.planeY==77);
    assert(swim.triangles[0].valid && !swim.triangles[1].valid && !swim.triangles[2].valid);
    assert(swim.floorY[0][0]==1 && swim.floorY[1][0]==11 && swim.triangles[1].points[0].y==77);
    assert(decodeOwnedNavigationMesh(raw,vertices,10.0f,collision,true,8,{1,77,2}).triangles.empty());
    std::atomic<unsigned> revision{2};
    bool cancelled=false;
    {
        nav::ComputationScope computation(revision,1);
        try {decodeOwnedNavigationMesh(raw,vertices,10.0f,collision,false,0,{});}
        catch(const nav::ComputationCancelled&) {cancelled=true;}
    }
    assert(cancelled);
    auto plane=decodeOwnedNavigationMesh({},vertices,1.0f,collision,true,-1,{1,4,2});
    assert(plane.openPlane && plane.fixedHeight && plane.planeY==4);
    auto badPlane=decodeOwnedNavigationMesh({},vertices,1.0f,collision,true,-1,{1,std::numeric_limits<double>::quiet_NaN(),2});
    assert(!badPlane.openPlane);
}
