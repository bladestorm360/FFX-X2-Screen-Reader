#include "external_fixture.hpp"
// Actual utility memory consumer and resource owner, retained EV data, controlled
// affine getters. No game, SDK, registry, or installer operations.
#include "../utility_memory.hpp"
#include "../utility_context.hpp"
#include "../utilities.hpp"
#include "../atel_resources.hpp"
#include "../events.hpp"
#ifdef FFX_NAVIGATION_REAL_QUEUE_TEST
#include "../speech_queue.hpp"
#endif
#include "../utility_trials.hpp"
#include "../utility_trials_data.hpp"
#include <fstream>
#include <map>
#include <chrono>
#include <cstdio>
#include <cstring>
#include <cstdlib>
#include <cstdarg>
namespace {
using namespace ffx;
struct Listener {Handler before,after;};std::map<unsigned,Listener> listeners;
std::vector<uint8_t> ev;std::array<uint8_t,0x220> controller{};
std::vector<std::array<uint8_t,0xb58>> workers;
unsigned epoch=1,scalar=42,getterCalls=0,workerCalls=0,codeCopies=0,foreignReads=0,allocations=0,frees=0;
#ifdef FFX_NAVIGATION_REAL_QUEUE_TEST
thread_local bool nativeOwner=false,observe=false,utilityReads=false;
#else
bool nativeOwner=true,observe=false,utilityReads=false;
#endif
uintptr_t chunk=0,code=0;unsigned codeLength=0;
void check(bool ok,const char* why) {if(!ok) {std::fprintf(stderr,"FAIL: %s\n",why);std::exit(1);}}
template<class T> T word(uintptr_t p) {T v;std::memcpy(&v,reinterpret_cast<const void*>(p),sizeof(v));return v;}
template<class T> void put(void* p,T v) {std::memcpy(p,&v,sizeof(v));}
uintptr_t __cdecl workerGetter(int index) {
    if(!nativeOwner) {++foreignReads;return 0;}++workerCalls;
    return index>=0 && static_cast<unsigned>(index)<workers.size()?reinterpret_cast<uintptr_t>(workers[index].data()):0;
}
uintptr_t __cdecl valueGetter(uintptr_t,int,int) {
    if(!nativeOwner) {++foreignReads;return 0;}++getterCalls;
    return reinterpret_cast<uintptr_t>(&scalar);
}
void prepare(const char* name,bool drain=true,bool changed=false) {
    if(!ev.empty()) {Event released;released.args[0]=reinterpret_cast<uintptr_t>(ev.data());listeners.at(0x48d490).before(released);}
    const std::string folder(name,2);
    std::ifstream file((ffx_fixture::evidenceRoot()/"offline-output/FFX/20261003_audit/files/ffx_ps2/ffx/master/jppc/event/obj").string()+"/"+folder+"/"+name+"/"+name+".ebp",std::ios::binary);
    check(file.good(),"retained full EV");ev.assign(std::istreambuf_iterator<char>(file),{});
    const auto parent=reinterpret_cast<uintptr_t>(ev.data());chunk=parent+word<unsigned>(parent+4);
    code=chunk+word<unsigned>(chunk+0x30);codeLength=word<unsigned>(chunk);
    if(changed) *reinterpret_cast<uint8_t*>(code)^=1;
    ++epoch;controller={};
    const unsigned count=word<uint16_t>(chunk+0x34);check(count && count<=500,"retained field worker count");
    workers.clear();workers.resize(count);
    for(unsigned i=0;i<count;++i) {
        put(workers[i].data(),chunk+word<unsigned>(chunk+0x38+i*4));
        put(workers[i].data()+4,chunk);put(workers[i].data()+0x2e,static_cast<uint16_t>(i));
    }
    put(controller.data()+0xc,static_cast<uint16_t>(count));
    Event load;load.args[0]=12;load.nativeStack=0x910000;load.epoch=epoch;listeners.at(0x48d230).before(load);
    Event allocation;allocation.caller=0x48d2a4;allocation.edi=ev.size();allocation.args[0]=(allocation.edi+0x7ffu)&~0x7ffu;
    allocation.epoch=epoch;allocation.result=parent;
    observe=true;listeners.at(0x2871c0).before(allocation);listeners.at(0x2871c0).after(allocation);
    load.result=parent;listeners.at(0x48d230).after(load);observe=false;
    if(drain) {nativeOwner=false;drainAtelResourcesForTest();nativeOwner=true;}
}
}
void* operator new(size_t size) {if(observe) ++allocations;auto* p=std::malloc(size?size:1);if(!p) throw std::bad_alloc();return p;}
void operator delete(void* p) noexcept {if(p && observe) ++frees;std::free(p);}
void operator delete(void* p,size_t) noexcept {::operator delete(p);}
namespace ffx {
unsigned generation() {
#ifdef FFX_NAVIGATION_REAL_QUEUE_TEST
    return speechq::generation();
#else
    return epoch;
#endif
}
bool nativeGameThread() {return nativeOwner;}
bool copy(uintptr_t source,void* target,size_t size) {
    if(!nativeOwner) {++foreignReads;return false;}
    if(source<0x10000 || size>0x10000) return false;
    if(utilityReads && source>=code && source-code<codeLength) ++codeCopies;
    __try {std::memcpy(target,reinterpret_cast<const void*>(source),size);return true;} __except(EXCEPTION_EXECUTE_HANDLER) {return false;}
}
uintptr_t address(unsigned rva) {
    if(rva==0xf26ae8) {static uintptr_t p;p=reinterpret_cast<uintptr_t>(controller.data());return reinterpret_cast<uintptr_t>(&p);}
    if(rva==0x46a7c0) return reinterpret_cast<uintptr_t>(&workerGetter);
    if(rva==0x46c500) return reinterpret_cast<uintptr_t>(&valueGetter);
    return rva;
}
bool hookSnapshot(unsigned rva,Handler before,Handler after) {listeners[rva]={before,after};return true;}
#ifdef FFX_NAVIGATION_REAL_QUEUE_TEST
void log(const char* format,...) {va_list arguments;va_start(arguments,format);vfprintf(stderr,format,arguments);fputc('\n',stderr);va_end(arguments);}
#else
void log(const char*,...) {}
#endif
UtilityContext utilityContext() {return {};}
bool utilityContextValid(const UtilityContext&,bool) {return false;}
bool utilityActionAllowed(bool) {return false;}
bool deferNativeAction(NativeAction) {return false;}
void utilityReply(const wchar_t*) {}
}
int fixtureMain() {
    using namespace ffx;
    check(startAtelResources(),"resource bootstrap");installAtelResources();
    double totalUs=0,maxUs=0;
    for(const auto& trial:trialUtilities) {
        prepare(trial.script);
        utilityReads=observe=true;const auto start=std::chrono::steady_clock::now();
        const auto script=utilityScript();
        UtilityScript candidate;uintptr_t capturedWorker=0;
        check(captureUtilityScript(candidate) && candidate.name==script.name && candidate.chunk==script.chunk && candidate.allocationSerial==script.allocationSerial,
            "bounded Native script candidate preserves real field identity");
        check(captureUtilityWorker(candidate,trial.treasure.worker,capturedWorker) && capturedWorker==utilityWorker(script,trial.treasure.worker),
            "bounded Native worker candidate preserves indexed ownership");
        check(scriptChecksum(script)==trial.checksum,"actual retained trial prepared checksum");
        check(utilityWorker(script,trial.treasure.worker)==workerGetter(static_cast<int>(trial.treasure.worker)),"actual owned worker header and native index");
        check(utilityValue(script,trial.ready.low,trial.ready.high,trial.ready.element)==42,"actual trial readiness declaration routes through affine getter");
        int captured=0;
        check(captureUtilityValue(script,trial.ready.low,trial.ready.high,trial.ready.element,captured) && captured==42,"bounded primitive capture preserves the actual readiness getter");
        const auto elapsed=std::chrono::duration<double,std::micro>(std::chrono::steady_clock::now()-start).count();
        totalUs+=elapsed;maxUs=(std::max)(maxUs,elapsed);observe=utilityReads=false;
        const auto rowOffset=word<unsigned>(script.header+0x14);
        unsigned row=0;check(readyAtelDeclaration(chunk,static_cast<unsigned>(script.header-chunk),trial.ready.low,trial.ready.high,row),"actual owned trial readiness row");
        const auto at=chunk+rowOffset+row*8;const unsigned old=word<unsigned>(at);put(reinterpret_cast<void*>(at),old^1u);
        const auto gettersBefore=getterCalls;observe=utilityReads=true;
        check(!captureUtilityValue(script,trial.ready.low,trial.ready.high,trial.ready.element,captured),"changed row primitive refuses before getter and without allocated exception");
        observe=utilityReads=false;check(getterCalls==gettersBefore,"failed primitive never enters the affine getter");
        bool refused=false;try {utilityValue(script,trial.ready.low,trial.ready.high,trial.ready.element);} catch(...) {refused=true;}
        check(refused,"changed live declaration refuses prepared index before getter");put(reinterpret_cast<void*>(at),old);
        ++epoch;check(scriptChecksum(script)==trial.checksum,"graphics epoch retains still-live physical resource");
        nativeOwner=false;
        check(!captureUtilityScript(candidate) && !captureUtilityWorker(script,0,capturedWorker),"Worker cannot capture affine script/getter identities");
        check(!captureUtilityValue(script,trial.ready.low,trial.ready.high,trial.ready.element,captured),"Worker cannot enter bounded native utility capture");nativeOwner=true;
    }
    put(controller.data()+0x1a,static_cast<uint16_t>(3));UtilityScript denied;const auto beforeWrongOwner=workerCalls;
    observe=true;check(!captureUtilityScript(denied) && utilityPreparation()==AtelPreparation::Missing,"segmented battle controller is not an owned field source");observe=false;
    check(workerCalls==beforeWrongOwner,"wrong controller refuses before worker getter");put(controller.data()+0x1a,static_cast<uint16_t>(0));
    prepare("bltz0200");auto lobby=utilityScript();check(scriptChecksum(lobby)==0x58a5c0a5,"actual large Blitz lobby identity");
    prepare("bvyt0900",false);auto pending=utilityScript();bool held=false;
    try {scriptChecksum(pending);} catch(...) {held=true;}
    check(held && atelPreparation(chunk)==AtelPreparation::Pending,"pending utility identity never hashes native code");
    nativeOwner=false;drainAtelResourcesForTest();nativeOwner=true;
    check(scriptChecksum(pending)==0x97fd6083,"owned Via preparation becomes ready");
    prepare("bvyt0900",true,true);check(scriptChecksum(utilityScript())!=0x97fd6083,"newly loaded changed code is not accepted as old identity");
    check(!codeCopies && !foreignReads && !allocations && !frees,"successful Native consumer heap0/0, code copies0; resource Worker engine reads0");
    std::printf("PASS: actual utility consumer + real owned EV service; six trials/lobby/Via, pending/no native hash, live declaration and graphics lifetime guards.\nMETRIC six cold Native observations total=%.3fus max=%.3fus; getter=%u worker=%u. Controlled adapters, no audible/gameplay/whole-mod claim.\n",totalUs,maxUs,getterCalls,workerCalls);
    return 0;
}
int main() {try {return fixtureMain();} catch(const std::exception& error) {observe=false;std::fprintf(stderr,"FAIL: utility fixture exception: %s\n",error.what());return 1;}}
