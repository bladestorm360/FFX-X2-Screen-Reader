#include "owned_nav_target.hpp"
#include "navigation_observations.hpp"
// The configured push action supplies consent; original scripts own movement.
#include "pedestal.hpp"
#include "pedestal_rvas.hpp"
#include "auto_walk.hpp"
#include "nav_audio.hpp"
#include "movement_keys.hpp"
#include "settings.hpp"
#include "mod_menu.hpp"
#include "utilities.hpp"
#include <atomic>
#include <cmath>
#include <algorithm>
namespace ffx {
namespace {
struct Session {
    NativeRouteTarget target;
    UtilityScript script;
    PedestalPlan plan;
    unsigned epoch=0,segment=0,consent=0;
    std::array<unsigned,2> directions{};
    bool held=false,owned=false,started=false,single=false,stopping=false,approaching=false,crossing=false,contact=false,committed=false,waitingShape=false;
    uintptr_t signal=0,pawn=0;
    uint64_t previous=0,progress=0;
    Point last,lastPlayer;

} active;
std::atomic<bool> armed{false},manual{false};
unsigned drain=0,injected=0,published=0;
std::atomic<unsigned> cancelRevision{1};
unsigned observedCancel=1;
thread_local uintptr_t interpreter=0;
thread_local unsigned interpreterEpoch=0;
bool fieldControl() {
    const auto controller=read<uintptr_t>(address(0xf26ae8));
    return controller && (read<uint8_t>(controller)&2) && read<uintptr_t>(address(0xf00740));
}
void suppress(unsigned mask) {
    for(const auto rva:pedestal_rva::aggregates)
        write<uint16_t>(address(rva),static_cast<uint16_t>(read<uint16_t>(address(rva))&~mask));
}
void inject(unsigned bits) {
    const auto held=read<uint16_t>(address(pedestal_rva::Held));
    published|=bits&~held;
    write<uint16_t>(address(pedestal_rva::Held),static_cast<uint16_t>(held|bits));
    write<uint16_t>(address(pedestal_rva::Pressed),static_cast<uint16_t>(read<uint16_t>(address(pedestal_rva::Pressed))|(bits&~injected)));
    injected=bits;
}
void stop(const char* reason,bool announce=false) {
    if(published) suppress(published);
    if(armed || active.epoch) {
        if(active.held) drain|=active.consent;
        stopAutoWalk(); recordLog("pedestal assistance stopped: %s",reason);
        if(announce) output(L"Pedestal assistance stopped",false,"pedestal-push");
    }
    armed=false;manual=false;active={};injected=published=0;publishPedestalConsent(0);
    // Only the fixed target/plan and qualified eight-character script carrier
    // remain. Resetting this Native-owned session needs no container retirement.
}
bool context() {
    if(!setting(Setting::AutoWalk) || !gameFocused() || scene || inBattle() || modMenuActive() || utilityConsoleActive()) return false;
    using Mode=int(__cdecl*)();
    return reinterpret_cast<Mode>(address(0x46c840))()==0;
}
unsigned pc(uintptr_t worker) {
    const auto signal=read<uintptr_t>(worker+0x90);
    const auto level=read<uint8_t>(worker+0x32);
    const auto type=read<uint8_t>(read<uintptr_t>(worker));
    if(!signal || level>((type==5 || type==6)?1u:8u) || (read<uint8_t>(signal+0xe)&15)!=level)
        return 0xffffffff;
    const auto code=active.script.chunk+active.script.codeOffset;
    const auto cursor=read<uintptr_t>(worker+0x12c+level*0x4c+0x18);
    return cursor>=code && cursor<code+active.script.length?static_cast<unsigned>(cursor-code):0xffffffff;
}
bool producer(uintptr_t worker,unsigned local,unsigned entry) {
    return worker && read<uintptr_t>(worker+4)==active.script.chunk && read<uint16_t>(worker+0x2e,0xffff)==local &&
        read<uint16_t>(read<uintptr_t>(worker+0x90)+8,0xffff)==entry;
}
bool ownsDisable(Event& event) {
    if(!armed || !active.held || active.epoch!=generation() || event.parentRva!=0x464180 || interpreterEpoch!=generation() ||
        !producer(interpreter,active.plan.producer,active.plan.entry)) return false;
    const auto at=pc(interpreter);
    switch(active.target.pushControl) {
    case 1: return at==0x12ab || at==0x139b || at==0x13c7 || at==0x13e1 || at==0x13ed;
    case 2: return at==0x274b;
    case 3: return at==0xd17c;
    case 4: return at==0xe2ad || at==0xe44a;
    case 5: {
        const unsigned sites[]={0x14540,0x14700,0x148c0,0x14a80,0x14c40};
        return active.plan.producer>=56 && active.plan.producer<=60 && at==sites[active.plan.producer-56];
    }
    case 6: {
        const unsigned sites[]={0x747e,0x75f6,0x776e,0x78e6};
        return active.plan.producer>=44 && active.plan.producer<=47 && at==sites[active.plan.producer-44];
    }
    case 7: return at==0x1c3d3 || at==0x1c407;
    default: return false;
    }
}
bool needsHeldInput() {
    const auto worker=utilityWorker(active.script,active.plan.producer);
    if(!producer(worker,active.plan.producer,active.plan.entry)) return false;
    const auto at=pc(worker);
    switch(active.target.pushControl) {
    case 1: return at>=0x12ab && at<0x1b42;
    case 2: return at>=0x274b && at<0x2c17;
    case 3: return at>=0xd17c && at<0xd346;
    case 4: return at>=0xe2ad && at<0xe2d5;
    case 7: return active.plan.phase==1 && at>=0x1c3d3 && at<0x1c407;
    default: return false;
    }
}
void entered(Event& e) { e.before[0]=interpreter; e.before[1]=interpreterEpoch; interpreter=e.args[0]; interpreterEpoch=generation(); }
void left(Event& e) { interpreter=static_cast<uintptr_t>(e.before[0]); interpreterEpoch=static_cast<unsigned>(e.before[1]); }
void disabled(Event& e) {
    if(!armed) return;
    if(ownsDisable(e)) {
        active.signal=read<uintptr_t>(interpreter+0x90); active.owned=active.started=true; active.contact=false; stopAutoWalk();
        // The same interpreter reads its direction immediately after disabling
        // control. Replace approach input before that read, not next frame.
        if(active.target.pushControl<=2) {
            suppress(pedestalInputMask(0xf000)|active.consent);
            inject(active.directions[active.segment]);
        }
        if(active.target.pushControl==7 && pc(interpreter)==0x1c407) {
            active.committed=true; if(published) suppress(published); published=0;
        }
    }
    else stop("unrelated player-control disable");
}
void restored(Event&) { if(armed) active.owned=false; }
void text(Event&) { stop("dialogue"); }
void collected(Event&) { published=0; }
void requestStarted(Event& e) {
    if(!armed || active.epoch!=generation() || read<uintptr_t>(e.args[0]+4)!=active.script.chunk) return;
    const auto local=read<uint16_t>(e.args[0]+0x2e,0xffff),entry=read<uint16_t>(e.args[1]+8,0xffff);
    if(active.target.pushControl==7 && active.plan.phase==0 && local==39 && entry==6) stop("entrance rail push launched");
    else if(active.held && (active.target.pushControl==5 || active.target.pushControl==6) &&
        local==active.plan.producer && entry==active.plan.entry) active.started=true;
}
void finishing(Event& e) {
    e.before[0]=0;
    if(!armed || active.epoch!=generation()) return;
    using Worker=uintptr_t(__cdecl*)(int);
    const auto worker=reinterpret_cast<Worker>(address(0x46a7c0))(static_cast<int>(e.args[0]));
    e.before[0]=read<uintptr_t>(worker+4); e.before[1]=read<uint16_t>(worker+0x2e,0xffff);
    e.before[2]=read<uint16_t>(read<uintptr_t>(worker+0x90)+8,0xffff);
}
void finished(Event& e) {
    if(!armed || !active.started || active.epoch!=generation() || static_cast<uintptr_t>(e.before[0])!=active.script.chunk) return;
    const unsigned local=static_cast<unsigned>(e.before[1]),entry=static_cast<unsigned>(e.before[2]);
    if(active.target.pushControl==7 && active.plan.phase==1 && local==38 && entry==15 && !active.committed) {
        active.started=active.owned=false; active.signal=0; return;
    }
    const bool motion=active.target.pushControl==4 && local==1 && (entry==10 || entry==12);
    const bool once=(active.target.pushControl==5 || active.target.pushControl==6 || active.target.pushControl==7) &&
        local==active.plan.producer && entry==active.plan.entry;
    if(motion || once) {
        output(active.target.pushControl==6?L"Pedestal activated":L"Pedestal push complete",false,"pedestal-push");
        stop("native push completed");
    }
}
bool approach() {
    const auto player=playerPosition();
    if(!active.target.zone.crossing && inInteractionZone(active.target.zone,player)) {
        stopAutoWalk(); active.approaching=false; faceNavigationTarget(active.target); return true;
    }
    if(!autoWalkActive() && !active.approaching) active.approaching=requestNavigationAssistRoute();
    active.approaching=active.approaching || autoWalkActive(); return false;
}
void freshTarget(Point position) {
    const auto previous=active.target.position;
    active.target.position=position;
    if(active.crossing) return;
    // Known trial staging/contact shapes are rigid offsets from this actor.
    // Only horizontal movement is retained until the next owned observation;
    // vertical changes or a turn require a fresh qualified shape.
    if(std::abs(position.y-previous.y)>.1) {active.target.zone.valid=false;active.waitingShape=true;}
    else {
        const Point delta{position.x-previous.x,0,position.z-previous.z};
        for(auto* point:{&active.target.zone.center,&active.target.zone.end,&active.target.zone.obstacle}) {point->x+=delta.x;point->z+=delta.z;}
    }
    nav::requestNavigationScene();

}
}
PedestalNavigationState capturePedestalNavigationState() {
    PedestalNavigationState s;if(!nativeGameThread() || !armed || active.epoch!=generation()) return s;
    s.armed=true;s.epoch=active.epoch;s.control=active.target.pushControl;s.local=active.target.pushLocal;
    s.id=active.target.id;s.worker=active.target.worker;s.held=active.held;s.contact=active.contact;s.crossing=active.crossing;
    if(active.segment>active.plan.moves.size()) return {};
    s.plan=active.plan;s.plan.moves.erase(s.plan.moves.begin(),s.plan.moves.begin()+active.segment);return s;
}
void cancelPedestal() {cancelRevision.fetch_add(1,std::memory_order_acq_rel);if(nativeGameThread()) {observedCancel=cancelRevision.load(std::memory_order_acquire);stop("selection or context changed");}else {armed=false;manual=true;publishPedestalConsent(0);}}
bool pedestalActiveCrossing(const Entity& target) {
    return armed && active.epoch==generation() && active.crossing && target.pushControl==7 &&
        target.pushLocal==active.target.pushLocal && target.worker==active.target.worker && target.id==active.target.id;
}
bool pedestalActiveContact(const Entity& target) {
    return armed && active.epoch==generation() && active.contact && target.pushControl==active.target.pushControl &&
        target.pushLocal==active.target.pushLocal && target.worker==active.target.worker && target.id==active.target.id;
}
bool pedestalActivePlan(const Entity& target,PedestalPlan& plan) {
    if(!armed || active.epoch!=generation() || target.pushControl!=active.target.pushControl ||
        target.pushLocal!=active.target.pushLocal || target.worker!=active.target.worker || target.id!=active.target.id) return false;
    plan=active.plan; plan.moves.erase(plan.moves.begin(),plan.moves.begin()+active.segment); return true;
}
void armPedestalOwned(const NativeRouteTarget& target,const UtilityScript& script,const PedestalPlan& plan) {
    if(!nativeGameThread()) return;
    stop("new target");
    observedCancel=cancelRevision.load(std::memory_order_acquire);
    if(!target.pushControl || !setting(Setting::AutoWalk)) return;
    try {
        const auto consent=plan.moves.empty()?0:plan.moves.front().mode==PushMode::Single?pedestalConfirmMask():0x40;
        if(plan.complete || !plan.blocked.empty() || plan.moves.empty() || !target.zone.valid || target.inactive || !consent) return;
        active.target=target;active.script=script;active.plan=plan; active.epoch=generation(); active.consent=consent;
        unsigned index=0;for(const auto& move:plan.moves) {
            const auto bits=pedestalInputMask(move.held);
            if(move.held && pedestalLogicalInput(bits)!=move.held) {stop("invalid inverse input mapping");return;}
            active.directions[index++]=bits;
        }
        active.pawn=read<uintptr_t>(address(0xf00740));
        if(!active.pawn || active.pawn!=playerActor() || !read<uint16_t>(active.pawn+2)) {stop("controlled actor unavailable");return;}
        if(!publishPedestalConsent(consent)) {stop("consent bindings unavailable");recordLog("pedestal consent bindings unavailable");return;}
        active.last=target.position; active.lastPlayer=playerPosition(); active.progress=GetTickCount64(); armed=true;
    }catch(...) {stop("unavailable verified state");}
}
void armPedestal(const Entity& target) {
    UtilityScript script;if(!pedestalScript(target.pushControl,script)) {stop("new target");return;}
    try {OwnedNavTarget copied;if(!captureOwnedNavTarget(target,copied)) return;const auto plan=currentPedestalPlan(target);armPedestalOwned(copied.native,script,plan);}catch(...) {stop("unavailable verified state");}
}
void updatePedestalOwnedTarget(const NativeRouteTarget& target) {
    if(!nativeGameThread() || !armed || active.epoch!=generation() || target.worker!=active.target.worker || target.id!=active.target.id || target.pushControl!=active.target.pushControl || target.pushLocal!=active.target.pushLocal) return;
    active.target=target;
    if(target.zone.valid && !target.inactive) active.waitingShape=false;
}
void updatePedestalInput() {
    const auto revision=cancelRevision.load(std::memory_order_acquire);if(revision!=observedCancel) {observedCancel=revision;stop("selection or context changed");}
    const auto raw=read<uint16_t>(address(pedestal_rva::Held));
    const auto drained=drain; drain&=raw; if(drained) suppress(drained);
    if(!armed) return;
    const bool wasHeld=active.held;
    active.held=(raw&active.consent)!=0;
    if(!active.held) {
        if(wasHeld || active.owned || active.started || active.single || active.stopping) { stop("push action released"); return; }
        active.approaching=autoWalkActive(); return;
    }
    suppress(active.consent);
    const auto logical=pedestalLogicalInput(raw&~active.consent);
    if(logical&0x0fff) { stop("manual native action"); return; }
    if(active.epoch!=generation() || !context() || manual.exchange(false)) { stop("context or manual input"); return; }
    const auto now=GetTickCount64();
    if(active.previous && now-active.previous>600) { stop("input interrupted"); return; }
    active.previous=now;
    try {
        UtilityScript current;
        if(!pedestalScript(active.target.pushControl,current) || current.chunk!=active.script.chunk) { stop("map changed"); return; }
        PedestalSnapshot snapshot;if(!capturePedestalSnapshot(active.script,active.target,snapshot)) {stop("unavailable native push facts");return;}
        const auto live=pedestalPlan(active.target.pushControl,active.target.pushLocal,snapshot);
        if(distanceXZ(snapshot.position,active.last)>.1) { active.last=snapshot.position; active.progress=now; }
        const auto player=playerPosition();
        if(distanceXZ(player,active.lastPlayer)>.1) { active.lastPlayer=player; active.progress=now; }
        if(now-active.progress>15000) { stop("no progress",true); return; }
        if(live.complete && active.target.pushControl<=3) {
            output(L"Pedestal push complete",false,"pedestal-push"); stop("destination reached"); return;
        }
        if(!live.blocked.empty() && (!active.started || active.plan.moves[active.segment].mode==PushMode::Slide)) {
            output(live.blocked.data(),false,"pedestal-push");stop("state guard changed");return;
        }
        const bool walking=autoWalkActive();
        if((logical&0xf000) && !walking && !active.approaching) { stop("manual directional input"); return; }
        auto& move=active.plan.moves[active.segment];
        if(move.mode==PushMode::Slide && distanceXZ(snapshot.position,move.goal)<.8) {
            active.stopping=true;
            if(!fieldControl()) return; // Native ending animation owns control.
            if(active.segment+1==active.plan.moves.size()) {
                output(L"Pedestal push complete",false,"pedestal-push"); stop("destination reached"); return;
            }
            ++active.segment; active.started=active.stopping=active.contact=active.approaching=false; injected=0;
            freshTarget(snapshot.position);
            active.target.zone.valid=false;
            active.waitingShape=true;
            requestNavigationAssistInstruction();
        }
        if(active.owned) {
            const auto worker=utilityWorker(active.script,active.plan.producer);
            if(read<uintptr_t>(worker+0x90)!=active.signal || !producer(worker,active.plan.producer,active.plan.entry)) {
                stop("native push signal changed"); return;
            }
            if(!active.stopping && needsHeldInput()) inject(active.directions[active.segment]);
            return;
        }
        if(!fieldControl()) { stop("player control unavailable"); return; }
        if(active.started && move.mode==PushMode::Scripted) return;
        freshTarget(snapshot.position);
        if(active.waitingShape) return; // Only synthetic assistance waits for copied facts; original processing continues.
        if(active.target.inactive || !active.target.zone.valid) { stop("push target inactive",true); return; }
        if(!approach()) return;
        const auto& next=active.plan.moves[active.segment];
        if(active.target.pushControl<=2 && !active.started) {
            if(!active.contact) {
                const auto radius=active.target.zone.contactRadius,height=active.target.zone.height;
                active.target.zone={};active.target.zone.shape=1;active.target.zone.center=active.target.position;
                active.target.zone.range=radius-.15;active.target.zone.height=height;active.target.zone.valid=std::isfinite(radius) && radius>.5;
                if(!active.target.zone.valid) {stop("contact target unavailable",true);return;}
                active.contact=true;
                output(L"Pushing pedestal",false,"pedestal-push");
                approach();
            }
            return;
        }
        if(next.mode==PushMode::Crossing) {
            if(!active.crossing) {
                nav::requestNavigationScene();active.target.zone.valid=false;active.waitingShape=true;
                active.crossing=true; approach();
            }
            return;
        }
        if(next.mode==PushMode::Single) {
            if(!active.single) { inject(active.consent&(~active.consent+1u)); active.single=true; }
        } else {
            if(!active.started && !injected) output(L"Pushing pedestal",false,"pedestal-push");
            inject(active.directions[active.segment]);
        }
    } catch (...) { stop("unreadable native push state"); }
}
void pedestalPlayerInput(unsigned vk) {
    if(armed && !pedestalConsentVk(vk) && (vk==VK_ESCAPE || vk==VK_UP || vk==VK_DOWN || vk==VK_LEFT || vk==VK_RIGHT ||
        vk=='W' || vk=='A' || vk=='S' || vk=='D')) manual=true;
}
void pedestalKeyboardInput(const uint8_t* keys) {
    if(!armed) return;
    for(unsigned scan:{0x11u,0x1eu,0x1fu,0x20u,0xc8u,0xd0u,0xcbu,0xcdu,0x01u})
        if((keys[scan]&0x80) && !pedestalConsentKey(scan)) manual=true;
    for(unsigned scan=1;scan<256;++scan) if((keys[scan]&0x80) && pedestalMovementKey(scan)) manual=true;
}
void pedestalControllerInput(const pad::State& state) {
    if(!armed) return;
    const auto consent=pedestalConsentButtons();
    if((state.held&~consent) || std::abs(int(state.lx))>=5000 || std::abs(int(state.ly))>=5000 ||
        std::abs(int(state.rx))>=5000 || std::abs(int(state.ry))>=5000 ||
        (state.lt>=30 && !(consent&pad::bit(pad::LT))) || (state.rt>=30 && !(consent&pad::bit(pad::RT)))) manual=true;
}
void installPedestal() {
    observedCancel=cancelRevision.load(std::memory_order_acquire);
    hook(0x471d10,collected,nullptr);
    hook(pedestal_rva::Interpreter,entered,left);
    hook(0x471980,disabled,nullptr); hook(0x471a50,nullptr,restored);
    hook(0x469040,nullptr,requestStarted); hook(0x469170,finishing,finished);
    hook(0x4b8980,text,nullptr);
}
#ifdef FFX_READER_TEST
bool pedestalTargetForTest(NativeRouteTarget& target) {if(!nativeGameThread() || !armed || active.epoch!=generation()) return false;target=active.target;return true;}
#endif
}
