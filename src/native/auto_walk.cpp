#include "input_device.hpp"
// Port of the user's FFXII navigation/auto_walk.cpp: camera-relative input,
// same-poll manual priority, 250 ms stale mask, 600 ms interrupted field gap,
// line rejoin, octant hysteresis, bounded unstick and 15 s progress cap.
#include "auto_walk.hpp"
#include "nav_audio.hpp"
#include "settings.hpp"
#include "mod_menu.hpp"
#include "utilities.hpp"
#include "movement_keys.hpp"
#include "pedestal.hpp"
#include "field_controls.hpp"
#include "text_plan.hpp"
#include "swimming_navigation.hpp"
#include "game_bindings.hpp"
#include "controller.hpp"
#include <atomic>
#include <cmath>
#include <algorithm>
namespace ffx {
#ifdef FFX_READER_TEST
uint64_t autoWalkClock();
#endif
namespace {
std::atomic<NavigationKeyboardAssist> keyboardAssist{nullptr};
std::atomic<NavigationControllerAssist> controllerAssist{nullptr};
std::atomic<bool> engaged{false},cancel{false};
std::atomic<unsigned> manualRevision{1};
std::atomic<unsigned> mask{0},maskEpoch{0};
std::atomic<unsigned> maskCollisionSerial{0};
std::atomic<uint64_t> stamp{0};
unsigned epoch=0;
uint64_t previous=0,octantSince=0,improved=0,movingSince=0,unstickUntil=0,collisionPendingSince=0;
int octant=-1;
bool unstickRight=false;
double best=-1;
Point last;
FieldControlsSnapshot messageGuard;
std::array<unsigned,4> scans{0xc8,0xcb,0xd0,0xcd};
unsigned diveScan=0; int diveButton=-1;
std::array<unsigned,8> swimManualScans{};
constexpr unsigned octMasks[]={1,9,8,12,4,6,2,3}; // forward, left, backward, right
uint64_t nowMs() {
#ifdef FFX_READER_TEST
    return autoWalkClock();
#else
    return GetTickCount64();
#endif
}
bool safe() {
    // Archived 86C840 reports active dialogue, as used by trial utilities.
    using Mode=int(__cdecl*)();
    // Common 005E -> 871980 clears controller bit 2 and controlled actor;
    // Common 005D -> 871A50 restores them. This includes silent cutscenes.
    const auto fieldController=read<uintptr_t>(address(0xf26ae8));
    return setting(Setting::AutoWalk) && gameFocused() && !scene && !inBattle() && !modMenuActive() && !utilityConsoleActive() &&
        fieldController && (read<uint8_t>(fieldController)&2) && read<uintptr_t>(address(0xf00740)) &&
        reinterpret_cast<Mode>(address(0x46c840))()==0;
}
void stop(const char* reason,bool speak=false) {
    const bool was=engaged.exchange(false); mask=0; cancel=false; stamp=0;messageGuard={};
    if(was) { recordLog("auto walk stopped: %s",reason); if(speak) output(L"Auto walk stopped",false,"auto-walk"); }
}
bool messageAllowsAssist() {
    if(!messageGuard.candidate) return true;
    const auto state=classifyFieldControls(messageGuard);
    if(state==FieldControlsClassification::Pending) return false;
    if(state==FieldControlsClassification::Blocking) {stop("dialogue");return false;}
    messageGuard={};return true;
}
unsigned command() {
    if(!messageAllowsAssist() || !engaged || cancel || maskEpoch!=generation() || maskCollisionSerial!=navCollisionSerial() || !safe() || nowMs()-stamp.load()>250) return 0;
    return mask.load();
}
void textOpened(Event& e) {
    const auto slot=static_cast<unsigned>(e.args[0]);
    if(slot<8) {
        // Messages already copied the localized page on this same return.
        // Its worker classifies that owned packet. While pending, only synthetic
        // assistance is withheld; original/manual native input stays untouched.
        const auto window=read<uintptr_t>(address(0x14676b0)+slot*4);
        messageGuard=fieldControlsForSource(slot,read<uintptr_t>(window+0x94));
        if(messageGuard.candidate) {messageAllowsAssist();return;}
    }
    stop("dialogue");
}
void controlDisabled(Event&) { stop("player control disabled"); }
}
void setNavigationInputAssist(NavigationKeyboardAssist keyAssist,NavigationControllerAssist padAssist) { keyboardAssist=keyAssist;controllerAssist=padAssist; }
void stopAutoWalk() {manualRevision.fetch_add(1,std::memory_order_acq_rel);if(nativeGameThread()) stop("cancelled");else {engaged=false;cancel=true;mask=0;stamp=0;}}
unsigned autoWalkInputRevision() {return manualRevision.load(std::memory_order_acquire);}
bool autoWalkActive() { return engaged && !cancel; }
void startAutoWalk() {
    stop("new route");
    NavLeg leg;
    if(!safe() || !navLeg(leg) || leg.pendingCollision || leg.arrived || leg.target.inactive) return;
    // Ordinary octant/hysteresis steering can leave a narrow butterfly detour.
    // This timed activity retains manual movement and the verified route cues.
    if(leg.target.butterfly) { output(L"Follow the butterfly route manually",false,"auto-walk"); return; }
    diveScan=configuredActionKeyboardScan(5);
    for(unsigned i=0;i<4;++i) {
        const auto keys=configuredActionKeyboardScans(i+4);
        swimManualScans[i*2]=keys[0]; swimManualScans[i*2+1]=keys[1];
    }
    diveButton=configuredActionControllerButton(5);
    if(leg.target.swimDepth && ((nativeInputUsesController() && diveButton<0) || (!nativeInputUsesController() && !diveScan))) {
        output(L"Auto walk unavailable: Dive has no configured control",false,"auto-walk"); return;
    }
    epoch=generation(); maskEpoch=epoch; previous=0; octant=-1; best=-1;collisionPendingSince=0;
    const auto now=nowMs(); improved=movingSince=now; unstickUntil=0;
    last=playerPosition(); cancel=false; engaged=true;
    scans=movementKeys();
    TextPlan announcement;prepareTextPlan(announcement,"auto-walk",false);
    announcement.headline=planConcat(announcement,planLiteral(announcement,L"Auto walking to "),planLiteral(announcement,leg.target.name.data()));
    commitTextPlan(announcement);
    updateAutoWalk();
}
void updateAutoWalk() {
    if(!messageAllowsAssist()) {mask=0;return;}
    if(!engaged) return;
    const auto now=nowMs();
    if(cancel || epoch!=generation() || !safe() || (previous && now-previous>600)) { stop("context or player input"); return; }
    previous=now;
    NavLeg leg;
    if(!navLeg(leg)) { stop("route lost",true); return; }
    // Withhold only synthetic input while the worker rebuilds a changed gate.
    // No wait occurs on this owner. An unavailable result stops immediately;
    // a pending result has the existing600ms field-recovery bound.
    if(leg.pendingCollision) {
        mask=0;
        if(!collisionPendingSince) collisionPendingSince=now;
        else if(now-collisionPendingSince>600) stop("collision route update unavailable",true);
        return;
    }
    if(collisionPendingSince) {collisionPendingSince=0;octant=-1;unstickUntil=0;movingSince=improved=now;best=-1;}
    maskCollisionSerial=leg.collisionSerial;
    if(leg.target.inactive) { stop("destination inactive",true); return; }
    if(!leg.arrived && leg.epoch!=epoch) { stop("map changed"); return; }
    const auto player=playerPosition();
    if(!std::isfinite(player.x) || !std::isfinite(player.y) || !std::isfinite(player.z)) { stop("unreadable player"); return; }
    const double delta=distanceXZ(player,last);
    if(delta>55.65) { stop("player relocated"); return; }
    if(leg.arrived) { if(!leg.target.butterfly && !leg.target.swimDepth) faceNavigationTarget(leg.target); stop("arrived"); return; }
    if(leg.target.swimDepth) {
        if(!swimmingGoalActive(leg.target) || player.y>250) { stop("swimming transition"); return; }
        const double depth=std::max(0.0,251-player.y)+distanceXZ(player,leg.final);
        if(best<0 || depth<best-1) { best=depth; improved=now; }
        else if(now-improved>=15000) { stop("no diving progress",true); return; }
        last=player;
        if(distanceXZ(player,leg.goal)<=.1) {mask=16;stamp=now;return;}

    }
    if(!leg.target.swimDepth) {
        if(delta>.1) { last=player; movingSince=now; }
        const double distance=distanceXZ(player,leg.final);
        if(best<0 || distance<best-1) { best=distance; improved=now; }
        else if(now-improved>=15000) { stop("no progress",true); return; }
    }
    if(!leg.target.swimDepth && now-movingSince>1800 && now>=unstickUntil) {
        if(leg.target.butterfly) { stop("butterfly route stalled",true); return; }
        refreshNavCollision();
        if(!navLeg(leg) || leg.arrived) { stop("replan unavailable",true); return; }
        unstickRight=!unstickRight; unstickUntil=now+700; movingSince=now;
        recordLog("auto walk bounded sidestep/replan at %.2f %.2f %.2f",player.x,player.y,player.z);
    }
    Point steer=leg.goal;
    const auto x=leg.goal.x-leg.start.x,z=leg.goal.z-leg.start.z,length2=x*x+z*z;
    if(length2>.001) {
        const auto t=std::clamp(((player.x-leg.start.x)*x+(player.z-leg.start.z)*z)/length2,0.0,1.0);
        Point nearest{leg.start.x+t*x,player.y,leg.start.z+t*z};
        if(distanceXZ(nearest,player)>.35) {
            const auto lead=std::min(1.0,t+2/std::sqrt(length2));
            steer={leg.start.x+lead*x,player.y,leg.start.z+lead*z};
        }
    }
    double pan=0,front=0;
    if(!relativeBearing({steer.x-player.x,0,steer.z-player.z},pan,front)) { stop("camera unavailable"); return; }
    const auto degrees=std::atan2(pan,front)*180/3.14159265358979323846;
    int desired=(static_cast<int>(std::round(degrees/45))+8)&7;
    if(octant>=0 && desired!=octant) {
        auto difference=degrees-octant*45; while(difference>180) difference-=360; while(difference< -180) difference+=360;
        const int gap=std::abs(desired-octant);
        if(std::abs(difference)<=32.5 || ((gap==1 || gap==7) && now-octantSince<250)) desired=octant;
    }
    if(desired!=octant) { octant=desired; octantSince=now; }
    if(now<unstickUntil) desired=(desired+(unstickRight?2:6))&7;
    mask=octMasks[desired] | (leg.target.swimDepth?16u:0u); stamp=now;
}
void autoWalkPlayerInput(unsigned vk) {
    pedestalPlayerInput(vk);
    if(pedestalConsentVk(vk)) return;
    if(vk=='W' || vk=='A' || vk=='S' || vk=='D' || vk==VK_UP || vk==VK_DOWN || vk==VK_LEFT || vk==VK_RIGHT || vk==VK_ESCAPE || vk==VK_RETURN) {
        manualRevision.fetch_add(1,std::memory_order_acq_rel);
        cancel=true; mask=0;
    }
}
void autoWalkKeyboard(uint8_t* keys) {
    pedestalKeyboardInput(keys);
    bool physical=false;
    for(unsigned scan:{0x11u,0x1eu,0x1fu,0x20u,0xc8u,0xd0u,0xcbu,0xcdu,0x1cu,0x01u})
        if((keys[scan]&0x80) && !pedestalConsentKey(scan)) physical=true;
    for(unsigned scan:scans) if((keys[scan]&0x80) && !pedestalConsentKey(scan)) physical=true;
    if(physical) manualRevision.fetch_add(1,std::memory_order_acq_rel);
    if(const auto assist=keyboardAssist.load()) assist(keys);
    if(!engaged) return;
    // Raw physical buffer is checked before adding configured movement bits. Held-state
    // edge tracking must retain this original buffer, never injected keys.
    for(unsigned scan: {0x11u,0x1eu,0x1fu,0x20u,0xc8u,0xd0u,0xcbu,0xcdu,0x1cu,0x01u})
        if((keys[scan]&0x80) && !pedestalConsentKey(scan)) { cancel=true; mask=0; return; }
    const auto bits=command();
    for(unsigned i=0;i<scans.size();++i) if(keys[scans[i]]&0x80) { cancel=true; mask=0; return; }
    if(mask.load()&16) for(auto scan:swimManualScans) if(scan && (keys[scan]&0x80)) { cancel=true; mask=0; return; }
    if(bits&16) {
        if(diveScan) keys[diveScan]|=0x80; else { cancel=true; mask=0; }
    }
    for(unsigned i=0;i<scans.size();++i) if(bits&(1u<<i)) keys[scans[i]]|=0x80;
}
void autoWalkController(pad::State& state) {
    const auto consent=pedestalConsentButtons();
    const bool physical=std::abs(int(state.lx))>=5000 || std::abs(int(state.ly))>=5000 ||
        std::abs(int(state.rx))>=5000 || std::abs(int(state.ry))>=5000 ||
        (state.lt>=30 && !(consent&pad::bit(pad::LT))) || (state.rt>=30 && !(consent&pad::bit(pad::RT))) || (state.held&~consent);
    if(physical) manualRevision.fetch_add(1,std::memory_order_acq_rel);
    if(const auto assist=controllerAssist.load()) assist(state);
    if(!engaged) return;
    const bool manual=std::abs(int(state.lx))>=5000 || std::abs(int(state.ly))>=5000 ||
        std::abs(int(state.rx))>=5000 || std::abs(int(state.ry))>=5000 ||
        (state.lt>=30 && !(consent&pad::bit(pad::LT))) || (state.rt>=30 && !(consent&pad::bit(pad::RT)));
    if(manual || (state.held&~consent)) { cancel=true; mask=0; return; }
    if(messageGuard.candidate && classifyFieldControls(messageGuard)==FieldControlsClassification::Pending) return;
    if(maskCollisionSerial!=navCollisionSerial()) return;
    const auto bits=command();
    if(bits&16) {
        if(diveButton<0) { cancel=true; mask=0; return; }
        if(diveButton==pad::LT) state.lt=255;
        else if(diveButton==pad::RT) state.rt=255;
        else state.held|=pad::bit(diveButton);
    }
    state.lx=static_cast<int16_t>((bits&8?24000:0)-(bits&2?24000:0));
    state.ly=static_cast<int16_t>((bits&1?24000:0)-(bits&4?24000:0));
}
namespace {
void movementModeBefore(Event& e) {
    e.before[0]=e.args[0]==read<uintptr_t>(address(0xf00740)) && read<uint8_t>(e.args[0]+0x185)!=e.args[1];
}
void movementModeAfter(Event& e) { if(e.before[0]) { stop("movement mode changed"); resetNavAudio(); } }
}
void installAutoWalk() {
    hook(0x42b260,movementModeBefore,movementModeAfter);
    // Inspect the installed source after the native language selection.
    // The native dialogue/control guard already blocks injection mid-call.
    hook(0x4b8980,nullptr,textOpened);
    hook(0x471980,controlDisabled,nullptr); // verified native player-control disable
}
}
