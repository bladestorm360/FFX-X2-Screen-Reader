#pragma once
#include "runtime.hpp"
namespace ffx {
enum class Setting : unsigned {
    Speech, CombatDetail, AutoDetail, Beacon, BeaconVolume, Walls, WallVolume,
    WallRange, Arrival, ArrivalVolume, OverdriveAssist, VoicedDialogue,HideUnreachable, RaceCues,PartialSteps,AutoWalk,AutoWalkPath,MenuIndex,
    LightningWarning,LightningAutoDodge,ShowMaximumHp,ButterflyCues,OverdriveCues,ShowMaximumMp,PolishText,SpeechBackend,HiddenAirshipDestinations,JechtAssist,JechtCues,GagazetAssist,GagazetCues,SpeakPunctuationNames,SinCollectionAssist,
    ChocoboAssist,RemiemAssist,ButterflyAssist,BlitzAssist,Count
};
struct SettingInfo {
    const wchar_t* name;
    const wchar_t* description;
    const wchar_t* key;
    int initial, minimum, maximum, step;
};
const SettingInfo& settingInfo(Setting);
int setting(Setting);
// Changes invalidate queued native actions even across an Off/On round trip.
unsigned settingRevision(Setting);
void setSetting(Setting, int);
void adjustSetting(Setting, int);
std::wstring settingValue(Setting);
void loadSettings();
void saveSettings();
bool startSettingsWriter(HMODULE module);
using SettingsExtensionWriter=bool(*)(const std::wstring&);
bool registerSettingsWriterExtension(unsigned slot,SettingsExtensionWriter);
void wakeSettingsWriter() noexcept;
#ifdef FFX_READER_TEST
void drainSettingsForTest();
using SettingsWriteSinkForTest=bool(*)(const std::wstring&,const int*,size_t);
void setSettingsWriteSinkForTest(SettingsWriteSinkForTest);
#endif
std::wstring settingsPath();
std::wstring menuIndex(unsigned selected,unsigned count);
bool gameFocused();
void setInputDialogWindow(HWND); // UI owner publishes its process-lifetime window.
bool inputDialogActive(); // Win32 focus only; safe on SDK/native/UI owners.
}
