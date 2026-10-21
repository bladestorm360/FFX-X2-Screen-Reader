#include "../settings.hpp"
#include "../controller.hpp"
#include "ready_speech_adapter.hpp"
#include <atomic>
#include <cstdio>
#include <cstdlib>
namespace ffx {
void log(const char*,...) {}
bool testForeground=true;
bool output(const std::wstring&,bool,const char*) {return true;}
void drainController() {}
bool captureControllerBinding(uint64_t);
std::atomic<bool> entered{false},releaseWrite{false},finished{false};
HANDLE enteredEvent=nullptr;
int finalValues[static_cast<unsigned>(Setting::Count)]{};
std::atomic<unsigned> bindingWrites{0};
std::atomic<bool> bindingDone{false};
DWORD nativeOwner=0;
bool bindingSink(const std::wstring&,const int* values,size_t count) {
    if(GetCurrentThreadId()==nativeOwner || count!=pad::CountCommands) return false;
    // First write fails; the existing settings worker retries without native waiting.
    if(++bindingWrites==1) return false;
    bindingDone=values[pad::Description]==pad::LB;return true;
}
bool sink(const std::wstring&,const int* values,size_t count) {
    if(!entered.exchange(true)) SetEvent(enteredEvent);
    while(!releaseWrite.load()) Sleep(1);
    for(size_t i=0;i<count;++i) finalValues[i]=values[i];
    if(finalValues[static_cast<unsigned>(Setting::Beacon)]==0 &&
       finalValues[static_cast<unsigned>(Setting::BeaconVolume)]==100) finished=true;
    return true;
}
}
int main() {
    using namespace ffx;
    enteredEvent=CreateEventW(nullptr,TRUE,FALSE,nullptr);
    if(!enteredEvent) return 1;
    setSettingsWriteSinkForTest(sink);
    nativeOwner=GetCurrentThreadId();initializeControllerBindings();setControllerWriteSinkForTest(bindingSink);
    if(!startSettingsWriter(GetModuleHandleW(nullptr))) return 2;
    setSetting(Setting::Beacon,1);
    if(WaitForSingleObject(enteredEvent,5000)!=WAIT_OBJECT_0) return 3;
    const auto started=GetTickCount64();
    setSetting(Setting::Beacon,0); setSetting(Setting::BeaconVolume,1000);
    resetControllerBindings();learnControllerBinding(pad::Description);
    if(!captureControllerBinding(pad::bit(pad::LB)) || controllerBinding(pad::Description)!=pad::LB) return 9;
    if(GetTickCount64()-started>250) return 4;
    if(setting(Setting::AutoDetail)!=0 || setting(Setting::HiddenAirshipDestinations)!=0) return 5;
    if(setting(Setting::BeaconVolume)!=100) return 6;
    if(setting(Setting::SpeakPunctuationNames)!=0) return 11;
    const auto& punctuation=settingInfo(Setting::SpeakPunctuationNames);
    if(std::wstring(punctuation.key)!=L"speak_punctuation_names" ||
       std::wstring(punctuation.name)!=L"Speak punctuation names") return 12;
    releaseWrite=true;
    const auto until=GetTickCount64()+5000;
    while((!finished.load() || !bindingDone.load()) && GetTickCount64()<until) Sleep(1);
    if(!bindingDone || bindingWrites<2) return 10;
    if(!finished || finalValues[static_cast<unsigned>(Setting::Beacon)]!=0 ||
       finalValues[static_cast<unsigned>(Setting::BeaconVolume)]!=100) return 7;
    const auto& hidden=settingInfo(Setting::HiddenAirshipDestinations);
    if(std::wstring(hidden.key)!=L"hidden_airship_destinations" ||
       std::wstring(hidden.name)!=L"Enable hidden destinations menu for airship map") return 8;
    CloseHandle(enteredEvent);
    puts("PASS: held settings disk work cannot delay native settings/remap updates; owned controller saves retry on the worker.");return 0;
}
