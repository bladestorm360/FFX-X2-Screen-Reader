#include "settings.hpp"
#include <atomic>
#include <algorithm>
#include <array>
#include "process_storage.hpp"
namespace ffx {
namespace {
constexpr SettingInfo options[] = {
    {L"Speech",L"Read game events with speech and braille. Menu controls remain available when disabled.",L"speech",1,0,1,1},
    {L"Combat detail",L"Normal reads actions, damage and status changes. Detailed adds command descriptions on selection.",L"combat_detail",0,0,1,1},
    {L"Autodetail",L"Read native descriptions on highlight. F7 toggles this setting; I reads the current description manually.",L"auto_detail",0,0,1,1},
    {L"Audio beacon",L"Follow the selected collision route. Stereo indicates left and right; lower pitch indicates behind; faster pings mean nearer.",L"beacon",0,0,1,1},
    {L"Beacon volume",L"Set beacon loudness from 20 to 100 percent.",L"beacon_volume",60,20,100,10},
    {L"Wall tones",L"Hear nearby collision boundaries relative to the camera. Left and right use stereo; ahead and behind use distinct pitches.",L"wall_tones",0,0,1,1},
    {L"Wall volume",L"Set wall tone loudness from 20 to 100 percent.",L"wall_volume",40,20,100,10},
    {L"Wall range",L"Distance in walking steps at which collision boundaries begin sounding.",L"wall_range",2,1,5,1},
    {L"Arrival sound",L"Play a short sound when the final route destination is reached while the audio beacon is on.",L"arrival",1,0,1,1},
    {L"Arrival volume",L"Set arrival sound loudness from 20 to 100 percent.",L"arrival_volume",60,20,100,10},
    {L"Overdrive auto-success",L"Automatic success for Tidus, Auron and Lulu. Wakka's Attack Reels selects all double-hit attack icons; other reels pause for a matching result choice.",L"overdrive_assist",0,0,1,1},
    {L"Voiced dialogue output",L"Suppress, Braille only, or Speak anyway during native voice playback. Unvoiced, paused, muted or failed voice uses normal speech. Braille only stays silent if no braille display is available. T repeats the current dialogue line while dialogue is active.",L"voiced_dialogue",0,0,2,1},
    {L"Hide unreachable destinations",L"Check destinations in the background and omit those proved unreachable. Pending or unresolved collision data keeps destinations listed. U or field right stick click toggles this filter.",L"hide_unreachable",0,0,1,1},
    {L"Race pickup cue",L"Continuous stereo centers the next available Catcher Chocobo balloon. Equal left and right means centered; louder means nearer. F6 toggles this during the race.",L"race_pickup_cue",1,0,1,1},
    {L"Partial steps",L"Read path distances to one decimal place, such as 0.6 south. Off rounds distances to whole numbers, with a minimum of one. Route geometry and audio precision are unchanged.",L"partial_steps",0,0,1,1},
    {L"Auto walk",L"Backslash walks the selected collision route. Your movement cancels immediately. Menus, dialogue, combat, map changes, lost focus and stalled routes stop walking. Interact manually on arrival.",L"auto_walk",0,0,1,1},
    {L"Speak auto walk path",L"Read route directions before walking. Off announces only Auto walking to the selected destination.",L"auto_walk_path",0,0,1,1},
    {L"Menu indexing",L"Announce the selected entry number and total in mod menus and destination lists.",L"menu_index",0,0,1,1},
    {L"Lightning warning",L"Play a short beep as the Thunder Plains warning flash begins, giving the full dodge window. Press dodge once when it sounds. Turn off Auto dodge to practise manually.",L"lightning_warning",1,0,1,1},
    {L"Lightning auto-dodge",L"Automatically dodge ordinary Thunder Plains lightning. Disabled during dialogue, menus and battle.",L"lightning_auto_dodge",0,0,1,1},
    {L"Show maximum HP",L"In battle and field targeting, read current HP followed by the maximum HP number. Off reads current HP and its percentage of maximum.",L"show_maximum_hp",0,0,1,1},
    {L"Butterfly cues",L"In the Makalania hunt, continuous stereo guides toward a blue butterfly. A low pulsing tone warns of a nearby red butterfly; steer away from that tone. Both grow louder as you approach. F6 toggles this during the hunt.",L"butterfly_cues",1,0,1,1},
    {L"Overdrive cues",L"With auto-success off, speaks Auron's button sequence, beeps for Tidus's success zone and Wakka's symbols, and reads Lulu's cast count.",L"overdrive_cues",1,0,1,1},
    {L"Speak maximum MP",L"With H in battle, include maximum MP. Off reads current MP shown on the battle panel.",L"show_maximum_mp",0,0,1,1},
    {L"Polish text",L"Automatically detect the installed Polish translation when its verified area names are ready. Polish forces Polish glyphs.",L"polish_text",0,0,1,1},
    {L"Speech backend",L"Select Automatic or an available Prism backend.",L"speech_backend",0,0,15,1},
    {L"Enable hidden destinations menu for airship map",L"Enable the optional six-destination search picker. Off keeps the menu and its shortcut disabled.",L"hidden_airship_destinations",0,0,1,1},
    {L"Jecht Shot auto-success",L"Complete the original practice and timed memory prompts aboard the S.S. Winno. The game performs the shot and decides its result. Turn off to play manually.",L"jecht_assist",0,0,1,1},
    {L"Jecht Shot cues",L"With Jecht Shot auto-success off, beep and speak the direction plus Confirm for each visible memory. Press the named controls together; center prompts require Confirm alone.",L"jecht_cues",1,0,1,1},
    {L"Gagazet puzzle auto-success",L"Complete Wakka's target throw and the three-swimmer puzzle through the game's original success paths. Turn off to solve manually.",L"gagazet_assist",0,0,1,1},
    {L"Gagazet puzzle cues",L"Speak instructions, selected openings and throw timing for the Gagazet Submerged Passage puzzles.",L"gagazet_cues",1,0,1,1},
    {L"Speak punctuation names",L"Speak names for highlighted punctuation during name or password entry. Off sends the original symbols to the screen reader.",L"speak_punctuation_names",0,0,1,1},
    {L"Sin object collection",L"Enable on any Sin map before entering The Nucleus. The game collects its normal objects and awards rewards. Stays enabled across battles; turn Off to collect manually.",L"sin_collection_assist",0,0,1,1},
    {L"Chocobo training auto-success",L"Enable on Calm Lands South before starting. Automatically finish the course you choose, including a perfect Catcher result. Confirm normal reward dialogue.",L"chocobo_assist",0,0,1,1},
    {L"Remiem race auto-success",L"Enable at Remiem before starting. Finish without poles: earn the first victory and each remaining chest reward in order, then use the normal five-chest repeat result. Confirm normal reward dialogue.",L"remiem_assist",0,0,1,1},
    {L"Butterfly hunt auto-success",L"Enable before starting. Collect seven butterflies in the current course and story phase. The normal timer and chest remain; open the chest and confirm its reward manually.",L"butterfly_assist",0,0,1,1},
    {L"Blitzball auto-win",L"Enable at team setup. Any confirmed live Action Commands option triggers a native win; setup, formation and forfeit choices do not. Native standings and prizes are preserved.",L"blitz_assist",0,0,1,1}
};
static_assert(_countof(options)==static_cast<unsigned>(Setting::Count));
std::atomic<int> values[_countof(options)];
std::atomic<unsigned> revisions[_countof(options)];
const bool defaults=[] { for(unsigned i=0;i<_countof(options);++i) values[i]=options[i].initial; return true; }();
std::atomic<unsigned long> dirtyRevision{0};
std::atomic<HANDLE> settingsWake{nullptr};
ProcessStorage<std::wstring> workerPathStorage;
std::wstring& workerPath() noexcept {return workerPathStorage.get();}
std::atomic<int> writerState{0};
std::atomic<HWND> inputDialog{nullptr};
unsigned completedRevision=0; // persistence owner only
std::atomic<SettingsExtensionWriter> extensions[4];
#ifdef FFX_READER_TEST
SettingsWriteSinkForTest testSink=nullptr;
#endif
bool writeOwned(const int* snapshot) {
#ifdef FFX_READER_TEST
    if(testSink) return testSink(workerPath(),snapshot,_countof(options));
#endif
    bool success=true;
    for(unsigned i=0;i<_countof(options);++i) {
        const auto value=std::to_wstring(snapshot[i]);
        if(!WritePrivateProfileStringW(L"Accessibility",options[i].key,value.c_str(),workerPath().c_str())) {
            success=false;recordLog("settings write failed error=%lu",GetLastError());
        }
    }
    return success;
}
bool drainOwned() {
    const auto captured=dirtyRevision.load();
    bool completed=true;
    if(captured && captured!=completedRevision) {
        std::array<int,_countof(options)> snapshot{};
        for(unsigned i=0;i<snapshot.size();++i) snapshot[i]=values[i].load();
        if(writeOwned(snapshot.data())) completedRevision=captured;
        else completed=false;
    }
    for(auto& extension:extensions) if(const auto write=extension.load(std::memory_order_acquire))
        if(!write(workerPath())) completed=false;
    return completed && dirtyRevision.load()==completedRevision;
}
DWORD WINAPI settingsWorker(void*) {
    DWORD timeout=INFINITE;
    for(;;) {
        WaitForSingleObject(settingsWake.load(),timeout);
        // A failed filesystem operation never becomes a busy retry loop.
        timeout=drainOwned()?INFINITE:1000;
    }
}
}
const SettingInfo& settingInfo(Setting id) { return options[static_cast<unsigned>(id)]; }
int setting(Setting id) { return values[static_cast<unsigned>(id)].load(); }
std::wstring settingsPath() {
    wchar_t path[MAX_PATH]{};
    const auto length=GetModuleFileNameW(nullptr,path,MAX_PATH);
    if(!length || length>=MAX_PATH) return {};
    std::wstring result(path); const auto slash=result.find_last_of(L"\\/");
    return slash==std::wstring::npos?L"":result.substr(0,slash+1)+L"FFXAccessibility.ini";
}
void loadSettings() {
    const auto path=settingsPath(); if(path.empty()) return;
    for(unsigned i=0;i<_countof(options);++i) {
        const auto& option=options[i];
        const int value=GetPrivateProfileIntW(L"Accessibility",option.key,option.initial,path.c_str());
        values[i]=value>=option.minimum && value<=option.maximum?value:option.initial;
    }
}
void saveSettings() {
    ++dirtyRevision;
    wakeSettingsWriter();
}
void wakeSettingsWriter() noexcept {if(const auto wake=settingsWake.load(std::memory_order_acquire)) SetEvent(wake);}
bool registerSettingsWriterExtension(unsigned slot,SettingsExtensionWriter write) {
    if(slot>=_countof(extensions) || !write) return false;
    SettingsExtensionWriter expected=nullptr;
    if(!extensions[slot].compare_exchange_strong(expected,write,std::memory_order_release) && expected!=write) return false;
    wakeSettingsWriter();return true;
}
bool startSettingsWriter(HMODULE module) {
    (void)module;
    int expected=0;
    if(!writerState.compare_exchange_strong(expected,1)) return writerState.load()==2;
    HMODULE pinned=nullptr;
    if(!GetModuleHandleExW(GET_MODULE_HANDLE_EX_FLAG_FROM_ADDRESS|GET_MODULE_HANDLE_EX_FLAG_PIN,
        reinterpret_cast<LPCWSTR>(&startSettingsWriter),&pinned)) {writerState=3;return false;}
    workerPathStorage.prepare();
    workerPath()=settingsPath(); if(workerPath().empty()) {writerState=3;return false;}
    const auto wake=CreateEventW(nullptr,FALSE,FALSE,nullptr); if(!wake) {writerState=3;return false;}
    settingsWake.store(wake);
    const auto thread=CreateThread(nullptr,0,settingsWorker,nullptr,0,nullptr);
    // Once published, the handle remains valid for process life, even if
    // thread creation fails. A producer may already have copied it.
    if(!thread) {writerState=3;return false;}
    writerState=2;
    CloseHandle(thread);
    if(dirtyRevision.load()) SetEvent(wake);
    return true;
}
unsigned settingRevision(Setting id) {return revisions[static_cast<unsigned>(id)].load(std::memory_order_acquire);}
void setSetting(Setting id,int value) {
    const auto& option=settingInfo(id);
    const auto next=std::clamp(value,option.minimum,option.maximum);
    if(values[static_cast<unsigned>(id)].exchange(next)!=next) revisions[static_cast<unsigned>(id)].fetch_add(1,std::memory_order_release);
    saveSettings();
}
void adjustSetting(Setting id,int delta) {
    const auto& option=settingInfo(id);
    if(id==Setting::VoicedDialogue) { setSetting(id,(setting(id)+delta%3+3)%3); return; }
    setSetting(id,option.maximum==1?1-setting(id):setting(id)+delta*option.step);
}
std::wstring settingValue(Setting id) {
    const auto& option=settingInfo(id);
    if(id==Setting::VoicedDialogue) {
        static const wchar_t* names[]={L"Suppress",L"Braille only",L"Speak anyway"};
        return names[setting(id)];
    }
    if(id==Setting::CombatDetail) return setting(id)?L"Detailed":L"Normal";
    if(id==Setting::PolishText) { static const wchar_t* names[]={L"Auto",L"Polish"}; return names[setting(id)]; }
    if(option.maximum==1) return setting(id)?L"On":L"Off";
    return std::to_wstring(setting(id))+(id==Setting::WallRange?L" steps":L" percent");
}
std::wstring menuIndex(unsigned selected,unsigned count) {
    return setting(Setting::MenuIndex)?L", "+std::to_wstring(selected+1)+L" of "+std::to_wstring(count):L"";
}
void setInputDialogWindow(HWND window) {inputDialog.store(window,std::memory_order_release);}
bool inputDialogActive() {
    const auto dialog=inputDialog.load(std::memory_order_acquire);
    return dialog && GetAncestor(GetForegroundWindow(),GA_ROOT)==dialog;
}
bool gameFocused() {
#ifdef FFX_READER_TEST
    extern bool testForeground;
    return testForeground && !inputDialogActive();
#else
    DWORD pid=0; GetWindowThreadProcessId(GetForegroundWindow(),&pid);
    return pid==GetCurrentProcessId() && !inputDialogActive();
#endif
}
#ifdef FFX_READER_TEST
void drainSettingsForTest() {
    // Fixtures without a worker drive the same owned-write routine explicitly.
    if(writerState.load()!=0) return;
    workerPathStorage.prepare();
    if(workerPath().empty()) workerPath()=settingsPath();
    for(unsigned attempt=0;attempt<8 && !drainOwned();++attempt) {}
}
void setSettingsWriteSinkForTest(SettingsWriteSinkForTest sink) { testSink=sink; }
#endif
}
