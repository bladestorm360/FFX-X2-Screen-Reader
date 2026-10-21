#pragma once
#include "navigation.hpp"
#include <cmath>
#include <cstring>
namespace ffx::nav {
inline bool sameRouteEvents(const RouteEvents& a,const RouteEvents& b) {
    if(a.kind!=b.kind || a.count!=b.count || a.ready!=b.ready || a.count>a.rows.size()) return false;
    const auto point=[](Point x,Point y){return x.x==y.x && x.y==y.y && x.z==y.z;};
    for(unsigned i=0;i<a.count;++i) if(!point(a.rows[i].a,b.rows[i].a) || !point(a.rows[i].b,b.rows[i].b) ||
        !point(a.rows[i].landing,b.rows[i].landing) || a.rows[i].height!=b.rows[i].height) return false;
    return true;
}
// Four ready header/callback/geometry reads on the existing native observation.
// No parsing/hash, allocation, waits, new polling or borrowed pointer handoff.
inline RouteEvents captureRouteEvents(uintptr_t controller) {
    RouteEvents result;
    if(!nativeGameThread()) {result.ready=false;return result;}
    if(read<uint16_t>(address(0xd2ca90))!=203) return result;
    result.kind=1;result.ready=false;AtelIdentity identity;
    const auto chunk=read<uintptr_t>(controller+0x48);
    if(!readyAtelIdentity(chunk,identity) || identity.span.chunk!=chunk ||
        std::strcmp(identity.name,"sins0200") || identity.length!=14465 || identity.checksum!=0xb5ed9bb4 || identity.workers!=48) return result;
    using Worker=uintptr_t(__cdecl*)(int);
    for(unsigned local=17;local<=20;++local) {
        unsigned header=0;if(!readyAtelWorkerHeader(chunk,local,header)) return result;
        const auto worker=reinterpret_cast<Worker>(address(0x46a7c0))(local);
        if(!worker || read<uintptr_t>(worker+4)!=chunk || read<uintptr_t>(worker)!=chunk+header ||
            read<uint16_t>(worker+0x2e,0xffff)!=local || read<uint8_t>(chunk+header)!=2) return result;
        // Only original entry4 requests the qualified W0 ledge sequence. An
        // inactive/remapped native crossing must retire its directed constraint.
        const auto facts=captureNativeInteractionFacts(worker,EntryMask{1u<<4},identity.span);
        if(!(facts.channels&16)) continue;
        const auto geometry=worker+0x558;DirectedCrossing row;
        row.a={read<float>(geometry),read<float>(geometry+4),read<float>(geometry+8)};
        row.b={read<float>(geometry+0x30),read<float>(geometry+0x34),read<float>(geometry+0x38)};
        row.height=read<float>(geometry+0x1c);
        if(!std::isfinite(row.a.x) || !std::isfinite(row.a.y) || !std::isfinite(row.a.z) ||
           !std::isfinite(row.b.x) || !std::isfinite(row.b.y) || !std::isfinite(row.b.z) ||
           !std::isfinite(row.height) || std::hypot(row.b.x-row.a.x,row.b.z-row.a.z)<.001) return result;
        // W0 entries6/7 have a dynamic Z but fixed X/Y. Mid-edge Z only
        // identifies that landing side; it is never a synthesized jump target.
        row.landing=local<19?Point{-110.06990814208984,-.6498910188674927,(row.a.z+row.b.z)/2}:
            Point{91.16837310791016,14.116618156433105,-40.34036636352539};
        result.rows[result.count++]=row;
    }
    result.ready=true;return result;
}
}
