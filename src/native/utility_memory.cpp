#include "utility_memory.hpp"
#include "events.hpp"
#include "utilities.hpp"
#include "utility_context.hpp"
#include "utility_scripts.hpp"
#include "native_worker_index.hpp"
#include "atel_resources.hpp"
#include <array>
#include <stdexcept>

namespace ffx {
namespace {
using WorkerGetter = uintptr_t(__cdecl*)(int);
enum class DeclarationFailure {None,Owner,Pending,Changed};
DeclarationFailure captureVariableIndex(const UtilityScript& script,unsigned low,unsigned high,unsigned& index) {
    AtelSpan span;
    if(!nativeAtelSpan(script.chunk,span) || span.serial!=script.allocationSerial || script.header<script.chunk ||
        script.header-script.chunk>span.size || span.size-(script.header-script.chunk)<0x28) return DeclarationFailure::Owner;
    if(!readyAtelDeclaration(script.chunk,static_cast<unsigned>(script.header-script.chunk),low,high,index)) return DeclarationFailure::Pending;
    const auto offset=read<unsigned>(script.header+0x14);unsigned row[2]{};
    if(index>=read<uint16_t>(script.header+2) || offset>span.size || (index+1ull)*8>span.size-offset ||
        !copy(script.chunk+offset+index*8,row,sizeof(row)) || row[0]!=low || row[1]!=high) return DeclarationFailure::Changed;
    return DeclarationFailure::None;
}
unsigned variable(const UtilityScript& script, unsigned low, unsigned high) {
    unsigned index=0;switch(captureVariableIndex(script,low,high,index)) {
    case DeclarationFailure::Owner:throw std::runtime_error("Native declaration owner changed");
    case DeclarationFailure::Pending:throw std::runtime_error("Required owned script declaration is not ready");
    case DeclarationFailure::Changed:throw std::runtime_error("Native declaration changed after preparation");
    default:return index;
    }
}
}
AtelPreparation utilityPreparation(uintptr_t* capturedChunk) {
    const auto controller=read<uintptr_t>(address(0xf26ae8));
    const auto count=read<uint16_t>(controller+0xc);
    if(!controller || read<uint16_t>(controller+0x1a,0xffff)!=0 || !count || count>500) return AtelPreparation::Missing;
    const auto worker=reinterpret_cast<WorkerGetter>(address(0x46a7c0))(read<uint16_t>(controller+0xa));
    const auto chunk=read<uintptr_t>(worker+4);
    if(capturedChunk) *capturedChunk=chunk;
    return chunk?atelPreparation(chunk):AtelPreparation::Missing;
}
bool captureUtilityScript(UtilityScript& script) noexcept {
    if(!nativeGameThread()) return false;
    try {
        const auto controller=read<uintptr_t>(address(0xf26ae8));
        const unsigned count=read<uint16_t>(controller+0xc),global=read<uint16_t>(controller+0xa);
        if(!controller || read<uint16_t>(controller+0x1a,0xffff)!=0 || !count || count>500 || global>=count) return false;
        const auto worker=reinterpret_cast<WorkerGetter>(address(0x46a7c0))(static_cast<int>(global));
        const auto header=read<uintptr_t>(worker),chunk=read<uintptr_t>(worker+4);AtelSpan span;
        if(!worker || !header || !nativeAtelSpan(chunk,span) || span.size<0x38 || header<chunk ||
            header-chunk>span.size || span.size-(header-chunk)<0x28) return false;
        const unsigned length=read<unsigned>(chunk),offset=read<unsigned>(chunk+0x30),workers=read<uint16_t>(chunk+0x34),nameAt=read<unsigned>(chunk+0xc);
        if(!length || offset>span.size || length>span.size-offset || !workers || workers>1024 ||
            (0x38ull+workers*4)>span.size || nameAt>span.size-8) return false;
        char name[9]{};if(!copy(chunk+nameAt,name,8)) return false;
        for(unsigned i=0;i<8 && name[i];++i) if(!(name[i]>='a' && name[i]<='z') && !(name[i]>='0' && name[i]<='9')) return false;
        // The candidate's eight-character name fits the verified x86 std::string
        // small buffer. Production callers keep this owner-local carrier.
        script.controller=controller;script.worker=worker;script.header=header;script.chunk=chunk;
        script.length=length;script.codeOffset=offset;script.workers=workers;script.allocationSerial=span.serial;script.name=name;
        return true;
    } catch(...) {return false;}
}
UtilityScript utilityScript() {
    UtilityScript script;
    script.controller = read<uintptr_t>(address(0xf26ae8));
    const unsigned count = read<uint16_t>(script.controller + 0xc);
    if (!script.controller || read<uint16_t>(script.controller+0x1a,0xffff)!=0 || !count || count > 500) throw std::runtime_error("No active field script");
    script.worker = reinterpret_cast<WorkerGetter>(address(0x46a7c0))(read<uint16_t>(script.controller + 0xa));
    script.header = read<uintptr_t>(script.worker);
    script.chunk = read<uintptr_t>(script.worker + 4);
    if (!script.header || !script.chunk) throw std::runtime_error("No active field worker");
    AtelSpan span;
    if(!nativeAtelSpan(script.chunk,span) || span.size<0x38 || script.header<script.chunk ||
        script.header-script.chunk>span.size || span.size-(script.header-script.chunk)<0x28)
        throw std::runtime_error("No exact native field resource receipt");
    script.length = read<unsigned>(script.chunk);
    script.allocationSerial=span.serial;
    script.codeOffset = read<unsigned>(script.chunk + 0x30);
    script.workers = read<uint16_t>(script.chunk + 0x34);
    const auto nameOffset = read<unsigned>(script.chunk + 0xc);
    if (!script.length || script.codeOffset>span.size || script.length>span.size-script.codeOffset ||
        !script.workers || script.workers > 1024 || nameOffset>span.size-8)
        throw std::runtime_error("Invalid native script identity");
    char name[9]{};
    if (!copy(script.chunk + nameOffset, name, 8)) throw std::runtime_error("Unreadable native script name");
    for (unsigned i = 0; name[i] && i < 8; ++i)
        if (!(name[i] >= 'a' && name[i] <= 'z') && !(name[i] >= '0' && name[i] <= '9'))
            throw std::runtime_error("Invalid native script name");
    script.name = name;
    return script;
}
unsigned scriptChecksum(const UtilityScript& script) {
    AtelIdentity identity;
    if(!readyAtelIdentity(script.chunk,identity) || identity.span.serial!=script.allocationSerial || identity.length!=script.length || identity.workers!=script.workers ||
        identity.codeOffset!=script.codeOffset || script.name!=identity.name)
        throw std::runtime_error("Owned field script identity is not ready");
    return identity.checksum;
}
uintptr_t utilityWorker(const UtilityScript& script, unsigned local) {
    AtelSpan span;
    if(!nativeAtelSpan(script.chunk,span) || span.serial!=script.allocationSerial) throw std::runtime_error("Native utility allocation changed");
    if (local >= script.workers) throw std::runtime_error("Worker outside the verified script");
    NativeWorkerIdentity identity;
    if(!nativeWorkerIdentity(script.controller,script.chunk,local,identity)) throw std::runtime_error("Required worker is not loaded");
    const auto worker=identity.worker;
    unsigned headerOffset=0;
    if(!readyAtelWorkerHeader(script.chunk,local,headerOffset) || read<uintptr_t>(worker)!=script.chunk+headerOffset)
        throw std::runtime_error("Worker ownership mismatch");
    return worker;
}
bool captureUtilityWorker(const UtilityScript& script,unsigned local,uintptr_t& worker) noexcept {
    if(!nativeGameThread()) return false;
    try {
    AtelSpan span;NativeWorkerIdentity identity;unsigned header=0;
    if(local>=script.workers || !nativeAtelSpan(script.chunk,span) || span.serial!=script.allocationSerial ||
        !nativeWorkerIdentity(script.controller,script.chunk,local,identity) ||
        !readyAtelWorkerHeader(script.chunk,local,header) || header>span.size || span.size-header<0x28 ||
        read<uintptr_t>(identity.worker)!=script.chunk+header) return false;
    worker=identity.worker;return true;
    } catch(...) {return false;}
}
int utilityValue(const UtilityScript& script, unsigned low, unsigned high, unsigned element) {
    using Getter = uintptr_t(__cdecl*)(uintptr_t, int, int);
    if (element >= (high & 0xffff)) throw std::runtime_error("Element outside the native declaration");
    const unsigned index = variable(script, low, high);
    const auto pointer = reinterpret_cast<Getter>(address(0x46c500))(script.worker, index, element);
    unsigned value = 0;
    const unsigned format = low >> 28;
    const unsigned size = format < 2 ? 1 : format < 4 ? 2 : 4;
    if (format > 5 || !copy(pointer, &value, size)) throw std::runtime_error("Unreadable native variable");
    switch (low >> 28) {
    case 0: return static_cast<uint8_t>(value);
    case 1: return static_cast<int8_t>(value);
    case 2: return static_cast<uint16_t>(value);
    case 3: return static_cast<int16_t>(value);
    case 4: case 5: return static_cast<int>(value);
    default: throw std::runtime_error("Expected an integer declaration");
    }
}
bool captureUtilityValue(const UtilityScript& script,unsigned low,unsigned high,unsigned element,int& result) noexcept {
    if(!nativeGameThread() || element>=(high&0xffff) || low>>28>5) return false;
    try {
        unsigned index=0;if(captureVariableIndex(script,low,high,index)!=DeclarationFailure::None) return false;
        using Getter=uintptr_t(__cdecl*)(uintptr_t,int,int);
        const auto source=reinterpret_cast<Getter>(address(0x46c500))(script.worker,index,element);
        const unsigned format=low>>28,size=format<2?1u:format<4?2u:4u;unsigned value=0;
        if(!copy(source,&value,size)) return false;
        result=format==0?static_cast<int>(static_cast<uint8_t>(value)):format==1?static_cast<int>(static_cast<int8_t>(value)):
            format==2?static_cast<int>(static_cast<uint16_t>(value)):format==3?static_cast<int>(static_cast<int16_t>(value)):static_cast<int>(value);
        return true;
    } catch(...) {return false;}
}
void utilityWrite(const UtilityScript& script, const UtilityBinding& value) {
    using Setter = void(__cdecl*)(uintptr_t, int, int, int, int);
    if (value.element >= (value.high & 0xffff) || value.low >> 28 > 5) throw std::runtime_error("Invalid solver element");
    const unsigned index = variable(script, value.low, value.high);
    reinterpret_cast<Setter>(address(0x4713a0))(script.worker, index, value.element, value.value, 0);
}
void utilityRequest(const UtilityScript& script, unsigned local, unsigned entry,FixedCallback<void(bool),64> completed,FixedCallback<bool(),64> qualified,bool dispatchIdle,bool activateWorker) {
    using Request = int(__cdecl*)(int, int, int, int, int);
    struct RequestData { int source=0,target=0,result=0; uintptr_t function=0,worker=0,dispatch=0; bool activateWorker=false; };
    struct ScriptSnapshot {uintptr_t controller,worker,header,chunk;unsigned length,codeOffset,workers,serial;};
    const ScriptSnapshot identity{script.controller,script.worker,script.header,script.chunk,script.length,script.codeOffset,script.workers,script.allocationSerial};
    const auto token=utilityContext();
    NativeAction action; action.epoch=generation();
    action.prepare=[identity,local,entry,token,qualified,dispatchIdle,activateWorker](NativeAction& command) mutable {
        auto data=command.data<RequestData>();
        UtilityScript copiedScript;copiedScript.controller=identity.controller;copiedScript.worker=identity.worker;
        copiedScript.header=identity.header;copiedScript.chunk=identity.chunk;copiedScript.length=identity.length;
        copiedScript.codeOffset=identity.codeOffset;copiedScript.workers=identity.workers;
        copiedScript.allocationSerial=identity.serial;
        if(!utilityContextValid(token,true) || !utilityActionAllowed() ||
            read<uintptr_t>(address(0xf26ae8))!=copiedScript.controller) return false;
        if(qualified && !qualified()) return false;
        const auto worker=utilityWorker(copiedScript,local);
        const auto header=read<uintptr_t>(worker);
        if(entry>=read<uint16_t>(header+8)) return false;
        NativeWorkerIdentity targetIdentity;
        if(!nativeWorkerIdentity(copiedScript.controller,copiedScript.chunk,local,targetIdentity) || targetIdentity.worker!=worker) return false;
        data.source=read<uint16_t>(copiedScript.controller+0xa);data.target=static_cast<int>(targetIdentity.global);
        data.function=address(0x46e920);data.worker=worker;data.dispatch=dispatchIdle?address(0x465390):0;
        data.activateWorker=activateWorker;
        command.data(data);return true;
    };
    action.invoke=[entry](NativeAction& command) {
        auto data=command.data<RequestData>();
        const auto flags=read<uint16_t>(data.worker+0x34);
        // Kilika W16:1 explicitly deactivates W35 on room entry. Its normal
        // door solution later uses Common00A8 (8577F0) to OR only bit2 back.
        // The verified automatic insertion opts into that same prerequisite;
        // ordinary requests never wake disabled workers.
        if(data.activateWorker && !write<uint16_t>(data.worker+0x34,flags|2)) return;
        data.result=reinterpret_cast<Request>(data.function)(data.source,data.target,0,7,entry);command.data(data);
        if(data.result!=1 && data.activateWorker && !(flags&2))
            write<uint16_t>(data.worker+0x34,read<uint16_t>(data.worker+0x34)&~2u);
        // 86E920 only enqueues. An idle worker can retain its previous priority,
        // so the ordinary -1 scheduler update cannot start an equal-level job.
        // Use the ordinary -1 priority rule while another signal is active;
        // mirror 869170's post-completion dispatch10 only when idle. Native
        // gating/preemption/order/initialization remain authoritative.
        if(data.result==1 && data.dispatch)
            reinterpret_cast<void(__cdecl*)(int,int)>(data.dispatch)(data.target,read<uintptr_t>(data.worker+0x90)?-1:10);
        recordLog("utility request worker=%u entry=%u result=%d flags-before=%x flags=%x priority=%u queued=%p active=%p",
            read<uint16_t>(data.worker+0x2e),entry,data.result,flags,read<uint16_t>(data.worker+0x34),read<uint8_t>(data.worker+0x32),
            reinterpret_cast<void*>(read<uintptr_t>(data.worker+0x80)),reinterpret_cast<void*>(read<uintptr_t>(data.worker+0x90)));
    };
    action.complete=[completed](NativeAction& command,bool invoked) mutable {
        const auto data=command.data<RequestData>();
        const bool success=invoked && data.result==1;
        if(completed) completed(success);
        if(!success) utilityReply(L"The native script request was cancelled or refused.");
    };
    if(!deferNativeAction(std::move(action))) throw std::runtime_error("Native action owner or queue unavailable");
}
void utilityWarp(unsigned room, unsigned spawn,bool force,bool macalaniaTest,bool gagazetTest) {
    using Warp = void(__cdecl*)(int, int, int);
    using Setter = void(__cdecl*)(uintptr_t,int,int,int,int);
    using Getter = uintptr_t(__cdecl*)(uintptr_t,int,int);
    struct WarpData { uintptr_t controller=0,function=0,storyWorker=0,storySetter=0,globals=0; const wchar_t* refusal=nullptr; unsigned storyIndex=0; bool addedFlag=false,setupFailed=false; };
    if(macalaniaTest && (room!=239 || spawn!=1 || force)) throw std::runtime_error("Invalid Macalania test destination");
    if(gagazetTest && (room!=272 || spawn!=0 || force || macalaniaTest)) throw std::runtime_error("Invalid Gagazet test destination");
    const auto token=utilityContext();
    NativeAction action; action.epoch=generation();
    action.prepare=[token,force,macalaniaTest,gagazetTest](NativeAction& command) {
        auto data=command.data<WarpData>();
        if(!utilityContextValid(token,true) || !utilityActionAllowed(force)) return false;
        const auto refuse=[&](const wchar_t* reason) {data.refusal=reason;command.data(data);return false;};
        UtilityScript script;
        if(macalaniaTest || gagazetTest) {
            if(!captureUtilityScript(script)) return refuse(L"Test warp cancelled: the current field script is not ready. Try again after loading finishes.");
        } else script=utilityScript();
        data.controller=script.controller;
        if(macalaniaTest || gagazetTest) {
            if(utilityBusy()) return false;
            if(captureVariableIndex(script,0x20000a00,1,data.storyIndex)!=DeclarationFailure::None)
                return refuse(L"Test warp cancelled: the current story declaration is not ready. Try again after loading finishes.");
            const auto story=reinterpret_cast<Getter>(address(0x46c500))(script.worker,data.storyIndex,0);
            uintptr_t globals=0;uint16_t progress=0;uint8_t selector=0;
            // Original FUN_0086c270 location0 resolves controller+2c plus the
            // declaration's low24-bit offset. Salvage Ship declares story+a00,
            // but not selector+280. Qualify this same global owner through the
            // real story getter before copying the single selector byte.
            if(!copy(script.controller+0x2c,&globals,sizeof(globals)) || !globals || globals>UINTPTR_MAX-0xa02 ||
                story!=globals+0xa00 || !copy(story,&progress,sizeof(progress)) || !copy(globals+0x280,&selector,sizeof(selector)))
                return refuse(L"Test warp cancelled: the current story memory could not be verified.");
            if(progress>1530) return refuse(gagazetTest?L"Load an early save, such as the Salvage Ship, before using warp gagazet-test.":L"Load an early save, such as the Salvage Ship, before using warp macalania-test.");
            if(selector!=0) return refuse(L"Testing requires a normal early-save initialization state.");
            if(gagazetTest) {
                // Both original Gagazet scripts declare location0 D8 as the
                // same three-byte cave state. The source need not declare it.
                // Qualify its owner through the actual story getter above.
                std::array<uint8_t,3> caveFlags{};
                if(!copy(globals+0xd8,caveFlags.data(),caveFlags.size())) return refuse(L"Gagazet test setup cancelled: cave flags are not ready.");
                data.globals=globals;
            }
            data.storyWorker=script.worker;
            data.storySetter=address(0x4713a0);
        }
        const auto flags=read<uint8_t>(script.controller); data.addedFlag=(flags&0x80)==0;
        data.function=address(0x46fed0);
        command.data(data);return write<uint8_t>(script.controller,flags|0x80);
    };
    action.invoke=[room,spawn,gagazetTest](NativeAction& command) {
        auto data=command.data<WarpData>();
        // Explicit developer tests only. Preparation and mutation share this
        // native owner; a cancelled/stale command never changes test progress.
        if(gagazetTest && !write<std::array<uint8_t,3>>(data.globals+0xd8,{})) {
            data.setupFailed=true;data.refusal=L"Gagazet test setup could not reset the cave flags. No transition was started.";
            command.data(data);return;
        }
        // Gagazet2625 admits native cave setup below2680's already-complete
        // first-trial branch. Dry-cave W33's original W15:7 event then forms
        // the swimming party. Macalania1550/entrance1 selects W30:1's playable
        // layout. Neither test command sets a puzzle success or barrier state.
        if(data.storySetter) reinterpret_cast<Setter>(data.storySetter)(data.storyWorker,data.storyIndex,0,gagazetTest?2625:1550,0);
        reinterpret_cast<Warp>(data.function)(room,spawn,0);
    };
    action.complete=[](NativeAction& command,bool success) {
        const auto data=command.data<WarpData>();
        // Completion is called only in the original generation. A successful
        // load resets the generation, so its old controller is never touched.
        if(data.controller && data.addedFlag &&
            read<uintptr_t>(address(0xf26ae8))==data.controller) {
            const auto flags=read<uint8_t>(data.controller);
            write<uint8_t>(data.controller,flags&~0x80);
        }
        if(!success || data.setupFailed) utilityReply(data.refusal?data.refusal:L"The field transition was cancelled or failed.");
    };
    if(!deferNativeAction(std::move(action))) throw std::runtime_error("Native action owner or queue unavailable");
}
}
