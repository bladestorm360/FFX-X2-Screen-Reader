#include "../hook_calls.hpp"
#include "../owned_retirement.hpp"
#include <atomic>
#include <cstdio>
#include <cstdlib>
#include <new>
namespace {
thread_local bool nativeCapture=false;
std::atomic<unsigned> allocations{0},deletions{0};
HANDLE held=nullptr,release=nullptr;
std::atomic<bool> hold{true};
struct Blocking {
    std::wstring owned;
    explicit Blocking(const wchar_t* text):owned(text) {}
    Blocking(Blocking&&)=default;
    ~Blocking() {
        if(!owned.empty() && hold.exchange(false)) {
            SetEvent(held);if(WaitForSingleObject(release,5000)!=WAIT_OBJECT_0) std::abort();
        }
    }
};
void check(bool ok,const char* why) {if(!ok) {std::fprintf(stderr,"FAIL: %s\n",why);std::exit(1);}}
}
void* operator new(std::size_t size) {
    if(nativeCapture) ++allocations;
    if(auto* value=std::malloc(size?size:1)) return value;
    throw std::bad_alloc();
}
void operator delete(void* value) noexcept {if(nativeCapture && value) ++deletions;std::free(value);}
void operator delete(void* value,std::size_t) noexcept {operator delete(value);}
namespace ffx {void abandonNativeCall(NativeCall* call) noexcept {retireNativeCall(call);}}
int main() {
    held=CreateEventW(nullptr,TRUE,FALSE,nullptr);release=CreateEventW(nullptr,TRUE,FALSE,nullptr);
    check(held && release,"worker fixture events");
    ffx::retirement::prepare<std::vector<Blocking>>();
    std::vector<Blocking> block;block.emplace_back(L"held owned destructor");
    check(ffx::retirement::retire(block),"publish held destructor before bootstrap worker");
    check(ffx::retirement::start(nullptr),"start unrelated owned-container retirement");
    check(WaitForSingleObject(held,5000)==WAIT_OBJECT_0,"destructor worker held");
    std::array<ffx::NativeCall*,4096> frames{};
    const auto start=GetTickCount64();nativeCapture=true;
    for(auto& frame:frames) {frame=ffx::claimNativeCall();check(frame!=nullptr,"exact owned frame capacity");}
    check(!ffx::claimNativeCall(),"explicit frame pressure has no heap fallback");
    // POD before/copy values belong to the active call until its native return.
    for(unsigned i=0;i<frames.size();++i) {frames[i]->events[0].before[0]=i;frames[i]->events[0].copied[0]=i+1;}
    for(auto* frame:frames) ffx::retireNativeCall(frame);
    for(auto& frame:frames) {
        frame=ffx::claimNativeCall();check(frame!=nullptr,"all POD frame capacity recycles while owned destructor worker remains held");
        check(frame->events[0].before[0]==0 && frame->events[0].copied[0]==0 && !frame->events[0].copiedCount,"reclaimed call starts with clean bounded metadata");
    }
    for(auto* frame:frames) ffx::retireNativeCall(frame);
    check(GetTickCount64()-start<1000,"held worker never stalls frame publication/recycling");
    nativeCapture=false;SetEvent(release);
    check(!allocations && !deletions,"native frame storage neither allocates nor frees Event-owned storage");
    std::puts("PASS: 4096 bounded POD frames, blocked unrelated destructor independence, pressure/immediate reuse and zero native allocation/deletion.");
}
