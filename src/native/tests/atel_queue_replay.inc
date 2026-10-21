#include "atel_queue_bytes.inc"
namespace {
std::map<unsigned,uintptr_t> queueCode;
std::array<uint8_t,0x1000> queueDescriptors{};
std::array<uint8_t,64> spareSignal{};
unsigned queueFailure=0,queueStarts=0,queueDispatches=0;
int __cdecl queueUnpaused() {return 0;}
unsigned __cdecl queueWorkerOperand(uintptr_t,uintptr_t) {return 1;}
void __cdecl queueUnexpected() {check(false,"original scheduler attempted an unmodeled active-thread operation");}
void __cdecl queueThreadSetup(uintptr_t w,uintptr_t,unsigned) {check(read<uint8_t>(read<uintptr_t>(w))==3,"original type3 zone has no character stack setup");}
void __cdecl queueInitObserved(uintptr_t w,uintptr_t sig) {
    reinterpret_cast<void(__cdecl*)(uintptr_t,uintptr_t)>(queueCode.at(0x869040))(w,sig);
    Event e;e.epoch=generation();e.args[0]=w;e.args[1]=sig;fire(0x469040,e);++queueStarts;
    initPc=static_cast<unsigned>(read<uintptr_t>(w+0x12c+(read<uint8_t>(sig+14)&15)*0x4c+0x18)-(ptr(chunk)+0x1000));
}
void __cdecl dispatchQueued(int index,int level) {
    if(!originalRequestCode) {
        check(read<uintptr_t>(worker(index)+0x90)!=0,"synchronous SDK boundary already initialized its accepted request");
        return;
    }
    ++queueDispatches;reinterpret_cast<void(__cdecl*)(int,int)>(queueCode.at(0x865390))(index,level);
}
uintptr_t queueDependency(unsigned va) {
    if(va==0x86a7c0)return reinterpret_cast<uintptr_t>(worker);
    if(va==0x86de90)return reinterpret_cast<uintptr_t>(queueWorkerOperand);
    if(va==0x869040)return reinterpret_cast<uintptr_t>(queueInitObserved);
    if(va==0x780d60)return reinterpret_cast<uintptr_t>(queueUnpaused);
    if(va==0x8636f0)return reinterpret_cast<uintptr_t>(queueThreadSetup);
    if(va==0x8725f0 || va==0x862c00 || va==0x862c60 || va==0x872800 || va==0x863980)return reinterpret_cast<uintptr_t>(queueUnexpected);
    if(va==0x1326ae8)return address(0xf26ae8);
    if(va==0x1327098)return reinterpret_cast<uintptr_t>(&queueFailure);
    if(va>=0x1325b60 && va<=0x1325d5c)return reinterpret_cast<uintptr_t>(queueDescriptors.data())+va-0x1325b60;
    return queueCode.at(va);
}
template<size_t N,size_t M> void queueBody(unsigned va,const unsigned char (&body)[N],const std::array<AtelReplayFixup,M>& fixups) {
    const auto at=queueCode.at(va);std::memcpy(reinterpret_cast<void*>(at),body,N);
    for(const auto& fix:fixups) {const auto value=queueDependency(fix.va)-(fix.relative?at+fix.offset+4:0);std::memcpy(reinterpret_cast<void*>(at+fix.offset),&value,4);}
    FlushInstructionCache(GetCurrentProcess(),reinterpret_cast<void*>(at),N);
}
void prepareQueueCode() {
    for(unsigned va:{0x86e920u,0x86e990u,0x865390u,0x869040u,0x869170u,0x86dba0u,0x86dbc0u,0x86dbe0u,0x8715f0u,0x8576c0u,0x8577f0u}) {
        const auto p=VirtualAlloc(nullptr,4096,MEM_RESERVE|MEM_COMMIT,PAGE_EXECUTE_READWRITE);check(p!=nullptr,"isolated original queue code allocated");queueCode[va]=reinterpret_cast<uintptr_t>(p);
    }
    queueBody(0x86e920,queueRequestBody,queueRequestFixups);queueBody(0x86e990,queueInsertBody,queueInsertFixups);
    queueBody(0x865390,queueDispatchBody,queueDispatchFixups);queueBody(0x869040,queueInitBody,queueInitFixups);
    queueBody(0x869170,queueFinishBody,queueFinishFixups);queueBody(0x86dba0,queueAfterBody,queueAfterFixups);
    queueBody(0x86dbc0,queueBeforeBody,queueBeforeFixups);queueBody(0x86dbe0,queueAppendBody,queueAppendFixups);
    queueBody(0x8715f0,queueAckBody,queueAckFixups);
    queueBody(0x8576c0,queueDeactivateBody,queueDeactivateFixups);
    queueBody(0x8577f0,queueActivateBody,queueActivateFixups);
}
void queuePriorRunning() {
    spareSignal.fill(0);const auto spare=reinterpret_cast<uintptr_t>(spareSignal.data());
    // A prior completed level7 request leaves +32=7. Real RET dispatches the
    // already queued work, then later -1 updates compare against that old value.
    put(chunk,0x500,uint8_t{3});put(target,0x34,uint16_t{14});put(target,0x32,uint8_t{7});
    put(target,0x80,ptr(signal));put(target,0x90,ptr(signal));put(target,0x88,spare);
    put(signal,0,uintptr_t{0});put(signal,4,ptr(target)+0x80);put(signal,10,uint16_t{0xffff});
    put(signal,14,uint8_t{0x27});put(signal,15,uint8_t{0});
    write<uintptr_t>(spare+4,ptr(target)+0x88);
    write<uintptr_t>(spare,spare+32);write<uintptr_t>(spare+36,spare);
    queueStarts=queueDispatches=0;
}
void queueIdleAfterOriginalFinish() {
    queuePriorRunning();
    reinterpret_cast<void(__cdecl*)(int,int,int)>(queueCode.at(0x869170))(1,-1,0);
    check(!read<uintptr_t>(ptr(target)+0x90) && read<uint8_t>(ptr(target)+0x32)==7,"original RET leaves idle worker's previous priority intact");
    queueStarts=queueDispatches=0;
}
void nativeQueuedKilikaFixture() {
    prepareQueueCode();const auto& t=trialUtilities[1];const auto& c=puzzleContracts[1];const auto& interaction=c.interactions.front();
    loadSolver(t,c,interaction);originalRequestCode=queueCode.at(0x86e920);queueIdleAfterOriginalFinish();
    auto script=utilityScript();utilityRequest(script,t.treasure.worker,t.treasure.entry);
    reinterpret_cast<void(__cdecl*)(int,int)>(queueCode.at(0x865390))(1,-1);
    check(requests==1 && !queueStarts && !read<uintptr_t>(ptr(target)+0x90),"original enqueue plus ordinary native update reproduces accepted-but-unstarted equal-priority request");
    loadSolver(t,c,interaction);queueIdleAfterOriginalFinish();puzzleCommand(L"on");natural(interaction);complete(3);pumpPuzzleSolver();
    check(requests==1 && queueDispatches==1 && queueStarts==1 && initPc==t.treasure.pc && utilityBusy(),
        "production solver request explicitly dispatches the original idle queue and enters full Kilika insertion exactly once");
    const auto owned=read<uintptr_t>(ptr(target)+0x90);check(owned && owned!=ptr(signal),"native queue uses its real current free slot rather than the old completed signal");
    natural(interaction);complete(3);
    check(queueStarts==1 && read<uintptr_t>(ptr(target)+0x90)==owned && utilityBusy(),"repeated entry interaction cannot suppress or restart the active original insertion");
    loadSolver(t,c,interaction);queuePriorRunning();puzzleCommand(L"on");natural(interaction);complete(3);pumpPuzzleSolver();
    check(requests==1 && queueDispatches==1 && !queueStarts && read<uintptr_t>(ptr(target)+0x90)==ptr(signal),
        "a running original request is never forced out to start the utility");
    Event ended;ended.args[0]=1;fire(0x469170,ended,true,false);
    reinterpret_cast<void(__cdecl*)(int,int,int)>(queueCode.at(0x869170))(1,-1,0);
    fire(0x469170,ended,false,true);
    check(queueStarts==1 && initPc==t.treasure.pc && utilityBusy(),"normal native RET starts the queued owned insertion without suppressing the predecessor");
    // Original W16:1 at387E executes Common00A7(35). W35:1 is the
    // permanently waiting halt atC8D1, not an idle +90==0 worker. This was
    // absent from the previous replay and matches the user's entry-room case.
    loadSolver(t,c,interaction);queuePriorRunning();put(target,0x32,uint8_t{0});put(signal,8,uint16_t{1});put(signal,14,uint8_t{0x20});
    reinterpret_cast<void(__cdecl*)(uintptr_t,uintptr_t,uintptr_t)>(queueCode.at(0x8576c0))(0,0,0);
    check(read<uint16_t>(ptr(target)+0x34)==12,"original room setup disables only bit2 on the destruction recess worker");
    auto disabled=utilityScript();utilityRequest(disabled,t.treasure.worker,t.treasure.entry);
    reinterpret_cast<void(__cdecl*)(int,int)>(queueCode.at(0x865390))(1,-1);
    check(!queueStarts && read<uintptr_t>(ptr(target)+0x90)==ptr(signal),"original disabled-worker gate reproduces accepted-but-unstarted request despite active priority0 halt");
    loadSolver(t,c,interaction);queuePriorRunning();put(target,0x32,uint8_t{0});put(signal,8,uint16_t{1});put(signal,14,uint8_t{0x20});
    reinterpret_cast<void(__cdecl*)(uintptr_t,uintptr_t,uintptr_t)>(queueCode.at(0x8576c0))(0,0,0);
    const auto disabledFlags=read<uint16_t>(ptr(target)+0x34);
    reinterpret_cast<int(__cdecl*)(uintptr_t,uintptr_t,uintptr_t)>(queueCode.at(0x8577f0))(0,0,0);
    const auto activatedFlags=read<uint16_t>(ptr(target)+0x34);put(target,0x34,disabledFlags);
    puzzleCommand(L"on");natural(interaction);complete(3);pumpPuzzleSolver();
    check(queueDispatches==1 && queueStarts==1 && initPc==51412 && utilityBusy(),"production solver wakes the originally disabled recess and natively preempts its halt into full insertion");
    check(read<uint16_t>(ptr(target)+0x34)==activatedFlags,"scoped activation exactly matches original Common00A8 and preserves every other worker flag");
    natural(interaction);complete(3);check(queueStarts==1 && utilityBusy(),"repeated entry interaction still preserves activated owned insertion");
    setValue(t.ready.low,t.ready.high,t.ready.value,t.ready.element);complete(1);originalRequestCode=0;pumpPuzzleSolver();
    check(initPc==t.collect.pc,"activated insertion completion advances to the original treasure chest");
    setValue(0x01000a70,1,t.mask);complete(2);pumpPuzzleSolver();
    check(initPc==puzzleCompletions[1].later.pc,"activated insertion and collection advance to the original trial completion");
    for(unsigned i=0;i<_countof(trialUtilities);++i)check(trialUtilities[i].treasure.activateWorker==(i==1),"only Kilika's verified destruction insertion opts into worker activation");
    loadSolver(t,c,interaction);queuePriorRunning();
    reinterpret_cast<void(__cdecl*)(uintptr_t,uintptr_t,uintptr_t)>(queueCode.at(0x8576c0))(0,0,0);
    put(target,0x88,uintptr_t{0});originalRequestCode=queueCode.at(0x86e920);
    utilityRequest(utilityScript(),t.treasure.worker,t.treasure.entry,{},{},true,true);
    check(requests==1 && !queueStarts && read<uint16_t>(ptr(target)+0x34)==12,"refused native request restores its scoped activation bit without changing other flags");
    loadSolver(t,c,interaction);queuePriorRunning();
    reinterpret_cast<void(__cdecl*)(uintptr_t,uintptr_t,uintptr_t)>(queueCode.at(0x8576c0))(0,0,0);
    delayedActions=true;runUtilityScript(utilityScript(),t.treasure);
    check(pendingActions.size()==1 && read<uint16_t>(ptr(target)+0x34)==12,"reserving the delayed insertion never wakes the native worker");
    auto cancelled=std::move(pendingActions.back());pendingActions.clear();invalidateUtilityContext();
    check(!cancelled.prepare(cancelled) && read<uint16_t>(ptr(target)+0x34)==12,"stale context cancels activation before any native flag mutation");
    cancelled.complete(cancelled,false);delayedActions=false;
    originalRequestCode=0;resetPuzzleSolver();
    puts("PASS: original ATEL enqueue/list/scheduler/init/RET replay reproduces idle equal-priority stall; production Kilika request starts exactly once via native idle dispatch, real signal reuse and queued player guard. Interpreter animation/UI remains simulated.");
    puts("PASS: original Kilika deactivate/activate and queued scheduler reproduce the disabled active-halt stall; production scoped activation matches original flags, starts full insertion, preserves other scripts and reaches chest/completion. Native animation/dialogue effects remain simulated.");
}
}
