#include "ready_speech_adapter.hpp"
#include "ready_utility_adapter.hpp"
#include "../text_plan.hpp"
#include "../events.hpp"
#include "test_copied_controls.hpp"
#include "../controller.hpp"
#include "../settings.hpp"
#include "../speech_backends.hpp"
#include "../mod_menu.hpp"
#include "../minigame_completion.hpp"
#include "../zanarkand_patterns.hpp"
#include "../sin_collection.hpp"
#include "../nav_audio.hpp"
#include "../airship_search.hpp"
#include "../puzzle_solver.hpp"
#include <map>
#include <cstdio>
#include <cstdlib>
namespace ffx {
bool captureControllerBinding(uint64_t);
std::vector<SpeechBackendChoice> speechBackendChoices() { return {{0,L"Automatic"},{1,L"NVDA"},{3,L"SAPI"}}; }
std::wstring speechBackendStatus() { return L"NVDA"; }
bool copySpeechBackendSnapshot(SpeechBackendSnapshot& out) {
    out={};out.ready=1;out.count=3;out.rows[0].preference=0;out.rows[1].preference=1;out.rows[2].preference=3;
    wcscpy_s(out.rows[0].name,L"Automatic");wcscpy_s(out.rows[1].name,L"NVDA");wcscpy_s(out.rows[2].name,L"SAPI");wcscpy_s(out.status,L"NVDA");return true;
}
int selectedBackend=-1;
void selectSpeechBackend(int id) { selectedBackend=id; }
bool testForeground=true;
unsigned epoch=1,drains=0,previews=0,repeats=0,navUpdates=0;
bool navPaused=false;
bool captured=false;
std::vector<std::wstring> said;
std::map<unsigned,void(*)(bool)> keys;
std::map<unsigned,bool(*)()> contexts;
std::atomic<int> scene{0};
bool fixtureEsc=false;
bool escMenuActive() {return fixtureEsc;}
unsigned generation() { return epoch; }
void log(const char*,...) {}
std::wstring join(const std::vector<std::wstring>& parts,const wchar_t* separator) {
    std::wstring value;for(const auto& part:parts) if(!part.empty()) {if(!value.empty()) value+=separator;value+=part;}return value;
}
bool hook(unsigned,Handler,Handler) {return false;}
uintptr_t address(unsigned) {return 0;}
bool copy(uintptr_t,void*,size_t) {return false;}
bool outputDialogue(const std::wstring& text,bool interrupt,const char* producer,bool) {return output(text,interrupt,producer);}
void emitSelectionDetailOwned(std::wstring,SelectionDetailToken,bool,const speechq::Header&) {}
void appendSelectionDetailOwned(std::wstring,SelectionDetailToken) {}
bool output(const std::wstring& text,bool,const char*) { said.push_back(text); return true; }
void hotkey(unsigned vk,void(*handler)(bool)) { keys[vk]=handler; }
void hotkeyWhen(unsigned vk,void(*handler)(bool),bool(*available)()) {keys[vk]=handler;contexts[vk]=available;}
void captureSpeech(bool value) { captured=value; }
bool namingContext=false; unsigned nameReads=0;
bool namingReadbackAvailable() {return namingContext;}
bool readEnteredName() { if(!namingContext) return false; ++nameReads; return true; }
void repeatSpeech(bool) { ++repeats; }
void drainController() { ++drains; }
void pauseNavAudio() { navPaused=true; }
Point playerPosition() { return {5,0,5}; }
void refreshSelectedBeacon() {}
void refreshMinigameAudio() {}
void updateNavAudio(Point point,bool moving) {
    if(point.x!=5 || moving) std::exit(1);
    ++navUpdates; navPaused=modMenuActive();
}
void stopNavPreview() {}
void previewNavAudio(bool) { ++previews; }
bool inBattle() { return false; }
std::wstring lightningDodgeControls() { return L"Dodge: fixture control."; }
bool puzzleArmed() { return false; }
std::wstring puzzleAction; bool trialPresent=false,viaPresent=false;
bool fixtureGagazet=false; std::wstring gagazetAction;
bool fixtureSin=false,sinArmedFixture=false; std::wstring sinAction;
unsigned minigameAssistMask=0;
void cancelMinigameCompletion() {}
bool minigameAssistAvailable(Setting id) {
    unsigned bit=0;
    switch(id) {
    case Setting::ChocoboAssist: bit=0; break;
    case Setting::RemiemAssist: bit=1; break;
    case Setting::ButterflyAssist: bit=2; break;
    case Setting::BlitzAssist: bit=3; break;
    default: return false;
    }
    return (minigameAssistMask&(1u<<bit))!=0;
}
bool viaPurificoActive() { return viaPresent; }
bool viaCollecting() { return false; }
UtilityText viaCommand(std::wstring_view action) { puzzleAction=action;return L"Fixture dungeon"; }
UtilityText puzzleCommand(std::wstring_view action) { puzzleAction=action; return L"Fixture trial"; }
bool gagazetActive() { return fixtureGagazet; }
UtilityText gagazetCommand(std::wstring_view action) { gagazetAction=action; return L"Fixture Gagazet"; }
bool sinCollectionActive() { return fixtureSin; }
bool sinCollectionArmed() { return sinArmedFixture; }
UtilityText sinCollectionCommand(std::wstring_view action) { sinAction=action;if(action==L"on")sinArmedFixture=true;else if(action==L"off")sinArmedFixture=false;return L"Fixture Sin collection"; }
UtilityScript utilityScript() { return {}; }
bool fixtureZanarkand=false,patternReady=false;
ZanarkandPatternSnapshot displayedPattern;
bool captureUtilityScript(UtilityScript& out) noexcept {out={};if(fixtureZanarkand)out.name="dome0700";return true;}
bool captureZanarkandPattern(ZanarkandPatternSnapshot& out) {out={};if(!patternReady)return false;out=displayedPattern;return true;}
const TrialUtility* currentTrial(const UtilityScript&) {
    static const TrialUtility trial=[] { TrialUtility value{}; value.name=L"Fixture trial"; return value; }();
    return trialPresent?&trial:nullptr;
}
const TrialUtility* captureCurrentTrial(const UtilityScript& script) noexcept {return currentTrial(script);}
void utilityReply(const std::wstring& text) { said.push_back(text); }
void consoleReplyReady(const wchar_t*,size_t) {}
void resetSpherePlanner() {}
bool spherePlannerActive() { return false; }
bool fixtureWakka=false;
bool fixtureAirshipContext=false;
unsigned begunAirshipDestination=99;
bool airshipBeginAfterClose=false;
bool hiddenAirshipMenuAvailable() { return setting(Setting::HiddenAirshipDestinations)!=0 && fixtureAirshipContext; }
bool beginHiddenAirshipSearch(unsigned destination) { if(!hiddenAirshipMenuAvailable()) return false; begunAirshipDestination=destination; airshipBeginAfterClose=!modMenuActive()&&!captured; return true; }
unsigned wakkaKey=0,wakkaResets=0;
bool wakkaMenuActive() { return fixtureWakka; }
void wakkaMenuKey(unsigned vk) { wakkaKey=vk; }
void resetWakka() { fixtureWakka=false; ++wakkaResets; }
void spherePlannerKey(unsigned,bool) {}
}
#include "test_speech_domains.hpp"
using namespace ffx;
void menuKeyForTest(unsigned key,bool shift=false) {ffx::modMenuKey(key,shift);drainSpeechDomainsForTest();}
void openMenuForTest(const ModMenuState& state) {ffx::openModMenu(state);drainSpeechDomainsForTest();}
void pressForTest(unsigned key,bool shift=false) {if(!contexts.count(key) || contexts[key]()) keys[key](shift);drainSpeechDomainsForTest();}
void check(bool ok,const char* message) { if(!ok) { fprintf(stderr,"FAIL: %s\n",message); exit(1); } }
void selectRow(const std::wstring& label) {
    menuKeyForTest(VK_HOME);
    for(unsigned i=0;i<64;++i) {
        if(said.back().find(label)==0) return;
        menuKeyForTest(VK_DOWN);
    }
    check(false,"requested row is accessible in the current menu");
}
int main() {
    installTextPlans();
    SetErrorMode(SEM_FAILCRITICALERRORS|SEM_NOGPFAULTERRORBOX|SEM_NOOPENFILEERRORBOX);
    initializeControllerBindings();
    pad::Router router; pad::State state;
    router.update(state,true,false); check(!router.draining,"neutral focus acquisition");
    state.held=pad::bit(pad::View); auto event=router.update(state,true,false);
    check(router.mod && event.exclusive && !event.edges,"View consumed as modifier");
    state={}; router.update(state,true,false);
    state.held=pad::bit(pad::North); event=router.update(state,true,false);
    check(event.exclusive && event.edges==state.held,"single mod action edge");
    router.complete(); check(router.update(state,true,false).exclusive,"held command cannot leak into game");
    state={}; router.update(state,true,false);
    state.held=pad::bit(pad::South); check(!router.update(state,true,false).exclusive,"game controls restored after neutral");
    router.update(state,false,false); check(!router.mod && router.draining,"focus loss cancels command mode");
    router.update(state,true,false); check(router.draining,"held controls cannot leak on refocus");
    state={}; router.update(state,true,false);
    check(router.update(state,true,true).exclusive,"settings take exclusive controller ownership");
    pad::Router speechRouter; state={}; speechRouter.update(state,true,false);
    state.lx=6000; check(speechRouter.update(state,true,false).input,"small intentional stick motion interrupts before gesture threshold");
    check(!speechRouter.update(state,true,false).input,"held analog direction cannot repeatedly silence announcements");
    state={}; check(!speechRouter.update(state,true,false).input,"returning to neutral is not a new speech interruption");
    state.held=pad::bit(pad::View); check(speechRouter.update(state,true,false).input,"View prefix is deliberate input even when its edge is consumed");
    pad::Gesture gesture;
    check(gesture.update(0,25000)==0 && gesture.update(0,18000)==0,"stick hysteresis");
    check(gesture.update(0,0)==-1 && gesture.update(-25000,0)==2,"stick gesture rearm");
    resetControllerBindings(); setSetting(Setting::Speech,1);
    installModMenu(); setSetting(Setting::MenuIndex,1); setSetting(Setting::AutoDetail,0); setSetting(Setting::Beacon,0); setSetting(Setting::AutoWalk,0);
    pressForTest(VK_F6,false);
    check(navUpdates==1 && !navPaused,"F6 on immediately refreshes the route without movement");
    pressForTest(VK_F6,false);
    check(!setting(Setting::Beacon) && navUpdates==2 && !navPaused,"F6 off refreshes cues without pausing independent wall tones");
    check(!contexts[VK_F9]() && !contexts[VK_F10]() && !contexts[VK_F11](),"ordinary field releases F9-F11 instead of globally claiming guide/repeat commands");
    scene=6;
    pressForTest(VK_F6,false); pressForTest(VK_F7,false); pressForTest(VK_F11,false);
    check(setting(Setting::Beacon)==1 && setting(Setting::AutoDetail)==1 && repeats==1,"beacon, autodetail and repeat shortcuts are distinct");
    namingContext=true; pressForTest(VK_F11,false); check(nameReads==1 && repeats==1,"F11 reads entered name in naming context");
    namingContext=false; pressForTest(VK_F11,false); check(repeats==2,"closing naming restores menu F11 repeat");
    scene=0;trialPresent=true;
    pressForTest(VK_F10,false); check(!modMenuActive() && said.back()==L"Fixture trial","path shortcut does not open settings");
    trialPresent=false;
    pressForTest(VK_F8,false);
    check(modMenuActive() && captured,"spoken menu captures game narration");
    check(said.back()==L"Accessibility mod menu. Speech: On, 1 of 6","main menu keeps Speech, Speech options and existing grouped submenus plus Close");
    menuKeyForTest(VK_F6); menuKeyForTest(VK_F7);
    check(!setting(Setting::Beacon) && !setting(Setting::AutoDetail),"toggles remain available inside settings");
    check(navPaused,"F6 inside settings keeps route audio paused");
    const int previousPunctuationOption=setting(Setting::SpeakPunctuationNames);
    setSetting(Setting::SpeakPunctuationNames,0);
    selectRow(L"Speech options, submenu"); menuKeyForTest(VK_RETURN);
    check(said.back()==L"Speech options. Speak punctuation names: Off, 1 of 2" &&
        settingInfo(Setting::SpeakPunctuationNames).initial==0,"Speech options exposes punctuation naming Off by default");
    menuKeyForTest(VK_RIGHT);
    check(setting(Setting::SpeakPunctuationNames)==1 && said.back()==L"Speak punctuation names: On, 1 of 2","Speech options toggles punctuation naming On");
    menuKeyForTest(VK_LEFT);
    check(setting(Setting::SpeakPunctuationNames)==0 && said.back()==L"Speak punctuation names: Off, 1 of 2","Speech options restores punctuation naming Off");
    menuKeyForTest(VK_ESCAPE);
    setSetting(Setting::SpeakPunctuationNames,previousPunctuationOption);
    selectRow(L"Miscellaneous, submenu"); menuKeyForTest(VK_RIGHT);
    check(said.back()==L"Miscellaneous. Autodetail: Off, 1 of 14","Miscellaneous groups other settings and omits trial controls outside a trial");
    selectRow(L"Jecht Shot auto-success:");check(settingInfo(Setting::JechtAssist).initial==0,"Jecht automation defaults Off");
    setSetting(Setting::JechtAssist,0);menuKeyForTest(VK_RIGHT);drainSettingsForTest();loadSettings();
    check(setting(Setting::JechtAssist)==1,"Jecht assistance is accessible and persists");menuKeyForTest(VK_LEFT);
    selectRow(L"Autodetail:"); menuKeyForTest('I');
    check(said.back()==settingInfo(Setting::AutoDetail).description,"I reads selected setting description");
    selectRow(L"Overdrive auto-success:");
    setSetting(Setting::OverdriveAssist,1); menuKeyForTest(VK_RETURN);
    check(!setting(Setting::OverdriveAssist) && said.back().find(L"Overdrive auto-success")!=std::wstring::npos,"F8 menu owns Overdrive auto-success toggle");
    selectRow(L"Voiced dialogue output:");
    setSetting(Setting::VoicedDialogue,0); menuKeyForTest(VK_RIGHT);
    check(setting(Setting::VoicedDialogue)==1 && said.back().find(L"Braille only")!=std::wstring::npos,"voiced dialogue row changes mode and announces label");
    menuKeyForTest(VK_RETURN); check(setting(Setting::VoicedDialogue)==2,"Enter cycles voiced dialogue");
    menuKeyForTest(VK_RIGHT); check(setting(Setting::VoicedDialogue)==0,"three-way mode wraps forward");
    menuKeyForTest(VK_LEFT); check(setting(Setting::VoicedDialogue)==2,"three-way mode wraps backward");
    setSetting(Setting::VoicedDialogue,1); drainSettingsForTest();loadSettings();
    check(setting(Setting::VoicedDialogue)==1,"voiced dialogue preference survives settings reload");
    setSetting(Setting::VoicedDialogue,0);
    selectRow(L"Lightning auto-dodge:");
    check(settingInfo(Setting::LightningAutoDodge).initial==0,"lightning automation defaults Off");
    setSetting(Setting::LightningAutoDodge,0); menuKeyForTest(VK_RIGHT); drainSettingsForTest();loadSettings();
    check(setting(Setting::LightningAutoDodge)==1,"Miscellaneous exposes and saves lightning auto-dodge independently");
    menuKeyForTest(VK_LEFT); check(!setting(Setting::LightningAutoDodge),"lightning auto-dodge can be disabled");
    selectRow(L"Show maximum HP:");
    check(settingInfo(Setting::ShowMaximumHp).initial==0,"battle HP defaults to bar percentage");
    setSetting(Setting::ShowMaximumHp,0); menuKeyForTest(VK_RIGHT); drainSettingsForTest();loadSettings();
    check(setting(Setting::ShowMaximumHp)==1,"Miscellaneous saves the numeric maximum HP toggle");
    menuKeyForTest(VK_LEFT); check(!setting(Setting::ShowMaximumHp),"bar percentage can be restored");
    selectRow(L"Speak maximum MP:");
    check(settingInfo(Setting::ShowMaximumMp).initial==0,"H defaults to displayed current MP");
    setSetting(Setting::ShowMaximumMp,0); menuKeyForTest(VK_RIGHT); drainSettingsForTest();loadSettings();
    check(setting(Setting::ShowMaximumMp)==1 && !setting(Setting::ShowMaximumHp),"Miscellaneous persists independent maximum MP");
    menuKeyForTest(VK_LEFT); check(!setting(Setting::ShowMaximumMp),"current MP can be restored");
    selectRow(L"Overdrive cues:"); setSetting(Setting::OverdriveCues,1); menuKeyForTest(VK_LEFT); drainSettingsForTest();loadSettings();
    check(!setting(Setting::OverdriveCues),"Miscellaneous persists manual overdrive audio independently");
    menuKeyForTest(VK_RIGHT); check(setting(Setting::OverdriveCues)==1,"manual overdrive audio can be restored");
    menuKeyForTest(VK_ESCAPE);
    check(said.back().find(L"Accessibility mod menu. Miscellaneous, submenu")==0,"Miscellaneous Back restores its own main-menu row");
    menuKeyForTest(VK_RIGHT); selectRow(L"Speech backend: NVDA, submenu"); menuKeyForTest(VK_RETURN);
    check(said.back()==L"Speech backend. Automatic, 1 of 4","Speech backend choices are listed in their own submenu");
    menuKeyForTest('I'); check(said.back().find(L"Choose Automatic or an available speech backend.")==0,"Speech backend description is available by keyboard");
    menuKeyForTest(VK_DOWN); menuKeyForTest(VK_DOWN);
    check(said.back()==L"SAPI, 3 of 4","SAPI selection has the correct menu position");
    menuKeyForTest(VK_RETURN); check(selectedBackend==3 && said.back()==L"SAPI, 3 of 4","Enter selects the SAPI Prism preference");
    menuKeyForTest(VK_END); check(said.back()==L"Back, 4 of 4","Speech backend Back remains the final row");
    menuKeyForTest(VK_RETURN); check(said.back().find(L"Miscellaneous. Speech backend: NVDA, submenu")==0,"Speech backend Back restores its parent row"); menuKeyForTest(VK_ESCAPE);
    selectRow(L"Navigation audio, submenu"); menuKeyForTest(VK_RIGHT);
    check(said.back().find(L"Navigation audio. Audio beacon:")==0 && modMenuActive() && captured,"Right opens the named audio submenu with game speech still captured");
    selectRow(L"Beacon volume:"); setSetting(Setting::BeaconVolume,60); menuKeyForTest(VK_RIGHT);
    check(setting(Setting::BeaconVolume)==70 && said.back().find(L"70 percent")!=std::wstring::npos,"audio submenu adjusts the existing saved volume");
    selectRow(L"Lightning warning:"); setSetting(Setting::LightningWarning,1); menuKeyForTest(VK_LEFT); drainSettingsForTest();loadSettings();
    check(!setting(Setting::LightningWarning) && !setting(Setting::LightningAutoDodge),"lightning warning preference is independent and persists");
    menuKeyForTest(VK_RIGHT); check(setting(Setting::LightningWarning)==1,"lightning warning can be reenabled");
    selectRow(L"Butterfly cues:");
    check(settingInfo(Setting::ButterflyCues).initial==1,"butterfly audio defaults On independently");
    setSetting(Setting::ButterflyCues,1); menuKeyForTest(VK_LEFT); drainSettingsForTest();loadSettings();
    check(!setting(Setting::ButterflyCues) && setting(Setting::RaceCues),"butterfly setting persists without disabling race cues");
    menuKeyForTest(VK_RIGHT); check(setting(Setting::ButterflyCues)==1,"butterfly audio can be reenabled");
    selectRow(L"Preview beacon"); menuKeyForTest(VK_RIGHT);
    check(previews==0,"Right cannot accidentally trigger an audio preview");
    menuKeyForTest(VK_RETURN); check(previews==1,"Enter activates the audio preview");
    selectRow(L"Jecht Shot cues:");check(settingInfo(Setting::JechtCues).initial==1,"manual Jecht cues default On");
    setSetting(Setting::JechtCues,1);menuKeyForTest(VK_LEFT);drainSettingsForTest();loadSettings();
    check(!setting(Setting::JechtCues) && !setting(Setting::JechtAssist),"Jecht manual cues save independently of assistance");menuKeyForTest(VK_RIGHT);
    menuKeyForTest(VK_END); check(said.back()==L"Back, 14 of 14","End stays within the audio submenu");
    menuKeyForTest(VK_RETURN);
    check(said.back().find(L"Accessibility mod menu. Navigation audio, submenu")==0 && captured,"Back returns to the audio entry without closing settings");
    selectRow(L"Navigation, submenu"); menuKeyForTest(VK_RETURN);
    check(said.back()==L"Navigation. Auto walk: Off, 1 of 6","Auto walk is first in the named Navigation submenu");
    check(settingInfo(Setting::HiddenAirshipDestinations).initial==0 && !hiddenAirshipMenuAvailable(),"hidden airship picker is unavailable by default");
    selectRow(L"Enable hidden destinations menu for airship map:");
    check(said.back().find(L"Enable hidden destinations menu for airship map: Off")==0,"default-off setting remains visible without exposing its picker action");
    setSetting(Setting::HiddenAirshipDestinations,1); fixtureAirshipContext=true;
    check(hiddenAirshipMenuAvailable(),"enabled setting and native context expose the picker action");
    selectRow(L"Hidden destinations for airship map, submenu"); menuKeyForTest(VK_RETURN);
    check(said.back().find(L"Hidden destinations for airship map. Baaj Temple")==0,"picker opens through the contextual Navigation action");
    menuKeyForTest(VK_ESCAPE);
    check(said.back().find(L"Navigation. Hidden destinations for airship map, submenu")==0,"picker Back restores the parent selection");
    for(unsigned destination=0;destination<6;++destination) {
        selectRow(L"Hidden destinations for airship map, submenu"); menuKeyForTest(VK_RETURN);
        menuKeyForTest(VK_HOME); for(unsigned i=0;i<destination;++i) menuKeyForTest(VK_DOWN);
        check(said.back().find(hiddenAirshipNames[destination])==0,"picker exposes the approved destination name");
        menuKeyForTest(VK_RETURN);
        check(!modMenuActive() && !captured && begunAirshipDestination==destination && airshipBeginAfterClose,"each picker row closes settings before dispatching its destination");
        pressForTest(VK_F8,false); selectRow(L"Navigation, submenu"); menuKeyForTest(VK_RETURN);
    }
    selectRow(L"Hidden destinations for airship map, submenu"); menuKeyForTest(VK_RETURN);
    fixtureAirshipContext=false; menuKeyForTest(VK_RETURN);
    check(!modMenuActive() && !captured && begunAirshipDestination==5,"native context loss closes picker without dispatch");
    fixtureAirshipContext=true;
    pressForTest(VK_F8,false); selectRow(L"Navigation, submenu"); menuKeyForTest(VK_RETURN);
    selectRow(L"Enable hidden destinations menu for airship map:"); setSetting(Setting::HiddenAirshipDestinations,0);
    check(!hiddenAirshipMenuAvailable(),"turning off the setting closes the availability gate");
    setSetting(Setting::HiddenAirshipDestinations,1);
    selectRow(L"Partial steps:");
    setSetting(Setting::PartialSteps,0); menuKeyForTest(VK_RIGHT);
    check(setting(Setting::PartialSteps)==1 && said.back().find(L"Partial steps: On")!=std::wstring::npos,"partial distances have an accessible settings toggle");
    drainSettingsForTest();loadSettings(); check(setting(Setting::PartialSteps)==1,"partial-distance choice survives reload");
    menuKeyForTest(VK_LEFT); check(setting(Setting::PartialSteps)==0,"whole distances can be restored");
    selectRow(L"Auto walk:");
    setSetting(Setting::AutoWalk,0); menuKeyForTest(VK_RIGHT);
    check(setting(Setting::AutoWalk)==1 && said.back().find(L"Auto walk: On")!=std::wstring::npos,"F8 exposes the requested auto-walk toggle");
    drainSettingsForTest();loadSettings(); check(setting(Setting::AutoWalk)==1,"auto-walk setting survives reload");
    menuKeyForTest(VK_LEFT); check(setting(Setting::AutoWalk)==0,"auto walk can be switched off independently of route audio");
    selectRow(L"Speak auto walk path:"); setSetting(Setting::AutoWalkPath,0); menuKeyForTest(VK_RIGHT);
    check(setting(Setting::AutoWalkPath)==1,"path narration toggles independently of auto walking");
    drainSettingsForTest();loadSettings(); check(setting(Setting::AutoWalkPath)==1,"path narration choice persists");
    menuKeyForTest(VK_LEFT); check(!setting(Setting::AutoWalkPath),"route speech can be turned off");
    selectRow(L"Hide unreachable destinations:"); menuKeyForTest('I');
    check(said.back()==settingInfo(Setting::HideUnreachable).description,"Navigation descriptions use the selected setting, independent of its enum index");
    menuKeyForTest(VK_ESCAPE);
    check(said.back().find(L"Accessibility mod menu. Navigation, submenu")==0 && captured,"Escape restores the Navigation parent entry");
    selectRow(L"Navigation, submenu"); menuKeyForTest(VK_RIGHT); menuKeyForTest(VK_END); menuKeyForTest(VK_LEFT);
    check(said.back().find(L"Accessibility mod menu. Navigation, submenu")==0,"Left on Back returns to the saved parent row");
    selectRow(L"Miscellaneous, submenu"); menuKeyForTest(VK_RETURN);
    selectRow(L"Autodetail:");
    menuKeyForTest(VK_RETURN); check(setting(Setting::AutoDetail),"settings row changes same autodetail option as F7");
    drainSettingsForTest();
    check(GetPrivateProfileIntW(L"Accessibility",L"auto_detail",999,settingsPath().c_str())==1,"autodetail persists");
    menuKeyForTest(VK_ESCAPE);
    menuKeyForTest(VK_HOME);
    menuKeyForTest(VK_LEFT); check(!setting(Setting::Speech),"boolean setting changes");
    menuKeyForTest(VK_RIGHT); check(setting(Setting::Speech),"speech setting remains reachable while disabled");
    selectRow(L"Miscellaneous, submenu"); menuKeyForTest(VK_RETURN);
    selectRow(L"Controller bindings, submenu");
    menuKeyForTest(VK_RETURN); menuKeyForTest(VK_RETURN);
    check(learningControllerBinding(),"bindings menu starts learn");
    captureControllerBinding(pad::bit(pad::West));
    check(controllerBinding(pad::PreviousTarget)==pad::West && controllerBinding(pad::Gil)==pad::Left,"binding collision swaps assignments");
    drainSettingsForTest();
    check(GetPrivateProfileIntW(L"Controller",L"command_1",999,settingsPath().c_str())==pad::West,"rebinding persists");
    menuKeyForTest(VK_RETURN); captureControllerBinding(pad::bit(pad::View));
    check(!learningControllerBinding() && controllerBinding(pad::PreviousTarget)==pad::West,"cancel preserves binding");
    menuKeyForTest(VK_RETURN); menuKeyForTest(VK_F8);
    check(!learningControllerBinding() && modMenuActive(),"F8 cancels binding learn");
    menuKeyForTest(VK_END); menuKeyForTest(VK_RETURN);
    check(said.back().find(L"Miscellaneous. Controller bindings, submenu")==0,"binding Back restores its own nested parent entry");
    menuKeyForTest(VK_ESCAPE);
    check(modMenuActive() && captured,"leaving Miscellaneous keeps settings open");
    menuKeyForTest(VK_ESCAPE);
    check(!modMenuActive() && !captured && drains>0,"close releases speech and drains held controller");
    check(!navPaused,"closing settings refreshes route audio without waiting for movement");
    pressForTest(VK_F8,false); menuKeyForTest(VK_F8); check(!modMenuActive(),"F8 closes captured settings");
    setSetting(Setting::AutoDetail,0);
    setSetting(Setting::BeaconVolume,100); adjustSetting(Setting::BeaconVolume,1);
    check(setting(Setting::BeaconVolume)==100,"volume clamps instead of wrapping");
    setSetting(Setting::BeaconVolume,60); resetControllerBindings();
    drainSettingsForTest();loadSettings(); check(setting(Setting::Speech)==1,"persisted settings reload");
    trialPresent=true; pressForTest(VK_F9,false); check(puzzleAction==L"previous","F9 selects previous trial step");
    pressForTest(VK_F10,false); check(puzzleAction==L"next","F10 selects next trial step");
    pressForTest(VK_F10,true); check(puzzleAction==L"step","Shift F10 reads selected trial step");
    pressForTest(VK_F11,false); check(puzzleAction==L"step","F11 reads the selected trial step, independent of speech history");
    pressForTest(VK_F11,true); check(puzzleAction==L"expected","Shift F11 reads expected trial state without changing selection");
    ModMenuState noGagazetMenu{};noGagazetMenu.status=L"Fixture context";noGagazetMenu.epoch=epoch;
    openMenuForTest(noGagazetMenu);selectRow(L"Miscellaneous, submenu");menuKeyForTest(VK_RETURN);menuKeyForTest(VK_END);
    check(said.back()==L"Back, 14 of 14","Gagazet cave puzzles are absent from Miscellaneous outside their context");
    menuKeyForTest(VK_ESCAPE);menuKeyForTest(VK_ESCAPE);
    fixtureGagazet=true;
    pressForTest(VK_F11,false); check(gagazetAction==L"step","F11 reads the Gagazet selected step");
    pressForTest(VK_F11,true); check(gagazetAction==L"expected","Shift F11 reads the Gagazet expected state");
    fixtureGagazet=false;
    ModMenuState noSinMenu{};noSinMenu.status=L"Fixture context";noSinMenu.epoch=epoch;
    openMenuForTest(noSinMenu);selectRow(L"Miscellaneous, submenu");menuKeyForTest(VK_RETURN);menuKeyForTest(VK_END);
    check(said.back()==L"Back, 14 of 14","Sin collection submenu is absent outside its captured context");
    menuKeyForTest(VK_ESCAPE);menuKeyForTest(VK_ESCAPE);
    constexpr Setting minigameAssistSettings[]={Setting::ChocoboAssist,Setting::RemiemAssist,Setting::ButterflyAssist,Setting::BlitzAssist};
    const auto priorMenuIndex=setting(Setting::MenuIndex);
    // These assertions include the optional counts; restore the caller's setting below.
    setSetting(Setting::MenuIndex,1);
    const auto availableMinigameRows=[&](unsigned rows) {
        unsigned mask=0;menuKeyForTest(VK_HOME);
        for(unsigned row=0;row<rows;++row) {
            for(unsigned option=0;option<_countof(minigameAssistSettings);++option) {
                const std::wstring prefix=std::wstring(settingInfo(minigameAssistSettings[option]).name)+L":";
                if(said.back().find(prefix)==0) mask|=1u<<option;
            }
            menuKeyForTest(VK_DOWN);
        }
        return mask;
    };
    minigameAssistMask=0;
    ModMenuState minigameMenu{};minigameMenu.status=L"Fixture context";minigameMenu.epoch=epoch;
    openMenuForTest(minigameMenu);selectRow(L"Miscellaneous, submenu");menuKeyForTest(VK_RETURN);
    check(availableMinigameRows(14)==0,"all four minigame assistance rows are individually hidden outside preparation contexts");
    menuKeyForTest(VK_END);check(said.back()==L"Back, 14 of 14","Miscellaneous keeps its existing row count outside preparation contexts");
    menuKeyForTest(VK_ESCAPE);menuKeyForTest(VK_ESCAPE);
    for(unsigned i=0;i<_countof(minigameAssistSettings);++i) {
        minigameAssistMask=1u<<i;
        check(minigameAssistAvailable(minigameAssistSettings[i]),"fixture exposes only the selected minigame preparation context");
        openMenuForTest(minigameMenu);selectRow(L"Miscellaneous, submenu");menuKeyForTest(VK_RETURN);
        check(said.back()==L"Miscellaneous. Autodetail: Off, 1 of 15","one available minigame setting adds exactly one Miscellaneous row");
        const std::wstring label=settingInfo(minigameAssistSettings[i]).name;
        check(availableMinigameRows(15)==(1u<<i),"each minigame preparation flag exposes only its matching setting row");
        selectRow(label+L":");
        check(setting(minigameAssistSettings[i])==0 && settingInfo(minigameAssistSettings[i]).initial==0,
            "available minigame assistance is a visible default-off setting");
        menuKeyForTest(i==3?VK_RETURN:VK_RIGHT);
        check(setting(minigameAssistSettings[i])==1,"existing keyboard/controller menu action toggles available minigame assistance");
        menuKeyForTest(VK_ESCAPE);
        check(modMenuActive() && setting(minigameAssistSettings[i])==1,"Back preserves minigame assistance while the parent menu remains open");
        selectRow(L"Miscellaneous, submenu");menuKeyForTest(VK_RETURN);selectRow(label+L":");
        check(setting(minigameAssistSettings[i])==1,"reopening Miscellaneous preserves the selected minigame assistance value");
        menuKeyForTest(VK_ESCAPE);menuKeyForTest(VK_ESCAPE);
        check(!modMenuActive() && setting(minigameAssistSettings[i])==1,"closing settings preserves minigame assistance");
        setSetting(minigameAssistSettings[i],0);
    }
    minigameAssistMask=0;setSetting(Setting::MenuIndex,priorMenuIndex);
    fixtureSin=true;sinArmedFixture=false;
    pressForTest(VK_F8,false);check(sinAction==L"status","F8 captures active Sin context and obtains its initial status");
    selectRow(L"Miscellaneous, submenu");menuKeyForTest(VK_RETURN);
    selectRow(L"Sin object collection, submenu");menuKeyForTest(VK_RETURN);
    check(said.back()==L"Sin object collection. Collect objects automatically: Off, 1 of 4","Sin collection submenu and default-off row use the captured context");
    menuKeyForTest(VK_RIGHT);check(sinAction==L"on"&&sinArmedFixture,"Sin collection toggle enables its saved preference command");
    check(said.back()==L"Fixture Sin collection","Sin collection toggle speaks through the mod-menu producer");
    menuKeyForTest(VK_LEFT);check(sinAction==L"off"&&!sinArmedFixture,"Sin collection toggle can explicitly disarm");
    selectRow(L"Read collection instructions");menuKeyForTest(VK_RETURN);check(sinAction==L"guide","Sin collection instructions dispatch the guide command");
    selectRow(L"Collection status");menuKeyForTest(VK_RETURN);check(sinAction==L"status","Sin collection status dispatches the status command");
    menuKeyForTest(VK_END);check(said.back().find(L"Back")==0,"Sin collection Back row is available");menuKeyForTest(VK_RETURN);
    check(said.back().find(L"Miscellaneous. Sin object collection, submenu")==0,"Sin collection Back restores its parent selection");
    menuKeyForTest(VK_ESCAPE);menuKeyForTest(VK_ESCAPE);fixtureSin=false;
    pressForTest(VK_F8,false); selectRow(L"Miscellaneous, submenu"); menuKeyForTest(VK_RETURN);
    selectRow(L"Cloister of Trials, submenu"); menuKeyForTest(VK_RETURN);
    check(said.back().find(L"Cloister of Trials. Solve current trial automatically:")==0,"supported trial submenu announces its own title");
    selectRow(L"Read path to current step"); menuKeyForTest(VK_RETURN);
    check(puzzleAction==L"path","trial actions remain reachable after menu reorganization");
    menuKeyForTest(VK_END); menuKeyForTest(VK_RETURN);
    check(said.back().find(L"Miscellaneous. Cloister of Trials, submenu")==0,"trial Back restores its nested parent row");
    menuKeyForTest(VK_ESCAPE);
    menuKeyForTest(VK_ESCAPE);
    pressForTest(VK_F8,false); selectRow(L"Navigation, submenu"); menuKeyForTest(VK_RETURN); ++epoch; menuKeyForTest(VK_DOWN);
    check(!modMenuActive() && !captured,"a generation change closes submenus and releases capture");
    trialPresent=false; const auto beforeRepeat=repeats; pressForTest(VK_F11,false);
    check(repeats==beforeRepeat,"F11 is handed back on the ordinary field after leaving a trial");
    fixtureEsc=true;pressForTest(VK_F11,false);fixtureEsc=false;
    check(repeats==beforeRepeat+1,"F11 retains repeat in the native Escape menu");
    check(settingInfo(Setting::MenuIndex).initial==0 && settingInfo(Setting::AutoWalkPath).initial==0,"new accessibility defaults are off");
    pressForTest(VK_F8,false); selectRow(L"Miscellaneous, submenu"); menuKeyForTest(VK_RETURN);
    selectRow(L"Menu indexing:"); setSetting(Setting::MenuIndex,1); menuKeyForTest(VK_LEFT);
    check(said.back()==L"Menu indexing: Off","menu indexing off removes only the optional count");
    drainSettingsForTest();loadSettings(); check(!setting(Setting::MenuIndex),"menu indexing choice persists"); menuKeyForTest(VK_ESCAPE); menuKeyForTest(VK_ESCAPE);
    viaPresent=true;pressForTest(VK_F8,false);selectRow(L"Miscellaneous, submenu");menuKeyForTest(VK_RETURN);selectRow(L"Via Purifico, submenu");menuKeyForTest(VK_RETURN);
    check(said.back().find(L"Via Purifico. Collect all treasures automatically")!=std::wstring::npos,"Via gets its own contextual dungeon controls");
    pressForTest(VK_F11,false);check(puzzleAction==L"step","F11 retains the Via Purifico selected-step route");
    pressForTest(VK_F11,true);check(puzzleAction==L"expected","Shift F11 retains the Via Purifico expected-state route");
    menuKeyForTest(VK_RETURN);check(puzzleAction==L"on","Via automatic collection is explicit");
    menuKeyForTest(VK_ESCAPE);menuKeyForTest(VK_ESCAPE);menuKeyForTest(VK_ESCAPE);viaPresent=false;
    fixtureGagazet=true;
    ModMenuState gagazetMenu{};gagazetMenu.status=L"Fixture Gagazet";gagazetMenu.epoch=epoch;gagazetMenu.gagazet=true;
    openMenuForTest(gagazetMenu);
    selectRow(L"Miscellaneous, submenu");menuKeyForTest(VK_RETURN);
    check(said.back().find(L"Miscellaneous. Autodetail:")==0,"Gagazet context preserves the Miscellaneous menu");
    selectRow(L"Gagazet cave puzzles, submenu");menuKeyForTest(VK_RETURN);
    check(said.back().find(L"Gagazet cave puzzles. Gagazet puzzle auto-success:")==0,"Gagazet cave puzzle submenu is exposed in its active context");
    check(settingInfo(Setting::GagazetAssist).initial==0 && settingInfo(Setting::GagazetCues).initial==1,"Gagazet assistance defaults Off and cues default On");
    setSetting(Setting::GagazetAssist,0);menuKeyForTest(VK_RIGHT);check(setting(Setting::GagazetAssist)==1,"Gagazet automatic solving row toggles");
    menuKeyForTest(VK_LEFT);check(setting(Setting::GagazetAssist)==0,"Gagazet automatic solving can be disabled");
    setSetting(Setting::GagazetCues,1);menuKeyForTest(VK_DOWN);menuKeyForTest(VK_RIGHT);check(setting(Setting::GagazetCues)==0,"Gagazet cue row toggles independently");
    menuKeyForTest(VK_RIGHT);check(setting(Setting::GagazetCues)==1,"Gagazet cues can be restored independently");
    selectRow(L"Read puzzle instructions");menuKeyForTest(VK_RETURN);check(gagazetAction==L"guide","Gagazet instructions dispatch the guide command");
    selectRow(L"Read selected opening");menuKeyForTest(VK_RETURN);check(gagazetAction==L"selected","Gagazet selected opening dispatches the selected command");
    selectRow(L"Puzzle status");menuKeyForTest(VK_RETURN);check(gagazetAction==L"status","Gagazet status dispatches the status command");
    menuKeyForTest(VK_END);check(said.back().find(L"Back")==0,"Gagazet Back is present");menuKeyForTest(VK_RETURN);
    check(said.back().find(L"Miscellaneous. Gagazet cave puzzles, submenu")==0,"Gagazet Back restores its parent selection");
    menuKeyForTest(VK_ESCAPE);menuKeyForTest(VK_ESCAPE);
    gagazetMenu.epoch=epoch;openMenuForTest(gagazetMenu);selectRow(L"Miscellaneous, submenu");menuKeyForTest(VK_RETURN);selectRow(L"Gagazet cave puzzles, submenu");menuKeyForTest(VK_RETURN);
    ++epoch;menuKeyForTest(VK_DOWN);check(!modMenuActive()&&!captured,"a generation change closes the Gagazet submenu and releases capture");
    fixtureGagazet=false;
    fixtureWakka=true; check(modMenuActive(),"native reel choice acquires central menu ownership");
    menuKeyForTest(VK_RETURN); check(wakkaKey==VK_RETURN,"keyboard and controller-confirm menu action routes to reel choice");
    menuKeyForTest(VK_DOWN); check(wakkaKey==VK_DOWN,"central menu routes reel-choice browsing without opening settings");
    const auto oldResets=wakkaResets; resetModMenu();
    check(!modMenuActive() && wakkaResets==oldResets+1,"context reset centrally releases reel-choice ownership");
    trialPresent=fixtureZanarkand=true;setSetting(Setting::MenuIndex,1);pressForTest(VK_F8);
    selectRow(L"Miscellaneous, submenu");menuKeyForTest(VK_RETURN);selectRow(L"Cloister of Trials, submenu");menuKeyForTest(VK_RETURN);
    selectRow(L"Review displayed glyph sequence");menuKeyForTest(VK_RETURN);
    check(said.back().find(L"No completed glyph display")!=std::wstring::npos,"unseen pattern review asks for the original display");
    displayedPattern={5,false,{57,58,59,61,64}};patternReady=true;menuKeyForTest(VK_RETURN);
    check(said.back().find(L"Last displayed glyph sequence. 1: Floor panel 2: orange T")!=std::wstring::npos,"F8 opens the completed display as a named arrow buffer");
    menuKeyForTest(VK_DOWN);check(said.back().find(L"2: Floor panel 3: yellow straight line")==0,"sequence Down selects the next distinct glyph");
    menuKeyForTest('I');check(said.back().find(L"Floor panel 9: blue L")!=std::wstring::npos,"sequence I reads every original glyph");
    menuKeyForTest(VK_UP);check(said.back().find(L"1: Floor panel 2: orange T")==0,"sequence Up returns to the prior glyph");
    menuKeyForTest(VK_END);check(said.back().find(L"Back")==0,"sequence buffer has an accessible Back row");menuKeyForTest(VK_RETURN);
    check(said.back().find(L"Cloister of Trials. Review displayed glyph sequence")==0,"sequence Back restores its parent focus");
    menuKeyForTest(VK_RETURN);patternReady=false;menuKeyForTest(VK_DOWN);check(!modMenuActive(),"retired pattern context closes the review buffer");
    fixtureZanarkand=false;pressForTest(VK_F8);selectRow(L"Miscellaneous, submenu");menuKeyForTest(VK_RETURN);selectRow(L"Cloister of Trials, submenu");menuKeyForTest(VK_RETURN);menuKeyForTest(VK_END);
    check(said.back()==L"Back, 7 of 7","other trials keep their existing menu without a Zanarkand review entry");menuKeyForTest(VK_F8);
    puts("PASS: modifier ownership, focus/neutral drain, gestures, spoken menu, settings persistence, reel-choice routing, bindings and scoped Zanarkand glyph review.");
}
