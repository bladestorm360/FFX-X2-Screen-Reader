#include "pedestal.hpp"
#include <cmath>
#include <algorithm>
#include <stdexcept>
namespace ffx {
namespace {
const char* scriptName(unsigned control) {
    const char* names[]={"","bsyt0100","klyt1000","djyt0600","mcyt0500","djyt0600","dome0700","bvyt1100"};
    return control<_countof(names)?names[control]:"";
}
Point pushVector(const PedestalMove& move,Point p) {
    const double x=move.goal.x-p.x,z=move.goal.z-p.z;
    return std::abs(x)>std::abs(z)?Point{x>0?1.0:-1.0,0,0}:Point{0,0,z>0?1.0:-1.0};
}
}
bool pedestalScript(unsigned control,UtilityScript& script) {
    if(!control) return false;
    if(!captureUtilityScript(script)) return false;
    if(script.name!=scriptName(control)) return false;
    if(!captureCurrentTrial(script) || !captureUtilityWorker(script,0,script.worker)) return false;
    script.header=read<uintptr_t>(script.worker);
    return true;
}
bool capturePedestalSnapshot(const UtilityScript& script,const NativeRouteTarget& target,PedestalSnapshot& s) noexcept {
    try {s={};s.position=target.position;
    uintptr_t worker=0;if(!captureUtilityWorker(script,target.pushLocal,worker)) return false;
    if(read<uint8_t>(read<uintptr_t>(worker))==1) {
        const auto actor=read<uintptr_t>(worker+0x9c);
        if(!actor || !read<uint16_t>(actor+2)) return false;
        s.position={read<float>(actor+0xc),read<float>(actor+0x10),read<float>(actor+0x14)};
    }
    bool available=true;const auto value=[&](unsigned low) {int result=0;if(!captureUtilityValue(script,low,1,0,result)) {available=false;return;}s.values[low]=result;};
    switch(target.pushControl) {
    case 1: value(0x5c0000cc); value(0x5c000100); break;
    case 2: value(0x0c00005a); value(0x0c000064); break;
    case 3: for(unsigned offset:{0x99u,0x9au,0x9bu,0x9du,0x9eu,0xd9u,0xdbu}) value(0x0c000000+offset); break;
    case 4: for(unsigned offset:{0x96u,0x97u,0xa3u,0xc6u}) value(0x0c000000+offset); break;
    case 5: value(0x0c0000de+target.pushLocal-56); break;
    case 6: value(0x0c000021); break;
    case 7: for(unsigned offset:{7u,8u,0x1fu,0x20u,0x24u,0x29u}) value(0x0c000000+offset); break;
    }
    if(!available || !std::isfinite(s.position.x) || !std::isfinite(s.position.y) || !std::isfinite(s.position.z))
        return false;
    return true;}catch(...) {return false;}
}
PedestalSnapshot pedestalSnapshot(const UtilityScript& script,const Entity& target) {
    NativeRouteTarget facts;facts.position=target.position;facts.pushControl=target.pushControl;facts.pushLocal=target.pushLocal;
    PedestalSnapshot snapshot;if(!capturePedestalSnapshot(script,facts,snapshot)) throw std::runtime_error("The pedestal state is unavailable");return snapshot;
}
PedestalPlan currentPedestalPlan(const Entity& target) {
    UtilityScript script;
    if(!pedestalScript(target.pushControl,script)) throw std::runtime_error("No verified pedestal script is active");
    return pedestalPlan(target.pushControl,target.pushLocal,pedestalSnapshot(script,target));
}
void preparePedestalZone(Entity& entity,uintptr_t worker) {
    if(!entity.pushControl) return;
    try {
        if(pedestalActiveCrossing(entity)) { entity.inactive=!pedestalCrossingZone(entity); return; }
        if(pedestalActiveContact(entity)) { entity.inactive=entity.inactive || !pedestalContactZone(entity); return; }
        auto plan=currentPedestalPlan(entity);
        if(!plan.complete && plan.blocked.empty()) pedestalActivePlan(entity,plan);
        if(entity.pushControl<=2) {
            if(!plan.moves.empty()) {
                const auto delta=pushVector(plan.moves.front(),entity.position);
                entity.zone.approach={-delta.x,0,-delta.z};
            }
            return;
        }
        if(entity.pushControl==5 || entity.pushControl==6) return;
        if(plan.complete || !plan.blocked.empty() || plan.moves.empty()) { entity.inactive=true; return; }
        const auto p=entity.position;
        const auto delta=pushVector(plan.moves.front(),p);
        auto& zone=entity.zone;
        if(entity.pushControl==7 && plan.phase==0) {
            // The actual entrance action is worker39's crossing edge, not
            // actor13's empty interaction entries.
            UtilityScript script; if(!pedestalScript(entity.pushControl,script)) return;
            const auto edge=utilityWorker(script,39);
            unsigned channels=0;
            interactionEntries(edge,EntryMask{1u<<6},&channels);
            const auto edgeZone=interactionZone(edge,16);
            const auto player=playerActor();
            const double playerRadius=(read<float>(player+0x5c)+read<float>(player+0x64))*.5*read<float>(player+0x4e4);
            zone={}; zone.shape=3; zone.width=zone.depth=.4; zone.height=1.5;
            zone.center={(edgeZone.center.x+edgeZone.end.x)*.5,edgeZone.center.y,
                (edgeZone.center.z+edgeZone.end.z)*.5-std::max(4.0,playerRadius+2)};
            zone.valid=edgeZone.valid && std::isfinite(playerRadius) && playerRadius>0;
            entity.inactive=!zone.valid || !channels; return;
        }
        const auto actor=read<uintptr_t>(worker+0x9c);
        const double radius=actorContactRadius(actor,playerActor());
        const double approach=std::max(5.0,radius+.7);
        zone={}; zone.shape=3; zone.height=9.5;
        zone.width=zone.depth=.4;
        zone.center={p.x-delta.x*approach,p.y,p.z-delta.z*approach};
        zone.obstacle=p; zone.obstacleRadius=radius;
        // Native Djose/Macalania side rectangles exclude distances >=9.6.
        zone.valid=std::isfinite(radius) && radius>0 && approach+.4<9.6;
        if(entity.pushControl==7) {
            zone.height=2;
            zone.center={p.x,p.y,std::max(268.0,p.z+approach)};
        }
        entity.inactive=!zone.valid;
    } catch (...) {
        // No unverified scripted rules are used. Ordinary geometry remains
        // readable; arming requires the verified state separately.
        if(entity.pushControl>2 && entity.pushControl!=5 && entity.pushControl!=6) entity.inactive=true;
    }
}
bool pedestalCrossingZone(Entity& target) {
    if(target.pushControl!=7) return false;
    UtilityScript script; if(!pedestalScript(7,script)) return false;
    const auto edge=utilityWorker(script,39);
    unsigned channels=0; interactionEntries(edge,EntryMask{1u<<6},&channels);
    target.zone=interactionZone(edge,16); target.zone.crossing=true; target.zone.talk=false; target.zone.height=2;
    return target.zone.valid && channels;
}
bool pedestalContactZone(Entity& target) {
    if(target.pushControl<1 || target.pushControl>2) return false;
    const auto radius=target.zone.contactRadius;
    const auto height=target.zone.height;
    // Ordinary DI/pad walking makes the native collision edge. Aggregate-only
    // script directions cannot move the player across the staging gap.
    target.zone={}; target.zone.shape=1; target.zone.center=target.position;
    target.zone.range=radius-.15; target.zone.height=height;
    target.zone.valid=std::isfinite(radius) && radius>.5;
    return target.zone.valid;
}
std::wstring pedestalInstruction(const Entity& entity) {
    if(!entity.pushControl) return {};
    try {
        UtilityScript script; if(!pedestalScript(entity.pushControl,script)) throw std::runtime_error("No verified pedestal script");
        const auto snapshot=pedestalSnapshot(script,entity);
        auto plan=pedestalPlan(entity.pushControl,entity.pushLocal,snapshot);
        if(!plan.complete && plan.blocked.empty()) pedestalActivePlan(entity,plan);
        const auto side=describePedestalApproach(plan,snapshot.position,playerPosition());
        return (side.empty()?L"":side+L" ")+describePedestalPlan(plan,snapshot.position,pedestalConsentName(plan));
    }
    catch (...) { return L"The current pedestal state is unavailable."; }
}
}
