#include "external_fixture.hpp"
#include "../native_worker_index.hpp"
#include <cstdio>
#include <cstdlib>
#include <vector>
#include <chrono>
#include <atomic>
#include "native_worker_getter_bytes.inc"
namespace {
using namespace ffx;
std::array<uint8_t,0x238> controller{};
std::vector<uint8_t> bodies;
uintptr_t currentController=reinterpret_cast<uintptr_t>(controller.data()),cachedWorker=0,code=0;
unsigned errorState=0,cachedController=0,cachedIndex=0,epoch=1,getCalls=0,invalidateAt=0;
thread_local bool observing=false;
std::atomic<unsigned> allocations{0},frees{0};
void check(bool value,const char* reason) {if(!value) {std::fprintf(stderr,"FAIL: %s\n",reason);std::exit(1);}}
template<class T> void put(uintptr_t p,T value) {std::memcpy(reinterpret_cast<void*>(p),&value,sizeof(value));}
uintptr_t global(unsigned address) {
    switch(address) {
    case 0x1326ae8:return reinterpret_cast<uintptr_t>(&currentController);
    case 0x1327098:return reinterpret_cast<uintptr_t>(&errorState);
    case 0x132709c:return reinterpret_cast<uintptr_t>(&cachedController);
    case 0x13270a0:return reinterpret_cast<uintptr_t>(&cachedIndex);
    case 0x13270a4:return reinterpret_cast<uintptr_t>(&cachedWorker);
    default:check(false,"unqualified archived getter operand");return 0;
    }
}
uintptr_t __cdecl get(int index) {
    ++getCalls;if(invalidateAt && getCalls==invalidateAt) invalidateNativeWorkerIndex();
    return reinterpret_cast<uintptr_t(__cdecl*)(int)>(code)(index);
}
void initialize() {
    bodies.resize(500*0xb58);
    put<uint16_t>(currentController+0xc,500);put<uint16_t>(currentController+0x10,500);
    put<uintptr_t>(currentController+0x1c,reinterpret_cast<uintptr_t>(bodies.data()));
    put<uint16_t>(currentController+0x1a,17);
    for(unsigned i=0;i<500;++i) {
        const auto w=reinterpret_cast<uintptr_t>(bodies.data())+i*0xb58;
        put<uintptr_t>(w+4,0x12345600);put<uint16_t>(w+0x2e,static_cast<uint16_t>((i*17+13)%500));
    }
    code=reinterpret_cast<uintptr_t>(VirtualAlloc(nullptr,sizeof(archivedWorkerGetter),MEM_COMMIT|MEM_RESERVE,PAGE_READWRITE));check(code!=0,"controlled getter code allocation");
    std::memcpy(reinterpret_cast<void*>(code),archivedWorkerGetter,sizeof(archivedWorkerGetter));
    for(const auto& relocation:workerGetterGlobals) put<uintptr_t>(code+relocation.offset,global(relocation.address));
    DWORD prior;check(VirtualProtect(reinterpret_cast<void*>(code),sizeof(archivedWorkerGetter),PAGE_EXECUTE_READ,&prior)!=0,"controlled getter protection");
    FlushInstructionCache(GetCurrentProcess(),reinterpret_cast<void*>(code),sizeof(archivedWorkerGetter));
}
}
void* operator new(size_t size) {if(observing) ++allocations;auto* p=std::malloc(size?size:1);if(!p) throw std::bad_alloc();return p;}
void operator delete(void* p) noexcept {if(p && observing) ++frees;std::free(p);}
void operator delete(void* p,size_t) noexcept {::operator delete(p);}
namespace ffx {
unsigned generation() {return epoch;}
uintptr_t address(unsigned rva) {
    if(rva==0x46a7c0) return reinterpret_cast<uintptr_t>(&get);
    if(rva==0xf26ae8) return reinterpret_cast<uintptr_t>(&currentController);
    check(false,"unexpected native index address");return 0;
}
bool copy(uintptr_t source,void* out,size_t size) {__try {std::memcpy(out,reinterpret_cast<void*>(source),size);return true;}__except(1) {return false;}}
}
int main() {
    using namespace ffx;
    initialize();NativeWorkerIdentity identity;
    observing=true;
    const auto coldStart=std::chrono::steady_clock::now();
    check(nativeWorkerIdentity(currentController,0x12345600,100,identity),"cold copied identity registry");
    const auto cold=std::chrono::duration<double,std::micro>(std::chrono::steady_clock::now()-coldStart).count();
    check(identity.global==211 && identity.worker==reinterpret_cast<uintptr_t>(bodies.data())+211*0xb58 && getCalls==501,"actual archived getter preserves partial local/global mapping");
    const auto warmStart=std::chrono::steady_clock::now();const auto before=getCalls;
    for(unsigned i=0;i<256;++i) check(nativeWorkerIdentity(currentController,0x12345600,100,identity),"warm indexed identity");
    const auto warm=std::chrono::duration<double,std::micro>(std::chrono::steady_clock::now()-warmStart).count();
    check(getCalls==before+256 && !errorState,"exactly one native getter per warm query, native error counters unchanged");
    check(cachedWorker==identity.worker && static_cast<uint16_t>(cachedIndex)==211,"native getter cache ends at returned owner");
    const auto missingBefore=getCalls;check(!nativeWorkerIdentity(currentController,0x12345600,999,identity),"missing local identity");
    check(getCalls==missingBefore,"missing row incurs no passive rescan");
    put<uint16_t>(reinterpret_cast<uintptr_t>(bodies.data())+7*0xb58+0x2e,999);invalidateNativeWorkerIndex();
    check(nativeWorkerIdentity(currentController,0x12345600,999,identity) && identity.global==7,"native construction revision recollects same-count ready row");
    ++epoch;invalidateAt=getCalls+250;
    check(!nativeWorkerIdentity(currentController,0x12345600,100,identity),"overlapping native construction rejects incomplete copied registry");
    invalidateAt=0;check(nativeWorkerIdentity(currentController,0x12345600,100,identity),"next owned attempt after revision succeeds");
    put<uintptr_t>(reinterpret_cast<uintptr_t>(bodies.data())+211*0xb58+4,0x99990000);
    check(!nativeWorkerIdentity(currentController,0x12345600,100,identity),"warm raw identity mismatch rejects old chunk");
    const auto boundedBefore=getCalls;put<uint16_t>(currentController+0xc,501);
    check(!nativeWorkerIdentity(currentController,0x12345600,100,identity) && getCalls==boundedBefore,"native500 work bound rejects without getters");
    put<uint16_t>(currentController+0xc,500);
    check(!nativeWorkerIdentity(currentController,0x12345600,1024,identity) && getCalls==boundedBefore,"1024 local bound rejects without getters");
    observing=false;check(!allocations && !frees,"Native C++ heap0/0 cold/warm/cancel/guard paths");
    std::printf("PASS: actual retained86A7C0 getter in controlled host, first local/global mapping, warm one-getter lookup, construction/generation/identity/500/1024 guards; Native heap0/0. Cold500 collection %.3f us;256 warm queries %.3f us. Controlled facts, component only.\n",cold,warm);
    VirtualFree(reinterpret_cast<void*>(code),0,MEM_RELEASE);
}
