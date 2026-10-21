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
bool controllerOwned() { return padOwned; }
bool swimmingGoalActive(const NativeRouteTarget&) { return depthActive; }
int dialogue=0;
Point player,forward{0,0,1};
NavLeg leg;
uintptr_t memory=0;
std::map<unsigned,Handler> hooks;
std::vector<std::wstring> speech;
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
double distanceXZ(Point a,Point b) { return std::hypot(a.x-b.x,a.z-b.z); }
bool relativeBearing(Point delta,double& pan,double& front) {
    const auto length=std::hypot(delta.x,delta.z);
    pan=length?(delta.x*forward.z-delta.z*forward.x)/length:0;
    front=length?(delta.x*forward.x+delta.z*forward.z)/length:1;
    return camera;
}
bool navLeg(NavLeg& result) { result=leg;result.pendingCollision=result.collisionSerial!=navCollisionSerial(); return known; }
void resetNavAudio() {}
void refreshNavCollision() { ++replans; }
bool output(const std::wstring& text,bool,const char*) { speech.push_back(text); return true; }
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
bool faceNavigationTarget(const NativeRouteTarget&) { ++faceCalls; return true; }
void log(const char*,...) {}
}
using namespace ffx;
void check(bool value,const char* reason) { if(!value) { std::fprintf(stderr,"FAIL: %s\n",reason); std::exit(1); } }
std::array<uint8_t,256> keyboard() { std::array<uint8_t,256> keys{}; autoWalkKeyboard(keys.data()); return keys; }
void begin(Point destination={0,0,100}) {
    stopAutoWalk(); player={}; forward={0,0,1}; known=camera=focused=enabled=true;
    menu=console=battle=false; scene=dialogue=0; clockValue+=1000;
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
    begin(); pad::State state; autoWalkController(state); check(state.ly==24000 && !state.lx,"physical controller receives forward left-stick input");
    state={}; state.lx=18000; autoWalkController(state); check(state.lx==18000 && !state.ly,"manual stick remains intact and cancels auto walk");
    begin(); clockValue+=251; check(!keyboard()[0xc8],"stale input expires after 250 ms");
    clockValue+=400; updateAutoWalk(); check(!keyboard()[0xc8],"interrupted field event cancels instead of resuming");
    begin(); leg.arrived=true; updateAutoWalk(); check(!keyboard()[0xc8] && faceCalls==1,"central route arrival releases input and faces the interactible");
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
    begin(); *reinterpret_cast<uint8_t*>(address(0x1000000))=0;
    check(!keyboard()[0xc8],"already disabled native field control cannot receive synthetic input");
    for(unsigned context=0;context<7;++context) {
        begin();
        if(context==0) focused=false; if(context==1) battle=true; if(context==2) menu=true;
        if(context==3) console=true; if(context==4) scene=3; if(context==5) dialogue=1; if(context==6) enabled=false;
        check(!keyboard()[0xc8],"context blocks injection in the same device poll"); updateAutoWalk();
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
    keys=keyboard(); check(!keys[0xc8] && !keys[0xcd] && speech.back()==L"Auto walk stopped","15-second no-progress cap releases all movement");
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
    VirtualFree(reinterpret_cast<void*>(memory),0,MEM_RELEASE);
    std::puts("PASS: auto walk steering, native remaps, manual priority, controller, arrival, context cancellation, stale input and bounded recovery.");
}
