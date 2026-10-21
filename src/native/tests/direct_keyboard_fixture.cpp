// DirectInput COM lifetime fixture; it never acquires
// keyboard focus, injects keys, starts speech or opens the game.
#include "../events.hpp"
#define DIRECTINPUT_VERSION 0x0800
#include <dinput.h>
#include <map>
#include <array>
#include <cstdio>
#include <cstring>
namespace ffx {
std::map<unsigned,Handler> fixtureHooks;
bool hook(unsigned rva,Handler enter,Handler) { fixtureHooks[rva]=enter; return rva==0x22d770 || rva==0x22db80; }
bool nativeGameThread() { return true; }
bool tryNativeReaderAccess() noexcept {return nativeGameThread();}
void releaseNativeReaderAccess() noexcept {}
bool registered=false;
unsigned registrations=0;
bool gameFocused() { return true; }
bool inputDialogActive() {return false;}
bool modMenuActive() { return false; }
bool utilityConsoleActive() { return false; }
bool controllerOwned() { return false; }
bool capturesKey(unsigned,bool,bool) { return false; }
bool consumeKey(unsigned,bool,bool) { return false; }
void autoWalkKeyboard(uint8_t*) {}
void directKeyboard(bool active) { registered=active; if(active) ++registrations; }
unsigned generation() { return 1; }
uintptr_t address(unsigned rva) { return reinterpret_cast<uintptr_t>(GetModuleHandleW(nullptr))+rva; }
bool copy(uintptr_t source,void* target,size_t size) {
    __try { memcpy(target,reinterpret_cast<void*>(source),size); return true; }
    __except(1) { return false; }
}
void log(const char*,...) {}
}
struct Context { IDirectInput8A* input; unsigned devices=0, instanceGuids=0; bool ok=true; };
static BOOL CALLBACK keyboard(const DIDEVICEINSTANCEA* instance,void* data) {
    auto& context=*static_cast<Context*>(data);
    IDirectInputDevice8A* device=nullptr;
    if(FAILED(context.input->CreateDevice(instance->guidInstance,&device,nullptr))) { context.ok=false; return DIENUM_CONTINUE; }
    ++context.devices;
    if(!IsEqualGUID(instance->guidInstance,GUID_SysKeyboard)) ++context.instanceGuids;
    std::array<uint8_t,32> native{};
    const auto raw=reinterpret_cast<uintptr_t>(device);
    memcpy(native.data()+0x14,&raw,sizeof(raw));
    ffx::Event update; update.ecx=reinterpret_cast<uintptr_t>(native.data());
    auto handler=ffx::fixtureHooks.at(0x22d770); handler(update);
    if(!ffx::registered) context.ok=false;
    device->Release();
    if(ffx::registered) context.ok=false;
    return DIENUM_CONTINUE;
}
LONG CALLBACK fixtureFault(EXCEPTION_POINTERS* fault) {
    if(fault->ExceptionRecord->ExceptionCode==EXCEPTION_ACCESS_VIOLATION) {
        MEMORY_BASIC_INFORMATION info{};VirtualQuery(fault->ExceptionRecord->ExceptionAddress,&info,sizeof(info));wchar_t module[MAX_PATH]{};GetModuleFileNameW(static_cast<HMODULE>(info.AllocationBase),module,MAX_PATH);
        std::fprintf(stderr,"COM fault %lx module-RVA=%lx EAX=%lx ECX=%lx EDX=%lx\n",fault->ExceptionRecord->ExceptionCode,static_cast<unsigned long>(reinterpret_cast<uintptr_t>(fault->ExceptionRecord->ExceptionAddress)-reinterpret_cast<uintptr_t>(info.AllocationBase)),fault->ContextRecord->Eax,fault->ContextRecord->Ecx,fault->ContextRecord->Edx);
        std::fwprintf(stderr,L"Module: %ls\n",module);
    }
    return EXCEPTION_CONTINUE_SEARCH;
}
int main() {
    AddVectoredExceptionHandler(1,fixtureFault);
    wchar_t path[MAX_PATH]{};
    if(!GetSystemDirectoryW(path,MAX_PATH) || wcscat_s(path,L"\\dinput8.dll")) return 1;
    const auto system=LoadLibraryW(path);
    using Create=HRESULT(WINAPI*)(HINSTANCE,DWORD,REFIID,LPVOID*,LPUNKNOWN);
    const auto create=reinterpret_cast<Create>(GetProcAddress(system,"DirectInput8Create"));
    IDirectInput8A* input=nullptr;
    if(!create || FAILED(create(GetModuleHandleW(nullptr),DIRECTINPUT_VERSION,IID_IDirectInput8A,reinterpret_cast<void**>(&input),nullptr))) return 1;
    ffx::installDirectInputKeyboard();
    Context context{input};
    const auto result=input->EnumDevices(DI8DEVCLASS_KEYBOARD,keyboard,&context,DIEDFL_ATTACHEDONLY);
    input->Release();
    if(FAILED(result) || !context.ok || !context.devices || ffx::registrations!=context.devices) {
        fprintf(stderr,"FAIL: PInput update registration: devices=%u registered=%u hr=%lx\n",context.devices,ffx::registrations,result); return 1;
    }
    printf("PASS: PInput update registered %u DirectInput keyboard(s), including %u with instance GUIDs distinct from SysKeyboard; Release restored registration state.\n",context.devices,context.instanceGuids);
}
