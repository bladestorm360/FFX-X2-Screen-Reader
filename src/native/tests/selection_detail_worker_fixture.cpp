#include "../selection_details.hpp"
#include "../speech_queue.hpp"
#include "../events.hpp"
#include "../settings.hpp"
#include <atomic>
#include <cstdio>
#include <cstdlib>
namespace {
thread_local bool nativeCapture=false;
std::atomic<unsigned> allocations{0},destructions{0},idle{0};
std::atomic<bool> hold{true};
HANDLE entered=nullptr,resume=nullptr;
void check(bool ok,const char* message) {if(!ok) {std::fprintf(stderr,"FAIL: %s\n",message);std::exit(1);}}
void held() {check(WaitForSingleObject(entered,3000)==WAIT_OBJECT_0,"preparation held");}
void release() {hold=false;SetEvent(resume);}
void blockAgain() {ResetEvent(entered);ResetEvent(resume);hold=true;}
void waitIdle(unsigned before) {
    const auto deadline=GetTickCount64()+3000;
    while(idle.load()==before && GetTickCount64()<deadline) Sleep(1);
    check(idle.load()!=before,"maintenance follows the complete drain");
}
std::wstring take() {
    const auto deadline=GetTickCount64()+3000;ffx::speechq::Utterance result;
    while(!ffx::speechq::take(result) && GetTickCount64()<deadline) Sleep(1);
    check(!result.text.empty(),"SDK sink receives expected owned output");return result.text;
}
void observedIdle() {++idle;}
void noOp(const ffx::speechq::Header&,const std::wstring&) {}
}
void* operator new(size_t n) {if(nativeCapture) ++allocations;if(auto p=std::malloc(n?n:1)) return p;throw std::bad_alloc();}
void* operator new[](size_t n) {return ::operator new(n);}
void operator delete(void* p) noexcept {if(nativeCapture && p) ++destructions;std::free(p);}
void operator delete[](void* p) noexcept {::operator delete(p);}
void operator delete(void* p,size_t) noexcept {::operator delete(p);}
void operator delete[](void* p,size_t) noexcept {::operator delete(p);}
namespace ffx {
unsigned generation() {return speechq::generation();}
int setting(Setting s) {return s==Setting::Speech?1:0;}
bool readCopiedTextDetail() {return false;}
void resetCopiedTextDetail() {}
namespace speechq {
void captureBeforeDrainForTest() {if(hold.load()) {SetEvent(entered);WaitForSingleObject(resume,INFINITE);}}
}
}
int main() {
    entered=CreateEventW(nullptr,TRUE,FALSE,nullptr);resume=CreateEventW(nullptr,TRUE,FALSE,nullptr);
    const auto backendWake=CreateEventW(nullptr,FALSE,FALSE,nullptr);
    check(entered && resume && backendWake,"fixture barriers");
    ffx::installSelectionDetails();
    ffx::speechq::registerDomain(31,noOp,nullptr);ffx::speechq::registerDomainMaintenance(31,observedIdle);
    check(ffx::speechq::start(GetModuleHandleW(nullptr),backendWake),"preparation starts without a real SDK");held();
    const std::wstring label=L"Equal displayed detail",extra=L"More";
    nativeCapture=true;
    ffx::selectionDetail(label,true);ffx::selectionDetail(label,true);ffx::appendSelectionDetail(extra);
    check(ffx::readSelectionDetail(),"manual request never waits for owned text preparation");
    nativeCapture=false;release();
    check(take()==label && take()==label && take()==label+L". More","equal distinct automatic events and latest appended manual buffer are preserved");
    blockAgain();const unsigned trigger=0;
    check(ffx::speechq::captureDomain(31,&trigger,sizeof(trigger),nullptr,0,ffx::generation()),"barrier wake");held();
    nativeCapture=true;ffx::selectionDetail(label,false);check(ffx::readSelectionDetail(),"captured manual read accepted");ffx::resetSelectionDetail();
    check(!ffx::readSelectionDetail(),"reset immediately removes native detail availability");nativeCapture=false;
    const auto before=idle.load();release();waitIdle(before);check(!ffx::speechq::pending(),"reset retires queued stale manual reads");
    blockAgain();check(ffx::speechq::captureDomain(31,&trigger,sizeof(trigger),nullptr,0,ffx::generation()),"second barrier wake");held();
    nativeCapture=true;unsigned accepted=0;
    while(accepted<2048 && ffx::speechq::captureDomain(31,&trigger,sizeof(trigger),nullptr,0,ffx::generation())) ++accepted;
    check(accepted>=2047 && accepted<=2048,"metadata is full, accounting for a possible unconsumed wake record");
    check(!ffx::speechq::captureDomain(31,&trigger,sizeof(trigger),nullptr,0,ffx::generation()),"full capture rejects without waiting");
    ffx::resetSelectionDetail();check(!ffx::readSelectionDetail(),"lost reset marker cannot restore availability");nativeCapture=false;
    const auto full=idle.load();release();waitIdle(full);
    check(allocations.load()==0 && destructions.load()==0 && !ffx::speechq::pending(),"capture/read/reset pressure has no native heap or owned destruction");
    puts("PASS: owned selection details, held-preparation independence, equal events, append/read FIFO, zero native heap and lost-marker retirement.");
    return 0;
}
