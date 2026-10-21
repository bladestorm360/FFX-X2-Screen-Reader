#include "../events.hpp"
#include <MinHook.h>
#include <cstdio>
#include <cstdlib>
#include <stdexcept>

namespace ffx {
uintptr_t address(unsigned rva) { return rva; }
bool copy(uintptr_t p,void* out,size_t length) {
    __try { memcpy(out,reinterpret_cast<void*>(p),length); return true; }
    __except(1) { return false; }
}
unsigned epoch=1;
unsigned generation() { return epoch; }
void log(const char*,...) {}
}
namespace {
unsigned entered=0,left=0;
bool throws=false;
std::vector<unsigned> returned;
void check(bool ok,const char* why) {
    if(!ok) { std::fprintf(stderr,"FAIL: %s\n",why); std::exit(1); }
}
void before(ffx::Event& e) {
    ++entered;
    const unsigned parent=e.rva==10?0:e.rva==11?10:11;
    // Fail safely before executing the invalid return chain in the old adapter.
    check(e.parentRva==parent,"tail jump must retain the outer pending return record");
    check(e.args[0]==(e.rva==10?7u:12u),"tail jump snapshots its own rewritten native arguments");
}
void after(ffx::Event& e) { ++left; returned.push_back(e.rva); }
__declspec(noinline) int __cdecl pageBody(int value) {
    volatile int result=value;
    if(throws) throw std::runtime_error("native tail unwind");
    return result*3;
}
__declspec(naked) int __cdecl pageInit(int) {
    __asm {
        push ebp
        mov ebp,esp
        pop ebp
        jmp pageBody
    }
}
// Same rewritten stack argument and indirect-tail-dispatch shape as TK close.
__declspec(naked) int __cdecl dialogueClose(int) {
    __asm {
        push ebp
        mov ebp,esp
        mov eax,[ebp+8]
        add eax,5
        mov [ebp+8],eax
        pop ebp
        jmp pageInit
    }
}
}
int main() {
    if(!ffx::observeGameThread()) return 1;
    SetErrorMode(SEM_FAILCRITICALERRORS|SEM_NOGPFAULTERRORBOX);
    check(MH_Initialize()==MH_OK,"initialize hooks");
    check(ffx::hookAddress(reinterpret_cast<uintptr_t>(dialogueClose),10,before,after),"close hook");
    check(ffx::hookAddress(reinterpret_cast<uintptr_t>(pageInit),11,before,after),"page hook");
    check(ffx::hookAddress(reinterpret_cast<uintptr_t>(pageBody),12,before,after),"body hook");
    check(ffx::enableHooks(),"enable hooks");
    for(unsigned i=0;i<1000;++i) {
        returned.clear();
        check(dialogueClose(7)==36,"native tail-chain return and caller cleanup");
        check(returned==std::vector<unsigned>{12,11,10},"all pending tail returns run in native order");
    }
    check(entered==3000 && left==3000,"tail chains retain every native event");
    throws=true; bool caught=false;
    try { dialogueClose(7); } catch(const std::runtime_error&) { caught=true; }
    throws=false;
    check(caught && dialogueClose(7)==36,"exception cleanup discards unwound tail records");
    check(entered==3006 && left==3003,"exception recovery preserves the next tail chain");
    ffx::discardHooks();
    std::puts("PASS: rewritten native tail chains, complete return ordering, unchanged results and exception recovery.");
}
