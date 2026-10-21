#include "navigation.hpp"
#include "camera_direction.hpp"
#include "collision_rules.hpp"
#include "navigation_events.hpp"
#include <cmath>
#include <limits>
#include <algorithm>

namespace ffx {
// The native swimming input/physics path uses motion +186. +185 and gravity
// can differ during Dive; they must not hide the active swimming component.
bool swimmingPlane() {
    const auto actor=playerActor();
    return actor && read<uint16_t>(actor+2) && read<uint8_t>(actor+0x186)==2;
}
Mesh readMesh() {
    Mesh mesh;
    // Legacy native route callers use the same bounded event facts as owned
    // scene/reachability consumers. No floor parsing is added to this capture.
    mesh.events=nav::captureRouteEvents(read<uintptr_t>(address(0xf26ae8)));
    const int count = read<int>(address(0xf01a48));
    const auto triangles = read<uintptr_t>(address(0xf01a44)), vertices = read<uintptr_t>(address(0xf01a4c));
    const auto scale = read<float>(address(0xf01a50));
    const auto root=read<uintptr_t>(address(0xf01a54));
    const int vertexCount=root?read<int16_t>(root+0xa,-1):-1;
    if(mesh.events.kind==1 && (count!=1220 || vertexCount!=1179 || !std::isfinite(scale) || std::abs(scale-27.899267578125)>.00001)) mesh.events.ready=false;
    const auto player=playerActor(); if(!player) return {};
    const bool swimming=swimmingPlane();
    if(!triangles && count==0 && swimming) {
        mesh.openPlane=true; mesh.fixedHeight=true; mesh.planeY=playerPosition().y;
        return std::isfinite(mesh.planeY)?mesh:Mesh{};
    }
    if (count <= 0 || count > 32767 || vertexCount<=0 || vertexCount>32767 || triangles < 0x10000 || vertices < 0x10000 ||
        triangles==0xcccccccc || vertices==0xcccccccc || triangles==0xcdcdcdcd || vertices==0xcdcdcdcd ||
        triangles>std::numeric_limits<uintptr_t>::max()-static_cast<unsigned>(count)*16u ||
        vertices>std::numeric_limits<uintptr_t>::max()-static_cast<unsigned>(vertexCount)*8u || !std::isfinite(scale) || scale <= 0) return mesh;
    mesh.triangles.resize(count);
    nav::CollisionSnapshot collisionState;
    collisionState.actorFlags=read<unsigned>(player+0x198);
    collisionState.actorIsControlled=player==read<uintptr_t>(address(0xf00740));
    if(!copy(address(0xf01998)+48*4,collisionState.remaps.data(),sizeof(collisionState.remaps))) return {};
    for (int i = 0; i < count; ++i) {
        auto& t = mesh.triangles[i];
        int16_t raw[6]; unsigned collision;
        if (!copy(triangles + i * 16, raw, sizeof(raw)) || !copy(triangles + i * 16 + 12, &collision, sizeof(collision))) continue;
        t.valid = true;
        t.collisionGroup=collision&127;
        // Native ChMap collision checker resolves mutable groups48..63 and
        // player-specific gates. Raw seven-bit group IDs are not passability.
        t.collision = nav::blockedForGroup(collision,collisionState)?1u:0u;
        for (unsigned j = 0; j < 3; ++j) {
            t.vertices[j] = raw[j]; t.neighbours[j] = raw[j + 3];
            int16_t coords[4];
            if (raw[j] < 0 || raw[j]>=vertexCount || !copy(vertices + raw[j] * 8, coords, sizeof(coords))) { t.valid = false; break; }
            t.points[j] = {coords[0] / static_cast<double>(scale), coords[1] / static_cast<double>(scale), coords[2] / static_cast<double>(scale)};
            t.center.x += t.points[j].x / 3; t.center.y += t.points[j].y / 3; t.center.z += t.points[j].z / 3;
        }
    }
    if(swimming) {
        mesh.swimmingFloor=true;mesh.floorY.resize(mesh.triangles.size());
        for(unsigned i=0;i<mesh.triangles.size();++i)
            for(unsigned j=0;j<3;++j) mesh.floorY[i][j]=mesh.triangles[i].points[j].y;
        // 83E5F0 advances X/Z through native adjacency independently of Y.
        // Restrict projection to the controlled actor's actual component;
        // upper decks and disconnected floors remain inaccessible.
        const int start=read<int16_t>(player+0x824,-1);
        if(start<0 || start>=count || !mesh.triangles[start].valid || !std::isfinite(playerPosition().y)) return {};
        if(start>=0 && start<count && mesh.triangles[start].valid) {
            std::vector<bool> reached(count,false); std::vector<int> pending{start}; reached[start]=true;
            while(!pending.empty()) {
                const auto at=pending.back(); pending.pop_back();
                for(int next:mesh.triangles[at].neighbours) if(next>=0 && next<count && !reached[next] &&
                    mesh.triangles[next].valid && mesh.triangles[next].collision==0) { reached[next]=true; pending.push_back(next); }
            }
            mesh.fixedHeight=true; mesh.planeY=playerPosition().y;
            for(int i=0;i<count;++i) {
                auto& t=mesh.triangles[i]; t.valid=t.valid && reached[i];
                for(auto& p:t.points) p.y=mesh.planeY;
                t.center.y=mesh.planeY;
            }
        }
    }
    // ChMapMove retains the actor's actual collision triangle at +824. Use
    // that floor at the player's position when nearby polygons overlap.
    mesh.playerTriangle=read<int16_t>(player+0x824,-1); mesh.player=playerPosition();
    return mesh;
}
uintptr_t playerActor() {
    const auto actors = read<uintptr_t>(address(0x1fc44e4));
    const auto controlled=read<uintptr_t>(address(0xf00740));
    const auto count=read<unsigned>(address(0x1fc44e0));
    if(actors && count<=128 && controlled>=actors && controlled<actors+count*0x880 && (controlled-actors)%0x880==0) return controlled;
    return actors;
}
bool activeField() {
    const auto controller=read<uintptr_t>(address(0xf26ae8));
    const auto workers=read<uint16_t>(controller+0xc);
    if(!controller || !(read<uint8_t>(controller)&2) || !workers || workers>500) return false;
    const auto actors=read<uintptr_t>(address(0x1fc44e4)),controlled=read<uintptr_t>(address(0xf00740));
    const auto count=read<unsigned>(address(0x1fc44e0));
    // EnableControl also enables scripts with no player. Require its actual
    // bound actor; playerActor's navigation fallback is insufficient here.
    if(!actors || !count || count>128 || controlled<actors || controlled>=actors+count*0x880 ||
        (controlled-actors)%0x880) return false;
    const auto enabled=read<uint16_t>(controlled+2);
    if(!enabled || enabled>255 || read<unsigned>(controlled+0x824)==0xcdcdcdcd) return false;
    for(unsigned offset:{0xcu,0x10u,0x14u}) {
        const auto value=read<float>(controlled+offset,std::numeric_limits<float>::quiet_NaN());
        if(!std::isfinite(value) || value==-431602080) return false;
    }
    const auto chunk=read<uintptr_t>(controller+0x48);
    AtelSpan span;if(!nativeAtelSpan(chunk,span) || span.size<0xc) return false;
    const auto begin=read<unsigned>(chunk+4),end=read<unsigned>(chunk+8);
    return begin && end>begin && end<=span.size && (end-begin)%32==0 &&
        read<uint8_t>(address(0xd2ca90)+0xc)<(end-begin)/32;
}
Point playerPosition() {
    const auto actors = playerActor();
    const float invalid = std::numeric_limits<float>::quiet_NaN();
    return {read<float>(actors + 0xc, invalid), read<float>(actors + 0x10, invalid), read<float>(actors + 0x14, invalid)};
}
std::wstring direction(Point delta) {
    if(std::abs(delta.x)<.01 && std::abs(delta.z)<.01) return cameraDirectionLabel(delta,{},false);
    Point forward,right;const bool valid=cameraBasis(forward,right);
    return cameraDirectionLabel(delta,forward,valid);
}

bool cameraBasis(Point& forward,Point& right) {
    if(nav::ownedCamera) {forward=nav::ownedCamera->forward;right=nav::ownedCamera->right;return nav::ownedCamera->valid;}
    const auto camera=read<uintptr_t>(address(0x1f11430)); if(!camera) return false;
    const double x=read<float>(camera+0x40)-read<float>(camera+0x65c),z=read<float>(camera+0x48)-read<float>(camera+0x664);
    const double length=std::hypot(x,z); if(!std::isfinite(length) || length<.001) return false;
    forward={x/length,0,z/length}; right={forward.z,0,-forward.x}; return true;
}
bool relativeBearing(Point delta,double& pan,double& front) {
    Point forward,right; if(!cameraBasis(forward,right)) return false;
    const double length=std::hypot(delta.x,delta.z);
    if(!std::isfinite(length)) return false;
    pan=length>.001?std::clamp((delta.x*right.x+delta.z*right.z)/length,-1.0,1.0):0;
    front=length>.001?std::clamp((delta.x*forward.x+delta.z*forward.z)/length,-1.0,1.0):1;
    return true;
}
}
