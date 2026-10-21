// Return worker-computed movement intentions to their native execution owner.
// Four ticketed slots carry copied geometry/script/saved-state facts. Revalidate
// epoch, focus, manual-input revision, route identity and 250ms data age before
// facing/arming/starting; a worker never dereferences the stored native tokens.
// See Docs/features/navigation/index.md#movement.
#include "navigation_intents.hpp"
#include "navigation_events.hpp"
#include "owned_nav_target.hpp"
#include "nonblocking_snapshots.hpp"
#include "process_storage.hpp"
#include "auto_walk.hpp"
#include "nav_audio.hpp"
#include "pedestal_rvas.hpp"
#include "settings.hpp"
#include "mod_menu.hpp"
#include "utilities.hpp"
#include <atomic>
#include <cstring>
namespace ffx::nav {
namespace {
constexpr unsigned Slots=4,Writing=0x80000000u,Held=0x40000000u;
struct Frame {
    NativeRouteTarget target;AtelSpan span;AtelIdentity script;PedestalPlan plan;SceneBindings bindings;RouteEvents events;
    AtelSavedFacts saved;std::array<uint8_t,0x68c0-0x1ec> currentSaved;
    std::array<uint8_t,0x4c> geometry;
    uintptr_t controller=0,primary=0;unsigned epoch=0,revision=0,manual=0,global=0,local=0,header=0,type=0,model=0,actorCount=0,zeroGlobal=0,zeroHeader=0,collisionSerial=0;
    Intent kind=Intent::Face;ULONGLONG capturedAt=0;
    bool pointOnly=false;Point player,actorPose;
};
ProcessStorage<std::array<Frame,Slots>> frames;
std::array<std::atomic<unsigned>,Slots> tickets{};
async::TryCaptureQueue<unsigned,Slots> pending;
std::atomic<unsigned> revision{1},nextTicket{0};
void release(unsigned slot,unsigned token) {if(slot<Slots) tickets[slot].compare_exchange_strong(token,0,std::memory_order_release,std::memory_order_relaxed);}
bool scriptFor(const Frame& f,UtilityScript& script) {
    using Getter=uintptr_t(__cdecl*)(int);const auto worker=reinterpret_cast<Getter>(address(0x46a7c0))(static_cast<int>(f.zeroGlobal));
    if(!worker || read<uintptr_t>(worker+4)!=f.span.chunk || read<uint16_t>(worker+0x2e,0xffff)!=0 || read<uintptr_t>(worker)!=f.span.chunk+f.zeroHeader) return false;
    script.controller=f.controller;script.chunk=f.span.chunk;script.worker=worker;script.header=f.span.chunk+f.zeroHeader;script.name=f.script.name;
    script.length=f.script.length;script.codeOffset=f.script.codeOffset;script.workers=f.script.workers;script.allocationSerial=f.span.serial;return true;
}
bool qualify(unsigned slot,unsigned token) {
    if(slot>=Slots || tickets[slot].load(std::memory_order_acquire)!=(token|Held)) return false;
    auto& f=frames.get()[slot];if(f.epoch!=generation() || f.revision!=revision.load(std::memory_order_acquire) || f.manual!=autoWalkInputRevision() ||
        f.capturedAt>GetTickCount64() || GetTickCount64()-f.capturedAt>250 || !nativeReaderResourcesReady() || !gameFocused() || scene || inBattle() || modMenuActive() || utilityConsoleActive()) return false;
    const auto controller=read<uintptr_t>(address(0xf26ae8)),player=playerActor();
    if(controller!=f.controller || read<uintptr_t>(controller+0x48)!=f.primary || !(read<uint8_t>(controller)&2) || player!=read<uintptr_t>(address(0xf00740))) return false;
    const auto count=read<uint16_t>(controller+0xc);if(!count || count>SceneWorkers || (!f.pointOnly && f.global>=count)) return false;
    AtelSpan current;if(!nativeAtelSpan(f.span.chunk,current) || current.serial!=f.span.serial || current.size!=f.span.size) return false;
    if(f.pointOnly) {
        if(f.kind!=Intent::Start || f.target.pushControl || f.span.chunk!=f.primary || distanceXZ(playerPosition(),f.player)>.1 || std::abs(playerPosition().y-f.player.y)>.1) return false;
    }else {
    using Getter=uintptr_t(__cdecl*)(int);const auto worker=reinterpret_cast<Getter>(address(0x46a7c0))(static_cast<int>(f.global));
    if(read<uintptr_t>(worker+4)!=f.span.chunk || read<uintptr_t>(worker)!=f.span.chunk+f.header || read<uint16_t>(worker+0x2e,0xffff)!=f.local || read<uint8_t>(f.span.chunk+f.header)!=f.type) return false;
    std::array<uint8_t,0x4c> geometry;if(!copy(worker+(f.type==5 || f.type==6?0x284:0x558),geometry.data(),geometry.size()) || geometry!=f.geometry) return false;
    if(f.target.actor>=0) {
        const auto actors=read<uintptr_t>(address(0x1fc44e4));const auto capacity=read<unsigned>(address(0x1fc44e0));
        if(!actors || !capacity || capacity>SceneActors || static_cast<unsigned>(f.target.actor)>=capacity) return false;
        const auto actor=actors+f.target.actor*0x880;const auto enabled=read<uint16_t>(actor+2);
        const Point position{read<float>(actor+0xc),read<float>(actor+0x10),read<float>(actor+0x14)};
        if(read<uint16_t>(actor)!=f.model || !enabled || enabled>255 || !validNativePoint(position) || distanceXZ(position,f.actorPose)>.1 || std::abs(position.y-f.actorPose.y)>.1) return false;
    }
    }
    // Gate-affecting saved state must still match the observation. A changed
    // puzzle/story flag makes the route intention obsolete even in the same map.
    const auto saved=address(0xd2ca90)+0x1ec;
    if(!f.saved.valid || read<uintptr_t>(controller+0x2c)!=saved || !copy(saved,f.currentSaved.data(),f.currentSaved.size()) || f.currentSaved!=f.saved.bytes) return false;
    if(f.kind==Intent::ArmAndStart || f.kind==Intent::ArmAndFace || f.kind==Intent::UpdatePedestal) {
        if(f.zeroGlobal>=count || f.bindings.count>f.bindings.rows.size()) return false;
        AtelIdentity identity;if(!readyAtelIdentity(f.span.chunk,identity) || identity.span.serial!=f.span.serial || identity.checksum!=f.script.checksum || identity.length!=f.script.length) return false;
        UtilityScript script;if(!scriptFor(f,script)) return false;
        for(unsigned i=0;i<f.bindings.count;++i) {int value=0;const auto& row=f.bindings.rows[i];if(!captureUtilityValue(script,row.binding,row.high,row.element,value) || value!=row.value) return false;}
    }
    if(f.kind==Intent::AssistStart) {
        const auto active=capturePedestalNavigationState();
        if(!active.armed || !active.held || active.epoch!=f.epoch || active.worker!=f.target.worker || active.id!=f.target.id || active.control!=f.target.pushControl || active.local!=f.target.pushLocal) return false;
    }
    if(f.kind==Intent::Start || f.kind==Intent::AssistStart || f.kind==Intent::ArmAndStart) {
        // A callback may change after worker preparation but before native
        // movement starts. Never start a route against obsolete ledge facts.
        if(f.collisionSerial!=navCollisionSerial() || !f.events.ready || !sameRouteEvents(f.events,captureRouteEvents(controller))) return false;
        NavLeg leg;if(!navLeg(leg) || leg.pendingCollision || leg.target.worker!=f.target.worker || leg.target.id!=f.target.id || leg.target.pushControl!=f.target.pushControl || leg.target.pushLocal!=f.target.pushLocal) return false;
        if(f.pointOnly && distanceXZ(leg.final,f.target.position)>.1) return false;
    }
    return true;
}
void invoke(unsigned slot) {
    const auto& f=frames.get()[slot];
    if(f.kind==Intent::Face) faceNavigationTarget(f.target);
    else if(f.kind==Intent::UpdatePedestal) updatePedestalOwnedTarget(f.target);
    else if(f.kind==Intent::AssistFailed) cancelPedestal();
    else {
        if(f.kind==Intent::ArmAndStart || f.kind==Intent::ArmAndFace) {UtilityScript script;if(!scriptFor(f,script)) return;armPedestalOwned(f.target,script,f.plan);}
        if(f.kind==Intent::ArmAndFace) faceNavigationTarget(f.target);else startAutoWalk();
    }
}
}
void startNavigationIntents() {frames.prepare();}
unsigned navigationIntentRevision() {return revision.load(std::memory_order_acquire);}
void invalidateNavigationIntents() {revision.fetch_add(1,std::memory_order_acq_rel);}
bool submitNavigationIntent(const Scene& observation,const Entity& target,Intent kind,unsigned manual,unsigned wantedRevision) {
    if(nativeGameThread() || !frames.ready() || (target.worker>=0 && static_cast<unsigned>(target.worker)>=observation.count) || wantedRevision!=navigationIntentRevision()) return false;
    const bool pointOnly=target.worker<0;if(pointOnly && (kind!=Intent::Start || target.pushControl)) return false;
    const WorkerFact* worker=pointOnly?nullptr:&observation.workers[target.worker];unsigned part=SceneParts;
    if(worker) {if(!worker->valid || worker->part>=observation.partCount) return false;part=worker->part;}
    else for(unsigned i=0;i<observation.partCount;++i) if(observation.parts[i].span.chunk==observation.primary) {part=i;break;}
    if(part>=observation.partCount) return false;
    unsigned slot=Slots,token=(nextTicket.fetch_add(1)+1)&~(Writing|Held);if(!token) token=(nextTicket.fetch_add(1)+1)&~(Writing|Held);
    for(unsigned i=0;i<Slots;++i) {unsigned empty=0;if(tickets[i].compare_exchange_strong(empty,token|Writing,std::memory_order_acquire)) {slot=i;break;}}
    if(slot==Slots) return false;auto& f=frames.get()[slot];OwnedNavTarget captured;
    if(!captureOwnedNavTarget(target,captured)) {release(slot,token|Writing);return false;}f.target=captured.native;
    f.span=observation.parts[part].span;f.controller=observation.controller;f.primary=observation.primary;f.epoch=observation.epoch;f.revision=wantedRevision;f.manual=manual;f.pointOnly=pointOnly;f.player=observation.player;f.events=observation.geometry.events;f.collisionSerial=observation.collisionSerial;
    if(worker) {f.global=worker->global;f.local=worker->local;f.header=worker->header;f.type=worker->type;f.geometry=worker->geometry;}f.saved=observation.saved;f.kind=kind;f.capturedAt=observation.capturedAt;
    f.model=0;f.actorPose={};if(target.actor>=0 && static_cast<unsigned>(target.actor)<observation.actorCount) {
        const auto& body=observation.actors[target.actor].bytes;std::memcpy(&f.model,body.data(),sizeof(uint16_t));
        float x=0,y=0,z=0;std::memcpy(&x,body.data()+0xc,4);std::memcpy(&y,body.data()+0x10,4);std::memcpy(&z,body.data()+0x14,4);f.actorPose={x,y,z};
    }
    if(kind==Intent::ArmAndStart || kind==Intent::ArmAndFace || kind==Intent::UpdatePedestal) {
        f.bindings=observation.parts[part].bindings;
        if(!readyAtelIdentity(f.span.chunk,f.script) || !scenePedestalPlan(observation,target,f.plan)) {release(slot,token|Writing);return false;}
        bool zero=false;for(unsigned i=0;i<observation.count;++i) {const auto& row=observation.workers[i];if(row.valid && row.part==part && !row.local) {f.zeroGlobal=row.global;f.zeroHeader=row.header;zero=true;break;}}
        if(!zero) {release(slot,token|Writing);return false;}
    }
    tickets[slot].store(token,std::memory_order_release);if(!pending.tryPush(slot)) {release(slot,token);return false;}return true;
}
void dispatchNavigationIntents() {
    if(!nativeGameThread()) return;unsigned slot=Slots;
    for(unsigned count=0;count<Slots && pending.tryPop(slot);++count) {
        const auto token=tickets[slot].load(std::memory_order_acquire);if(!token || (token&(Held|Writing))) continue;
        auto expected=token;if(!tickets[slot].compare_exchange_strong(expected,token|Held,std::memory_order_acquire)) continue;
        NativeAction action;action.epoch=frames.get()[slot].epoch;
        action.prepare=[slot,token] {return qualify(slot,token);};action.invoke=[slot] {invoke(slot);};
        action.retire=[slot,token] {release(slot,token|Held);};
        const auto kind=frames.get()[slot].kind;
        action.complete=[kind](bool okay) {if(!okay && kind!=Intent::UpdatePedestal) output(L"Navigation action canceled because the player, input or destination changed. Request the route again.",false,"navigation-action-canceled");};
        if(!deferNativeAction(action)) {release(slot,token|Held);output(L"Navigation action is busy. Request the route again.",false,"navigation-pressure");}
    }
}
}
