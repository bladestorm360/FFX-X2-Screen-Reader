#include "navigation.hpp"
#include "interaction_region.hpp"
#include "callback_guard_facts.hpp"
#include "swimming_navigation.hpp"
#include "pedestal.hpp"
#include <algorithm>
#include <cmath>
#include <cstring>
#include <optional>
namespace ffx {
namespace {
bool callbackAvailable(uintptr_t worker,unsigned entry) {
    // Prove an early no-op using only pure native script guards. Unknown
    // operations retain the interaction; no game call or write is simulated.
    const auto header=read<uintptr_t>(worker),chunk=read<uintptr_t>(worker+4);
    const unsigned length=read<unsigned>(chunk),codeOffset=read<unsigned>(chunk+0x30);
    const unsigned entries=read<unsigned>(header+0x20),jumps=read<unsigned>(header+0x24);
    if(!chunk || !length || length>0x800000 || codeOffset>0x800000-length ||
        !entries || entries>0x800000-(entry+1)*4) return true;
    unsigned pc=read<unsigned>(chunk+entries+entry*4,length);
    using Value=std::optional<int>;
    std::vector<int> stack; Value a,x,y;
    const auto pop=[&]() -> Value { if(stack.empty()) return {}; const int value=stack.back(); stack.pop_back(); return value; };
    for(unsigned n=0;n<256;++n) {
        uint8_t op; uint16_t operand=0;
        if(pc>=length || !copy(chunk+codeOffset+pc,&op,1)) return true;
        const unsigned size=op&0x80?3:1;
        if(size>length-pc || (size==3 && !copy(chunk+codeOffset+pc+1,&operand,2))) return true;
        pc+=size;
        if(op==0x00) continue;
        if(op==0x34 || (op>=0x3c && op<=0x40) || op==0x54) return false;
        if(op==0xae) stack.push_back(static_cast<int16_t>(operand));
        else if(op==0xad) {
            const auto offset=read<unsigned>(header+0x18);
            int value; if(operand>=read<uint16_t>(header+4) || offset>0x800000u-(operand+1u)*4u ||
                !copy(chunk+offset+operand*4,&value,4)) return true;
            stack.push_back(value);
        } else if(op==0x9f || op==0xa2) {
            // Match the owned proof's bounded scalar/index0 policy. Original
            // type6 seal guards use indexed loads even for this single byte.
            if(op==0xa2) {const auto index=pop();if(!index || *index!=0) return true;}
            const auto offset=read<unsigned>(header+0x14);
            unsigned declaration;
            if(operand>=read<uint16_t>(header+2) || offset>0x800000u-(operand+1u)*8u ||
                !copy(chunk+offset+operand*8,&declaration,4) || !read<uint16_t>(chunk+offset+operand*8+4)) return true;
            // Only saved world/story guards prove lasting unavailability.
            // Per-worker proximity/motion registers must not hide a distant
            // exit while its native collision/callback flags remain enabled.
            if(((declaration>>25)&7)!=0) return true;
            using Variable=uintptr_t(__cdecl*)(uintptr_t,int,int);
            const auto source=reinterpret_cast<Variable>(address(0x46c500))(worker,operand,0);
            int value=0;
            switch(declaration>>28) {
            case 0: { uint8_t v; if(!copy(source,&v,1)) return true; value=v; break; }
            case 1: { int8_t v; if(!copy(source,&v,1)) return true; value=v; break; }
            case 2: { uint16_t v; if(!copy(source,&v,2)) return true; value=v; break; }
            case 3: { int16_t v; if(!copy(source,&v,2)) return true; value=v; break; }
            case 4: case 5: if(!copy(source,&value,4)) return true; break;
            default: return true;
            }
            stack.push_back(value);
        } else if(op==0x25 || op==0x2a || op==0x2c) {
            auto value=pop(); if(!value) return true;
            (op==0x25?a:op==0x2a?x:y)=value;
        } else if(op==0x26 || op==0x28 || op==0x29) {
            const auto value=op==0x26?a:op==0x28?x:y;
            if(!value) return true; stack.push_back(*value);
        } else if(op==0x2b) {
            if(stack.empty()) return true; stack.push_back(stack.back());
        } else if(op==0x19 || op==0x1c) {
            const auto value=pop(); if(!value) return true;
            stack.push_back(op==0x19?!*value:~*value);
        } else if(op>=1 && op<=0x0f) {
            const auto right=pop(),left=pop(); if(!left || !right) return true;
            const auto l=*left,r=*right; int value=0;
            switch(op) {
            case 1: value=l || r; break; case 2: value=l && r; break;
            case 3: value=l|r; break; case 4: value=l^r; break; case 5: value=l&r; break;
            case 6: value=l==r; break; case 7: value=l!=r; break;
            case 8: value=static_cast<unsigned>(l)>static_cast<unsigned>(r); break;
            case 9: value=static_cast<unsigned>(l)<static_cast<unsigned>(r); break;
            case 10: value=l>r; break; case 11: value=l<r; break;
            case 12: value=static_cast<unsigned>(l)>=static_cast<unsigned>(r); break;
            case 13: value=static_cast<unsigned>(l)<=static_cast<unsigned>(r); break;
            case 14: value=l>=r; break; case 15: value=l<=r; break;
            }
            stack.push_back(value);
        } else if(op==0xb0 || op==0xb1 || op==0xb2 || op==0xd5 || op==0xd6 || op==0xd7) {
            bool jump=op==0xb0 || op==0xd5;
            if(!jump) {
                const auto condition=op>=0xd5?pop():x;
                if(!condition) return true;
                if(op>=0xd5) x=condition;
                jump=(op==0xb1 || op==0xd6)?*condition!=0:*condition==0;
            } else if(op==0xd5) { x=pop(); if(!x) return true; }
            if(jump) {
                if(operand>=read<uint16_t>(header+0xa) || !jumps || jumps>0x800000u-(operand+1u)*4u) return true;
                pc=read<unsigned>(chunk+jumps+operand*4,length);
            }
        } else if(op==0xd8 && operand==93) {
            // Only the prepared exact allocation can authorize a known
            // cleanup-only branch. No parser/hash fallback or game call runs
            // here, and a pending/unknown identity retains the interaction.
            AtelIdentity identity;unsigned expectedHeader=0;
            const auto local=read<uint16_t>(worker+0x2e,0xffff);
            if(readyAtelIdentity(chunk,identity) && readyAtelWorkerHeader(chunk,local,expectedHeader) &&
                header==chunk+expectedHeader && unavailableCallbackEpilogue(identity.name,identity.length,identity.checksum,local,entry,pc-size,operand)) return false;
            return true;
        } else return true;
    }
    return true;
}
}
NativeInteractionFacts captureNativeInteractionFacts(uintptr_t worker,EntryMask mask) {
    AtelSpan span;if(!nativeGameThread() || !worker || !nativeAtelSpan(read<uintptr_t>(worker+4),span)) return {};
    return captureNativeInteractionFacts(worker,mask,span);
}
NativeInteractionFacts captureNativeInteractionFacts(uintptr_t worker,EntryMask mask,const AtelSpan& span) {
    NativeInteractionFacts facts;if(!nativeGameThread() || !worker) return facts;
    const auto header=read<uintptr_t>(worker),chunk=read<uintptr_t>(worker+4);
    if(chunk!=span.chunk || span.epoch!=generation() || header<chunk || header-chunk>span.size || span.size-(header-chunk)<0x28) return facts;
    const unsigned type=read<uint8_t>(header);
    if(!(read<uint16_t>(worker+0x34)&0x80) || (type==1 && read<uint8_t>(worker+0xaa)!=1)) return facts;
    const auto flags=read<uint8_t>(worker+0xab);
    using Entry=uint16_t(__cdecl*)(unsigned,unsigned,uintptr_t);
    using Priority=uint8_t(__cdecl*)(unsigned,unsigned);
    for(unsigned channel=0;channel<7;++channel) if(flags&(1u<<channel)) {
        if(type==6 && channel!=5) continue; // Player entry only; leave is not an approach action.
        if(type==1 && channel==0 && ((read<uint16_t>(worker+0x34)&0x220) || (read<uint8_t>(worker+0x36)&8))) continue;
        if(reinterpret_cast<Priority>(address(0x4767e0))(type,channel)==255) continue;
        const auto entry=reinterpret_cast<Entry>(address(0x476360))(type,channel,read<uintptr_t>(worker+0x44));
        if(entry>=read<uint16_t>(header+8) || !mask.contains(entry)) continue;
        facts.entries[channel]=entry;facts.channels|=1u<<channel;
    }
    return facts;
}
std::vector<unsigned> interactionEntries(uintptr_t worker,EntryMask mask,unsigned* channels) {
    std::vector<unsigned> result;
    if(channels) *channels=0;
    const unsigned type=read<uint8_t>(read<uintptr_t>(worker));
    if(!worker || !(read<uint16_t>(worker+0x34)&0x80) || (type==1 && read<uint8_t>(worker+0xaa)!=1)) return result;
    const auto flags=read<uint8_t>(worker+0xab);
    using Entry=uint16_t(__cdecl*)(unsigned,unsigned,uintptr_t);
    using Priority=uint8_t(__cdecl*)(unsigned,unsigned);
    for(unsigned channel=0;channel<7;++channel) if(flags&(1u<<channel)) {
        if(type==6 && channel!=5) continue;
        if(type==1 && channel==0 && ((read<uint16_t>(worker+0x34)&0x220) || (read<uint8_t>(worker+0x36)&8))) continue;
        // +34 bit 100 records the player being inside a rectangle. It is
        // movement history, not a disabled exit (866C60 writes it each crossing).
        if(reinterpret_cast<Priority>(address(0x4767e0))(type,channel)==255) continue;
        const auto entry=reinterpret_cast<Entry>(address(0x476360))(type,channel,read<uintptr_t>(worker+0x44));
        if(entry>=read<uint16_t>(read<uintptr_t>(worker)+8) || !mask.contains(entry)) continue;
        if(!callbackAvailable(worker,entry)) continue;
        result.push_back(entry); if(channels) *channels|=1u<<channel;
    }
    return result;
}
InteractionZone interactionZone(uintptr_t worker,unsigned channels) {
    InteractionZone zone;
    const unsigned type=read<uint8_t>(read<uintptr_t>(worker));
    if(type==6) {
        std::array<uint8_t,0x4c> geometry{};
        return copy(worker+0x284,geometry.data(),geometry.size())?playerEntryRegion(geometry,channels):zone;
    }
    if(type<1 || type>3 || !channels) return zone;
    const auto geometry=worker+0x558;
    zone.center={read<float>(geometry),read<float>(geometry+4),read<float>(geometry+8)};
    zone.end={read<float>(geometry+0x30),read<float>(geometry+0x34),read<float>(geometry+0x38)};
    zone.angle=read<float>(geometry+0x34);
    zone.width=std::abs(read<float>(geometry+0x30)); zone.depth=std::abs(read<float>(geometry+0x38));
    zone.height=read<float>(geometry+(type==1 && (channels&1)?0x2c:0x1c));
    const auto controller=read<uintptr_t>(address(0xf26ae8));
    zone.range=channels&1?read<float>(geometry+0x40)+read<float>(controller+0x1f0):
        (channels&8) || (type==2 && (channels&0x20))?read<float>(geometry+0x48):0;
    zone.shape=type;
    zone.talk=(channels&1)!=0;
    if(zone.talk) zone.viewAngle=read<float>(geometry+0x3c);
    zone.crossing=!(channels&1) && (channels&16);
    zone.valid=std::isfinite(zone.center.x) && std::isfinite(zone.center.y) && std::isfinite(zone.center.z) &&
        std::isfinite(zone.range) && std::isfinite(zone.height) && std::isfinite(zone.angle) &&
        std::isfinite(zone.width) && std::isfinite(zone.depth) && std::isfinite(zone.viewAngle) && zone.range>=0;
    return zone;
}
void constrainInteractionZone(InteractionZone& zone,uintptr_t selected) {
    zone.worker=read<uint16_t>(selected+0x2e); zone.competing.clear();
    if(!zone.valid || !zone.talk) return;
    const auto controller=read<uintptr_t>(address(0xf26ae8));
    const unsigned count=read<uint16_t>(controller+0xc);
    if(!controller || count>500) return;
    using Worker=uintptr_t(__cdecl*)(int);
    const auto own=selected;
    for(unsigned i=0;i<count;++i) {
        const auto worker=reinterpret_cast<Worker>(address(0x46a7c0))(i);
        if(!worker || worker==own) continue;
        unsigned channels=0; interactionEntries(worker,EntryMask{},&channels);
        if(!(channels&1)) continue;
        const auto other=interactionZone(worker,1);
        if(!other.valid || other.viewAngle<=0) continue;
        const double reach=zone.range+zone.width+zone.depth+other.range+other.width+other.depth;
        if(distanceXZ(zone.center,other.center)>reach) continue;
        auto shape=static_cast<const InteractionShape&>(other); shape.worker=i;
        zone.competing.push_back(shape);
    }
}
void configurePedestalTarget(Entity& entity,const char* script,unsigned local) {
    entity.pushControl=script && !std::strcmp(script,"bsyt0100") && local==7?1:
        script && !std::strcmp(script,"klyt1000") && local==6?2:
        script && !std::strcmp(script,"djyt0600") && local==1?3:
        script && !std::strcmp(script,"mcyt0500") && local==1?4:
        script && !std::strcmp(script,"djyt0600") && local>=56 && local<=60?5:
        script && !std::strcmp(script,"dome0700") && local>=44 && local<=47?6:
        script && !std::strcmp(script,"bvyt1100") && local==13?7:0;
    entity.pushLocal=local;
}
double actorContactRadius(uintptr_t actor,uintptr_t player) {
    const auto radiusOf=[](uintptr_t body) {
        return (read<float>(body+0x5c)+read<float>(body+0x64))*.5*read<float>(body+0x4e4);
    };
    return actor && player?radiusOf(actor)+radiusOf(player):0;
}
bool pedestalRecessTarget(const Entity& push,uintptr_t worker,Entity& recess) {
    // These three actors use entry2 for their integral sphere sockets.
    // Besaid's sphere socket is a separate, already-labelled worker44.
    if(push.pushControl<2 || push.pushControl>4) return false;
    unsigned channels=0; interactionEntries(worker,EntryMask{1u<<2},&channels);
    recess=push; recess.name=L"Pedestal sphere recess"; recess.pushControl=0; recess.pushLocal=0;
    recess.usefulEntries=EntryMask{1u<<2};
    recess.zone=interactionZone(worker,1); constrainInteractionZone(recess.zone,worker);
    recess.inactive=!(channels&1); recess.unreachable=false;
    return recess.zone.valid;
}
void refreshTargetZone(Entity& entity,uintptr_t worker,unsigned channels) {
    entity.zone=interactionZone(worker,channels);
    if(read<uint8_t>(read<uintptr_t>(worker))==6 && !entity.zone.valid) entity.inactive=true;
    if(!entity.pushControl || entity.pushControl>2) {
        constrainInteractionZone(entity.zone,worker); preparePedestalZone(entity,worker); return;
    }
    unsigned liveChannels=0; interactionEntries(worker,EntryMask{},&liveChannels);
    entity.inactive=(liveChannels&4)==0;
    const auto actor=read<uintptr_t>(worker+0x9c),player=playerActor();
    const auto radiusOf=[](uintptr_t body) {
        return (read<float>(body+0x5c)+read<float>(body+0x64))*.5*read<float>(body+0x4e4);
    };
    const double actorRadius=radiusOf(actor),playerRadius=radiusOf(player);
    const double radius=actorRadius+playerRadius;
    auto& zone=entity.zone;
    zone={}; zone.worker=entity.worker; zone.shape=1;
    zone.center={read<float>(actor+0xc),read<float>(actor+0x10),read<float>(actor+0x14)};
    // Native 43D3B0 recomputes +4E8 from these live bounds/scales, then
    // dispatches channel 2 on contact. Stop just outside it for manual pushing.
    zone.contactRadius=radius; zone.height=-1;
    if((read<unsigned>(actor+0x194)&read<unsigned>(player+0x194)&0x40000000)!=0) {
        const auto bottom=[](uintptr_t body) {
            const double extent=read<float>(body+0x51c);
            return extent==0?read<float>(body+0x6a8):read<float>(body+0x10)-extent;
        };
        const double playerY=read<float>(player+0x10);
        zone.contactMinY=bottom(actor)-(read<float>(player+0x698)-playerY);
        zone.contactMaxY=read<float>(actor+0x698)-(bottom(player)-playerY);
    }
    zone.approach=entity.pushControl==1?Point{0,0,1}:Point{};
    zone.valid=actor && player && std::isfinite(actorRadius) && actorRadius>0 && std::isfinite(playerRadius) && playerRadius>0 && std::isfinite(radius) &&
        std::isfinite(zone.contactMinY) && std::isfinite(zone.contactMaxY) && zone.contactMinY<=zone.contactMaxY &&
        std::isfinite(zone.center.x) && std::isfinite(zone.center.y) && std::isfinite(zone.center.z);
    if(!zone.valid) entity.inactive=true;
    preparePedestalZone(entity,worker);
}
void refreshNavigationTarget(Entity& entity) {
    if(entity.swimDepth) { refreshSwimmingGoal(entity); return; }
    if(entity.worker<0) return;
    using Worker=uintptr_t(__cdecl*)(int);
    const auto worker=reinterpret_cast<Worker>(address(0x46a7c0))(entity.worker);
    unsigned channels=0;
    const auto entries=interactionEntries(worker,entity.usefulEntries,&channels);
    const bool cid=entity.actor>=0 && entity.id==0x2101;
    if(cid) channels&=1u;
    entity.inactive=cid?!channels:entries.empty();
    // Keep the semantic callback selection used at discovery. A no-op talk
    // callback must never replace an exit's crossing geometry on movement.
    refreshTargetZone(entity,worker,channels?channels:entity.authoredTrialControl || cid?1u:0u);
}

}
