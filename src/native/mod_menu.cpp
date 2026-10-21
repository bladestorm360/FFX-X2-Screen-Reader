// F8 and context-owned submenus over the settings authority. Native context
// capture supplies trial/puzzle availability; owned menu state handles rows and
// descriptions. Keep enum/page order, settings defaults, persistence and binding
// capture synchronized. See Docs/features/runtime/index.md#input-and-settings.
#include "mod_menu.hpp"
#include "zanarkand_patterns.hpp"
#include "wakka.hpp"
#include "puzzle_solver.hpp"
#include "via_purifico.hpp"
#include "utilities.hpp"
#include "events.hpp"
#include "settings.hpp"
#include "speech_backends.hpp"
#include "controller.hpp"
#include "nav_audio.hpp"
#include "airship_search.hpp"
#include "sphere_grid.hpp"
#include "naming.hpp"
#include "minigames.hpp"
#include "lightning.hpp"
#include "gagazet.hpp"
#include "sin_collection.hpp"
#include "minigame_completion.hpp"
#include "text_plan.hpp"
#include "esc_details.hpp"
#include <atomic>
#include <stdexcept>
#include <cstring>
#include <algorithm>
namespace ffx {
namespace {
enum class Page { Main,Audio,Navigation,Miscellaneous,Bindings,Puzzle,SpeechBackends,HiddenAirshipDestinations,ZanarkandPattern,Gagazet,SinCollection,SpeechOptions,Count };
std::atomic<bool> active{false};
Page page=Page::Main;
unsigned row=0,epoch=0;
struct MenuContext { bool supported=false,armed=false,via=false,zanarkand=false,gagazet=false,sin=false;unsigned epoch=0;UtilityText status;ZanarkandPatternSnapshot pattern; } context;
SpeechBackendSnapshot speechSnapshot{};
constexpr unsigned SettingsCount=static_cast<unsigned>(Setting::Count);
enum Action : unsigned { AudioMenu=SettingsCount,NavigationMenu,MiscMenu,BindingsMenu,TrialMenu,BeaconPreview,WallPreview,SpeechBackendMenu,HiddenAirshipMenu,GagazetMenu,GagazetGuide,GagazetSelected,GagazetStatus,SinMenu,SinGuide,SinStatus,Back,Close,SpeechOptionsMenu };
constexpr unsigned option(Setting id) { return static_cast<unsigned>(id); }
constexpr unsigned mainItems[]={option(Setting::Speech),SpeechOptionsMenu,AudioMenu,NavigationMenu,MiscMenu,Close};
constexpr unsigned audioItems[]={option(Setting::Beacon),option(Setting::BeaconVolume),option(Setting::Walls),
    option(Setting::WallVolume),option(Setting::WallRange),option(Setting::Arrival),option(Setting::ArrivalVolume),
    option(Setting::RaceCues),option(Setting::ButterflyCues),option(Setting::LightningWarning),option(Setting::JechtCues),BeaconPreview,WallPreview,Back};
constexpr unsigned navigationItems[]={option(Setting::AutoWalk),option(Setting::AutoWalkPath),option(Setting::HideUnreachable),option(Setting::PartialSteps),option(Setting::HiddenAirshipDestinations),HiddenAirshipMenu,Back};
constexpr unsigned miscItems[]={option(Setting::AutoDetail),option(Setting::VoicedDialogue),option(Setting::CombatDetail),
    option(Setting::MenuIndex),option(Setting::OverdriveAssist),option(Setting::OverdriveCues),option(Setting::LightningAutoDodge),option(Setting::JechtAssist),option(Setting::ShowMaximumHp),option(Setting::ShowMaximumMp),option(Setting::PolishText),option(Setting::ChocoboAssist),option(Setting::RemiemAssist),option(Setting::ButterflyAssist),option(Setting::BlitzAssist),SpeechBackendMenu,BindingsMenu,TrialMenu,GagazetMenu,SinMenu,Back};
constexpr unsigned gagazetItems[]={option(Setting::GagazetAssist),option(Setting::GagazetCues),GagazetGuide,GagazetSelected,GagazetStatus,Back};
constexpr unsigned sinItems[]={SinMenu,SinGuide,SinStatus,Back};
constexpr unsigned speechItems[]={option(Setting::SpeakPunctuationNames),Back};
struct MenuDefinition { const wchar_t* title; const unsigned* items; unsigned count; };
constexpr MenuDefinition menus[]={
    {L"Accessibility mod menu",mainItems,_countof(mainItems)},
    {L"Navigation audio",audioItems,_countof(audioItems)},
    {L"Navigation",navigationItems,_countof(navigationItems)},
    {L"Miscellaneous",miscItems,_countof(miscItems)},
    {L"Controller bindings",nullptr,pad::BindingCount+2},
    {L"Cloister of Trials",nullptr,7},
    {L"Speech backend",nullptr,0},
    {L"Hidden destinations for airship map",nullptr,7},
    {L"Last displayed glyph sequence",nullptr,0},
    {L"Gagazet cave puzzles",gagazetItems,_countof(gagazetItems)},
    {L"Sin object collection",sinItems,_countof(sinItems)},
    {L"Speech options",speechItems,_countof(speechItems)}};
static_assert(_countof(menus)==static_cast<unsigned>(Page::Count));
struct MenuPosition { Page page=Page::Main; unsigned row=0; };
MenuPosition parents[_countof(menus)];
const MenuDefinition& definition() { return menus[static_cast<unsigned>(page)]; }
void captureSpeechStatus() {
    SpeechBackendSnapshot current{};
    if(copySpeechBackendSnapshot(current)) std::memcpy(speechSnapshot.status,current.status,sizeof(speechSnapshot.status));
    else speechSnapshot.status[0]=0;
}
bool visible(unsigned item) {
    if(item==TrialMenu) return context.supported;
    if(item==HiddenAirshipMenu) return hiddenAirshipMenuAvailable();
    if(item==GagazetMenu) return context.gagazet;
    if(item==SinMenu) return context.sin;
    if(item==option(Setting::ChocoboAssist) || item==option(Setting::RemiemAssist) ||
       item==option(Setting::ButterflyAssist) || item==option(Setting::BlitzAssist))
        return minigameAssistAvailable(static_cast<Setting>(item));
    return true;
}
unsigned count() {
    if(page==Page::Puzzle && context.zanarkand) return 8;
    if(page==Page::ZanarkandPattern) return context.pattern.count+1;
    if(page==Page::SpeechBackends) return static_cast<unsigned>(speechSnapshot.ready && speechSnapshot.count>0?std::min(speechSnapshot.count,static_cast<int>(speechSnapshot.rows.size())):0)+1;
    if(page==Page::HiddenAirshipDestinations) return 7;
    const auto& menu=definition(); if(!menu.items) return menu.count;
    unsigned total=0; for(unsigned i=0;i<menu.count;++i) if(visible(menu.items[i])) ++total;
    return total;
}
unsigned selectedItem() {
    const auto& menu=definition(); unsigned local=0;
    if(menu.items) for(unsigned i=0;i<menu.count;++i) if(visible(menu.items[i]) && local++==row) return menu.items[i];
    return Close;
}
TextRef recipeRow(TextPlan& p) {
    if(page==Page::ZanarkandPattern) return row<context.pattern.count?
        planConcat(p,planInteger(p,row+1),planConcat(p,planLiteral(p,L": "),planLiteral(p,zanarkandGlyphLabel(context.pattern.glyphs[row])))):planLiteral(p,L"Back");
    if(page==Page::SpeechBackends) return row+1<count()?planLiteral(p,speechSnapshot.rows[row].name):planLiteral(p,L"Back");
    if(page==Page::HiddenAirshipDestinations) return planLiteral(p,row<6?hiddenAirshipNames[row]:L"Back");
    if(page==Page::SinCollection) {
        if(row==0) return planConcat(p,planLiteral(p,L"Collect objects automatically: "),planSettingValue(p,sinCollectionArmed()?1:0,0));
        return planLiteral(p,row==1?L"Read collection instructions":row==2?L"Collection status":L"Back");
    }
    if(page==Page::Bindings) {
        if(row>=pad::BindingCount) return planLiteral(p,row==pad::BindingCount?L"Restore controller defaults":L"Back");
        const auto command=pad::bindingCommands[row];
        const auto control=controllerBinding(command);
        return planConcat(p,planConcat(p,planLiteral(p,pad::names[command]),planLiteral(p,L": ")),planLiteral(p,controllerControlNameLiteral(control)));
    }
    if(page==Page::Puzzle) {
        if(context.zanarkand && row>=6) return planLiteral(p,row==6?L"Review displayed glyph sequence":L"Back");
        const wchar_t* labels[]={L"Solve current trial automatically",L"Read current puzzle step",L"Read path to current step",L"Next puzzle step",L"Previous puzzle step",L"Trial status",L"Back"};
        auto base=planLiteral(p,context.via && row==0?L"Collect all treasures automatically":labels[row]);
        return row==0?planConcat(p,base,planConcat(p,planLiteral(p,L": "),planSettingValue(p,(context.via?viaCollecting():puzzleArmed())?1:0,0))):base;
    }
    const auto item=selectedItem();
    if(item<SettingsCount) {
        const auto id=static_cast<Setting>(item);unsigned style=0;
        if(id==Setting::VoicedDialogue) style=1; else if(id==Setting::CombatDetail) style=2;
        else if(id==Setting::PolishText) style=3; else if(id==Setting::WallRange) style=4;
        else if(settingInfo(id).maximum>1) style=5;
        const auto value=setting(id);
        return planConcat(p,planConcat(p,planLiteral(p,settingInfo(id).name),planLiteral(p,L": ")),planSettingValue(p,value,style));
    }
    switch(item) {
    case SpeechBackendMenu: captureSpeechStatus();return planConcat(p,planConcat(p,planLiteral(p,L"Speech backend: "),planLiteral(p,speechSnapshot.status)),planLiteral(p,L", submenu"));
    case HiddenAirshipMenu: return planLiteral(p,L"Hidden destinations for airship map, submenu");
    case GagazetMenu: return planLiteral(p,L"Gagazet cave puzzles, submenu");
    case GagazetGuide: return planLiteral(p,L"Read puzzle instructions");
    case GagazetSelected: return planLiteral(p,L"Read selected opening");
    case GagazetStatus: return planLiteral(p,L"Puzzle status");
    case SinMenu: return planLiteral(p,L"Sin object collection, submenu");
    case SpeechOptionsMenu: return planLiteral(p,L"Speech options, submenu");
    case AudioMenu: return planLiteral(p,L"Navigation audio, submenu");
    case NavigationMenu: return planLiteral(p,L"Navigation, submenu");
    case MiscMenu: return planLiteral(p,L"Miscellaneous, submenu");
    case BindingsMenu: return planLiteral(p,L"Controller bindings, submenu");
    case TrialMenu: return planLiteral(p,context.via?L"Via Purifico, submenu":L"Cloister of Trials, submenu");
    case BeaconPreview: return planLiteral(p,L"Preview beacon");
    case WallPreview: return planLiteral(p,L"Preview wall tones");
    case Back: return planLiteral(p,L"Back");
    default: return planLiteral(p,L"Close mod menu");
    }
}
void emitReady(const wchar_t* value,const char* producer="mod-menu") {
    TextPlan p;prepareTextPlan(p,producer);p.headline=planLiteral(p,value);commitTextPlan(p);
}
void announce(bool opening=false) {
    TextPlan p;prepareTextPlan(p,"mod-menu");TextRef root=0;
    if(opening) root=planConcat(p,planLiteral(p,page==Page::Puzzle && context.via?L"Via Purifico":definition().title),planLiteral(p,L". "));
    root=planConcat(p,root,recipeRow(p));
    if(setting(Setting::MenuIndex)) root=planConcat(p,root,planConcat(p,planLiteral(p,L", "),planConcat(p,planInteger(p,row+1),planConcat(p,planLiteral(p,L" of "),planInteger(p,count())))));
    p.headline=root;commitTextPlan(p);
}
void close() {
    active=false; captureSpeech(false); cancelControllerBinding(); drainController(); pauseNavAudio(); stopNavPreview();
    emitReady(L"Mod menu closed");
    refreshSelectedBeacon();
      updateNavAudio(playerPosition(),false);
    refreshMinigameAudio();
}
void enterPage(Page target) {
    if(target==Page::SpeechBackends) {speechSnapshot={};copySpeechBackendSnapshot(speechSnapshot);}
    parents[static_cast<unsigned>(target)]={page,row}; page=target; row=0;
    stopNavPreview(); drainController(); announce(true);
}
void back() {
    if(page==Page::Main) { close(); return; }
    const auto parent=parents[static_cast<unsigned>(page)]; page=parent.page; row=parent.row;
    stopNavPreview(); drainController(); announce(true);
}
void description() {
    if(page==Page::ZanarkandPattern) {
        TextPlan p;prepareTextPlan(p,"mod-menu");TextRef text=0;
        for(unsigned i=0;i<context.pattern.count;++i) {
            if(i) text=planConcat(p,text,planLiteral(p,L". "));
            text=planConcat(p,text,planConcat(p,planInteger(p,i+1),planConcat(p,planLiteral(p,L": "),planLiteral(p,zanarkandGlyphLabel(context.pattern.glyphs[i])))));
        }
        p.headline=text;commitTextPlan(p);return;
    }
    if(page==Page::Puzzle && context.zanarkand && row==6) {emitReady(L"Interact with a pattern display first. After its animation finishes, review the displayed glyphs here with Up and Down. Panel names match the floor targets. I reads the whole sequence.");return;}
    if(page==Page::SpeechBackends) { captureSpeechStatus();TextPlan p;prepareTextPlan(p,"mod-menu");p.headline=planConcat(p,planLiteral(p,L"Choose Automatic or an available speech backend. Speech and braille use its supported channels. Current backend: "),planLiteral(p,speechSnapshot.status));commitTextPlan(p);return; }
    if(page==Page::Bindings && row<=pad::BindingCount) emitReady(L"Enter or South face starts rebinding. Press a button or move a stick. View or Menu cancels. View and Menu are reserved. Assignments swap when a control is already used.");
    else if(page==Page::Puzzle && row<6) emitUtilityText(context.status,"mod-menu");
    else {
        const auto item=(page==Page::Bindings || page==Page::Puzzle)?Back:selectedItem();
        if(item<SettingsCount) {
            const auto* description=settingInfo(static_cast<Setting>(item)).description;
            if(item==option(Setting::LightningWarning)) { TextPlan p;prepareTextPlan(p,"mod-menu");const auto name=planPrimaryDodgeName(p);const auto named=planSuffix(p,planPrefix(p,planLiteral(p,L"Dodge: "),name,L""),planLiteral(p,L". Press once when the beep begins."),L"");
                const auto controls=planPair(p,TextPlanOp::Or,named,planLiteral(p,L"Press the game's configured dodge action once when the beep begins."));p.headline=planConcat(p,planConcat(p,planLiteral(p,description),planLiteral(p,L" ")),controls);commitTextPlan(p); }
            else emitReady(description);
        }
        else if(item==SpeechBackendMenu) emitReady(L"Choose Automatic or an available Prism speech backend. Voice and speed remain controlled by that backend.");
        else if(item==SpeechOptionsMenu) emitReady(L"Open speech settings for naming glyphs.");
        else if(item==AudioMenu) emitReady(L"Open beacon, wall and arrival sounds, volumes, wall range, race pickup cues, lightning warnings and sound previews.");
        else if(item==NavigationMenu) emitReady(L"Open Auto walk, path announcements, the unreachable destination filter and Partial steps.");
        else if(item==MiscMenu) emitReady(L"Open speech detail, voiced dialogue, menu indexing, translation text, timing assistance, Chocobo and Remiem races, butterfly hunts and Blitzball when available, controller bindings, trial controls, Gagazet cave puzzles and Sin object collection when available.");
        else if(item==Back) emitReady(L"Return to the parent menu and its selected submenu entry. Settings remain open.");
        else emitReady(L"Up and Down select. Left and Right change a value. Right or Enter opens a submenu. Enter activates. I reads the description. Escape returns. Home and End go to the first and last row.");
    }
}
void puzzleAction() {
    const wchar_t* commands[]={L"",L"step",L"path",L"next",L"previous",L"status"};
    try {
        if(context.via) { const auto text=viaCommand(row==0?(viaCollecting()?L"off":L"on"):commands[row]);emitUtilityText(text,"mod-menu"); }
        else if(row==0) { const auto text=puzzleCommand(puzzleArmed()?L"off":L"on");emitUtilityText(text,"mod-menu"); }
        else { const auto text=puzzleCommand(commands[row]);emitUtilityText(text,"mod-menu"); }
    } catch(const std::exception& error) {
        emitUtilityText(utilityError(L"",error.what(),L""),"mod-menu");
    }
}
void activate(int delta,bool confirmed) {
    if(page==Page::ZanarkandPattern) {
        if(row==context.pattern.count) {if(confirmed || delta<0) back();}
        else if(confirmed) announce();
        return;
    }
    if(page==Page::HiddenAirshipDestinations && !hiddenAirshipMenuAvailable()) { close(); return; }
    if(page==Page::SpeechBackends) {
        if(row+1==count()) { if(confirmed || delta<0) back(); }
        else if(confirmed) selectSpeechBackend(speechSnapshot.rows[row].preference);
        return;
    }
    if(page==Page::SinCollection) {
        if(row==3) { if(confirmed || delta<0) back(); return; }
        if(!context.sin) { close(); return; }
        if(row==0 && (confirmed || delta!=0)) {
            try { emitUtilityText(sinCollectionCommand(sinCollectionArmed()?L"off":L"on"),"mod-menu"); }
            catch(const std::exception& error) { emitUtilityText(utilityError(L"",error.what(),L""),"mod-menu"); }
        } else if(confirmed && row==1) {
            try { emitUtilityText(sinCollectionCommand(L"guide"),"mod-menu"); }
            catch(const std::exception& error) { emitUtilityText(utilityError(L"",error.what(),L""),"mod-menu"); }
        } else if(confirmed && row==2) {
            try { emitUtilityText(sinCollectionCommand(L"status"),"mod-menu"); }
            catch(const std::exception& error) { emitUtilityText(utilityError(L"",error.what(),L""),"mod-menu"); }
        }
        return;
    }
    if(page==Page::Bindings) {
        if(row==pad::BindingCount+1) { if(confirmed || delta<0) back(); }
        else if(confirmed) { if(row<pad::BindingCount) learnControllerBinding(pad::bindingCommands[row]); else resetControllerBindings(); }
    } else if(page==Page::Puzzle) {
        if(row==(context.zanarkand?7u:6u)) { if(confirmed || delta<0) back(); }
        else if(context.zanarkand && row==6) {
            if(confirmed || delta>0) {
                if(captureZanarkandPattern(context.pattern)) enterPage(Page::ZanarkandPattern);
                else emitReady(L"No completed glyph display is available. Interact with a pattern display first.");
            }
        } else if(confirmed) puzzleAction();
    } else if(page==Page::HiddenAirshipDestinations) {
        if(row==6) { if(confirmed || delta<0) back(); }
        else if(confirmed) {
            if(!hiddenAirshipMenuAvailable()) { close(); return; }
            const auto destination=row;
            close();
            beginHiddenAirshipSearch(destination);
        }
    } else {
        const auto item=selectedItem();
        if(item<SettingsCount) { adjustSetting(static_cast<Setting>(item),delta); announce(); pauseNavAudio(); return; }
        if(item==Back) { if(confirmed || delta<0) back(); return; }
        if(!confirmed && delta<0) return;
        switch(item) {
        case SpeechBackendMenu: enterPage(Page::SpeechBackends); break;
        case SpeechOptionsMenu: enterPage(Page::SpeechOptions); break;
        case HiddenAirshipMenu: if(hiddenAirshipMenuAvailable()) enterPage(Page::HiddenAirshipDestinations); else announce(); break;
        case GagazetMenu: if(context.gagazet) enterPage(Page::Gagazet); else announce(); break;
        case SinMenu: if(context.sin) enterPage(Page::SinCollection); else announce(); break;
        case AudioMenu: enterPage(Page::Audio); break;
        case NavigationMenu: enterPage(Page::Navigation); break;
        case MiscMenu: enterPage(Page::Miscellaneous); break;
        case BindingsMenu: enterPage(Page::Bindings); break;
        case TrialMenu: enterPage(Page::Puzzle); break;
        case GagazetGuide: if(confirmed && context.gagazet) { try { emitUtilityText(gagazetCommand(L"guide"),"mod-menu"); } catch(const std::exception& e) { emitUtilityText(utilityError(L"",e.what(),L""),"mod-menu"); } } break;
        case GagazetSelected: if(confirmed && context.gagazet) { try { emitUtilityText(gagazetCommand(L"selected"),"mod-menu"); } catch(const std::exception& e) { emitUtilityText(utilityError(L"",e.what(),L""),"mod-menu"); } } break;
        case GagazetStatus: if(confirmed && context.gagazet) { try { emitUtilityText(gagazetCommand(L"status"),"mod-menu"); } catch(const std::exception& e) { emitUtilityText(utilityError(L"",e.what(),L""),"mod-menu"); } } break;
        case BeaconPreview: if(confirmed) previewNavAudio(false); break;
        case WallPreview: if(confirmed) previewNavAudio(true); break;
        default: if(confirmed) close();
        }
    }
}
void menuKey(bool) {
    if(active) { close(); return; }
    ModMenuState snapshot; snapshot.epoch=generation();
    snapshot.gagazet=gagazetActive();
    snapshot.sin=sinCollectionActive();
    try {
        if(snapshot.sin) snapshot.status=sinCollectionCommand(L"status");
        else if(snapshot.gagazet) snapshot.status=gagazetCommand(L"status");
        else if(!inBattle()) {
            UtilityScript script;const auto trial=captureUtilityScript(script)?captureCurrentTrial(script):nullptr;
            if(trial) { snapshot.supported=true; snapshot.zanarkand=script.name=="dome0700"; snapshot.armed=puzzleArmed(); snapshot.status=puzzleCommand(L"status"); }
            else if(viaPurificoActive()) { snapshot.supported=snapshot.via=true;  snapshot.armed=viaCollecting(); snapshot.status=viaCommand(L"status"); }
        }
    } catch(...) { snapshot.supported=false; }
    openModMenu(snapshot);
}
void guideKey(const wchar_t* action) {
    try {
        const std::wstring_view command(action);
        const bool gagazetCommandSupported=command==L"guide" || command==L"selected" || command==L"status" || command==L"step" || command==L"expected";
        const auto text=gagazetActive() && gagazetCommandSupported?gagazetCommand(command):(viaPurificoActive()?viaCommand(action):puzzleCommand(action));
        emitUtilityText(text,"mod-menu");
    }
    catch(const std::exception& error) { emitUtilityText(utilityError(L"",error.what())); }
}
void stepKey(bool) { guideKey(L"previous"); }
void pathKey(bool shift) { guideKey(shift?L"step":L"next"); }
bool guideKeysAvailable() {
    // F9/F10 belong to an actual guide, not every ordinary field. Capture uses
    // the prepared native script identity; no parsing, worker wait or speech.
    if(inBattle() || scene) return false;
    if(gagazetActive() || viaPurificoActive()) return true;
    UtilityScript script;
    return captureUtilityScript(script) && captureCurrentTrial(script)!=nullptr;
}
bool readKeyAvailable() {
    // Keep contextual naming/puzzle readback and menu/battle repeat, but hand
    // unused field F11 back to native input (F6-F8 remain reserved globally).
    return namingReadbackAvailable() || guideKeysAvailable() || scene || inBattle() || escMenuActive();
}
void readPuzzleKey(bool shift) {
    if(readEnteredName()) return;
    if(gagazetActive()) { guideKey(shift?L"expected":L"step"); return; }
    bool trial=false;
    UtilityScript script;trial=!inBattle() && captureUtilityScript(script) && captureCurrentTrial(script)!=nullptr;
    if(trial || viaPurificoActive()) guideKey(shift?L"expected":L"step"); else repeatSpeech(shift);
}
void beaconKey(bool) {
    adjustSetting(Setting::Beacon,1);
    TextPlan p;prepareTextPlan(p,"mod-setting");p.headline=planConcat(p,planLiteral(p,L"Audio beacon: "),planSettingValue(p,setting(Setting::Beacon),0));commitTextPlan(p);
    refreshSelectedBeacon();
    updateNavAudio(playerPosition(),false);
}
void detailKey(bool) {
    adjustSetting(Setting::AutoDetail,1);
    TextPlan p;prepareTextPlan(p,"mod-setting");p.headline=planConcat(p,planLiteral(p,L"Autodetail: "),planSettingValue(p,setting(Setting::AutoDetail),0));commitTextPlan(p);
}
}
void openModMenu(const ModMenuState& snapshot) {
    cancelMinigameCompletion();
    resetSpherePlanner();
    copySpeechBackendSnapshot(speechSnapshot);
    context={};context.supported=snapshot.supported;context.armed=snapshot.armed;context.via=snapshot.via;context.zanarkand=snapshot.zanarkand;context.gagazet=snapshot.gagazet;context.sin=snapshot.sin;context.epoch=snapshot.epoch;epoch=snapshot.epoch;
    if(!snapshot.status.valid) return;context.status=snapshot.status;
    (void)snapshot.trial;page=Page::Main; row=0; active=true;
    captureSpeech(true); drainController(); pauseNavAudio(); announce(true);
}
void openHiddenAirshipMenu() {
    if(!hiddenAirshipMenuAvailable()) return;
    resetSpherePlanner(); context={}; epoch=generation(); parents[static_cast<unsigned>(Page::HiddenAirshipDestinations)]={Page::Main,0}; page=Page::HiddenAirshipDestinations; row=0; active=true;
    captureSpeech(true); drainController(); pauseNavAudio(); announce(true);
}
void modMenuReply(const std::wstring&) {}
bool modMenuActive() { return active.load() || spherePlannerActive() || wakkaMenuActive(); }
void modMenuKey(unsigned vk,bool shift) {
    if(wakkaMenuActive()) { wakkaMenuKey(vk); return; }
    if(spherePlannerActive()) { spherePlannerKey(vk,shift); return; }
    if(!active) return;
    if(epoch!=generation()) { close(); return; }
    if(page==Page::ZanarkandPattern) {
        ZanarkandPatternSnapshot current;
        if(!captureZanarkandPattern(current) || current.count!=context.pattern.count || current.glyphs!=context.pattern.glyphs) {close();return;}
    }
    if(page==Page::HiddenAirshipDestinations && !hiddenAirshipMenuAvailable()) { close(); return; }
    if(learningControllerBinding()) {
        if(vk==VK_ESCAPE || vk==VK_F8) { cancelControllerBinding(); output(L"Controller binding cancelled",true,"mod-menu"); }
        return;
    }
    switch(vk) {
    case VK_F8: close(); break;
    case VK_F6: beaconKey(false); break;
    case VK_F7: detailKey(false); break;
    case VK_ESCAPE: case VK_BACK:
        back(); break;
    case VK_UP: row=(row+count()-1)%count(); announce(); break;
    case VK_DOWN: row=(row+1)%count(); announce(); break;
    case VK_HOME: row=0; announce(); break;
    case VK_END: row=count()-1; announce(); break;
    case VK_LEFT: activate(-1,false); break;
    case VK_RIGHT: activate(1,false); break;
    case VK_RETURN: case VK_SPACE: activate(1,true); break;
    case 'I': case 'N': case 'O': description(); break;
    }
}
void resetModMenu() { resetWakka(); resetSpherePlanner(); if(active) close(); }
void installModMenu() {
    installUtilityText();
    hotkey(VK_F8,menuKey); hotkeyWhen(VK_F9,stepKey,guideKeysAvailable); hotkeyWhen(VK_F10,pathKey,guideKeysAvailable);
    hotkey(VK_F6,beaconKey); hotkey(VK_F7,detailKey);
    hotkeyWhen(VK_F11,readPuzzleKey,readKeyAvailable);
}
}
