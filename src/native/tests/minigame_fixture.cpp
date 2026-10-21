#include "external_fixture.hpp"
#include <atomic>
#include "../minigames.hpp"
#include "../minigame_memory.hpp"
#include "../minigame_spatial.hpp"
#include "../events.hpp"
#include "../profile.hpp"
#include "../settings.hpp"
#include "ready_speech_adapter.hpp"
#include "minigame_domain_adapter.hpp"
#include "../atel_resources.hpp"
#include <map>
#include <fstream>
#include <cstdio>
#include <cstdlib>
namespace {
using namespace ffx;
std::vector<uint8_t> chunk,actors(128*0x880),saved(0x2000),variables(0x2000),controller(0x200);
size_t nativeChunkOffset=0;
std::vector<std::vector<uint8_t>> workers;
struct Listener { Handler before,after; };
std::map<unsigned,std::vector<Listener>> listeners;
unsigned epoch=1; bool focused=true,menu=false,cueActive=false,enabled=true;
float cuePan=0,cueProximity=0; unsigned geometryReads=0;
unsigned cuePublications=0;
std::vector<std::wstring> spoken;
thread_local bool workerDrain=false;
void check(bool valid,const char* reason) { if(!valid) { fprintf(stderr,"FAIL: %s\n",reason); exit(1); } }
uintptr_t ptr(std::vector<uint8_t>& bytes) { return reinterpret_cast<uintptr_t>(bytes.data()); }
uintptr_t chunkPtr() { return ptr(chunk)+nativeChunkOffset; }
template<class T> void put(std::vector<uint8_t>& bytes,size_t offset,T value) { check(offset+sizeof(value)<=bytes.size(),"fixture write bounds"); memcpy(bytes.data()+offset,&value,sizeof(value)); }
uintptr_t __cdecl worker(int index) { check(!workerDrain,"Worker engine worker lookup is forbidden"); return index>=0 && static_cast<size_t>(index)<workers.size()?ptr(workers[index]):0; }
uintptr_t __cdecl geometry(uintptr_t w) { check(!workerDrain,"Worker engine geometry lookup is forbidden"); ++geometryReads; return w+0x558; }
uintptr_t __cdecl variable(uintptr_t w,int index,int element) {
    check(!workerDrain,"Worker engine variable lookup is forbidden");
    const auto h=read<uintptr_t>(w),c=read<uintptr_t>(w+4);
    const auto low=read<unsigned>(c+read<unsigned>(h+0x14)+index*8);
    const auto size=low>>28==6?4u:1u;
    return ((low>>25&7)==6?ptr(variables):ptr(saved))+(low&0xffffff)+element*size;
}
void set(const MinigameBinding& binding,float value,unsigned element=0) {
    const auto p=variable(worker(0),binding.index,element);
    if(binding.low>>28==6) write<float>(p,value); else write<uint8_t>(p,static_cast<uint8_t>(value));
}
void change(const MinigameBinding& binding,unsigned value,unsigned element=0) {
    Event event; event.rva=0x4713a0; event.args[0]=worker(0); event.args[1]=binding.index; event.args[2]=element; event.epoch=epoch;
    const auto& list=listeners.at(event.rva);
    for(const auto& listener:list) if(listener.before) listener.before(event);
    set(binding,static_cast<float>(value),element);
    for(const auto& listener:list) if(listener.after) listener.after(event);
}
void load(const char* source,bool prepare=true,bool changedCode=false) {
    resetMinigames(); ++epoch;
    const std::string path=(ffx_fixture::evidenceRoot()/"offline-output/FFX/20261003_audit/files").string()+"/"+source;
    std::ifstream file(path,std::ios::binary); check(file.good(),"archived minigame source");
    std::vector<uint8_t> raw((std::istreambuf_iterator<char>(file)),std::istreambuf_iterator<char>());
    const auto start=read<unsigned>(reinterpret_cast<uintptr_t>(raw.data())+4);
    unsigned end=static_cast<unsigned>(raw.size());
    for(unsigned i=1;i<11;++i) { const auto value=read<unsigned>(reinterpret_cast<uintptr_t>(raw.data())+4+i*4); if(value && value>=start && value<=raw.size()) { end=value; break; } }
    check(start<end && end<=raw.size(),"archived EBP chunk"); chunk=std::move(raw);nativeChunkOffset=start;
    const auto parent=ptr(chunk),base=chunkPtr();
    if(changedCode) {const auto code=read<unsigned>(base+0x30);check(start+code<chunk.size(),"changed fixture code byte bound");chunk[start+code]^=1;}
    Event loadEvent;loadEvent.args[0]=12;loadEvent.nativeStack=0x900000;loadEvent.epoch=epoch;
    check(listeners.at(0x48d230).size()==1 && listeners.at(0x2871c0).size()==1,"single exact resource listener in this host");
    listeners.at(0x48d230).front().before(loadEvent);
    Event allocation;allocation.caller=address(0x48d230)+0x74;allocation.edi=static_cast<unsigned>(chunk.size());
    allocation.args[0]=(allocation.edi+0x7ffu)&~0x7ffu;allocation.result=parent;allocation.epoch=epoch;
    listeners.at(0x2871c0).front().before(allocation);listeners.at(0x2871c0).front().after(allocation);
    loadEvent.result=parent;listeners.at(0x48d230).front().after(loadEvent);
    const auto count=read<uint16_t>(base+0x34);
    workers.assign(count,std::vector<uint8_t>(0x600));
    std::fill(actors.begin(),actors.end(),uint8_t{0}); std::fill(saved.begin(),saved.end(),uint8_t{0}); std::fill(variables.begin(),variables.end(),uint8_t{0});
    for(unsigned i=0;i<count;++i) {
        put(workers[i],0,base+read<unsigned>(base+0x38+i*4)); put(workers[i],4,base);
        put(workers[i],0x2e,static_cast<uint16_t>(i));
        if(i<128) { put(workers[i],0x9c,ptr(actors)+i*0x880); put(actors,i*0x880,static_cast<uint16_t>(1)); put(actors,i*0x880+2,static_cast<uint16_t>(1)); }
        put(workers[i],0x558,static_cast<float>(i*10)); put(workers[i],0x55c,0.f); put(workers[i],0x560,0.f);
        put(workers[i],0x5a0,10.f); put(workers[i],0x574,6.f);
    }
    put(controller,0xa,static_cast<uint16_t>(0)); put(controller,0xc,count);
    spoken.clear(); cueActive=false;
    if(prepare) {workerDrain=true;drainAtelResourcesForTest();speechq::fixture::drain();workerDrain=false;}
}
void move(unsigned local,float x,float z) { put(workers[local],0x558,x); put(workers[local],0x560,z); }
void commit() { Event event; event.rva=0x43d3b0; event.epoch=epoch; for(const auto& listener:listeners.at(event.rva)) if(listener.after) listener.after(event); }
void drain() { workerDrain=true;speechq::fixture::drain();workerDrain=false; }
void changeAndDrain(const MinigameBinding& binding,unsigned value,unsigned element=0) { const auto before=spoken.size();change(binding,value,element);check(spoken.size()==before,"domain receiver is deferred until explicit drain");drain(); }
bool dispatchAndDrain(unsigned vk,bool repeat=false) { const auto before=spoken.size();const auto consumed=dispatchMinigameKey(vk,repeat);check(spoken.size()==before,"key domain receiver is deferred until explicit drain");drain();return consumed; }
void refreshAndDrain() { refreshMinigameAudio();drain(); }
void commitAndDrain() { commit();drain(); }
}
namespace ffx {
std::atomic<int> scene{0};
unsigned generation() { return epoch; }
// Resource preparation is the only simulated background phase in this host.
bool nativeGameThread() { return !workerDrain; }
bool gameFocused() { return focused; }
bool modMenuActive() { return menu; }
bool utilityConsoleActive() { return false; }
bool inBattle() { return false; }
bool butterflyActive() { return false; }
void refreshButterflyAudio() {}
void butterflyDirections(bool) {}
void butterflyStatus() {}
uintptr_t playerActor() { return ptr(actors); }
bool copy(uintptr_t source,void* target,size_t size) { check(!workerDrain,"Worker native copy is forbidden"); __try { memcpy(target,reinterpret_cast<void*>(source),size); return true; } __except(1) { return false; } }
uintptr_t address(unsigned rva) {
    if(rva==0x48d230) return rva;
    // Both supported profiles resolve through the shared profile machinery.
    const auto resolved=profileRva(rva);
    if(resolved==profileRva(0x46a7c0)) return reinterpret_cast<uintptr_t>(worker);
    if(resolved==profileRva(0x46c500)) return reinterpret_cast<uintptr_t>(variable);
    if(rva==0x46c240) return reinterpret_cast<uintptr_t>(geometry);
    if(resolved==profileRva(0xf26ae8)) { static uintptr_t p; p=ptr(controller); return reinterpret_cast<uintptr_t>(&p); }
    if(resolved==profileRva(0x1fc44e4)) { static uintptr_t p; p=ptr(actors); return reinterpret_cast<uintptr_t>(&p); }
    if(resolved==profileRva(0x1fc44e0)) { static unsigned count=128; return reinterpret_cast<uintptr_t>(&count); }
    throw std::runtime_error("Unexpected minigame native address");
}
bool hook(unsigned rva,Handler before,Handler after) { listeners[rva].push_back({before,after}); return true; }
bool hookSnapshot(unsigned rva,Handler before,Handler after) { return hook(rva,before,after); }
void log(const char*,...) {}
bool output(const std::wstring& text,bool,const char*) { spoken.push_back(text); return true; }
void publishPositionalCue(float pan,float proximity,bool active,unsigned requested) { if(requested==epoch) {++cuePublications;cuePan=pan;cueProximity=proximity;cueActive=active;} }
int setting(Setting id) { return id==Setting::RaceCues?enabled:0; }
void adjustSetting(Setting id,int) { if(id==Setting::RaceCues) enabled=!enabled; }
std::wstring settingValue(Setting) { return enabled?L"on":L"off"; }
std::wstring direction(Point p) { return p.x>0?L"north":p.x<0?L"south":L"here"; }
double distanceXZ(Point a,Point b) { return std::hypot(a.x-b.x,a.z-b.z); }
bool cameraBasis(Point& forward,Point& right) { forward={1,0,0};right={0,0,-1};return true; }
}
int main(int argc,char**) {
    using namespace ffx;
    if(argc>1) nativeProfile=NativeProfile::Installed;
    // Geometry's added extension is normally populated by signature resolution.
    extendedAddresses[0x46c240]=0x46c240;
    speechq::fixture::spoken=&spoken;
    check(startAtelResources(),"ATEL fixture owner startup");installAtelResources();
    installMinigames();
    const auto spatial=pickupSolution({0,0,0},{100,0,-10},0,10,6);
    check(spatial.valid && spatial.pan==1 && !spatial.aligned,"native pickup radius boundary");
    check(!pickupSolution({0,0,0},{100,7,0},0,10,6).valid,"native pickup height rejects another floor");
    load("ffx_ps2/ffx/master/jppc/event/obj/bl/bltz0000/bltz0000.ebp",false);
    set(blitzBindings[3],2);set(blitzBindings[2],0);set(blitzBindings[4],2);
    change(blitzBindings[0],0);change(blitzBindings[0],1);change(blitzBindings[0],1);change(blitzBindings[0],2);
    drain();check(spoken.empty() && !minigameCapturesKey(VK_F1),"first accepted frames remain pending without unqualified output");
    set(blitzBindings[0],99);std::fill(chunk.begin(),chunk.end(),uint8_t{0xa5});
    workerDrain=true;drainAtelResourcesForTest();speechq::fixture::drain();workerDrain=false;
    check(spoken.size()==4 && spoken[0]==L"Goal for your team" && spoken[2]==spoken[0] &&
        spoken[1].find(L"team 1")!=std::wstring::npos && spoken[3].find(L"team 2")!=std::wstring::npos && minigameCapturesKey(VK_F5),
        "held first events/equal distinct goals use complete captured facts after engine source overwrite");
    load("ffx_ps2/ffx/master/jppc/event/obj/bl/bltz0000/bltz0000.ebp");
    const auto blitz=minigameScript(worker(0)); check(blitz.kind==MinigameKind::Blitz,"large native match script checksum");
    set(blitzBindings[3],2); set(blitzBindings[2],0); set(blitzBindings[4],2);
    changeAndDrain(blitzBindings[0],0);
    check(minigameCapturesKey(VK_F1) && minigameCapturesKey(VK_F5) && !minigameCapturesKey(VK_F6),"Auto mode owns only its five context keys");
    changeAndDrain(blitzBindings[0],1); check(spoken.size()==2 && spoken[0]==L"Goal for your team","real score setter announces goal");
    const auto beforeHeldQuery=spoken.size();check(dispatchMinigameKey(VK_F5,false),"accepted copied score query");
    set(blitzBindings[0],7);drain();check(spoken.size()==beforeHeldQuery+1 && spoken.back().find(L"team 1")!=std::wstring::npos,"held query cannot read a later native counter");
    set(blitzBindings[0],1);const auto beforeEqual=spoken.size();dispatchMinigameKey(VK_F5,false);dispatchMinigameKey(VK_F5,false);drain();
    check(spoken.size()==beforeEqual+2 && spoken[beforeEqual]==spoken[beforeEqual+1],"equal distinct accepted queries retained");
    const auto beforeCancel=spoken.size();dispatchMinigameKey(VK_F5,false);++speechq::fixture::cancellation;drain();
    check(spoken.size()==beforeCancel,"held original speech anchor remains canceled");
    dispatchAndDrain(VK_F5);check(spoken.size()==beforeCancel+1,"fresh query after cancellation remains available");
    const auto beforePressure=spoken.size();
    for(unsigned i=0;i<128;++i) check(dispatchMinigameKey(VK_F5,false),"full accepted snapshot FIFO request");
    check(speechq::fixture::fifo.size()==128,"all128 accepted snapshots held independently");
    check(dispatchMinigameKey(VK_F5,false) && spoken.size()==beforePressure+1 && spoken.back().find(L"queue is full")!=std::wstring::npos,
        "over-capacity request explicitly reports pressure without entering worker code");
    drain();check(spoken.size()==beforePressure+129 && minigameCapturesKey(VK_F5),"complete accepted query order and latest fact recovery after pressure");
    for(unsigned i=0;i<128;++i) check(spoken[beforePressure+1+i].find(L"team 1")!=std::wstring::npos,"accepted copied score preserved under pressure");
    const auto count=spoken.size(); changeAndDrain(blitzBindings[0],1); check(spoken.size()==count,"same native state causes no new event");
    changeAndDrain(blitzBindings[2],100); dispatchAndDrain(VK_F1,false); check(spoken[count].find(L"loose")!=std::wstring::npos,"in-flight ball query preserves sentinel");
    changeAndDrain(blitzBindings[3],10); changeAndDrain(blitzBindings[2],0); check(spoken.back()==L"Opponents have the ball","turnover through flight is announced");
    changeAndDrain(blitzBindings[5],1); check(!minigameCapturesKey(VK_F1),"manual mode releases command-mode keys");
    load("ffx_ps2/ffx/master/jppc/event/obj/na/nagi0000/nagi0000.ebp");
    const auto course=minigameScript(worker(0)); check(course.kind==MinigameKind::Race,"race checksum and variant");
    put(actors,0,static_cast<uint16_t>(0x5033)); move(0,0,0);
    for(unsigned i=39;i<59;++i) { put(actors,i*0x880,static_cast<uint16_t>(0x5063)); move(i,-1000,0); }
    move(39,100,-10); move(40,200,10);
    set(raceBindings[0],0); set(raceBindings[1],4); set(raceBindings[2],1); set(raceBindings[4],3);
    changeAndDrain(raceBindings[7],1);
    check(cueActive && cuePan==1 && cueProximity>0,"continuous cue selects nearest available balloon");
    check(minigameCapturesKey(VK_F1) && !minigameCapturesKey(VK_F2) && minigameCapturesKey(VK_F6),"race key ownership excludes other mod commands");
    changeAndDrain(raceBindings[4],2); check(cueActive && cuePan==-1,"collected balloon is removed immediately by native flag");
    changeAndDrain(raceBindings[5],1); check(spoken.back()==L"Balloon collected","native collection counter event");
    for(unsigned leg=2;leg<=4;++leg) { changeAndDrain(raceBindings[2],leg); check(cueActive,"all course legs retain race cue"); }
    const auto liveUpdates=cuePublications,liveSpeech=spoken.size(); float previousProximity=cueProximity;
    for(unsigned frame=1;frame<=64;++frame) {
        move(0,frame*.5f,frame*10.f/64); commitAndDrain();
        check(cueActive && cueProximity>=previousProximity,"consecutive movement commits update pickup continuously without a query");
        previousProximity=cueProximity;
    }
    check(cuePublications==liveUpdates+64 && cuePan==0 && spoken.size()==liveSpeech,"at-speed cue updates every accepted movement with no scan or speech requirement");
    move(0,0,10); commitAndDrain(); check(cueActive && cuePan==0,"movement commit provides exact centering at speed");
    scene=6; refreshAndDrain(); check(!cueActive && !minigameCapturesKey(VK_F1),"menu scene pauses and releases context");
    scene=0; refreshAndDrain(); check(cueActive,"scene exit restores current race cue");
    focused=false; refreshAndDrain(); check(!cueActive,"foreign focus cancels cue"); focused=true;
    menu=true; refreshAndDrain(); check(!cueActive,"mod menu cancels cue"); menu=false; refreshAndDrain();
    dispatchAndDrain(VK_F6,false); check(!cueActive && !enabled,"race F6 toggles immediately"); dispatchAndDrain(VK_F6,false); check(cueActive,"race F6 restores immediately");
    changeAndDrain(raceBindings[3],2); check(!cueActive && !minigameCapturesKey(VK_F1),"finish flag stops race before map reload");
    changeAndDrain(raceBindings[3],0); ++epoch; check(!minigameCapturesKey(VK_F1),"new generation rejects stale minigame context"); resetMinigames(); check(!cueActive,"teardown clears cue");
    load("ffx_ps2/ffx/master/jppc/event/obj/bl/bltz0000/bltz0000.ebp",false);
    change(blitzBindings[0],0);change(blitzBindings[0],1);++epoch;resetMinigames();
    workerDrain=true;drainAtelResourcesForTest();speechq::fixture::drain();workerDrain=false;
    check(spoken.empty() && !minigameCapturesKey(VK_F5),"old held first events are canceled before dispatch after reset");
    load("ffx_ps2/ffx/master/jppc/event/obj/bl/bltz0000/bltz0000.ebp");changeAndDrain(blitzBindings[0],0);
    for(unsigned i=0;i<128;++i) check(dispatchMinigameKey(VK_F5,false),"new generation tickets cannot leak old discarded reservations");
    drain();check(spoken.size()==128,"all new generation reservations released after stale dispatch");
    load("ffx_ps2/ffx/master/jppc/event/obj/bl/bltz0000/bltz0000.ebp",false,true);
    change(blitzBindings[0],0);change(blitzBindings[0],1);drain();
    workerDrain=true;drainAtelResourcesForTest();speechq::fixture::drain();workerDrain=false;
    check(spoken.empty() && !minigameCapturesKey(VK_F5),"changed code cannot use candidate-only native facts to emit or capture keys");
    puts("PASS: held first resource preparation/source overwrite/equal distinct events and queries/anchors/full FIFO/recovery/stale dispatch/changed hash; fake transport, worker engine reads forbidden.");
    puts("PASS: checked archived minigames, Auto keys, goals, in-flight turnovers, all race legs, live pickup removal, exact stereo, context, focus and generation guards.");
}
