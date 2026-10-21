#pragma once
#include "owned_mesh_decoder.hpp"
#include "navigation_events.hpp"
#include <limits>
#include <cstring>
namespace ffx::nav {
struct GeometryFacts {unsigned epoch=0,revision=0,count=0,vertexCount=0;uintptr_t root=0,triangles=0,vertices=0;float scale=0;};
struct NativeGeometryContext {uintptr_t actor=0;GeometryFacts geometry;CollisionSnapshot collision;Point player;int floor=-1;unsigned waterBake=0,waterCrossings=0;bool swimming=false;RouteEvents events;};
inline bool validNativePoint(Point p){return std::isfinite(p.x)&&std::isfinite(p.y)&&std::isfinite(p.z);}
inline bool nativeRegion(uintptr_t p,unsigned bytes){return p>=0x10000&&p!=0xcccccccc&&p!=0xcdcdcdcd&&p<=UINTPTR_MAX-bytes;}
// Same original ChMap/controlled-actor facts used by reachability. The native
// owner copies scalars and fixed remaps; decoders/searches remain on workers.
inline bool captureNativeGeometry(NativeGeometryContext& c,unsigned revision,bool requireControl=true){
    if(!nativeGameThread()||scene||inBattle())return false;
    const auto actor=playerActor(),controller=read<uintptr_t>(address(0xf26ae8));
    if(!actor||!read<uint16_t>(actor+2) || (requireControl &&
        (actor!=read<uintptr_t>(address(0xf00740))||!controller||!(read<uint8_t>(controller)&2))))return false;
    c={};c.actor=actor;c.geometry.epoch=generation();c.geometry.revision=revision;
    const int count=read<int>(address(0xf01a48),-1);if(count<0||count>32767)return false;
    c.geometry.count=static_cast<unsigned>(count);c.geometry.triangles=read<uintptr_t>(address(0xf01a44));
    c.geometry.vertices=read<uintptr_t>(address(0xf01a4c));c.geometry.scale=read<float>(address(0xf01a50));c.geometry.root=read<uintptr_t>(address(0xf01a54));
    // Original91D5E0/937B70 both use Root+A signed-short count and8-byte rows.
    const int vertices=count && c.geometry.root?read<int16_t>(c.geometry.root+0xa,-1):0;
    if(vertices<0||(count&&!vertices))return false;c.geometry.vertexCount=static_cast<unsigned>(vertices);
    c.player=playerPosition();c.floor=read<int16_t>(actor+0x824,-1);c.swimming=read<uint8_t>(actor+0x186)==2;
    c.events=captureRouteEvents(controller);
    AtelIdentity identity;
    if(readyAtelIdentity(read<uintptr_t>(controller+0x48),identity) && identity.length==24363 && identity.checksum==0x131b7d53 &&
        std::memcmp(identity.name,"mtgz0600",9)==0) {
        c.waterBake=1;
        using Worker=uintptr_t(__cdecl*)(int);
        for(unsigned i=0;i<3;++i) {
            unsigned header=0;if(!readyAtelWorkerHeader(identity.span.chunk,35+i,header)) continue;
            const auto worker=reinterpret_cast<Worker>(address(0x46a7c0))(35+i);
            if(read<uintptr_t>(worker+4)!=identity.span.chunk || read<uintptr_t>(worker)!=identity.span.chunk+header) continue;
            if(captureNativeInteractionFacts(worker,EntryMask(0x4),identity.span).channels) c.waterCrossings|=1u<<i;
        }
    }
    c.collision.actorFlags=read<unsigned>(actor+0x198);c.collision.actorIsControlled=actor==read<uintptr_t>(address(0xf00740));
    if(!validNativePoint(c.player)||(count && (!std::isfinite(c.geometry.scale)||c.geometry.scale<=0))||
        !copy(address(0xf01998)+48*4,c.collision.remaps.data(),sizeof(c.collision.remaps)))return false;
    return count?nativeRegion(c.geometry.triangles,c.geometry.count*16)&&nativeRegion(c.geometry.vertices,c.geometry.vertexCount*8):c.swimming&&!c.geometry.triangles;
}
}
