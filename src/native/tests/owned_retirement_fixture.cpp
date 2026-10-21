#include "../owned_retirement.hpp"
#include "../resource_reset.hpp"
#include <atomic>
#include <thread>
#include <cstdio>
#include <cstdlib>
#include <new>
#include <set>
namespace {
std::atomic<bool> nativeCapture{false};
std::atomic<unsigned> nativeAllocations{0},nativeDestructions{0},destroyed{0};
HANDLE destructorStarted=nullptr,releaseDestructor=nullptr;
std::atomic<bool> holdDestructor{false};
DWORD owner=0;
bool loading=false;
std::atomic<unsigned> epoch{1};
void check(bool value,const char* why) {
    if(!value) {std::fprintf(stderr,"FAIL: %s\n",why);std::exit(1);}
}
struct Record {
    std::wstring text;
    explicit Record(const wchar_t* value):text(value) {}
    Record(Record&&)=default;
    ~Record() {
        if(!text.empty()) {
            if(nativeCapture.load()) ++nativeDestructions;
            if(holdDestructor.exchange(false)) {
                SetEvent(destructorStarted);
                check(WaitForSingleObject(releaseDestructor,5000)==WAIT_OBJECT_0,"release owned destructor worker");
            }
            ++destroyed;
        }
    }
};
std::vector<Record> records;
std::wstring description;
unsigned resetCalls=0,arrivalCalls=0,arrivalRoom=0;
void reset() {
    check(GetCurrentThreadId()==owner,"reset executes only on native owner");
    ++resetCalls;
    ffx::retirement::retire(records);ffx::retirement::retire(description);
}
void arrival(unsigned room,unsigned eventEpoch) {
    check(GetCurrentThreadId()==owner && eventEpoch==epoch,"arrival getters stay on current native owner");
    ++arrivalCalls;arrivalRoom=room;
}
}
void* operator new(std::size_t size) {
    if(nativeCapture.load()) ++nativeAllocations;
    if(void* value=std::malloc(size?size:1)) return value;
    throw std::bad_alloc();
}
void operator delete(void* value) noexcept {std::free(value);}
void operator delete(void* value,std::size_t) noexcept {std::free(value);}
namespace ffx {
unsigned generation() {return epoch.load();}
bool nativeGameThread() {return GetCurrentThreadId()==owner;}
bool nativeResourceLoading() noexcept {return loading;}
}
int main() {
    using namespace ffx::retirement;
    owner=GetCurrentThreadId();
    prepare<std::vector<Record>>();prepare<std::wstring>();prepare<std::set<unsigned>>();
    records.emplace_back(L"equal owned event");records.emplace_back(L"equal owned event");
    description.assign(600,L'X');
    destructorStarted=CreateEventW(nullptr,TRUE,FALSE,nullptr);
    releaseDestructor=CreateEventW(nullptr,TRUE,FALSE,nullptr);
    check(destructorStarted && releaseDestructor,"destructor fixture events");
    nativeCapture=true;
    check(retire(records) && retire(description) && records.empty() && description.empty(),"native swaps owned buffers with prepared empties");
    nativeCapture=false;
    holdDestructor=true;
    std::thread worker([]{drainForTest();});
    check(WaitForSingleObject(destructorStarted,5000)==WAIT_OBJECT_0,"owned destructor held on worker");
    records.emplace_back(L"snapshot after blocked worker");
    ffx::configureOwnerReset(reset,arrival);
    ++epoch;loading=true;
    const auto start=GetTickCount64();
    std::thread loader([] {ffx::publishFieldArrival(209,epoch);check(!ffx::serviceOwnerReset(),"foreign loader never resets native state");});
    loader.join();
    check(resetCalls==0 && !ffx::ownerResetReady() && !ffx::serviceOwnerReset(),"loading only invalidates readiness");
    loading=false;nativeCapture=true;
    check(ffx::serviceOwnerReset(),"native reset returns while destructor worker is held");
    check(resetCalls==1 && records.empty() && arrivalCalls==1 && arrivalRoom==209,"bounded arrival request processed after owner reset");
    check(GetTickCount64()-start<500,"worker does not stall owner reset");
    ffx::publishFieldArrival(209,epoch);ffx::publishFieldArrival(209,epoch);
    check(ffx::serviceOwnerReset() && arrivalCalls==3,"equal distinct arrival events are retained");
    nativeCapture=false;
    SetEvent(releaseDestructor);worker.join();drainForTest();
    check(destroyed==3,"all retired owned records destroyed on worker");
    std::array<std::set<unsigned>,65> sets;
    for(unsigned i=0;i<65;++i) sets[i].insert(i);
    nativeCapture=true;
    for(unsigned i=0;i<64;++i) check(retire(sets[i]) && sets[i].empty(),"exact prepared retirement capacity");
    check(!retire(sets[64]) && sets[64].count(64)==1,"pressure leaves ownership and source intact");
    nativeCapture=false;drainForTest();nativeCapture=true;
    check(retire(sets[64]) && sets[64].empty(),"retirement capacity recovers after worker cleanup");
    nativeCapture=false;drainForTest();
    check(!nativeAllocations && !nativeDestructions,"native retirement neither allocates nor destroys owned records");
    CloseHandle(destructorStarted);CloseHandle(releaseDestructor);
    std::puts("PASS: bounded owned retirement, equal records, held destructor independence, native-only reset/arrival, pressure ownership and zero producer allocation/destruction.");
}
