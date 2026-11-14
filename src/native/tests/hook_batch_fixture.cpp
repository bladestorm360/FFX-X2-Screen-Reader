#include "../events.hpp"
#include "../hook_calls.hpp"
#include "../resource_reset.hpp"
#include <MinHook.h>
#include <cstdio>
#include <cstdlib>

namespace ffx {
uintptr_t address(unsigned rva) { return rva; }
bool copy(uintptr_t p,void* out,size_t length) {
    __try { memcpy(out,reinterpret_cast<void*>(p),length); return true; }
    __except(1) { return false; }
}
unsigned generation() { return 1; }
void log(const char*,...) {}
}
namespace {
// 0x46fed0 is a resource-loading call. Readers rely on it to pause during
// loads, so the batch must still refuse to run without it.
constexpr unsigned ResourceRva=0x46fed0,OrdinaryRva=20,BadRva=21;
unsigned entered=0;
void check(bool ok,const char* why) {
    if(!ok) { std::fprintf(stderr,"FAIL: %s\n",why); std::exit(1); }
}
// Distinct bodies keep the linker from folding the listeners into one.
template<unsigned N> void listener(ffx::Event&) { entered|=1u<<N; }
__declspec(noinline) int __cdecl ping(int value) { volatile int result=value; return result+1; }
uintptr_t uncommitted() {
    // Reserved but never committed: the same check a stale or wrong address fails.
    return reinterpret_cast<uintptr_t>(VirtualAlloc(nullptr,0x1000,MEM_RESERVE,PAGE_NOACCESS));
}
}
int main() {
    if(!ffx::observeGameThread()) return 1;
    SetErrorMode(SEM_FAILCRITICALERRORS|SEM_NOGPFAULTERRORBOX);
    check(MH_Initialize()==MH_OK,"initialize hooks");
    check(ffx::hookAddress(reinterpret_cast<uintptr_t>(ping),OrdinaryRva,listener<0>,nullptr),"ordinary hook");
    check(!ffx::hookAddress(uncommitted(),BadRva,listener<1>,nullptr),"an unavailable hook target is reported");
    check(ffx::hookAddress(reinterpret_cast<uintptr_t>(ping),OrdinaryRva,listener<1>,nullptr) &&
        ffx::hookAddress(reinterpret_cast<uintptr_t>(ping),OrdinaryRva,listener<2>,nullptr) &&
        ffx::hookAddress(reinterpret_cast<uintptr_t>(ping),OrdinaryRva,listener<3>,nullptr) &&
        ffx::hookAddress(reinterpret_cast<uintptr_t>(ping),OrdinaryRva,listener<4>,nullptr) &&
        ffx::hookAddress(reinterpret_cast<uintptr_t>(ping),OrdinaryRva,listener<5>,nullptr) &&
        ffx::hookAddress(reinterpret_cast<uintptr_t>(ping),OrdinaryRva,listener<6>,nullptr) &&
        ffx::hookAddress(reinterpret_cast<uintptr_t>(ping),OrdinaryRva,listener<7>,nullptr),"listeners fill one target");
    check(ffx::enableHooks(),"one bad target does not discard the batch");
    check(ping(1)==2 && entered==(1u<<ffx::NativeListenerLimit)-1,"the remaining hooks and listeners run");
    ffx::discardHooks();

    check(MH_Initialize()==MH_OK,"initialize second batch");
    check(!ffx::hookAddress(uncommitted(),ResourceRva,listener<0>,nullptr),"an unavailable resource scope target is reported");
    check(!ffx::enableHooks(),"a batch without its resource scope hook is refused");
    ffx::discardHooks();

    // A ninth listener on one target is a code error and still refuses the batch.
    check(MH_Initialize()==MH_OK,"initialize third batch");
    bool registered=true;
    for(unsigned i=0;i<ffx::NativeListenerLimit;++i) {
        static ffx::Handler const handlers[]={listener<0>,listener<1>,listener<2>,listener<3>,listener<4>,listener<5>,listener<6>,listener<7>};
        registered=registered && ffx::hookAddress(reinterpret_cast<uintptr_t>(ping),OrdinaryRva,handlers[i],nullptr);
    }
    check(registered && !ffx::hookAddress(reinterpret_cast<uintptr_t>(ping),OrdinaryRva,listener<8>,nullptr),"a listener past the limit is reported");
    check(!ffx::enableHooks(),"a listener past the limit refuses the batch");
    std::puts("PASS: a bad hook target is skipped; a missing resource scope hook or a listener past the limit still refuses the batch.");
}
