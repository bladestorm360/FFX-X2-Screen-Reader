// Optional autosuccess enters the original finish/result paths. Only approved
// activity counters are supplied; native scripts retain inventory, prizes,
// records, standings, story decisions, actor cleanup and reward dialogues.
// See Docs/features/puzzles/minigame_completion_20261020.md for native evidence.
#include "minigame_completion.hpp"
#include "utility_memory.hpp"
#include "native_worker_index.hpp"
#include "events.hpp"
#include "mod_menu.hpp"
#include "utilities.hpp"
#include "room_areas.hpp"
#include <algorithm>
#include <cstring>
namespace ffx {
namespace {
struct Rule {
    Setting setting; const char* name; unsigned hash,length,workers;
    unsigned local,entry,pc,index,low,initial1,initial2,targetPc=0,secondPc=0;
};
// Source locators/typed declarations, not executable game bytes. The companion
// offline check matches every row against the retained original ATEL variants.
constexpr Rule rules[]={
    {Setting::ChocoboAssist,"nagi0000",0x6722a493,225132,150,82,7,164090,104,0x0c0000b0,163914,0,215637,215288},
    {Setting::ChocoboAssist,"nagi0000",0x42b3fc26,225368,150,82,7,164366,104,0x0c0000b0,164190,0,215873,215524},
    {Setting::RemiemAssist,"lmyt0000",0xef16e26d,37071,105,35,7,23068,17,0x56000000,22898,0,36147,0},
    {Setting::ButterflyAssist,"mcfr0100",0x908ca3c3,78486,55,37,8,31126,50,0x56000000,30841,30956,30448,0},
    {Setting::ButterflyAssist,"mcfr0200",0xc248925f,14255,40,26,8,12339,35,0x56000000,12054,12169,11661,0},
    {Setting::BlitzAssist,"bltz0000",0x2ba296ba,4647290,41,36,6,3074303,258,0x0c00128d,0,0},
    {Setting::BlitzAssist,"bltz0000",0x4c6c4230,4647290,41,36,6,3074303,258,0x0c00128d,0,0},
    {Setting::BlitzAssist,"bltz0000",0x601e7c1c,4646208,41,36,6,3073221,258,0x0c00128d,0,0},
    {Setting::BlitzAssist,"bltz0000",0xef631536,4647290,41,36,6,3074303,258,0x0c00128d,0,0}
};
struct Attempt {
    uintptr_t chunk=0,signal=0; unsigned epoch=0,allocation=0,serial=0,pending=0; bool handled=false;
};
std::array<Attempt,4> attempts{};
struct Write {unsigned low=0,index=0;int value=0;};
struct Request {unsigned local=0,entry=0,global=0;uintptr_t worker=0;};
// Four fixed owner-local slots. No worker receives game pointers; prepare
// revalidates all identities, and invoke immediately runs on the PInput owner.
struct Job {
    bool used=false;unsigned ticket=0,context=0,revision=0,attempt=0;
    const Rule* rule=nullptr;uintptr_t controller=0,chunk=0,worker=0,signal=0;
    unsigned epoch=0,allocation=0,writes=0,requests=0,source=0;
    std::array<Write,8> values{};std::array<Request,2> calls{};
};
std::array<Job,4> jobs{};
unsigned ticketSerial=0,contextSerial=1;
thread_local uintptr_t interpreter=0;
thread_local unsigned interpreterEpoch=0;
unsigned slot(Setting s) {return s==Setting::ChocoboAssist?0u:s==Setting::RemiemAssist?1u:s==Setting::ButterflyAssist?2u:3u;}
bool allowed() {return nativeGameThread() && gameFocused() && !scene && !inBattle() && !modMenuActive() && !utilityConsoleActive();}
bool identity(UtilityScript& s,const Rule& r) {
    AtelIdentity id;
    return captureUtilityScript(s) && s.name==r.name && s.length==r.length && s.workers==r.workers &&
        readyAtelIdentity(s.chunk,id) && id.span.serial==s.allocationSerial && id.length==s.length &&
        id.codeOffset==s.codeOffset && id.workers==s.workers && id.checksum==r.hash && s.name==id.name;
}
bool owner(UtilityScript& s,uintptr_t w,unsigned local) {
    uintptr_t actual=0;
    if(!captureUtilityWorker(s,local,actual) || w!=actual || read<uintptr_t>(w+4)!=s.chunk) return false;
    s.worker=w;s.header=read<uintptr_t>(w);return true;
}
bool signal(uintptr_t w,unsigned& entry,unsigned& pc,const UtilityScript& s) {
    const auto sig=read<uintptr_t>(w+0x90);const unsigned level=read<uint8_t>(w+0x32,255);
    if(!sig || level>8 || (read<uint8_t>(sig+14)&15u)!=level) return false;
    entry=read<uint16_t>(sig+8,0xffff);
    const auto cursor=read<uintptr_t>(w+0x12c+level*0x4c+0x18),code=s.chunk+s.codeOffset;
    if(cursor<code || cursor-code>=s.length) return false;
    pc=static_cast<unsigned>(cursor-code);return true;
}
// Keep live rows in the qualification, including after resource preparation.
bool binding(const UtilityScript& s,unsigned low,unsigned& index) {
    unsigned a=0,b=0;AtelSpan span;
    if(!readyAtelDeclaration(s.chunk,static_cast<unsigned>(s.header-s.chunk),low,1,index) ||
        !readyAtelDeclarationAt(s.chunk,static_cast<unsigned>(s.header-s.chunk),index,a,b) || a!=low || b!=1 ||
        !nativeAtelSpan(s.chunk,span) || index>=read<uint16_t>(s.header+2)) return false;
    const auto offset=read<unsigned>(s.header+0x14);
    return offset<=span.size && (index+1ull)*8<=span.size-offset &&
        read<unsigned>(s.chunk+offset+index*8)==low && read<unsigned>(s.chunk+offset+index*8+4)==1;
}
bool add(Job& j,const UtilityScript& s,unsigned low,int value) {
    if(j.writes==j.values.size()) return false;
    unsigned index=0;if(!binding(s,low,index)) return false;
    j.values[j.writes++]={low,index,value};return true;
}
bool request(Job& j,const UtilityScript& s,unsigned local,unsigned entry,unsigned expectedPc) {
    if(j.requests==j.calls.size()) return false;
    uintptr_t w=0;NativeWorkerIdentity id;AtelSpan span;
    if(!captureUtilityWorker(s,local,w) || !nativeWorkerIdentity(s.controller,s.chunk,local,id) ||
        !nativeAtelSpan(s.chunk,span) || !(read<uint16_t>(w+0x34)&2) || !read<uintptr_t>(w+0x88)) return false;
    const auto header=read<uintptr_t>(w);const auto entries=read<unsigned>(header+0x20);
    if(entry>=read<uint16_t>(header+8) || entries>span.size || (entry+1ull)*4>span.size-entries ||
        expectedPc>=s.length || read<unsigned>(s.chunk+entries+entry*4)!=expectedPc) return false;
    j.calls[j.requests++]={local,entry,id.global,w};return true;
}
bool value(const UtilityScript& s,unsigned low,int& v) {return captureUtilityValue(s,low,1,0,v);}
bool plan(Job& j,UtilityScript& s) {
    // Every plan is bounded, constant-size and validated before its first write.
    // Only transient race/hunt/score operands are writable, never inventory,
    // achievements, completion/reward flags or story variables.
    const auto kind=j.rule->setting;
    if(kind==Setting::ChocoboAssist) {
        int course=0,flags=0,running=0;
        if(!value(s,0x0c0000bc,course) || course<1 || course>4 || !value(s,0x0d0000c9,flags) ||
            (flags&6) || !value(s,0x0c0000e1,running) || running!=1) return false;
        for(unsigned low:{0x0c0000b0u,0x0c0000b1u,0x0c0000b2u,0x0c0000d5u,0x0c0000d4u,0x0c0000d6u})
            if(!add(j,s,low,0)) return false;
        // One balloon makes Catcher's raw time strictly negative. Exact zero
        // alone would display0:00 without setting the native perfect-time bit.
        if(!add(j,s,0x0c0000d3,course==4?1:0)) return false;
        return request(j,s,109,2,j.rule->targetPc) && (course!=4 || request(j,s,108,2,j.rule->secondPc));
    }
    if(kind==Setting::RemiemAssist) {
        // The running timer can only follow W70's real race start. W102:2 is
        // the original finish-line collision, including mounting/control and
        // first-win/repeat-reward cleanup; it is not a debug grant route.
        int rewards=0;if(!value(s,0x01000a9b,rewards)) return false;
        unsigned chests=5;
        // Native first victory gives the mirror. Later chest tiers are separate
        // one-time prizes: always choosing5 would leave tiers1..4 inaccessible
        // while assistance is On. Earn each missing tier, then retain the native
        // five-chest repeat path once all are owned. Never write these flags.
        constexpr unsigned earned[]={8,4,16,32,64};
        if(rewards&2) for(unsigned i=0;i<5;++i) if(!(rewards&earned[i])) {chests=i+1;break;}
        return add(j,s,0x0c00000a,0) && add(j,s,0x0c00000b,static_cast<int>(chests)) && request(j,s,102,2,j.rule->targetPc);
    }
    if(kind==Setting::ButterflyAssist) {
        int active=0,count=0;
        if(!value(s,0x0c000041,active) || active!=1 || !value(s,0x0c000042,count) || count<0 || count>7) return false;
        if(count==7) {attempts[slot(kind)].handled=true;return false;}
        for(unsigned low=0x0c000043;low<=0x0c000049;++low) if(!add(j,s,low,1)) return false;
        // Seven actual pickup guards prevent a later blue touch raising8.
        // The original counter display enables this phase's seventh-pickup
        // chest. The starter's normal30/40-second timer and cleanup continue.
        return add(j,s,0x0c000042,7) && request(j,s,j.rule->workers==55?36u:25u,6,j.rule->targetPc);
    }
    int choice=-1,own=-1,opponent=-1,limit=0,period=-1;
    if(!value(s,0x0c00128d,choice) || choice<0 || choice>2 || !value(s,0x1452,own) || own<0 || own>99 ||
        !value(s,0x1453,opponent) || opponent<0 || opponent>99 || !value(s,0x5c000148,limit) || limit<=0 || limit>36000 ||
        !value(s,0x1454,period) || period<0 || period>1) return false;
    opponent=std::min(opponent,98);own=std::max(own,opponent+1);
    // Native W23 expires the final period and preempts the live W36 action;
    // W31/W38 and the outside-match engine still decide results and prizes.
    return add(j,s,0x1452,own) && add(j,s,0x1453,opponent) && add(j,s,0x1454,1) && add(j,s,0x5c000144,limit);
}
bool prepare(Job& j) {
    if(!j.used || !allowed() || j.epoch!=generation() || j.context!=contextSerial ||
        !setting(j.rule->setting) || settingRevision(j.rule->setting)!=j.revision) return false;
    auto& a=attempts[slot(j.rule->setting)];
    if(a.serial!=j.attempt || a.pending!=j.ticket || a.handled) return false;
    UtilityScript s;unsigned entry=0,pc=0;
    if(!identity(s,*j.rule) || s.chunk!=j.chunk || s.controller!=j.controller || s.allocationSerial!=j.allocation ||
        !owner(s,j.worker,j.rule->local) || read<uintptr_t>(j.worker+0x90)!=j.signal ||
        !signal(j.worker,entry,pc,s) || entry!=j.rule->entry) return false;
    const auto at=s.chunk+s.codeOffset+j.rule->pc;
    unsigned index=0;
    if(read<uint8_t>(at)!=0xa0 || read<uint16_t>(at+1)!=j.rule->index || !binding(s,j.rule->low,index) || index!=j.rule->index) return false;
    j.writes=j.requests=0;
    j.source=read<uint16_t>(s.controller+0xa,0xffff);
    return j.source<read<uint16_t>(s.controller+0xc) && plan(j,s);
}
void invoke(Job& j) {
    using Setter=void(__cdecl*)(uintptr_t,int,int,int,int);
    using Req=int(__cdecl*)(int,int,int,int,int);
    using Dispatch=void(__cdecl*)(int,int);
    // A partially refused native dispatch must never replay the earlier writes
    // or finish requests. A fresh native attempt is needed after such a refusal.
    attempts[slot(j.rule->setting)].handled=true;
    const auto set=reinterpret_cast<Setter>(address(0x4713a0));
    for(unsigned i=0;i<j.writes;++i) set(j.worker,j.values[i].index,0,j.values[i].value,0);
    for(unsigned i=0;i<j.requests;++i) {
        const auto& r=j.calls[i];
        if(reinterpret_cast<Req>(address(0x46e920))(j.source,r.global,0,7,r.entry)!=1) {
            recordLog("minigame completion: native request refused kind=%u worker=%u entry=%u",slot(j.rule->setting),r.local,r.entry);return;
        }
        reinterpret_cast<Dispatch>(address(0x465390))(r.global,read<uintptr_t>(r.worker+0x90)?-1:10);
    }
}
void queue(const Rule& rule,UtilityScript& s) {
    auto& a=attempts[slot(rule.setting)];
    if(a.pending || a.handled) return;
    unsigned index=0;while(index<jobs.size() && jobs[index].used) ++index;
    if(index==jobs.size()) return;
    auto& j=jobs[index];j={};j.used=true;j.ticket=++ticketSerial;
    if(!j.ticket) j.ticket=++ticketSerial;
    j.context=contextSerial;j.rule=&rule;j.revision=settingRevision(rule.setting);j.attempt=a.serial;
    j.controller=s.controller;j.chunk=s.chunk;j.worker=s.worker;j.signal=read<uintptr_t>(s.worker+0x90);
    j.epoch=generation();j.allocation=s.allocationSerial;a.pending=j.ticket;
    const auto ticket=j.ticket;
    NativeAction action;action.epoch=j.epoch;
    action.prepare=[index,ticket] {return jobs[index].ticket==ticket && prepare(jobs[index]);};
    action.invoke=[index,ticket] {if(jobs[index].ticket==ticket) invoke(jobs[index]);};
    action.retire=[index,ticket] {
        auto& job=jobs[index];if(job.ticket!=ticket || !job.used) return;
        auto& attempt=attempts[slot(job.rule->setting)];if(attempt.pending==ticket) attempt.pending=0;
        job.used=false;
    };
    if(!deferNativeAction(action)) {a.pending=0;j.used=false;}
}
void cancelContext(Event&) {cancelMinigameCompletion();}
void entered(Event& e) {e.before[24]=interpreter;e.before[25]=interpreterEpoch;interpreter=e.args[0];interpreterEpoch=generation();}
void left(Event& e) {interpreter=e.before[24];interpreterEpoch=static_cast<unsigned>(e.before[25]);}
void changed(Event& e) {
    // Default-Off and unrelated operands return before any engine/context read.
    if(e.args[1]!=17 && e.args[1]!=35 && e.args[1]!=50 && e.args[1]!=52 && e.args[1]!=104 && e.args[1]!=258) return;
    if(!setting(Setting::ChocoboAssist) && !setting(Setting::RemiemAssist) &&
        !setting(Setting::ButterflyAssist) && !setting(Setting::BlitzAssist)) return;
    if(!allowed() || e.epoch!=generation() || e.parentRva!=0x464180 || interpreterEpoch!=generation() ||
        interpreter!=e.args[0] || e.args[2]) return;
    for(const auto& r:rules) {
        if(!setting(r.setting)) continue;
        const auto local=read<uint16_t>(e.args[0]+0x2e,0xffff);
        const bool blitzReset=r.setting==Setting::BlitzAssist && local==23 && e.args[1]==52;
        if(!blitzReset && (local!=r.local || e.args[1]!=r.index)) continue;
        UtilityScript s;unsigned entry=0,pc=0,index=0;
        if(!identity(s,r) || !owner(s,e.args[0],local) || !signal(s.worker,entry,pc,s)) continue;
        const unsigned reset=2868116; // W23 is unchanged in the PSV variant; W36 shifts later.
        const bool initial=blitzReset?(entry==6 && pc==reset && !e.args[3]):
            ((pc==r.initial1 || (r.initial2 && pc==r.initial2)) && (entry==6 || entry==7));
        if(blitzReset && !initial) continue;
        if(!blitzReset && !initial && (entry!=r.entry || pc!=r.pc)) continue;
        const auto low=blitzReset?0x5c000144u:r.low;
        if(read<uint8_t>(s.chunk+s.codeOffset+pc)!=0xa0 || read<uint16_t>(s.chunk+s.codeOffset+pc+1)!=e.args[1] ||
            !binding(s,low,index) || index!=e.args[1]) continue;
        auto& a=attempts[slot(r.setting)];
        const auto sig=read<uintptr_t>(s.worker+0x90);
        if(initial || a.epoch!=generation() || a.chunk!=s.chunk || a.allocation!=s.allocationSerial || (!blitzReset && a.signal!=sig)) {
            const auto serial=a.serial+1;a={};a.epoch=generation();a.chunk=s.chunk;a.allocation=s.allocationSerial;a.signal=sig;a.serial=serial;
        }
        if(!initial) queue(r,s);
        return;
    }
}
}
bool minigameAssistAvailable(Setting id) {
    if(!nativeGameThread() || inBattle()) return false;
    // UI availability must not depend on completion-resource preparation. The
    // native room/resource header also exposes the option before race start.
    if(id==Setting::RemiemAssist && areaIndex(read<uint16_t>(address(0xd2ca90)))==276) return true;
    UtilityScript s;if(!captureUtilityScript(s)) return false;
    if(id==Setting::BlitzAssist && s.name=="bltz0200" && s.length==113266 && s.workers==7) return true;
    for(const auto& r:rules) if(id==r.setting && s.name==r.name && s.length==r.length && s.workers==r.workers) return true;
    return false;
}
void cancelMinigameCompletion() {++contextSerial;for(auto& a:attempts) a.pending=0;}
void resetMinigameCompletion() {cancelMinigameCompletion();attempts={};interpreter=0;interpreterEpoch=0;}
void installMinigameCompletion() {
    hook(0x464180,entered,left);hook(0x4713a0,nullptr,changed);
    hook(0x4aa0b0,cancelContext,nullptr); // Existing native window-open producer.
    hook(0x4a0d70,cancelContext,nullptr); // Existing battle-entry producer.
}
}
