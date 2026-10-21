// Real production hook adapter + MinHook, on this host's own functions only.
// Measure the added globally hot getter while assistance is Off; native ABI,
// return-pointer preservation and nested interpreter ownership are included.
#include "../events.hpp"
#include "../sin_collection.hpp"
#include "../utility_memory.hpp"
#include "../settings.hpp"
#include <MinHook.h>
#include <chrono>
#include <cstdio>
#include <cstdlib>
#include <cstring>
namespace {
float value=25;unsigned unexpectedReads=0,baselineTicks=0,baselineTicksOther=0;uintptr_t sink=0;
__declspec(noinline) uintptr_t __cdecl nativeGetter(uintptr_t worker,int variable,int element) {
    volatile auto arguments=worker+variable+element;(void)arguments;return reinterpret_cast<uintptr_t>(&value);
}
__declspec(noinline) void __cdecl interpreter(uintptr_t worker,unsigned count) {
    for(unsigned i=0;i<count;++i) sink=nativeGetter(worker,static_cast<int>(i&7),0);
}
void empty(ffx::Event&) {++baselineTicks;}
void emptyOther(ffx::Event&) {baselineTicksOther+=2;}
void check(bool okay,const char* message) {if(!okay) {std::fprintf(stderr,"FAIL: %s\n",message);std::exit(1);}}
void measure(const char* label) {
    for(unsigned count:{100u,1000u,10000u}) {
        double first=0,total=0,peak=0;
        for(unsigned run=0;run<6;++run) {
            const auto began=std::chrono::steady_clock::now();interpreter(0x12345678,count);
            const auto cost=std::chrono::duration<double,std::micro>(std::chrono::steady_clock::now()-began).count();
            if(!run) first=cost;else {total+=cost;if(cost>peak)peak=cost;}
            check(sink==reinterpret_cast<uintptr_t>(&value),"actual hooked Native pointer return is preserved");
        }
        std::printf("METRIC actual Native adapter %s getters=%u first=%.3fus warm5mean=%.3fus max=%.3fus.\n",label,count,first,total/5,peak);
    }
}
}
namespace ffx {
int setting(Setting) {return 0;} // Controlled default-Off settings dependency.
void setSetting(Setting,int) {}
std::atomic<int> scene{0};
uintptr_t address(unsigned rva) {
    if(rva==0x464180) return reinterpret_cast<uintptr_t>(&interpreter);
    if(rva==0x46c500) return reinterpret_cast<uintptr_t>(&nativeGetter);
    return rva;
}
bool copy(uintptr_t from,void* to,size_t size) {__try {std::memcpy(to,reinterpret_cast<void*>(from),size);return true;}__except(1) {return false;}}
unsigned generation() {return 1;}
void log(const char*,...) {}
// Off must return before entering any of these qualification dependencies.
bool captureUtilityScript(UtilityScript&) noexcept {++unexpectedReads;return false;}
bool captureUtilityWorker(const UtilityScript&,unsigned,uintptr_t&) noexcept {++unexpectedReads;return false;}
bool captureUtilityValue(const UtilityScript&,unsigned,unsigned,unsigned,int&) noexcept {++unexpectedReads;return false;}
bool readyAtelIdentity(uintptr_t,AtelIdentity&) {++unexpectedReads;return false;}
bool readyAtelWorkerHeader(uintptr_t,unsigned,unsigned&) {++unexpectedReads;return false;}
bool readyAtelDeclarationAt(uintptr_t,unsigned,unsigned,unsigned&,unsigned&) {++unexpectedReads;return false;}
bool nativeAtelSpan(uintptr_t,AtelSpan&) {++unexpectedReads;return false;}
bool inBattle() {return false;}bool gameFocused() {return true;}bool modMenuActive() {return false;}bool utilityConsoleActive() {return false;}
}
int main() {
    check(ffx::observeGameThread(),"actual Native owner bootstrap");check(MH_Initialize()==MH_OK,"existing MinHook initializes in test host");
    // Jecht/Lightning already observe this production site. Keep two baseline
    // listeners when comparing the incremental Sin observer/getter hook.
    check(ffx::hook(0x464180,empty,empty) && ffx::hook(0x464180,emptyOther,emptyOther) && ffx::enableHooks(),"baseline interpreter listeners enabled");
    measure("baseline two interpreter listeners");ffx::installSinCollection();check(ffx::applyAddedHooks(),"collection hooks enabled in own host");
    measure("collection Off");check(!unexpectedReads,"Off never reads script, resource or game state");
    ffx::resetSinCollection();interpreter(0x12345678,100);check(!unexpectedReads,"reset remains Off");
    ffx::discardHooks();MH_Uninitialize();
    std::puts("PASS: actual hook adapter default-Off fast guard, nested interpreter scope and pointer ABI. Controlled host only; game call frequency, frame impact and audible latency unmeasured.");
}
