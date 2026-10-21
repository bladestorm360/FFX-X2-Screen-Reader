#include "external_fixture.hpp"
// Actual production capture/domain/preparation/sink/resource and seven-channel
// interaction capture, using controlled affine getters and real archived EV.
#include "../butterfly.hpp"
#include "../speech_queue.hpp"
#include "../speech_domains.hpp"
#include "../settings.hpp"
#include "../native_worker_index.hpp"
#include "../atel_resources.hpp"
#include <atomic>
#include <chrono>
#include <fstream>
#include <map>
#include <cstdio>
#include <cstdlib>
#include <thread>
#include <cmath>
namespace {
using namespace ffx;
DWORD nativeOwner=0;thread_local bool measuring=false;
std::atomic<unsigned> allocations{0},frees{0},foreignReads{0},cueEvents{0};
std::atomic<bool> held{false},cueActive{false};HANDLE released=nullptr,preparationHeld=nullptr;
struct Listener {Handler before=nullptr,after=nullptr;};std::map<unsigned,std::vector<Listener>> listeners;
std::vector<uint8_t> fileBytes,actors(128*0x880),saved(0x68c0-0x1ec),variables(0x2000),controller(0x200);
std::vector<std::vector<uint8_t>> workers;unsigned chunkOffset=0;
std::vector<speechq::Utterance> collected;bool focused=true,enabled=true;
double nativeUs=0,nativeMaxUs=0;unsigned nativeCalls=0,entryCalls=0,priorityCalls=0;
void check(bool ok,const char* why){if(!ok){std::fprintf(stderr,"FAIL: %s\n",why);std::exit(1);}}
uintptr_t ptr(std::vector<uint8_t>& v){return reinterpret_cast<uintptr_t>(v.data());}
uintptr_t chunk(){return ptr(fileBytes)+chunkOffset;}
void owner(){if(GetCurrentThreadId()!=nativeOwner){++foreignReads;check(false,"Worker entered affine engine getter/copy");}}
template<class T> void put(std::vector<uint8_t>& bytes,unsigned at,T value){check(at<=bytes.size()&&sizeof(T)<=bytes.size()-at,"controlled write bound");std::memcpy(bytes.data()+at,&value,sizeof(value));}
uintptr_t __cdecl worker(int i){owner();return i>=0&&static_cast<size_t>(i)<workers.size()?ptr(workers[i]):0;}
uintptr_t __cdecl variable(uintptr_t w,int index,int element){
    owner();const auto h=read<uintptr_t>(w),c=read<uintptr_t>(w+4),low=read<unsigned>(c+read<unsigned>(h+0x14)+index*8);
    const unsigned width=low>>28==0?1u:4u;
    if((low>>25&7)==3)return w+0x300+(low&0xffffff)+element*width;
    return ((low>>25&7)==6?ptr(variables):ptr(saved))+(low&0xffffff)+element*width;
}
uint8_t __cdecl priority(unsigned,unsigned){owner();++priorityCalls;return 1;}
uint16_t __cdecl entry(unsigned,unsigned,uintptr_t){owner();++entryCalls;return 4;}
struct Cost {
    std::chrono::steady_clock::time_point start=std::chrono::steady_clock::now();
    Cost(){measuring=true;}
    ~Cost(){measuring=false;const auto us=std::chrono::duration<double,std::micro>(std::chrono::steady_clock::now()-start).count();nativeUs+=us;nativeMaxUs=std::max(nativeMaxUs,us);++nativeCalls;}
};
void set(unsigned local,unsigned index,int value){
    const auto w=worker(static_cast<int>(local)),source=variable(w,index,0);
    const auto low=read<unsigned>(chunk()+read<unsigned>(read<uintptr_t>(w)+0x14)+index*8);
    if(low>>28==0)write<uint8_t>(source,static_cast<uint8_t>(value));else write<int>(source,value);
}
void change(unsigned local,unsigned index,int value){
    Event e;e.rva=0x4713a0;e.epoch=generation();e.args[0]=worker(static_cast<int>(local));e.args[1]=index;Cost cost;
    for(const auto& listener:listeners.at(e.rva))if(listener.before)listener.before(e);
    set(local,index,value);
    for(const auto& listener:listeners.at(e.rva))if(listener.after)listener.after(e);
}
void query(){Cost cost;butterflyDirections(false);}
void commit(){Event e;e.epoch=generation();Cost cost;for(const auto& l:listeners.at(0x43d3b0))if(l.after)l.after(e);}
void takeReady(){speechq::Utterance item;while(speechq::take(item))if(speechq::current(item.header))collected.push_back(std::move(item));}
template<class P> void wait(P ready,const char* why){const auto end=GetTickCount64()+5000;do{takeReady();if(ready())return;Sleep(1);}while(GetTickCount64()<end);check(false,why);}
void hold(){ResetEvent(preparationHeld);ResetEvent(released);held=true;speechq::notifyPreparation();check(WaitForSingleObject(preparationHeld,5000)==WAIT_OBJECT_0,"test holds preparation owner outside Native work");}
void resume(){held=false;SetEvent(released);speechq::notifyPreparation();}
void body(unsigned local,float x,float z,bool red=false){
    const unsigned a=local*0x880;put(actors,a,uint16_t{0x5008});put(actors,a+2,uint16_t{1});
    put(actors,a+0xc,x);put(actors,a+0x10,0.f);put(actors,a+0x14,z);
    put(actors,a+0x334,red?.9f:.1f);put(actors,a+0x340,red?.1f:.3f);put(actors,a+0x34c,red?.1f:1.f);
    put(actors,a+0x354,1.f);put(actors,a+0x358,1.f);put(actors,a+0x5c,1.f);put(actors,a+0x64,1.f);put(actors,a+0x4e4,2.f);
    put(workers[local],0x558,x);put(workers[local],0x55c,0.f);put(workers[local],0x560,z);
}
void load(){
    resetButterfly();speechq::invalidate("fixture-load");collected.clear();
    if(!fileBytes.empty()){Event free;free.args[0]=ptr(fileBytes);Cost cost;for(const auto& l:listeners.at(0x48d490))if(l.before)l.before(free);}
    std::ifstream input(ffx_fixture::evidenceRoot()/"offline-output/FFX/20261003_audit/files/ffx_ps2/ffx/master/jppc/event/obj/mc/mcfr0100/mcfr0100.ebp",std::ios::binary);
    check(input.good(),"actual archived Butterfly EV");fileBytes.assign(std::istreambuf_iterator<char>(input),{});chunkOffset=read<unsigned>(ptr(fileBytes)+4);
    const auto base=chunk();const auto count=read<uint16_t>(base+0x34);workers.assign(count,std::vector<uint8_t>(0x700));
    std::fill(actors.begin(),actors.end(),uint8_t{0});std::fill(saved.begin(),saved.end(),uint8_t{0});std::fill(variables.begin(),variables.end(),uint8_t{0});
    for(unsigned i=0;i<count;++i){put(workers[i],0,base+read<unsigned>(base+0x38+i*4));put(workers[i],4,base);put(workers[i],0x2e,static_cast<uint16_t>(i));put(workers[i],0x9c,ptr(actors)+i*0x880);put(workers[i],0x34,uint16_t{0x80});put(workers[i],0xaa,uint8_t{1});put(workers[i],0xab,uint8_t{4});}
    put(controller,0xa,uint16_t{0});put(controller,0xc,count);put(controller,0x2c,ptr(saved));invalidateNativeWorkerIndex();
    body(0,0,0);put(actors,0,uint16_t{1});body(20,10,0);body(21,0,15,true);set(37,50,9);set(37,51,29);set(37,52,0);
    Event loadEvent;loadEvent.args[0]=12;loadEvent.nativeStack=0x900000;loadEvent.epoch=generation();
    Event allocation;allocation.caller=0x48d2a4;allocation.edi=static_cast<unsigned>(fileBytes.size());allocation.args[0]=(allocation.edi+0x7ffu)&~0x7ffu;allocation.result=ptr(fileBytes);allocation.epoch=generation();
    {Cost cost;for(const auto& l:listeners.at(0x48d230))if(l.before)l.before(loadEvent);for(const auto& l:listeners.at(0x2871c0))if(l.before)l.before(allocation);for(const auto& l:listeners.at(0x2871c0))if(l.after)l.after(allocation);loadEvent.result=ptr(fileBytes);for(const auto& l:listeners.at(0x48d230))if(l.after)l.after(loadEvent);}
}
void prepareResource(){std::thread prepare([]{drainAtelResourcesForTest();});prepare.join();}
}
void* operator new(size_t n){if(measuring)++allocations;if(auto p=std::malloc(n?n:1))return p;throw std::bad_alloc();}
void operator delete(void* p)noexcept{if(p&&measuring)++frees;std::free(p);}
void operator delete(void* p,size_t)noexcept{::operator delete(p);}
namespace ffx {
std::atomic<int> scene{0};unsigned generation(){return speechq::generation();}bool nativeGameThread(){return GetCurrentThreadId()==nativeOwner;}
bool copy(uintptr_t from,void* to,size_t bytes){owner();__try{std::memcpy(to,reinterpret_cast<void*>(from),bytes);return true;}__except(1){return false;}}
uintptr_t address(unsigned rva){
    owner();if(rva==0x46a7c0)return reinterpret_cast<uintptr_t>(&worker);if(rva==0x46c500)return reinterpret_cast<uintptr_t>(&variable);
    if(rva==0x4767e0)return reinterpret_cast<uintptr_t>(&priority);if(rva==0x476360)return reinterpret_cast<uintptr_t>(&entry);
    if(rva==0xf26ae8){static uintptr_t p;p=ptr(controller);return reinterpret_cast<uintptr_t>(&p);}
    if(rva==0xd2ca90)return ptr(saved)-0x1ec;
    if(rva==0x1fc44e4){static uintptr_t p;p=ptr(actors);return reinterpret_cast<uintptr_t>(&p);}
    if(rva==0x1fc44e0){static unsigned n=128;return reinterpret_cast<uintptr_t>(&n);}
    if(rva==0x48d230||rva==0x48d490||rva==0x2871c0)return rva;check(false,"unexpected affine binding");return 0;
}
bool hook(unsigned rva,Handler before,Handler after){listeners[rva].push_back({before,after});return true;}
bool hookSnapshot(unsigned rva,Handler before,Handler after){return hook(rva,before,after);}
bool gameFocused(){owner();return focused;}bool modMenuActive(){owner();return false;}bool utilityConsoleActive(){owner();return false;}bool inBattle(){owner();return false;}
uintptr_t playerActor(){owner();return ptr(actors);}Point playerPosition(){owner();return {read<float>(ptr(actors)+0xc),read<float>(ptr(actors)+0x10),read<float>(ptr(actors)+0x14)};}
bool cameraBasis(Point& forward,Point& right){owner();forward={1,0,0};right={0,0,1};return true;}
int setting(Setting id){return id==Setting::Speech?1:id==Setting::ButterflyCues?enabled:0;}
bool output(const wchar_t* s,bool interrupt,const char* p){return speechq::output(s,std::wcslen(s),interrupt,p,false,generation());}
bool output(const std::wstring& s,bool interrupt,const char* p){return speechq::output(s.data(),s.size(),interrupt,p,false,generation());}
double distanceXZ(Point a,Point b){return std::hypot(a.x-b.x,a.z-b.z);}void log(const char*,...){}
void publishButterflyCue(const ButterflyCue& c){if(c.epoch==generation()){++cueEvents;cueActive=c.active;}}
void preparePedestalZone(Entity&,uintptr_t){check(false,"legacy pedestal processing is outside this capture fixture");}
void refreshSwimmingGoal(Entity&){check(false,"legacy swimming processing is outside this capture fixture");}
}
namespace ffx::speechq {void captureBeforeDrainForTest(){if(held.load()){SetEvent(preparationHeld);WaitForSingleObject(released,INFINITE);}}}
int main(){
    nativeOwner=GetCurrentThreadId();released=CreateEventW(nullptr,TRUE,TRUE,nullptr);preparationHeld=CreateEventW(nullptr,TRUE,FALSE,nullptr);const auto backend=CreateEventW(nullptr,FALSE,FALSE,nullptr);
    check(released&&preparationHeld&&backend,"controlled barrier events");check(speechq::start(GetModuleHandleW(nullptr),backend),"production preparation/sink startup");check(startAtelResources(),"production owned resource startup");installAtelResources();installButterfly();
    load();const auto accepted=std::chrono::steady_clock::now();change(0,26,1);change(20,27,1);
    wait([]{return !speechq::capturesPending();},"pending first Butterfly frames copied into preparation");check(collected.empty(),"pending resource cannot vocalize unqualified first events");
    const auto code=read<unsigned>(chunk()+0x30);fileBytes[chunkOffset+code]^=1;set(0,27,99);prepareResource();
    wait([]{return collected.size()==2;},"first copied start/catch resume after resource-ready wake");
    const auto firstUsable=std::chrono::duration<double,std::milli>(std::chrono::steady_clock::now()-accepted).count();
    check(collected[0].text==L"Butterfly hunt started"&&collected[1].text==L"Blue butterfly. 1 of 7","real FIFO uses original captured facts after engine source overwrite");
    hold();load();prepareResource();change(0,26,1);resume();wait([]{return collected.size()==1&&cueActive.load();},"fresh prepared hunt audio and headline");collected.clear();
    hold();query();query();resume();wait([]{return collected.size()==4;},"equal direction queries plus hazard warnings retained");check(collected[0].text==collected[2].text&&collected[1].text==L"Red butterfly nearby"&&collected[1].text==collected[3].text,"actual equal output is never deduplicated");collected.clear();
    hold();query();speechq::interrupt();resume();wait([]{return !speechq::capturesPending();},"canceled query drains");Sleep(20);takeReady();check(collected.empty(),"actual original cancellation anchor remains silent");
    query();wait([]{return collected.size()==2;},"fresh query after cancellation");collected.clear();
    speechq::invalidate("graphics-reset");resetButterfly();change(20,27,1);wait([]{return !speechq::capturesPending()&&cueActive.load();},"live allocation requalifies after graphics reset without recapture");takeReady();collected.clear();
    hold();for(unsigned i=0;i<64;++i)query();query();resume();wait([]{return collected.size()==129;},"all64 accepted equal queries plus one explicit overload report");
    unsigned directions=0,hazards=0,reports=0;for(const auto& item:collected){if(item.text.find(L"Blue butterfly:")==0)++directions;if(item.text==L"Red butterfly nearby")++hazards;if(item.text.find(L"queue is full")!=std::wstring::npos)++reports;}
    check(directions==64&&hazards==64&&reports==1,"actual bounded FIFO pressure keeps every accepted direction and hazard");collected.clear();
    focused=false;commit();check(!cueActive.load(),"Native focus loss cancels cue immediately");wait([]{return !speechq::capturesPending();},"focus frame processed");focused=true;
    hold();query();speechq::invalidate("stale-query");resetButterfly();resume();wait([]{return !speechq::capturesPending();},"old epoch central dispatch retires pending query");Sleep(20);takeReady();check(collected.empty(),"stale query cannot restore audio/speech");
    check(!allocations&&!frees&&!foreignReads&&entryCalls&&entryCalls==priorityCalls,"actual capture/native getters heap0/0 and Worker engine reads0");
    std::printf("PASS: actual Butterfly Native/resource/interaction/domain/capture/preparation/sink, pending first events, source reuse, equal queries, canceled headers,64-pressure, graphics lifetime, focus/stale audio. Native heap0/0, Worker engine reads0.\nMETRIC controlled host calls=%u total=%.3fus max=%.3fus; first available=%.3fms including deliberate resource hold; fixed entry/priority getter calls=%u/%u. No SDK/audible/gameplay/whole-mod claim.\n",nativeCalls,nativeUs,nativeMaxUs,firstUsable,entryCalls,priorityCalls);std::fflush(stdout);ExitProcess(0);
}
