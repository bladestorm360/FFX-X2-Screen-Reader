// Maps copied SDL state to context commands and consumes those inputs immediately; Grid commands queue to the keyboard phase. See Docs/features/index.md, Docs/features/runtime/index.md#input-and-settings and Docs/dylan_reports_20261018.md.
#include "input_device.hpp"
#include "controller.hpp"
#include "blitz_review.hpp"
#include "field_controls.hpp"
#include "battle_sensor.hpp"
#include "auto_walk.hpp"
#include "pedestal.hpp"
#include "events.hpp"
#include "utilities.hpp"
#include "mod_menu.hpp"
#include "settings.hpp"
#include "sphere_grid.hpp"
#include "physical_gamepad.hpp"
#include "nonblocking_snapshots.hpp"
#include <xinput.h>
#include <atomic>
#include <cstring>
#include <algorithm>
#include <cwctype>
namespace ffx {
bool captureControllerBinding(uint64_t);
namespace {
pad::State physicalState;
pad::Router router;
uint64_t consumed=0;
std::atomic<bool> owned{false},drain{false};
struct Bridge { XINPUT_STATE state{};uint64_t counter=0;unsigned device=0,focus=0,overflow=0,epoch=0; };
async::AtomicSnapshot<Bridge> bridge;
XINPUT_STATE filteredState{};
physical::Frame latestFrame;
physical::Edge pendingEdge;
bool havePending=false,haveFrame=false,gameSelect=false;
unsigned contextEpoch=0; // verified native input owner only
std::atomic<bool> importsReady{false};
// Only the native input owner observes battle/grid state. Foreign XInput
// callers consume this copied generation token, never native UI pointers.
std::atomic<uint64_t> reservedContext{0};
unsigned reservedControls() {
    const auto token=reservedContext.load(std::memory_order_acquire);
    return static_cast<unsigned>(token>>32)==generation()?static_cast<unsigned>(token):0;
}
void stripReserved(XINPUT_GAMEPAD& game) {
    const auto mask=reservedControls();
    if(mask&1)game.sThumbRX=game.sThumbRY=0;
    if(mask&2)game.bLeftTrigger=game.bRightTrigger=0;
}
bool currentState(XINPUT_STATE& state,physical::Frame* current=nullptr) {
    if(!owned.load(std::memory_order_acquire) || !nativeReaderResourcesReady()) return false;
    Bridge copied;physical::Frame frame;
    if(!bridge.tryRead(copied) || !physical::snapshot(frame) || !frame.connected || frame.pressure ||
        !physical::fresh(frame.counter,frame.frequency) || !physical::fresh(copied.counter,frame.frequency) || copied.device!=frame.device ||
        copied.focus!=frame.focus || copied.overflow!=frame.overflow || copied.epoch!=generation() ||
        !nativeReaderResourcesReady()) return false;
    state=copied.state;
    stripReserved(state.Gamepad);
    if(!gameFocused()) state.Gamepad={};
    if(current) *current=frame;
    return true;
}
using GetState=DWORD(WINAPI*)(DWORD,XINPUT_STATE*);
using SetState=DWORD(WINAPI*)(DWORD,XINPUT_VIBRATION*);
GetState nativeGet=nullptr;
SetState nativeSet=nullptr;
DWORD WINAPI getState(DWORD index,XINPUT_STATE* state) {
    XINPUT_STATE copied;
    if(!currentState(copied)) {
        const auto result=nativeGet?nativeGet(index,state):ERROR_DEVICE_NOT_CONNECTED;
        if(result==ERROR_SUCCESS && state && inputDialogActive()) state->Gamepad={};
        else if(result==ERROR_SUCCESS && state)stripReserved(state->Gamepad);
        return result;
    }
    if(!state) return ERROR_BAD_ARGUMENTS;
    if(index) return ERROR_DEVICE_NOT_CONNECTED;
    *state=copied;return ERROR_SUCCESS;
}
DWORD WINAPI setState(DWORD index,XINPUT_VIBRATION* vibration) {
    XINPUT_STATE copied;physical::Frame frame;
    if(!currentState(copied,&frame)) return nativeSet?nativeSet(index,vibration):ERROR_DEVICE_NOT_CONNECTED;
    if(!vibration) return ERROR_BAD_ARGUMENTS;
    if(index) return ERROR_DEVICE_NOT_CONNECTED;
    // One bounded queue claim; SDK handles and rumble belong to their owner.
    if(physical::requestRumble((unsigned(vibration->wLeftMotorSpeed)<<16)|vibration->wRightMotorSpeed,frame)) return ERROR_SUCCESS;
    return nativeSet?nativeSet(index,vibration):ERROR_DEVICE_NOT_CONNECTED;
}
#ifndef FFX_SDL_OWNER_TEST
bool replaceImport(const char* name,void* replacement,void** original) {
    const auto base=reinterpret_cast<uintptr_t>(GetModuleHandleW(nullptr));
    IMAGE_DOS_HEADER dos{}; if(!copy(base,&dos,sizeof(dos)) || dos.e_magic!=IMAGE_DOS_SIGNATURE || dos.e_lfanew<=0) return false;
    IMAGE_NT_HEADERS32 nt{}; if(!copy(base+dos.e_lfanew,&nt,sizeof(nt)) || nt.Signature!=IMAGE_NT_SIGNATURE) return false;
    const auto directory=nt.OptionalHeader.DataDirectory[IMAGE_DIRECTORY_ENTRY_IMPORT];
    if(!directory.VirtualAddress || directory.VirtualAddress>=nt.OptionalHeader.SizeOfImage || directory.Size<sizeof(IMAGE_IMPORT_DESCRIPTOR)) return false;
    for(unsigned offset=0;offset+sizeof(IMAGE_IMPORT_DESCRIPTOR)<=directory.Size;offset+=sizeof(IMAGE_IMPORT_DESCRIPTOR)) {
        IMAGE_IMPORT_DESCRIPTOR descriptor{}; if(!copy(base+directory.VirtualAddress+offset,&descriptor,sizeof(descriptor)) || !descriptor.Name) break;
        if(!descriptor.FirstThunk) continue;
        auto module=ascii(base+descriptor.Name,64);
        std::transform(module.begin(),module.end(),module.begin(),[](wchar_t c) { return static_cast<wchar_t>(std::towlower(c)); });
        if(module.find(L"xinput")==std::wstring::npos) continue;
        const auto native=reinterpret_cast<void*>(GetProcAddress(GetModuleHandleW(module.c_str()),name));
        for(unsigned i=0;i<512;++i) {
            if((descriptor.OriginalFirstThunk && descriptor.OriginalFirstThunk>=nt.OptionalHeader.SizeOfImage-i*4) ||
               descriptor.FirstThunk>=nt.OptionalHeader.SizeOfImage-i*4) break;
            const auto slot=reinterpret_cast<void**>(base+descriptor.FirstThunk+i*4);
            const auto current=read<uintptr_t>(reinterpret_cast<uintptr_t>(slot)); if(!current) break;
            const auto thunk=descriptor.OriginalFirstThunk?read<unsigned>(base+descriptor.OriginalFirstThunk+i*4):0;
            // Windows SDK XInput libraries import by ordinal. Compare the
            // resolved export instead of assuming an import has a name.
            const bool named=thunk && !(thunk&IMAGE_ORDINAL_FLAG32) &&
                thunk<nt.OptionalHeader.SizeOfImage-2 && ascii(base+thunk+2,64)==std::wstring(name,name+std::strlen(name));
            if(!named && (!native || current!=reinterpret_cast<uintptr_t>(native))) continue;
            DWORD protection=0;
            if(!VirtualProtect(slot,sizeof(void*),PAGE_READWRITE,&protection)) return false;
            *original=*slot; InterlockedExchangePointer(slot,replacement);
            DWORD ignored=0; VirtualProtect(slot,sizeof(void*),protection,&ignored); return true;
        }
    }
    return false;
}
#endif
bool nativeMenuActive(bool battle) {
    if(battle) {
        const auto index=read<uint8_t>(address(0x1fcc092),255);
        const auto win=index<16?address(0xf3c910)+index*0xf0:0;
        return win && read<uint8_t>(win+1)==4 && read<unsigned>(win+0x44)!=0;
    }
    using MenuMask=unsigned(__cdecl*)();
    return reinterpret_cast<MenuMask>(address(0x4aa410))()!=0 || sphereGridOpen();
}
void padRead(Event&) {}
void nativePad(Event& e) {
    if(!nativeGameThread() || (e.parentRva!=0x22db80 && e.parentRva!=0x22dd50)) return;
    // 62D700 is shared by the native DirectInput and XInput gamepad decoders,
    // and keyboard/mouse base classes: accept only verified pad parents.
    // Replace raw controls before native edge dispatch.
    XINPUT_STATE state{};if(!currentState(state)) {
        const auto mask=reservedControls();
        if(mask&1) {write<int>(e.ecx+0x374,0);write<int>(e.ecx+0x380,0);}
        if(mask&2) {write<uint8_t>(e.ecx+0x39e,0);write<uint8_t>(e.ecx+0x39f,0);}
        return;
    }
    const auto game=state.Gamepad;
    write<int>(e.ecx+0x370,game.sThumbLX/128);
    write<int>(e.ecx+0x37c,-game.sThumbLY/128);
    write<int>(e.ecx+0x374,game.sThumbRX/128);
    write<int>(e.ecx+0x380,-game.sThumbRY/128);
    constexpr WORD masks[]={XINPUT_GAMEPAD_X,XINPUT_GAMEPAD_A,XINPUT_GAMEPAD_B,XINPUT_GAMEPAD_Y,
        XINPUT_GAMEPAD_LEFT_SHOULDER,XINPUT_GAMEPAD_RIGHT_SHOULDER,0,0,
        XINPUT_GAMEPAD_BACK,XINPUT_GAMEPAD_START,XINPUT_GAMEPAD_LEFT_THUMB,XINPUT_GAMEPAD_RIGHT_THUMB,
        XINPUT_GAMEPAD_DPAD_UP,XINPUT_GAMEPAD_DPAD_RIGHT,XINPUT_GAMEPAD_DPAD_DOWN,XINPUT_GAMEPAD_DPAD_LEFT};
    for(unsigned i=0;i<_countof(masks);++i) write<uint8_t>(e.ecx+0x398+i,(game.wButtons&masks[i])!=0);
    write<uint8_t>(e.ecx+0x39e,game.bLeftTrigger!=0);
    write<uint8_t>(e.ecx+0x39f,game.bRightTrigger!=0);
}
void menuEdges(uint64_t edges) {
    if(captureControllerBinding(edges)) return;
    auto edge=[&](int control) { return (edges&pad::bit(control))!=0; };
    if(edge(pad::East) || edge(pad::View) || edge(pad::Menu)) modMenuKey(VK_ESCAPE);
    else if(edge(pad::Up)) modMenuKey(VK_UP);
    else if(edge(pad::Down)) modMenuKey(VK_DOWN);
    else if(edge(pad::Left)) modMenuKey(VK_LEFT);
    else if(edge(pad::Right)) modMenuKey(VK_RIGHT);
    else if(edge(pad::South)) modMenuKey(VK_RETURN);
    else if(edge(pad::West) || edge(pad::RSUp)) modMenuKey('I');
    else if(edge(pad::LB)) modMenuKey(VK_HOME);
    else if(edge(pad::RB)) modMenuKey(VK_END);
}
void command(int id) {
    if(id==pad::GameSelect) { gameSelect=true; return; }
    struct Key { unsigned vk; bool shift; };
    static constexpr Key keys[]={{VK_OEM_3,false},{VK_OEM_4,false},{VK_OEM_6,false},{VK_OEM_4,true},{VK_OEM_6,true},
        {'P',false},{VK_OEM_5,false},{VK_F6,false},{'T',false},{'H',false},{'I',false},{VK_OEM_COMMA,false},{VK_OEM_PERIOD,false},
        {'S',false},{VK_F9,false},{VK_F10,false},{VK_F11,false}};
    if(id==pad::Gil) { dispatchHotkey(inBattle()?'H':'G'); return; }
    if(id==pad::Map) { dispatchHotkey('M'); return; }
    if(id==pad::Filter) { dispatchHotkey('U'); return; }
    dispatchHotkey(keys[id].vk,keys[id].shift);
}
bool gridCommandContext() {return !inBattle() && sphereGridOpen();}
uint64_t freeBattleFaceMask(bool nativeMenu) {
    // Field face assignments do not own these buttons in battle. Preserve any
    // configured battle command before choosing an otherwise unused face.
    for(const int face:{pad::North,pad::South,pad::East,pad::West}) {
        bool used=false;
        for(int id=0;id<pad::CountCommands;++id)
            if(pad::context(id,true,true,nativeMenu) && controllerBinding(id)==face) {used=true;break;}
        if(!used)return pad::bit(face);
    }
    return 0;
}
void publish(bool exclusive) {
    auto visible=physicalState; visible.held&=~consumed;
    if(consumed&pad::bit(pad::RT)) visible.rt=0;
    if(consumed&pad::bit(pad::LT)) visible.lt=0;
    if(consumed&(pad::bit(pad::RSUp)|pad::bit(pad::RSDown)|pad::bit(pad::RSLeft)|pad::bit(pad::RSRight))) visible.rx=visible.ry=0;
    XINPUT_GAMEPAD game{};
    if(!exclusive) autoWalkController(visible);
    if(gameSelect) { game.wButtons=XINPUT_GAMEPAD_BACK; gameSelect=false; }
    if(!exclusive) {
        constexpr int controls[]={pad::South,pad::East,pad::West,pad::North,pad::Menu,pad::L3,pad::R3,pad::LB,pad::RB,pad::Up,pad::Down,pad::Left,pad::Right};
        constexpr WORD buttons[]={XINPUT_GAMEPAD_A,XINPUT_GAMEPAD_B,XINPUT_GAMEPAD_X,XINPUT_GAMEPAD_Y,XINPUT_GAMEPAD_START,
            XINPUT_GAMEPAD_LEFT_THUMB,XINPUT_GAMEPAD_RIGHT_THUMB,XINPUT_GAMEPAD_LEFT_SHOULDER,XINPUT_GAMEPAD_RIGHT_SHOULDER,
            XINPUT_GAMEPAD_DPAD_UP,XINPUT_GAMEPAD_DPAD_DOWN,XINPUT_GAMEPAD_DPAD_LEFT,XINPUT_GAMEPAD_DPAD_RIGHT};
        for(unsigned i=0;i<_countof(controls);++i) if(visible.held&pad::bit(controls[i])) game.wButtons|=buttons[i];
        game.sThumbLX=visible.lx; game.sThumbLY=visible.ly; game.sThumbRX=visible.rx; game.sThumbRY=visible.ry;
        game.bLeftTrigger=visible.lt; game.bRightTrigger=visible.rt;
    }
    stripReserved(game);
    if(std::memcmp(&filteredState.Gamepad,&game,sizeof(game)) || !filteredState.dwPacketNumber) ++filteredState.dwPacketNumber;
    filteredState.Gamepad=game;
    Bridge copied;copied.state=filteredState;copied.counter=latestFrame.counter;
    copied.device=latestFrame.device;copied.focus=latestFrame.focus;copied.overflow=latestFrame.overflow;copied.epoch=contextEpoch;
    bridge.publish(copied);
}
}
void installController() {
    initializeControllerBindings();
    physical::startOwner();
#ifdef FFX_SDL_OWNER_TEST
    extern DWORD WINAPI controllerNativeStateForTest(DWORD,XINPUT_STATE*);
    extern DWORD WINAPI controllerNativeRumbleForTest(DWORD,XINPUT_VIBRATION*);
    nativeGet=controllerNativeStateForTest;nativeSet=controllerNativeRumbleForTest;
    const bool input=true,rumble=true;
#else
    const bool input=replaceImport("XInputGetState",reinterpret_cast<void*>(&getState),reinterpret_cast<void**>(&nativeGet));
    const bool rumble=replaceImport("XInputSetState",reinterpret_cast<void*>(&setState),reinterpret_cast<void**>(&nativeSet));
#endif
    importsReady=input && rumble;
    hook(0x22d700,nativePad,nullptr);
    hook(0x22db80,padRead,nullptr); hook(0x22dd50,padRead,nullptr);
    recordLog("SDL game input imports state=%d vibration=%d",input,rumble);
}
void pumpController() {
    if(!nativeGameThread()) return;
    physical::requestReady();
#if defined(FFX_READER_TEST) && !defined(FFX_SDL_OWNER_TEST)
    physical::manualOwnerStep();
#endif
    if(!importsReady.load()) return;
    if(!nativeReaderResourcesReady()) {reservedContext.store(0,std::memory_order_release);owned.store(false,std::memory_order_release);drain=true;return;}
    const auto nextEpoch=generation();
    const bool battleNow=inBattle(),gridNow=!battleNow && sphereGridOpen();
    reservedContext.store((uint64_t{nextEpoch}<<32)|(battleNow?1u:0u)|(gridNow?2u:0u),std::memory_order_release);
    if(contextEpoch!=nextEpoch) {
        if(contextEpoch) drain=true;
        contextEpoch=nextEpoch;router={};consumed=0;havePending=false;gameSelect=false;
        cancelControllerBinding();
    }
    physical::Frame fresh;
    if(physical::snapshot(fresh)) {
        const bool changed=!haveFrame || fresh.device!=latestFrame.device || fresh.focus!=latestFrame.focus || fresh.overflow!=latestFrame.overflow;
        latestFrame=fresh;haveFrame=true;
        if(changed) {router={};consumed=0;havePending=false;cancelControllerBinding();}
    }
    const auto now=GetTickCount64();
    if(!haveFrame || !latestFrame.connected || latestFrame.pressure || !physical::fresh(latestFrame.counter,latestFrame.frequency)) {
        owned.store(false,std::memory_order_release);router={};consumed=0;return;
    }
    physicalState=latestFrame.state;uint64_t inputStamp=latestFrame.inputStamp;
    for(unsigned i=0;i<8;++i) {
        physical::Edge edge;
        if(havePending) {edge=pendingEdge;havePending=false;}else if(!physical::nextEdge(edge)) break;
        if(edge.device!=latestFrame.device || edge.focus!=latestFrame.focus || edge.overflow!=latestFrame.overflow ||
            now-edge.stamp>250 || !controllerEventCurrent(edge.stamp)) continue;
        if(edge.stamp>latestFrame.stamp) {pendingEdge=edge;havePending=true;break;}
        // Replay discrete quick presses for the command router, but always
        // pass the newest owned analog values to native input. Old snapshots
        // must not turn high-rate axis reports into frame-by-frame movement.
        physicalState.held=edge.state.held;inputStamp=edge.stamp;break;
    }
    if(!controllerEventCurrent(inputStamp)) {router.previous=physicalState.held;router.previousAnalog=physicalState.analogActive();}
    consumed&=physicalState.held;if(drain.exchange(false)) router.draining=true;
    constexpr uint64_t sticks=pad::bit(pad::RSUp)|pad::bit(pad::RSDown)|pad::bit(pad::RSLeft)|pad::bit(pad::RSRight);
    constexpr uint64_t triggers=pad::bit(pad::LT)|pad::bit(pad::RT);
    if(battleNow)consumed|=physicalState.held&sticks;
    if(gridNow)consumed|=physicalState.held&triggers;
    owned.store(true,std::memory_order_release);
    const bool menu=modMenuActive();
    auto result=router.update(physicalState,gameFocused(),menu || utilityConsoleActive());
    if(result.input) observeInputDeviceAt(true,inputStamp);
    if(result.input && !utilityConsoleActive()) interruptSpeechForInput();
    if(result.changed) output(router.mod?L"Mod command mode":L"Game controls",true,"mod-controller");
    constexpr uint64_t reviewDirections=pad::bit(pad::Up)|pad::bit(pad::Down)|pad::bit(pad::Left)|pad::bit(pad::Right);
    const auto reviewKeys=!router.mod?0:blitzReviewAvailable()?reviewDirections|pad::bit(pad::RT):
        (fieldControlsCapturesKey(VK_DOWN,true) || battleSensorAvailable())?reviewDirections:0;
    if(menu) menuEdges(result.edges);
    else if(router.mod && (result.edges&pad::bit(pad::Menu))) {
        dispatchHotkey(VK_F8); router.complete(); result.exclusive=true;
    } else if(result.edges&reviewKeys) {
        for(const auto binding: {std::pair<int,unsigned>{pad::Up,VK_UP},{pad::Down,VK_DOWN},{pad::Left,VK_LEFT},{pad::Right,VK_RIGHT},{pad::RT,VK_OEM_5}})
            if(result.edges&pad::bit(binding.first)) { dispatchHotkey(binding.second,false,binding.first!=pad::RT); break; }
        router.complete(); result.exclusive=true;
    } else if(!utilityConsoleActive()) {
        const bool battle=inBattle(),nativeMenu=nativeMenuActive(battle);
        constexpr uint64_t faces=pad::bit(pad::West)|pad::bit(pad::North)|pad::bit(pad::South)|pad::bit(pad::East);
        if(gridNow && router.mod && (result.edges&faces)) {
            const unsigned key=result.edges&pad::bit(pad::West)?VK_F2:result.edges&pad::bit(pad::North)?VK_F3:
                result.edges&pad::bit(pad::South)?VK_F4:VK_F5;
            queueControllerHotkey(key,false,false,gridCommandContext);router.complete();result.exclusive=true;
        } else if(gridNow && (result.edges&triggers)) {
            queueControllerHotkey(VK_OEM_5,(result.edges&pad::bit(pad::LT))!=0,false,gridCommandContext);
            if(router.mod) {router.complete();result.exclusive=true;}
        } else if(battleNow && (result.edges&(pad::bit(pad::RSLeft)|pad::bit(pad::RSRight)))) {
            const bool order=(result.edges&pad::bit(pad::RSRight))!=0;
            dispatchHotkey(order?'T':'O',true);
            if(router.mod) {router.complete();result.exclusive=true;}
        } else if(battleNow && (result.edges&pad::bit(pad::RSDown))) {
            dispatchHotkey('H',true);
            if(router.mod) {router.complete();result.exclusive=true;}
        } else if(battleNow && router.mod && (result.edges&faces) && (result.edges&freeBattleFaceMask(nativeMenu))) {
            dispatchHotkey('O');router.complete();result.exclusive=true;
        } else {
        for(int id=0;id<pad::CountCommands;++id) {
            const int control=controllerBinding(id);
            if(control>=0 && pad::context(id,router.mod,battle,nativeMenu)) consumed|=physicalState.held&pad::bit(control);
        }
        for(int id=0;id<pad::CountCommands;++id) {
            const int control=controllerBinding(id);
            if(control<0 || !pad::context(id,router.mod,battle,nativeMenu)) continue;
            consumed|=physicalState.held&pad::bit(control);
            if(result.edges&pad::bit(control)) {
                consumed|=pad::bit(control); command(id);
                if(router.mod) { router.complete(); result.exclusive=true; }
                break;
            }
        }
        }
    }
    pedestalControllerInput(physicalState);
    publish(result.exclusive || modMenuActive() || utilityConsoleActive());
}
bool controllerOwned() { XINPUT_STATE state{};return currentState(state); }
void drainController() { drain=true; }
#ifdef FFX_SDL_OWNER_TEST
DWORD controllerStateForTest(DWORD index,XINPUT_STATE* state) {return getState(index,state);}
DWORD controllerRumbleForTest(DWORD index,XINPUT_VIBRATION* value) {return setState(index,value);}
#endif
}
