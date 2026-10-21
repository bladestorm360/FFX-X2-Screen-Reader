#include "../events.hpp"
#include "../controller.hpp"
#include "../physical_gamepad.hpp"
#include "../nav_audio.hpp"
#include "../input_device.hpp"
#include "../settings.hpp"
#include "ready_speech_adapter.hpp"
#include <xinput.h>
#include <atomic>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <map>
#include <chrono>
#include <algorithm>
#include <vector>
namespace ffx {
DWORD nativeThread=0;std::atomic<int> scene{0};
std::atomic<bool> focus{true};
std::atomic<bool> dialogFocus{false};
unsigned originalReads=0,originalMotors=0,actions=0,interrupts=0,menuMaskValue=0;
std::map<unsigned,Handler> callbacks;
bool nativeGameThread() {return GetCurrentThreadId()==nativeThread;}
bool gameFocused() {return focus.load();}
bool inputDialogActive() {return dialogFocus.load();}
std::atomic<unsigned> resourceEpoch{1};
std::atomic<bool> resourcesReady{true};
unsigned generation() {return resourceEpoch.load();}
bool nativeReaderResourcesReady() noexcept {return resourcesReady.load();}
bool output(const std::wstring&,bool,const char*) {return true;}
void log(const char*,...) {}
bool hook(unsigned rva,Handler enter,Handler) {callbacks[rva]=enter;return true;}
bool copy(uintptr_t source,void* destination,size_t size) {std::memcpy(destination,reinterpret_cast<void*>(source),size);return true;}
std::wstring ascii(uintptr_t source,size_t length) {const auto p=reinterpret_cast<const char*>(source);return std::wstring(p,p+strnlen_s(p,length));}
unsigned __cdecl nativeMenuMask() {return menuMaskValue;}
uintptr_t address(unsigned rva) {return rva==0x4aa410?reinterpret_cast<uintptr_t>(&nativeMenuMask):0;}
bool inBattle() {return false;}
bool modMenuActive() {return false;}
bool utilityConsoleActive() {return false;}
bool sphereGridOpen() {return false;}
bool blitzReviewAvailable() {return false;}
bool battleSensorAvailable() {return false;}
bool fieldControlsCapturesKey(unsigned,bool) {return false;}
void initializeControllerBindings() {}
bool captureControllerBinding(uint64_t) {return false;}
void cancelControllerBinding() {}
int controllerBinding(int id) {return id>=0 && id<pad::CountCommands?pad::defaults[id]:-1;}
void modMenuKey(unsigned,bool) {}
void dispatchHotkey(unsigned,bool,bool) {++actions;}
// The real input fixture covers queue phase/retirement; this fixture needs only
// the controller-side guarded queue seam for linkage and routing.
bool queueControllerHotkey(unsigned vk,bool shift,bool control,bool(*guard)()) {
    if(!guard || !guard()) return false;
    dispatchHotkey(vk,shift,control); return true;
}
void interruptSpeechForInput() {++interrupts;}
void pedestalControllerInput(const pad::State&) {}
void autoWalkController(pad::State&) {}
void cancelWallCues() {}
unsigned copiedWallCues() {return 0;}
int setting(Setting id) {return id==Setting::Beacon || id==Setting::Arrival || id==Setting::OverdriveCues?1:0;}
DWORD WINAPI controllerNativeStateForTest(DWORD,XINPUT_STATE* state) {++originalReads;if(state){*state={};state->Gamepad.wButtons=XINPUT_GAMEPAD_B;}return ERROR_SUCCESS;}
DWORD WINAPI controllerNativeRumbleForTest(DWORD,XINPUT_VIBRATION*) {++originalMotors;return ERROR_SUCCESS;}
DWORD controllerStateForTest(DWORD,XINPUT_STATE*);
DWORD controllerRumbleForTest(DWORD,XINPUT_VIBRATION*);
namespace physical {bool physicalFocusForTest() {return ffx::focus.load();}}
}
using namespace ffx;
void check(bool ok,const char* message) {if(!ok){std::fprintf(stderr,"FAIL: %s\n",message);std::exit(1);}}
template<class F> void until(F predicate,const char* why,unsigned limit=3000) {
    const auto end=GetTickCount64()+limit;while(!predicate() && GetTickCount64()<end) Sleep(1);check(predicate(),why);
}
struct SDK {
    unsigned(__cdecl* get)(unsigned)=nullptr;
    void(__cdecl* release)(unsigned)=nullptr;
    void(__cdecl* button)(unsigned,int,uint64_t)=nullptr;
    void(__cdecl* axis)(unsigned,int,uint64_t)=nullptr;
    void(__cdecl* connected)(int)=nullptr;
    void(__cdecl* pause)(int)=nullptr;
    void load() {
        HMODULE module=nullptr;until([&]{module=GetModuleHandleW(L"SDL3.dll");return module!=nullptr;},"fake SDL module loaded only on its owner");
        get=reinterpret_cast<decltype(get)>(GetProcAddress(module,"Test_Get"));release=reinterpret_cast<decltype(release)>(GetProcAddress(module,"Test_Release"));
        button=reinterpret_cast<decltype(button)>(GetProcAddress(module,"Test_Button"));axis=reinterpret_cast<decltype(axis)>(GetProcAddress(module,"Test_Axis"));
        connected=reinterpret_cast<decltype(connected)>(GetProcAddress(module,"Test_Connected"));pause=reinterpret_cast<decltype(pause)>(GetProcAddress(module,"Test_PausePump"));
        check(get && release && button && axis && connected && pause,"complete fake-SDK control ABI");
    }
} sdk;
physical::Frame frame() {physical::Frame value;until([&]{return physical::snapshot(value) && value.stamp;},"owned copied controller frame");return value;}
void settle() {const auto before=sdk.get(4);until([&]{return sdk.get(4)>before+3;},"dedicated SDL owner continues pumping");}
void nativePump() {const auto begin=GetTickCount64();pumpController();check(GetTickCount64()-begin<50,"native controller publication cannot wait for SDK/audio workers");}
XINPUT_STATE state() {XINPUT_STATE value{};check(controllerStateForTest(0,&value)==ERROR_SUCCESS,"native snapshot or original input succeeds");return value;}
void ready() {until([&]{nativePump();return controllerOwned();},"copied physical backend becomes ready");nativePump();}
int main(int argc,char** argv) {
    nativeThread=GetCurrentThreadId();const char* mode=argc>1?argv[1]:"normal";
    SetEnvironmentVariableA("FFX_SDL_TEST_CASE",mode);char tid[32]{};sprintf_s(tid,"%lu",nativeThread);SetEnvironmentVariableA("FFX_SDL_TEST_MAIN",tid);
    if(!std::strcmp(mode,"preloaded")) check(LoadLibraryW(L"SDL3.dll")!=nullptr,"surrogate preloaded host SDK");
    startNavAudio();installController();nativePump();sdk.load();
    if(!std::strcmp(mode,"version") || !std::strcmp(mode,"owner") || !std::strcmp(mode,"preloaded") || !std::strcmp(mode,"pads")) {
        Sleep(40);for(unsigned i=0;i<20;++i)nativePump();const auto prior=originalReads;state();
        check(!controllerOwned() && originalReads==prior+1,"unsupported/preowned/failed SDK retains native input");
        if(!std::strcmp(mode,"pads")) check(sdk.get(4)>0,"gamepad failure retains the successfully initialized event owner");
    } else {
        if(!std::strcmp(mode,"init")) {
            until([&]{return sdk.get(2)!=0;},"fake SDK initialization deliberately held");
            for(unsigned i=0;i<100;++i) {nativePump();state();}
            check(!controllerOwned() && originalReads>=100,"blocked SDK initialization never delays or steals native controls");sdk.release(1);
        }
        ready();
        const auto loadingReads=originalReads;resourcesReady=false;
        check(state().Gamepad.wButtons==XINPUT_GAMEPAD_B && originalReads==loadingReads+1,"loading hands XInput back to the native backend");
        dialogFocus=true;focus=false;
        check(!state().Gamepad.wButtons,"node entry neutralizes XInput fallback when the SDL bridge is unavailable");
        dialogFocus=false;focus=true;
        ++resourceEpoch;resourcesReady=true;const auto retiredReads=originalReads;
        state();check(originalReads==retiredReads+1 && !controllerOwned(),"old resource-generation controller publication is refused");
        nativePump();check(controllerOwned(),"fresh native owner publication restores input after resource retirement");
        if(!std::strcmp(mode,"audio")) {
            until([&]{return sdk.get(3)!=0;},"audio device opening deliberately held");
            const auto pumps=sdk.get(4);for(unsigned i=0;i<100;++i)nativePump();
            settle();nativePump();check(sdk.get(4)>pumps && controllerOwned(),"slow audio device open cannot stall controller owner or game callback");sdk.release(2);
        }
        settle();nativePump();
        sdk.button(pad::South,1,0);sdk.button(pad::South,0,0);settle();nativePump();
        check(state().Gamepad.wButtons&XINPUT_GAMEPAD_A,"complete quick press between native frames is preserved");nativePump();check(!state().Gamepad.wButtons,"quick release follows on the next native frame");
        const auto prior=actions;sdk.axis(5,32767,0);sdk.axis(5,0,0);settle();nativePump();
        check(actions==prior+1 && !state().Gamepad.bRightTrigger,"quick trigger command is captured once without native leakage");nativePump();
        // An older queued pad edge must not override a later keyboard event.
        const auto old=GetTickCount64();sdk.button(pad::North,1,old);sdk.button(pad::North,0,old);Sleep(2);observeInputDevice(false);settle();nativePump();nativePump();
        check(!nativeInputUsesController(),"queued earlier hardware cannot override a later keyboard prompt device");
        focus=false;sdk.button(pad::South,1,0);settle();nativePump();check(!state().Gamepad.wButtons,"background controller buttons are not leaked");
        const auto beforeFocusActions=actions;focus=true;settle();nativePump();check(!state().Gamepad.wButtons && actions==beforeFocusActions,"held buttons drain on refocus");
        sdk.button(pad::South,0,0);settle();nativePump();sdk.button(pad::South,1,0);settle();nativePump();check(state().Gamepad.wButtons&XINPUT_GAMEPAD_A,"fresh focused press restores ordinary input");
        sdk.button(pad::South,0,0);settle();nativePump();
        // DualSense-class axis reports arrive much faster than the native
        // frame rate. Ordinary stick samples must never become a FIFO replay.
        sdk.pause(1);until([&]{return sdk.get(9)!=0;},"owner parked before analog burst");
        for(unsigned i=1;i<=48;++i) sdk.axis(0,static_cast<int>(i*180),0);
        sdk.pause(0);settle();nativePump();
        check(controllerOwned() && state().Gamepad.sThumbLX==8640,
            "one native read gets the newest stick value after a high-rate analog burst");
        sdk.axis(0,0,0);settle();nativePump();
        if(!std::strcmp(mode,"normal")) {
            using Clock=std::chrono::steady_clock;
            std::vector<double> responses;double callbackMax=0,directMax=0;
            for(unsigned i=0;i<96;++i) {
                const int target=i&1?8500:6500;const auto begin=Clock::now();sdk.axis(0,target,0);
                physical::Frame copied;
                while((!physical::snapshot(copied) || copied.state.lx!=target) &&
                    std::chrono::duration<double>(Clock::now()-begin).count()<3) SwitchToThread();
                check(copied.state.lx==target,"high-rate source reaches copied snapshot");
                const auto native=Clock::now();nativePump();const auto value=state();
                callbackMax=std::max(callbackMax,std::chrono::duration<double,std::micro>(Clock::now()-native).count());
                check(value.Gamepad.sThumbLX==target,"same native read supplies current analog controls");
                responses.push_back(std::chrono::duration<double,std::milli>(Clock::now()-begin).count());
                const auto direct=Clock::now();XINPUT_STATE baseline{};controllerNativeStateForTest(0,&baseline);
                directMax=std::max(directMax,std::chrono::duration<double,std::micro>(Clock::now()-direct).count());
            }
            std::sort(responses.begin(),responses.end());
            std::printf("Controller controlled96 source-to-native-read ms p50=%.3f p95=%.3f max=%.3f; native pump+read max-us=%.3f; direct fake-native max-us=%.3f. SDK source/focus/backend synthetic.\n",
                responses[48],responses[91],responses.back(),callbackMax,directMax);
            check(responses.back()<=10 && callbackMax<=10000,"controlled controller pipeline meets requested10ms added-delay bound");
            sdk.axis(0,0,0);settle();nativePump();
        }
        // Deliberate queue pressure preserves original native controls.
        for(unsigned i=0;i<100;++i) {sdk.button(pad::North,1,0);sdk.button(pad::North,0,0);}sdk.button(pad::North,1,0);
        until([&]{return frame().overflow && frame().pressure;},"bounded controller queue saturates explicitly");nativePump();const auto oldOriginal=originalReads;state();
        check(!controllerOwned() && originalReads==oldOriginal+1,"queue pressure hands controls back to the native backend");
        sdk.button(pad::North,0,0);settle();for(unsigned i=0;i<12;++i)nativePump();ready();
        sdk.pause(1);until([&]{return sdk.get(9)!=0;},"SDK pumping deliberately held");Sleep(12);
        const auto staleReads=originalReads;nativePump();state();check(!controllerOwned() && originalReads==staleReads+1,"stale SDK heartbeat restores native input");
        sdk.pause(0);settle();ready();
        sdk.pause(1);until([&]{return sdk.get(9)!=0;},"SDK owner parked for exact rumble ordering");
        XINPUT_VIBRATION vibration{123,456};check(controllerRumbleForTest(0,&vibration)==ERROR_SUCCESS,"native rumble publication is bounded");
        const auto rumble=sdk.get(5);sdk.connected(0);sdk.pause(0);until([&]{return !frame().connected;},"device disconnect copied");nativePump();
        check(!controllerOwned() && sdk.get(5)==rumble,"old-device rumble does not cross reconnect lifetime");sdk.connected(1);settle();ready();
        check(controllerRumbleForTest(0,&vibration)==ERROR_SUCCESS,"fresh device rumble accepted");until([&]{return sdk.get(5)>rumble;},"rumble executed only by SDK owner");
        check(sdk.get(6)==((123u<<16)|456u),"copied motor values preserved");
        until([&]{return sdk.get(7)!=0;},"audio device worker observed");check(sdk.get(7)!=sdk.get(0),"audio device worker is independent of SDL controller owner");
    }
    check(sdk.get(1)==0 && sdk.get(8)==0,"all SDL calls obey exclusive SDK/audio ownership; none runs on native test main");
    std::printf("PASS: fake SDK owner case %s; native handback/nonblocking publication and SDL thread ownership.\n",mode);return 0;
}
