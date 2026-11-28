#include <atomic>
// Synthetic owner/input events only; never launches or connects to the game.
#include "../auto_walk.hpp"
#include "../nav_audio.hpp"
#include "../movement_keys.hpp"
#include "../settings.hpp"
#include "../text_plan.hpp"
#include "../field_controls.hpp"
#include "ready_speech_adapter.hpp"
#include <map>
#include <cstdio>
#include <cstdlib>
#include <cstring>
namespace ffx {
std::atomic<int> scene{0};
uint64_t clockValue=1000;
unsigned epoch=1,replans=0;
bool focused=true,battle=false,menu=false,console=false,known=true,enabled=true,camera=true;
bool swimmingHud=false,hudPending=false;
bool depthActive=true,padOwned=false;
bool nativeOwner=true;
bool nativeGameThread() { return nativeOwner; }
bool resourcesReady=true;
bool nativeReaderResourcesReady() noexcept { return resourcesReady; }
bool controllerOwned() { return padOwned; }
bool swimmingGoalActive(const NativeRouteTarget&) { return depthActive; }
int dialogue=0;
Point player,forward{0,0,1};
NavLeg leg;
uintptr_t memory=0;
std::map<unsigned,Handler> hooks;
std::vector<std::wstring> speech;
bool lastInterrupt=false;
uint64_t autoWalkClock() { return clockValue; }
int __cdecl mode() { return dialogue; }
void __cdecl caption(unsigned id,char* out,unsigned size) {
    const char* names[]={"Move Up","Move Left","Move Down","Move Right"};
    strcpy_s(out,size,id>=632 && id<636?names[id-632]:"Other");
}
uintptr_t address(unsigned rva) {
    if(rva==0x46c840) return reinterpret_cast<uintptr_t>(mode);
    if(rva==0x2414a0) return reinterpret_cast<uintptr_t>(caption);
    return memory+rva;
}
bool copy(uintptr_t p,void* out,size_t size) {
    __try { memcpy(out,reinterpret_cast<void*>(p),size); return true; } __except(1) { return false; }
}
std::wstring ascii(uintptr_t p,size_t) { auto s=reinterpret_cast<const char*>(p); return std::wstring(s,s+strlen(s)); }
bool hook(unsigned rva,Handler enter,Handler leave) { hooks[rva]=enter?enter:leave; return true; }
FieldControlsSnapshot fieldControlsForSource(unsigned slot,uintptr_t source) {
    FieldControlsSnapshot s;s.slot=slot;s.source=source;s.candidate=swimmingHud;return s;
}
FieldControlsClassification classifyFieldControls(const FieldControlsSnapshot&) {
    return hudPending?FieldControlsClassification::Pending:swimmingHud?FieldControlsClassification::Nonblocking:FieldControlsClassification::Blocking;
}
bool gameFocused() { return focused; }
bool inBattle() { return battle; }
bool modMenuActive() { return menu; }
bool utilityConsoleActive() { return console; }
bool consent=false;
bool pedestalConsentKey(unsigned scan) { return consent && scan==0x1c; }
bool pedestalConsentVk(unsigned vk) { return consent && vk==VK_RETURN; }
uint64_t pedestalConsentButtons() { return consent?pad::bit(pad::South):0; }
void pedestalKeyboardInput(const uint8_t*) {}
void pedestalControllerInput(const pad::State&) {}
void pedestalPlayerInput(unsigned) {}
int setting(Setting s) { return s==Setting::AutoWalk && enabled; }
unsigned generation() { return epoch; }
Point playerPosition() { return player; }
#ifndef FFX_NAV_CUES_INTEGRATION_TEST
double distanceXZ(Point a,Point b) { return std::hypot(a.x-b.x,a.z-b.z); }
#endif
bool relativeBearing(Point delta,double& pan,double& front) {
    const auto length=std::hypot(delta.x,delta.z);
    pan=length?(delta.x*forward.z-delta.z*forward.x)/length:0;
    front=length?(delta.x*forward.x+delta.z*forward.z)/length:1;
    return camera;
}
#ifndef FFX_NAV_CUES_INTEGRATION_TEST
bool navLeg(NavLeg& result) { result=leg;result.pendingCollision=result.collisionSerial!=navCollisionSerial(); return known; }
void resetNavAudio() {}
void refreshNavCollision() { ++replans; }
#endif
bool output(const std::wstring& text,bool interrupt,const char*) { lastInterrupt=interrupt;speech.push_back(text); return true; }
// Semantic-only fixture adapter: it checks bounded Literal/Concat recipes and
// emits synchronously through the fake output. It provides no queue, worker,
// Native heap, thread, or production latency evidence.
void prepareTextPlan(TextPlan& plan,const char* producer,bool interrupt) {
    plan=TextPlan{};plan.epoch=generation();plan.interrupt=interrupt;
    unsigned i=0;
    if(producer) while(i+1<sizeof(plan.producer) && producer[i]) {plan.producer[i]=producer[i];++i;}
    plan.producer[i]=0;
}
TextRef planLiteral(TextPlan& plan,const wchar_t* value) {
    if(!plan.valid || plan.count>=plan.nodes.size() || !value) {plan.valid=false;return 0;}
    TextPlanNode node;node.offset=plan.used;
    unsigned i=0;
    for(;i<2048 && value[i];++i) {
        if(plan.used+sizeof(wchar_t)>=plan.bytes.size()) {plan.valid=false;return 0;}
        std::memcpy(plan.bytes.data()+plan.used,value+i,sizeof(wchar_t));
        plan.used+=sizeof(wchar_t);node.length+=sizeof(wchar_t);
    }
    if(i==2048 && value[i]) {plan.valid=false;return 0;}
    plan.bytes[plan.used]=0;plan.nodes[plan.count]=node;return ++plan.count;
}
TextRef planConcat(TextPlan& plan,TextRef left,TextRef right) {
    if(!plan.valid || plan.count>=plan.nodes.size() || !left || !right || left>plan.count || right>plan.count) {
        plan.valid=false;return 0;
    }
    TextPlanNode node;node.op=TextPlanOp::Concat;node.left=left;node.right=right;
    plan.nodes[plan.count]=node;return ++plan.count;
}
bool commitTextPlan(const TextPlan& plan) {
    if(!plan.valid || plan.epoch!=generation() || !plan.headline || plan.headline>plan.count ||
        plan.count>plan.nodes.size() || plan.used>plan.bytes.size()) return false;
    std::array<std::wstring,TextPlanNodeLimit> rendered{};
    for(unsigned i=0;i<plan.count;++i) {
        const auto& node=plan.nodes[i];
        if(node.left>i || node.right>i) return false;
        if(node.op==TextPlanOp::Literal) {
            if(node.offset>plan.used || node.length>plan.used-node.offset || node.length%sizeof(wchar_t)) return false;
            rendered[i].resize(node.length/sizeof(wchar_t));
            if(node.length) std::memcpy(rendered[i].data(),plan.bytes.data()+node.offset,node.length);
        } else if(node.op==TextPlanOp::Concat) {
            rendered[i]=(node.left?rendered[node.left-1]:L"")+(node.right?rendered[node.right-1]:L"");
        } else return false;
    }
    return output(rendered[plan.headline-1],plan.interrupt,plan.producer);
}
unsigned faceCalls=0;
bool faceResult=true;
bool faceNavigationTarget(const NativeRouteTarget&) { ++faceCalls; return faceResult; }
void log(const char*,...) {}
}
using namespace ffx;
unsigned resumes=0;std::wstring resumedName;
bool onResume(const NativeRouteTarget& target) {++resumes;resumedName=target.name.data();return true;}
// Resume assertions use native updates without advancing the synthetic clock.
void frames(unsigned count) {for(unsigned i=0;i<count;++i) updateAutoWalk();}
void check(bool value,const char* reason) { if(!value) { std::fprintf(stderr,"FAIL: %s\n",reason); std::exit(1); } }
std::array<uint8_t,256> keyboard() { std::array<uint8_t,256> keys{}; autoWalkKeyboard(keys.data()); return keys; }
void begin(Point destination={0,0,100}) {
    navRouteStopReported=false;
    stopAutoWalk(); player={}; forward={0,0,1}; known=camera=focused=enabled=true;
    menu=console=battle=false;resourcesReady=true; scene=dialogue=0; clockValue+=1000;
    *reinterpret_cast<uintptr_t*>(address(0xf26ae8))=address(0x1000000);
    *reinterpret_cast<uint8_t*>(address(0x1000000))=2;
    *reinterpret_cast<uintptr_t*>(address(0xf00740))=address(0x1001000);
    leg={player,destination,destination,{},epoch,false};
    check(wcscpy_s(leg.target.name.data(),leg.target.name.size(),L"Save Sphere")==0,"bounded route target name");
    startAutoWalk();
}
int main() {
    memory=reinterpret_cast<uintptr_t>(VirtualAlloc(nullptr,0x2100000,MEM_COMMIT|MEM_RESERVE,PAGE_READWRITE));
    check(memory!=0,"isolated native keyboard configuration");
    installAutoWalk();
    begin();speech.clear();Event stopEvent;hooks.at(0x4b8980)(stopEvent);updateAutoWalk();
    check(speech.size()==1 && speech.back()==L"Auto walk stopped: dialogue" && !lastInterrupt,"dialogue stop speaks its reason once without interrupting");
    begin();speech.clear();player={100,0,0};updateAutoWalk();updateAutoWalk();
    check(speech.size()==1 && speech.back()==L"Auto walk stopped: player relocated","relocation stop speaks once");
    begin();speech.clear();++epoch;updateAutoWalk();updateAutoWalk();
    check(speech.empty(),"map transition leaves speech to the map announcement");
    begin();speech.clear();stopAutoWalk();updateAutoWalk();check(speech.empty(),"explicit toggle cancellation stays silent");
    // Backslash during a walk stops it and says so, without reading or restarting the route.
    begin();speech.clear();check(toggleAutoWalkOff(),"Backslash during a walk is taken as the stop toggle");updateAutoWalk();updateAutoWalk();
    check(!autoWalkActive() && !keyboard()[0xc8] && speech.size()==1 && speech.back()==L"Auto walk stopped","the toggle stops the walk once and says Auto walk stopped");
    speech.clear();check(!toggleAutoWalkOff() && speech.empty(),"with no walk active Backslash keeps its route command");
    begin();speech.clear();autoWalkPlayerInput(VK_ESCAPE);updateAutoWalk();
    check(speech.empty() && !autoWalkActive(),"manual player-input cancellation stays silent");
    begin();speech.clear();known=false;updateAutoWalk();updateAutoWalk();
    check(speech.size()==1 && speech.back()==L"Auto walk stopped: route lost" && !claimNavRouteStopSpeech(),"unavailable route speaks one specific stop and claims worker retirement feedback");
    stopAutoWalk(); check(!autoWalkActive(),"manual revision checks start with autowalk stopped");
    auto revision=autoWalkInputRevision(); std::array<uint8_t,256> manualKeys{}; manualKeys[0x11]=0x80;
    autoWalkKeyboard(manualKeys.data());
    check(autoWalkInputRevision()==revision+1,"physical W press increments stopped autowalk input revision");
    stopAutoWalk(); check(!autoWalkActive(),"manual revision checks remain stopped after keyboard input");
    revision=autoWalkInputRevision(); pad::State manualPad; manualPad.lx=6000; autoWalkController(manualPad);
    check(autoWalkInputRevision()==revision+1,"manual controller stick increments stopped autowalk input revision");
    stopAutoWalk(); check(!autoWalkActive(),"empty-poll revision check starts stopped");
    revision=autoWalkInputRevision(); manualKeys={}; autoWalkKeyboard(manualKeys.data()); manualPad={}; autoWalkController(manualPad);
    check(autoWalkInputRevision()==revision,"empty keyboard and controller polls leave input revision unchanged");
    stopAutoWalk();
    begin(); check(keyboard()[0xc8]==0x80,"Backslash route injects FFX forward key");
    navCollisionRevision.fetch_add(1);
    check(!keyboard()[0xc8] && autoWalkActive(),"gate change withholds stale mask in the same keyboard poll");
    pad::State pendingPad;pendingPad.lx=2000;autoWalkController(pendingPad);
    check(pendingPad.lx==2000 && !pendingPad.ly,"pending collision preserves original small controller input");
    clockValue+=10;updateAutoWalk();check(autoWalkActive() && !keyboard()[0xc8],"collision rebuild pauses without blocking or retiring route");
    leg.collisionSerial=navCollisionSerial();leg.goal=leg.final={100,0,0};clockValue+=10;updateAutoWalk();
    check(autoWalkActive() && keyboard()[0xcd]==0x80 && !keyboard()[0xc8],"fresh gate detour resumes same route in new direction");
    begin();navCollisionRevision.fetch_add(1);clockValue+=10;updateAutoWalk();clockValue+=601;updateAutoWalk();
    check(!autoWalkActive() && !keyboard()[0xc8],"unavailable collision rebuild stops after bounded recovery");
    begin();
    forward={1,0,0}; clockValue+=300; updateAutoWalk(); check(keyboard()[0xcb]==0x80,"camera-relative left steering after camera turns");
    begin(); std::array<uint8_t,256> keys{}; keys[0xcd]=0x80; autoWalkKeyboard(keys.data());
    check(keys[0xcd]==0x80 && !keys[0xc8],"same-poll manual movement wins without an extra auto key");
    updateAutoWalk(); check(!keyboard()[0xc8],"manual cancellation never resumes itself");
    // Screen-reader modifiers: Insert 0xD2, Numpad Insert/0 0x52, Caps Lock 0x3A.
    begin(); keys={}; keys[0xd2]=0x80; keys[0xd0]=0x80; autoWalkKeyboard(keys.data());
    check(autoWalkActive() && !keys[0xd0] && keys[0xc8]==0x80,"Insert+Down reader command neither cancels nor steers auto walk");
    keys={}; keys[0xd0]=0x80; autoWalkKeyboard(keys.data());
    check(autoWalkActive() && !keys[0xd0],"reader-owned Down stays exempt after Insert is released first");
    keyboard(); keys={}; keys[0xd0]=0x80; autoWalkKeyboard(keys.data());
    check(!autoWalkActive() && keys[0xd0]==0x80,"a fresh Down press without a reader modifier still cancels");
    begin(); keys={}; keys[0x3a]=0x80; keys[0x1c]=0x80; autoWalkKeyboard(keys.data());
    check(autoWalkActive() && !keys[0x1c],"Caps Lock+Enter reader command does not cancel");
    begin(); keys={}; keys[0x52]=0x80; keys[0xc8]=0x80; keys[0x01]=0x80; autoWalkKeyboard(keys.data());
    check(autoWalkActive() && !keys[0x01],"Numpad Insert+Up+Escape reader command does not cancel");
    keyboard(); updateAutoWalk(); check(autoWalkActive(),"reader command leaves the walk running on the next tick");
    begin(); autoWalkReaderModifier(true); autoWalkPlayerInput(VK_DOWN); autoWalkPlayerInput(VK_RETURN); updateAutoWalk();
    check(autoWalkActive(),"message-path arrow and Enter with a reader modifier held do not cancel");
    autoWalkReaderModifier(false); autoWalkPlayerInput(VK_DOWN); updateAutoWalk();
    check(!autoWalkActive(),"message-path arrow without a reader modifier still cancels");
    stopAutoWalk(); keyboard(); revision=autoWalkInputRevision(); keys={}; keys[0xd2]=0x80; keys[0x11]=0x80; autoWalkKeyboard(keys.data());
    check(autoWalkInputRevision()==revision && keys[0x11]==0x80,"stopped reader command is not counted as manual movement and is left to the game");
    keys={}; autoWalkKeyboard(keys.data());
    begin(); pad::State state; autoWalkController(state); check(state.ly==24000 && !state.lx,"physical controller receives forward left-stick input");
    state={}; state.lx=18000; autoWalkController(state); check(state.lx==18000 && !state.ly,"manual stick remains intact and cancels auto walk");
    begin(); clockValue+=251; check(!keyboard()[0xc8],"stale input expires after 250 ms");
    clockValue+=400; updateAutoWalk(); check(!keyboard()[0xc8],"interrupted field event cancels instead of resuming");
    begin(); leg.arrived=true; updateAutoWalk(); check(!keyboard()[0xc8] && faceCalls==1,"central route arrival releases input and faces the interactible");
    {
        const auto talkTarget=[] {leg.target.zone.valid=true;leg.target.zone.talk=true;leg.target.zone.range=20;leg.target.zone.center={0,0,20};leg.arrived=true;};
        begin();talkTarget();speech.clear();faceCalls=0;faceResult=true;updateAutoWalk();
        check(autoWalkActive() && !keyboard()[0xc8] && !faceCalls,"arrival releases movement before facing");
        updateAutoWalk();updateAutoWalk();
        check(!autoWalkActive() && faceCalls==1 && speech.empty(),"the next native update faces once and retires the walk");
        begin();talkTarget();leg.target.pushControl=1;speech.clear();faceCalls=0;
        updateAutoWalk();check(!keyboard()[0xc8] && !faceCalls,"a pedestal receives no extra movement");
        updateAutoWalk();check(!autoWalkActive() && faceCalls==1 && speech.empty(),"a pedestal arrival ends silently after facing");
        begin();talkTarget();speech.clear();faceResult=false;updateAutoWalk();updateAutoWalk();
        check(!autoWalkActive() && speech.size()==1 && speech.back()==L"Auto walk stopped: not in interaction range","the owner speaks when facing fails its interaction check");
        faceResult=true;
    }
    begin();leg.pendingObservation=true;updateAutoWalk();
    check(autoWalkActive() && !keyboard()[0xc8],"pending copied route observation withholds movement");
    leg.pendingObservation=false;updateAutoWalk();
    check(autoWalkActive() && keyboard()[0xc8],"the next complete route observation resumes movement");
    begin(); Event e; hooks.at(0x4b8980)(e); check(!keyboard()[0xc8],"native dialogue installation releases input immediately");
    begin(); swimmingHud=true; hooks.at(0x4b8980)(e);
    check(keyboard()[0xc8]==0x80,"nonblocking swimming HUD refresh retains keyboard auto walk");
    state={}; autoWalkController(state); check(state.ly==24000,"swimming HUD retains controller auto walk");
    hudPending=true;hooks.at(0x4b8980)(e);
    check(!keyboard()[0xc8] && autoWalkActive(),"pending copied HUD classification withholds synthetic assistance without stopping the route");
    state={};state.lx=2000;autoWalkController(state);check(state.lx==2000,"pending copied HUD leaves even small original controller input untouched");
    hudPending=false;check(keyboard()[0xc8]==0x80,"owned nonblocking HUD result resumes the same route");
    swimmingHud=false; hooks.at(0x4b8980)(e);
    check(!keyboard()[0xc8],"real swimming dialogue still cancels the same route");
    begin(); hooks.at(0x471980)(e); check(!keyboard()[0xc8],"native silent cutscene control-disable event cancels immediately");
    {
        auto* control=reinterpret_cast<uint8_t*>(address(0x1000000));
        begin();speech.clear();*control=0;hooks.at(0x471980)(e);
        check(!autoWalkActive() && !keyboard()[0xc8] && speech.empty(),"the native takeover event cancels without competing with the game");
        ++epoch;updateAutoWalk();check(speech.empty(),"the map change owns subsequent speech");
        begin();speech.clear();*control=0;updateAutoWalk();
        check(!autoWalkActive() && speech.empty(),"an already disabled controller also cancels silently on the native update");
        *control=2;
        updateAutoWalk();check(!keyboard()[0xc8] && speech.empty(),"restored control does not resume a retired walk");
    }
    begin(); *reinterpret_cast<uint8_t*>(address(0x1000000))=0;
    check(!keyboard()[0xc8],"already disabled native field control cannot receive synthetic input");
    for(unsigned context=0;context<7;++context) {
        begin();speech.clear();
        if(context==0) focused=false; if(context==1) battle=true; if(context==2) menu=true;
        if(context==3) console=true; if(context==4) scene=3; if(context==5) dialogue=1; if(context==6) enabled=false;
        check(!keyboard()[0xc8],"context blocks injection in the same device poll"); updateAutoWalk();
        if(context==2 || context==3 || context==6) check(speech.empty(),"menu, console and disabled-setting cancellation stay silent");
        focused=enabled=true; menu=console=battle=false; scene=dialogue=0;
        check(!keyboard()[0xc8],"context cancellation requires a fresh destination command");
    }
    begin(); ++epoch; check(!keyboard()[0xc8],"map generation cancels stale input");
    begin(); player={100,0,0}; updateAutoWalk(); check(!keyboard()[0xc8],"large player relocation cancels route");
    begin(); camera=false; updateAutoWalk(); check(!keyboard()[0xc8],"unreadable camera cannot steer");
    begin(); autoWalkPlayerInput(VK_ESCAPE); check(!keyboard()[0xc8],"Escape stops walking");
    begin(); consent=true; autoWalkPlayerInput(VK_RETURN);
    keys={}; keys[0x1c]=0x80; autoWalkKeyboard(keys.data());
    check(keys[0xc8]==0x80,"armed push Confirm does not cancel approach steering");
    state={}; state.held=pad::bit(pad::South); autoWalkController(state);
    check(state.ly==24000,"only configured held controller Confirm is exempt during push approach");
    state={}; state.held=pad::bit(pad::South); state.lx=18000; autoWalkController(state);
    check(state.lx==18000 && !state.ly,"Confirm plus manual stick still cancels assisted approach");
    consent=false;
    begin(); const auto before=replans;
    for(unsigned i=0;i<19;++i) { clockValue+=100; updateAutoWalk(); }
    check(replans==before+1 && keyboard()[0xcd]==0x80,"blocked player replans and takes a bounded side step");
    for(unsigned i=0;i<133;++i) { clockValue+=100; updateAutoWalk(); }
    keys=keyboard(); check(!keys[0xc8] && !keys[0xcd] && speech.back()==L"Auto walk stopped: no progress","15-second no-progress cap releases all movement and speaks the reason");
    // Both assigned bindings and the exact native scan table are used.
    std::array<unsigned,100> config; config.fill(128);
    *reinterpret_cast<uintptr_t*>(address(0x1efb504))=reinterpret_cast<uintptr_t>(config.data());
    config[0x60/4+49]=101; *reinterpret_cast<uint8_t*>(address(0x830e90)+73)=0x4f;
    begin(); check(keyboard()[0x4f]==0x80 && !keyboard()[0xc8],"alternate numpad movement binding replaces default arrows");
    keys={}; keys[0x4f]=0x80; autoWalkKeyboard(keys.data()); check(!keys[0xc8],"remapped physical movement cancels in the same poll");
    begin(); leg.target.butterfly=true; startAutoWalk();
    check(!autoWalkActive(),"timed butterfly routes retain manual steering rather than unsafe octant detours");
    config[0x60/4+5]=102; config[0x60/4+6]=103; config[0x60/4+19]=13;
    *reinterpret_cast<uint8_t*>(address(0x830e90)+74)=0x2d;
    *reinterpret_cast<uint8_t*>(address(0x830e90)+75)=0x2e;
    begin(); leg.target.swimDepth=true;leg.final=leg.goal={0,251,0}; depthActive=true; startAutoWalk();
    check(keyboard()[0x2d]==0x80 && !keyboard()[0x4f],"depth goal injects configured Dive without horizontal movement");
    const auto priorReplans=replans;
    for(unsigned i=0;i<30;++i) { player.y+=2; clockValue+=100; updateAutoWalk(); }
    check(autoWalkActive() && replans==priorReplans,"vertical progress neither stalls nor produces horizontal sidesteps");
    padOwned=true; state={}; autoWalkController(state);
    check(state.held==pad::bit(pad::East) && !state.lx && !state.ly,"physical controller uses configured Dive face button");
    state={}; state.held=pad::bit(pad::West); autoWalkController(state);
    check(state.held==pad::bit(pad::West),"manual Return to ship wins in the same controller poll");
    padOwned=false; begin(); leg.target.swimDepth=true;leg.final=leg.goal={0,251,0}; startAutoWalk();
    keys={}; keys[0x2e]=0x80; autoWalkKeyboard(keys.data());
    check(keys[0x2e]==0x80 && !keys[0x2d],"manual Return to ship wins in the same keyboard poll");
    begin(); leg.target.swimDepth=true;leg.final=leg.goal={0,251,0}; startAutoWalk(); depthActive=false; updateAutoWalk();
    check(!autoWalkActive() && !keyboard()[0x2d],"native depth producer ending releases Dive immediately"); depthActive=true;
    begin(); leg.target.swimDepth=true;leg.final=leg.goal={0,251,0}; startAutoWalk(); player.y=250; clockValue+=100; updateAutoWalk();
    check(autoWalkActive(),"native threshold equality still requires Dive");
    player.y=250.01; clockValue+=100; updateAutoWalk(); check(!autoWalkActive(),"strict native depth completion releases Dive without facing a fake point");
    config[0x60/4+5]=128; config[0x60/4+54]=102;
    begin(); leg.target.swimDepth=true;leg.final=leg.goal={0,251,0}; startAutoWalk(); check(keyboard()[0x2d]==0x80,"unassigned primary Dive uses native alternate key");
    config[0x60/4+54]=128;
    begin(); leg.target.swimDepth=true;leg.final=leg.goal={0,251,0}; startAutoWalk(); check(!autoWalkActive(),"unassigned Dive does not inject an invented key");
    config[0x60/4+5]=102; config[0x60/4+19]=17;
    padOwned=true; begin(); leg.target.swimDepth=true;leg.final=leg.goal={0,251,0}; startAutoWalk(); state={}; autoWalkController(state);
    check(state.lt==255 && !state.held && !state.lx && !state.ly,"Dive remapped to a controller trigger uses the native trigger channel"); padOwned=false;
    config[0x60/4+55]=104; *reinterpret_cast<uint8_t*>(address(0x830e90)+76)=0x2f;
    begin(); leg.target.swimDepth=true;leg.final=leg.goal={0,251,0}; startAutoWalk(); keys={}; keys[0x2f]=0x80; autoWalkKeyboard(keys.data());
    check(!keys[0x2d] && keys[0x2f]==0x80,"alternate manual Return to ship cancels Dive in the same poll");
    begin(); leg.target.swimDepth=true;leg.final=leg.goal={0,251,0}; startAutoWalk();
    for(unsigned i=0;i<151;++i) { clockValue+=100; updateAutoWalk(); }
    check(!autoWalkActive() && !keyboard()[0x2d],"a stalled depth goal releases Dive after the normal progress cap");
    begin();leg.target.swimDepth=true;leg.final=leg.goal={40,251,0};startAutoWalk();
    auto combined=keyboard();check(combined[0x2d]==0x80 && combined[0xcd],"Underwater exit assistance holds native Dive and steers horizontally");
    padOwned=true;state={};autoWalkController(state);
    check(state.lt==255 && state.lx>0,"physical controller combines configured Dive trigger and southward-route steering");
    state={};state.ly=20000;autoWalkController(state);check(!state.lt,"manual swimming still cancels both injected channels in the same poll");padOwned=false;
    std::puts("PASS: swim depth Auto walk, configured keyboard/controller Dive, vertical progress, strict completion and manual cancellation.");
    // Battle resume: a walk stopped by a battle starts again toward the same destination.
    setAutoWalkResume(onResume);
    const auto battleStop=[&] {begin();resumes=0;battle=true;updateAutoWalk();check(!autoWalkActive(),"battle stops auto walk");frames(20);};
    battleStop();check(!resumes,"no resume while the battle lasts");
    battle=false;frames(1);check(resumes==1 && resumedName==L"Save Sphere","first safe native field update requests the same destination without a timer");
    frames(700);check(resumes==1,"a resume fires once");
    battleStop();uint8_t battleKeys[256]{};battleKeys[0xc8]=battleKeys[0x1c]=0x80;autoWalkKeyboard(battleKeys);autoWalkPlayerInput(VK_RETURN);
    battle=false;std::array<uint8_t,256> confirm{};confirm[0x1c]=0x80;autoWalkKeyboard(confirm.data());autoWalkPlayerInput(VK_RETURN);frames(1);
    check(resumes==1,"battle commands and confirming battle results do not cancel the resume");
    battleStop();battle=false;dialogue=1;frames(1);std::array<uint8_t,256> walk{};walk[0xc8]=0x80;autoWalkKeyboard(walk.data());dialogue=0;frames(1);
    check(!resumes,"walking after the battle cancels the resume");
    // Input in the first frame after the battle, before auto walk has seen it end, counts too.
    battleStop();battle=false;walk={};walk[0xc8]=0x80;autoWalkKeyboard(walk.data());walk={};autoWalkKeyboard(walk.data());frames(15);
    check(!resumes,"walking straight after the battle ends cancels the resume");
    // The resume belongs to the field the walk was interrupted in.
    auto& room=*reinterpret_cast<uint16_t*>(address(0xd2ca90));room=70;
    battleStop();room=71;battle=false;frames(15);room=70;
    check(!resumes,"a battle that returns to another room drops the resume");
    battleStop();battle=false;player={100,0,0};frames(15);
    check(!resumes,"a scripted relocation after the battle drops the resume");
    battleStop();battle=false;dialogue=1;frames(1);++epoch;room=71;dialogue=0;frames(1);room=70;
    check(!resumes,"a map change after the field is back drops the resume");
    battleStop();battle=false;dialogue=1;frames(1);++epoch;dialogue=0;frames(1);
    check(!resumes,"a new generation after battle end drops the resume even in the same room");
    battleStop();++epoch;battle=false;frames(15);
    check(resumes==1,"a field reload while the battle closes keeps the resume");
    battleStop();battle=false;autoWalkPlayerInput(VK_UP);frames(15);
    check(!resumes,"a window-message arrow straight after the battle cancels the resume");
    battleStop();battle=false;stopAutoWalk();frames(15);
    check(!resumes,"a navigation command straight after the battle cancels the resume");
    battleStop();walk={};walk[0xc8]=0x80;autoWalkKeyboard(walk.data());walk={};autoWalkKeyboard(walk.data());stopAutoWalk();battle=false;frames(15);
    check(resumes==1,"input and route teardown while the battle lasts keep the resume");
    battleStop();battle=false;pad::State stick;stick.ly=20000;autoWalkController(stick);frames(1);
    check(!resumes,"moving the stick after the battle cancels the resume");
    battleStop();battle=false;dialogue=1;frames(1);stopAutoWalk();dialogue=0;frames(1);
    check(!resumes,"a navigation command after the battle cancels the resume");
    battleStop();++epoch;suspendAutoWalk();battle=false;frames(1);
    check(resumes==1,"the battle's field reload keeps the resume");
    battleStop();battle=false;releaseAutoWalk();frames(1);
    check(resumes==1,"losing the route at the end of a battle keeps the resume");
    battleStop();releaseAutoWalk();battle=false;frames(15);
    check(resumes==1,"losing the route during a battle keeps the resume");
    begin();resumes=0;releaseAutoWalk();battle=true;frames(5);battle=false;frames(15);
    check(!resumes,"a walk ended by route loss is not resumed after a later battle");
    // Common 005E ends the walk immediately. Only the native battle flag can
    // turn that candidate into a resume, regardless of transition duration.
    auto& control=*reinterpret_cast<uint8_t*>(address(0x1000000));
    begin();resumes=0;Event disabled;hooks.at(0x471980)(disabled);control=0;clockValue+=120000;frames(50);battle=true;frames(20);battle=false;control=2;frames(1);
    check(resumes==1,"a walk stopped by control loss resumes after a slow battle transition");
    begin();resumes=0;hooks.at(0x471980)(disabled);control=0;
    battle=true;hooks.at(0x382730)(disabled);battle=false;control=2;hooks.at(0x471a50)(disabled);frames(1);
    check(resumes==1,"native battle completion confirms a candidate without intervening field updates");
    begin();resumes=0;hooks.at(0x471980)(disabled);control=0;frames(1);control=2;hooks.at(0x471a50)(disabled);battle=true;frames(1);battle=false;frames(1);
    check(!resumes,"native control restoration without a battle retires an authored-event candidate");
    begin();resumes=0;hooks.at(0x471980)(disabled);control=0;frames(1);control=2;frames(1);battle=true;frames(1);battle=false;frames(1);
    check(!resumes,"the first update with restored control also retires a nonbattle candidate");
    begin();resumes=0;hooks.at(0x471980)(disabled);control=0;++epoch;frames(1);battle=true;frames(1);battle=false;control=2;frames(1);
    check(!resumes,"an exit's map change retires the candidate before a later battle");
    begin();resumes=0;autoWalkPlayerInput(VK_UP);hooks.at(0x471980)(disabled);control=0;battle=true;frames(1);battle=false;control=2;frames(1);
    check(!resumes,"manual cancellation before the control-disable hook never arms a resume");
    battleStop();enabled=false;frames(1);enabled=true;battle=false;frames(15);
    check(!resumes,"turning auto walk off during the battle cancels the resume");
    begin();resumes=0;autoWalkPlayerInput(VK_ESCAPE);updateAutoWalk();battle=true;frames(5);battle=false;frames(15);
    check(!resumes,"a walk the player cancelled is not resumed after a battle");
    begin();resumes=0;speech.clear();Event opened;hooks.at(0x4b8980)(opened);updateAutoWalk();frames(40);battle=true;frames(5);battle=false;frames(15);
    check(!resumes,"dialogue does not arm a resume for a later battle");
    begin();resumes=0;++epoch;updateAutoWalk();frames(1);walk={};walk[0x11]=0x80;autoWalkKeyboard(walk.data());battle=true;frames(5);battle=false;frames(15);
    check(!resumes,"walking between the stop and the battle cancels the resume");
    begin();resumes=0;++epoch;updateAutoWalk();battle=true;frames(1);battle=false;frames(1);
    check(!resumes,"a resource change without a confirmed battle does not arm a resume");
    battleStop();battle=false;resourcesReady=false;frames(1);check(!resumes,"resume waits for native resource readiness");
    resourcesReady=true;frames(1);check(resumes==1,"first ready field update requests the route");
    battleStop();battle=false;dialogue=1;frames(30);check(!resumes,"resume waits while dialogue holds the field");
    dialogue=0;frames(12);check(resumes==1,"resume fires once dialogue ends");
    battleStop();battle=false;dialogue=1;frames(1);clockValue+=120000;frames(1);dialogue=0;frames(1);
    check(resumes==1,"elapsed wall time never expires a pending battle resume in the same field");
    battleStop();battle=false;autoWalkPlayerInput(VK_ESCAPE);frames(1);check(!resumes,"Escape cancels before the first safe field update");
    battleStop();battle=false;startAutoWalk();frames(1);check(!resumes,"an explicit new walk replaces a pending resume");
    setAutoWalkResume(nullptr);
    std::puts("PASS: auto walk resumes once after a battle and stays cancelled by player movement, commands and the setting.");
    VirtualFree(reinterpret_cast<void*>(memory),0,MEM_RELEASE);
    std::puts("PASS: auto walk steering, native remaps, manual priority, controller, arrival, context cancellation, stale input and bounded recovery.");
    return 0;
}
