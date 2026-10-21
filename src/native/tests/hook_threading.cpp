#include "../events.hpp"
#include "../map_transition.hpp"
#include <MinHook.h>
#include <atomic>
#include <thread>
#include <cstdio>
#include <cstdlib>
#include <stdexcept>

namespace ffx {
std::atomic<unsigned> epoch{1};
uintptr_t address(unsigned rva) { return rva?rva:reinterpret_cast<uintptr_t>(GetModuleHandleW(nullptr)); }
bool copy(uintptr_t p,void* out,size_t length) {
    __try { memcpy(out,reinterpret_cast<void*>(p),length); return true; }
    __except(1) { return false; }
}
unsigned generation() { return epoch.load(); }
void log(const char*,...) {}
}
namespace {
std::atomic<unsigned> callbacks{0},inside{0},overlaps{0},resets{0};
std::vector<std::wstring> copiedRows;
HANDLE started=nullptr,loading=nullptr,releaseLoad=nullptr;
HANDLE deferredStarted=nullptr,deferredRelease=nullptr;
HANDLE foreignStart=nullptr,foreignDone=nullptr;
HANDLE readerBusy=nullptr,readerRelease=nullptr;
using ForeignCall=int(__cdecl*)(int);
ForeignCall foreignDevice=nullptr;
bool throwLoad=false,nestedLoad=false;
unsigned arrivals=0;
void check(bool ok,const char* reason) {
    if(!ok) { std::fprintf(stderr,"FAIL: %s\n",reason); std::exit(1); }
}
__declspec(noinline) int __cdecl contentionCall(int value) {volatile int result=value;return result+9;}
void holdReader(ffx::Event&) {SetEvent(readerBusy);check(WaitForSingleObject(readerRelease,5000)==WAIT_OBJECT_0,"release held fixture reader");}
void reader(ffx::Event&) {
    ++callbacks;
    if(inside.fetch_add(1)) ++overlaps;
    else {
        // Exercise aligned allocations used by navigation and text snapshots.
        copiedRows.assign(64,std::wstring(300,L'A'));
        copiedRows.clear();
    }
    --inside;
}
void reset(ffx::Event& event) {
    check(!inside.load(),"resource reset cannot overlap an active reader");
    ++resets; ++ffx::epoch; event.epoch=ffx::generation(); // Loader publishes POD only.
}
__declspec(noinline) int __cdecl observed(int value) { volatile int result=value; return result+7; }
__declspec(noinline) int __cdecl mapRequest(int value) { volatile int result=value; return result+3; }
__declspec(noinline) int __cdecl windowMessage(int value) { volatile int result=value; return result+4; }
__declspec(noinline) int __cdecl deviceMethod(int value) { volatile int result=value; return result+8; }
__declspec(noinline) int __cdecl ownerCallback(int value) { volatile int result=value; return result+6; }
void waitingOwner(ffx::Event&) {
    SetEvent(foreignStart);
    check(WaitForSingleObject(foreignDone,2000)==WAIT_OBJECT_0,
        "foreign window hook must not wait for the reader lock while the input owner waits for its native worker");
}
__declspec(noinline) int __cdecl deferredEvent(int value) {
    volatile int result=value;
    SetEvent(deferredStarted);
    check(WaitForSingleObject(deferredRelease,5000)==WAIT_OBJECT_0,"release deferred native event");
    return result+2;
}
__declspec(noinline) int __cdecl resourceLoad(int value) {
    volatile int result=value;
    if(throwLoad) throw std::runtime_error("native load unwind");
    if(nestedLoad) result=mapRequest(result);
    SetEvent(loading);
    check(WaitForSingleObject(releaseLoad,5000)==WAIT_OBJECT_0,"release native loader");
    return result+9;
}
__declspec(noinline) int __cdecl fieldInitialize(int value) {
    volatile int result=resourceLoad(value);
    observed(5);
    return result+5;
}
void fieldReturned(ffx::Event& event) {
    check(event.epoch==ffx::generation() && event.args[0]==2,"completed field event uses the fresh generation and copied request");
    ++arrivals;
}
void concurrent() {
    check(WaitForSingleObject(started,5000)==WAIT_OBJECT_0,"start reader threads");
    for(unsigned i=0;i<1000;++i) check(observed(5)==12,"original remains unchanged across threads");
}
}
int main() {
    SetErrorMode(SEM_FAILCRITICALERRORS|SEM_NOGPFAULTERRORBOX);
    check(ffx::observeGameThread(),"establish the qualified native owner before ordinary callbacks");
    check(MH_Initialize()==MH_OK,"initialize native hooks");
    check(ffx::hookAddress(reinterpret_cast<uintptr_t>(observed),1,reader,reader),"ordinary event hook");
    check(ffx::hookAddress(reinterpret_cast<uintptr_t>(deferredEvent),2,reader,reader),"event spanning load completion");
    check(ffx::hookAddress(reinterpret_cast<uintptr_t>(resourceLoad),0x5093b0,reset,nullptr),"resource scope hook");
    check(ffx::hookAddress(reinterpret_cast<uintptr_t>(mapRequest),0x46fed0,reset,nullptr),"map scope hook");
    check(ffx::hookAddress(reinterpret_cast<uintptr_t>(fieldInitialize),ffx::map_transition::Initialize,reset,fieldReturned),"completed field scope hook");
    check(ffx::enableHooks(),"enable hooks");
    started=CreateEventW(nullptr,TRUE,FALSE,nullptr);
    loading=CreateEventW(nullptr,TRUE,FALSE,nullptr);
    releaseLoad=CreateEventW(nullptr,TRUE,FALSE,nullptr);
    deferredStarted=CreateEventW(nullptr,TRUE,FALSE,nullptr);
    deferredRelease=CreateEventW(nullptr,TRUE,FALSE,nullptr);
    check(started && loading && releaseLoad && deferredStarted && deferredRelease,"native test events");
    std::thread first(concurrent),second(concurrent);
    SetEvent(started); first.join(); second.join();
    check(callbacks==0 && !overlaps && !inside,"foreign ordinary calls cannot access owner-affine reader state");
    for(unsigned i=0;i<2000;++i) check(observed(5)==12,"owner original result preserved");
    check(callbacks==4000 && !overlaps && !inside,"all equal distinct owner events retained without a reader lock");
    check(ffx::observeGameThread(),"establish owner for nonblocking reader access");
    nestedLoad=true;
    std::thread loader([] { check(resourceLoad(2)==14,"nested native loading preserves result"); });
    check(WaitForSingleObject(loading,5000)==WAIT_OBJECT_0,"loader reached native body");
    check(!ffx::nativeReaderResourcesReady(),"published resource readiness is false during native loading");
    {ffx::NativeReaderAccess access;check(!access,"COM adapters cannot access reader state during native resource loading");}
    const auto before=callbacks.load();
    check(observed(5)==12 && callbacks==before,"no native getter reader during worker resource loading");
    std::thread deferred([] { check(deferredEvent(3)==5,"deferred original remains unchanged"); });
    check(WaitForSingleObject(deferredStarted,5000)==WAIT_OBJECT_0,"ordinary native event can run during loading");
    SetEvent(releaseLoad); loader.join();
    check(ffx::nativeReaderResourcesReady(),"published resource readiness restored after nested loading");
    SetEvent(deferredRelease); deferred.join();
    check(callbacks==before,"event entered while loading cannot later use an absent before snapshot");
    check(resets==2 && observed(5)==12 && callbacks==before+2,"nested generations release loading barrier");
    throwLoad=true;
    bool caught=false;
    try { resourceLoad(2); } catch(const std::runtime_error&) { caught=true; }
    throwLoad=false;
    check(caught && observed(5)==12 && callbacks==before+4,"unwound loader scope releases on next event");
    auto prior=callbacks.load();
    check(fieldInitialize(2)==19 && arrivals==1 && callbacks==prior,"field completion survives nested load generations while ordinary readers stay suspended");
    check(fieldInitialize(2)==19 && arrivals==2,"each complete field initialization has one arrival event");
    throwLoad=true; caught=false;
    try { fieldInitialize(2); } catch(const std::runtime_error&) { caught=true; }
    throwLoad=false;
    check(caught && observed(5)==12 && callbacks==prior+2 && arrivals==2,"failed field initialization emits no arrival and releases loading scopes");
    check(ffx::observeGameThread(),"establish the native input owner");
    readerBusy=CreateEventW(nullptr,TRUE,FALSE,nullptr);readerRelease=CreateEventW(nullptr,TRUE,FALSE,nullptr);
    check(readerBusy && readerRelease,"reader-access contention events");
    check(ffx::hookAddress(reinterpret_cast<uintptr_t>(contentionCall),5,holdReader,nullptr),"held reader access fixture");
    check(ffx::applyAddedHooks(),"enable reader-access fixture");
    std::thread contender([] {check(contentionCall(1)==10,"foreign original call preserved");});contender.join();
    check(WaitForSingleObject(readerBusy,0)==WAIT_TIMEOUT,"foreign call never enters owner reader or waits on it");
    SetLastError(0x7541);
    {ffx::NativeReaderAccess access;check(bool(access),"owner COM access requires no cross-thread reader lock");}
    check(GetLastError()==0x7541,"owner access preserves LastError");
    std::thread independentLoader([] {check(resourceLoad(2)==14,"foreign resource original still runs");});independentLoader.join();
    CloseHandle(readerBusy);CloseHandle(readerRelease);
    check(ffx::hookAddress(reinterpret_cast<uintptr_t>(windowMessage),0xffff0000,reader,reader),"shared window message hook");
    check(ffx::hookAddress(reinterpret_cast<uintptr_t>(deviceMethod),0xfffe0000,reader,reader),"shared COM device hook");
    check(ffx::hookAddress(reinterpret_cast<uintptr_t>(ownerCallback),4,waitingOwner,nullptr),"input owner waiting for native worker");
    check(ffx::applyAddedHooks(),"enable worker ownership regression");
    foreignStart=CreateEventW(nullptr,TRUE,FALSE,nullptr);
    foreignDone=CreateEventW(nullptr,TRUE,FALSE,nullptr);
    check(foreignStart && foreignDone,"foreign worker regression events");
    auto thunk=static_cast<uint8_t*>(VirtualAlloc(nullptr,32,MEM_COMMIT|MEM_RESERVE,PAGE_READWRITE));
    check(thunk!=nullptr,"foreign COM caller mapping");
    const uint8_t code[]={0xff,0x74,0x24,0x04,0xb8,0,0,0,0,0xff,0xd0,0x83,0xc4,0x04,0xc3};
    memcpy(thunk,code,sizeof(code));
    const auto target=reinterpret_cast<uintptr_t>(deviceMethod); memcpy(thunk+5,&target,4);
    DWORD oldProtection=0;
    check(VirtualProtect(thunk,32,PAGE_EXECUTE_READ,&oldProtection)!=0,"foreign COM caller executable protection");
    FlushInstructionCache(GetCurrentProcess(),thunk,32); foreignDevice=reinterpret_cast<ForeignCall>(thunk);
    prior=callbacks.load();
    std::thread foreign([] {
        check(WaitForSingleObject(foreignStart,2000)==WAIT_OBJECT_0,"start the native worker");
        check(foreignDevice(3)==11,"foreign original COM call retains its result");
        check(windowMessage(3)==7,"foreign original window call retains its result");
        SetEvent(foreignDone);
    });
    check(ownerCallback(3)==9,"owner original remains unchanged while waiting for the worker");
    foreign.join();
    check(callbacks==prior,"foreign message pump never enters the reader callback");
    check(windowMessage(3)==7 && callbacks==prior+2,"owner message pump retains both accessibility events");
    check(deviceMethod(3)==11 && callbacks==prior+4,"game-image COM caller retains both accessibility events");
    ffx::discardHooks();
    CloseHandle(started); CloseHandle(loading); CloseHandle(releaseLoad);
    CloseHandle(deferredStarted); CloseHandle(deferredRelease);
    CloseHandle(foreignStart); CloseHandle(foreignDone);
    VirtualFree(thunk,0,MEM_RELEASE);
    std::puts("PASS: ordinary native owner events retained without callback locks, foreign originals unchanged, foreign message/COM callbacks do not deadlock, copied loader epochs, completed fields, suspension and exception recovery.");
}
