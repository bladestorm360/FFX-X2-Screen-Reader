#include <atomic>
#include "../input_device.hpp"
#include "../events.hpp"
#include "../controller.hpp"
#include "../sdl_runtime.hpp"
#include "../settings.hpp"
#include "ready_speech_adapter.hpp"
#include <xinput.h>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <cstdarg>
#include <map>
namespace ffx {
bool captureControllerBinding(uint64_t);
bool testForeground=true,fixtureMenu=false,fixtureGrid=false;
bool nativeGameThread() {return true;}
bool nativeReaderResourcesReady() noexcept {return true;}
unsigned generation() {return 1;}
unsigned actions=0,menuKeys=0,motors=0,lastVk=0; bool lastShift=false,lastControl=false,blitz=false;
unsigned queuedControllerHotkeys=0,queuedVk=0; bool queuedShift=false,queuedControl=false;
bool fieldReview=false;
bool sensorReview=false;
bool battleSensorAvailable() { return sensorReview; }
pad::State lastPhysical;
unsigned physicalReads=0;
void pedestalControllerInput(const pad::State& state) { lastPhysical=state; ++physicalReads; }
bool fieldControlsCapturesKey(unsigned vk,bool control) { return fieldReview && control && vk>=VK_LEFT && vk<=VK_DOWN; }
unsigned speechInputs=0;
void interruptSpeechForInput() { ++speechInputs; }
std::atomic<int> scene{0};int nativeScene=0; bool battle=false;
uint8_t commandWindow[0xf0]{}; uint8_t commandIndex=0;
std::map<unsigned,Handler> nativeListeners;
bool hook(unsigned rva,Handler enter,Handler) { nativeListeners[rva]=enter; return true; }
unsigned __cdecl menuMask() { return static_cast<unsigned>(nativeScene); }
uintptr_t address(unsigned rva) {
    if(rva==0x4aa410) return reinterpret_cast<uintptr_t>(menuMask);
    if(rva==0x1fcc092) return reinterpret_cast<uintptr_t>(&commandIndex);
    if(rva==0xf3c910) return reinterpret_cast<uintptr_t>(commandWindow);
    return 0;
}
bool inBattle() { return battle; }
bool sphereGridOpen() { return fixtureGrid; }
bool modMenuActive() { return fixtureMenu; }
bool utilityConsoleActive() { return false; }
void modMenuKey(unsigned,bool) { ++menuKeys; }
void dispatchHotkey(unsigned vk,bool shift,bool control) { lastVk=vk; lastShift=shift; lastControl=control; if(vk==VK_F8) fixtureMenu=true; else ++actions; }
// Queue timing is covered by the real input fixture; this fake verifies controller routing/consumption only.
bool queueControllerHotkey(unsigned vk,bool shift,bool control,bool(*guard)()) {
    if(!guard || !guard()) return false;
    ++queuedControllerHotkeys; queuedVk=vk; queuedShift=shift; queuedControl=control;
    dispatchHotkey(vk,shift,control); return true;
}
bool blitzReviewAvailable() { return blitz; }
void autoWalkController(pad::State&) {}
bool output(const std::wstring&,bool,const char*) { return true; }
void log(const char* format,...) { va_list args; va_start(args,format); vfprintf(stderr,format,args); fputc('\n',stderr); va_end(args); }
bool copy(uintptr_t source,void* target,size_t size) {
    __try { memcpy(target,reinterpret_cast<void*>(source),size); return true; }
    __except(1) { return false; }
}
std::wstring ascii(uintptr_t source,size_t length) {
    std::wstring text;
    for(size_t i=0;i<length && read<uint8_t>(source+i);++i) text+=read<uint8_t>(source+i);
    return text;
}
}
using namespace ffx;
void check(bool ok,const char* message) { if(!ok) { fprintf(stderr,"FAIL: %s, SDL=%s\n",message,SDL_GetError()); exit(1); } }
bool SDLCALL rumble(void*,Uint16 left,Uint16 right) { motors=(unsigned(left)<<16)|right; return true; }
int main() {
    SDL_SetHint(SDL_HINT_JOYSTICK_HIDAPI,"0"); SDL_SetHint(SDL_HINT_XINPUT_ENABLED,"0");
    SDL_SetHint(SDL_HINT_JOYSTICK_WGI,"0"); SDL_SetHint(SDL_HINT_JOYSTICK_DIRECTINPUT,"0");
    SDL_SetHint(SDL_HINT_JOYSTICK_GAMEINPUT,"0");
    SDL_SetHint(SDL_HINT_JOYSTICK_GAMEINPUT_RAW,"0");
    check(sdl::load() && SDL_InitSubSystem(SDL_INIT_GAMEPAD),"isolated x86 SDL gamepad initialization");
    SDL_VirtualJoystickDesc descriptor{}; SDL_INIT_INTERFACE(&descriptor);
    descriptor.type=SDL_JOYSTICK_TYPE_GAMEPAD; descriptor.naxes=6; descriptor.nbuttons=26;
    descriptor.button_mask=(1u<<26)-1; descriptor.axis_mask=(1u<<6)-1;
    descriptor.name="FFX isolated controller fixture"; descriptor.Rumble=rumble;
    const auto id=SDL_AttachVirtualJoystick(&descriptor);
    check(id!=0,"virtual gamepad");
    const auto joystick=SDL_OpenJoystick(id); check(joystick!=nullptr,"virtual joystick handle");
    SDL_SetJoystickVirtualAxis(joystick,SDL_GAMEPAD_AXIS_LEFT_TRIGGER,-32768);
    SDL_SetJoystickVirtualAxis(joystick,SDL_GAMEPAD_AXIS_RIGHT_TRIGGER,-32768);
    // Keep both named XInput imports present so production IAT routing is tested.
    XINPUT_STATE game{}; XINPUT_VIBRATION vibration{};
    XInputGetState(0,&game); XInputSetState(0,&vibration);
    // Bootstrap-load the fixture-owned INI before resetting defaults. Other
    // isolated checks can leave a custom layout in this same test directory.
    initializeControllerBindings();resetControllerBindings(); installController(); pumpController(); pumpController();
    check(controllerOwned(),"SDL takes ownership after native imports are bridged");
    check(!nativeInputUsesController(),"neutral connected controller leaves keyboard prompts selected");
    // GetTickCount64 orders devices with keyboard winning equal timestamps.
    // Real virtual transitions must advance that coarse clock, unlike deliberate
    // tie tests in the input-device fixtures.
    auto button=[&](int control,bool down) { SDL_SetJoystickVirtualButton(joystick,control,down); Sleep(20); pumpController(); };
    auto readGame=[&] { check(XInputGetState(0,&game)==ERROR_SUCCESS,"game input supplied through XInput bridge"); return game.Gamepad.wButtons; };
    check(!speechInputs,"neutral controller startup does not silence speech");
    button(pad::South,true); check((readGame()&XINPUT_GAMEPAD_A) && speechInputs==1,"native controller action interrupts and passes through to game");
    check(nativeInputUsesController(),"actual controller press selects controller prompts");
    observeInputDevice(false);
    pumpController(); check(speechInputs==1 && !nativeInputUsesController(),"held controller does not override a later keyboard event");
    button(pad::South,false); button(pad::View,true); check(!readGame(),"modifier swallowed by game bridge");
    check(lastPhysical.held&pad::bit(pad::View),"pedestal cancellation sees physical input on exclusive mod frames");
    button(pad::View,false); button(pad::North,true);
    check(lastPhysical.held&pad::bit(pad::North),"pedestal cancellation precedes consumed button filtering");
    check(actions==1 && lastVk=='M' && !readGame(),"mod Y reads native map and consumes game controls");
    pumpController(); check(actions==1 && !readGame(),"held action is drained");
    button(pad::North,false); button(pad::South,true); check(readGame()&XINPUT_GAMEPAD_A,"native controls resume after release");
    button(pad::South,false); button(pad::View,true); button(pad::View,false); button(pad::Menu,true);
    check(fixtureMenu && !readGame(),"settings chord captures native Start");
    button(pad::Menu,false); button(pad::Up,true); check(menuKeys==1 && !readGame(),"controller navigates mod menu exclusively");
    button(pad::Up,false); fixtureMenu=false; pumpController(); pumpController();
    auto modCommand=[&](int control,unsigned vk) {
        button(pad::View,true); button(pad::View,false); button(control,true);
        check(lastVk==vk && !readGame(),"contextual mod command consumes native game action");
        const auto n=actions; pumpController(); check(actions==n,"held mod command does not repeat");
        button(control,false); pumpController();
    };
    modCommand(pad::West,'G'); modCommand(pad::South,VK_F9); modCommand(pad::East,VK_F10);
    battle=true; const auto beforeStatusSpeech=speechInputs; modCommand(pad::West,'H');
    check(speechInputs>beforeStatusSpeech,"controller status query interrupts before producing character details"); battle=false;
    button(pad::Left,true); check(lastVk==VK_OEM_4 && !lastShift && !readGame(),"field dpad selects previous destination without mod mode");
    button(pad::Left,false); button(pad::Up,true); check(lastVk==VK_OEM_4 && lastShift && !readGame(),"field up selects previous category"); button(pad::Up,false);
    SDL_SetJoystickVirtualAxis(joystick,SDL_GAMEPAD_AXIS_LEFTX,25000); pumpController();
    button(pad::Right,true); readGame(); check(lastVk==VK_OEM_6 && game.Gamepad.sThumbLX>20000 && !game.Gamepad.wButtons,"field navigation preserves analog movement and masks only command");
    button(pad::Right,false); SDL_SetJoystickVirtualAxis(joystick,SDL_GAMEPAD_AXIS_LEFTX,0); pumpController();
    SDL_SetJoystickVirtualAxis(joystick,SDL_GAMEPAD_AXIS_RIGHT_TRIGGER,32767); pumpController(); readGame();
    check(lastVk==VK_OEM_5 && !game.Gamepad.bRightTrigger,"field right trigger reads route without game trigger leakage");
    SDL_SetJoystickVirtualAxis(joystick,SDL_GAMEPAD_AXIS_RIGHT_TRIGGER,-32768); pumpController();
    button(pad::L3,true); check(lastVk==VK_F6 && !readGame(),"field L3 toggles beacon"); button(pad::L3,false);
    button(pad::R3,true); check(lastVk=='U' && !readGame(),"field R3 toggles unreachable filter"); button(pad::R3,false);
    scene=0; nativeScene=4; const auto beforeMenuSpeech=speechInputs;
    button(pad::Left,true); check((readGame()&XINPUT_GAMEPAD_DPAD_LEFT) && speechInputs==beforeMenuSpeech+1,"native menu movement interrupts while retaining game dpad controls"); button(pad::Left,false);
    SDL_SetJoystickVirtualAxis(joystick,SDL_GAMEPAD_AXIS_RIGHTY,-32767); pumpController(); readGame();
    check(lastVk=='I' && !game.Gamepad.sThumbRY,"native menus describe selection with right stick up without mod mode");
    const auto n=actions; pumpController(); check(actions==n,"held description gesture does not repeat");
    SDL_SetJoystickVirtualAxis(joystick,SDL_GAMEPAD_AXIS_RIGHTY,0); pumpController(); scene=4; nativeScene=0;
    const auto beforeFieldDescription=actions;
    SDL_SetJoystickVirtualAxis(joystick,SDL_GAMEPAD_AXIS_RIGHTY,-32767); pumpController(); readGame();
    check(actions==beforeFieldDescription && game.Gamepad.sThumbRY>20000,"field right stick cannot dispatch menu description");
    SDL_SetJoystickVirtualAxis(joystick,SDL_GAMEPAD_AXIS_RIGHTY,0); pumpController();
    uint8_t nativePadState[0x400]; std::memset(nativePadState,0xff,sizeof(nativePadState));
    Event decoded; decoded.ecx=reinterpret_cast<uintptr_t>(nativePadState); decoded.parentRva=0x22db80;

    // Battle right-stick gestures belong to the mod while the native decoder
    // continues to receive neutral right-stick axes for both gamepad parents.
    battle=true;
    const auto beforeBattleAxis=actions;
    SDL_SetJoystickVirtualAxis(joystick,SDL_GAMEPAD_AXIS_RIGHTX,32767); pumpController();
    check(actions==beforeBattleAxis+1 && lastVk=='T' && lastShift,"battle right-stick right dispatches Shift+T once");
    check(!readGame() && game.Gamepad.sThumbRX==0 && game.Gamepad.sThumbRY==0,"battle gesture axes are neutral in XInput bridge");
    auto verifyNativePadNeutral=[&](const char* message) {
        std::memset(nativePadState,0xff,sizeof(nativePadState));
        decoded.ecx=reinterpret_cast<uintptr_t>(nativePadState);
        decoded.parentRva=0x22db80; nativeListeners[0x22d700](decoded);
        check(!read<int>(decoded.ecx+0x374) && !read<int>(decoded.ecx+0x380),message);
        decoded.parentRva=0x22dd50; nativeListeners[0x22d700](decoded);
        check(!read<int>(decoded.ecx+0x374) && !read<int>(decoded.ecx+0x380),message);
        std::memset(nativePadState,0xff,sizeof(nativePadState));
        decoded.parentRva=0x22d770; nativeListeners[0x22d700](decoded);
        check(read<int>(decoded.ecx+0x374)==-1,"shared keyboard dispatcher remains untouched by gamepad filtering");
    };
    verifyNativePadNeutral("battle native DirectInput and XInput decoders neutralize right-stick axes");
    const auto afterRightBattle=actions; pumpController();
    check(actions==afterRightBattle,"held battle right gesture does not repeat");
    SDL_SetJoystickVirtualAxis(joystick,SDL_GAMEPAD_AXIS_RIGHTX,0); pumpController();
    SDL_SetJoystickVirtualAxis(joystick,SDL_GAMEPAD_AXIS_RIGHTX,-32767); pumpController();
    check(actions==afterRightBattle+1 && lastVk=='O' && lastShift,"battle right-stick left dispatches Shift+O once");
    check(!readGame() && game.Gamepad.sThumbRX==0 && game.Gamepad.sThumbRY==0,"left battle gesture axes remain neutral in XInput bridge");
    verifyNativePadNeutral("left battle gesture stays neutral in both native decoders");
    SDL_SetJoystickVirtualAxis(joystick,SDL_GAMEPAD_AXIS_RIGHTX,10000); pumpController();
    readGame(); check(actions==afterRightBattle+1 && !game.Gamepad.sThumbRX,"subthreshold battle residual right axis is suppressed");
    verifyNativePadNeutral("subthreshold battle residual right axis stays neutral in both decoders");
    SDL_SetJoystickVirtualAxis(joystick,SDL_GAMEPAD_AXIS_RIGHTX,0); pumpController();
    const auto beforeVisibleVitals=actions;
    SDL_SetJoystickVirtualAxis(joystick,SDL_GAMEPAD_AXIS_RIGHTY,32767);pumpController();
    check(actions==beforeVisibleVitals+1 && lastVk=='H' && lastShift,"battle right-stick down dispatches Shift+H for visible panels");
    check(!readGame() && !game.Gamepad.sThumbRX && !game.Gamepad.sThumbRY,"battle down gesture is consumed by XInput");
    verifyNativePadNeutral("battle down gesture is consumed by both native gamepad decoders");
    pumpController();check(actions==beforeVisibleVitals+1,"held battle down gesture does not repeat");
    SDL_SetJoystickVirtualAxis(joystick,SDL_GAMEPAD_AXIS_RIGHTY,0);pumpController();
    const auto beforeActiveMeter=actions;
    modCommand(pad::North,'O');
    check(actions==beforeActiveMeter+1 && !lastShift,"battle View+Y/Triangle reads active meter with unshifted O");
    // A remapped active-status command keeps its face button. The meter takes
    // the next free face rather than overriding a saved battle assignment.
    learnControllerBinding(pad::Gil);captureControllerBinding(pad::bit(pad::North));pumpController();pumpController();
    modCommand(pad::North,'H');
    modCommand(pad::South,'O');check(!lastShift,"battle active meter falls back to free South when North is assigned");
    resetControllerBindings();pumpController();pumpController();battle=false;
    readGame(); check(game.Gamepad.sThumbRX==0,"battle context exit leaves a neutral right axis");
    SDL_SetJoystickVirtualAxis(joystick,SDL_GAMEPAD_AXIS_RIGHTX,32767); pumpController(); readGame();
    check(game.Gamepad.sThumbRX>20000,"ordinary field right-stick input resumes after battle context");
    SDL_SetJoystickVirtualAxis(joystick,SDL_GAMEPAD_AXIS_RIGHTX,0); pumpController();

    // Sphere-grid mod commands consume both the SDL bridge and both native
    // decoder routes; trigger VKs are checked directly at the dispatch seam.
    fixtureGrid=true;
    auto enterMod=[&] { button(pad::View,true); button(pad::View,false); };
    auto gridFace=[&](int face,unsigned vk) {
        enterMod(); const auto queuedBefore=queuedControllerHotkeys; button(face,true);
        check(queuedControllerHotkeys==queuedBefore+1 && queuedVk==vk && !queuedShift && !queuedControl,
            "sphere-grid View+face routes exact VK through guarded controller queue");
        check(lastVk==vk && !lastShift && !readGame(),"sphere-grid View+face dispatches its F-key and consumes XInput");
        check(!game.Gamepad.sThumbRX && !game.Gamepad.sThumbRY && !game.Gamepad.bLeftTrigger && !game.Gamepad.bRightTrigger,
            "sphere-grid face command leaves reserved axes and triggers neutral");
        std::memset(nativePadState,0xff,sizeof(nativePadState)); decoded.ecx=reinterpret_cast<uintptr_t>(nativePadState);
        decoded.parentRva=0x22db80; nativeListeners[0x22d700](decoded);
        check(!read<int>(decoded.ecx+0x374) && !read<int>(decoded.ecx+0x380) && !read<uint8_t>(decoded.ecx+0x39e) && !read<uint8_t>(decoded.ecx+0x39f),
            "sphere-grid face command is consumed by native DirectInput decoder");
        decoded.parentRva=0x22dd50; nativeListeners[0x22d700](decoded);
        check(!read<int>(decoded.ecx+0x374) && !read<int>(decoded.ecx+0x380) && !read<uint8_t>(decoded.ecx+0x39e) && !read<uint8_t>(decoded.ecx+0x39f),
            "sphere-grid face command is consumed by native XInput decoder");
        const auto issued=actions; pumpController(); check(actions==issued && queuedControllerHotkeys==queuedBefore+1,"held sphere-grid face command does not repeat");
        button(face,false); pumpController();
    };
    gridFace(pad::West,VK_F2); gridFace(pad::North,VK_F3); gridFace(pad::South,VK_F4); gridFace(pad::East,VK_F5);
    auto gridTrigger=[&](int axis,unsigned expectedVk,bool shift,bool withView) {
        if(withView) enterMod();
        const auto queuedBefore=queuedControllerHotkeys; SDL_SetJoystickVirtualAxis(joystick,axis,32767); pumpController();
        check(queuedControllerHotkeys==queuedBefore+1 && queuedVk==expectedVk && queuedShift==shift && !queuedControl,
            "sphere-grid trigger routes exact VK and modifiers through guarded controller queue");
        check(lastVk==expectedVk && lastShift==shift,"sphere-grid direct trigger dispatches exact VK and shift state");
        check(!readGame() && !game.Gamepad.bLeftTrigger && !game.Gamepad.bRightTrigger,"sphere-grid trigger is neutral in XInput while held");
        std::memset(nativePadState,0xff,sizeof(nativePadState)); decoded.ecx=reinterpret_cast<uintptr_t>(nativePadState);
        decoded.parentRva=0x22db80; nativeListeners[0x22d700](decoded);
        check(!read<uint8_t>(decoded.ecx+0x39e) && !read<uint8_t>(decoded.ecx+0x39f),"sphere-grid trigger is neutral in native decoder while held");
        decoded.parentRva=0x22dd50; nativeListeners[0x22d700](decoded);
        check(!read<uint8_t>(decoded.ecx+0x39e) && !read<uint8_t>(decoded.ecx+0x39f),"sphere-grid trigger is neutral in second native decoder while held");
        std::memset(nativePadState,0xff,sizeof(nativePadState));
        decoded.ecx=reinterpret_cast<uintptr_t>(nativePadState);
        decoded.parentRva=0x22d770; nativeListeners[0x22d700](decoded);
        check(read<uint8_t>(decoded.ecx+0x39e)==0xff && read<uint8_t>(decoded.ecx+0x39f)==0xff,
            "shared keyboard dispatcher remains untouched by grid trigger filtering");
        const auto issued=actions; pumpController(); check(actions==issued && queuedControllerHotkeys==queuedBefore+1,"held sphere-grid trigger does not repeat");
        SDL_SetJoystickVirtualAxis(joystick,axis,-32768); pumpController();
        SDL_SetJoystickVirtualAxis(joystick,axis,3000); pumpController(); readGame();
        check(!game.Gamepad.bLeftTrigger && !game.Gamepad.bRightTrigger,"subthreshold residual sphere-grid trigger is suppressed");
        SDL_SetJoystickVirtualAxis(joystick,axis,-32768); pumpController();
    };
    gridTrigger(SDL_GAMEPAD_AXIS_LEFT_TRIGGER,VK_OEM_5,true,false);
    gridTrigger(SDL_GAMEPAD_AXIS_LEFT_TRIGGER,VK_OEM_5,true,true);
    gridTrigger(SDL_GAMEPAD_AXIS_RIGHT_TRIGGER,VK_OEM_5,false,false);
    gridTrigger(SDL_GAMEPAD_AXIS_RIGHT_TRIGGER,VK_OEM_5,false,true);
    fixtureGrid=false; pumpController();
    const auto beforeGridExitTrigger=actions;
    SDL_SetJoystickVirtualAxis(joystick,SDL_GAMEPAD_AXIS_RIGHT_TRIGGER,32767); pumpController(); readGame();
    check(actions==beforeGridExitTrigger+1 && lastVk==VK_OEM_5 && !game.Gamepad.bRightTrigger,
        "after sphere-grid exit the ordinary field route binding resumes");
    SDL_SetJoystickVirtualAxis(joystick,SDL_GAMEPAD_AXIS_RIGHT_TRIGGER,-32768); pumpController();
    const auto beforeBattleDescription=actions;
    battle=true; SDL_SetJoystickVirtualAxis(joystick,SDL_GAMEPAD_AXIS_RIGHTY,-32767); pumpController();
    check(actions==beforeBattleDescription,"battle animation without command menu cannot read stale description");
    SDL_SetJoystickVirtualAxis(joystick,SDL_GAMEPAD_AXIS_RIGHTY,0); pumpController();
    commandWindow[1]=4; write<unsigned>(reinterpret_cast<uintptr_t>(commandWindow)+0x44,1);
    SDL_SetJoystickVirtualAxis(joystick,SDL_GAMEPAD_AXIS_RIGHTY,-32767); pumpController(); readGame();
    check(actions==beforeBattleDescription+1 && lastVk=='I' && !game.Gamepad.sThumbRY,"active battle command menu permits description");
    SDL_SetJoystickVirtualAxis(joystick,SDL_GAMEPAD_AXIS_RIGHTY,0); pumpController(); battle=false;
    button(pad::Left,true); SDL_SetJoystickVirtualAxis(joystick,SDL_GAMEPAD_AXIS_RIGHT_TRIGGER,32767); pumpController();
    std::memset(nativePadState,0xff,sizeof(nativePadState));
    decoded.parentRva=0x22db80;
    nativeListeners[0x22d700](decoded);
    check(!nativePadState[0x3a7] && !nativePadState[0x39f],"DirectInput native decoder cannot leak reserved dpad or trigger");
    decoded.parentRva=0x22dd50; nativeListeners[0x22d700](decoded);
    check(!nativePadState[0x3a7] && !nativePadState[0x39f],"XInput native decoder gets the same filtered controls");
    decoded.parentRva=0x22d770; nativePadState[0x3a7]=0x7f; nativeListeners[0x22d700](decoded);
    check(nativePadState[0x3a7]==0x7f,"shared native keyboard dispatcher is not written as a gamepad");
    button(pad::Left,false); SDL_SetJoystickVirtualAxis(joystick,SDL_GAMEPAD_AXIS_RIGHT_TRIGGER,-32768); pumpController();
    vibration.wLeftMotorSpeed=123; vibration.wRightMotorSpeed=456;
    check(XInputSetState(0,&vibration)==ERROR_SUCCESS,"game vibration accepted"); pumpController();
    check(motors==((123u<<16)|456),"game vibration forwarded to SDL device");
    const auto beforeForeignSpeech=speechInputs;
    testForeground=false; button(pad::South,true); check(!readGame() && speechInputs==beforeForeignSpeech,"foreign focus suppresses controls and speech interruption");
    testForeground=true; pumpController(); check(!readGame() && speechInputs==beforeForeignSpeech,"held input suppressed on refocus without silencing speech");
    button(pad::South,false); button(pad::South,true); check(readGame()&XINPUT_GAMEPAD_A,"new focused input resumes");
    button(pad::South,false); blitz=true;
    button(pad::View,true); button(pad::View,false); button(pad::Down,true);
    check(lastVk==VK_DOWN && lastControl && !readGame(),"View then Down reads Blitzball buffer without moving game selection");
    button(pad::Down,false); button(pad::View,true); button(pad::View,false);
    SDL_SetJoystickVirtualAxis(joystick,SDL_GAMEPAD_AXIS_RIGHT_TRIGGER,32767); pumpController();
    check(lastVk==VK_OEM_5 && !lastControl && !readGame(),"View then right trigger queries the assigned Blitzball position");
    SDL_SetJoystickVirtualAxis(joystick,SDL_GAMEPAD_AXIS_RIGHT_TRIGGER,-32768); pumpController(); blitz=false;
    fieldReview=true; button(pad::View,true); button(pad::View,false); button(pad::Down,true);
    check(lastVk==VK_DOWN && lastControl && !readGame(),"View then Down reviews native swimming controls without native movement");
    button(pad::Down,false); fieldReview=false;
    sensorReview=true; button(pad::View,true); button(pad::View,false); button(pad::Right,true);
    check(lastVk==VK_RIGHT && lastControl && !readGame(),"View then Right reviews Sensor information without moving the target");
    button(pad::Right,false); sensorReview=false;
    SDL_DetachVirtualJoystick(id); pumpController(); check(!controllerOwned(),"disconnect releases ownership");
    SDL_CloseJoystick(joystick);
    const auto replacement=SDL_AttachVirtualJoystick(&descriptor);
    check(replacement!=0,"reconnected virtual gamepad");
    const auto second=SDL_OpenJoystick(replacement);
    SDL_SetJoystickVirtualAxis(second,SDL_GAMEPAD_AXIS_LEFT_TRIGGER,-32768);
    SDL_SetJoystickVirtualAxis(second,SDL_GAMEPAD_AXIS_RIGHT_TRIGGER,-32768);
    pumpController(); pumpController(); check(controllerOwned(),"SDL reconnect restores ownership in input loop");
    SDL_SetJoystickVirtualButton(second,pad::South,true); pumpController();
    check(readGame()&XINPUT_GAMEPAD_A,"reconnected controller supplies native game input");
    SDL_DetachVirtualJoystick(replacement); pumpController(); SDL_CloseJoystick(second);
    puts("PASS: real x86 SDL virtual gamepad, native XInput import bridge, mod/menu ownership, Select, rumble, focus and disconnect.");
}
