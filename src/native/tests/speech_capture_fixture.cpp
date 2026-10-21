#include "../speech_capture.hpp"
#include <atomic>
#include <thread>
#include <vector>
#include <cstdio>
#include <cstdlib>
#include <new>

thread_local bool nativeCapture=false;
std::atomic<unsigned> nativeAllocations{0};
void* operator new(size_t size) {
    if(nativeCapture) nativeAllocations.fetch_add(1);
    if(auto* result=std::malloc(size?size:1)) return result;
    throw std::bad_alloc();
}
void* operator new[](size_t size) {return ::operator new(size);}
void operator delete(void* p) noexcept {std::free(p);}
void operator delete[](void* p) noexcept {std::free(p);}
void operator delete(void* p,size_t) noexcept {std::free(p);}
void operator delete[](void* p,size_t) noexcept {std::free(p);}
namespace {
using namespace ffx::speechq;
void check(bool value,const char* reason) {
    if(!value) {std::fprintf(stderr,"FAIL: %s\n",reason);std::exit(1);}
}
Header make(unsigned epoch,unsigned flags=0) {Header value{};value.budgetEpoch=epoch;value.flags=flags;return value;}
bool publish(Header header,const wchar_t* text,size_t length) {
    nativeCapture=true;const bool accepted=capture(header,text,length);nativeCapture=false;return accepted;
}
unsigned drain(const wchar_t* expected=nullptr) {
    Header header{};std::wstring text;unsigned count=0,previous=0;
    while(pop(header,text)) {
        check(!count || static_cast<int>(header.sequence-previous)>0,"accepted event order");previous=header.sequence;
        if(expected) check(text==expected,"owned exact copied text");
        release(header);++count;
    }
    check(!capturesPending(),"drained capture count");return count;
}
}
int main() {
    using namespace ffx::speechq;
    const auto epoch=currentBudgetEpoch();
    std::wstring source=L"Owned copied \u0105\u015b\u017c caption";
    check(publish(make(epoch),source.data(),source.size()),"initial owned capture");
    source.assign(L"overwritten source");
    check(drain(L"Owned copied \u0105\u015b\u017c caption")==1,"copied unicode source lifetime");
    std::atomic<unsigned> rejected{0};std::vector<std::thread> producers;
    for(unsigned worker=0;worker<4;++worker) producers.emplace_back([&] {
        const std::wstring line=L"Equal valid event";
        for(unsigned i=0;i<100;++i) if(!publish(make(epoch),line.data(),line.size())) ++rejected;
    });
    for(auto& producer:producers) producer.join();
    check(!rejected && drain(L"Equal valid event")==400,"400 equal concurrent events accepted without deduplication");
    const auto fullEpoch=nextBudgetEpoch();
    for(unsigned i=0;i<EventLimit;++i) check(publish(make(fullEpoch),L"event",5),"complete event budget");
    check(!publish(make(fullEpoch),L"overflow",8),"event cap rejects without allocation/wait");
    const auto bytesEpoch=nextBudgetEpoch();const std::wstring large(4096,L'X');
    for(unsigned i=0;i<128;++i) check(publish(make(bytesEpoch),large.data(),large.size()),"fresh generation accepts full character budget while old captures are retained");
    check(!publish(make(bytesEpoch),L"overflow",8),"exact character cap");
    check(drain()==EventLimit+128,"worker retires old/current owned captures");
    const auto maximumEpoch=nextBudgetEpoch();const std::wstring maximum(CharacterLimit,L'M');
    check(publish(make(maximumEpoch),maximum.data(),maximum.size()),"one maximum-sized ready snapshot");
    Header header{};std::wstring copied;check(pop(header,copied) && copied==maximum,"maximum owned snapshot and block chain");release(header);
    // Reuse every budget bank repeatedly after worker retirement. Producer
    // metadata never releases strings or reuses a live old-generation bank.
    for(unsigned i=0;i<100;++i) {
        const auto next=nextBudgetEpoch();check(publish(make(next),L"bank",4),"retired bank reuse");
        check(drain(L"bank")==1,"bank lease balances after retirement");
    }
    check(!publish(make(currentBudgetEpoch()),maximum.data(),CharacterLimit+1),"oversize rejected before read");
    check(!publish(make(currentBudgetEpoch()),nullptr,10),"invalid source rejected before read");
    auto* pages=static_cast<wchar_t*>(VirtualAlloc(nullptr,8192,MEM_RESERVE|MEM_COMMIT,PAGE_READWRITE));
    check(pages!=nullptr,"guard-page fixture allocation");DWORD protection=0;
    check(VirtualProtect(reinterpret_cast<char*>(pages)+4096,4096,PAGE_NOACCESS,&protection)!=0,"guard-page fixture protection");
    check(!publish(make(currentBudgetEpoch()),pages,4096),"borrowed source fault rejects and releases the in-progress capture");
    check(publish(make(currentBudgetEpoch()),L"after fault",11) && drain(L"after fault")==1,"source fault leaves no ordering barrier or budget debt");
    VirtualFree(pages,0,MEM_RELEASE);
    check(nativeAllocations==0,"native capture path never calls the C++ allocator");
    std::puts("PASS: owned UTF16/copy lifetime, 400 concurrent equal ordered events, exact event/character limits, retained generations, maximum snapshot, budget-bank reuse and zero native allocations.");
}
