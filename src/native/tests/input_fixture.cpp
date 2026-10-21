#include <atomic>
#include "../input_device.hpp"
#include "../events.hpp"
#include "../controller.hpp"
#define DIRECTINPUT_VERSION 0x0800
#include <dinput.h>
#include <map>
#include <vector>
#include <cstdio>
#include <cstdlib>
namespace ffx {
void directInputStateForTest(Event&,bool);
void directInputDataForTest(Event&,bool);
void directInputReleaseForTest(Event&);
bool testForeground=true;
bool testInputDialog=false;
bool inputDialogActive() {return testInputDialog;}
bool testEsc=false;
bool escMenuActive() {return testEsc;}
bool gameFocused() { return testForeground; }
std::atomic<int> scene{0};
bool inBattle() { return false; }
void pauseNavAudio() {}
void updateAutoWalk() {}
void updatePedestalInput() {}
void autoWalkPlayerInput(unsigned) {}
void autoWalkKeyboard(uint8_t*) {}
bool blitzCapturesKey(unsigned vk,bool control) { return control && vk==VK_DOWN; }
bool fieldControlsCapturesKey(unsigned vk,bool control) { return control && vk==VK_UP; }
unsigned fieldInputs=0;
bool dispatchFieldControlsKey(unsigned vk,bool control) { if(!fieldControlsCapturesKey(vk,control)) return false; ++fieldInputs; return true; }
bool sensorReview=false; unsigned sensorInputs=0;
bool battleSensorCapturesKey(unsigned vk,bool control) { return sensorReview && control && vk==VK_RIGHT; }
bool dispatchBattleSensorKey(unsigned vk,bool control) { if(!battleSensorCapturesKey(vk,control)) return false; ++sensorInputs; return true; }
unsigned blitzInputs=0;
bool dispatchBlitzKey(unsigned vk,bool control) { if(!blitzCapturesKey(vk,control)) return false; ++blitzInputs; return true; }
unsigned speechInputs=0;
void interruptSpeechForInput() { ++speechInputs; }
bool activity=false; unsigned activityInputs=0;
bool minigameCapturesKey(unsigned vk) { return activity && vk==VK_F6; }
bool dispatchMinigameKey(unsigned vk,bool) { if(!minigameCapturesKey(vk)) return false; ++activityInputs; return true; }
void installController() {}
unsigned controllerPumps=0;
void pumpController() { ++controllerPumps; }
bool ownedPad=false;
bool controllerOwned() { return ownedPad; }
std::vector<unsigned> menuKeys;
void modMenuKey(unsigned vk,bool) { menuKeys.push_back(vk); }
void pumpUtilities() {}
bool nativeGameThread() { return true; }
bool tryNativeReaderAccess() noexcept {return nativeGameThread();}
void releaseNativeReaderAccess() noexcept {}
bool observeGameThread() { return true; }
void updateUtilityContext() {}
void cancelLightningWarning() {}
void resetOverdrive() {}
void resetWakka() {}
void pumpPuzzleSolver() {}
bool testModMenu=false;
bool testPlanner=false;
bool modMenuActive() { return testModMenu || testPlanner; }
bool testConsole=false;
bool utilityConsoleActive() { return testConsole; }
unsigned epoch=1;
struct Listener { Handler enter, leave; };
std::map<unsigned,Listener> events;
bool hook(unsigned rva,Handler enter,Handler leave) { events[rva]={enter,leave}; return true; }
bool hookAddress(uintptr_t,unsigned id,Handler enter,Handler leave) { return hook(id,enter,leave); }
bool applyAddedHooks() { return true; }
uintptr_t address(unsigned offset) { return offset?offset:reinterpret_cast<uintptr_t>(GetModuleHandleW(nullptr)); }
bool copy(uintptr_t p,void* out,size_t length) {
    __try { memcpy(out,reinterpret_cast<void*>(p),length); return true; }
    __except(1) { return false; }
}
unsigned generation() { return epoch; }
void log(const char*,...) {}
unsigned inputPressure=0;
bool output(const wchar_t*,bool,const char*) {++inputPressure;return true;}
}
using namespace ffx;
static unsigned pressed=0, shifted=0;
static bool statusVisible=false;
static bool statusContext() { return statusVisible; }
static bool gridVisible=false;
static unsigned gridEntries=0;
static bool gridContext() {return gridVisible;}
static void gridEntry(bool) {++gridEntries;}
static unsigned previewNode=0,observedNode=0,previewCalls=0;
static bool previewBackward=false;
static void gridPreview(bool backward) {observedNode=previewNode;previewBackward=backward;++previewCalls;}
static bool fieldConsole=true;
static bool consoleContext() { return fieldConsole; }
static unsigned consolePresses=0;
static void consoleKey(bool shift) { (void)shift; if(!scene) ++consolePresses; }
static void key(bool shift) { ++pressed; if(shift)++shifted; }
static unsigned functionKeyCalls=0;
static bool guideKeyContext=false;
static bool guideContext() {return guideKeyContext;}
static void functionKey(bool) {++functionKeyCalls;}
static void check(bool value,const char* reason) { if(!value){std::fprintf(stderr,"FAIL: %s\n",reason);std::exit(1);} }
static void fire(unsigned id,Event e) {
    e.rva=id;
    if(id==0xfffe0001) { directInputStateForTest(e,e.before[31]==2); return; }
    if(id==0xfffe0002) { directInputDataForTest(e,e.before[31]==2); return; }
    if(id==0xfffe0003) { directInputReleaseForTest(e); return; }
    const auto handler=events.at(id);
    if(handler.enter)handler.enter(e); if(handler.leave)handler.leave(e);
}
static void drain() { fire(0x471d10,{}); }
int main() {
    // Set only this test thread's message-key table. Physical async input and
    // the user's desktop are unchanged while GetKeyState becomes deterministic.
    BYTE fixtureModifiers[256]{};
    check(SetKeyboardState(fixtureModifiers)!=0,"isolate synthetic message modifiers");
    installInput(); hotkey('N',key); hotkey(VK_OEM_4,key);
    // The actual mod_menu fixture checks production registrations. Exercise
    // that ownership layout through the production native input adapters too.
    for(unsigned vk=VK_F2;vk<=VK_F5;++vk) hotkeyWhen(vk,functionKey,gridContext);
    for(unsigned vk=VK_F6;vk<=VK_F8;++vk) hotkey(vk,functionKey);
    for(unsigned vk=VK_F9;vk<=VK_F11;++vk) hotkeyWhen(vk,functionKey,guideContext);
    for(unsigned vk=VK_F1;vk<=VK_F12;++vk) {
        const bool reserved=vk>=VK_F6 && vk<=VK_F8;
        check(capturesKey(vk)==reserved,"ordinary field reserves only F6-F8");
        MSG probe{};probe.message=WM_KEYDOWN;probe.wParam=vk;
        Event message;message.result=1;message.args[0]=reinterpret_cast<uintptr_t>(&probe);message.args[4]=PM_REMOVE;
        fire(0xffff0000,message);drain();
        check((probe.message==WM_NULL)==reserved,"Windows field function-key presses pass unless reserved");
        probe.message=WM_KEYUP;fire(0xffff0000,message);
        check((probe.message==WM_NULL)==reserved,"Windows releases retain the original function-key owner");
    }
    for(unsigned vk=VK_F9;vk<=VK_F11;++vk) {
        guideKeyContext=true;const auto before=functionKeyCalls;
        check(consumeKey(vk,false),"contextual guide function key can queue");guideKeyContext=false;drain();
        check(functionKeyCalls==before,"guide closure retires queued function keys without field fallthrough");
    }
    check(events.count(0x225930)!=0,"framework input callback covers save/load independently of field input");
    fire(0x225930,{}); check(controllerPumps==1 && !pressed,"save/load input refreshes SDL without dispatching field commands");
    hotkeyWhen(VK_OEM_5,gridPreview,gridContext);gridVisible=true;previewNode=1;
    check(queueControllerHotkey(VK_OEM_5,false,false,gridContext) && !previewCalls,
        "controller grid request copies only command before native input return");
    // The producer between entry/return is controlled here; the real input
    // queue/return dispatcher is production code, not a duplicate implementation.
    previewNode=2;fire(0x225930,{});
    check(previewCalls==1 && observedNode==2 && !previewBackward,
        "controller cursor guidance observes the newly current native cursor at keyboard's boundary");
    check(queueControllerHotkey(VK_OEM_5,true,false,gridContext),"backward controller preview admitted");
    previewNode=3;fire(0x225930,{});
    check(previewCalls==2 && observedNode==3 && previewBackward,"successive commands refresh cursor and preserve backward direction");
    fire(0x225930,{});check(previewCalls==2,"later native returns do not replay a controller command");
    check(queueControllerHotkey(VK_OEM_5,false,false,gridContext),"scoped controller request admitted before grid exit");
    gridVisible=false;fire(0x225930,{});check(previewCalls==2,"closed grid drops pending command without field route fallthrough");
    check(!queueControllerHotkey(VK_OEM_5,false,false,gridContext),"closed grid cannot queue a new controller request");
    gridVisible=true;queueControllerHotkey(VK_OEM_5,false,false,gridContext);++epoch;fire(0x225930,{});
    check(previewCalls==2,"generation retires queued controller preview");
    queueControllerHotkey(VK_OEM_5,false,false,gridContext);testForeground=false;fire(0x225930,{});testForeground=true;
    check(previewCalls==2,"focus loss retires queued controller preview");
    for(unsigned i=0;i<64;++i) check(queueControllerHotkey(VK_OEM_5,false,false,gridContext),"bounded controller FIFO accepts its declared capacity");
    check(!queueControllerHotkey(VK_OEM_5,false,false,gridContext) && inputPressure==1,"controller overload refuses immediately with feedback");
    gridVisible=false;fire(0x225930,{});check(previewCalls==2,"retirement clears a full controller queue without stale output");
    MSG msg{}; msg.message=WM_KEYDOWN; msg.wParam='N';
    Event message; message.result=1; message.args[0]=reinterpret_cast<uintptr_t>(&msg); message.args[4]=PM_REMOVE;
    fire(0xffff0000,message); check(!pressed,"copied message waits for owner input event");
    drain(); check(pressed==1,"message key press");
    check(msg.message==WM_NULL,"mod key removed from the game's Windows message");
    msg.message=WM_KEYDOWN;
    msg.lParam=1u<<30; fire(0xffff0000,message); drain(); check(pressed==1,"message repeat filtered");
    msg.lParam=0; message.args[4]=0; fire(0xffff0000,message); drain(); check(pressed==1,"Peek without removal filtered");
    message.args[4]=PM_REMOVE; message.result=static_cast<uintptr_t>(-1);
    fire(0xffff0002,message); drain(); check(pressed==1,"GetMessage error filtered");
    message.result=1; testForeground=false; fire(0xffff0000,message); drain(); check(pressed==1,"foreign foreground filtered");
    testForeground=true; fire(0xffff0000,message); ++epoch; drain(); check(pressed==1,"old generation key dropped");
    msg.message=WM_KEYDOWN;
    // Nested A/W forwarding has one message owner.
    Event outer=message; outer.rva=0xffff0000;
    events.at(0xffff0000).enter(outer);
    fire(0xffff0001,message); events.at(0xffff0000).leave(outer); drain();
    check(pressed==2,"nested Windows message produces one event");
    uintptr_t table[32]{}; for(auto& slot:table)slot=0x10000;
    uintptr_t device=reinterpret_cast<uintptr_t>(table), object=reinterpret_cast<uintptr_t>(&device);
    std::array<uint8_t,32> nativeKeyboard{}; memcpy(nativeKeyboard.data()+0x14,&object,sizeof(object));
    Event update; update.ecx=reinterpret_cast<uintptr_t>(nativeKeyboard.data()); fire(0x22d770,update);
    check(events.count(0x22d770) && reinterpret_cast<uintptr_t*>(device)[9]!=0x10000,
          "native PInput keyboard update patches synthetic 32-entry vtable without capabilities");
    std::array<uint8_t,256> keys{};
    Event state; state.args[0]=object; state.args[1]=256; state.args[2]=reinterpret_cast<uintptr_t>(keys.data());
    constexpr unsigned functionScans[]={DIK_F1,DIK_F2,DIK_F3,DIK_F4,DIK_F5,DIK_F6,DIK_F7,DIK_F8,DIK_F9,DIK_F10,DIK_F11,DIK_F12};
    for(unsigned i=0;i<std::size(functionScans);++i) {
        keys.fill(0);fire(0xfffe0001,state);keys[functionScans[i]]=0x80;fire(0xfffe0001,state);drain();
        check(keys[functionScans[i]]==(i>=5 && i<=7?0:0x80),"DirectInput state preserves nonreserved field function keys");
    }
    keys.fill(0);fire(0xfffe0001,state);
    keys[DIK_N]=0x80; fire(0xfffe0001,state); fire(0xffff0000,message); drain();
    check(pressed==3,"DirectInput is the single keyboard producer");
    check(!keys[DIK_N],"mod key removed from game DirectInput state");
    keys[DIK_N]=0x80;
    fire(0xfffe0001,state); drain(); check(pressed==3,"held DirectInput key filtered");
    keys.fill(0); fire(0xfffe0001,state); keys[DIK_LSHIFT]=0x80; keys[DIK_LBRACKET]=0x80;
    const auto shiftedBefore=shifted; // Earlier synthetic messages use neutral modifiers.
    fire(0xfffe0001,state); drain(); check(pressed==4&&shifted==shiftedBefore+1,"DirectInput Shift category key");
    state.result=static_cast<uintptr_t>(DIERR_INPUTLOST); fire(0xfffe0001,state); state.result=0;
    keys.fill(0); keys[DIK_N]=0x80; fire(0xfffe0001,state); drain(); check(pressed==5,"reacquired keyboard");
    keys.fill(0); keys[DIK_UP]=0x80; fire(0xfffe0001,state);
    testModMenu=true; keys[DIK_UP]=0x80; fire(0xfffe0001,state);
    check(!keys[DIK_UP],"game key held before opening mod menu is captured");
    testModMenu=false; keys[DIK_UP]=0x80; fire(0xfffe0001,state);
    check(!keys[DIK_UP],"held key drains after closing mod menu");
    keys.fill(0); fire(0xfffe0001,state);
    DIDEVICEOBJECTDATA records[2]{}; records[0].dwOfs=DIK_N; records[0].dwData=0;
    records[1].dwOfs=DIK_N; records[1].dwData=0x80; unsigned count=2;
    Event data; data.args[0]=object; data.args[1]=sizeof(records[0]); data.args[2]=reinterpret_cast<uintptr_t>(records);
    data.args[3]=reinterpret_cast<uintptr_t>(&count); data.args[4]=DIGDD_PEEK;
    fire(0xfffe0002,data); drain(); check(pressed==5 && count==1,"buffered Peek suppresses mod press without enqueueing or draining");
    records[0].dwOfs=DIK_N; records[0].dwData=0;
    records[1].dwOfs=DIK_N; records[1].dwData=0x80; count=2;
    data.args[4]=0; fire(0xfffe0002,data); drain(); check(pressed==6,"buffered keyboard events");
    Event release; release.args[0]=object; fire(0xfffe0003,release);
    msg.message=WM_KEYDOWN;
    fire(0xffff0000,message); drain(); check(pressed==7,"released keyboard restores message fallback");
    msg.message=WM_KEYDOWN; testConsole=true; fire(0xffff0000,message); drain(); check(pressed==7,"typing in utility console cannot invoke game hotkeys");
    msg.message=WM_KEYDOWN; testConsole=false; testModMenu=true; fire(0xffff0000,message); drain(); check(pressed==7,"mod menu input cannot invoke game hotkeys");
    testModMenu=false;
    uintptr_t padTable[32]{}; for(auto& slot:padTable)slot=0x20000;
    uintptr_t padVtable=reinterpret_cast<uintptr_t>(padTable),padObject=reinterpret_cast<uintptr_t>(&padVtable);
    DIJOYSTATE2 joystick{}; joystick.rgbButtons[0]=0x80;
    Event padState; padState.args[0]=padObject; padState.args[1]=sizeof(joystick); padState.args[2]=reinterpret_cast<uintptr_t>(&joystick);
    directInputStateForTest(padState,false);
    check(joystick.rgbButtons[0]==0x80,"foreign synthetic pad remains unchanged until native classification");
    std::array<uint8_t,32> nativePad{}; memcpy(nativePad.data()+0x14,&padObject,sizeof(padObject));
    Event padUpdate; padUpdate.ecx=reinterpret_cast<uintptr_t>(nativePad.data()); fire(0x22db80,padUpdate); ownedPad=true;
    padState.result=static_cast<uintptr_t>(DIERR_INPUTLOST); directInputStateForTest(padState,true);
    check(!joystick.rgbButtons[0] && joystick.rgdwPOV[0]==0xffffffff && padState.overrideResult && padState.result==DI_OK,"native pad state is neutral while SDL owns controls");
    // Both captured menus own presses, held repeats and releases in the
    // message fallback; closing a menu must drain its held/release key too.
    menuKeys.clear(); msg.wParam=VK_DOWN; msg.lParam=0; msg.message=WM_KEYDOWN;
    testPlanner=true; fire(0xffff0000,message); drain();
    check(msg.message==WM_NULL && menuKeys==std::vector<unsigned>{VK_DOWN},"planner exclusively receives arrow press");
    msg.message=WM_KEYDOWN; msg.lParam=1u<<30; fire(0xffff0000,message); drain();
    check(msg.message==WM_NULL && menuKeys.size()==1,"planner arrow repeat cannot leak or duplicate navigation");
    testPlanner=false; msg.message=WM_KEYDOWN; fire(0xffff0000,message);
    check(msg.message==WM_NULL,"message held after planner close remains consumed");
    msg.message=WM_KEYUP; fire(0xffff0000,message);
    check(msg.message==WM_NULL,"planner arrow release consumed after close");
    msg.message=WM_KEYDOWN; msg.lParam=0; fire(0xffff0000,message);
    check(msg.message==WM_KEYDOWN,"fresh arrow restores game control after release");
    testModMenu=true; msg.message=WM_KEYDOWN; fire(0xffff0000,message); drain();
    check(msg.message==WM_NULL && menuKeys.size()==2,"settings exclusively receives arrow press");
    msg.message=WM_KEYUP; fire(0xffff0000,message); testModMenu=false;
    // Recover the live instance from PInput after final Release, as in the game.
    fire(0x22d770,update);
    // The game sees neutral DirectInput state while either menu is open.
    keys.fill(0); keys[DIK_DOWN]=0x80;
    testPlanner=true; fire(0xfffe0001,state); drain();
    check(!keys[DIK_DOWN] && menuKeys.back()==VK_DOWN,"instance keyboard arrow belongs only to planner");
    keys[DIK_DOWN]=0x80; testPlanner=false; fire(0xfffe0001,state);
    check(!keys[DIK_DOWN],"planner's held DirectInput arrow drains after close");
    keys.fill(0); fire(0xfffe0001,state); keys[DIK_DOWN]=0x80; fire(0xfffe0001,state);
    check(keys[DIK_DOWN]==0x80,"fresh DirectInput arrow restores game after release");
    keys.fill(0); fire(0xfffe0001,state); keys[DIK_UP]=0x80; testModMenu=true; fire(0xfffe0001,state);
    check(!keys[DIK_UP],"settings takes exclusive DirectInput arrows");
    testModMenu=false;
    hotkeyWhen(VK_RIGHT,key,statusContext);
    check(!capturesKey(VK_RIGHT),"inactive status leaves game arrows intact");
    statusVisible=true; check(capturesKey(VK_RIGHT),"status captures arrows conditionally");
    const auto beforeStatus=pressed; consumeKey(VK_RIGHT,false); statusVisible=false; drain();
    check(pressed==beforeStatus,"queued status arrow cannot execute after closure");
    statusVisible=true; consumeKey(VK_RIGHT,false); drain(); check(pressed==beforeStatus+1,"active status arrow dispatches");
    statusVisible=false;
    hotkey(VK_F6,key); activity=true;
    const auto beforeActivity=pressed; consumeKey(VK_F6,false); activity=false; drain();
    check(pressed==beforeActivity && activityInputs==0,"queued race key cannot fall through to field beacon after finish");
    activity=true; consumeKey(VK_F6,false); drain();
    check(activityInputs==1 && pressed==beforeActivity,"active minigame key owns its contextual action"); activity=false;
    hotkeyChordWhen(VK_F1,true,consoleKey,consoleContext,true);
    check(!capturesKey(VK_F1,false) && capturesKey(VK_F1,true),"field console requires Shift without capturing plain F1");
    check(!capturesKey(VK_F12,false) && !capturesKey(VK_F12,true),"F12 remains available to NVDA");
    const auto beforeHistory=speechInputs;
    check(!consumeKey(VK_F12,false) && speechInputs==beforeHistory,"NVDA F12 passthrough does not silence its history");
    observeInputDevice(true);
    consumeKey(VK_DOWN,false); check(speechInputs==beforeHistory+1 && !nativeInputUsesController(),"native menu keyboard input selects keyboard prompts and interrupts without mod capture");
    consumeKey(VK_F1,true); scene=6; drain(); check(!consolePresses,"queued console shortcut is cancelled outside the field"); scene=0;
    scene=6;check(consumeKey(VK_F1,true),"unavailable console still reserves Shift F1");drain();
    check(!consolePresses,"unavailable console handler cannot open a field console");scene=0;
    fieldConsole=false; check(!consumeKey(VK_F1,true),"excluded console registration leaves Shift F1 native"); fieldConsole=true;
    keys.fill(0); fire(0xfffe0001,state); keys[DIK_F1]=0x80; fire(0xfffe0001,state); drain();
    check(keys[DIK_F1]==0x80 && !consolePresses,"plain DirectInput F1 stays with game");
    keys.fill(0); fire(0xfffe0001,state); keys[DIK_RSHIFT]=0x80; keys[DIK_F1]=0x80; fire(0xfffe0001,state); drain();
    check(!keys[DIK_F1] && consolePresses==1,"Shift F1 opens console once from native keyboard");
    keys[DIK_RSHIFT]=0x80; keys[DIK_F1]=0x80; fire(0xfffe0001,state); drain(); check(consolePresses==1,"held console chord cannot reopen it");
    keys.fill(0); fire(0xfffe0001,state);
    DIDEVICEOBJECTDATA chordRecords[2]{}; chordRecords[0].dwOfs=DIK_LSHIFT; chordRecords[0].dwData=0x80;
    chordRecords[1].dwOfs=DIK_F1; chordRecords[1].dwData=0x80; unsigned chordCount=2;
    data.args[2]=reinterpret_cast<uintptr_t>(chordRecords); data.args[3]=reinterpret_cast<uintptr_t>(&chordCount); data.args[4]=DIGDD_PEEK;
    fire(0xfffe0002,data); drain(); check(chordCount==1 && consolePresses==1,"buffered Peek recognizes Shift F1 without dispatch");
    chordRecords[1].dwOfs=DIK_F1; chordRecords[1].dwData=0x80; chordCount=2; data.args[4]=0;
    fire(0xfffe0002,data); drain(); check(chordCount==1 && consolePresses==2,"buffered read dispatches console chord once");
    // Use the real bounded queue and both production input adapters. A console
    // overload may refuse its action, but must never turn into native F1/turbo.
    keys.fill(0);fire(0xfffe0001,state);directKeyboard(false);
    const auto beforeConsolePressure=consolePresses;
    for(unsigned i=0;i<64;++i) check(consumeKey('N',false),"fill actual command queue before console chord");
    check(consumeKey(VK_F1,true),"reserved console chord is swallowed even when its command cannot queue");
    check(!consumeKey(VK_F1,false),"plain F1 passes under the same full queue");
    fixtureModifiers[VK_SHIFT]=0x80;check(SetKeyboardState(fixtureModifiers)!=0,"set message Shift for console pressure");
    msg={};msg.message=WM_KEYDOWN;msg.wParam=VK_F1;fire(0xffff0000,message);
    check(msg.message==WM_NULL,"Windows console chord cannot leak under pressure");
    testConsole=true;fixtureModifiers[VK_SHIFT]=0;SetKeyboardState(fixtureModifiers);
    msg.message=WM_KEYDOWN;msg.lParam=1u<<30;fire(0xffff0000,message);
    check(msg.message==WM_NULL,"opening console retains ownership of repeated F1 after Shift release");
    msg.message=WM_KEYUP;fire(0xffff0000,message);
    check(msg.message==WM_NULL,"opening console retains ownership through Windows F1 release");
    testConsole=false;
    keys[DIK_LSHIFT]=0x80;keys[DIK_F1]=0x80;fire(0xfffe0001,state);
    check(!keys[DIK_F1],"DirectInput state console chord cannot leak under pressure");
    testConsole=true;keys[DIK_LSHIFT]=0;keys[DIK_F1]=0x80;fire(0xfffe0001,state);
    check(!keys[DIK_F1],"DirectInput keeps console F1 held suppression after Shift release");
    keys.fill(0);fire(0xfffe0001,state);testConsole=false;
    chordRecords[0]={};chordRecords[0].dwOfs=DIK_LSHIFT;chordRecords[0].dwData=0x80;
    chordRecords[1]={};chordRecords[1].dwOfs=DIK_F1;chordRecords[1].dwData=0x80;chordCount=2;data.args[4]=DIGDD_PEEK;
    fire(0xfffe0002,data);check(chordCount==1,"buffered Peek hides reserved F1 with a full command queue");
    chordRecords[1].dwOfs=DIK_F1;chordRecords[1].dwData=0x80;chordCount=2;data.args[4]=0;
    fire(0xfffe0002,data);check(chordCount==1,"buffered consuming read also hides reserved F1 under pressure");
    testConsole=true;
    chordRecords[0].dwOfs=DIK_LSHIFT;chordRecords[0].dwData=0;
    chordRecords[1].dwOfs=DIK_F1;chordRecords[1].dwData=0;chordCount=2;
    fire(0xfffe0002,data);check(chordCount==1 && chordRecords[0].dwOfs==DIK_LSHIFT,"buffered F1 release remains console owned");
    testConsole=false;directKeyboard(true);drain();
    check(consolePresses==beforeConsolePressure,"pressure never invents or replays a console request");
    keys.fill(0);fire(0xfffe0001,state);keys[DIK_F1]=0x80;fire(0xfffe0001,state);
    check(keys[DIK_F1]==0x80,"fresh plain F1 returns to game after complete console release");
    keys.fill(0);fire(0xfffe0001,state);
    directKeyboard(false);const auto beforeMessageConsole=consolePresses;
    fixtureModifiers[VK_SHIFT]=0x80;SetKeyboardState(fixtureModifiers);
    msg={};msg.message=WM_KEYDOWN;msg.wParam=VK_F1;fire(0xffff0000,message);drain();
    check(msg.message==WM_NULL && consolePresses==beforeMessageConsole+1,"Windows fallback dispatches a fresh console chord once");
    testForeground=false;fixtureModifiers[VK_SHIFT]=0;SetKeyboardState(fixtureModifiers);
    msg.message=WM_KEYDOWN;msg.lParam=1u<<30;fire(0xffff0000,message);
    check(msg.message==WM_NULL,"foreground handoff cannot release held console F1 to game");
    msg.message=WM_KEYUP;fire(0xffff0000,message);
    check(msg.message==WM_NULL,"focus loss retains Windows F1 ownership through release");
    testForeground=true;msg.message=WM_KEYDOWN;msg.lParam=0;fire(0xffff0000,message);
    check(msg.message==WM_KEYDOWN,"fresh plain Windows F1 stays native after console handoff");
    msg.message=WM_KEYUP;fire(0xffff0000,message);directKeyboard(true);
    keys.fill(0); fire(0xfffe0001,state); keys[DIK_LCONTROL]=0x80; keys[DIK_DOWN]=0x80;
    fire(0xfffe0001,state); drain(); check(!keys[DIK_DOWN] && blitzInputs==1,"Ctrl Down has one Blitzball buffer owner and never reaches game selection");
    keys.fill(0); fire(0xfffe0001,state); keys[DIK_DOWN]=0x80; fire(0xfffe0001,state); drain();
    check(keys[DIK_DOWN]==0x80 && blitzInputs==1,"plain Down remains the game player's selector");
    keys.fill(0); fire(0xfffe0001,state); keys[DIK_LCONTROL]=0x80; keys[DIK_UP]=0x80;
    fire(0xfffe0001,state); drain(); check(!keys[DIK_UP] && fieldInputs==1,"Ctrl Up has one native swimming controls owner");
    keys.fill(0); fire(0xfffe0001,state); sensorReview=true; keys[DIK_LCONTROL]=0x80; keys[DIK_RIGHT]=0x80;
    fire(0xfffe0001,state); drain(); check(!keys[DIK_RIGHT] && sensorInputs==1,"Ctrl Right has one Sensor review owner without target movement");
    keys.fill(0); fire(0xfffe0001,state); keys[DIK_LCONTROL]=0x80; keys[DIK_RIGHT]=0x80;
    fire(0xfffe0001,state); sensorReview=false; drain(); check(sensorInputs==1,"expired Sensor context discards its queued review key");
    // Fixed queue capacity preserves equal events and restores native handback
    // under pressure. Availability changes retire old queued commands.
    hotkey('N',key);const auto beforePressure=pressed;
    for(unsigned i=0;i<64;++i) check(consumeKey('N',false),"distinct equal key commands fit the fixed event budget");
    check(!consumeKey('N',false),"saturated key queue hands the current press back immediately");
    directKeyboard(true);drain();check(pressed==beforePressure,"availability revision cancels queued keys without clearing a shared container");
    check(consumeKey('N',false),"publication resumes after old queue drained");drain();check(pressed==beforePressure+1,"fresh command retained after pressure handback");
    hotkey('I',key);const auto beforeEsc=pressed;
    testEsc=true;check(consumeKey('I',false),"Escape captures manual help");
    fire(0x225930,{});check(pressed==beforeEsc+1,"Escape dispatches I at PInput return without any field input event");
    drain();check(pressed==beforeEsc+1,"later field callback cannot dispatch the same Escape key twice");
    consumeKey('I',false);consumeKey('I',false);fire(0x225930,{});
    check(pressed==beforeEsc+3,"equal distinct Escape help requests remain distinct");
    consumeKey('I',false);testEsc=false;drain();check(pressed==beforeEsc+3,"closed Escape retires its pending help key");
    consumeKey('I',false);testEsc=true;fire(0x225930,{});
    check(pressed==beforeEsc+3,"opening Escape does not replay an underlying field command");testEsc=false;
    check(consumeKey('I',false),"native Help captures manual help");fire(0x225930,{});
    check(pressed==beforeEsc+4,"native Help also dispatches I without field-input events");
    // Original 76F580 always calls 776220 after reading menu input. Help can
    // process commands without the field/PInput return used above.
    const auto beforeHelp=pressed;
    consumeKey('I',false);
    check(events.count(0x376220)!=0,"native Help dispatcher owns a copied-key drain independent of PInput and field updates");
    fire(0x376220,{});
    check(pressed==beforeHelp+1,"Help input return dispatches I when no PInput callback occurs");
    fire(0x225930,{});drain();
    check(pressed==beforeHelp+1,"later PInput and field callbacks cannot replay the same Help request");
    const auto beforeGrid=pressed;gridVisible=true;hotkeyChordWhen(VK_F6,false,gridEntry,gridContext);
    check(consumeKey(VK_F6,false),"F6 grid entry captured under its grid scope");gridVisible=false;fire(0x225930,{});
    check(!gridEntries && pressed==beforeGrid,"expired grid F6 cannot fall through into the field beacon");
    consumeKey(VK_F6,false);fire(0x225930,{});check(pressed==beforeGrid+1 && !gridEntries,"outside grid F6 retains the existing field function");
    gridVisible=true;consumeKey(VK_F6,false);fire(0x225930,{});check(gridEntries==1 && pressed==beforeGrid+1,"active grid F6 invokes only node entry");gridVisible=false;
    keys.fill(0);fire(0xfffe0001,state);const auto beforeDialog=pressed;
    testInputDialog=true;testForeground=false;ownedPad=false;
    keys[DIK_N]=0x80;keys[DIK_RETURN]=0x80;fire(0xfffe0001,state);drain();
    check(!keys[DIK_N]&&!keys[DIK_RETURN]&&pressed==beforeDialog,"node edit dialog suppresses game and mod keyboard presses even without SDL ownership");
    keys[DIK_N]=0x80;keys[DIK_RETURN]=0x80;fire(0xfffe0001,state);drain();
    check(!keys[DIK_N]&&!keys[DIK_RETURN]&&pressed==beforeDialog,"typing repeats stay with the dialog");
    records[0].dwOfs=DIK_N;records[0].dwData=0;records[1].dwOfs=DIK_RETURN;records[1].dwData=0;count=2;
    data.args[2]=reinterpret_cast<uintptr_t>(records);data.args[3]=reinterpret_cast<uintptr_t>(&count);data.args[4]=0;
    fire(0xfffe0002,data);drain();check(!count&&pressed==beforeDialog,"dialog owns buffered key releases");
    joystick.rgbButtons[0]=0x80;padState.result=0;padState.overrideResult=false;directInputStateForTest(padState,true);
    check(!joystick.rgbButtons[0]&&joystick.rgdwPOV[0]==0xffffffff&&padState.overrideResult,"dialog neutralizes native DirectInput pad fallback independently of SDL");
    keys.fill(0);fire(0xfffe0001,state);keys[DIK_N]=0x80;fire(0xfffe0001,state);
    testInputDialog=false;testForeground=true;keys[DIK_N]=0x80;fire(0xfffe0001,state);drain();
    check(!keys[DIK_N]&&pressed==beforeDialog,"dialog's held shortcut drains after closing");
    keys.fill(0);fire(0xfffe0001,state);keys[DIK_N]=0x80;fire(0xfffe0001,state);drain();
    check(pressed==beforeDialog+1,"fresh shortcut works again after dialog key release");
    std::puts("PASS: native keyboard, Escape PInput manual help/retirement, exclusive Shift-F1 under queue pressure/console focus, plain F1 and NVDA F12 passthrough, modifier/context shortcuts, exclusive menus, buffered Peek, focus, generation and SDL isolation.");
}
