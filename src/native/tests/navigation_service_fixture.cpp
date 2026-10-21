// Production command/scene/resource/route/intent pipeline over retained EVs.
// Native scene/getter and mutation endpoints are controlled adapters. Domain
// transport is semantic; actual capture/SDK/MinHook ownership has separate hosts.
#define FFX_NAVIGATION_SERVICE_TEST
#define main navigationSceneComponentEntry
#include "navigation_scene_fixture.cpp"
#undef main
#include "../navigation_intents.hpp"
#include "../puzzle_guide_owned.hpp"
#include "../puzzle_guide_targets.hpp"
#include "../nav_audio.hpp"
#include "../sphere_grid.hpp"
#include "../auto_walk.hpp"
#include "../reachability.hpp"
#include "../text_plan.hpp"
namespace {
unsigned starts=0,faces=0,arms=0,actions=0,refusals=0;
std::array<uint8_t,0x668> cameraBytes{};
}
namespace ffx {
// The unrelated Gil producer is covered by reader/TextPlan hosts; reaching it
// in this navigation command fixture is an error, never invented text behavior.
void prepareTextPlan(TextPlan&,const char*,bool) {check(false,"unexpected Gil producer");}
TextRef planLiteral(TextPlan&,const wchar_t*) {check(false,"unexpected Gil literal");return 0;}
TextRef planInteger(TextPlan&,int64_t) {check(false,"unexpected Gil value");return 0;}
TextRef planConcat(TextPlan&,TextRef,TextRef) {check(false,"unexpected Gil formatting");return 0;}
bool commitTextPlan(const TextPlan&) {check(false,"unexpected Gil commit");return false;}
std::vector<Entity> butterflyTargets() {check(false,"legacy live butterfly scan entered");return {};}
void refreshButterflyTarget(Entity&) {check(false,"legacy affine butterfly refresh entered");}
void refreshButterflyStarter(Entity&) {check(false,"legacy affine starter refresh entered");}
void hotkey(unsigned vk,void(*fn)(bool)) {navigationKeys[vk]=fn;}
bool nativeReaderResourcesReady() noexcept {return true;}
bool deferNativeAction(NativeAction action) {
    check(nativeOwner,"intent stays on Native action owner");bool ready=action.prepare(action);
    if(ready) {action.invoke(action);++actions;}else ++refusals;
    if(action.complete) action.complete(action,ready);if(action.retire) action.retire(action);return true;
}
void installAutoWalk() {}void installPedestal() {}
void stopAutoWalk() {++manualInputRevision;}
void startAutoWalk() {check(nativeOwner,"approved start stays Native");++starts;}
void cancelPedestal() {}
void armPedestalOwned(const NativeRouteTarget&,const UtilityScript&,const PedestalPlan&) {check(nativeOwner,"arming stays Native");++arms;}
void updatePedestalOwnedTarget(const NativeRouteTarget&) {check(nativeOwner,"POD assistance handback stays Native");}
bool faceNavigationTarget(const NativeRouteTarget&) {check(nativeOwner,"facing stays Native");++faces;return true;}
bool sphereGridOpen() {return false;}void spherePathKey(bool) {}
void resetBattle() {}void resetNavigationScripts() {}
void finishMapArrival() {}void announceCurrentMap() {}void prepareNavigationScriptRetirement() {}
void consoleReplyReady(const wchar_t*,size_t) {}
void adjustSetting(Setting,int) {}std::wstring settingValue(Setting) {return L"On";}
std::wstring menuIndex(unsigned,unsigned) {return {};}
void installWallCues() {}void captureWallCues() {}void cancelWallCues() {}
void publishNavAudio(const NavAudioState&) {}void pauseNavAudio() {}void pauseNavRouteAudio() {}void stopNavPreview() {}
void navAudioArrival() {}void publishPositionalCue(float,float,bool,unsigned) {}
}
#ifndef FFX_NAVIGATION_REAL_QUEUE_TEST
namespace {
void ownerEvent() {ffx::Event e;e.rva=0x225930;for(const auto& listener:navigationListeners[e.rva]) if(listener.after) listener.after(e);}
void work() {nativeOwner=false;ffx::drainSpeechDomainsForTest();ffx::drainReachabilityForTest();ffx::drainSpeechDomainsForTest();nativeOwner=true;}
void key(unsigned vk,bool shift=false) {nativeOwner=true;navigationKeys.at(vk)(shift);}
void fresh(unsigned events=3) {for(unsigned i=0;i<events;++i) {ownerEvent();work();}}
ffx::Entity selected() {ffx::Entity target;nativeOwner=false;check(ffx::navigationSelectedForTest(target),"owned command selection available");nativeOwner=true;return target;}
}
int main() {try {
    using namespace ffx;check(startAtelResources(),"pipeline resource bootstrap");installAtelResources();installNavigation();
    // Native encoded title getter: all requested IDs share a controlled ready
    // game-format string; exact getter bounds, copying and decoder are real.
    areaBytes[0]=0x50;areaBytes[1]=0x82;areaBytes[2]=0x74;areaBytes[3]=0x70;areaBytes[4]=0;
    prepare("cdsp0000");setScene();cameraPointer=reinterpret_cast<uintptr_t>(cameraBytes.data());put(cameraBytes.data()+0x40,1.f);
    put(actorBodies[0].bytes.data()+0x4fc,300.f);
    const auto begin=std::chrono::steady_clock::now();key(VK_OEM_3);fresh();
    check(!navigationPendingForTest() && !navigationSaid.empty() && areaGetterCalls>0,"cold scan resolves names in one batched Native getter handback");
    auto target=selected();const auto cold=std::chrono::duration<double,std::milli>(std::chrono::steady_clock::now()-begin).count();
    const auto warmNameCalls=areaGetterCalls;const auto before=navigationSaid.size();for(unsigned i=0;i<5;++i) key('P');fresh(1);
    check(areaGetterCalls==warmNameCalls,"warm copied scenes reuse exact bank/epoch/revision title facts");
    check(!navigationPendingForTest() && navigationSaid.size()==before+5,"five equal distance actions keep five outputs");
    for(size_t i=before+1;i<navigationSaid.size();++i) check(navigationSaid[i]==navigationSaid[before],"equal independent action contents preserved");
    // Select the recovered cdsp controller's actual depth exit and use the
    // production swimming route. This is a copied-script integration fixture.
    bool depth=target.swimDepth;for(unsigned i=0;i<128 && !depth;++i) {key(VK_OEM_6);fresh(1);target=selected();depth=target.swimDepth;}
    check(depth,"retained cdsp depth controller remains selectable");
    key(VK_OEM_5);ownerEvent();work();
    NavLeg leg;check(navLeg(leg) && leg.target.swimDepth,"owned command publishes a complete Native steering snapshot");
    const auto oldStarts=starts;ownerEvent();check(starts==oldStarts+1 && actions>0,"copied route hands approved start back to Native");work();
    key(VK_OEM_5);ownerEvent();work();const auto canceledStarts=starts;++manualInputRevision;ownerEvent();work();
    check(starts==canceledStarts && refusals>0,"manual input cancels delayed start without reactivating movement");
    const auto copiedBefore=navigationSaid.size();key('P');ownerEvent();
    for(auto& body:actorBodies) std::fill(body.bytes.begin(),body.bytes.end(),static_cast<uint8_t>(0xff));
    for(auto& worker:workers) std::fill(worker.begin()+0x558,worker.begin()+0x5a4,static_cast<uint8_t>(0xff));work();
    check(navigationSaid.size()==copiedBefore+1 && !foreignReads,"source overwrite cannot change copied output or trigger Worker engine reads");
    setScene();put(actorBodies[0].bytes.data()+0x4fc,300.f);
    for(unsigned i=0;i<4;++i) {key('P');ownerEvent();}const auto heldBefore=navigationSaid.size();
    key('P');ownerEvent();work();fresh();
    check(!navigationPendingForTest() && navigationSaid.size()>=heldBefore+5,"held observation pressure recovers and retains independent admitted commands");
    key('P');resetNavigation();work();fresh();check(!navigationPendingForTest(),"reset clears retired commands without stale effects");
    check(!foreignReads,"entire owned command, labeling, route and cue pipeline has no Worker engine getter/read");
    prepare("bsyt0100");setScene();check(nav::captureNavigationScene(sceneCapture,nav::navigationObservationRevision(),false),"guide captures actual Besaid Native declarations");
    check(sceneCapture.partCount==1 && sceneCapture.parts[0].guideReady,"every Besaid guide checkpoint is a qualified original declaration");
    for(unsigned row=0;row<sceneCapture.parts[0].bindings.count;++row) sceneCapture.parts[0].bindings.rows[row].value=0;
    auto changeGuideValue=[&](unsigned low,int v) {bool found=false;for(auto& row:sceneCapture.parts[0].bindings.rows) if(row.binding==low) {row.value=v;found=true;}check(found,"copied guide value exists");};
    speechq::Header guideHeader;guideHeader.generation=generation();
    auto guide=[&](OwnedGuideAction action) {nativeOwner=false;const auto text=puzzleGuideOwned(action,sceneCapture,Mesh{},std::vector<Entity>{},guideHeader,manualInputRevision,nav::navigationIntentRevision());nativeOwner=true;return text;};
    resetOwnedPuzzleGuide();check(guide(OwnedGuideAction::Step).find(L"Step 1 of")!=std::wstring::npos,"owned manual guide starts at original first step");
    check(guide(OwnedGuideAction::Previous).find(L"Step 1 of")!=std::wstring::npos,"owned previous clamps at first step");
    changeGuideValue(0x5c0000ec,1);check(guide(OwnedGuideAction::Step).find(L"Step 1 of")!=std::wstring::npos,"state change cannot auto-advance manual selection");
    check(guide(OwnedGuideAction::Expected).find(L"Step 2 of")!=std::wstring::npos,"expected follows actual copied durable checkpoint");
    check(guide(OwnedGuideAction::Step).find(L"Step 1 of")!=std::wstring::npos,"expected cannot move manual selection");
    check(guide(OwnedGuideAction::Next).find(L"Step 2 of")!=std::wstring::npos,"manual next advances one step");guide(OwnedGuideAction::Completed);
    check(guide(OwnedGuideAction::Step).find(L"Step 2 of")!=std::wstring::npos,"completion leaves manual selection unchanged");
    for(unsigned i=0;i<20;++i) guide(OwnedGuideAction::Next);check(guide(OwnedGuideAction::Step).find(L"Step 13 of 13")!=std::wstring::npos,"last-step clamp preserved");
    check(guide(OwnedGuideAction::Step).find(L"Besaid Sphere must be in the pedestal")!=std::wstring::npos,"missing original prerequisite preserved");
    changeGuideValue(0x5c0000cc,1);check(guide(OwnedGuideAction::Step).find(L"push it onto the final floor depression")!=std::wstring::npos,"original final objective preserved");
    changeGuideValue(0x5c0000dc,1);check(guide(OwnedGuideAction::Expected).find(L"Step 7 of")!=std::wstring::npos,"durable checkpoint advances expected through empty carried sphere");
    for(auto& body:actorBodies) std::fill(body.bytes.begin(),body.bytes.end(),static_cast<uint8_t>(0xff));
    const auto equalGuide=guide(OwnedGuideAction::Step);check(equalGuide==guide(OwnedGuideAction::Step) && !foreignReads,"equal guide actions and source overwrite use only copied facts");
    std::printf("PASS: owned real Besaid guide declarations, manual/expected/clamp/durable prerequisite semantics, equal requests and source overwrite; component cases; production command wiring is checked separately below.\n");
    setScene();resetOwnedPuzzleGuide();const auto beforeGuideQueue=navigationSaid.size();
    for(unsigned i=0;i<4;++i) check(nav::submitNavigationQuery(nav::Query::GuideStep,false,ownedPuzzleGuideRevision()),"admit distinct owned guide commands");fresh();
    check(navigationSaid.size()==beforeGuideQueue+4,"four equal guide commands retain four actual pipeline responses");
    for(size_t i=beforeGuideQueue;i<navigationSaid.size();++i) check(navigationSaid[i].find(L"Step 1 of")==0,"queued guide starts at original manual first step");
    check(nav::submitNavigationQuery(nav::Query::GuideNext,false,ownedPuzzleGuideRevision()),"admit guide next");fresh();check(navigationSaid.back().find(L"Step 2 of")==0,"queued guide next changes one manual step");
    const auto completionSpeech=navigationSaid.size();unsigned claimed=0;for(unsigned i=0;i<32;++i) if(claimOwnedGuideRefresh()) {++claimed;check(nav::submitNavigationQuery(nav::Query::GuideCompleted,false,ownedPuzzleGuideRevision()),"admit one environmental completion update");}fresh();
    check(claimed==1 && navigationSaid.size()==completionSpeech,"environmental completion replaces only silent state, without suppressing independent voiced commands");
    check(nav::submitNavigationQuery(nav::Query::GuideStep,false,ownedPuzzleGuideRevision()),"manual repeat after completion");fresh();check(navigationSaid.back().find(L"Step 2 of")==0,"queued completion preserves manual selection");
    check(!foreignReads,"queued guide and native scene pipeline never enters Worker getters");
    std::printf("PASS: owned guide integrated command FIFO, four equal responses, one-step navigation and bounded silent completion refresh; semantic transport endpoints only.\n");
    setScene();const auto origin=playerPosition();Entity fixedGoal;fixedGoal.name=L"Floor puzzle panel";fixedGoal.position={origin.x+5,origin.y,origin.z+10};
    const auto fixedStarts=starts;check(queuePuzzleGuidePoint(fixedGoal.position,fixedGoal.worker,fixedGoal.zone.talk,L"Stand beside the panel and wait for its spoken cue.",UtilityText{}),"admit owned fixed guide target");fresh(3);
    check(starts==fixedStarts+1,"fixed approved point starts only after matching Native scene/controller/pose/route guard");
    check(queuePuzzleGuidePoint(fixedGoal.position,fixedGoal.worker,fixedGoal.zone.talk,L"Stand beside the panel and wait for its spoken cue.",UtilityText{}),"admit delayed fixed target");ownerEvent();work();const auto canceledFixed=starts;++manualInputRevision;ownerEvent();work();check(starts==canceledFixed,"manual input cancels fixed-point start without reactivation");
    check(queuePuzzleGuidePoint(fixedGoal.position,fixedGoal.worker,fixedGoal.zone.talk,L"",UtilityText{}),"admit abandoned target payload");resetNavigation();speechq::fixture::pending.clear();work();
    check(queuePuzzleGuidePoint(fixedGoal.position,fixedGoal.worker,fixedGoal.zone.talk,L"",UtilityText{}),"fresh target payload recovers after canceled marker");fresh();check(!foreignReads,"owned direct guide routing never reads engine/getter on Worker");
    const auto typedBefore=navigationSaid.size();UtilityText intro(L"Step 1 of 11. Reach the panel.");
    check(queuePuzzleGuidePoint(fixedGoal.position,-1,false,L"Stand beside the panel.",intro),"admit fixed typed guide point without Native Entity");intro={};fresh(3);
    check(navigationSaid.size()==typedBefore+1 && navigationSaid.back().find(L"Step 1 of 11. Reach the panel.")==0,"copied direct guide prefix and route form one output after source reset");
    check(!foreignReads,"typed direct guide uses no Worker engine reads");
    std::printf("PASS: owned direct guide target lease, fixed Native start, manual cancellation and abandoned-marker recovery; semantic endpoints only.\n");
    std::printf("PASS: production owned navigation commands, batched names, five equal outputs, depth route, guarded Native start/manual cancellation, source reuse, pressure and reset.\nMETRIC cold copied pipeline %.3fms; semantic domain/mutation adapters, real EV/getter/resource/route code. No game/backend/audible claim.\n",cold);
    return 0;
}catch(const std::exception& e) {observe=false;std::fprintf(stderr,"FAIL: service fixture exception %s\n",e.what());return 1;}}

#endif
