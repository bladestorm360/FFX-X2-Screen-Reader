// Windows adapter: one exclusive process-lifetime SDL owner, primitive handoff.
#include "physical_gamepad.hpp"
#include "sdl_runtime.hpp"
#include "nonblocking_snapshots.hpp"
#include "runtime.hpp"
#include "settings.hpp"
#include <algorithm>
#include <atomic>
#include <stdexcept>
namespace ffx::physical {
#ifdef FFX_SDL_OWNER_TEST
bool physicalFocusForTest();
#endif
namespace {
async::AtomicSnapshot<Frame> frames;
async::TryCaptureQueue<Edge,64> edges;
struct Rumble { unsigned motors=0,device=0,focus=0;uint64_t stamp=0; };
async::TryCaptureQueue<Rumble,16> rumble;
std::atomic<HANDLE> beginEvent{nullptr},audioEvent{nullptr};
std::atomic<bool> requested{false},started{false},audioReady{false};
SDL_Gamepad* gamepad=nullptr;
SDL_JoystickID selected=0;
Frame frame;
pad::State raw,last;
pad::Gesture leftGesture,rightGesture;
bool initialized=false,padsReady=false,eventReady=false,devicesDirty=true,priorFocus=false,pressure=false;
uint64_t focusChangedAt=0,deviceBorn=0;
uint64_t now() { return GetTickCount64(); }
bool focused() {
#ifdef FFX_SDL_OWNER_TEST
    return physicalFocusForTest();
#else
    return gameFocused();
#endif
}
void completeAudio(bool available) {
    audioReady.store(available,std::memory_order_release);
    if(const auto ready=audioEvent.load(std::memory_order_acquire)) SetEvent(ready);
}
void initializeOwner() {
    initialized=true;
    if(!sdl::load()) {completeAudio(false);return;}
    sdl::SDL_SetHint(SDL_HINT_JOYSTICK_ALLOW_BACKGROUND_EVENTS,"1");
    eventReady=sdl::SDL_InitSubSystem(SDL_INIT_EVENTS);
    padsReady=eventReady && sdl::SDL_InitSubSystem(SDL_INIT_GAMEPAD);
    if(!padsReady) recordLog("SDL owner gamepad unavailable: %s",sdl::SDL_GetError());
    sdl::SDL_SetHint(SDL_HINT_AUDIO_DEVICE_SAMPLE_FRAMES,"256");
#if defined(FFX_READER_TEST) && !defined(FFX_SDL_OWNER_TEST)
    completeAudio(false); // virtual controller fixture owns no audio device
#else
    completeAudio(sdl::SDL_InitSubSystem(SDL_INIT_AUDIO));
#endif
}
void virtualControls(pad::State& state) {
    state.held&=((uint64_t{1}<<26)-1);
    if(state.lt>75) state.held|=pad::bit(pad::LT);
    if(state.rt>75) state.held|=pad::bit(pad::RT);
    const int left=leftGesture.update(state.lx,state.ly),right=rightGesture.update(state.rx,state.ry);
    if(left>=0) state.held|=pad::bit(pad::LSUp+left);
    if(right>=0) state.held|=pad::bit(pad::RSUp+right);
}
bool same(const pad::State& a,const pad::State& b) {
    return a.held==b.held && a.lx==b.lx && a.ly==b.ly && a.rx==b.rx && a.ry==b.ry && a.lt==b.lt && a.rt==b.rt;
}
void transition(uint64_t stamp,bool focus) {
    virtualControls(raw);
    if(same(raw,last)) return;
    frame.inputStamp=stamp;
    // Keep command/button transitions, never an axis-sample history. A
    // high-report-rate stick can produce many values in one native frame.
    if(focus && raw.held!=last.held && !edges.tryPush({raw,stamp,frame.device,frame.focus,frame.overflow})) {
        ++frame.overflow;pressure=true;
    }
    last=raw;
}
void announceDevice(bool connected) {
    if(!connected) {output(L"Controller disconnected",false,"mod-controller");return;}
    wchar_t name[128]{};
    if(const auto encoded=sdl::SDL_GetGamepadName(gamepad))
        MultiByteToWideChar(CP_UTF8,0,encoded,-1,name,_countof(name));
    output(L"Controller connected"+(name[0]?std::wstring(L": ")+name:std::wstring{})+
        L". View or Select arms one mod command; View then Menu opens settings.",false,"mod-controller");
}
void choose() {
    if(gamepad && !sdl::SDL_GamepadConnected(gamepad)) {
        sdl::SDL_CloseGamepad(gamepad);gamepad=nullptr;selected=0;++frame.device;
        raw=last={};leftGesture={};rightGesture={};devicesDirty=true;announceDevice(false);
    }
    if(gamepad || !devicesDirty) return;
    devicesDirty=false;int count=0;auto ids=sdl::SDL_GetGamepads(&count);
    if(ids && count>0) gamepad=sdl::SDL_OpenGamepad(ids[0]);
    sdl::SDL_free(ids);
    if(gamepad) {selected=sdl::SDL_GetGamepadID(gamepad);++frame.device;deviceBorn=now();raw=last={};leftGesture={};rightGesture={};announceDevice(true);}
}
void sample() {
    raw={};
    for(int i=0;i<26;++i) if(sdl::SDL_GetGamepadButton(gamepad,static_cast<SDL_GamepadButton>(i))) raw.held|=pad::bit(i);
    raw.lx=sdl::SDL_GetGamepadAxis(gamepad,SDL_GAMEPAD_AXIS_LEFTX);
    raw.ly=static_cast<int16_t>(std::clamp(-int(sdl::SDL_GetGamepadAxis(gamepad,SDL_GAMEPAD_AXIS_LEFTY)),-32768,32767));
    raw.rx=sdl::SDL_GetGamepadAxis(gamepad,SDL_GAMEPAD_AXIS_RIGHTX);
    raw.ry=static_cast<int16_t>(std::clamp(-int(sdl::SDL_GetGamepadAxis(gamepad,SDL_GAMEPAD_AXIS_RIGHTY)),-32768,32767));
    raw.lt=static_cast<uint8_t>(std::max(0,int(sdl::SDL_GetGamepadAxis(gamepad,SDL_GAMEPAD_AXIS_LEFT_TRIGGER)))*255/32767);
    raw.rt=static_cast<uint8_t>(std::max(0,int(sdl::SDL_GetGamepadAxis(gamepad,SDL_GAMEPAD_AXIS_RIGHT_TRIGGER)))*255/32767);
}
uint64_t eventTime(uint64_t sdkStamp) {
    const uint64_t clock=sdl::SDL_GetTicksNS(),tick=now();
    if(!sdkStamp || sdkStamp>clock) return tick;
    const uint64_t age=(clock-sdkStamp)/1000000;return age>tick?0:tick-age;
}
void event(const SDL_Event& value,bool focus) {
    if(value.type==SDL_EVENT_GAMEPAD_ADDED || value.type==SDL_EVENT_GAMEPAD_REMOVED || value.type==SDL_EVENT_GAMEPAD_REMAPPED) {devicesDirty=true;return;}
    if(!gamepad) return;
    if((value.type==SDL_EVENT_GAMEPAD_BUTTON_DOWN || value.type==SDL_EVENT_GAMEPAD_BUTTON_UP) && value.gbutton.which==selected) {
        if(value.gbutton.button>=26) return;
        if(value.gbutton.down) raw.held|=pad::bit(value.gbutton.button);else raw.held&=~pad::bit(value.gbutton.button);
    } else if(value.type==SDL_EVENT_GAMEPAD_AXIS_MOTION && value.gaxis.which==selected) {
        const int v=value.gaxis.value;
        switch(value.gaxis.axis) {
        case SDL_GAMEPAD_AXIS_LEFTX:raw.lx=static_cast<int16_t>(v);break;
        case SDL_GAMEPAD_AXIS_LEFTY:raw.ly=static_cast<int16_t>(std::clamp(-v,-32768,32767));break;
        case SDL_GAMEPAD_AXIS_RIGHTX:raw.rx=static_cast<int16_t>(v);break;
        case SDL_GAMEPAD_AXIS_RIGHTY:raw.ry=static_cast<int16_t>(std::clamp(-v,-32768,32767));break;
        case SDL_GAMEPAD_AXIS_LEFT_TRIGGER:raw.lt=static_cast<uint8_t>(std::max(0,v)*255/32767);break;
        case SDL_GAMEPAD_AXIS_RIGHT_TRIGGER:raw.rt=static_cast<uint8_t>(std::max(0,v)*255/32767);break;
        default:return;
        }
    } else return;
    const auto stamp=eventTime(value.common.timestamp);
    transition(stamp,focus && stamp>=focusChangedAt && stamp>=deviceBorn);
}
void step() {
    if(!initialized) initializeOwner();
    const bool focus=focused();
    if(focus!=priorFocus) {++frame.focus;focusChangedAt=now();priorFocus=focus;leftGesture={};rightGesture={};}
    if(eventReady) sdl::SDL_PumpEvents();
    if(padsReady) {
        choose();
        SDL_Event values[16];
        // Bound one owner's batch; more events remain for its next iteration.
        const int count=sdl::SDL_PeepEvents(values,16,SDL_GETEVENT,SDL_EVENT_GAMEPAD_AXIS_MOTION,SDL_EVENT_GAMEPAD_REMAPPED);
        for(int i=0;i<count;++i) event(values[i],focus);
        sdl::SDL_UpdateGamepads();choose();
        if(gamepad) {sample();transition(now(),focus);}
        Rumble request;
        for(unsigned i=0;i<16 && rumble.tryPop(request);++i)
            if(gamepad && focus && request.device==frame.device && request.focus==frame.focus && now()-request.stamp<=250)
                sdl::SDL_RumbleGamepad(gamepad,static_cast<Uint16>(request.motors>>16),static_cast<Uint16>(request.motors),250);
    }
    if(raw.neutral()) pressure=false;
    frame.state=raw;frame.stamp=now();frame.connected=gamepad!=nullptr;frame.focused=focus;frame.pressure=pressure;
    LARGE_INTEGER counter,frequency;QueryPerformanceCounter(&counter);QueryPerformanceFrequency(&frequency);
    frame.counter=counter.QuadPart;frame.frequency=frequency.QuadPart;
    frames.publish(frame);
}
DWORD WINAPI owner(void*) {
    HANDLE timer=nullptr;
    try {
        WaitForSingleObject(beginEvent.load(),INFINITE); // parked SDK owner, never game caller
        initializeOwner();
        // Sleep(4) can round to the system's ~15.6ms timer period. This timer
        // is private to the SDK worker and does not change process/global
        // timer resolution or make a native callback wait.
        timer=CreateWaitableTimerExW(nullptr,nullptr,CREATE_WAITABLE_TIMER_HIGH_RESOLUTION,TIMER_MODIFY_STATE|SYNCHRONIZE);
        if(!timer) throw std::runtime_error("Controller high-resolution timer unavailable");
        for(;;) {
            step();LARGE_INTEGER due;due.QuadPart=-20000; // 2ms, relative
            if(!SetWaitableTimer(timer,&due,0,nullptr,nullptr,FALSE) || WaitForSingleObject(timer,INFINITE)!=WAIT_OBJECT_0)
                throw std::runtime_error("Controller high-resolution timer failed");
        }
    } catch(...) {
        if(timer) CloseHandle(timer);
        frame.connected=0;frame.stamp=now();frames.publish(frame);completeAudio(false);
        recordLog("SDL owner retired after SDK exception; native input retained");return 0;
    }
}
}
void startOwner() {
#if !defined(FFX_READER_TEST) || defined(FFX_SDL_OWNER_TEST)
    if(started.exchange(true)) return;
    HMODULE pin=nullptr;
    if(!GetModuleHandleExW(GET_MODULE_HANDLE_EX_FLAG_FROM_ADDRESS|GET_MODULE_HANDLE_EX_FLAG_PIN,reinterpret_cast<LPCWSTR>(&owner),&pin)) return;
    const auto begin=CreateEventW(nullptr,TRUE,FALSE,nullptr),ready=CreateEventW(nullptr,TRUE,FALSE,nullptr);
    if(!begin || !ready) {if(begin)CloseHandle(begin);if(ready)CloseHandle(ready);return;}
    beginEvent.store(begin,std::memory_order_release);audioEvent.store(ready,std::memory_order_release);
    const auto thread=CreateThread(nullptr,0,owner,nullptr,0,nullptr);
    if(thread) CloseHandle(thread);else completeAudio(false);
#endif
}
void requestReady() {
    if(requested.exchange(true)) return;
    if(const auto event=beginEvent.load(std::memory_order_acquire)) SetEvent(event);
}
bool snapshot(Frame& value) {return frames.tryRead(value);}
bool fresh(uint64_t counter,uint64_t frequency) {
    LARGE_INTEGER current;QueryPerformanceCounter(&current);
    return counter && frequency && static_cast<uint64_t>(current.QuadPart)>=counter &&
        static_cast<uint64_t>(current.QuadPart)-counter<=frequency/100;
}
bool nextEdge(Edge& value) {return edges.tryPop(value);}
bool requestRumble(unsigned motors,const Frame& current) {return rumble.tryPush({motors,current.device,current.focus,now()});}
bool audioAvailable() {return audioReady.load(std::memory_order_acquire);}
void waitForAudio() {if(const auto event=audioEvent.load(std::memory_order_acquire)) WaitForSingleObject(event,INFINITE);}
#if defined(FFX_READER_TEST) && !defined(FFX_SDL_OWNER_TEST)
void manualOwnerStep() {step();}
#endif
#ifdef FFX_SDL_OWNER_TEST
void ownerStepForTest() {step();}
#endif
}
