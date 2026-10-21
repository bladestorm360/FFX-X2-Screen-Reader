#include "external_fixture.hpp"
#include <atomic>
#include "../butterfly.hpp"
#include "../events.hpp"
#include "../settings.hpp"
#include "../navigation_targets.hpp"
#include "ready_speech_adapter.hpp"
#include "minigame_domain_adapter.hpp"
#include "../atel_resources.hpp"
#include <map>
#include <fstream>
#include <cstdio>
#include <cstdlib>
#include <cmath>
#include <cstring>
#include <stdexcept>
namespace {
using namespace ffx;
std::vector<uint8_t> chunk,actors(128*0x880),saved(0x66d4),variables(0x2000),controller(0x200);
size_t nativeChunkOffset=0;
std::vector<std::vector<uint8_t>> workers;
struct Listener { Handler before,after; };
std::map<unsigned,std::vector<Listener>> listeners;
unsigned epoch=1; bool focused=true,menu=false,battle=false,enabled=true,callback=true;
thread_local bool workerDrain=false;
ButterflyCue cue; std::vector<std::wstring> spoken;
void check(bool good,const char* text) { if(!good) { fprintf(stderr,"FAIL: %s\n",text); exit(1); } }
uintptr_t ptr(std::vector<uint8_t>& v) { return reinterpret_cast<uintptr_t>(v.data()); }
uintptr_t chunkPtr() { return ptr(chunk)+nativeChunkOffset; }
template<class T> void put(std::vector<uint8_t>& v,size_t at,T value) { check(at+sizeof(value)<=v.size(),"fixture write bounds"); memcpy(v.data()+at,&value,sizeof(value)); }
uintptr_t __cdecl worker(int index) { check(!workerDrain,"Worker game worker lookup is forbidden"); return index>=0 && static_cast<size_t>(index)<workers.size()?ptr(workers[index]):0; }
uintptr_t __cdecl geometry(uintptr_t w) { check(!workerDrain,"Worker game geometry getter is forbidden"); return w+0x558; }
uintptr_t __cdecl variable(uintptr_t w,int index,int element) {
    check(!workerDrain,"Worker game variable getter is forbidden");
    const auto h=read<uintptr_t>(w),c=read<uintptr_t>(w+4);
    const auto low=read<unsigned>(c+read<unsigned>(h+0x14)+index*8);
    const unsigned size=low>>28==0?1u:4u;
    if((low>>25&7)==3) return w+0x300+(low&0xffffff)+element*size;
    return ((low>>25&7)==6?ptr(variables):ptr(saved))+(low&0xffffff)+element*size;
}
void set(unsigned local,unsigned index,int v) {
    const auto w=worker(static_cast<int>(local)),p=variable(w,index,0);
    const unsigned low=read<unsigned>(chunkPtr()+read<unsigned>(read<uintptr_t>(w)+0x14)+index*8);
    if(low>>28==0) write<uint8_t>(p,static_cast<uint8_t>(v)); else write<int>(p,v);
}
void change(unsigned local,unsigned index,int v,bool drainDomain=true) {
    Event event; event.rva=0x4713a0; event.epoch=epoch; event.args[0]=worker(static_cast<int>(local));event.args[1]=index;
    for(const auto& l:listeners[event.rva]) if(l.before) l.before(event);
    const auto before=spoken.size();set(local,index,v);check(spoken.size()==before,"native event cannot speak before explicit domain drain");
    for(const auto& l:listeners[event.rva]) if(l.after) l.after(event);
    check(spoken.size()==before,"native callback cannot speak before explicit domain drain");if(drainDomain){workerDrain=true;speechq::fixture::drain();workerDrain=false;}
}
void commit(bool drainDomain=true,bool allowOverload=false) { Event e;e.epoch=epoch;const auto before=spoken.size();for(const auto& l:listeners[0x43d3b0]) if(l.after) l.after(e);check(spoken.size()==before || allowOverload,"native movement callback defers domain speech until drain");if(drainDomain){workerDrain=true;speechq::fixture::drain();workerDrain=false;} }
void refreshAudio() {const auto before=spoken.size();refreshButterflyAudio();check(spoken.size()==before,"audio refresh defers domain speech");workerDrain=true;speechq::fixture::drain();workerDrain=false;}
void directions(bool hazards=false,bool drainDomain=true) {const auto before=spoken.size();butterflyDirections(hazards);check(spoken.size()==before,"direction query defers domain speech");if(drainDomain){workerDrain=true;speechq::fixture::drain();workerDrain=false;}}
void status(bool drainDomain=true) {const auto before=spoken.size();butterflyStatus();check(spoken.size()==before,"status query defers domain speech");if(drainDomain){workerDrain=true;speechq::fixture::drain();workerDrain=false;}}
void actor(unsigned local,float x,float z,bool red=false) {
    const unsigned at=local*0x880;
    put(actors,at,static_cast<uint16_t>(0x5008));put(actors,at+2,static_cast<uint16_t>(1));
    put(actors,at+0xc,x);put(actors,at+0x10,0.f);put(actors,at+0x14,z);
    put(actors,at+0x334,red?.9f:.1f);put(actors,at+0x340,red?.1f:.3f);put(actors,at+0x34c,red?.1f:1.f);
    put(actors,at+0x354,1.f);put(actors,at+0x358,1.f);put(actors,at+0x5c,1.f);put(actors,at+0x64,1.f);put(actors,at+0x4e4,2.f);
    put(workers[local],0x558,x);put(workers[local],0x55c,0.f);put(workers[local],0x560,z);
}
void load(bool north,bool prepare=true,bool changedCode=false) {
    resetButterfly();++epoch;focused=true;menu=battle=false;enabled=callback=true;
    const std::string name=north?"mcfr0200":"mcfr0100";
    if(!chunk.empty()) { Event freed;freed.args[0]=ptr(chunk);for(const auto& l:listeners.at(0x48d490)) if(l.before) l.before(freed); }
    std::ifstream file((ffx_fixture::evidenceRoot()/"offline-output/FFX/20261003_audit/files/ffx_ps2/ffx/master/jppc/event/obj/mc").string()+"/"+name+"/"+name+".ebp",std::ios::binary);
    check(file.good(),"actual archived butterfly source");std::vector<uint8_t> raw((std::istreambuf_iterator<char>(file)),{});
    const auto start=read<unsigned>(ptr(raw)+4);unsigned end=static_cast<unsigned>(raw.size());
    for(unsigned i=1;i<11;++i) {const auto v=read<unsigned>(ptr(raw)+4+i*4);if(v && v>=start && v<=raw.size()) {end=v;break;}}
    check(start<end,"archived script chunk");chunk=std::move(raw);nativeChunkOffset=start;
    const auto parent=ptr(chunk),base=chunkPtr();
    if(changedCode) {const auto code=read<unsigned>(base+0x30);check(start+code<chunk.size(),"changed bytecode fixture bound");chunk[start+code]^=1;}
    Event loadEvent;loadEvent.args[0]=12;loadEvent.nativeStack=0x900000;loadEvent.epoch=epoch;
    check(listeners.at(0x48d230).size()==1 && listeners.at(0x2871c0).size()==1,"single ATEL receipt listener in fixture");
    listeners.at(0x48d230).front().before(loadEvent);
    Event allocation;allocation.caller=address(0x48d230)+0x74;allocation.edi=static_cast<unsigned>(chunk.size());
    allocation.args[0]=(allocation.edi+0x7ffu)&~0x7ffu;allocation.result=parent;allocation.epoch=epoch;
    listeners.at(0x2871c0).front().before(allocation);listeners.at(0x2871c0).front().after(allocation);
    loadEvent.result=parent;listeners.at(0x48d230).front().after(loadEvent);
    const auto count=read<uint16_t>(base+0x34);workers.assign(count,std::vector<uint8_t>(0x700));
    std::fill(actors.begin(),actors.end(),uint8_t{0});std::fill(saved.begin(),saved.end(),uint8_t{0});std::fill(variables.begin(),variables.end(),uint8_t{0});
    for(unsigned i=0;i<count;++i) {put(workers[i],0,base+read<unsigned>(base+0x38+i*4));put(workers[i],4,base);put(workers[i],0x2e,static_cast<uint16_t>(i));put(workers[i],0x9c,ptr(actors)+i*0x880);put(workers[i],0x34,static_cast<uint16_t>(0x80));put(workers[i],0xab,uint8_t{4});put(workers[i],0xaa,uint8_t{1});put(workers[i],0x44,uintptr_t{});}
    put(controller,0xa,uint16_t{0});put(controller,0xc,count);put(controller,0x2c,ptr(saved));
    actor(0,0,0);put(actors,0,static_cast<uint16_t>(1));
    actor(north?10u:20u,10,0);actor(north?11u:21u,0,15,true);
    set(north?26u:37u,north?35u:50u,9);set(north?26u:37u,north?36u:51u,29);set(north?26u:37u,north?37u:52u,0);
    spoken.clear();cue={};
    if(prepare) {workerDrain=true;drainAtelResourcesForTest();speechq::fixture::drain();workerDrain=false;}
}
}
namespace ffx {
std::atomic<int> scene{0};
unsigned generation() { return epoch; }bool gameFocused() {return focused;}bool modMenuActive() {return menu;}bool utilityConsoleActive() {return false;}bool inBattle() {return battle;}
bool nativeGameThread() {return !workerDrain;}
bool copy(uintptr_t s,void* d,size_t n) {check(!workerDrain,"Worker native copy is forbidden");__try {memcpy(d,reinterpret_cast<void*>(s),n);return true;}__except(1){return false;}}
uintptr_t playerActor() {check(!workerDrain,"Worker player actor getter is forbidden");return ptr(actors);}Point playerPosition() {check(!workerDrain,"Worker player position getter is forbidden");return {read<float>(ptr(actors)+0xc),read<float>(ptr(actors)+0x10),read<float>(ptr(actors)+0x14)};}
uintptr_t address(unsigned rva) {
    if(rva==0x46a7c0)return reinterpret_cast<uintptr_t>(worker);if(rva==0x46c500)return reinterpret_cast<uintptr_t>(variable);if(rva==0x46c240)return reinterpret_cast<uintptr_t>(geometry);
    if(rva==0xf26ae8){static uintptr_t p;p=ptr(controller);return reinterpret_cast<uintptr_t>(&p);}if(rva==0xd2ca90)return ptr(saved)-0x1ec;if(rva==0x1fc44e4){static uintptr_t p;p=ptr(actors);return reinterpret_cast<uintptr_t>(&p);}if(rva==0x1fc44e0){static unsigned n=128;return reinterpret_cast<uintptr_t>(&n);}if(rva==0x48d230||rva==0x2871c0||rva==0x48d490)return rva;throw std::runtime_error("unexpected dependency");
}
bool hook(unsigned rva,Handler a,Handler b){listeners[rva].push_back({a,b});return true;}bool hookSnapshot(unsigned rva,Handler a,Handler b){return hook(rva,a,b);}void log(const char*,...){}bool output(const std::wstring& s,bool,const char*){spoken.push_back(s);return true;}
const NavigationScript* navigationCatalog(const AtelScript&){return nullptr;}
void publishButterflyCue(const ButterflyCue& c){cue=c;}int setting(Setting id){return id==Setting::ButterflyCues?enabled:0;}
double distanceXZ(Point a,Point b){return std::hypot(a.x-b.x,a.z-b.z);}std::wstring direction(Point p){return p.x>0?L"north":p.z>0?L"east":L"here";}
bool cameraBasis(Point& forward,Point& right){check(!workerDrain,"Worker camera getter is forbidden");forward={1,0,0};right={0,0,1};return true;}
std::vector<unsigned> interactionEntries(uintptr_t,EntryMask m,unsigned* channels){if(channels)*channels=callback?4u:0u;return callback && m.contains(4)?std::vector<unsigned>{4}:std::vector<unsigned>{};}
NativeInteractionFacts captureNativeInteractionFacts(uintptr_t w,EntryMask m){NativeInteractionFacts facts;if(!nativeGameThread()||!w||!callback||!m.contains(4))return facts;facts.channels=4;facts.entries[2]=4;return facts;}
double actorContactRadius(uintptr_t a,uintptr_t p){const auto r=[](uintptr_t b){return (read<float>(b+0x5c)+read<float>(b+0x64))*.5*read<float>(b+0x4e4);};return r(a)+r(p);}
}
int main(){
    using namespace ffx;speechq::fixture::spoken=&spoken;check(startAtelResources(),"ATEL owner startup");installAtelResources();installButterfly();
    for(bool north:{false,true}) {
        load(north);const unsigned active=north?20u:26u,count=north?21u:27u,first=north?10u:20u,timer=north?26u:37u,time=north?35u:50u;
        const unsigned start=north?9u:19u;
        actor(start,-25,0); actor(start-1,-20,0);
        Entity decoration; configureButterflyStarter(decoration,north?"mcfr0200":"mcfr0100",start-1,worker(static_cast<int>(start-1)));
        check(!decoration.butterflyStart,"decorative rainbow companion cannot become hunt start");
        Entity starter; starter.id=0x5008;
        configureButterflyStarter(starter,north?"mcfr0200":"mcfr0100",start,worker(static_cast<int>(start)));
        check(starter.butterfly && starter.butterflyStart && !starter.hazardous && starter.name==L"Butterfly hunt start" && !starter.inactive && starter.zone.valid && !butterflyActive(),"actual archived starter has fresh physical zone outside hunt");
        check(starter.id==0x5008 && starter.worker==static_cast<int>(start) && starter.zone.range==4,"starter retains original model ID and verified global worker");
        check(starter.distance==distanceXZ(playerPosition(),starter.position),"starter distance retains native navigation units");
        put(actors,start*0x880+0x194,0x100u);refreshButterflyStarter(starter);check(starter.inactive && !starter.zone.valid,"starter body disable cancels target");put(actors,start*0x880+0x194,0u);
        callback=false;refreshButterflyStarter(starter);check(starter.inactive,"starter lost touch callback cancels target");callback=true;
        put(actors,start*0x880+0x358,0.f);refreshButterflyStarter(starter);check(starter.inactive,"starter fading away cancels target");put(actors,start*0x880+0x358,1.f);
        refreshButterflyStarter(starter);check(!starter.inactive && starter.id==0x5008,"starter becomes available again through fresh native state");
        Entity synthetic=starter;synthetic.id=0xb000u+start;refreshButterflyStarter(synthetic);check(!synthetic.inactive && synthetic.id==0xb000u+start,"copied starter retains synthetic stable ID");
        std::swap(workers[start],workers[start+1]);Entity moved;moved.id=0x5008;configureButterflyStarter(moved,north?"mcfr0200":"mcfr0100",start,worker(static_cast<int>(start+1)));check(!moved.inactive && moved.worker==static_cast<int>(start+1),"starter resolves global worker rather than local alias");refreshButterflyStarter(moved);check(!moved.inactive,"starter refresh honors verified global worker");std::swap(workers[start],workers[start+1]);
        auto unrelated=chunk; const auto nameOffset=read<unsigned>(chunkPtr()+0xc);
        memcpy(unrelated.data()+nativeChunkOffset+nameOffset,"other000",8);
        put(workers[0],4,ptr(unrelated)+nativeChunkOffset); Event foreign; foreign.rva=0x4713a0;
        foreign.epoch=epoch; foreign.args[0]=worker(0); foreign.args[1]=active;
        for(const auto& listener:listeners.at(foreign.rva)) if(listener.before) listener.before(foreign);
        put(workers[0],4,chunkPtr());
        change(0,active,1);refreshButterflyStarter(starter);check(starter.inactive && !starter.zone.valid,"actual hunt activation cancels starter after touch");check(butterflyActive(),"actual archived script identity activates hunt");check(spoken.back()==L"Butterfly hunt started","native active transition starts hunt");
        auto list=butterflyTargets();check(list.size()==2 && list[0].name==L"Blue butterfly" && list[1].name==L"Red butterfly","fresh renderer classifies authored actors");check(list[0].zone.range==4 && list[0].zone.contactRadius==0,"native combined physical contact radius");
        check(list[0].distance==distanceXZ(playerPosition(),list[0].position) && list[1].distance==distanceXZ(playerPosition(),list[1].position),"blue and red distances retain native navigation units");
        check(cue.active && cue.hazard,"near red hazard overrides blue pickup cue");
        directions();check(spoken[spoken.size()-2].find(L"0.9")!=std::wstring::npos,"spoken butterfly distance converts native units to steps");
        status();check(spoken.back().find(L"29.9 seconds")!=std::wstring::npos,"actual signed local native countdown");
        for(int caught=1;caught<=7;++caught){change(first,count,caught);check(spoken.back()==L"Blue butterfly. "+std::to_wstring(caught)+L" of 7","each producer collection increment spoken");}
        const auto messages=spoken.size();change(first,count,7);check(spoken.size()==messages,"same value is not a collection event");
        put(actors,first*0x880+0x358,0.f);commit();check(butterflyTargets().size()==1,"native fade target immediately removes collected blue");
        actor(first,10,0,true);commit();check(butterflyTargets()[0].name==L"Red butterfly","same worker changes to red in another story layout");
        actor(first,10,0);put(actors,first*0x880+0x334,.5f);commit();check(butterflyTargets().size()==1,"unconfirmed intermediate shade excluded");
        actor(first,10,0);put(actors,first*0x880+0x194,0x100u);commit();check(butterflyTargets().size()==1,"native collision disable removes target");
        put(actors,first*0x880+0x194,0u);callback=false;commit();check(butterflyTargets().empty(),"inactive actual touch callback excluded");callback=true;
        put(actors,(first+1)*0x880+0x4e4,-1.f);commit();list=butterflyTargets();check(list.size()==2 && list[1].hazardous && !list[1].zone.valid && !cue.active,"invalid red bounds retained and cues cancelled fail closed");put(actors,(first+1)*0x880+0x4e4,2.f);
        focused=false;refreshAudio();check(!cue.active && !butterflyActive(),"foreign focus cancels cue and context");focused=true;
        menu=true;refreshAudio();check(!cue.active,"settings menu cancels cue");menu=false;battle=true;refreshAudio();check(!cue.active,"battle cancels cue");battle=false;
        enabled=false;refreshAudio();check(!cue.active,"independent cue toggle cancels immediately");enabled=true;
        change(0,active,0);check(!butterflyActive() && !cue.active && spoken.back()==L"All seven blue butterflies caught","actual completed state ends hunt");
        change(0,count,0);change(0,active,1);change(timer,time,0);change(timer,time+1,0);change(0,active,0);check(spoken.back()==L"Butterfly hunt time expired","native time expiry announced");
        change(0,active,1);++epoch;refreshButterflyStarter(starter);check(starter.inactive && !starter.zone.valid,"generation rejects stale copied starter");check(!butterflyActive(),"generation invalidates old context");refreshAudio();check(!cue.active,"generation cancels cue");
        load(north);change(0,active,1);change(north?27u:39u,count,7);check(!butterflyActive() && !cue.active && spoken.back()==L"Butterfly hunt ended","exit sentinel is cancellation, not success");commit();check(!butterflyActive(),"exit sentinel stays cancelled until generation changes");
        load(north,true,true);change(0,active,1);check(!butterflyActive(),"modified bytecode rejected by immutable prepared identity");
        load(north);const auto h=read<uintptr_t>(worker(0)),decl=read<unsigned>(h+0x14);put(chunk,static_cast<size_t>(chunkPtr()-ptr(chunk))+decl+active*8,0x0c000040u);change(0,active,1);check(!butterflyActive(),"changed active declaration rejected");
    }
    load(false,false);change(0,26,1,false);change(20,27,1,false);
    check(spoken.empty(),"first activation and catches remain pending before resource preparation");
    const auto codeOffset=read<unsigned>(chunkPtr()+0x30);check(nativeChunkOffset+codeOffset<chunk.size(),"pending source overwrite bound");chunk[nativeChunkOffset+codeOffset]^=1;
    workerDrain=true;drainAtelResourcesForTest();speechq::fixture::drain();workerDrain=false;
    check(spoken.size()>=2 && spoken[0]==L"Butterfly hunt started" && spoken[1]==L"Blue butterfly. 1 of 7","pending first activation and catch survive owned preparation after source overwrite");
    change(20,27,2);check(spoken.back()==L"Blue butterfly. 2 of 7","catch producer remains ordered after pending start");
    const auto queryBegin=spoken.size();directions(false,false);directions(false,false);check(spoken.size()==queryBegin,"equal direction queries wait for FIFO drain");
    workerDrain=true;speechq::fixture::drain();workerDrain=false;
    check(spoken.size()==queryBegin+4 && spoken[queryBegin]==spoken[queryBegin+2] && spoken[queryBegin+1]==spoken[queryBegin+3] && spoken[queryBegin+1]==L"Red butterfly nearby","equal direction queries and hazards remain distinct FIFO commands");
    const auto canceledBegin=spoken.size();status(false);++speechq::fixture::cancellation;workerDrain=true;speechq::fixture::drain();workerDrain=false;
    check(spoken.size()==canceledBegin,"cancellation header rejects already queued speech");
    status();check(spoken.size()==canceledBegin+1,"fresh cancellation generation permits speech");
    load(false);change(0,26,1);const auto pressureBegin=spoken.size();
    for(unsigned i=0;i<65;++i)commit(false,true);
    check(spoken.size()==pressureBegin+1 && spoken.back()==L"The Butterfly request queue is full.","bounded 64-ticket pressure reports overload once");
    workerDrain=true;speechq::fixture::drain();workerDrain=false;
    check(spoken.size()==pressureBegin+1,"overload drains without replaying stale speech");
    commit();check(spoken.size()==pressureBegin+1,"queue accepts new work after bounded recovery");
    resetButterfly();check(!cue.active,"teardown cancels cue");puts("PASS: both real archived butterfly fields; collections, native timer, changing colors, collision/alpha, context, generation and exact identity guards.");
}
