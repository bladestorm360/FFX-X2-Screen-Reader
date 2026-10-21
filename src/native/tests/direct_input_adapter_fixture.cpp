#include "../events.hpp"
#define DIRECTINPUT_VERSION 0x0800
#include <dinput.h>
#include <atomic>
#include <cstdio>
#include <cstdlib>
#include <cstring>
namespace ffx {
DWORD owner=0;Handler keyboardUpdate=nullptr,padUpdate=nullptr;
unsigned registrations=0;bool registered=false,padOwned=false,foreground=true;
std::atomic<unsigned> capabilitiesCalls{0},stateCalls{0},dataCalls{0},releaseCalls{0};
bool readerAccess=true;
bool nativeGameThread() {return GetCurrentThreadId()==owner;}
bool tryNativeReaderAccess() noexcept {return nativeGameThread() && readerAccess;}
void releaseNativeReaderAccess() noexcept {}
bool hook(unsigned rva,Handler before,Handler) {if(rva==0x22d770)keyboardUpdate=before;else if(rva==0x22db80)padUpdate=before;else std::abort();++registrations;return true;}
bool gameFocused() {return foreground;}
bool inputDialogActive() {return false;}
bool modMenuActive() {return false;}
bool utilityConsoleActive() {return false;}
bool controllerOwned() {return padOwned;}
bool capturesKey(unsigned vk,bool,bool) {return vk=='N';}
bool consumeKey(unsigned vk,bool,bool) {SetLastError(9876);return foreground && vk=='N';}
void autoWalkKeyboard(uint8_t*) {}
void directKeyboard(bool active) {registered=active;SetLastError(8765);}
bool copy(uintptr_t source,void* target,size_t size) {__try{std::memcpy(target,reinterpret_cast<void*>(source),size);return true;}__except(1){return false;}}
}
using namespace ffx;
struct Device {uintptr_t* table;bool destroy=false;};
struct NativeTable {uintptr_t adjustment=0x14;std::array<uintptr_t,32> entries;};
NativeTable originalTable;auto& original=originalTable.entries;
HRESULT fixtureResult=DI_OK;bool throwOriginal=false;
HRESULT WINAPI originalCaps(void*,void*) {++capabilitiesCalls;return DI_OK;}
ULONG WINAPI originalRelease(void* object) {if(static_cast<Device*>(object)->table[-1]!=0x14)std::abort();++releaseCalls;if(static_cast<Device*>(object)->destroy)VirtualFree(object,0,MEM_RELEASE);SetLastError(0x7420);return 0;}
HRESULT WINAPI originalState(void* object,DWORD,void*) {if(static_cast<Device*>(object)->table[-1]!=0x14)std::abort();++stateCalls;if(throwOriginal)RaiseException(0xe0424242,0,0,nullptr);SetLastError(0x7418);return fixtureResult;}
HRESULT WINAPI originalData(void*,DWORD,void*,DWORD*,DWORD) {++dataCalls;SetLastError(0x7419);return fixtureResult;}
HRESULT WINAPI originalPoll(void*) {return 0x1234;}
using State=HRESULT(WINAPI*)(void*,DWORD,void*);
using Data=HRESULT(WINAPI*)(void*,DWORD,void*,DWORD*,DWORD);
using Release=ULONG(WINAPI*)(void*);
void check(bool good,const char* reason) {if(!good){std::fprintf(stderr,"FAIL: %s\n",reason);std::exit(1);}}
void update(Device* device,bool pad=false) {std::array<uint8_t,32> native{};auto raw=reinterpret_cast<uintptr_t>(device);std::memcpy(native.data()+0x14,&raw,sizeof(raw));Event e;e.ecx=reinterpret_cast<uintptr_t>(native.data());(pad?padUpdate:keyboardUpdate)(e);}
bool exceptionPropagates(State state,void* device) {__try{state(device,0,nullptr);}__except(GetExceptionCode()==0xe0424242?EXCEPTION_EXECUTE_HANDLER:EXCEPTION_CONTINUE_SEARCH){return true;}return false;}
struct Foreign {State state;Device* device;uint8_t key=0x80;HRESULT result=0;};
DWORD WINAPI foreignRead(void* context) {auto& value=*static_cast<Foreign*>(context);std::array<uint8_t,256> keys{};keys[DIK_N]=0x80;value.result=value.state(value.device,256,keys.data());value.key=keys[DIK_N];return 0;}
int main() {
    owner=GetCurrentThreadId();installDirectInputKeyboard();check(registrations==2,"exact native keyboard and DirectInput pad owner events");
    original.fill(reinterpret_cast<uintptr_t>(&originalPoll));original[2]=reinterpret_cast<uintptr_t>(&originalRelease);original[3]=reinterpret_cast<uintptr_t>(&originalCaps);
    original[9]=reinterpret_cast<uintptr_t>(&originalState);original[10]=reinterpret_cast<uintptr_t>(&originalData);
    Device native{original.data()},external{original.data()};observeDirectInput(reinterpret_cast<uintptr_t>(&external));
    check(external.table==original.data() && !registered,"foreign COM factory is never intercepted");update(&native);
    check(registered && native.table!=original.data() && external.table==original.data(),"only the verified live native instance receives its bounded adapter");
    check(native.table[-1]==0x14,"native interface adjustment preceding the vtable preserved");
    for(unsigned i=0;i<32;++i)if(i!=2 && i!=9 && i!=10)check(native.table[i]==original[i],"all untouched COM methods preserved exactly");
    auto state=reinterpret_cast<State>(native.table[9]);auto data=reinterpret_cast<Data>(native.table[10]);auto release=reinterpret_cast<Release>(native.table[2]);
    std::array<uint8_t,256> keys{};keys[DIK_N]=0x80;
    check(state(&native,256,keys.data())==DI_OK && !keys[DIK_N] && GetLastError()==0x7418,"stdcall keyboard state filtering preserves original LastError");
    readerAccess=false;keys.fill(0);keys[DIK_N]=0x80;
    check(state(&native,256,keys.data())==DI_OK && keys[DIK_N]==0x80 && GetLastError()==0x7418,"busy/loading reader state hands original keyboard input back without filtering");
    readerAccess=true;
    keys.fill(0);state(&native,256,keys.data());fixtureResult=DIERR_INPUTLOST;keys[DIK_N]=0x80;
    check(state(&native,256,keys.data())==DIERR_INPUTLOST && keys[DIK_N]==0x80,"native keyboard error forwarded without modifying failed data");fixtureResult=DI_OK;
    DIDEVICEOBJECTDATA records[2]{};records[0].dwOfs=DIK_N;records[0].dwData=0;records[1].dwOfs=DIK_N;records[1].dwData=0x80;DWORD count=2;
    check(data(&native,sizeof(records[0]),records,&count,DIGDD_PEEK)==DI_OK && count==1 && GetLastError()==0x7419,"exact buffered method ABI and Peek suppression preserve return and LastError");
    Foreign foreign{state,&native};const auto thread=CreateThread(nullptr,0,foreignRead,&foreign,0,nullptr);check(thread!=nullptr,"fixture foreign caller created");WaitForSingleObject(thread,3000);CloseHandle(thread);
    check(foreign.result==DI_OK && foreign.key==0x80,"foreign method callers forward original data without reading native mod state");
    throwOriginal=true;check(exceptionPropagates(state,&native),"native original exception remains native");throwOriginal=false;
    // Slot-specific adapters still find the original after an unrelated chain changes the object table.
    native.table=external.table;keys.fill(0);check(state(&native,256,keys.data())==DI_OK,"saved adapter does not depend on the object's later vtable");update(&native);
    check(release(&native)==0 && !registered && GetLastError()==0x7420,"final Release retains count and native error while restoring keyboard handback");
    Device pad{original.data()};update(&pad,true);padOwned=true;DIJOYSTATE2 joystick{};joystick.rgbButtons[0]=0x80;fixtureResult=DIERR_INPUTLOST;
    check(reinterpret_cast<State>(pad.table[9])(&pad,sizeof(joystick),&joystick)==DI_OK && !joystick.rgbButtons[0] && joystick.rgdwPOV[0]==0xffffffff,"known native pad neutral input and POV on SDL ownership");
    padOwned=false;joystick.rgbButtons[0]=0x80;
    check(reinterpret_cast<State>(pad.table[9])(&pad,sizeof(joystick),&joystick)==DIERR_INPUTLOST && joystick.rgbButtons[0]==0x80,"missing physical backend leaves original native input intact");fixtureResult=DI_OK;
    auto* dying=static_cast<Device*>(VirtualAlloc(nullptr,4096,MEM_COMMIT|MEM_RESERVE,PAGE_READWRITE));check(dying!=nullptr,"fixture releasable native object");dying->table=original.data();dying->destroy=true;update(dying);
    auto destroy=reinterpret_cast<Release>(dying->table[2]);check(destroy(dying)==0 && !registered && GetLastError()==0x7420,"adapter never dereferences object after its original final Release");
    std::array<std::array<uintptr_t,32>,33> distinct{};unsigned unbound=0;
    for(auto& table:distinct){table=original;Device candidate{table.data()};update(&candidate);if(candidate.table==table.data())++unbound;}
    check(unbound>=3 && !registered,"finite process-lifetime table capacity fails back to native input");
    check(capabilitiesCalls==0 && registrations==2 && stateCalls>=8 && dataCalls==1 && releaseCalls==2,"bounded discovery invokes no capabilities query or late hook registration");
    std::puts("PASS: x86 DirectInput adapters; exact COM ABI, errors, owner/foreign isolation, Release lifetime, finite capacity and native handback.");
}
