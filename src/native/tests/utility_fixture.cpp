#include "external_fixture.hpp"
#include <atomic>
// Real archived code identities, synthetic memory/native API boundaries.
// No game process or desktop window is opened.
#include "../events.hpp"
#include "../utilities.hpp"
#include "../utility_trials.hpp"
#include "../utility_trials_data.hpp"
#include "../utility_warps.hpp"
#include "../navigation_targets.hpp"
#include "../puzzle_solver.hpp"
#include "../puzzle_solver_data.hpp"
#include "../puzzle_completion.hpp"
#include "../puzzle_completion_data.hpp"
#include "ready_speech_adapter.hpp"
#include "ready_utility_adapter.hpp"
#include "utility_prepared_adapter.hpp"
#include "../puzzle_guide.hpp"
#include "../puzzle_guide_owned.hpp"
#include "../puzzle_guide_bindings.hpp"
#include "../navigation_observations.hpp"
#include "../navigation_intents.hpp"
#include "../puzzle_guide_targets.hpp"
#include "../butterfly_navigation.hpp"
#include "../copied_controls.hpp"
#include "../pedestal.hpp"
#include "../puzzle_guide_data.hpp"
#include "../mod_menu.hpp"
#include "../zanarkand_patterns.hpp"
#include "../settings.hpp"
#include "../via_purifico.hpp"
#include "../via_purifico_data.hpp"
#include <map>
#include <fstream>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <stdexcept>
#include <algorithm>
using namespace ffx;
namespace {
std::vector<uint8_t> chunk(0x60000), root(0xb58), target(0xb58), helper(0xb58), physical(0xb58), controller(0x220), signal(32), helperSignal(32), physicalSignal(32), values(4096);
std::vector<uint8_t> ending(0xb58), departure(0xb58), endingSignal(32), departureSignal(32);
std::vector<uint8_t> globals(0x20000);
std::vector<uint8_t> bonusWorkerBytes(0xb58),bonusSignalBytes(32);
bool bonusLoaded=false;
std::vector<std::pair<unsigned,unsigned>> declarations;
struct Listener { Handler enter,leave; };
std::map<unsigned,std::vector<Listener>> hooks;
std::map<unsigned,void(*)(bool)> keys;
void(*consoleCallback)(bool)=nullptr; bool(*consoleAvailable)()=nullptr;
unsigned consoleVk=0; bool consoleShift=false,minigame=false;
unsigned consoleOpened=0;
std::map<Event*,std::vector<Event>> pendingEvents;
void fire(unsigned id,Event& e,bool enter=true,bool leave=true) {
    e.rva=id;
    std::vector<Event> snapshots;
    if(enter) for(auto h:hooks.at(id)) { auto local=e;if(h.enter)h.enter(local);snapshots.push_back(std::move(local)); }
    else { snapshots=std::move(pendingEvents.at(&e));pendingEvents.erase(&e); }
    if(leave) for(unsigned i=0;i<snapshots.size();++i) {
        snapshots[i].result=e.result;if(hooks.at(id)[i].leave)hooks.at(id)[i].leave(snapshots[i]);
    } else pendingEvents[&e]=std::move(snapshots);
}
std::vector<std::wstring> replies;
unsigned epoch=1, writes=0, requests=0, warps=0, dialogue=0, targetLocal=0;
unsigned nextRoom=0, nextSpawn=0, initPc=0;
int storyAtWarp=-1;
bool viaLoaded=false;
bool fixtureNativeOwner=true;
bool gagAutomatic=false,gagCues=true,gagFocus=true;
unsigned gagBeeps=0;
float gagLastFrequency=0;
int patternAutoDetail=0;
std::array<std::vector<uint8_t>,16> viaWorkers,viaSignals;
Point fixturePlayer;
int initY=0;
bool battle=false;
bool delayedActions=false;
std::vector<NativeAction> pendingActions;
uintptr_t originalRequestCode=0;
void __cdecl dispatchQueued(int,int);
uintptr_t ptr(std::vector<uint8_t>& bytes) { return reinterpret_cast<uintptr_t>(bytes.data()); }
void check(bool value,const char* reason) { if(!value){std::fprintf(stderr,"FAIL: %s\n",reason);std::exit(1);} }
template<class T> void put(std::vector<uint8_t>& bytes,unsigned off,T value) { memcpy(bytes.data()+off,&value,sizeof(value)); }
unsigned width(unsigned low) { return low>>28<2?1:low>>28<4?2:4; }
uintptr_t __cdecl worker(int index) { if(viaLoaded) return index>=0 && index<16?ptr(viaWorkers[index]):0;return index==0?ptr(root):index==1?ptr(target):index==2?ptr(helper):index==3?ptr(physical):index==4?ptr(ending):index==5?ptr(bonusLoaded?bonusWorkerBytes:departure):0; }
uintptr_t __cdecl getter(uintptr_t,int index,int element) {
    const auto low=declarations.at(index).first;
    if(((low>>25)&7)==0) {
        const auto offset=(low&0xffffff)+element*width(low);
        check(offset+width(low)<=globals.size(),"original location0 global extent");return ptr(globals)+offset;
    }
    return ptr(values)+index*128+element*width(low);
}
void __cdecl setter(uintptr_t,int index,int element,int value,int) { ++writes;memcpy(reinterpret_cast<void*>(getter(0,index,element)),&value,width(declarations.at(index).first)); }
int __cdecl mode() { return dialogue; }
int __cdecl request(int,int index,int,int level,int entry) {
    ++requests;
    if(originalRequestCode) return reinterpret_cast<int(__cdecl*)(int,int,int,int,int)>(originalRequestCode)(0,index,0,level,entry);
    if(viaLoaded) {
        check(index>0 && index<8 && entry==2,"Via requests only remaining original chest callbacks");
        auto& w=viaWorkers[index];auto& sig=viaSignals[index];
        put(w,0x80,ptr(sig));put(w,0x90,ptr(sig));put(sig,8,static_cast<uint16_t>(entry));put(sig,14,static_cast<uint8_t>(level));
        Event e;e.args[0]=ptr(w);e.args[1]=ptr(sig);fire(0x469040,e);
        initPc=static_cast<unsigned>(read<uintptr_t>(ptr(w)+0x12c+level*0x4c+0x18)-(ptr(chunk)+0x1000));return 1;
    }
    if(index==2&&entry==7)return 1;
    check(index==1||index==2||index==4||index==5,"requested target global index");
    auto& w=index==1?target:index==2?helper:index==4?ending:bonusLoaded?bonusWorkerBytes:departure;
    auto& sig=index==1?signal:index==2?helperSignal:index==4?endingSignal:bonusLoaded?bonusSignalBytes:departureSignal;
    put(w,0x80,ptr(sig));put(w,0x90,ptr(sig));put(sig,8,static_cast<uint16_t>(entry));put(sig,14,static_cast<uint8_t>(level));
    Event e;e.args[0]=ptr(w);e.args[1]=ptr(sig);fire(0x469040,e);
    initPc=static_cast<unsigned>(read<uintptr_t>(ptr(w)+0x12c+level*0x4c+0x18)-(ptr(chunk)+0x1000));
    initY=read<int>(ptr(w)+0x12c+level*0x4c+0x24);
    return 1;
}
void __cdecl warp(int room,int spawn,int) {
    storyAtWarp=-1;
    for(unsigned i=0;i<declarations.size();++i) if(declarations[i]==std::make_pair(0x20000a00u,1u)) storyAtWarp=read<uint16_t>(getter(0,i,0));
    ++warps;nextRoom=room;nextSpawn=spawn;++epoch;updateUtilityContext();
}
void declaration(unsigned low,unsigned high) {
    for(auto b:declarations)if(b.first==low&&b.second==high)return;
    const unsigned index=static_cast<unsigned>(declarations.size());declarations.push_back({low,high});
    put(chunk,0x200+index*8,low);put(chunk,0x204+index*8,high);
}
void setValue(unsigned low,unsigned high,int value,unsigned element=0) {
    for(unsigned i=0;i<declarations.size();++i)if(declarations[i]==std::make_pair(low,high)) {
        memcpy(reinterpret_cast<void*>(getter(0,i,element)),&value,width(low));return;
    }
    check(false,"fixture declaration missing");
}
void load(const TrialUtility& t,const ScriptAction& action,bool prepare=true) {
    viaLoaded=bonusLoaded=false;
    for(auto p:{&chunk,&root,&target,&helper,&physical,&controller,&signal,&helperSignal,&physicalSignal,&values,&globals})std::fill(p->begin(),p->end(),uint8_t{0});
    declarations.clear();targetLocal=action.worker;
    std::ifstream stream(std::string(t.script)+".bin",std::ios::binary);
    check(stream.good(),"archived script fixture");stream.read(reinterpret_cast<char*>(chunk.data()+0x1000),t.length);
    put(chunk,0,t.length);put(chunk,0xc,0x1d0u);put(chunk,0x30,0x1000u);put(chunk,0x34,static_cast<uint16_t>(100));
    // Keep the name outside all100 worker offsets; locals50/51 own +100/+104.
    memcpy(chunk.data()+0x1d0,t.script,8);
    put(chunk,0x38,0x400u);put(chunk,0x38+action.worker*4,0x500u);
    declaration(0x01000a70,1);declaration(0x20000a00,1);declaration(0x00000280,1);
    if(std::string(t.script)=="bvyt1100")declaration(0x0c00002b,1);
    declaration(t.ready.low,t.ready.high);
    for(auto b:t.preparation)declaration(b.low,b.high);
    put(chunk,0x402,static_cast<uint16_t>(declarations.size()));put(chunk,0x414,0x200u);
    put(chunk,0x500,static_cast<uint8_t>(1));put(chunk,0x508,static_cast<uint16_t>(16));put(chunk,0x520,0x700u);
    put(chunk,0x700+action.entry*4,action.entryPc);
    put(root,0,ptr(chunk)+0x400);put(root,4,ptr(chunk));put(root,0x2e,static_cast<uint16_t>(0));
    put(target,0,ptr(chunk)+0x500);put(target,4,ptr(chunk));put(target,0x2e,static_cast<uint16_t>(action.worker));
    put(target,0x88,ptr(signal));put(controller,0xc,static_cast<uint16_t>(2));
    put(controller,0x2c,ptr(globals));
    if (std::string(t.script)=="dome0700" && action.worker!=32) {
        put(controller,0xc,static_cast<uint16_t>(3));
        put(chunk,0x38+32*4,0x600u);put(chunk,0x608,static_cast<uint16_t>(8));
        put(helper,0,ptr(chunk)+0x600);put(helper,4,ptr(chunk));put(helper,0x2e,static_cast<uint16_t>(32));
    }
    writes=requests=warps=dialogue=0;battle=false;++epoch;
    updateUtilityContext();
    if(std::string(t.script)=="dome0700")setValue(0x20000a00,1,2875);
    if(prepare) utility_fixture_resources::prepareUtilityResources(ptr(chunk),chunk.data(),chunk.size());
}
void loadSolver(const TrialUtility& t,const PuzzleContract& c,const PuzzleInteraction& interaction) {
    load(t,t.treasure); resetPuzzleSolver();
    if(std::string(t.script)=="dome0700")for(const auto& b:nav::guide_dome0700)declaration(b.low,b.high);
    declaration(c.carried.low,c.carried.high);
    for(auto& i:c.interactions)for(auto b:{i.sphere,i.otherSphere})if(b.low)declaration(b.low,b.high);
    put(chunk,0x402,static_cast<uint16_t>(declarations.size()));
    if(t.collect.worker!=t.treasure.worker) {
        put(chunk,0x38+t.collect.worker*4,0x600u);put(chunk,0x600,static_cast<uint8_t>(1));
        put(chunk,0x608,static_cast<uint16_t>(16));put(chunk,0x620,0x800u);
        put(chunk,0x800+t.collect.entry*4,t.collect.entryPc);
        put(helper,0,ptr(chunk)+0x600);put(helper,4,ptr(chunk));put(helper,0x2e,static_cast<uint16_t>(t.collect.worker));put(helper,0x88,ptr(helperSignal));
    } else put(chunk,0x700+t.collect.entry*4,t.collect.entryPc);
    put(controller,0xc,static_cast<uint16_t>(4));
    put(chunk,0x38+interaction.worker*4,0x900u);put(chunk,0x900,static_cast<uint8_t>(3));
    put(chunk,0x908,static_cast<uint16_t>(8));put(chunk,0x920,0xa00u);put(chunk,0xa00+interaction.entry*4,interaction.entryPc);
    put(physical,0,ptr(chunk)+0x900);put(physical,4,ptr(chunk));put(physical,0x2e,static_cast<uint16_t>(interaction.worker));
    // A Besaid/Bevelle reveal can share the physical interaction's local worker;
    // use the same loaded worker/header and the separate activation signal.
    if(interaction.worker==t.treasure.worker) {
        put(target,0,ptr(chunk)+0x900);put(chunk,0xa00+t.treasure.entry*4,t.treasure.entryPc);
        if(t.collect.worker==t.treasure.worker)put(chunk,0xa00+t.collect.entry*4,t.collect.entryPc);
        put(physical,0x2e,static_cast<uint16_t>(99));
    }
    for(const auto& r:puzzleCompletions)if(std::string(r.script)==t.script) {
        for(auto b:r.preparation)declaration(b.low,b.high);
        auto setup=[&](std::vector<uint8_t>& w,std::vector<uint8_t>& sig,unsigned header,unsigned table,const ScriptAction& a) {
            std::fill(w.begin(),w.end(),uint8_t{0});std::fill(sig.begin(),sig.end(),uint8_t{0});
            put(chunk,0x38+a.worker*4,header);put(chunk,header,static_cast<uint8_t>(1));
            put(chunk,header+8,static_cast<uint16_t>(64));put(chunk,header+0x20,table);
            put(chunk,table+a.entry*4,a.entryPc);
            put(w,0,ptr(chunk)+header);put(w,4,ptr(chunk));put(w,0x2e,static_cast<uint16_t>(a.worker));put(w,0x88,ptr(sig));
        };
        setup(ending,endingSignal,0xb00,0xc00,r.first);
        if(r.later.worker==r.first.worker)put(chunk,0xc00+r.later.entry*4,r.later.entryPc);
        else setup(departure,departureSignal,0xe00,0xf00,r.later);
        put(controller,0xc,static_cast<uint16_t>((r.exit.worker || r.later.worker!=r.first.worker)?6:5));
        if(r.bonus.worker) {
            declaration(r.bonusCollected.low,r.bonusCollected.high);
            setup(bonusWorkerBytes,bonusSignalBytes,0x380,0x3b0,r.bonus);bonusLoaded=true;
            put(controller,0xc,static_cast<uint16_t>(6));
        }
        put(chunk,0x402,static_cast<uint16_t>(declarations.size()));
    }
    utility_fixture_resources::prepareUtilityResources(ptr(chunk),chunk.data(),chunk.size());
}
void natural(const PuzzleInteraction& interaction,unsigned source=0) {
    auto& w=read<uint16_t>(ptr(target)+0x2e)==interaction.worker?target:physical;
    put(w,0x80,ptr(physicalSignal));put(w,0x90,ptr(physicalSignal));
    put(physicalSignal,8,static_cast<uint16_t>(interaction.entry));put(physicalSignal,10,static_cast<uint16_t>(source));put(physicalSignal,14,static_cast<uint8_t>(1));
    Event e;e.args[0]=ptr(w);e.args[1]=ptr(physicalSignal);fire(0x469040,e);
}
void complete(unsigned index,bool finalChestReturn=true) {
    // Model the interpreter's PC at the original final RET. The production
    // collection guard must also refuse an earlier/native cancellation return.
    const auto w=worker(index),sig=read<uintptr_t>(w+0x90);
    if(finalChestReturn && sig) for(const auto& t:trialUtilities) {
        if(std::memcmp(chunk.data()+0x1d0,t.script,8) || read<uint16_t>(w+0x2e)!=t.collect.worker ||
            read<uint16_t>(sig+8)!=t.collect.entry || !t.collect.returnPc) continue;
        const auto level=read<uint8_t>(sig+14)&15;
        write<uintptr_t>(w+0x12c+level*0x4c+0x18,ptr(chunk)+0x1000+t.collect.returnPc);
    }
    Event e;e.args[0]=index;fire(0x469170,e,true,false);
    write<uintptr_t>(w+0x80,0);write<uintptr_t>(w+0x90,0);
    fire(0x469170,e,false,true);
}
}
namespace ffx {
void refreshSwimmingGoal(Entity& entity) { if(entity.swimDepth) std::abort(); }
void armPedestal(const Entity&) {}
void startAutoWalk() {}
void stopAutoWalk() {}
void cancelPedestal() {}
void clearNavRoute() {}
std::atomic<int> scene{0};
bool minigameActive() { return minigame; }
uintptr_t address(unsigned rva) {
    switch(rva) {
    case 0xf26ae8: { static uintptr_t c;c=ptr(controller);return reinterpret_cast<uintptr_t>(&c); }
    case 0x46a7c0:return reinterpret_cast<uintptr_t>(worker);
    case 0x46c500:return reinterpret_cast<uintptr_t>(getter);
    case 0x4713a0:return reinterpret_cast<uintptr_t>(setter);
    case 0x46e920:return reinterpret_cast<uintptr_t>(request);
    case 0x465390:return reinterpret_cast<uintptr_t>(dispatchQueued);
    case 0x46fed0:return reinterpret_cast<uintptr_t>(warp);
    case 0x46c840:return reinterpret_cast<uintptr_t>(mode);
    default:throw std::runtime_error("Unexpected native fixture address");
    }
}
bool copy(uintptr_t p,void* out,size_t size) { __try{memcpy(out,reinterpret_cast<void*>(p),size);return true;}__except(1){return false;} }
bool hook(unsigned id,Handler enter,Handler leave) { hooks[id].push_back({enter,leave});return true; }
void hotkey(unsigned id,void(*callback)(bool)) { keys[id]=callback; }
void hotkeyChordWhen(unsigned vk,bool shift,void(*callback)(bool),bool(*available)(),bool exclusive) { check(exclusive,"developer console chord is exclusive under queue pressure");consoleVk=vk; consoleShift=shift; consoleCallback=callback; consoleAvailable=available; }
void installModMenu() { hotkey(VK_F8,[](bool){}); }
bool modMenuActive() { return false; }
int setting(Setting s) { return s==Setting::AutoDetail?patternAutoDetail:s==Setting::GagazetAssist?gagAutomatic:s==Setting::GagazetCues?gagCues:0; }
bool gameFocused() {return gagFocus;}
void navAudioOverdriveBeep(float,unsigned) {++gagBeeps;}
void navAudioCancelGagazetBeep() {}
void navAudioTimingBeep(float frequency,unsigned,Setting gate) {check(gate==Setting::GagazetCues,"Gagazet cue has its independent setting gate");gagLastFrequency=frequency;++gagBeeps;}
Mesh readMesh() { return {}; }
void resetNavAudio() {}
void routeNavAudio(const std::vector<Point>&,const Entity&) {}
Point playerPosition() { return fixturePlayer; }
bool pedestalActiveCrossing(const Entity&) { return false; }
bool pedestalActiveContact(const Entity&) { return false; }
bool pedestalActivePlan(const Entity&,PedestalPlan&) { return false; }
std::wstring pedestalConsentName(const PedestalPlan&) { return L"Fixture push control"; }
uintptr_t playerActor() { return 0; }
std::wstring direction(Point) { return L"north"; }
void startUtilityConsole() {}
void openUtilityConsole() { ++consoleOpened; }
bool output(const std::wstring& text,bool,const char*) { replies.push_back(text); return true; }
bool utilityConsoleActive() { return false; }
void invalidateUtilityConsole() {}
void cancelLightningWarning() {}
bool nativeGameThread() { return fixtureNativeOwner; }
bool deferNativeAction(NativeAction action) {
    if(delayedActions) { pendingActions.push_back(std::move(action)); return true; }
    if(action.epoch!=generation()) return true;
    // Match the production executor: preparation exceptions become a failed
    // completion, rather than escaping to the command parser in this fixture.
    bool ready=false,success=false;
    try {ready=action.prepare(action);}catch(...) {}
    if(ready) {try {action.invoke(action);success=true;}catch(...) {}}
    if(action.epoch==generation() && action.complete) action.complete(action,success);
    return true;
}
unsigned generation() { return epoch; }
bool inBattle() { return battle; }
void log(const char*,...) {}
void utilityReply(const std::wstring& text) { replies.push_back(text); }
void emitUtilityText(const UtilityText& text,const char*) {replies.push_back(renderUtilityText(text));}
void installUtilityText() {}
std::wstring join(const std::vector<std::wstring>& parts,const wchar_t* sep) { std::wstring s;for(auto& p:parts){if(!s.empty())s+=sep;s+=p;}return s; }
}
#include "via_purifico_fixture.inc"
namespace {
std::wstring fixturePuzzleGuideSync(ffx::OwnedGuideAction action) {
    using namespace ffx;
    UtilityScript script=utilityScript();
    uintptr_t worker0=utilityWorker(script,0);
    check(worker0!=0,"Native guide fixture captures worker zero");
    script.worker=worker0;script.header=read<uintptr_t>(worker0);
    nav::Scene& guideScene=*([]()->nav::Scene* { static nav::Scene value{};return &value; })();
    std::memset(&guideScene,0,sizeof(guideScene));guideScene.epoch=generation();guideScene.revision=1;guideScene.primary=script.chunk;guideScene.partCount=1;
    auto& part=guideScene.parts[0];
    check(nativeAtelSpan(script.chunk,part.span),"Native guide fixture captures resource span");
    part.span.epoch=guideScene.epoch;part.guideReady=true;part.bindingsReady=true;part.identityReady=true;
    AtelIdentity identity{};check(readyAtelIdentity(script.chunk,identity),"Native guide fixture captures prepared identity");
    check(identity.span.serial==part.span.serial,"Native guide fixture identity matches span");
    check(declarations.size()<=part.bindings.rows.size(),"Native guide fixture declaration bound");
    part.bindings.count=static_cast<unsigned>(declarations.size());
    for(unsigned i=0;i<part.bindings.count;++i) {
        const auto [low,high]=declarations[i];auto& row=part.bindings.rows[i];row.binding=low;row.high=high;
        row.value=utilityValue(script,low,high);
    }
    speechq::Header header{};header.generation=guideScene.epoch;
    struct WorkerGuard {bool previous;WorkerGuard():previous(fixtureNativeOwner){fixtureNativeOwner=false;}~WorkerGuard(){fixtureNativeOwner=previous;}} guard;
    return puzzleGuideOwned(action,guideScene,Mesh{},std::vector<Entity>{},header,0,0);
}
}
namespace ffx::nav {
bool submitNavigationQuery(Query query,bool,unsigned,unsigned,uintptr_t,unsigned) {
    OwnedGuideAction action;
    switch(query) {
    case Query::GuideStep: action=OwnedGuideAction::Step;break;
    case Query::GuideNext: action=OwnedGuideAction::Next;break;
    case Query::GuidePrevious: action=OwnedGuideAction::Previous;break;
    case Query::GuideExpected: action=OwnedGuideAction::Expected;break;
    case Query::GuidePath: action=OwnedGuideAction::Path;break;
    case Query::GuideCompleted: action=OwnedGuideAction::Completed;break;
    case Query::GuideTileCompleted: action=OwnedGuideAction::TileCompleted;break;
    default:return false;
    }
    const auto text=fixturePuzzleGuideSync(action);if(!text.empty()) utilityReply(text);return true;
}
bool scenePedestalPlan(const Scene&,const Entity&,PedestalPlan&) { return false; }
InteractionZone sceneInteractionZone(const WorkerFact&,unsigned,float) { return {}; }
unsigned navigationIntentRevision() { return 0; }
void invalidateNavigationIntents() {}
bool submitNavigationIntent(const Scene&,const Entity&,Intent,unsigned,unsigned) { return false; }
}
namespace ffx {
OwnedTextContext copiedControlContext(const CopiedControls&) { return {}; }
bool cameraBasis(Point&,Point&) { return false; }
std::vector<Point> navigationRouteOwned(const Mesh&,Point,const Entity&,const std::vector<Entity>&) { std::abort(); }
bool queuePuzzleGuidePoint(Point,int,bool,const wchar_t*,const UtilityText&) { return false; }
bool takePuzzleGuideTarget(unsigned,unsigned,QueuedPuzzleTarget&) { return false; }
void startPuzzleGuideTargets() { std::abort(); }
void retirePuzzleGuideTargets() { std::abort(); }
}
#include "zanarkand_fixture.inc"
#include "zanarkand_pattern_fixture.inc"
#include "utility_choice_replay.inc"
#include "atel_queue_replay.inc"
#include "djose_solver_fixture.inc"
#include "bevelle_treasure_fixture.inc"
#include "trial_intercept_fixture.inc"
#include "trial_entry_fixture.inc"
#include "trial_collection_fixture.inc"
#include "gagazet_fixture.inc"
#include "gagazet_timing_fixture.inc"
int main() try { SetErrorMode(SEM_FAILCRITICALERRORS|SEM_NOGPFAULTERRORBOX);std::setvbuf(stdout,nullptr,_IONBF,0);installUtilities();
    nativeQueuedKilikaFixture();
    trialCollectionFixtures();
    djoseSolverFixtures();
    allTrialInterceptFixtures();
    trialEntryFixtures();
    bevelleTreasureFixtures();
    gagazetFixtures();
    gagTimingFixtures();
    zanarkandFixtures();
    zanarkandPatternFixtures();
    load(trialUtilities[0],trialUtilities[0].collect); resetOwnedPuzzleGuide();
    for(const auto& s:puzzleRoutes[0].steps) if(s.check) declaration(s.done.low,s.done.high);
    put(chunk,0x402,static_cast<uint16_t>(declarations.size()));
    utility_fixture_resources::prepareUtilityResources(ptr(chunk),chunk.data(),chunk.size());
    check(fixturePuzzleGuideSync(OwnedGuideAction::Step).find(L"Step 1 of")!=std::wstring::npos,"manual guide starts at first step");
    check(fixturePuzzleGuideSync(OwnedGuideAction::Previous).find(L"Step 1 of")!=std::wstring::npos,"guide previous clamps at first step");
    setValue(0x5c0000ec,1,1);
    check(fixturePuzzleGuideSync(OwnedGuideAction::Step).find(L"Step 1 of")!=std::wstring::npos,"reading selected step does not auto-advance");
    check(fixturePuzzleGuideSync(OwnedGuideAction::Expected).find(L"Step 2 of")!=std::wstring::npos,"expected read follows committed native checkpoint");
    check(fixturePuzzleGuideSync(OwnedGuideAction::Step).find(L"Step 1 of")!=std::wstring::npos,"expected read cannot move manual selection");
    check(fixturePuzzleGuideSync(OwnedGuideAction::Next).find(L"Step 2 of")!=std::wstring::npos,"manual next increments one step");
    fixturePuzzleGuideSync(OwnedGuideAction::Completed);
    check(fixturePuzzleGuideSync(OwnedGuideAction::Step).find(L"Step 2 of")!=std::wstring::npos,"native completion leaves manual selection unchanged");
    for(unsigned i=0;i<20;++i) fixturePuzzleGuideSync(OwnedGuideAction::Next);
    check(fixturePuzzleGuideSync(OwnedGuideAction::Step).find(L"Step 13 of 13")!=std::wstring::npos,"manual next clamps at last step");
    setValue(0x5c0000cc,1,0);
    check(fixturePuzzleGuideSync(OwnedGuideAction::Step).find(L"Besaid Sphere must be in the pedestal")!=std::wstring::npos,
        "final guide reads native sphere state and identifies a missing activation prerequisite");
    setValue(0x5c0000cc,1,1);
    check(fixturePuzzleGuideSync(OwnedGuideAction::Step).find(L"push it onto the final floor depression")!=std::wstring::npos,
        "an inserted Besaid Sphere exposes the original final depression objective");

    setValue(0x5c0000dc,1,1);
    check(fixturePuzzleGuideSync(OwnedGuideAction::Expected).find(L"Step 7 of")!=std::wstring::npos,"durable checkpoint recovers expected step past emptied carried sphere");
    resetOwnedPuzzleGuide();
    // Both Kilika entry sphere/recess choices activate the same full native
    // destruction insertion. Only its one owned window1 choice is automatic.
    for(unsigned local:{28u,29u}) {
        const auto& t=trialUtilities[1];const auto& c=puzzleContracts[1];
        const auto it=std::find_if(c.interactions.begin(),c.interactions.end(),[&](const auto& i){return i.worker==local;});
        check(it!=c.interactions.end(),"Kilika original take/place choice interaction exists");
        loadSolver(t,c,*it);puzzleCommand(L"on");natural(*it);complete(3);pumpPuzzleSolver();
        check(initPc==51412 && t.treasure.choice==0,"Kilika takes full setup instead of an animation tail");
        const auto insertionPC=ptr(target)+0x12c+(read<uint8_t>(ptr(signal)+14)&15)*0x4c+0x18;
        check(read<uintptr_t>(insertionPC)==ptr(chunk)+0x1000+51412,"owned insertion is not mistaken for another player interaction");
        const auto revealRequests=requests,revealWrites=writes;const auto revealReplies=replies.size();
        for(unsigned repeat=0;repeat<2;++repeat) {
            dialogue=1;natural(*it);dialogue=0;
            check(read<uintptr_t>(ptr(physical)+0x12c+0x4c+0x18)==ptr(chunk)+0x1000+it->returnPc && utilityBusy(),
                "queued Kilika entry interaction during reveal returns without its ordinary take/place UI");
            complete(3);
            check(requests==revealRequests && writes==revealWrites && replies.size()==revealReplies &&
                  read<uintptr_t>(insertionPC)==ptr(chunk)+0x1000+51412,
                  "retiring repeated player interactions preserves the exact original insertion and solver objective");
        }
        unsigned choiceFrame[8]={1,0xffff,0};
        Event choice;choice.args[0]=ptr(physical);choice.args[1]=reinterpret_cast<uintptr_t>(choiceFrame);choice.result=0;
        for(auto h:hooks.at(0x459a80))if(h.leave)h.leave(choice);
        check(!choice.overrideResult && choiceFrame[1]==0xffff,"another worker's choice stays manual during solver reveal");
        choice.args[0]=ptr(target);choiceFrame[0]=0;
        for(auto h:hooks.at(0x459a80))if(h.leave)h.leave(choice);
        check(!choice.overrideResult,"unrelated message window is never accepted automatically");
        choiceFrame[0]=1;
        replayWaitingChoice(choice);
        for(auto h:hooks.at(0x459a80))if(h.leave)h.leave(choice);
        check(choice.overrideResult && choice.result==1 && static_cast<int16_t>(choiceFrame[1])==0,"owned Kilika insertion completes native choice with Insert, preserving ret-int cleanup");
        replayAcceptedChoice(choice);
        choice.overrideResult=false;choice.result=0;choiceFrame[1]=0xffff;
        for(auto h:hooks.at(0x459a80))if(h.leave)h.leave(choice);
        check(!choice.overrideResult,"accepted insertion cannot affect a second menu");
        setValue(t.ready.low,t.ready.high,t.ready.value,t.ready.element);complete(1);pumpPuzzleSolver();
        check(initPc==t.collect.pc,"Kilika accepted reveal continues to original treasure chest");
        const auto chestRequests=requests;natural(*it);complete(3);
        check(requests==chestRequests && read<uintptr_t>(ptr(physical)+0x12c+0x4c+0x18)==ptr(chunk)+0x1000+it->returnPc,
            "queued entry interaction cannot steal the original chest collection phase");
        setValue(0x01000a70,1,t.mask);complete(2);pumpPuzzleSolver();
        check(initPc==puzzleCompletions[1].later.pc && !puzzleArmed() && !warps,"ordinary sphere/recess activation continues to native trial completion");
        const auto completionRequests=requests;natural(*it);complete(3);
        check(requests==completionRequests && read<uintptr_t>(ptr(physical)+0x12c+0x4c+0x18)==ptr(chunk)+0x1000+it->returnPc,
            "Kilika completion still owns interactions until its normal native finish");complete(4);
        puzzleCommand(L"off");put(physical,0x12c+0x4c+0x18,uintptr_t{0});natural(*it);
        check(read<uintptr_t>(ptr(physical)+0x12c+0x4c+0x18)==0,"disabled solver immediately restores native Kilika interaction");
    }
    load(trialUtilities[1],trialUtilities[1].treasure);trialCommand(L"treasure");
    utility_fixture_resources::prepareUtilityResources(ptr(chunk),chunk.data(),chunk.size());
    unsigned recycledFrame[2]={1,0xffff};Event recycledChoice;recycledChoice.args[0]=ptr(target);recycledChoice.args[1]=reinterpret_cast<uintptr_t>(recycledFrame);
    for(auto h:hooks.at(0x459a80))if(h.leave)h.leave(recycledChoice);
    check(!recycledChoice.overrideResult && recycledFrame[1]==0xffff,"same-address replacement allocation cannot inherit a solver's menu consent");
    for(const auto& t:trialUtilities) {
        load(t,t.treasure);
        check(trialCommand(L"treasure").find(L"preparing")!=std::wstring::npos,"all-six treasure start");
        check(requests==(std::string(t.script)=="dome0700"?2u:1u)&&initPc==t.treasure.pc,"exact original reveal continuation");
        check(writes==t.preparation.size(),"only documented preparation writes");
        check(utilityValue(utilityScript(),0x01000a70,1)==0,"preparation does not collect items");
        const auto busyFinish=trialCommand(L"finish");
        check(busyFinish.find(std::string(t.script)=="dome0700"?L"native exit":L"still running")!=std::wstring::npos&&!warps,"busy script cannot leave the trial");
        load(t,t.collect);setValue(t.ready.low,t.ready.high,t.ready.value,t.ready.element);
        check(trialCommand(L"collect").find(L"opening")!=std::wstring::npos&&requests==1,"all-six native collection");
        check(initPc==t.collect.pc&&initY==t.collect.regY,"correct chest continuation and temple bit branch");
        load(t,t.collect);setValue(0x01000a70,1,t.mask);
        check(trialCommand(L"collect").find(L"already")!=std::wstring::npos&&!requests,"repeat does not grant treasure again");
        setValue(0x20000a00,1,3000);
        if(std::string(t.script)=="dome0700") {
            check(trialCommand(L"finish").find(L"native exit")!=std::wstring::npos&&!warps&&!requests,"Zanarkand return treasure never uses the story/room shortcut");
        } else {
            check(trialCommand(L"finish").find(L"finishing")!=std::wstring::npos&&warps==1,"finish after treasure");
            check(nextRoom==t.exitRoom&&nextSpawn==t.exitSpawn,"correct exit room and entrance");
        }
        check(utilityValue(utilityScript(),0x20000a00,1)==3000,"later story progress preserved");
        load(t,t.treasure,false);chunk[0x1000]^=1;utility_fixture_resources::prepareUtilityResources(ptr(chunk),chunk.data(),chunk.size());bool failed=false;
        try{trialCommand(L"treasure");}catch(const std::exception&){failed=true;}
        check(failed&&!writes&&!requests,"changed code refused before writes");
        load(t,t.treasure);put(chunk,0x700+t.treasure.entry*4,999u);failed=false;
        try{trialCommand(L"treasure");}catch(const std::exception&){failed=true;}
        check(failed&&!writes&&!requests,"changed entry table refused before writes");
    }
    load(trialUtilities[0],trialUtilities[0].treasure);
    check(queueUtility(L"trial treasure")&&!requests,"UI thread only queues command");
    ++epoch;updateUtilityContext();pumpUtilities();check(!requests&&replies.back().find(L"cancelled")!=std::wstring::npos,"stale queued command rejected");
    check(warpCommand(L"122 999").find(L"not in")!=std::wstring::npos&&!warps,"invalid entrance rejected");
    check(warpCommand(L"besaid").find(L"Warping")!=std::wstring::npos&&nextRoom==122,"named warp");
    battle=true;check(warpCommand(L"kilika").find(L"field")!=std::wstring::npos,"battle warp rejected");
    battle=false;dialogue=1;updateUtilityContext();
    check(warpCommand(L"kilika").find(L"dialogue")!=std::wstring::npos,"ordinary warp respects dialogue");
    check(warpCommand(L"force kilika").find(L"Warping")!=std::wstring::npos&&nextRoom==108,"explicit recovery warp");
    check(listWarps(L"besaid").find(L"room 122")!=std::wstring::npos,"searchable trial location");
    AtelScript catalogScript;catalogScript.name="bsyt0100";
    catalogScript.code.resize(trialUtilities[0].length);catalogScript.checksum=trialUtilities[0].checksum;
    const auto catalog=navigationCatalog(catalogScript);
    check(catalog&&navigationTarget(catalog,46)&&std::wstring(navigationTarget(catalog,46)->label)==L"Besaid Sphere recess","script-bound trial label");
    check(!navigationTarget(catalog,19),"camera/ambient worker omitted");
    catalogScript.checksum^=1;check(!navigationCatalog(catalogScript),"labels rejected for different bytecode");
    for(unsigned n=0;n<5;++n) {
        const auto& t=trialUtilities[n];const auto& c=puzzleContracts[n];
        const auto& i=c.interactions.front();check(!c.interactions.empty(),"all six physical interaction contracts");
        loadSolver(t,c,i);check(puzzleCommand(L"on").find(L"solver on")!=std::wstring::npos,"current-trial menu arms solver");
        if(n==3) setValue(0x20000a00,1,1550);
        natural(i,88);check(!requests&&!warps,"non-player script request cannot activate solver");
        natural(i);auto activeIndex=read<uint16_t>(ptr(target)+0x2e)==i.worker?1u:3u;
        check(read<uintptr_t>(worker(activeIndex)+0x12c+0x4c+0x18)==ptr(chunk)+0x1000+i.returnPc,"interaction substituted before original side effects");
        natural(i);check(!requests,"repeated activation cannot duplicate objective");
        complete(activeIndex);pumpPuzzleSolver();check(initPc==t.treasure.pc,"natural interaction schedules original reveal without a separate goal chooser");
        setValue(t.ready.low,t.ready.high,t.ready.value,t.ready.element);complete(1);pumpPuzzleSolver();
        check(initPc==t.collect.pc&&initY==t.collect.regY,"reveal return schedules correct original chest");
        setValue(0x01000a70,1,t.mask);complete(t.collect.worker==t.treasure.worker?1:2);pumpPuzzleSolver();
        finishTrialBonus(t);
        const auto& end=puzzleCompletions[n];
        check(!warps&&initPc==(end.storyThreshold?end.first.pc:end.later.pc)&&!puzzleArmed(),"normal completion uses original game action after treasure, never console warp");
        if(end.exit.worker) {
            check(utilityValue(utilityScript(),0x20000a00,1)==0,"Zanarkand solver does not mark later story event complete");
            complete(4);pumpPuzzleSolver();check(initPc==end.exit.pc,"Zanarkand gate completion schedules original lift exit");
            complete(5);
        } else complete(4);
        loadSolver(t,c,i);puzzleCommand(L"on");setValue(c.carried.low,c.carried.high,c.destruction,c.carried.element);
        setValue(t.ready.low,t.ready.high,t.ready.value,t.ready.element);setValue(0x01000a70,1,t.mask);
        natural(i);complete(activeIndex);pumpPuzzleSolver();pumpPuzzleSolver();
        finishTrialBonus(t);
        check(!warps&&puzzleArmed(),"destruction interaction preserves chamber and does not duplicate treasure");
        puzzleCommand(L"off");check(!puzzleArmed(),"solver can be disabled");
        const auto stored=std::find_if(c.interactions.begin(),c.interactions.end(),[](const PuzzleInteraction& a){return a.sphere.low!=0;});
        check(stored!=c.interactions.end(),"every trial identifies stored Destruction Spheres");
        loadSolver(t,c,*stored);setValue(0x01000a70,1,t.mask);setValue(stored->sphere.low,stored->sphere.high,stored->sphere.value,stored->sphere.element);
        puzzleCommand(L"on");natural(*stored);
        complete(read<uint16_t>(ptr(target)+0x2e)==stored->worker?1:3);pumpPuzzleSolver();pumpPuzzleSolver();
        finishTrialBonus(t);
        check(puzzleArmed()&&requests==(n==4?1u:0u)&&!warps,"stored Destruction Sphere interaction stays in chamber after any remaining trial treasure");
        loadSolver(t,c,i);setValue(0x20000a00,1,3000);setValue(0x01000a70,1,t.mask);
        puzzleCommand(L"on");natural(i);complete(activeIndex);pumpPuzzleSolver();pumpPuzzleSolver();
        finishTrialBonus(t);
        check(!warps&&initPc==end.later.pc&&utilityValue(utilityScript(),0x20000a00,1)==3000,"later visits preserve story and use original later completion action");
        complete(end.later.worker==end.first.worker?4:5);
        loadSolver(t,c,i);puzzleCommand(L"on");++epoch;natural(i);check(!requests&&!warps&&!puzzleArmed(),"map change disarms old interaction snapshot");
    }
    for(unsigned progress:{1544u,1545u,1552u,1553u,3000u}) {
        const auto& t=trialUtilities[3]; const auto& c=puzzleContracts[3];
        loadSolver(t,c,c.interactions.front()); setValue(0x20000a00,1,static_cast<int>(progress));
        beginPuzzleCompletion(utilityScript());
        const auto& end=puzzleCompletions[3];
        check(initPc==(progress>=1545 && progress<1553?end.first.pc:end.later.pc),"Macalania hall handoff is bounded to the native escape story phase");
        check(utilityValue(utilityScript(),0x20000a00,1)==static_cast<int>(progress) && !warps,"Macalania exit selection preserves story and original transition ownership");
    }
    loadSolver(trialUtilities[3],puzzleContracts[3],puzzleContracts[3].interactions.front());
    check(puzzleCommand(L"fayth").find(L"Use the mod menu")!=std::wstring::npos && !writes && !requests && !warps,
        "the removed fayth command cannot override normal trial or return-story routing");
    loadSolver(trialUtilities[0],puzzleContracts[0],puzzleContracts[0].interactions.front()); resetPuzzleSolver();
    utility_fixture_resources::pending=true;
    const auto beforePendingReply=replies.size();
    check(queueUtility(L"puzzle on"),"first pending utility intent accepted");
    for(unsigned i=0;i<20;++i) pumpUtilities();
    check(!puzzleArmed() && !requests && !writes && replies.size()==beforePendingReply,"pending resource retains first utility intent without native work or premature error");
    utility_fixture_resources::finishPreparation();pumpUtilities();
    check(puzzleArmed() && replies.size()==beforePendingReply+1,"resource completion resumes the original first command once");
    puzzleCommand(L"off");
    utility_fixture_resources::pending=true;
    const auto orderedReply=replies.size();
    for(unsigned i=0;i<8;++i) check(queueUtility(i%2?L"puzzle off":L"puzzle on"),"all eight pending commands accepted in FIFO order");
    pumpUtilities();utility_fixture_resources::finishPreparation();
    for(unsigned i=0;i<8;++i) pumpUtilities();
    check(replies.size()==orderedReply+8 && !puzzleArmed(),"eight retained commands preserve every equal distinct result and FIFO on/off order");
    utility_fixture_resources::pending=true;check(queueUtility(L"puzzle on"),"pending stale intent accepted");pumpUtilities();
    ++epoch;updateUtilityContext();pumpUtilities();utility_fixture_resources::finishPreparation();pumpUtilities();
    check(!puzzleArmed() && !requests,"changed context cancels retained intent before preparation completes");
    load(trialUtilities[0],trialUtilities[0].treasure); resetPuzzleSolver();
#ifdef FFX_DEV_UTILITIES
    check(consoleVk==VK_F1 && consoleShift && consoleCallback && !keys.count(VK_F12),"development console uses Shift F1 and leaves F12 to NVDA");
    battle=false; scene=0; check(consoleAvailable(),"console chord reserved in field");
    consoleCallback(true); check(consoleOpened==1,"field context opens console");
    battle=true; check(consoleAvailable(),"battle reserves chord without native F1 fallthrough"); consoleCallback(true); battle=false;
    scene=6; check(consoleAvailable(),"menu reserves chord without native F1 fallthrough"); consoleCallback(true); scene=0;
    minigame=true; check(consoleAvailable(),"race reserves chord without native F1 fallthrough"); consoleCallback(true); minigame=false;
    check(consoleOpened==1,"unavailable contexts never open the console");
    check(warpCommand(L"196 0").find(L"Antechamber")!=std::wstring::npos && nextRoom==196 && nextSpawn==0,"Baaj seal test command selects Antechamber entrance zero");
    const auto beforeInvalidBaaj=warps;warpCommand(L"196 2");
    check(warps==beforeInvalidBaaj,"Baaj rejects entrance outside the two native entries");
    check(warpCommand(L"82 0").find(L"Djose Temple")!=std::wstring::npos && nextRoom==82 && nextSpawn==0,"Djose Temple exterior command");
    check(warpCommand(L"djose").find(L"Trials")!=std::wstring::npos && nextRoom==214,"Djose alias goes to trial chamber");
#else
    check(!consoleCallback&&!keys.count(VK_F12)&&executeUtility(L"warp besaid").find(L"Unknown")!=std::wstring::npos,"release/test build has no console chord or warp command entry");
#endif
    check(keys.count(VK_F8)==1,"mod menu remains available in every build");
    load(trialUtilities[0],trialUtilities[0].treasure); delayedActions=true;
    trialCommand(L"treasure");
    check(!requests && pendingActions.size()==1 && utilityBusy(),"deferred request reserves a busy signal before native invocation");
    auto requestAction=std::move(pendingActions.back()); pendingActions.clear();
    check(requestAction.prepare(requestAction),"reserved request remains qualified at owner boundary");
    requestAction.invoke(requestAction); requestAction.complete(requestAction,true);
    check(requests==1 && initPc==trialUtilities[0].treasure.pc && utilityBusy(),"deferred native initializer enters the exact reserved continuation");
    load(trialUtilities[0],trialUtilities[0].treasure); resetPuzzleSolver();
    trialCommand(L"treasure");
    auto cancelled=std::move(pendingActions.back()); pendingActions.clear();
    invalidateUtilityContext();
    check(!cancelled.prepare(cancelled),"context revision invalidates an already prepared command before native request");
    cancelled.complete(cancelled,false); check(!requests && !utilityBusy(),"cancelled reservation releases busy state");
    updateUtilityContext();
    load(trialUtilities[0],trialUtilities[0].treasure); trialCommand(L"treasure");
    auto occupied=std::move(pendingActions.back()); pendingActions.clear();
    put(target,0x88,ptr(helperSignal));
    check(!occupied.prepare(occupied),"native request slot consumed before dispatch cannot receive the reserved continuation");
    occupied.complete(occupied,false); check(!requests && !utilityBusy(),"lost native reservation cancels rather than running an unqualified entry");
    delayedActions=false;
    viaFixtures();
    std::puts("PASS: all-six natural interaction substitution, original reveal/collection handoff, destruction-only stay, normal completion, opt-out, generation guards, utilities and development/release console gates.");
} catch(const std::exception& error) {std::fprintf(stderr,"FAIL: uncaught fixture exception: %s\n",error.what());return 1;}
