#include "../events.hpp"
#include <MinHook.h>
#include <atomic>
#include <thread>
#include <cstdio>
#include <cstdlib>
#include <stdexcept>
#include <xmmintrin.h>

namespace ffx {
std::atomic<unsigned> epoch{1};
uintptr_t address(unsigned rva) { return rva; }
bool copy(uintptr_t p,void* out,size_t length) {
    __try { memcpy(out,reinterpret_cast<void*>(p),length); return true; }
    __except(1) { return false; }
}
unsigned generation() { return epoch.load(); }
void log(const char*,...) {}
}
namespace {
void check(bool ok,const char* why) {
    if(!ok) { std::fprintf(stderr,"FAIL: %s\n",why); std::exit(1); }
}
unsigned prepared=0,invoked=0,completed=0,initialized=0,failures=0,retired=0;
unsigned innerReturned=0,outerReturned=0,tailReturned=0;
unsigned expectedOuter=0,expectedTail=0;
bool queueAtInner=false,queueAtTail=false,enqueueFromInit=false,insideInvoke=false;
bool throwNative=false,overrideResult=false;
std::atomic<unsigned> resets{0};
HANDLE loaderStart=nullptr,loaderDone=nullptr,loadBody=nullptr,releaseLoad=nullptr;

__declspec(noinline) int __cdecl nativeInit(int value) {
    volatile int result=value;
    if(throwNative) throw std::runtime_error("synthetic original failure");
    return result+11;
}
__declspec(noinline) int __cdecl inputOwner(int value) { volatile int result=value; return result+2; }
__declspec(noinline) int __cdecl inner(int value) { volatile int result=value; return result+3; }
__declspec(noinline) int __cdecl outer(int value) {
    volatile int result=inner(value);
    check(invoked==0,"nested return must not drain before outer original completes");
    return result+5;
}
__declspec(noinline) int __cdecl tailBody(int value) { volatile int result=value; return result+7; }
__declspec(naked) int __cdecl tailHead(int) {
    __asm {
        push ebp
        mov ebp,esp
        pop ebp
        jmp tailBody
    }
}
__declspec(noinline) int __cdecl resourceLoad(int value) {
    volatile int result=value;
    if(loadBody) {
        SetEvent(loadBody);
        check(WaitForSingleObject(releaseLoad,5000)==WAIT_OBJECT_0,"release synthetic resource original");
    }
    return result+9;
}
void observe(ffx::Event&) { check(ffx::observeGameThread(),"PInput event establishes owner"); }
ffx::NativeAction simple() {
    return {ffx::generation(),
        [] { ++prepared; return true; },
        [] { ++invoked; },
        [](bool ok) { check(ok,"simple native invocation succeeds"); ++completed; },
        [] { ++retired; }};
}
void queueNested() {
    auto action=simple();
    action.invoke=[] {
        check(outerReturned==expectedOuter && tailReturned==expectedTail,
            "all outer and equal-stack tail callbacks precede native action");
        ++invoked;
        insideInvoke=true;
        check(nativeInit(4)==15,"deferred native arguments and result");
        check(invoked==1,"nested native return cannot recursively drain another action");
        _mm_setcsr(0x1f80);
        insideInvoke=false;
    };
    check(ffx::deferNativeAction(std::move(action)),"queue nested native action");
}
void innerLeave(ffx::Event&) { ++innerReturned; if(queueAtInner) queueNested(); }
void outerLeave(ffx::Event& e) {
    ++outerReturned;
    if(overrideResult) { e.overrideResult=true; e.result=123; }
}
void tailLeave(ffx::Event& e) {
    ++tailReturned;
    if(e.rva==12 && queueAtTail) queueNested();
}
void initLeave(ffx::Event&) {
    ++initialized;
    if(enqueueFromInit) {
        auto action=simple();
        action.invoke=[] {
            check(!insideInvoke,"follow-up action waits until original invocation returns"); ++invoked;
        };
        check(ffx::deferNativeAction(std::move(action)),"nested reader queues follow-up");
    }
}
void resourceBefore(ffx::Event& e) { ++resets; ++ffx::epoch; e.epoch=ffx::generation(); }
void resetCounts() { prepared=invoked=completed=initialized=failures=retired=0; }
void boundary() { check(inputOwner(8)==10,"owner original result remains unchanged"); }
void install() {
    check(MH_Initialize()==MH_OK,"initialize MinHook");
    check(ffx::hookAddress(reinterpret_cast<uintptr_t>(inputOwner),0x225930,observe,nullptr),"owner hook");
    check(ffx::hookAddress(reinterpret_cast<uintptr_t>(nativeInit),0x469040,nullptr,initLeave),"initialization hook");
    check(ffx::hookAddress(reinterpret_cast<uintptr_t>(inner),2,nullptr,innerLeave),"nested hook");
    check(ffx::hookAddress(reinterpret_cast<uintptr_t>(outer),3,nullptr,outerLeave),"outer hook");
    check(ffx::hookAddress(reinterpret_cast<uintptr_t>(tailHead),11,nullptr,tailLeave),"tail head hook");
    check(ffx::hookAddress(reinterpret_cast<uintptr_t>(tailBody),12,nullptr,tailLeave),"tail body hook");
    check(ffx::hookAddress(reinterpret_cast<uintptr_t>(resourceLoad),0x5093b0,resourceBefore,nullptr),"loader hook");
    check(ffx::enableHooks(),"enable real adapter hooks");
}
}
int main() {
    SetErrorMode(SEM_FAILCRITICALERRORS|SEM_NOGPFAULTERRORBOX);
    install(); boundary();
    check(ffx::nativeGameThread(),"owner known after PInput callback");
    std::thread wrongOwner([] {
        check(!ffx::nativeGameThread() && !ffx::observeGameThread(),"another thread cannot take owner");
        check(!ffx::deferNativeAction(simple()),"wrong owner cannot queue native actions");
    });
    wrongOwner.join();

    queueAtInner=true; enqueueFromInit=true; overrideResult=true;
    expectedOuter=outerReturned+1; expectedTail=tailReturned;
    _mm_setcsr(0x3f80);
    check(outer(2)==123,"deferred mutation preserves explicit original-result override");
    check(_mm_getcsr()==0x3f80,"deferred mutation preserves saved SSE state");
    check(prepared==2 && invoked==2 && completed==2 && initialized==1 && innerReturned==1,
        "initialization reader runs exactly once and follow-up runs after invocation");
    queueAtInner=enqueueFromInit=overrideResult=false;
    resetCounts(); queueAtTail=true;
    expectedOuter=outerReturned; expectedTail=tailReturned+2;
    check(tailHead(5)==12,"equal-stack tail original result remains intact");
    check(invoked==1 && initialized==1 && completed==1,"tail action drains once after complete tail chain");
    queueAtTail=false;

    resetCounts();
    loaderStart=CreateEventW(nullptr,TRUE,FALSE,nullptr);
    loaderDone=CreateEventW(nullptr,TRUE,FALSE,nullptr);
    check(loaderStart && loaderDone,"loader handshake events");
    std::thread loader([] {
        check(WaitForSingleObject(loaderStart,5000)==WAIT_OBJECT_0,"start loader during native action");
        check(resourceLoad(1)==10,"loader original completes while owner invokes");
        SetEvent(loaderDone);
    });
    auto waiting=simple();
    waiting.invoke=[] {
        ++invoked; SetEvent(loaderStart);
        check(WaitForSingleObject(loaderDone,5000)==WAIT_OBJECT_0,
            "native invoke holds no reader lock needed by loader callbacks");
    };
    check(ffx::deferNativeAction(std::move(waiting)) && ffx::deferNativeAction(simple()),"queue load-crossing and stale actions");
    boundary(); loader.join();
    check(resets==1 && prepared==1 && invoked==1 && completed==0 && retired==2,
        "resource generation cancels completion and queued stale preparation/invocation");
    CloseHandle(loaderStart); CloseHandle(loaderDone);

    resetCounts();
    loadBody=CreateEventW(nullptr,TRUE,FALSE,nullptr);
    releaseLoad=CreateEventW(nullptr,TRUE,FALSE,nullptr);
    check(loadBody && releaseLoad,"resource-scope events");
    std::thread loading([] { check(resourceLoad(3)==12,"resource original remains unchanged"); });
    check(WaitForSingleObject(loadBody,5000)==WAIT_OBJECT_0,"resource scope opened");
    check(ffx::deferNativeAction(simple()),"queue current-generation action during loading");
    boundary(); check(!prepared && !invoked && !completed,"active resource scope blocks action drain");
    SetEvent(releaseLoad); loading.join();
    CloseHandle(loadBody); CloseHandle(releaseLoad); loadBody=releaseLoad=nullptr;
    boundary(); check(prepared==1 && invoked==1 && completed==1,"next owner boundary resumes after loading");

    resetCounts();
    auto throwing=simple();
    throwing.invoke=[] { ++invoked; throwNative=true; (void)nativeInit(9); };
    throwing.complete=[](bool ok) { check(!ok,"native exception reports unsuccessful completion"); ++failures; throwNative=false; };
    check(ffx::deferNativeAction(std::move(throwing)) && ffx::deferNativeAction(simple()),
        "queue throwing native original and already-pending recovery");
    boundary();
    check(invoked==1 && failures==1 && !initialized,"native exception contained without fake initialization callback");
    check(completed==0,"unwound native frame defers remaining jobs until next owner entry");
    boundary();
    check(invoked==2 && completed==1 && retired==2,"next adapter entry repairs unwind and action dispatch recovers and retires both actions");

    resetCounts();
    auto refused=simple();
    refused.prepare=[] { ++prepared; return false; };
    refused.complete=[](bool ok) { check(!ok,"failed preparation completes as failure"); ++failures; };
    check(ffx::deferNativeAction(std::move(refused)) && ffx::deferNativeAction(simple()),"queue refused and valid actions");
    boundary();
    check(prepared==2 && invoked==1 && completed==1 && failures==1 && retired==2,"refused preparation skips invoke without blocking later action and retires both actions");
    ffx::discardHooks();
    std::puts("PASS: real-adapter native actions, owner/tail boundaries, nested initialization, loader concurrency, stale cancellation and exception recovery.");
}
