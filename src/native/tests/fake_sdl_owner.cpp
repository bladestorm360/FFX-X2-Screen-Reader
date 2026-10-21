#define SDL_BUILDING_LIBRARY
#define SDL_MAIN_HANDLED
#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#include <SDL3/SDL_main.h>
#include <SDL3/SDL_init.h>
#include <SDL3/SDL_events.h>
#include <SDL3/SDL_gamepad.h>
#include <SDL3/SDL_audio.h>
#include <SDL3/SDL_hints.h>
#include <SDL3/SDL_timer.h>
#include <SDL3/SDL_version.h>
#include <SDL3/SDL_stdinc.h>
#include <atomic>
#include <array>
#include <condition_variable>
#include <cstdint>
#include <cstdlib>
#include <cstring>
#include <deque>
#include <mutex>

namespace {
struct GamepadRecord { SDL_JoystickID id; };
// Immutable records are initialized before either fixture worker starts.
const std::array<GamepadRecord,64> pads=[] {
    std::array<GamepadRecord,64> value{};
    for(unsigned i=0;i<value.size();++i) value[i].id=i;
    return value;
}();
std::atomic<bool> connected{true};
std::atomic<SDL_JoystickID> currentID{1};
std::atomic<uint32_t> buttons{0};
std::atomic<int16_t> axes[6]{};
std::atomic<uint32_t> initMask{0};
std::atomic<DWORD> owner{0}, audioOpenThread{0};
std::atomic<unsigned> violations{0}, pumps{0}, rumbleCount{0}, motors{0};
std::atomic<bool> mainReady{false}, initEntered{false}, audioEntered{false}, pumpPaused{false}, pumpEntered{false};
std::atomic<unsigned> callsOnMain{0};
std::mutex eventMutex, waitMutex;
std::condition_variable changed;
std::deque<SDL_Event> queue;
std::atomic<bool> releaseInit{false}, releaseAudio{false};
alignas(16) unsigned char audioTokenBytes[64]{};
SDL_AudioStream* audioToken = reinterpret_cast<SDL_AudioStream*>(audioTokenBytes);
const char stableError[] = "fixture SDL error";
const char* testCase() { static const auto value=[] {std::array<char,32> v{};GetEnvironmentVariableA("FFX_SDL_TEST_CASE",v.data(),static_cast<DWORD>(v.size()));return v;}();return value.data(); }
DWORD recordCall(bool affinity=true) {
    DWORD tid=GetCurrentThreadId();
    char mainText[32]{};GetEnvironmentVariableA("FFX_SDL_TEST_MAIN",mainText,32);
    if(std::strtoul(mainText,nullptr,10)==tid) callsOnMain.fetch_add(1);
    if(affinity) { DWORD expected=owner.load(); if(!expected) { DWORD zero=0; owner.compare_exchange_strong(zero,tid); expected=owner.load(); }
        if(expected!=tid) violations.fetch_add(1); }
    return tid;
}
void stall(std::atomic<bool>& entered,std::atomic<bool>& release) {
    entered.store(true); std::unique_lock<std::mutex> lock(waitMutex);
    changed.wait(lock,[&]{return release.load();});
}
void push(SDL_Event e) { std::lock_guard<std::mutex> lock(eventMutex); queue.push_back(e); }
}
extern "C" {
SDL_DECLSPEC void SDLCALL SDL_SetMainReady(void) { recordCall(); mainReady.store(true); }
SDL_DECLSPEC bool SDLCALL SDL_IsMainThread(void) { DWORD t=recordCall(false); if(!mainReady.load()) return std::strcmp(testCase(),"owner")!=0; return t==owner.load(); }
SDL_DECLSPEC SDL_InitFlags SDLCALL SDL_WasInit(SDL_InitFlags flags) { recordCall(); uint32_t v=initMask.load(); return flags ? (v & flags) : v; }
SDL_DECLSPEC int SDLCALL SDL_GetVersion(void) { recordCall(false); return std::strcmp(testCase(),"version")==0 ? 3002000 : 3004004; }
SDL_DECLSPEC const char* SDLCALL SDL_GetError(void) { recordCall(false); return stableError; }
SDL_DECLSPEC bool SDLCALL SDL_SetHint(const char*,const char*) { recordCall(); return true; }
SDL_DECLSPEC Uint64 SDLCALL SDL_GetTicksNS(void) { recordCall(false); return GetTickCount64()*1000000ULL; }
SDL_DECLSPEC bool SDLCALL SDL_InitSubSystem(SDL_InitFlags flags) { recordCall(); if(std::strcmp(testCase(),"init")==0 && !initEntered.exchange(true)) stall(initEntered,releaseInit);
    if(std::strcmp(testCase(),"pads")==0 && (flags & SDL_INIT_GAMEPAD)) return false; initMask.fetch_or(flags); return true; }
SDL_DECLSPEC void SDLCALL SDL_PumpEvents(void) { recordCall(); pumps.fetch_add(1); if(pumpPaused.load()) { pumpEntered.store(true); while(pumpPaused.load()) Sleep(1); } }
SDL_DECLSPEC int SDLCALL SDL_PeepEvents(SDL_Event* out,int n,SDL_EventAction action,Uint32 lo,Uint32 hi) { recordCall(); if(action!=SDL_GETEVENT||!out||n<=0)return 0; std::lock_guard<std::mutex> lock(eventMutex); int count=0;
    for(auto it=queue.begin();it!=queue.end()&&count<n;) { if((Uint32)it->type>=lo&&(Uint32)it->type<=hi) { out[count++]=*it; it=queue.erase(it); } else ++it; } return count; }
SDL_DECLSPEC void SDLCALL SDL_UpdateGamepads(void) { recordCall(); }
SDL_DECLSPEC SDL_JoystickID* SDLCALL SDL_GetGamepads(int* count) { recordCall(); bool on=connected.load(); int n=on?1:0; if(count)*count=n; if(!n)return nullptr; auto* ids=(SDL_JoystickID*)std::malloc(sizeof(SDL_JoystickID)); if(ids)ids[0]=currentID.load(); else if(count)*count=0; return ids; }
SDL_DECLSPEC void SDLCALL SDL_free(void* p) { recordCall(false); std::free(p); }
SDL_DECLSPEC SDL_Gamepad* SDLCALL SDL_OpenGamepad(SDL_JoystickID id) { recordCall(); if(!connected.load()||id!=currentID.load()||id>=64)return nullptr; return reinterpret_cast<SDL_Gamepad*>(const_cast<GamepadRecord*>(&pads[id])); }
SDL_DECLSPEC void SDLCALL SDL_CloseGamepad(SDL_Gamepad*) { recordCall(); }
SDL_DECLSPEC bool SDLCALL SDL_GamepadConnected(SDL_Gamepad* p) { recordCall(); auto* r=reinterpret_cast<GamepadRecord*>(p); return p&&connected.load()&&r->id==currentID.load(); }
SDL_DECLSPEC SDL_JoystickID SDLCALL SDL_GetGamepadID(SDL_Gamepad* p) { recordCall(); return p?reinterpret_cast<GamepadRecord*>(p)->id:0; }
SDL_DECLSPEC const char* SDLCALL SDL_GetGamepadName(SDL_Gamepad*) { recordCall(); return "Fixture controller"; }
SDL_DECLSPEC bool SDLCALL SDL_GetGamepadButton(SDL_Gamepad* p,SDL_GamepadButton b) { recordCall(); return p&&b>=0&&b<26&&(buttons.load()&(1u<<(unsigned)b)); }
SDL_DECLSPEC Sint16 SDLCALL SDL_GetGamepadAxis(SDL_Gamepad* p,SDL_GamepadAxis a) { recordCall(); return p&&a>=0&&a<6?axes[(int)a].load():0; }
SDL_DECLSPEC bool SDLCALL SDL_RumbleGamepad(SDL_Gamepad* p,Uint16 low,Uint16 high,Uint32) { recordCall(); if(!p)return false; motors.store(((uint32_t)low<<16)|high);rumbleCount.fetch_add(1);return true; }
SDL_DECLSPEC SDL_AudioStream* SDLCALL SDL_OpenAudioDeviceStream(SDL_AudioDeviceID,const SDL_AudioSpec*,SDL_AudioStreamCallback,void*) { DWORD t=recordCall(false); audioOpenThread.store(t); if(std::strcmp(testCase(),"audio")==0&&!audioEntered.exchange(true)) stall(audioEntered,releaseAudio); return audioToken; }
SDL_DECLSPEC bool SDLCALL SDL_ResumeAudioStreamDevice(SDL_AudioStream* p) { recordCall(false); return p==audioToken; }
SDL_DECLSPEC void SDLCALL SDL_DestroyAudioStream(SDL_AudioStream*) { recordCall(false); }
SDL_DECLSPEC bool SDLCALL SDL_PutAudioStreamData(SDL_AudioStream* p,const void*,int) { recordCall(false); return p==audioToken; }
SDL_DECLSPEC bool SDLCALL SDL_ClearAudioStream(SDL_AudioStream* p) { recordCall(false); return p==audioToken; }

__declspec(dllexport) void __cdecl Test_Connected(int state) { std::lock_guard<std::mutex> lock(eventMutex); if(state==0&&connected.exchange(false)) { auto id=currentID.load(); buttons.store(0); for(auto& a:axes)a.store(0); SDL_Event e{};e.gdevice.type=SDL_EVENT_GAMEPAD_REMOVED;e.gdevice.timestamp=GetTickCount64()*1000000ULL;e.gdevice.which=id;queue.push_back(e); }
    else if(state==1&&!connected.load()) { auto id=currentID.load()+1; if(id<64) { currentID.store(id);buttons.store(0);for(auto& a:axes)a.store(0);connected.store(true);SDL_Event e{};e.gdevice.type=SDL_EVENT_GAMEPAD_ADDED;e.gdevice.timestamp=GetTickCount64()*1000000ULL;e.gdevice.which=id;queue.push_back(e); } } }
__declspec(dllexport) void __cdecl Test_Button(unsigned b,int down,uint64_t stampMs) { if(b>=26)return; uint32_t mask=1u<<b; if(down)buttons.fetch_or(mask);else buttons.fetch_and(~mask); SDL_Event e{};e.gbutton.type=down?SDL_EVENT_GAMEPAD_BUTTON_DOWN:SDL_EVENT_GAMEPAD_BUTTON_UP;e.gbutton.timestamp=(stampMs?stampMs:GetTickCount64())*1000000ULL;e.gbutton.which=currentID.load();e.gbutton.button=(Uint8)b;e.gbutton.down=down!=0;push(e); }
__declspec(dllexport) void __cdecl Test_Axis(unsigned a,int value,uint64_t stampMs) { if(a>=6)return;axes[a].store((int16_t)value);SDL_Event e{};e.gaxis.type=SDL_EVENT_GAMEPAD_AXIS_MOTION;e.gaxis.timestamp=(stampMs?stampMs:GetTickCount64())*1000000ULL;e.gaxis.which=currentID.load();e.gaxis.axis=(Uint8)a;e.gaxis.value=(Sint16)value;push(e); }
__declspec(dllexport) unsigned __cdecl Test_Get(unsigned i) { switch(i){case 0:return owner.load();case 1:return violations.load();case 2:return initEntered.load();case 3:return audioEntered.load();case 4:return pumps.load();case 5:return rumbleCount.load();case 6:return motors.load();case 7:return audioOpenThread.load();case 8:return callsOnMain.load();case 9:return pumpEntered.load();default:return 0;} }
__declspec(dllexport) void __cdecl Test_PausePump(int pause) { if(pause) pumpEntered.store(false); pumpPaused.store(pause!=0); }
__declspec(dllexport) void __cdecl Test_Release(unsigned mask) { if(mask&1)releaseInit.store(true);if(mask&2)releaseAudio.store(true);changed.notify_all(); }
}
