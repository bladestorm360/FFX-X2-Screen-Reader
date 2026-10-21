#include "external_fixture.hpp"
// Actual production capture/domain/preparation/sink and resource cache.
// Native getters use controlled archived-data host state; no SDK/game/device.
#include "../minigames.hpp"
#include "../minigame_memory.hpp"
#include "../speech_queue.hpp"
#include "../speech_domains.hpp"
#include "../settings.hpp"
#include "../native_worker_index.hpp"
#include <atomic>
#include <chrono>
#include <fstream>
#include <map>
#include <cstdio>
#include <cstdlib>
#include <thread>
namespace {
using namespace ffx;
DWORD nativeOwner=0;thread_local bool measuring=false;
std::atomic<unsigned> allocations{0},frees{0},foreignReads{0};
std::atomic<bool> held{false};HANDLE releasePreparation=nullptr,preparationHeld=nullptr;
struct Handlers {Handler before=nullptr,after=nullptr;};std::map<unsigned,Handlers> handlers;
std::vector<uint8_t> eventFile,actors(128*0x880),saved(0x2000),variables(0x2000),controller(0x200);
std::vector<std::vector<uint8_t>> workers;unsigned chunkOffset=0;
std::vector<speechq::Utterance> collected;
std::atomic<unsigned> cueCount{0};std::atomic<bool> cueActive{false};
bool enabled=true;double nativeUs=0,nativeMaxUs=0;unsigned nativeCalls=0;
void check(bool ok,const char* reason) {if(!ok){std::fprintf(stderr,"FAIL: %s\n",reason);std::exit(1);}}
uintptr_t ptr(std::vector<uint8_t>& bytes) {return reinterpret_cast<uintptr_t>(bytes.data());}
uintptr_t chunk() {return ptr(eventFile)+chunkOffset;}
void owner() {if(GetCurrentThreadId()!=nativeOwner){++foreignReads;check(false,"Worker entered affine native getter");}}
template<class T> void put(std::vector<uint8_t>& bytes,unsigned at,T value) {check(at<=bytes.size() && sizeof(T)<=bytes.size()-at,"controlled host write bound");std::memcpy(bytes.data()+at,&value,sizeof(value));}
uintptr_t __cdecl worker(int index) {owner();return index>=0 && static_cast<size_t>(index)<workers.size()?ptr(workers[index]):0;}
uintptr_t __cdecl geometry(uintptr_t w) {owner();return w+0x558;}
uintptr_t __cdecl variable(uintptr_t w,int index,int element) {
    owner();const auto header=read<uintptr_t>(w),base=read<uintptr_t>(w+4);
    const auto low=read<unsigned>(base+read<unsigned>(header+0x14)+index*8);
    const unsigned width=low>>28==6?4:1;
    return ((low>>25&7)==6?ptr(variables):ptr(saved))+(low&0xffffff)+element*width;
}
struct NativeCost {
    std::chrono::steady_clock::time_point start=std::chrono::steady_clock::now();
    NativeCost(){measuring=true;}
    ~NativeCost(){measuring=false;const auto us=std::chrono::duration<double,std::micro>(std::chrono::steady_clock::now()-start).count();nativeUs+=us;nativeMaxUs=std::max(nativeMaxUs,us);++nativeCalls;}
};
void set(const MinigameBinding& binding,unsigned value) {write<uint8_t>(variable(worker(0),binding.index,0),static_cast<uint8_t>(value));}
void change(const MinigameBinding& binding,unsigned value) {
    Event e;e.args[0]=worker(0);e.args[1]=binding.index;e.epoch=generation();
    const auto listener=handlers.at(0x4713a0);NativeCost cost;
    listener.before(e);set(binding,value);listener.after(e);
}
void query() {NativeCost cost;check(dispatchMinigameKey(VK_F5,false),"native accepted score query");}
void takeReady() {speechq::Utterance item;while(speechq::take(item)) if(speechq::current(item.header)) collected.push_back(std::move(item));}
template<class P> void wait(P predicate,const char* reason) {
    const auto end=GetTickCount64()+5000;
    do {takeReady();if(predicate())return;Sleep(1);} while(GetTickCount64()<end);
    check(false,reason);
}
void hold() {
    ResetEvent(preparationHeld);ResetEvent(releasePreparation);held=true;speechq::notifyPreparation();
    check(WaitForSingleObject(preparationHeld,5000)==WAIT_OBJECT_0,"test preparation owner held outside Native callbacks");
}
void resume() {held=false;SetEvent(releasePreparation);speechq::notifyPreparation();}
void load() {
    resetMinigames();speechq::invalidate("fixture-resource");collected.clear();
    std::ifstream file(ffx_fixture::evidenceRoot()/"offline-output/FFX/20261003_audit/files/ffx_ps2/ffx/master/jppc/event/obj/bl/bltz0000/bltz0000.ebp",std::ios::binary);
    check(file.good(),"actual archived EV file");eventFile.assign(std::istreambuf_iterator<char>(file),{});
    chunkOffset=read<unsigned>(ptr(eventFile)+4);const auto base=chunk();const auto count=read<uint16_t>(base+0x34);
    workers.assign(count,std::vector<uint8_t>(0x600));
    std::fill(saved.begin(),saved.end(),uint8_t{0});std::fill(variables.begin(),variables.end(),uint8_t{0});
    for(unsigned i=0;i<count;++i) {
        put(workers[i],0,base+read<unsigned>(base+0x38+i*4));put(workers[i],4,base);put(workers[i],0x2e,static_cast<uint16_t>(i));
        put(workers[i],0x9c,ptr(actors)+i*0x880);put(actors,i*0x880,static_cast<uint16_t>(1));put(actors,i*0x880+2,static_cast<uint16_t>(1));
        put(workers[i],0x558,static_cast<float>(i*10));put(workers[i],0x55c,0.f);put(workers[i],0x560,0.f);
    }
    put(controller,0xa,uint16_t{0});put(controller,0xc,count);invalidateNativeWorkerIndex();
    Event loadEvent;loadEvent.args[0]=12;loadEvent.nativeStack=0x900000;loadEvent.epoch=generation();
    Event allocation;allocation.caller=0x48d2a4;allocation.edi=static_cast<unsigned>(eventFile.size());allocation.args[0]=(allocation.edi+0x7ffu)&~0x7ffu;allocation.result=ptr(eventFile);allocation.epoch=generation();
    {NativeCost cost;handlers.at(0x48d230).before(loadEvent);handlers.at(0x2871c0).before(allocation);handlers.at(0x2871c0).after(allocation);loadEvent.result=ptr(eventFile);handlers.at(0x48d230).after(loadEvent);}
}
void prepareResource() {
    // The resource helper's test drain runs on an actual copied-data worker.
    // Joining occurs only in the controlled host, outside measured Native work.
    std::thread work([] {drainAtelResourcesForTest();});work.join();
}
}
void* operator new(size_t size) {if(measuring)++allocations;auto* p=std::malloc(size?size:1);if(!p)throw std::bad_alloc();return p;}
void operator delete(void* p) noexcept {if(p && measuring)++frees;std::free(p);}
void operator delete(void* p,size_t) noexcept {::operator delete(p);}
namespace ffx {
std::atomic<int> scene{0};
unsigned generation() {return speechq::generation();}
bool copy(uintptr_t source,void* out,size_t size) {owner();__try{std::memcpy(out,reinterpret_cast<void*>(source),size);return true;}__except(1){return false;}}
uintptr_t address(unsigned rva) {
    if(rva==0x48d230)return rva;
    if(rva==0x46a7c0)return reinterpret_cast<uintptr_t>(&worker);
    if(rva==0x46c500)return reinterpret_cast<uintptr_t>(&variable);
    if(rva==0x46c240)return reinterpret_cast<uintptr_t>(&geometry);
    if(rva==0xf26ae8){static uintptr_t p;p=ptr(controller);return reinterpret_cast<uintptr_t>(&p);}
    if(rva==0x1fc44e4){static uintptr_t p;p=ptr(actors);return reinterpret_cast<uintptr_t>(&p);}
    if(rva==0x1fc44e0){static unsigned count=128;return reinterpret_cast<uintptr_t>(&count);}
    check(false,"unexpected native binding in controlled host");return 0;
}
bool hook(unsigned rva,Handler before,Handler after){handlers[rva]={before,after};return true;}
bool hookSnapshot(unsigned rva,Handler before,Handler after){return hook(rva,before,after);}
bool gameFocused(){owner();return true;}
bool modMenuActive(){owner();return false;}
bool utilityConsoleActive(){owner();return false;}
bool inBattle(){owner();return false;}
bool butterflyActive(){owner();return false;}
void refreshButterflyAudio(){}
void butterflyDirections(bool){}
void butterflyStatus(){}
uintptr_t playerActor(){owner();return ptr(actors);}
bool cameraBasis(Point& forward,Point& right){owner();forward={1,0,0};right={0,0,-1};return true;}
int setting(Setting id){return id==Setting::Speech?1:id==Setting::RaceCues?enabled:0;}
void adjustSetting(Setting,int){}
std::wstring settingValue(Setting){return L"on";}
bool output(const wchar_t* text,bool interrupt,const char* producer){return speechq::output(text,std::wcslen(text),interrupt,producer,false,generation());}
bool output(const std::wstring& text,bool interrupt,const char* producer){return speechq::output(text.data(),text.size(),interrupt,producer,false,generation());}
double distanceXZ(Point a,Point b){return std::hypot(a.x-b.x,a.z-b.z);}
void publishPositionalCue(float,float,bool active,unsigned epoch){if(epoch==generation()){++cueCount;cueActive=active;}}
}
namespace ffx::speechq {
void captureBeforeDrainForTest(){if(held.load()){SetEvent(preparationHeld);WaitForSingleObject(releasePreparation,INFINITE);}}
}
int main() {
    nativeOwner=GetCurrentThreadId();releasePreparation=CreateEventW(nullptr,TRUE,TRUE,nullptr);preparationHeld=CreateEventW(nullptr,TRUE,FALSE,nullptr);
    const auto backendWake=CreateEventW(nullptr,FALSE,FALSE,nullptr);check(releasePreparation && preparationHeld && backendWake,"controlled host events");
    check(speechq::start(GetModuleHandleW(nullptr),backendWake),"actual production preparation and copied sink");
    check(startAtelResources(),"exact resource capture bootstrap");installAtelResources();installMinigames();
    load();set(blitzBindings[3],2);set(blitzBindings[2],0);set(blitzBindings[4],2);
    const auto accepted=std::chrono::steady_clock::now();change(blitzBindings[0],0);change(blitzBindings[0],1);change(blitzBindings[0],2);
    wait([]{return !speechq::capturesPending();},"first frames captured into production preparation");
    check(collected.empty() && !minigameCapturesKey(VK_F5),"actual queue retains pending first events without unqualified output");
    set(blitzBindings[0],99);std::fill(eventFile.begin(),eventFile.end(),uint8_t{0xa5});prepareResource();
    wait([]{return collected.size()==4 && minigameCapturesKey(VK_F5);},"first accepted events resume on resource-ready notification");
    const auto firstUsable=std::chrono::duration<double,std::milli>(std::chrono::steady_clock::now()-accepted).count();
    check(collected[0].text==L"Goal for your team" && collected[2].text==collected[0].text && collected[1].text.find(L"team 1")!=std::wstring::npos && collected[3].text.find(L"team 2")!=std::wstring::npos,"real queue preserves owned first/equal goal order after source overwrite");
    hold();load();prepareResource();set(blitzBindings[3],2);set(blitzBindings[2],0);set(blitzBindings[4],2);change(blitzBindings[0],0);resume();
    wait([]{return minigameCapturesKey(VK_F5) && !speechq::capturesPending();},"fresh prepared match baseline");
    // Graphics invalidation can retire reader state without freeing/reloading
    // this field allocation. The next real setter must use the live receipt.
    speechq::invalidate("graphics-reset");resetMinigames();change(blitzBindings[0],0);
    wait([]{return minigameCapturesKey(VK_F5) && !speechq::capturesPending();},"still-live field requalifies after graphics reset without file recapture");
    hold();query();speechq::interrupt();resume();
    wait([]{return !speechq::capturesPending();},"canceled captured query drain");Sleep(20);takeReady();check(collected.empty(),"actual original cancellation anchor suppresses old query");
    query();wait([]{return collected.size()==1;},"fresh query after cancellation");
    check(collected[0].text.find(L"team 0")!=std::wstring::npos,"fresh copied query remains available");
    collected.clear();hold();for(unsigned i=0;i<128;++i)query();query();resume();
    wait([]{return collected.size()==129 && minigameCapturesKey(VK_F5);},"actual128 accepted queries plus pressure report and input recovery");
    unsigned scores=0,reports=0;for(const auto& item:collected){if(item.text.find(L"Score:")==0)++scores;if(item.text.find(L"queue is full")!=std::wstring::npos)++reports;}
    check(scores==128 && reports==1,"real FIFO pressure retains all accepted equal queries and reports rejected intent");
    collected.clear();hold();query();speechq::invalidate("stale-test");resetMinigames();resume();
    wait([]{return !speechq::capturesPending();},"old generation discarded by actual central dispatch");Sleep(20);takeReady();check(collected.empty(),"stale queued query cannot emit or restore input");
    check(!allocations && !frees && !foreignReads,"actual Native capture heap0/0 and preparation worker engine reads0");
    std::printf("PASS: actual production minigame/domain/capture/preparation/sink plus owned ATEL; Native heap0/0, Worker engine reads0; first pending events/source overwrite/equal queries/cancellation/128-pressure/recovery/stale dispatch.\nMETRIC controlled getter host Native calls=%u total=%.3fus max=%.3fus; first retained utterances available=%.3fms including explicit fixture resource hold. No SDK/audible/gameplay/whole-mod claim.\n",nativeCalls,nativeUs,nativeMaxUs,firstUsable);
    std::fflush(stdout);ExitProcess(0); // Process-lifetime production owners finish with the controlled host.
}
