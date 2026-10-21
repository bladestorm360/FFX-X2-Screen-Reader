#include "input_device.hpp"
#include "lightning.hpp"
#include "events.hpp"
#include "settings.hpp"
#include "nav_audio.hpp"
#include "mod_menu.hpp"
#include "utilities.hpp"
#include "controller.hpp"
#include "atel_resources.hpp"
#include <array>
#include <algorithm>
#include <cstring>
namespace ffx {
namespace {
struct Script {
    const char* name;
    unsigned length,workers,hash,local,entry,entryPc,queryPc,bolts,count,elapsed;
    unsigned countLow,elapsedLow;
};
constexpr Script scripts[]={
    {"kami0000",46958,64,0x4b4a7773,12,1,0x3414,0x34b4,15,23,24,0x0c000007,0x0c000008},
    {"kami0300",56818,84,0x36d996be,18,6,0x6535,0x65eb,17,26,27,0x0c000009,0x0c00000a},
};
struct Binding {const Script* script=nullptr;AtelSpan span;unsigned codeOffset=0;bool ready=false;};
struct Cached {uintptr_t chunk=0;unsigned serial=0;Binding binding;};
std::array<Cached,4> cache{};
unsigned cacheEpoch=0,nextCache=0;
struct Strike {
    uintptr_t worker=0,chunk=0,signal=0;
    unsigned epoch=0,bolts=0;
    bool cancelled=false,warned=false,injected=false,opening=false,wanted=false;unsigned serial=0;
} strike;
thread_local uintptr_t interpreter=0;
thread_local unsigned interpreterEpoch=0;
bool inside(const AtelSpan& span,unsigned offset,unsigned size) {return offset<=span.size && size<=span.size-offset;}
Binding identify(uintptr_t chunk) {
    Binding result;if(!nativeAtelSpan(chunk,result.span) || !inside(result.span,0,0x38)) return {};
    if(cacheEpoch!=generation()) {cache={};nextCache=0;cacheEpoch=generation();}
    for(const auto& cached:cache) if(cached.chunk==chunk && cached.serial==result.span.serial) return cached.binding;
    const auto length=read<unsigned>(chunk),code=read<unsigned>(chunk+0x30),nameOffset=read<unsigned>(chunk+0xc);
    const auto workers=read<uint16_t>(chunk+0x34);
    char name[9]{};
    if(!length || !inside(result.span,code,length) || !inside(result.span,nameOffset,8) || !copy(chunk+nameOffset,name,8)) return {};
    for(const auto& candidate:scripts) if(length==candidate.length && workers==candidate.workers && !std::strcmp(name,candidate.name)) result.script=&candidate;
    result.codeOffset=code;
    if(!result.script) return result;
    const auto state=atelPreparation(chunk);
    if(state==AtelPreparation::Pending) return result; // Never cache a pending negative.
    AtelIdentity identity;
    if(state==AtelPreparation::Ready && !readyAtelIdentity(chunk,identity)) return result;
    result.ready=state==AtelPreparation::Ready && identity.span.serial==result.span.serial &&
        identity.checksum==result.script->hash && identity.length==result.script->length && identity.workers==result.script->workers &&
        !std::strcmp(identity.name,result.script->name);
    if(!result.ready) result.script=nullptr;
    cache[nextCache++%cache.size()]={chunk,result.span.serial,result};return result;
}
bool declaration(uintptr_t worker,uintptr_t chunk,const AtelSpan& span,unsigned index,unsigned low) {
    const auto header=read<uintptr_t>(worker);
    if(header<chunk || header-chunk>UINT32_MAX || !inside(span,static_cast<unsigned>(header-chunk),0x28) || index>=read<uint16_t>(header+2)) return false;
    const auto offset=read<unsigned>(header+0x14);
    unsigned raw[2]{};
    return inside(span,offset,(index+1)*8) && copy(chunk+offset+index*8,raw,sizeof(raw)) && raw[0]==low && raw[1]==1;
}
bool allowed() {
    if(!gameFocused() || scene || inBattle() || modMenuActive() || utilityConsoleActive()) return false;
    const auto controller=read<uintptr_t>(address(0xf26ae8));
    if(!controller || !(read<uint8_t>(controller)&2) || !read<uintptr_t>(address(0xf00740))) return false;
    using Mode=int(__cdecl*)();
    return reinterpret_cast<Mode>(address(0x46c840))()==0;
}
void entered(Event& event) {
    // Slots0/1 already belong to pedestal interpreter tracking.
    event.before[24]=interpreter; event.before[25]=interpreterEpoch;
    interpreter=event.args[0]; interpreterEpoch=generation();
}
void left(Event& event) {
    interpreter=static_cast<uintptr_t>(event.before[24]);
    interpreterEpoch=static_cast<unsigned>(event.before[25]);
}
void queryImpl(Event& event) {
    if(!nativeGameThread()) return;
    const auto worker=event.args[0];
    if(!nativeGameThread() || !worker || event.parentRva!=0x464180 || interpreter!=worker ||
       interpreterEpoch!=generation() || event.epoch!=generation()) return;
    const auto chunk=read<uintptr_t>(worker+4);
    const auto binding=chunk?identify(chunk):Binding{};const auto script=binding.script;
    if(!script || read<uint16_t>(worker+0x2e,0xffff)!=script->local) return;
    const auto header=read<uintptr_t>(worker),signal=read<uintptr_t>(worker+0x90);
    const auto level=read<uint8_t>(worker+0x32,0xff);
    if(!inside(binding.span,0x38+script->local*4,4) || header<chunk || header-chunk>UINT32_MAX ||
        !inside(binding.span,static_cast<unsigned>(header-chunk),0x28)) return;
    const auto offset=read<unsigned>(chunk+0x38+script->local*4);
    const auto entries=read<unsigned>(header+0x20);
    const auto code=chunk+binding.codeOffset;
    if(!header || !signal || level>8 || !inside(binding.span,offset,0x34) || header!=chunk+offset ||
       read<uint8_t>(header)!=1 || script->entry>=read<uint16_t>(header+8) ||
       !inside(binding.span,entries,(script->entry+1)*4) || read<unsigned>(chunk+entries+script->entry*4)!=script->entryPc ||
       read<uint16_t>(signal+8,0xffff)!=script->entry || (read<uint8_t>(signal+0xe)&15)!=level ||
       // 869D00 reads the opcode without advancing the cursor. 464180 advances
       // it only after 877770 and this return callback finish.
       read<uintptr_t>(worker+0x12c+level*0x4c+0x18)!=code+script->queryPc ||
       !declaration(worker,chunk,binding.span,script->bolts,0x20000210) ||
       !declaration(worker,chunk,binding.span,script->count,script->countLow) ||
       !declaration(worker,chunk,binding.span,script->elapsed,script->elapsedLow)) return;
    using Variable=uintptr_t(__cdecl*)(uintptr_t,int,int);
    const auto get=reinterpret_cast<Variable>(address(0x46c500));
    uint16_t bolts; uint8_t count,elapsed;
    if(!copy(get(worker,script->bolts,0),&bolts,sizeof(bolts)) ||
       !copy(get(worker,script->count,0),&count,sizeof(count)) ||
       !copy(get(worker,script->elapsed,0),&elapsed,sizeof(elapsed))) return;
    if(elapsed>=12) {cancelLightningWarning();return;}
    const bool same=strike.epoch==generation() && strike.worker==worker && strike.chunk==chunk &&
        strike.signal==signal && strike.bolts==bolts && strike.serial==binding.span.serial;
    if(!same) {navAudioCancelLightningWarning();strike={worker,chunk,signal,generation(),bolts};strike.serial=binding.span.serial;}
    if(!allowed()) { cancelLightningWarning(); return; }
    if(strike.cancelled) return;
    // CALL4003 starts the native flash immediately before the first query.
    // Sound at elapsed0 exposes the full twelve-tick manual dodge window.
    if(elapsed==0 && !strike.opening) {strike.opening=true;strike.wanted=setting(Setting::LightningWarning)!=0;}
    // Retain a copied first-window intent while qualification is pending. A
    // later native query must still prove the same open signal/bolt before it
    // can publish the warning; no expired window or disabled opening replays.
    if(binding.ready && strike.opening && !strike.warned) {
        strike.warned=true;
        if(strike.wanted && setting(Setting::LightningWarning)) {
            navAudioLightningWarning(strike.epoch);
            recordLog("lightning warning script=%s bolts=%u elapsed=%u",script->name,bolts,elapsed);
        }
    }
    // Common0051 tests the raw pressed bit5. The original native script counts
    // one press, performs the dodge and owns every counter/animation/reward.
    if(binding.ready && elapsed==6 && setting(Setting::LightningAutoDodge) && !strike.injected && !count && !event.result && !event.overrideResult) {
        strike.injected=true; event.result=1; event.overrideResult=true;
        recordLog("lightning auto-dodge script=%s bolts=%u elapsed=%u",script->name,bolts,elapsed);
    }
}
void query(Event& event) {
    try { queryImpl(event); }
    catch(...) { cancelLightningWarning(); }
}
}
void cancelLightningWarning() {
    strike.cancelled=true;
    navAudioCancelLightningWarning();
}
void resetLightning() {
    navAudioCancelLightningWarning(); strike={}; cache={}; cacheEpoch=nextCache=0;
    interpreter=0; interpreterEpoch=0;
}
void installLightning() {
    hook(0x464180,entered,left);
    hook(0x45ef20,nullptr,query);
}
#ifdef FFX_READER_TEST
std::wstring lightningDodgeControls() {
    const bool controller=nativeInputUsesController();
    // Name the primary raw dodge binding. An alternate menu-confirm key has
    // not been established as valid for Common0051's unremapped input query.
    const auto name=nativeBindingName(controller?18:4,controller);
    return name.empty()?L"Press the game's configured dodge action once when the beep begins.":L"Dodge: "+name+L". Press once when the beep begins.";
}
#endif
}
