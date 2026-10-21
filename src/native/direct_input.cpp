// Consume the game's actual DirectInput reads; no input polling is added.
#include "events.hpp"
#define DIRECTINPUT_VERSION 0x0800
#include <dinput.h>
#include <atomic>
#include <utility>
#include "controller.hpp"
#include "mod_menu.hpp"
#include "settings.hpp"
#include "utilities.hpp"
#include "auto_walk.hpp"

namespace ffx {
namespace {
std::atomic<uintptr_t> keyboard{0};
std::array<uint8_t, 256> held{};
std::array<bool,256> swallowed{};
uintptr_t activeKeyboard=0;
unsigned virtualKey(unsigned scan) {
    return MapVirtualKeyW(scan & 0x80 ? 0xe000 | (scan & 0x7f) : scan, MAPVK_VSC_TO_VK_EX);
}
void deviceState(Event& e) {
    if((controllerOwned() || inputDialogActive()) && e.before[31]==2 && e.args[1]<=sizeof(DIJOYSTATE2)) {
        uint8_t neutral[sizeof(DIJOYSTATE2)]{};
        if(e.args[1]>=sizeof(DIJOYSTATE)) for(unsigned i=0;i<4;++i) reinterpret_cast<DIJOYSTATE*>(neutral)->rgdwPOV[i]=0xffffffff;
        for(unsigned i=0;i<e.args[1];++i) write<uint8_t>(e.args[2]+i,neutral[i]);
        e.result=DI_OK; e.overrideResult=true; return;
    }
    if (e.args[0] != keyboard || e.args[1] != 256) return;
    if (FAILED(static_cast<HRESULT>(e.result))) { held.fill(0); swallowed.fill(false); return; }
    std::array<uint8_t, 256> keys{};
    if (!copy(e.args[2], keys.data(), keys.size())) return;
    const bool shift = (keys[DIK_LSHIFT] | keys[DIK_RSHIFT]) & 0x80;
    const bool control = (keys[DIK_LCONTROL] | keys[DIK_RCONTROL]) & 0x80;
    const bool modal=inputDialogActive() || (gameFocused() && modMenuActive() && !utilityConsoleActive());
    for (unsigned i = 0; i < keys.size(); ++i) {
        if((keys[i]&0x80) && !(held[i]&0x80)) swallowed[i]=consumeKey(virtualKey(i),shift,control);
        if(modal && (keys[i]&0x80)) swallowed[i]=true;
        if(!(keys[i]&0x80)) swallowed[i]=false;
        if(swallowed[i]) write<uint8_t>(e.args[2]+i,0);
    }
    held = keys;
    autoWalkKeyboard(keys.data());
    for(unsigned scan=0;scan<keys.size();++scan) if(keys[scan]!=held[scan]) write<uint8_t>(e.args[2]+scan,keys[scan]);
}
void deviceData(Event& e) {
    if((controllerOwned() || inputDialogActive()) && e.before[31]==2) { write<unsigned>(e.args[3],0); e.result=DI_OK; e.overrideResult=true; return; }
    if (e.args[0] != keyboard || FAILED(static_cast<HRESULT>(e.result))) return;
    const unsigned count = read<unsigned>(e.args[3]);
    if (count > 256 || e.args[1] < 16 || e.args[1] > 32 || !e.args[2]) return;
    const bool peek=(e.args[4]&DIGDD_PEEK)!=0;
    auto nextHeld=held;
    auto nextSwallowed=swallowed;
    unsigned kept=0;
    for (unsigned i = 0; i < count; ++i) {
        const auto entry = e.args[2] + i * e.args[1];
        const unsigned scan = read<unsigned>(entry), data = read<unsigned>(entry + 4);
        if (scan >= held.size()) continue;
        const bool down = (data & 0x80) != 0, wasDown = (nextHeld[scan] & 0x80) != 0;
        nextHeld[scan] = down ? 0x80 : 0;
        const bool shift=((nextHeld[DIK_LSHIFT] | nextHeld[DIK_RSHIFT]) & 0x80) != 0;
        const bool control=((nextHeld[DIK_LCONTROL] | nextHeld[DIK_RCONTROL]) & 0x80) != 0;
        if (down && !wasDown) nextSwallowed[scan]=peek?capturesKey(virtualKey(scan),shift,control):consumeKey(virtualKey(scan),shift,control);
        if(inputDialogActive() || (gameFocused() && modMenuActive() && !utilityConsoleActive())) nextSwallowed[scan]=true;
        const bool suppress=nextSwallowed[scan];
        if(!down) nextSwallowed[scan]=false;
        if(!suppress) {
            uint8_t record[32]{}; if(copy(entry,record,e.args[1])) {
                for(unsigned j=0;j<e.args[1];++j) write<uint8_t>(e.args[2]+kept*e.args[1]+j,record[j]);
                ++kept;
            }
        }
    }
    if(!peek) { held=nextHeld; swallowed=nextSwallowed; }
    write<unsigned>(e.args[3],kept);
}
// Windows SDK IDirectInputDevice8 has exactly32 methods. Each template
// carries its immutable table identity even if another adapter later chains it.
enum class Kind : unsigned { Keyboard=1,Pad=2 };
struct Table {
    std::array<uintptr_t,32> original{};
    // dinput8 uses vtable[-1] as its interface adjustment, not only32 methods.
    std::array<uintptr_t,33> adapted{};
    uintptr_t source=0;Kind kind=Kind::Keyboard;
};
std::array<Table,32> tables;
unsigned usedTables=0;
template<size_t I> ULONG WINAPI releaseAdapter(void* object) {
    using Function=ULONG(WINAPI*)(void*);
    const auto result=reinterpret_cast<Function>(tables[I].original[2])(object);
    const auto error=GetLastError();
    if(!result) {
        auto token=reinterpret_cast<uintptr_t>(object);
        if(keyboard.compare_exchange_strong(token,0)) {
            NativeReaderAccess access;
            if(access) { activeKeyboard=0;held.fill(0);swallowed.fill(false);directKeyboard(false); }
        }
    }
    SetLastError(error);return result;
}
template<size_t I> HRESULT WINAPI stateAdapter(void* object,DWORD size,void* buffer) {
    using Function=HRESULT(WINAPI*)(void*,DWORD,void*);
    const auto result=reinterpret_cast<Function>(tables[I].original[9])(object,size,buffer);
    const auto error=GetLastError();HRESULT filtered=result;
    NativeReaderAccess access;
    if(access) {
        try {
            Event e;e.args[0]=reinterpret_cast<uintptr_t>(object);e.args[1]=size;e.args[2]=reinterpret_cast<uintptr_t>(buffer);
            e.result=static_cast<uintptr_t>(result);e.before[31]=static_cast<intptr_t>(tables[I].kind);
            deviceState(e);if(e.overrideResult) filtered=static_cast<HRESULT>(e.result);
        } catch(...) {} // Native input remains intact on adapter failure.
    }
    SetLastError(error);return filtered;
}
template<size_t I> HRESULT WINAPI dataAdapter(void* object,DWORD stride,void* buffer,DWORD* count,DWORD flags) {
    using Function=HRESULT(WINAPI*)(void*,DWORD,void*,DWORD*,DWORD);
    const auto result=reinterpret_cast<Function>(tables[I].original[10])(object,stride,buffer,count,flags);
    const auto error=GetLastError();HRESULT filtered=result;
    NativeReaderAccess access;
    if(access) {
        try {
            Event e;e.args[0]=reinterpret_cast<uintptr_t>(object);e.args[1]=stride;e.args[2]=reinterpret_cast<uintptr_t>(buffer);
            e.args[3]=reinterpret_cast<uintptr_t>(count);e.args[4]=flags;e.result=static_cast<uintptr_t>(result);
            e.before[31]=static_cast<intptr_t>(tables[I].kind);deviceData(e);if(e.overrideResult) filtered=static_cast<HRESULT>(e.result);
        } catch(...) {}
    }
    SetLastError(error);return filtered;
}
struct Methods {uintptr_t release,state,data;};
template<size_t... I> std::array<Methods,sizeof...(I)> adapterMethods(std::index_sequence<I...>) {
    return {{{reinterpret_cast<uintptr_t>(&releaseAdapter<I>),reinterpret_cast<uintptr_t>(&stateAdapter<I>),reinterpret_cast<uintptr_t>(&dataAdapter<I>)}...}};
}
const auto methods=adapterMethods(std::make_index_sequence<32>{});
bool bindDevice(uintptr_t object,Kind kind) {
    const auto source=read<uintptr_t>(object);if(!object || !source) return false;
    Table* selected=nullptr;
    for(unsigned i=0;i<usedTables;++i) {
        auto& t=tables[i];
        if(source==reinterpret_cast<uintptr_t>(t.adapted.data()+1)) return t.kind==kind;
        if(t.source==source && t.kind==kind) {selected=&t;break;}
    }
    if(!selected) {
        if(usedTables==tables.size()) return false;
        auto& t=tables[usedTables];
        if(source<4 || !copy(source-4,t.adapted.data(),sizeof(t.adapted))) return false;
        std::memcpy(t.original.data(),t.adapted.data()+1,sizeof(t.original));
        if(!t.original[2] || !t.original[9] || !t.original[10]) return false;
        t.source=source;t.kind=kind;
        t.adapted[3]=methods[usedTables].release;t.adapted[10]=methods[usedTables].state;t.adapted[11]=methods[usedTables].data;
        selected=&t;++usedTables;
    }
    return write<uintptr_t>(object,reinterpret_cast<uintptr_t>(selected->adapted.data()+1));
}
void keyboardUpdating(Event& e) {
    if(!nativeGameThread()) return;
    const auto object=read<uintptr_t>(e.ecx+0x14);
    if(!object || !bindDevice(object,Kind::Keyboard)) {if(activeKeyboard) {keyboard=0;activeKeyboard=0;held.fill(0);swallowed.fill(false);directKeyboard(false);}return;}
    const auto previous=keyboard.exchange(object);
    if(previous!=object || activeKeyboard!=object) {activeKeyboard=object;held.fill(0);swallowed.fill(false);directKeyboard(true);}
}
void padUpdating(Event& e) {
    if(nativeGameThread()) bindDevice(read<uintptr_t>(e.ecx+0x14),Kind::Pad);
}
}
void observeDirectInput(uintptr_t) {
    // Factory GUIDs can be instance IDs. Native PInput updates qualify kind,
    // so bootstrap/proxy never calls COM APIs or intercepts foreign devices.
}
void installDirectInputKeyboard() {
    hook(0x22d770,keyboardUpdating,nullptr);hook(0x22db80,padUpdating,nullptr);
}
#ifdef FFX_READER_TEST
void directInputStateForTest(Event& e,bool pad) {e.before[31]=pad?2:1;deviceState(e);}
void directInputDataForTest(Event& e,bool pad) {e.before[31]=pad?2:1;deviceData(e);}
void directInputReleaseForTest(Event& e) {if(!e.result && keyboard==e.args[0]) {keyboard=0;activeKeyboard=0;held.fill(0);swallowed.fill(false);directKeyboard(false);}}
#endif
}
