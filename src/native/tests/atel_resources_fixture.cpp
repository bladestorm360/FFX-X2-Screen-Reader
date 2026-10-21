#include "external_fixture.hpp"
#include "../atel_resources.hpp"
#include "../events.hpp"
#include "../native_worker_index.hpp"
#include "../owned_atel_decoder.hpp"
#include <fstream>
#include <cstdio>
#include <cstdlib>
#include <map>
#include <thread>
#include <chrono>
#include <atomic>
namespace {
using namespace ffx;
struct Handlers {Handler before=nullptr,after=nullptr;};std::map<unsigned,Handlers> handlers;
unsigned epoch=1;thread_local bool observing=false,nativeOwner=true;
std::atomic<unsigned> allocations{0},frees{0},foreignCopies{0};
unsigned preparationSignals=0,wrongSignalOwner=0;
uintptr_t watchedReady=0,watchedPending=0;bool sawEarlyReady=false;
void prepared() noexcept {
    ++preparationSignals;if(nativeOwner) ++wrongSignalOwner;
    if(watchedReady && watchedPending && atelPreparation(watchedReady)==AtelPreparation::Ready &&
        atelPreparation(watchedPending)==AtelPreparation::Pending) sawEarlyReady=true;
}
uintptr_t allowed=0;size_t extent=0;unsigned fakeError=0;
void check(bool value,const char* reason) {if(!value) {std::fprintf(stderr,"FAIL: %s\n",reason);std::exit(1);}}
std::vector<uint8_t> asset(const char* path) {
    std::ifstream file(path,std::ios::binary);check(file.good(),"archived EV open");
    return {(std::istreambuf_iterator<char>(file)),std::istreambuf_iterator<char>()};
}
unsigned u32(const std::vector<uint8_t>& bytes,unsigned offset) {unsigned value;std::memcpy(&value,bytes.data()+offset,4);return value;}
void put(std::vector<uint8_t>& bytes,unsigned at,unsigned value) {std::memcpy(bytes.data()+at,&value,4);}
void capture(std::vector<uint8_t>& bytes,unsigned logical=0,unsigned group=12,uintptr_t caller=0x48d2a4) {
    const auto parent=reinterpret_cast<uintptr_t>(bytes.data());allowed=parent;extent=bytes.size();
    Event load;load.args[0]=group;load.nativeStack=0x900000;load.epoch=epoch;
    handlers.at(0x48d230).before(load);
    Event allocation;allocation.caller=caller;allocation.edi=logical?logical:bytes.size();
    allocation.args[0]=(allocation.edi+0x7ffu)&~0x7ffu;allocation.result=parent;allocation.epoch=epoch;
    handlers.at(0x2871c0).before(allocation);handlers.at(0x2871c0).after(allocation);
    load.result=parent;handlers.at(0x48d230).after(load);
}
void drain() {nativeOwner=false;ffx::drainAtelResourcesForTest();nativeOwner=true;}
void inspectOwned(const AtelScript& script,void* context) {
    check(!nativeOwner,"immutable script consumers run on preparation only");
    auto& result=*static_cast<std::array<unsigned,3>*>(context);
    result={script.checksum,static_cast<unsigned>(script.workers.size()),script.codeLength};
}
}
void* operator new(size_t size) {if(observing) ++allocations;auto* p=std::malloc(size?size:1);if(!p) throw std::bad_alloc();return p;}
void operator delete(void* p) noexcept {if(p && observing) ++frees;std::free(p);}
void operator delete(void* p,size_t) noexcept {::operator delete(p);}
namespace ffx {
unsigned generation() {return epoch;}
bool nativeGameThread() {return nativeOwner;}
uintptr_t address(unsigned rva) {return rva;}
bool copy(uintptr_t source,void* out,size_t size) {
    if(!nativeOwner) {++foreignCopies;return false;}
    if(size>0x10000 || source<allowed || source-allowed>extent || size>extent-(source-allowed)) return false;
    std::memcpy(out,reinterpret_cast<const void*>(source),size);return true;
}
bool hookSnapshot(unsigned rva,Handler before,Handler after) {handlers[rva]={before,after};return true;}
void log(const char*,...) {}
}
int main() {
    using namespace ffx;
    check(startAtelResources(),"bootstrap capture storage");installAtelResources();
    check(onAtelPrepared(prepared) && !onAtelPrepared(nullptr),"bounded preparation notification registration");
    const auto constructionBefore=worker_index::revision.load();Event construction;
    observing=true;handlers.at(0x46ca90).after(construction);observing=false;
    check(worker_index::revision.load()==constructionBefore+1,"constructor snapshot publishes only atomic identity revision");
    auto raw=asset((ffx_fixture::evidenceRoot()/"offline-output/FFX/20261003_audit/files/ffx_ps2/ffx/master/jppc/event/obj/bl/bltz0000/bltz0000.ebp").string().c_str());
    const unsigned start=u32(raw,4);const uintptr_t chunk=reinterpret_cast<uintptr_t>(raw.data())+start;
    watchedReady=chunk;
    for(unsigned i=1;i<AtelResourceParts;++i) {const auto part=u32(raw,4+i*4);if(part>=0x30 && part<raw.size()) watchedPending=reinterpret_cast<uintptr_t>(raw.data())+part;}
    check(watchedPending && watchedPending!=watchedReady,"real EV has an independent later part");
    AtelSpan span;AtelIdentity identity;
    observing=true;capture(raw);check(nativeAtelSpan(chunk,span),"exact primary span published Native");
    check(!readyAtelIdentity(chunk,identity),"pending worker never supplies invented identity");observing=false;
    check(atelPreparation(chunk)==AtelPreparation::Pending,"exact accepted receipt is pending before worker drain");
    check(span.size<=raw.size()-start && span.size>=0x38,"declared chunk lies in logical parent");
    const auto expected=decodeOwnedAtelScript(raw.data()+start,span.size);
    const auto before=std::chrono::steady_clock::now();std::fill(raw.begin(),raw.end(),uint8_t{0xa5});drain();
    const auto work=std::chrono::duration<double,std::milli>(std::chrono::steady_clock::now()-before).count();
    check(sawEarlyReady,"first completed real ATEL consumer is notified while later resource parts remain pending");
    watchedReady=watchedPending=0;
    observing=true;check(readyAtelIdentity(chunk,identity),"ready identity from captured bytes after source reuse");
    check(atelPreparation(chunk)==AtelPreparation::Ready,"completed real resource is ready");
    check(identity.checksum==expected.checksum && identity.length==expected.codeLength && identity.workers==expected.workers.size() && std::strcmp(identity.name,expected.name.c_str())==0,"real large Blitz identity parity");
    unsigned header=0;check(readyAtelWorkerHeader(chunk,0,header) && header==expected.workers[0].header,"indexed ready worker header");observing=false;
    AtelSavedFacts savedGuard;bool available=true;
    check(!readyAtelCallbackAvailable(span,0,2,savedGuard,available),"Native cannot enter the owned callback interpreter");
    nativeOwner=false;
    check(readyAtelCallbackAvailable(span,0,2,savedGuard,available) && !available,"actual large Blitz entry2 RET remains unavailable after engine source overwrite");
    std::array<unsigned,3> inspected{};
    check(visitReadyAtelScript(span,inspectOwned,&inspected) && inspected[0]==expected.checksum && inspected[1]==expected.workers.size() && inspected[2]==expected.codeLength,"exact immutable script consumer uses owned resource after source overwrite");
    nativeOwner=true;
    check(!visitReadyAtelScript(span,inspectOwned,&inspected),"Native cannot enter owned scanning/processing");
    const auto liveSerial=identity.span.serial;
    ++epoch;observing=true;
    check(readyAtelIdentity(chunk,identity) && nativeAtelSpan(chunk,span) && span.epoch==epoch && identity.span.epoch==epoch &&
        identity.span.serial==liveSerial,"speech/graphics generation change preserves exact still-live allocation identity");
    Event free;free.args[0]=reinterpret_cast<uintptr_t>(raw.data());handlers.at(0x48d490).before(free);
    check(!readyAtelIdentity(chunk,identity) && !nativeAtelSpan(chunk,span),"before-free scalar receipt invalidation rejects retired allocation");observing=false;
    auto nagi=asset((ffx_fixture::evidenceRoot()/"offline-output/FFX/20261003_audit/files/ffx_ps2/ffx/master/jppc/event/obj/na/nagi0000/nagi0000.ebp").string().c_str());
    const uintptr_t nextChunk=reinterpret_cast<uintptr_t>(nagi.data())+u32(nagi,4);
    observing=true;capture(nagi);observing=false;drain();check(readyAtelIdentity(nextChunk,identity),"real race part ready");
    check(readyAtelWorkerHeader(nextChunk,0,header),"race worker header");
    const auto* base=nagi.data()+u32(nagi,4);uint16_t count;std::memcpy(&count,base+header+2,2);
    unsigned begin;std::memcpy(&begin,base+header+0x14,4);check(count!=0,"real declaration count");
    unsigned low,high,index=99;std::memcpy(&low,base+begin,4);std::memcpy(&high,base+begin+4,4);
    observing=true;check(readyAtelDeclaration(nextChunk,header,low,high,index) && index==0,"indexed actual first declaration");observing=false;
    unsigned fixedLow=0,fixedHigh=0;observing=true;
    check(readyAtelDeclarationAt(nextChunk,header,0,fixedLow,fixedHigh) && fixedLow==low && fixedHigh==high &&
        !readyAtelDeclarationAt(nextChunk,header,count,fixedLow,fixedHigh),"fixed declaration index retains original row/bounds");observing=false;
    const auto rejectedBefore=rejectedAtelResourcesForTest();auto bad=nagi;put(bad,4,static_cast<unsigned>(bad.size()+1));
    capture(bad);check(rejectedAtelResourcesForTest()>rejectedBefore,"whole file offset rejection");
    const auto rejectedSize=rejectedAtelResourcesForTest();capture(nagi,static_cast<unsigned>(nagi.size()+1));
    check(rejectedAtelResourcesForTest()>rejectedSize,"exact logical copy boundary rejects overread");
    auto unsupported=nagi;unsupported[3]='2';const auto versionBefore=rejectedAtelResourcesForTest();
    observing=true;capture(unsupported);observing=false;check(rejectedAtelResourcesForTest()>versionBefore,"unqualified EV version wholly rejected");
    auto valid=nagi;const uintptr_t validChunk=reinterpret_cast<uintptr_t>(valid.data())+u32(valid,4);
    capture(valid,0,12,0x1234);drain();check(!readyAtelIdentity(validChunk,identity),"unrelated allocation caller rejected");
    for(unsigned i=0;i<AtelResourceSlots;++i) capture(nagi);
    const auto pressureBefore=rejectedAtelResourcesForTest();observing=true;capture(nagi);observing=false;
    check(rejectedAtelResourcesForTest()>pressureBefore,"held worker complete file pressure rejected without wait");drain();
    check(!readyAtelIdentity(nextChunk,identity),"rejected new allocation cannot inherit an old same-address checksum");
    capture(nagi);drain();check(readyAtelIdentity(nextChunk,identity),"fresh exact capture recovers after pressure");
    const auto accepted=identity.span.serial;observing=true;capture(nagi);
    check(!readyAtelIdentity(nextChunk,identity),"new same-address receipt invalidates old checksum before preparation");observing=false;
    drain();check(readyAtelIdentity(nextChunk,identity) && identity.span.serial!=accepted,"same-address final prepared receipt");
    auto rejectedPart=nagi;const auto rejectedStart=u32(rejectedPart,4);put(rejectedPart,rejectedStart+0xc,0xffffffff);
    const auto rejectedChunk=reinterpret_cast<uintptr_t>(rejectedPart.data())+rejectedStart;
    observing=true;capture(rejectedPart);observing=false;
    check(atelPreparation(rejectedChunk)==AtelPreparation::Pending,"accepted malformed part waits for exact owned preparation");drain();
    observing=true;check(atelPreparation(rejectedChunk)==AtelPreparation::Rejected && !readyAtelIdentity(rejectedChunk,identity) &&
        !readyAtelWorkerHeader(rejectedChunk,0,header) && !readyAtelDeclarationAt(rejectedChunk,0,0,fixedLow,fixedHigh),"malformed part terminal status cannot expose a previous prepared payload");observing=false;
    check(preparationSignals>0 && !wrongSignalOwner,"completion notifies copied-data consumer on worker only");
    auto residentA=nagi,residentB=nagi;
    const auto a=reinterpret_cast<uintptr_t>(residentA.data())+u32(residentA,4),b=reinterpret_cast<uintptr_t>(residentB.data())+u32(residentB,4);
    capture(residentA);drain();capture(residentB);drain();
    observing=true;check(readyAtelIdentity(a,identity) && readyAtelIdentity(b,identity),"transient capture-slot reuse preserves two independent live physical receipts");
    Event unrelatedFree;unrelatedFree.args[0]=0x1234;handlers.at(0x48d490).before(unrelatedFree);
    check(readyAtelIdentity(a,identity) && readyAtelIdentity(b,identity),"unrelated free cannot retire a live script");
    Event freeA;freeA.args[0]=reinterpret_cast<uintptr_t>(residentA.data());handlers.at(0x48d490).before(freeA);
    check(!nativeAtelSpan(a,span) && readyAtelIdentity(b,identity),"before-free invalidates only its exact opaque parent");observing=false;
    std::array<std::vector<uint8_t>,20> residents;
    std::array<uintptr_t,20> residentChunks{};
    for(unsigned i=0;i<residents.size();++i) {
        residents[i]=nagi;residentChunks[i]=reinterpret_cast<uintptr_t>(residents[i].data())+u32(residents[i],4);
        observing=true;capture(residents[i]);observing=false;drain();
    }
    observing=true;
    unsigned residentReady=0,residentRejected=0;
    for(unsigned i=0;i<residentChunks.size();++i) {
        const auto resident=residentChunks[i];
        if(readyAtelIdentity(resident,identity)) ++residentReady;
        else {++residentRejected;check(atelPreparation(resident)==AtelPreparation::Rejected,"aggregate cache pressure explicitly rejects the new part");}
        if(i<8) check(readyAtelIdentity(resident,identity),"eight live files survive more than four transient capture slots");
    }
    check(residentReady>=8 && residentRejected>0 && preparedAtelBytesForTest()<=64u*1024u*1024u,"large resident load has a bounded aggregate owned cache without evicting live parts");
    observing=false;std::printf("METRIC prepared resident files ready=%u rejected=%u retained bytes=%zu (64MiB limit)\n",residentReady,residentRejected,preparedAtelBytesForTest());observing=true;
    check(readyAtelIdentity(b,identity),"existing resident metadata survives twenty subsequent captures");
    for(const auto& resident:residents) {Event release;release.args[0]=reinterpret_cast<uintptr_t>(resident.data());handlers.at(0x48d490).before(release);}
    observing=false;drain();
    observing=true;
    for(const auto resident:residentChunks) check(!readyAtelIdentity(resident,identity),"worker reclamation follows exact free without exposing retired metadata");
    check(readyAtelIdentity(b,identity),"retired-bank reclamation preserves unrelated living metadata");observing=false;
    std::atomic<unsigned> concurrentMisses{0};
    const auto concurrentReader=[&] {
        for(unsigned i=0;i<10000;++i) {AtelIdentity result;if(!readyAtelIdentity(b,result) || result.checksum!=identity.checksum) ++concurrentMisses;}
    };
    std::thread readerOne(concurrentReader),readerTwo(concurrentReader);readerOne.join();readerTwo.join();
    check(!concurrentMisses,"stable immutable metadata serves simultaneous Native/preparation readers without contention negatives");
    check(!foreignCopies && !allocations && !frees,"Native heap0/0 and worker engine-read0");
    std::printf("PASS: exact loader receipt/owned real Blitz and race decode/ready indexed declarations/source reuse/generation/same-address/pressure; Native heap0/0, worker native copies0. Cold held-Blitz preparation %.3f ms; fixture only.\n",work);
}
