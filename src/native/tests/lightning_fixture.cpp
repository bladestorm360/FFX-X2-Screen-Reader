#include "external_fixture.hpp"
#include <atomic>
#include "../lightning.hpp"
#include "../events.hpp"
#include "../settings.hpp"
#include "../atel_resources.hpp"
#include "../input_device.hpp"
#include <map>
#include <fstream>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <algorithm>
#include <stdexcept>
namespace {
using namespace ffx;
struct Listener { Handler before,after; };
std::map<unsigned,std::vector<Listener>> listeners;
std::vector<uint8_t> chunk,worker(0x600),signal(0x20),saved(0x1000),controller(0x80);
size_t chunkOffset=0;
thread_local bool workerDrain=false;
unsigned epoch=1,local=12,entry=1,queryPc=0x34b4,countIndex=23,elapsedIndex=24,boltsIndex=15;
unsigned cues=0,cancels=0,cueEpoch=0;
bool focused=true,battle=false,menu=false,console=false,warning=true,automatic=true,padOwned=false;
int modeValue=0;
void check(bool valid,const char* reason) { if(!valid) { fprintf(stderr,"FAIL: %s\n",reason); exit(1); } }
uintptr_t ptr(std::vector<uint8_t>& bytes) { return reinterpret_cast<uintptr_t>(bytes.data()); }
uintptr_t chunkPtr() { return ptr(chunk)+chunkOffset; }
template<class T> void put(std::vector<uint8_t>& bytes,size_t offset,T value) {
    check(offset+sizeof(value)<=bytes.size(),"fixture bounds"); memcpy(bytes.data()+offset,&value,sizeof(value));
}
int __cdecl mode() { check(!workerDrain,"Worker mode getter is forbidden"); return modeValue; }
uintptr_t __cdecl variable(uintptr_t w,int index,int element) {
    check(!workerDrain,"Worker engine variable getter is forbidden");
    const auto header=read<uintptr_t>(w),resource=read<uintptr_t>(w+4);
    const auto low=read<unsigned>(resource+read<unsigned>(header+0x14)+index*8);
    const auto size=low>>28==2?2u:1u;
    return ((low>>25&7)==0?ptr(saved)+0x1ec:resource+read<unsigned>(resource+0x20))+(low&0xffffff)+element*size;
}
template<class T> void value(unsigned index,T v) { check(write<T>(variable(ptr(worker),index,0),v),"variable write"); }
void setCursor(unsigned pc) { put(worker,0x12c+0x18,chunkPtr()+read<unsigned>(chunkPtr()+0x30)+pc); }
void load(bool north=false,bool prepare=true,bool changedCode=false) {
    if(!chunk.empty()) { Event freed; freed.args[0]=ptr(chunk); for(const auto& h:listeners.at(0x48d490)) if(h.before) h.before(freed); }
    resetLightning(); ++epoch;
    local=north?18:12; entry=north?6:1; queryPc=north?0x65eb:0x34b4;
    countIndex=north?26:23; elapsedIndex=north?27:24; boltsIndex=north?17:15;
    const std::string name=north?"kami0300":"kami0000";
    const auto path=(ffx_fixture::evidenceRoot()/"offline-output/FFX/20261003_audit/files/ffx_ps2/ffx/master/jppc/event/obj/ka").string()+"/"+name+"/"+name+".ebp";
    std::ifstream file(path,std::ios::binary); check(file.good(),"archived lightning script");
    std::vector<uint8_t> raw((std::istreambuf_iterator<char>(file)),std::istreambuf_iterator<char>());
    const auto start=read<unsigned>(ptr(raw)+4); unsigned end=static_cast<unsigned>(raw.size());
    for(unsigned i=1;i<11;++i) { const auto offset=read<unsigned>(ptr(raw)+4+i*4); if(offset && offset>=start && offset<=raw.size()) { end=offset; break; } }
    check(start<end && end<=raw.size(),"archived EBP bounds"); chunk=std::move(raw); chunkOffset=start;
    const auto parent=ptr(chunk),base=chunkPtr();
    if(changedCode) { const auto code=read<unsigned>(base+0x30); check(start+code<chunk.size(),"changed code fixture byte bound"); chunk[start+code]^=1; }
    Event loadEvent; loadEvent.args[0]=12; loadEvent.nativeStack=0x910000; loadEvent.epoch=epoch;
    check(listeners.at(0x48d230).size()==1 && listeners.at(0x2871c0).size()==1,"single exact resource listener in fixture host");
    listeners.at(0x48d230).front().before(loadEvent);
    Event allocation; allocation.caller=address(0x48d230)+0x74; allocation.edi=static_cast<unsigned>(chunk.size());
    allocation.args[0]=(allocation.edi+0x7ffu)&~0x7ffu; allocation.result=parent; allocation.epoch=epoch;
    listeners.at(0x2871c0).front().before(allocation); listeners.at(0x2871c0).front().after(allocation);
    loadEvent.result=parent; listeners.at(0x48d230).front().after(loadEvent);
    const auto instruction=base+read<unsigned>(base+0x30)+queryPc;
    check(read<uint8_t>(instruction)==0xb5 && read<uint16_t>(instruction+1)==0x51,
        "live callback cursor points at the original archived CALL0051 opcode, not its successor");
    std::fill(worker.begin(),worker.end(),uint8_t{0}); std::fill(signal.begin(),signal.end(),uint8_t{0});
    std::fill(saved.begin(),saved.end(),uint8_t{0}); std::fill(controller.begin(),controller.end(),uint8_t{0});
    put(worker,0,base+read<unsigned>(base+0x38+local*4)); put(worker,4,base);
    put(worker,0x2e,static_cast<uint16_t>(local)); put(worker,0x90,ptr(signal));
    put(signal,8,static_cast<uint16_t>(entry)); put(controller,0,uint8_t{2}); setCursor(queryPc);
    value(boltsIndex,uint16_t{1}); value(countIndex,uint8_t{0}); value(elapsedIndex,uint8_t{0});
    focused=true; battle=menu=console=false; warning=automatic=true; modeValue=0; scene=0;
    cues=cancels=cueEpoch=0;
    if(prepare) { workerDrain=true; drainAtelResourcesForTest(); workerDrain=false; }
}
Event query(unsigned elapsed,bool physical=false,unsigned parent=0x464180,unsigned eventEpoch=0,bool ancestry=true) {
    value(elapsedIndex,static_cast<uint8_t>(elapsed));
    Event owner; owner.rva=0x464180; owner.args[0]=ptr(worker); owner.epoch=epoch;
    if(ancestry) for(const auto& h:listeners.at(owner.rva)) if(h.before) h.before(owner);
    Event e; e.rva=0x45ef20; e.parentRva=parent; e.epoch=eventEpoch?eventEpoch:epoch; e.args[0]=ptr(worker); e.result=physical;
    for(const auto& h:listeners.at(e.rva)) if(h.after) h.after(e);
    if(ancestry) for(const auto& h:listeners.at(owner.rva)) if(h.after) h.after(owner);
    return e;
}
void nativeCount(const Event& e) { if(e.result) value(countIndex,static_cast<uint8_t>(read<uint8_t>(variable(ptr(worker),countIndex,0))+1)); }
bool nativeSuccess() { return read<uint8_t>(variable(ptr(worker),countIndex,0))==1; }
}
namespace ffx {
std::atomic<int> scene{0};
void log(const char*,...) {}
unsigned generation() { return epoch; }
bool gameFocused() { return focused; }
bool nativeGameThread() { return true; }
bool inBattle() { return battle; }
bool modMenuActive() { return menu; }
bool utilityConsoleActive() { return console; }
bool controllerOwned() { return padOwned; }
std::wstring nativeBindingName(unsigned action,bool) { return std::to_wstring(action); }
std::wstring join(const std::vector<std::wstring>& parts,const wchar_t* separator) {
    std::wstring text; for(const auto& part:parts) { if(!text.empty()) text+=separator; text+=part; } return text;
}
bool copy(uintptr_t source,void* target,size_t size) { check(!workerDrain,"Worker fake native copy is forbidden"); __try { memcpy(target,reinterpret_cast<void*>(source),size); return true; } __except(EXCEPTION_EXECUTE_HANDLER) { return false; } }
uintptr_t address(unsigned rva) {
    if(rva==0x48d230) return rva;
    if(rva==0x46c500) return reinterpret_cast<uintptr_t>(variable);
    if(rva==0x46c840) return reinterpret_cast<uintptr_t>(mode);
    if(rva==0xf26ae8) { static uintptr_t p; p=ptr(controller); return reinterpret_cast<uintptr_t>(&p); }
    if(rva==0xf00740) { static uintptr_t p=1; return reinterpret_cast<uintptr_t>(&p); }
    if(rva==0x2871c0) return rva;
    if(rva==0x48d490) return rva;
    throw std::runtime_error("Unexpected lightning dependency");
}
bool hookSnapshot(unsigned rva,Handler before,Handler after) { listeners[rva].push_back({before,after}); return true; }
bool hook(unsigned rva,Handler before,Handler after) { listeners[rva].push_back({before,after}); return true; }
int setting(Setting id) { return id==Setting::LightningWarning?warning:id==Setting::LightningAutoDodge?automatic:0; }
void navAudioLightningWarning(unsigned requested) { ++cues; cueEpoch=requested; }
void navAudioCancelLightningWarning() { ++cancels; }
}
int main() {
    using namespace ffx;
    check(startAtelResources(),"ATEL fixture owner startup"); installAtelResources();
    installLightning();
    check(lightningDodgeControls()==L"Dodge: 4. Press once when the beep begins.","manual help names the primary configured raw dodge action without claiming the alternate Enter binding");
    padOwned=true; inputDeviceMark.store(1);
    check(lightningDodgeControls()==L"Dodge: 18. Press once when the beep begins.","controller help names its configured raw dodge action"); padOwned=false; inputDeviceMark.store(0);
    for(bool north:{false,true}) {
        load(north);
        for(unsigned elapsed=0;elapsed<12;++elapsed) {
            const auto beforeCue=cues;
            const auto e=query(elapsed); check(e.overrideResult==(elapsed==6),"one native edge at halfway"); nativeCount(e);
            check(cues==beforeCue+(elapsed==0?1:0),"warning begins with the native flash, before the full twelve-tick dodge window");
        }
        check(cues==1 && cueEpoch==epoch && nativeSuccess(),"both archived maps native success and cue");
        check(!query(6).overrideResult && cues==1,"repeated same-strike query cannot duplicate");
        value(boltsIndex,uint16_t{2}); value(countIndex,uint8_t{0});
        query(0);
        nativeCount(query(6)); check(cues==2 && nativeSuccess(),"next genuine strike rearms");
        load(north); nativeCount(query(1,true)); check(!query(6).overrideResult && nativeSuccess(),"early single manual press preserved");
        nativeCount(query(7,true)); check(!nativeSuccess(),"multiple physical presses preserve native failure");
        load(north); const auto manual=query(6,true); check(!manual.overrideResult && manual.result==1,"physical edge wins over auto");
        nativeCount(manual); check(nativeSuccess(),"manual halfway press succeeds unchanged");
        load(north); automatic=false; for(unsigned i=0;i<12;++i) nativeCount(query(i));
        check(!nativeSuccess() && cues==1,"warning-only preserves no-press native failure");
        load(north); warning=false; nativeCount(query(6)); check(cues==0 && nativeSuccess(),"auto independent of warning setting");
        load(north); warning=automatic=false; check(!query(6).overrideResult && cues==0,"both settings off");
    }
    load(); focused=false; query(0); query(5); focused=true;
    check(!query(6).overrideResult && cues==0,"focus return cannot rearm cancelled strike");
    value(boltsIndex,uint16_t{2}); value(countIndex,uint8_t{0}); check(query(6).overrideResult,"new strike after cancellation rearms");
    load(); query(0); cancelLightningWarning(); check(!query(6).overrideResult && cues==1,"explicit menu cancellation persists after the already emitted opening warning");
    for(unsigned reason=0;reason<6;++reason) {
        load();
        if(reason==0) battle=true; if(reason==1) menu=true; if(reason==2) console=true;
        if(reason==3) scene=1; if(reason==4) modeValue=1; if(reason==5) put(controller,0,uint8_t{0});
        check(!query(0).overrideResult && cues==0,"unsafe context cannot play the opening warning");
        check(!query(6).overrideResult && cues==0,"unsafe context fails closed");
        battle=menu=console=false; scene=modeValue=0; put(controller,0,uint8_t{2});
        check(!query(6).overrideResult && cues==0,"restored context retains cancelled strike");
    }
    load(); check(!query(6,false,0).overrideResult,"unrelated parent rejected");
    check(!query(6,false,0x464180,0,false).overrideResult,"stale global worker without interpreter ancestry rejected");
    check(!query(6,false,0x464180,epoch+1).overrideResult,"cross-generation event rejected");
    load(); setCursor(queryPc+1); check(!query(6).overrideResult && cues==0,"wrong instruction cursor rejected");
    load(); setCursor(queryPc+3); check(!query(6).overrideResult && cues==0,"post-instruction cursor cannot represent the live native callback");
    load(); put(worker,0x2e,uint16_t{13}); check(!query(6).overrideResult && cues==0,"wrong worker rejected");
    load(); put(signal,8,uint16_t{2}); check(!query(6).overrideResult && cues==0,"wrong native entry rejected");
    load(); put(signal,0xe,uint8_t{1}); check(!query(6).overrideResult && cues==0,"wrong signal level rejected");
    load(); put(worker,0x32,uint8_t{9}); check(!query(6).overrideResult && cues==0,"invalid native level rejected");
    load(); value(elapsedIndex,uint8_t{12}); check(!query(12).overrideResult && cues==0,"closed native window rejected");
    load(false,false); check(!query(0).overrideResult && cues==0,"pending first query neither cues nor injects");
    workerDrain=true; drainAtelResourcesForTest(); workerDrain=false;
    check(query(1).overrideResult==false && cues==1,"ready preparation retains exactly one opening cue");
    check(query(6).overrideResult,"ready preparation preserves automatic dodge window");
    load(false,false); warning=false; query(0); warning=true; workerDrain=true; drainAtelResourcesForTest(); workerDrain=false;
    check(cues==0,"disabled opening warning is not replayed after preparation");
    load(false,false); query(0); focused=false; query(1); workerDrain=true; drainAtelResourcesForTest(); workerDrain=false; focused=true;
    check(!query(6).overrideResult && cues==0,"focus loss while pending cancels retained first strike");
    load(false,false); query(0); value(elapsedIndex,uint8_t{12}); workerDrain=true; drainAtelResourcesForTest(); workerDrain=false;
    check(!query(12).overrideResult && cues==0,"expired window is not replayed after preparation");
    load(false,false); for(unsigned i=0;i<=6;++i) query(i);
    workerDrain=true; drainAtelResourcesForTest(); workerDrain=false;
    check(!query(7).overrideResult && cues==1,"late qualification does not move auto-injection to a later tick");
    load(false,true,true); check(!query(6).overrideResult && cues==0,"changed archived code rejected by immutable prepared identity");
    load(); const auto h=read<uintptr_t>(ptr(worker));
    write<unsigned>(chunkPtr()+read<unsigned>(h+0x14)+countIndex*8,0x0c000006);
    check(!query(6).overrideResult && cues==0,"changed native declaration rejected");
    load(); query(0); ++epoch; query(0); check(query(6).overrideResult && cueEpoch==epoch,"new resource generation clears stale ownership");
    puts("PASS: lightning actual EV receipts/preparation, pending first-window retention, both archived maps, manual input, cancellation and identity guards");
}
