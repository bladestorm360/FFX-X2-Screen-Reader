#include "../events.hpp"
#include "../hook_calls.hpp"
#include "../resource_reset.hpp"
#include <MinHook.h>
#include <array>
#include <cstdio>
#include <cstdlib>
#include <thread>

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
// 0x46fed0 is a resource-loading call: readers pause while it runs.
constexpr unsigned ResourceRva=0x46fed0,OrdinaryRva=20;
bool loadingInside=false;
void check(bool ok,const char* why) {
    if(!ok) { std::fprintf(stderr,"FAIL: %s\n",why); std::exit(1); }
}
void ignore(ffx::Event&) {}
__declspec(noinline) int __cdecl load(int depth) {
    volatile int value=depth;
    if(!depth) { loadingInside=ffx::nativeResourceLoading(); return 0; }
    return load(depth-1)+value;
}
__declspec(noinline) int __cdecl ping(int value) { volatile int result=value; return result+1; }
}
int main() {
    if(!ffx::observeGameThread()) return 1;
    SetErrorMode(SEM_FAILCRITICALERRORS|SEM_NOGPFAULTERRORBOX);
    check(MH_Initialize()==MH_OK,"initialize hooks");
    check(ffx::hookAddress(reinterpret_cast<uintptr_t>(load),ResourceRva,ignore,ignore),"resource hook");
    check(ffx::hookAddress(reinterpret_cast<uintptr_t>(ping),OrdinaryRva,ignore,ignore),"ordinary hook");
    check(ffx::enableHooks(),"enable hooks");

    // Nest deeper than the tracked call stack. The innermost resource calls
    // get no return adapter, so their end must be inferred from the stack.
    const int depth=static_cast<int>(ffx::NativeCallDepth)+16;
    check(load(depth)==depth*(depth+1)/2,"untracked nested resource calls return unchanged results");
    check(loadingInside,"an untracked resource call pauses readers while it runs");
    check(!ffx::nativeResourceLoading(),"overflow clears once the untracked resource calls return");

    // Frame pool exhaustion at the outermost level: no tracked call encloses it.
    std::array<ffx::NativeCall*,4096> frames{};
    for(auto& frame:frames) frame=ffx::claimNativeCall();
    check(!ffx::claimNativeCall(),"fixture holds every native call frame");
    loadingInside=false;
    check(load(0)==0,"untracked outermost resource call returns");
    check(loadingInside,"an untracked outermost resource call pauses readers while it runs");
    for(auto* frame:frames) ffx::retireNativeCall(frame);
    check(ping(1)==2,"ordinary call after the untracked resource call");
    check(!ffx::nativeResourceLoading(),"a later hooked call at or above the untracked frame clears the overflow");
    check(load(3)==6 && !ffx::nativeResourceLoading(),"tracked resource calls still open and close their scope");

    // A worker that overflows and then exits never returns through its
    // latched frame; its exit must still release the pause.
    for(auto& frame:frames) frame=ffx::claimNativeCall();
    loadingInside=false;
    std::thread worker([] { load(0); });
    worker.join();
    for(auto* frame:frames) ffx::retireNativeCall(frame);
    check(loadingInside,"an untracked resource call on a worker pauses readers");
    check(!ffx::nativeResourceLoading(),"a worker that exits while overflowed does not keep readers paused");

    ffx::discardHooks();
    std::puts("PASS: native call overflow pauses readers only while untracked resource calls run.");
}
