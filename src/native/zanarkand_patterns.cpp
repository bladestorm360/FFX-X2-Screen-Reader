// Observe completed, original pattern-display interactions; never solve a floor.
#include "zanarkand_patterns.hpp"
#include "utility_trials.hpp"
#include "utility_memory.hpp"
#include "utility_text.hpp"
#include "settings.hpp"
#include "events.hpp"
namespace ffx {
namespace {
// Only the serialized native owner accesses these identities. Text workers
// receive ready owned plans, and F8 receives only the bounded POD snapshot.
struct Display {
    uintptr_t worker=0,signal=0,chunk=0;
    unsigned epoch=0,allocation=0,code=0,level=0,serial=0;
    const ZanarkandSequence* sequence=nullptr;
} pending;
unsigned nextDisplay=0;
struct History {uintptr_t chunk=0;unsigned epoch=0,allocation=0;ZanarkandPatternSnapshot snapshot;} history;
bool verified(UtilityScript& script) {
    return nativeGameThread() && !inBattle() && captureUtilityScript(script) &&
        script.name=="dome0700" && captureCurrentTrial(script);
}
bool stage(UtilityScript script,int& result) {
    if(!captureUtilityWorker(script,0,script.worker)) return false;
    script.header=read<uintptr_t>(script.worker);
    return captureUtilityValue(script,0x5c000004,1,0,result);
}
uintptr_t thread(uintptr_t worker,uintptr_t signal,unsigned& level) {
    level=read<uint8_t>(signal+0xe)&15;
    const auto type=read<uint8_t>(read<uintptr_t>(worker));
    if(level>((type==5 || type==6)?1u:8u)) return 0;
    return worker+0x12c+level*0x4c;
}
void initialized(Event& e) {
    const auto worker=e.args[0],signal=e.args[1];
    if(e.epoch!=generation() || !nativeGameThread() || !worker || !signal || read<uint16_t>(signal+8,0xffff)!=2) return;
    const auto local=read<uint16_t>(worker+0x2e,0xffff);
    if(local!=53 && local!=54) return;
    UtilityScript script;uintptr_t registered=0;int currentStage=-1;
    if(!verified(script) || read<uintptr_t>(worker+4)!=script.chunk ||
       !captureUtilityWorker(script,local,registered) || registered!=worker || !stage(script,currentStage)) return;
    // A new display supersedes prior visible data, including an empty native
    // main-display stage. It cannot inherit the preceding sequence.
    pending={};history={};
    const ZanarkandSequence* sequence=nullptr;
    for(const auto& row:zanarkandSequences) if(row.worker==local && (local==53 || row.stage==static_cast<unsigned>(currentStage))) {sequence=&row;break;}
    if(!sequence) return;
    unsigned level=0;const auto t=thread(worker,signal,level);
    const auto code=script.chunk+script.codeOffset;
    if(!t || read<uintptr_t>(t+0x18)!=code+sequence->entryPc) return;
    pending={worker,signal,script.chunk,generation(),script.allocationSerial,script.codeOffset,level,++nextDisplay,sequence};
}
void finishing(Event& e) {
    if(!pending.sequence || pending.epoch!=generation() || e.epoch!=generation() || !nativeGameThread()) return;
    using Worker=uintptr_t(__cdecl*)(int);
    const auto worker=reinterpret_cast<Worker>(address(0x46a7c0))(static_cast<int>(e.args[0]));
    if(worker!=pending.worker) return;
    e.before[0]=1;e.before[2]=pending.serial;
    UtilityScript script;int currentStage=-1;AtelSpan span;
    if(!verified(script) || script.chunk!=pending.chunk || script.allocationSerial!=pending.allocation ||
       !nativeAtelSpan(pending.chunk,span) || span.serial!=pending.allocation ||
       read<uintptr_t>(worker+0x90)!=pending.signal || read<uint16_t>(pending.signal+8,0xffff)!=2 ||
       !stage(script,currentStage) || (pending.sequence->worker==54 && currentStage!=static_cast<int>(pending.sequence->stage))) return;
    unsigned level=0;const auto t=thread(worker,pending.signal,level);
    // 864180's RET branch calls869170 before its ordinary PC increment.
    // Only the verified final RET proves that every original display ran;
    // cancellation/cleanup at an earlier PC never publishes a sequence.
    if(!t || level!=pending.level || read<uintptr_t>(t+0x18)!=pending.chunk+pending.code+pending.sequence->endPc ||
       read<uint8_t>(pending.chunk+pending.code+pending.sequence->endPc)!=0x3c) return;
    e.before[1]=1;
}
void finished(Event& e) {
    // 869170 can initialize the next queued signal before returning. Its new
    // display must survive this earlier completion without being published.
    if(!e.before[0] || !pending.sequence || static_cast<unsigned>(e.before[2])!=pending.serial) return;
    const auto display=pending;pending={};
    AtelSpan span;
    if(!e.before[1] || display.epoch!=generation() || !display.sequence ||
       !nativeAtelSpan(display.chunk,span) || span.serial!=display.allocation) return;
    const auto& sequence=*display.sequence;
    history={display.chunk,display.epoch,display.allocation,{sequence.count,sequence.worker==54,sequence.glyphs}};
    UtilityText text(sequence.worker==53?L"First chamber displayed glyph sequence.":L"Main chamber displayed glyph sequence.");
    if(setting(Setting::AutoDetail)) {
        text.literal(L" ");
        for(unsigned i=0;i<sequence.count;++i)
            text.literal(zanarkandGlyphLabel(sequence.glyphs[i])).literal(i+1<sequence.count?L". ":L".");
    }
    emitUtilityText(text,"zanarkand-pattern");
}
}
bool captureZanarkandPattern(ZanarkandPatternSnapshot& result) {
    result={};UtilityScript script;
    if(!history.snapshot.count || history.epoch!=generation() || !verified(script) ||
       history.chunk!=script.chunk || history.allocation!=script.allocationSerial) return false;
    result=history.snapshot;return true;
}
void installZanarkandPatterns() {
    hook(0x469040,nullptr,initialized);hook(0x469170,finishing,finished);
}
}
