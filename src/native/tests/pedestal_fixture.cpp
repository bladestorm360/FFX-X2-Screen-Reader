#include <atomic>
// Isolated native input/script callbacks. Never launches or attaches to the game.
#include "../pedestal.hpp"
#include "../nav_audio.hpp"
#include "../settings.hpp"
#include "../input_device.hpp"
#include "../owned_retirement.hpp"
#include "../owned_nav_target.hpp"
#include "ready_speech_adapter.hpp"
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <functional>
#include <cmath>
#include <map>
#include <new>
#include <thread>
namespace {
bool trackPedestalAllocations=false;
unsigned pedestalAllocations=0;
}
void* operator new(size_t size) {
    if(trackPedestalAllocations) ++pedestalAllocations;
    if(auto* p=std::malloc(size?size:1)) return p;
    throw std::bad_alloc();
}
void* operator new[](size_t size) { return ::operator new(size); }
void operator delete(void* p) noexcept { std::free(p); }
void operator delete[](void* p) noexcept { ::operator delete(p); }
void operator delete(void* p,size_t) noexcept { ::operator delete(p); }
void operator delete[](void* p,size_t) noexcept { ::operator delete[](p); }
namespace ffx {
void refreshSwimmingGoal(Entity& target) { target.inactive=true; }
uintptr_t base=0;
unsigned epoch=1,starts=0,faces=0;
unsigned fixtureConsent=0x40;
bool padOwned=false;
std::atomic<int> scene{0};int dialogue=0;
bool enabled=true,focused=true,battle=false,menu=false,console=false,walking=false,verified=true;
UtilityScript script;
std::map<unsigned,int> values;
std::array<unsigned,16> remap{};
struct Listener { Handler before,after; };
std::map<unsigned,std::vector<Listener>> hooks;
std::vector<std::wstring> spoken;
std::vector<Point> route;
int __cdecl mode() { return dialogue; }
uint16_t __cdecl nativeRemap(unsigned,unsigned,uint16_t raw) {
    unsigned mapped=0; for(unsigned i=0;i<16;++i) if(raw&(1u<<i)) mapped|=1u<<remap[i]; return static_cast<uint16_t>(mapped);
}
uintptr_t __cdecl worker(unsigned local) { return base+0x1000000+local*0x1000; }
uint16_t __cdecl entry(unsigned type,unsigned channel,uintptr_t) {
    const unsigned actor[]={2,3,4,5,0xffff,0xffff,0xffff},zone[]={2,3,0xffff,5,4,6,7};
    return static_cast<uint16_t>(channel<7?(type==1?actor[channel]:zone[channel]):0xffff);
}
uint8_t __cdecl priority(unsigned type,unsigned channel) { return channel>=7 || (type==1?channel>=4:channel==2)?255:1; }
uintptr_t address(unsigned rva) {
    if(rva==0x46c840) return reinterpret_cast<uintptr_t>(mode);
    if(rva==0x488bb0) return reinterpret_cast<uintptr_t>(nativeRemap);
    if(rva==0x46a7c0) return reinterpret_cast<uintptr_t>(worker);
    if(rva==0x476360) return reinterpret_cast<uintptr_t>(entry);
    if(rva==0x4767e0) return reinterpret_cast<uintptr_t>(priority);
    return base+rva;
}
bool copy(uintptr_t p,void* out,size_t size) {
    __try { memcpy(out,reinterpret_cast<void*>(p),size); return true; } __except(1) { return false; }
}
unsigned generation() { return epoch; }
bool delayRoute=false;
unsigned routeRequests=0;
bool nativeOwner=true;
bool nativeGameThread() {return nativeOwner;}
bool nativeAtelSpan(uintptr_t,AtelSpan&) {return false;} // This semantic host mocks utilities, not physical resource receipts.
bool readyAtelIdentity(uintptr_t,AtelIdentity&) {return false;}
bool readyAtelWorkerHeader(uintptr_t,unsigned,unsigned&) {return false;}
bool hook(unsigned rva,Handler before,Handler after) { hooks[rva].push_back({before,after}); return true; }
void log(const char*,...) {}
bool output(const std::wstring& text,bool,const char*) { spoken.push_back(text); return true; }
bool gameFocused() { return focused; }
bool inBattle() { return battle; }
bool modMenuActive() { return menu; }
bool utilityConsoleActive() { return console; }
bool controllerOwned() { return padOwned; }
std::wstring nativeBindingName(unsigned action,bool pad) {
    const auto config=read<uintptr_t>(address(0x1efb504));
    const auto id=read<unsigned>(config+0x60+action*4,128);
    if(pad) return id==13?L"B":id==12?L"A":L"";
    return id==80?L"Space":id==77?L"Enter":id==118?L"Z":id==127?L"Numpad Enter":L"";
}
int setting(Setting s) { return s==Setting::AutoWalk && enabled; }
UtilityScript utilityScript() { return script; }
bool captureUtilityScript(UtilityScript& out) noexcept {out=script;return true;}
uintptr_t utilityWorker(const UtilityScript&,unsigned local) { return worker(local); }
bool captureUtilityWorker(const UtilityScript&,unsigned local,uintptr_t& out) noexcept {if(!nativeOwner) return false;out=worker(local);return true;}
int utilityValue(const UtilityScript&,unsigned low,unsigned,unsigned) { return values[low]; }
bool captureUtilityValue(const UtilityScript&,unsigned low,unsigned,unsigned,int& out) noexcept {const auto row=values.find(low);out=row==values.end()?0:row->second;return true;}
const TrialUtility* currentTrial(const UtilityScript&) { static TrialUtility trial{}; return verified?&trial:nullptr; }
const TrialUtility* captureCurrentTrial(const UtilityScript& candidate) noexcept {return currentTrial(candidate);}
uintptr_t playerActor() { return base+0x1300000; }
Point playerPosition() { const auto p=playerActor(); return {read<float>(p+0xc),read<float>(p+0x10),read<float>(p+0x14)}; }
Mesh readMesh() { Mesh mesh; mesh.openPlane=true; mesh.fixedHeight=true; mesh.planeY=playerPosition().y; return mesh; }
bool faceNavigationTarget(const Entity&) { ++faces; return true; }
bool faceNavigationTarget(const NativeRouteTarget&) {++faces;return true;}
bool autoWalkActive() { return walking; }
void stopAutoWalk() { walking=false; }
void startAutoWalk() { walking=true; ++starts; }
void routeNavAudio(const std::vector<Point>& points,const Entity&) { route=points; }
// Semantic immediate handback adapter: production scheduling/data-age is
// exercised separately by navigation_service_fixture. This host only preserves
// original trial input/PC/consent cases using the real pure routing algorithm.
bool requestNavigationAssistRoute() {
    ++routeRequests;if(delayRoute) return true;
    NativeRouteTarget target;if(!pedestalTargetForTest(target)) return false;
    const auto state=capturePedestalNavigationState();const auto origin=playerPosition();const auto mesh=readMesh();
    if(state.crossing) {
        const auto crossing=interactionZone(worker(39),16);
        static_cast<InteractionShape&>(target.zone)=static_cast<const InteractionShape&>(crossing);
        target.zone.crossing=true;target.zone.talk=false;target.zone.height=2;
    }
    nativeOwner=false;OwnedNavTarget copied{};copied.native=target;Entity owned;
    const bool restored=restoreOwnedNavTarget(copied,owned);const auto points=restored?interactionRoute(mesh,origin,owned.zone):std::vector<Point>{};
    if(!points.empty()) routeNavAudio(points,owned);nativeOwner=true;
    updatePedestalOwnedTarget(target);if(points.empty()) {cancelPedestal();return false;}startAutoWalk();return true;
}
void requestNavigationAssistInstruction() {}
namespace nav {void requestNavigationScene() {}}
bool relativeBearing(Point delta,double& pan,double& front) { pan=delta.x; front=delta.z; return true; }
bool cameraBasis(Point& forward,Point& right) { forward={0,0,1}; right={1,0,0}; return true; }
std::wstring direction(Point p) { return std::abs(p.x)>std::abs(p.z)?p.x>0?L"east":L"west":p.z>0?L"north":L"south"; }
std::wstring join(const std::vector<std::wstring>& parts,const wchar_t* separator) {
    std::wstring out; for(const auto& p:parts) { if(!out.empty()) out+=separator; out+=p; } return out;
}
}
using namespace ffx;
void check(bool result,const char* reason) { if(!result) { std::fprintf(stderr,"FAIL: %s\n",reason); std::exit(1); } }
void dispatch(unsigned rva,bool before,Event& event) {
    event.rva=rva; for(const auto& listener:hooks[rva]) { const auto f=before?listener.before:listener.after; if(f) f(event); }
}
void position(uintptr_t actor,Point p) { write<float>(actor+0xc,static_cast<float>(p.x)); write<float>(actor+0x10,static_cast<float>(p.y)); write<float>(actor+0x14,static_cast<float>(p.z)); }
void control(bool on) { write<uint8_t>(script.controller,on?2:0); write<uintptr_t>(address(0xf00740),on?playerActor():0); }
void signal(unsigned local,unsigned callback,unsigned pc) {
    const auto w=worker(local),s=base+0x1400000+local*0x40;
    write<uintptr_t>(w+0x90,s); write<uint16_t>(s+8,static_cast<uint16_t>(callback)); write<uint8_t>(s+0xe,1); write<uint8_t>(w+0x32,1);
    write<uintptr_t>(w+0x12c+0x4c+0x18,script.chunk+script.codeOffset+pc);
}
void disable(unsigned local,unsigned callback,unsigned pc,bool ancestry=true) {
    signal(local,callback,pc);
    Event interpreterEvent; interpreterEvent.args[0]=worker(local);
    if(ancestry) dispatch(0x464180,true,interpreterEvent);
    Event disabled; disabled.parentRva=ancestry?0x464180:0;
    dispatch(0x471980,true,disabled); control(false);
    if(ancestry) dispatch(0x464180,false,interpreterEvent);
}
void restore() { control(true); Event e; dispatch(0x471a50,false,e); }
void complete(unsigned local,unsigned callback) {
    signal(local,callback,0); Event e; e.args[0]=local; dispatch(0x469170,true,e); dispatch(0x469170,false,e);
}
void request(unsigned local,unsigned callback) {
    signal(local,callback,0); Event e; e.args[0]=worker(local); e.args[1]=read<uintptr_t>(worker(local)+0x90);
    dispatch(0x469040,false,e);
}
unsigned frame(unsigned held=0xffffffff) {
    if(held==0xffffffff) held=fixtureConsent;
    Event e; dispatch(0x471d10,true,e);
    for(unsigned rva:{0xf27080u,0xf27084u,0xf27088u,0xf2708cu,0xf27090u,0xf27094u}) write<unsigned>(address(rva),held);
    updatePedestalInput(); return read<unsigned>(address(0xf27080));
}
Entity setup(unsigned controlKind,unsigned local,Point p) {
    cancelPedestal(); frame(0); ++epoch; values.clear(); spoken.clear(); route.clear(); walking=false;
    enabled=focused=verified=true; battle=menu=console=false; scene=dialogue=0;
    fixtureConsent=controlKind==5 || controlKind==6?0x20:0x40; padOwned=false;inputDeviceMark.store(0);observeInputDevice(false);
    for(unsigned i=0;i<16;++i) remap[i]=i;
    const char* names[]={"","bsyt0100","klyt1000","djyt0600","mcyt0500","djyt0600","dome0700","bvyt1100"};
    script={base+0x1200000,worker(0),base+0x1500000,base+0x1100000,names[controlKind],200000,0x2000,96};
    write<uintptr_t>(address(0xf26ae8),script.controller);
    for(unsigned i=0;i<96;++i) {
        const auto w=worker(i),h=base+0x1500000+i*0x40,a=base+0x1300000+i*0x880;
        write<uintptr_t>(w,h); write<uintptr_t>(w+4,script.chunk); write<uint16_t>(w+0x2e,static_cast<uint16_t>(i));
        write<uint16_t>(h+8,32); write<uint8_t>(h,4); write<uint16_t>(w+0x34,0x80); write<uint8_t>(w+0xaa,1); write<uint8_t>(w+0xab,0x7f);
        write<uintptr_t>(w+0x9c,a); write<uint16_t>(a+2,1);
        write<float>(a+0x5c,2); write<float>(a+0x64,2); write<float>(a+0x4e4,1);
        position(a,p);
        write<float>(w+0x558,static_cast<float>(p.x)); write<float>(w+0x55c,static_cast<float>(p.y)); write<float>(w+0x560,static_cast<float>(p.z));
        write<float>(w+0x558+0x1c,10); write<float>(w+0x558+0x2c,10); write<float>(w+0x558+0x40,5);
        write<float>(w+0x558+0x3c,3.1415927f);
    }
    write<uint8_t>(read<uintptr_t>(worker(local)),controlKind==5 || controlKind==6?3:1);
    position(playerActor(),{p.x,p.y,p.z+4.25}); control(true);
    const auto config=base+0x1600000; write<uintptr_t>(address(0x1efb504),config);
    for(unsigned i=0;i<128;++i) { write<unsigned>(config+0x60+i*4,0); write<uint8_t>(address(0x830e90)+i,static_cast<uint8_t>(i)); }
    const unsigned movement[]={0xc8,0xd0,0xcd,0xcb};
    for(unsigned i=0;i<4;++i) {
        write<unsigned>(config+0x60+i*4,i+29); write<uint8_t>(address(0x830e90)+i+1,static_cast<uint8_t>(movement[i]));
    }
    write<uint8_t>(address(0x830e90)+49,0x1c); write<uint8_t>(address(0x830e90)+52,0x39);
    write<uint8_t>(address(0x830e90)+90,0x2c); write<uint8_t>(address(0x830e90)+99,0x9c);
    write<unsigned>(config+0x60+4*4,77); write<unsigned>(config+0x60+53*4,127); write<unsigned>(config+0x60+18*4,12);
    write<unsigned>(config+0x60+5*4,80); write<unsigned>(config+0x60+54*4,118); write<unsigned>(config+0x60+19*4,13);
    Entity target; target.name=L"Pedestal"; target.worker=local; target.position=p;
    configurePedestalTarget(target,names[controlKind],local);
    return target;
}
void ready(Entity& target) { refreshTargetZone(target,worker(target.pushLocal),1); armPedestal(target); }
PedestalSnapshot snapshot(Point p,std::initializer_list<PedestalBindings::Value> vars) {return {p,PedestalBindings(vars)};}
PedestalPlan measuredPlan(unsigned control,unsigned local,const PedestalSnapshot& s,const char* reason) {
    pedestalAllocations=0;trackPedestalAllocations=true;
    const auto plan=pedestalPlan(control,local,s);
    trackPedestalAllocations=false;
    check(pedestalAllocations==0,reason);
    check(plan.moves.size()<=2,"production pedestal plan stays within two fixed move rows");
    return plan;
}
void ruleTests() {
    auto s=snapshot({-14.4117,40,-65.6},{{0x5c0000cc,1},{0x5c000100,0}});
    auto p=measuredPlan(1,7,s,"Besaid production plan allocates no heap"); check(p.moves.size()==1 && p.moves[0].goal.z< -147,"Besaid loaded pedestal routes to original depression");
    check(describePedestalPlan(p,s.position).find(L"7.4")!=std::wstring::npos,"Besaid guidance announces remaining slide distance");
    check(describePedestalApproach(p,s.position,{s.position.x+4,s.position.y,s.position.z})==
        L"You are on the east side of the pedestal. Push from the north side.",
        "push guidance distinguishes current side from required opposite side");
    check(describePedestalApproach(p,s.position,{s.position.x,s.position.y,s.position.z+4})==
        L"You are on the north side of the pedestal. Push from the north side.",
        "correct push-side guidance agrees with original Besaid motion");
    s.values[0x5c0000cc]=0; check(!pedestalPlan(1,7,s).blocked.empty(),"Besaid final push requires inserted sphere");
    s=snapshot({.2215,-10,144.1042},{{0x0c00005a,1},{0x0c000064,0}}); p=measuredPlan(2,6,s,"Kilika production plan allocates no heap");
    check(p.moves.size()==2 && p.moves[0].goal.x>50 && p.moves[1].goal.z>196,"Kilika turns before crossing original depression edge");
    check(describePedestalApproach(p,s.position,{s.position.x+4,s.position.y,s.position.z}).find(L"Push from the west side.")!=std::wstring::npos,
        "Kilika alignment begins on the west side before the northbound turn");
    s.position.x=50.01099; p=measuredPlan(2,6,s,"Kilika segmented production plan allocates no heap");
    check(describePedestalApproach(p,s.position,{s.position.x-4,s.position.y,s.position.z}).find(L"Push from the south side.")!=std::wstring::npos,
        "Kilika side guidance changes after the original alignment segment");
    s=snapshot({0,-18,24},{{0x0c000099,1},{0x0c00009a,1},{0x0c00009b,0},{0x0c00009d,0},{0x0c00009e,0},{0x0c0000d9,0},{0x0c0000db,0}});
    check(s.values.count==7,"Djose production snapshot contains all seven required facts");
    p=measuredPlan(3,1,s,"Djose production plan allocates no heap"); check(p.moves[0].goal.x==70 && p.moves[0].held==0x2000,"empty Djose charger push targets verified volume");
    s.position.x=70; check(pedestalPlan(3,1,s).complete,"empty Djose arrival does not depend on charged-sphere result");
    s.position.x=0; s.values[0x0c00009b]=3; s.values[0x0c00009d]=s.values[0x0c00009e]=1;
    p=measuredPlan(3,1,s,"loaded Djose production plan allocates no heap"); check(p.phase==1 && p.moves[0].held==0x1000 && p.moves[0].goal.z>85,"loaded Djose gap uses original launch threshold");
    s=snapshot({34,39,12},{{0x0c0000a3,0},{0x0c0000c6,4},{0x0c000096,4},{0x0c000097,4}});
    const int phases[]={0,1,3,5}; const unsigned input[]={0x2000,0x1000,0x2000,0x8000};
    for(unsigned i=0;i<4;++i) { s.values[0x0c0000a3]=phases[i]; p=measuredPlan(4,1,s,"Macalania production plan allocates no heap"); check(p.moves.size()==1 && p.moves[0].held==input[i] && p.moves[0].mode==PushMode::Scripted,"Macalania preserves native stage/camera input"); }
    s.values[0x0c0000a3]=0; p=pedestalPlan(4,1,s);
    check(describePedestalApproach(p,s.position,{s.position.x+4,s.position.y,s.position.z}).find(L"Push from the south side.")!=std::wstring::npos,
        "Macalania approach uses world motion rather than its differently mapped raw input direction");
    s.values[0x0c0000a3]=2; check(!pedestalPlan(4,1,s).blocked.empty(),"wrong Macalania branch cannot become first-mound success");
    for(unsigned local=56;local<=60;++local) { s=snapshot({},{{0x0c0000de+local-56,0}}); p=pedestalPlan(5,local,s); check(p.moves.size()==1 && p.moves[0].mode==PushMode::Single,"each upper Djose pedestal uses one original Confirm"); }
    for(unsigned local=44;local<=47;++local) { s=snapshot({},{{0x0c000021,0}}); check(pedestalPlan(6,local,s).moves[0].mode==PushMode::Single,"each Zanarkand pedestal activates its pattern once"); }
    bool missingRejected=false;
    try { (void)pedestalPlan(1,7,snapshot({},{{0x5c0000cc,1} })); }
    catch(const std::out_of_range&) { missingRejected=true; }
    check(missingRejected,"missing required pedestal binding remains rejected");
}
int main() {
    base=reinterpret_cast<uintptr_t>(VirtualAlloc(nullptr,0x2100000,MEM_COMMIT|MEM_RESERVE,PAGE_READWRITE));
    check(base!=0,"isolated fixture memory"); installPedestal(); ruleTests();
    auto activeTarget=setup(1,7,{0,40,-65.6}); values[0x5c0000cc]=1; ready(activeTarget);
    PedestalPlan activeCopy; pedestalAllocations=0; trackPedestalAllocations=true;
    const bool activeCopied=pedestalActivePlan(activeTarget,activeCopy);
    if(activeCopied) activeCopy.moves.erase(activeCopy.moves.begin(),activeCopy.moves.begin()+1);
    trackPedestalAllocations=false;
    check(activeCopied && activeCopy.moves.size()==1 && activeCopy.moves.front().goal.z< -147,
        "active Besaid plan copies bounded moves and fixed-row erase preserves the remaining endpoint");
    check(pedestalAllocations==0,"active-plan copy and fixed-row erase allocate no heap");
    for(unsigned kind=2;kind<=4;++kind) {
        auto push=setup(kind,kind==2?6:1,{10,0,140}); Entity recess;
        check(pedestalRecessTarget(push,worker(push.pushLocal),recess) && !recess.pushControl && recess.zone.talk &&
            !recess.inactive && recess.name==L"Pedestal sphere recess","integral sphere sockets retain separate ordinary Confirm destinations");
        armPedestal(recess); check(frame(0x20)==0x20,"sphere recess preserves ordinary Confirm without claiming push assistance");
    }
    auto target=setup(1,7,{-14.4117,40,-65.6}); values[0x5c0000cc]=1; ready(target);
    check(pedestalConsentKey(0x39) && pedestalConsentKey(0x2c) && !pedestalConsentKey(0x1c) && pedestalConsentButtons()==pad::bit(pad::East),"configured Space/alternate push and physical B publish consent separately from Confirm");
    check(pedestalInstruction(target).find(L"Hold Space or Z")!=std::wstring::npos,"instructions announce actual configured push keys");
    padOwned=true;inputDeviceMark.store(0);observeInputDevice(true);
    check(pedestalInstruction(target).find(L"Hold B")!=std::wstring::npos,"controller instructions announce actual configured push button");
    padOwned=false;inputDeviceMark.store(0);observeInputDevice(false);
    check(frame()==0 && walking && !route.empty(),"held Space starts actual DI/pad walking across the pedestal contact gap");
    refreshTargetZone(target,worker(7),1);
    check(target.zone.contactRadius==0 && target.zone.range>0 && frame()==0 && walking,"live refresh preserves native contact walk rather than stopping again at staging");
    write<uint16_t>(address(0xf27080),0x4040); disable(7,4,0x12ab);
    check(read<uint16_t>(address(0xf27080))==0x1000,"owned disable replaces inward approach direction before same-interpreter initial input read");
    check(frame()==0x1000,"owned Besaid control disable preserves held movement");
    check(frame(0)==0,"release clears injection during original push animation");
    restore(); check(frame(0x20)==0x20,"released assistance does not resume or hijack a later ordinary Confirm");
    target=setup(1,7,{-14.4117,40,-65.6}); values[0x5c0000cc]=1;
    position(playerActor(),{target.position.x,target.position.y,target.position.z+4}); ready(target);
    check(!inInteractionZone(target.zone,playerPosition()) && frame()==0 && walking && distanceXZ(route.back(),target.position)>4.1,
        "starting already touching routes outward to clear native contact before re-entry");
    position(playerActor(),route.back()); walking=false; frame();
    check(pedestalActiveContact(target) && walking,"holding Space re-enters only after reaching the outside stage");
    target=setup(2,6,{.2215,-10,144.1042}); values[0x0c00005a]=1; ready(target);
    const auto pedestal=read<uintptr_t>(worker(6)+0x9c);
    position(pedestal,{49.1,-10,144.1042}); target.position={49.1,-10,144.1042};
    refreshTargetZone(target,worker(6),1);
    check(target.zone.approach.x== -1 && target.zone.approach.z==0,
        "armed current segment keeps its west-side approach at .9 residual despite live plan threshold");
    target=setup(3,1,{0,-18,24}); values[0x0c000099]=values[0x0c00009a]=1; ready(target);
    position(playerActor(),target.zone.center); check(frame()==0x2000,"lower Djose uses configured Space consent with original scripted direction");
    disable(42,1,0xd17c); check(frame()==0x2000,"lower Djose retains owned direction without exposing sphere Confirm");
    check(frame(0)==0,"lower Djose stops input immediately when Space is released");
    target=setup(1,7,{-14.4117,40,-65.6}); values[0x5c0000cc]=1; ready(target);
    frame(); disable(7,4,0x12ab); Event collected; dispatch(0x471d10,true,collected);
    for(unsigned rva:{0xf27080u,0xf27084u,0xf27088u,0xf2708cu,0xf27090u,0xf27094u}) write<unsigned>(address(rva),0xabcd0040);
    updatePedestalInput();
    for(unsigned rva:{0xf27080u,0xf27084u,0xf27088u,0xf2708cu,0xf27090u,0xf27094u})
        check(read<unsigned>(address(rva))>>16==0xabcd,"native 16-bit input operations preserve adjacent high-word canaries");
    check(read<uint16_t>(address(0xf27080))==0x1000,"word-sized native held input still pushes in the original direction");
    target=setup(1,7,{-14.4117,40,-65.6}); values[0x5c0000cc]=1; ready(target); frame(); disable(7,4,0x1402);
    check(frame()==0,"blocked-path dialogue disable cancels and drains held push"); frame(0); check(frame()==0x40,"blocked-path drain expires on physical release");
    target=setup(1,7,{-14.4117,40,-65.6}); values[0x5c0000cc]=1; ready(target); frame(); disable(7,4,0x12ab,false);
    check(frame()==0,"stale matching worker without interpreter ancestry cannot own disabled control");
    target=setup(1,7,{-14.4117,40,-65.6}); values[0x5c0000cc]=1; ready(target); frame(); disable(7,4,0x12ab);
    write<uintptr_t>(worker(7)+0x90,base+0x1401000); check(frame()==0,"signal replacement cannot inherit a previous push claim");
    target=setup(1,7,{-14.4117,40,-65.6}); values[0x5c0000cc]=1; position(playerActor(),{50,40,0}); ready(target);
    check(frame()==0 && walking && !route.empty(),"held Space uses ordinary autowalk to reach the correct side");
    check(frame(0)==0 && !walking,"release during approach stops walking immediately");
    target=setup(1,7,{-14.4117,40,-65.6}); values[0x5c0000cc]=1; ready(target);
    check(frame(0x60)==0x20,"Confirm cancels pushing and remains available for sphere interaction");
    target=setup(1,7,{-14.4117,40,-65.6}); values[0x5c0000cc]=1; ready(target);
    std::array<uint8_t,256> keys{}; keys[0xcd]=0x80; pedestalKeyboardInput(keys.data()); check(frame()==0,"manual keyboard movement has priority over a claimed push");
    target=setup(1,7,{-14.4117,40,-65.6}); values[0x5c0000cc]=1; ready(target);
    pad::State padState; padState.held=pad::bit(pad::East); padState.lx=18000; pedestalControllerInput(padState);
    check(frame()==0,"push plus physical stick input cancels without synthetic movement");
    for(unsigned c=0;c<7;++c) {
        target=setup(1,7,{-14.4117,40,-65.6}); values[0x5c0000cc]=1; ready(target);
        if(c==0) focused=false; if(c==1) menu=true; if(c==2) console=true; if(c==3) battle=true;
        if(c==4) scene=3; if(c==5) enabled=false; if(c==6) ++epoch;
        check(frame()==0,"context/map change cancels push input and drains consent");
    }
    target=setup(1,7,{-14.4117,40,-65.6}); values[0x5c0000cc]=1; remap[12]=15; remap[15]=12;
    write<unsigned>(base+0x1600000+0x60+5*4,118); write<unsigned>(base+0x1600000+0x60+19*4,12); ready(target);
    check(pedestalConsentKey(0x2c) && !pedestalConsentKey(0x39) && pedestalConsentButtons()==pad::bit(pad::South),"actual remapped push keyboard and physical controller bindings supply consent");
    frame(); disable(7,4,0x12ab); check(frame()==0x8000 && pedestalLogicalInput(0x8000)==0x1000,"inverse native direction mapping supplies the original logical push direction");
    target=setup(4,1,{34,39,12}); values[0x0c0000c6]=4; values[0x0c000096]=4; values[0x0c000097]=4; ready(target);
    position(playerActor(),target.zone.center); check(frame()==0x2000,"Macalania starts correct native direction at side rectangle");
    disable(29,1,0xe2ad); signal(29,1,0xe2d5); check(frame()==0,"Macalania releases direction after scripted launch");
    values[0x0c0000a3]=1; complete(1,10); check(frame()==0,"Macalania completion drains held Confirm instead of launching next puzzle stage");
    target=setup(7,13,{-18.7,-453.2,263.1}); values[0x0c000029]=values[0x0c00001f]=1; ready(target); position(playerActor(),target.zone.center);
    request(38,15);
    for(unsigned i=0;i<21;++i) { check(frame()==0x8000,"Bevelle optional push keeps consent across repeated native probes"); disable(38,15,0x1c3d3); restore(); complete(38,15); }
    frame(); disable(38,15,0x1c407); check(frame()==0,"Bevelle releases input when original optional push commits");
    complete(38,15); check(frame()==0,"completed Bevelle ride does not turn held consent into another interaction");
    target=setup(6,44,{70,0,25}); ready(target); check(frame()==0x20,"Zanarkand receives exactly one original Confirm activation");
    check(read<unsigned>(address(0xf27090))==0x20,"single activation supplies an edge after a held approach");
    check(frame()==0,"held Confirm does not repeat a single pedestal activation");
    disable(44,2,0x747e); complete(44,2); check(frame()==0,"native single-action completion drains consent");
    target=setup(6,44,{70,0,25}); ready(target); frame(); disable(44,2,0x7480);
    check(frame()==0,"unrelated disable inside same single-action entry cannot claim control");
    const unsigned upperSites[]={0x14540,0x14700,0x148c0,0x14a80,0x14c40};
    const unsigned patternSites[]={0x747e,0x75f6,0x776e,0x78e6};
    for(unsigned local=56;local<=60;++local) {
        target=setup(5,local,{30,-67,117}); ready(target); check(frame()==0x20,"each upper Djose control receives one native Confirm edge");
        disable(local,2,upperSites[local-56]); check(frame()==0,"upper Djose animation never receives repeat Confirm");
        complete(local,2); check(frame()==0,"upper Djose original return completes and drains the held action");
    }
    for(unsigned local=44;local<=47;++local) {
        target=setup(6,local,{70,0,25}); ready(target); frame(); disable(local,2,patternSites[local-44]);
        complete(local,2); check(frame()==0,"each Zanarkand pattern animation owns only its exact native action");
    }
    target=setup(6,44,{70,0,25}); remap[5]=6; remap[6]=5;
    write<unsigned>(base+0x1600000+0x60+5*4,118); write<unsigned>(base+0x1600000+0x60+19*4,13); ready(target);
    check(pedestalConsentKey(0x2c) && !pedestalConsentKey(0x1c) && frame(0x40)==0x40,
        "native regional Confirm remap remains authoritative for one-shot pattern controls");
    target=setup(1,7,{-14.4117,40,-65.6}); values[0x5c0000cc]=1;
    write<uintptr_t>(address(0x1efb504),0); ready(target);
    check(frame()==0x40 && !pedestalConsentKey(0x39) && !pedestalConsentButtons(),"missing native configuration cannot arm a synthetic push");
    target=setup(7,13,{-20.26033,-460.016,-36.25044});
    write<uint8_t>(read<uintptr_t>(worker(39)),2);
    position(worker(39)+0x54c,{-30.253,-459.998,-44.302});
    position(worker(39)+0x57c,{-9.742,-459.998,-44.302});
    ready(target);
    check(target.zone.valid && !target.zone.crossing && target.zone.center.z< -48,"Bevelle entrance stages safely before native crossing edge");
    position(playerActor(),target.zone.center); check(frame(0)==0 && !walking,"Bevelle staging does not push without held Confirm");
    check(frame()==0 && walking && !route.empty(),"held Confirm starts ordinary walking through Bevelle entrance trigger");
    const auto crossingStarts=starts;
    check(frame()==0 && walking && starts==crossingStarts,"Bevelle crossing remains active on subsequent held frames");
    refreshTargetZone(target,worker(13),1);
    check(target.zone.crossing && target.zone.shape==2,"live target refresh preserves the consented entrance crossing");
    request(39,6); check(frame()==0 && !walking,"native Bevelle entrance ride ends injected walking and drains consent");
    target=setup(1,7,{-14.4117,40,-65.6}); values[0x5c0000cc]=1;
    write<unsigned>(base+0x1600000+0x60+5*4,29); ready(target);
    check(!pedestalConsentKey(0xc8),"movement-overlapping push never disables manual movement priority");
    cancelPedestal();
    // Fixed Native target/plan state has no heap-owned Entity to retire.
    const auto failures=retirement::failures();
    for(unsigned i=0;i<70;++i) {
        target=setup(1,7,{-14.4117,40,-65.6});values[0x5c0000cc]=1;ready(target);cancelPedestal();
    }
    check(retirement::failures()==failures,"fixed session reset cannot consume a retirement pool");
    target=setup(1,7,{-14.4117,40,-65.6});values[0x5c0000cc]=1;ready(target);
    PedestalPlan heldPlan;check(pedestalActivePlan(target,heldPlan) && !heldPlan.moves.empty(),"fixed session remains usable after repeated reset");
    // The real Native session can wait for a copied turn shape without either
    // ending consent or doing pathfinding itself. The shape is admitted later.
    target=setup(1,7,{-30,40,-65.6});values[0x5c0000cc]=1;ready(target);
    position(read<uintptr_t>(worker(7)+0x9c),{-14.411718,40,-65.6});
    const auto requestsBefore=routeRequests;delayRoute=true;frame();
    check(pedestalActivePlan(target,heldPlan) && heldPlan.moves.size()==1 && !walking && routeRequests==requestsBefore,"turn withholds assistance while a fresh copied shape is pending");
    frame();check(pedestalActivePlan(target,heldPlan),"pending geometry does not retire an otherwise owned held session");
    refreshTargetZone(target,worker(7),1);OwnedNavTarget nextTarget{};check(captureOwnedNavTarget(target,nextTarget),"fixture copied turn target");
    updatePedestalOwnedTarget(nextTarget.native);delayRoute=false;frame();
    check(pedestalActivePlan(target,heldPlan),"fresh copied geometry keeps the original remaining turn plan");
    delayRoute=true;target=setup(1,7,{-14.4117,40,-65.6});values[0x5c0000cc]=1;ready(target);frame();
    check(capturePedestalNavigationState().held,"pending route retains explicit held consent");frame(0);check(!capturePedestalNavigationState().armed,"released consent retires a pending route before delayed movement can start");delayRoute=false;
    nativeOwner=false;cancelPedestal();nativeOwner=true;
    check(!pedestalActivePlan(target,heldPlan),"foreign cancellation immediately retires consent without touching Native fields");frame(0);
    std::puts("PASS: all-trial pedestal sides, Space push/native contact lifecycle, remaps and cancellation");
}
