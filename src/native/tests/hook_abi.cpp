#include "../events.hpp"
#include <MinHook.h>
#include <cstdio>
#include <stdexcept>
#include <xmmintrin.h>

namespace ffx {
uintptr_t address(unsigned rva) { return rva; }
bool copy(uintptr_t p, void* out, size_t length) { __try { memcpy(out, reinterpret_cast<void*>(p), length); return true; } __except(1) { return false; } }
unsigned epoch = 1;
unsigned generation() { return epoch; }
void log(const char*, ...) {}
}
static unsigned entered = 0, left = 0;
static bool invalidateEntry = false, throwEntry = false;
static bool overrideReturn=false,seenParent=false;
static bool seenReadyEdi=false;
static void before(ffx::Event& event) {
    ++entered; _mm_setcsr(0x1f80);
    if(event.parentRva) seenParent=true;
    if(event.rva==0x111) seenReadyEdi=event.edi==0x76543210 && event.nativeStack!=0 && event.caller!=0;
    if (invalidateEntry) ++ffx::epoch;
    if (throwEntry) throw std::runtime_error("fixture reader exception");
    SetLastError(ERROR_INVALID_DATA);
}
static void after(ffx::Event& event) { ++left; _mm_setcsr(0x1f80); if(overrideReturn) { event.overrideResult=true; event.result=27; } SetLastError(ERROR_ACCESS_DENIED); }
static volatile unsigned registerValue=0;
__declspec(noinline) void __cdecl readyRegisterTarget() {registerValue=registerValue+1;}
__declspec(naked) void __cdecl readyRegisterCaller() {
    __asm {push edi
        mov edi,076543210h
        call readyRegisterTarget
        pop edi
        ret}
}
static DWORD errorOnEntry=0;
__declspec(noinline) int __stdcall errorCall() {
    errorOnEntry=GetLastError(); SetLastError(ERROR_BAD_EXE_FORMAT); return 0;
}
__declspec(noinline) int __cdecl cdeclCall(int a, int b, int c) { volatile int sum = a + b; return sum * c; }
__declspec(noinline) int __stdcall stdcallCall(int a, int b, int c) { volatile int sum = a - b; return sum * c; }
__declspec(noinline) int __stdcall importedCall(int a,int b,int c) { volatile int sum=a-b; return sum*c; }
__declspec(noinline) int __fastcall fastcallCall(int a, int b, int c, int d) { volatile int sum = a + 2 * b + 3 * c; return sum + 4 * d; }
__declspec(noinline) double __cdecl floatCall(double x, double y) { volatile double value = x * y; return value + .25; }
__declspec(noinline) int recursiveCall(int n) { volatile int value = n; return value ? value + recursiveCall(value - 1) : 0; }
__declspec(noinline) int exceptionCall(int value) {
    volatile int mode = value;
    if (mode) throw std::runtime_error("fixture original exception");
    return 17;
}
__declspec(noinline) unsigned __int64 __stdcall wideCall(unsigned a, unsigned b) {
    volatile unsigned high = a;
    return (static_cast<unsigned __int64>(high) << 32) | b;
}
struct Object { int value;
    __declspec(noinline) int method(int x, int y) { volatile int sum = value + x; return sum * y; }
};
static void require(bool condition, const char* message) { if (!condition) throw std::runtime_error(message); }
int main() {
    if(!ffx::observeGameThread()) return 1;
    try {
        require(MH_Initialize() == MH_OK, "MinHook initialization");
        Object object{7};
        int(Object::*member)(int,int) = &Object::method;
        uintptr_t method = 0; memcpy(&method, &member, sizeof(method));
        for (auto target : {reinterpret_cast<uintptr_t>(cdeclCall), reinterpret_cast<uintptr_t>(stdcallCall),
            reinterpret_cast<uintptr_t>(fastcallCall), reinterpret_cast<uintptr_t>(floatCall),
            reinterpret_cast<uintptr_t>(recursiveCall), method})
            require(ffx::hookAddress(target, 1, before, after), "hook creation");
        require(ffx::enableHooks(), "enable batch");
        _mm_setcsr(0x3f80);
        for (unsigned i = 0; i < 1000; ++i) {
            require(cdeclCall(2,3,4) == 20, "cdecl arguments/return");
            require(stdcallCall(8,3,4) == 20, "stdcall cleanup/return");
            require(fastcallCall(1,2,3,4) == 30, "fastcall register arguments/stack");
            require(object.method(3,4) == 40, "thiscall ECX/cleanup");
            require(floatCall(1.5,2) == 3.25, "x87 floating return");
            require(recursiveCall(5) == 15, "nested hook return stack");
            require(_mm_getcsr() == 0x3f80, "SSE state restoration");
        }
        require(entered == 11000 && left == entered, "entry/return count");
        require(ffx::hookAddress(reinterpret_cast<uintptr_t>(errorCall),4,before,after),"native Windows error-state hook");
        require(ffx::applyAddedHooks(),"enable native Windows error-state hook");
        SetLastError(ERROR_FILE_NOT_FOUND);
        require(errorCall()==0 && errorOnEntry==ERROR_FILE_NOT_FOUND,"reader entry must preserve native Windows error state");
        require(GetLastError()==ERROR_BAD_EXE_FORMAT,"reader return must preserve original Windows failure code");
        require(seenParent,"nested original call retains parent hook identity");
        overrideReturn=true; require(cdeclCall(2,3,4)==27,"explicit return override reaches native caller"); overrideReturn=false;
        require(ffx::hookAddress(reinterpret_cast<uintptr_t>(exceptionCall),2,before,after), "exception hook");
        require(ffx::hookAddress(reinterpret_cast<uintptr_t>(wideCall),3,before,after), "wide return hook");
        require(ffx::applyAddedHooks(), "runtime added-hook batch");
        require(wideCall(0x12345678,0xabcdef01)==0x12345678abcdef01ull, "EDX:EAX 64-bit return");
        bool caught=false;
        try { exceptionCall(1); } catch(const std::runtime_error&) { caught=true; }
        require(caught && cdeclCall(2,3,4)==20 && exceptionCall(0)==17, "original exception unwind and context recovery");
        throwEntry=true; require(cdeclCall(2,3,4)==20, "reader exception preserves original"); throwEntry=false;
        const auto beforeLeft=left;
        invalidateEntry=true; require(cdeclCall(2,3,4)==20, "generation entry preserves original"); invalidateEntry=false;
        require(left==beforeLeft, "stale post-call event suppressed");
        require(cdeclCall(2,3,4)==20 && left==beforeLeft+1, "new generation post-call event");
        // FFX's FMOD calls pass through adjacent six-byte FF 25 import thunks.
        // Exercise that exact shape and stdcall cleanup through the adapter.
        auto thunks=static_cast<uint8_t*>(VirtualAlloc(nullptr,64,MEM_COMMIT|MEM_RESERVE,PAGE_READWRITE));
        require(thunks!=nullptr,"import thunk allocation"); memset(thunks,0x90,64);
        const auto slot=reinterpret_cast<uintptr_t>(thunks+32),callee=reinterpret_cast<uintptr_t>(importedCall);
        memcpy(thunks+32,&callee,4);
        for(unsigned offset:{0u,6u,12u}) { thunks[offset]=0xff; thunks[offset+1]=0x25; memcpy(thunks+offset+2,&slot,4); }
        DWORD protection; require(VirtualProtect(thunks,64,PAGE_EXECUTE_READ,&protection)!=0,"import thunk protection");
        FlushInstructionCache(GetCurrentProcess(),thunks,64);
        using Import=int(__stdcall*)(int,int,int);
        require(ffx::hookAddress(reinterpret_cast<uintptr_t>(thunks),20,before,after),"voice start import thunk hook");
        require(ffx::hookAddress(reinterpret_cast<uintptr_t>(thunks+12),21,before,after),"voice state import thunk hook");
        require(ffx::applyAddedHooks(),"import thunk hook batch");
        for(unsigned offset:{0u,6u,12u})
            require(reinterpret_cast<Import>(thunks+offset)(8,3,4)==20,"adjacent hooked/unhooked import arguments and cleanup");
        require(ffx::hookAddress(reinterpret_cast<uintptr_t>(readyRegisterTarget),0x111,before,after),"ready native register hook");
        require(ffx::applyAddedHooks(),"ready native register hook batch");
        readyRegisterCaller();
        require(seenReadyEdi && registerValue==1,"native EDI/logical size and stack identity copied without changing original");
        ffx::discardHooks();
        VirtualFree(thunks,0,MEM_RELEASE);
        puts("Native x86 hook ABI passed: 11000 mixed calls, Windows error state, runtime hooks, EDX:EAX, original/reader exceptions, generation, x87/SSE.");
        return 0;
    } catch (const std::exception& e) { fprintf(stderr, "%s\n", e.what()); return 1; }
}
