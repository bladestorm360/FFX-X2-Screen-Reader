#include "jecht.hpp"
#include "jecht_data.hpp"
#include "events.hpp"
#include "atel_resources.hpp"
#include "native_worker_index.hpp"
#include "settings.hpp"
#include "mod_menu.hpp"
#include "utilities.hpp"
#include "text_plan.hpp"
#include "nav_audio.hpp"
#include "jecht_voice.hpp"
#include "jecht_caption.hpp"
#include <cstring>
namespace ffx {
#ifdef FFX_READER_TEST
unsigned jechtTickForTest();
#endif
namespace {
thread_local uintptr_t interpreter=0;
thread_local unsigned interpreterEpoch=0;
struct Binding {AtelSpan span;unsigned script=2,code=0;bool ready=false;};
struct Cached {uintptr_t chunk=0;unsigned serial=0;Binding binding;};
Cached cache[4]{};unsigned cacheEpoch=0,nextCache=0;
struct Pace {
    unsigned epoch=0,allocation=0,script=2,value=0,began=0;
    JechtCaption caption;
    unsigned beganRound=0;
    bool announced=false,admitted=false,pressed=false,roundKnown=false;
};
Pace pace[2]{};bool lastAssist=false,confirmed=false;unsigned confirmedAt=0;
unsigned confirmedRound=0;
unsigned wakeTimes[16]{},wakeMask=0,wakeCursor=0;
thread_local JechtCaption caption;
thread_local unsigned captionWindow=8;
constexpr unsigned ReactionDelay=750,ConfirmGap=750,CaptionFallback=500,Successes=11;
constexpr unsigned ReactionSteps=23,ConfirmSteps=23; // Ceil(each target in seconds *30 original ticks/s).
#ifdef FFX_READER_TEST
unsigned tick() {return jechtTickForTest();}
#else
unsigned tick() {return GetTickCount();}
#endif
constexpr unsigned calls[]={0x4c,0x4d,0x50,0x51,0x122};
bool inside(const AtelSpan& s,unsigned offset,unsigned size) {return offset<=s.size && size<=s.size-offset;}
Binding identify(uintptr_t chunk) {
    Binding b;if(!nativeAtelSpan(chunk,b.span) || !inside(b.span,0,0x38)) return b;
    if(cacheEpoch!=generation()) {for(auto& c:cache)c={};cacheEpoch=generation();nextCache=0;}
    for(const auto& c:cache) if(c.chunk==chunk && c.serial==b.span.serial) return c.binding;
    const auto length=read<unsigned>(chunk),offset=read<unsigned>(chunk+0xc);
    b.code=read<unsigned>(chunk+0x30);const auto workers=read<uint16_t>(chunk+0x34);
    char name[9]{};if(!inside(b.span,offset,8) || !inside(b.span,b.code,length) || !copy(chunk+offset,name,8)) return b;
    for(unsigned i=0;i<2;++i) if(length==jechtScripts[i].length && workers==jechtScripts[i].workers && !std::strcmp(name,jechtScripts[i].name)) b.script=i;
    if(b.script==2) return b;
    const auto state=atelPreparation(chunk);if(state==AtelPreparation::Pending) return b;
    AtelIdentity id;
    b.ready=state==AtelPreparation::Ready && readyAtelIdentity(chunk,id) && id.span.serial==b.span.serial &&
        id.checksum==jechtScripts[b.script].checksum && id.length==length && id.workers==workers && !std::strcmp(id.name,name);
    cache[nextCache++%4]={chunk,b.span.serial,b};return b;
}
bool allowed() {return nativeGameThread() && gameFocused() && !inBattle() && !modMenuActive() && !utilityConsoleActive();}
bool position(uintptr_t worker,const Binding& b,unsigned local,unsigned entry,unsigned pc) {
    const unsigned level=read<uint8_t>(worker+0x32,0xff);
    if(level>8 || pc>=jechtScripts[b.script].length || read<uintptr_t>(worker+0x12c+level*0x4c+0x18)!=b.span.chunk+b.code+pc) return false;
    const auto controller=read<uintptr_t>(address(0xf26ae8));NativeWorkerIdentity identity;
    if(!controller || read<uint16_t>(controller+0x1a,0xffff)!=0 || !nativeWorkerIdentity(controller,b.span.chunk,local,identity) || identity.worker!=worker) return false;
    unsigned offset=0;if(!readyAtelWorkerHeader(b.span.chunk,local,offset)) return false;
    const auto header=read<uintptr_t>(worker),signal=read<uintptr_t>(worker+0x90);
    const auto entries=read<unsigned>(header+0x20);
    if(!inside(b.span,offset,0x34) || header!=b.span.chunk+offset || read<uint16_t>(worker+0x2e,0xffff)!=local ||
        !signal || level>8 || read<uint16_t>(signal+8,0xffff)!=entry || (static_cast<unsigned>(read<uint8_t>(signal+0xe))&15u)!=level ||
        entry>=read<uint16_t>(header+8) || !inside(b.span,entries,(entry+1)*4)) return false;
    const auto entryPc=read<unsigned>(b.span.chunk+entries+entry*4);
    const auto& s=jechtScripts[b.script];
    if(local==s.local && entry==s.entry && entryPc!=s.entryPc) return false;
    if(local==s.tutorial && entry==s.tutorialEntry && entryPc!=s.tutorialPc) return false;
    return true;
}
bool fact(uintptr_t worker,const Binding& b,unsigned index,unsigned low,int& value) {
    unsigned actual=0,high=0;
    if(!readyAtelDeclarationAt(b.span.chunk,static_cast<unsigned>(read<uintptr_t>(worker)-b.span.chunk),index,actual,high) || actual!=low || high!=1) return false;
    using Variable=uintptr_t(__cdecl*)(uintptr_t,int,int);
    const auto p=reinterpret_cast<Variable>(address(0x46c500))(worker,index,0);
    if(low>>28==5) return copy(p,&value,4);
    uint8_t v;if(!copy(p,&v,1))return false;value=v;return true;
}
void arm(const Binding& b,const JechtCue& cue) {
    const auto slot=cue.local-jechtScripts[b.script].local-1;if(slot>=2) return;
    const auto now=tick();
    pace[slot]={generation(),b.span.serial,b.script,cue.entry-6,now,
        {generation(),cue.sequence,jechtVoiceSerial(cue.sequence),now}};
    int round=0;auto& p=pace[slot];
    if(fact(interpreter,b,jechtScripts[b.script].timer,jechtScripts[b.script].timerLow,round) && round>=0 && round<300) {
        p.beganRound=static_cast<unsigned>(round);p.roundKnown=true;
    }
    wakeTimes[wakeCursor]=now;wakeMask|=1u<<wakeCursor;wakeCursor=(wakeCursor+1)%16;
}
bool paced(const Binding& b,const JechtQuery& q,unsigned round) {
    auto& p=pace[q.slot];
    if(p.epoch!=generation() || p.allocation!=b.span.serial || p.script!=b.script || p.value!=q.value) {
        for(const auto& cue:jechtCues) if(cue.script==b.script && cue.local==jechtScripts[b.script].local+q.slot+1 && cue.entry==q.value+6) {arm(b,cue);break;}
    }
    // Center requires Confirm twice in one eager predicate. Once this request
    // is admitted, every query must see the same response until the game clears
    // it; query two cannot be rejected by the gap started by query one.
    if(p.admitted)return true;
    const auto now=tick();
    // The original round counter advances with the game's simulation. Keep
    // pacing within that budget even under speedup or a coarse host clock.
    if(confirmed && round<confirmedRound)confirmed=false;
    const bool reacted=now-p.began>=ReactionDelay || (p.roundKnown && round>=p.beganRound && round-p.beganRound>=ReactionSteps);
    const bool spaced=!confirmed || now-confirmedAt>=ConfirmGap || (round>=confirmedRound && round-confirmedRound>=ConfirmSteps);
    if(!reacted || !spaced)return false;
    p.admitted=true;return true;
}
void entered(Event& e) {
    e.before[26]=interpreter;e.before[27]=interpreterEpoch;interpreter=e.args[0];interpreterEpoch=generation();
    const bool assist=setting(Setting::JechtAssist)!=0;
    if(nativeGameThread()) {
        if(assist!=lastAssist) {confirmed=false;lastAssist=assist;for(auto& p:pace)p.admitted=p.pressed=false;}
        // Wake deferred subtitle classification at its bounded fallback from
        // the existing script processing event, never a timer/polling thread.
        if(wakeMask) {
            const auto now=tick();bool expired=false;
            for(unsigned i=0;i<16;++i) if((wakeMask&(1u<<i)) && now-wakeTimes[i]>=CaptionFallback) {
                wakeMask&=~(1u<<i);expired=true;
            }
            if(expired)speechq::notifyPreparation();
        }
    }
}
void left(Event& e) {
    interpreter=e.before[26];interpreterEpoch=static_cast<unsigned>(e.before[27]);
}
bool command(const Event& e) {
    return allowed() && e.epoch==generation() && e.parentRva==0x464180 &&
        interpreterEpoch==generation() && interpreter==e.args[0];
}
void soundBefore(Event& e) {
    if(!allowed() || e.epoch!=generation() || e.parentRva!=0x464180 ||
        !interpreter || interpreterEpoch!=generation()) return;
    const auto b=identify(read<uintptr_t>(interpreter+4));if(!b.ready)return;
    const auto& s=jechtScripts[b.script];
    for(const auto& cue:jechtCues) if(cue.script==b.script && cue.sequence==e.args[0] &&
        position(interpreter,b,s.local,s.entry,cue.soundPc)) {arm(b,cue);return;}
}
void showBefore(Event& e) {
    std::memcpy(e.copied.data(),&caption,sizeof(caption));e.before[0]=captionWindow;
    caption={};captionWindow=8;e.before[1]=e.before[2]=0;e.before[3]=1;e.before[4]=2;
    if(!command(e))return;
    const auto b=identify(read<uintptr_t>(e.args[0]+4));if(!b.ready)return;
    for(const auto& cue:jechtCues) if(cue.script==b.script && position(e.args[0],b,cue.local,cue.entry,cue.pc)) {
        e.before[1]=cue.mask;e.before[2]=b.span.serial;captionWindow=cue.window;
        const auto slot=cue.local-jechtScripts[b.script].local-1,other=1-slot;
        auto& p=pace[slot];const auto& pending=pace[other];e.before[4]=slot;
        int active=0;
        if(pending.announced && pending.epoch==generation() && pending.allocation==b.span.serial && pending.script==b.script &&
            fact(e.args[0],b,jechtScripts[b.script].active[other],jechtScripts[b.script].activeLow[other],active) && active==1)
            e.before[3]=0;
        if(p.epoch==generation() && p.allocation==b.span.serial && p.script==b.script && p.value==cue.entry-6) {
            caption=p.caption;
        } else p={generation(),b.span.serial,b.script,cue.entry-6,tick(),{}};
        p.began=tick();p.admitted=p.pressed=p.announced=false;
        int round=0;
        if(fact(e.args[0],b,jechtScripts[b.script].timer,jechtScripts[b.script].timerLow,round) && round>=0 && round<300) {
            p.beganRound=static_cast<unsigned>(round);p.roundKnown=true;
        }
        return;
    }
}
void showAfter(Event& e) {
    std::memcpy(&caption,e.copied.data(),sizeof(caption));captionWindow=static_cast<unsigned>(e.before[0]);
    if(!e.before[1] || e.epoch!=generation() || !allowed() || !setting(Setting::JechtCues) || setting(Setting::JechtAssist)) return;
    AtelSpan span;if(!nativeAtelSpan(read<uintptr_t>(e.args[0]+4),span) || span.serial!=static_cast<unsigned>(e.before[2])) return;
    // Clear old narration for the first control cue. A second still-pending
    // memory appends instead of cutting off the first cue's spoken controls.
    TextPlan p;prepareTextPlan(p,"jecht-cue",e.before[3]!=0);p.controls.swap=false;
    TextRef names=0,gate=0;const auto mask=static_cast<uint16_t>(e.before[1]);
    for(const uint16_t bit:{uint16_t{0x1000},uint16_t{0x2000},uint16_t{0x4000},uint16_t{0x8000},uint16_t{0x20}}) if(mask&bit) {
        const auto name=planNativeButtonMask(p,bit);
        names=names?planPair(p,TextPlanOp::SpaceJoin,names,name):name;
        gate=gate?planPair(p,TextPlanOp::IfText,gate,name):name;
    }
    p.gate=gate;p.headline=names;
    if(commitTextPlan(p) && e.before[4]<2)pace[e.before[4]].announced=true;
    navAudioOverdriveBeep(880.f,generation());
}
void query(Event& e) {
    if(!setting(Setting::JechtAssist) || !allowed() || e.epoch!=generation() || e.parentRva!=0x464180 ||
        interpreterEpoch!=generation() || interpreter!=e.args[0] || e.overrideResult) return;
    unsigned call=0;for(unsigned i=0;i<std::size(calls);++i) if(e.rva==jecht_rva::queries[i]) call=calls[i];
    if(!call) return;
    const auto worker=e.args[0],chunk=read<uintptr_t>(worker+4);const auto b=identify(chunk);if(!b.ready)return;
    const auto& s=jechtScripts[b.script];
    for(const auto& q:jechtQueries) if(q.script==b.script && q.call==call && position(worker,b,q.local,q.entry,q.pc)) {
        const auto code=chunk+b.code+q.pc;
        if(read<uint8_t>(code)!=0xb5 || read<uint16_t>(code+1)!=call) return;
        unsigned currentRound=0;
        if(q.slot<2) {
            int active=0,selector=-1,timer=300,score=-1;
            if(!fact(worker,b,s.active[q.slot],s.activeLow[q.slot],active) || active!=1 ||
               !fact(worker,b,s.selector[q.slot],s.selectorLow[q.slot],selector) || selector!=static_cast<int>(q.value) ||
               !fact(worker,b,s.timer,s.timerLow,timer) || timer<0 || timer>=300 ||
               !fact(worker,b,s.score,s.scoreLow,score) || score<0 || score>=static_cast<int>(Successes)) return;
            currentRound=static_cast<unsigned>(timer);
            if(!paced(b,q,currentRound)) return;
        }
        // Answer this original script's input query. Its own branches change
        // scores/animations/rewards; no saved or completion flag is written.
        e.result=q.expected;e.overrideResult=true;
        if(q.slot<2 && q.call==0x51 && q.expected && !pace[q.slot].pressed) {
            pace[q.slot].pressed=true;confirmedAt=tick();confirmedRound=currentRound;confirmed=true;
        }
        return;
    }
}
}
JechtCaption captureJechtCaption(unsigned window) noexcept {return window==captionWindow && caption.epoch==generation()?caption:JechtCaption{};}
void resetJecht() {interpreter=0;interpreterEpoch=0;caption={};captionWindow=8;for(auto& c:cache)c={};cacheEpoch=nextCache=0;for(auto& p:pace)p={};wakeMask=wakeCursor=confirmedAt=confirmedRound=0;lastAssist=confirmed=false;}
void installJecht() {
    installJechtVoice();
    hook(0x464180,entered,left);
    // The interpreter processes many opcodes per call. Capture at the exact
    // Common executors while its original cursor still points to that opcode.
    hook(jecht_rva::Sound,soundBefore,nullptr);
    hook(jecht_rva::Show,showBefore,showAfter);
    // Common4C/4D/50/51 integer-return executors were read from the retained
    // native table at RVA850050 + id*16 + C. Installed signatures additionally
    // prove their held/pressed-word getter and original remapping body.
    for(const auto rva:jecht_rva::queries) hook(rva,nullptr,query);
}
}
