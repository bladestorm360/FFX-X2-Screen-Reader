namespace {
std::array<uint8_t,32> djoseSpareSignal{};
void djoseQueuedPlayer(const PuzzleInteraction& action,bool intercepted) {
    // Original enqueue/dispatch/init with Djose's real type3 collision wrapper.
    // Environmental worker storage is isolated; no interpreter UI is opened.
    const auto w=ptr(physical),sig=ptr(physicalSignal);
    const auto spare=reinterpret_cast<uintptr_t>(djoseSpareSignal.data());
    std::fill(physicalSignal.begin(),physicalSignal.end(),uint8_t{0});djoseSpareSignal.fill(0);
    put(physical,0x34,uint16_t{14});put(physical,0x32,uint8_t{0});
    put(physical,0x80,uintptr_t{0});put(physical,0x90,uintptr_t{0});put(physical,0x88,sig);
    put(physicalSignal,0,spare);put(physicalSignal,4,w+0x88);write<uintptr_t>(spare+4,sig);
    const auto starts=queueStarts,utilityRequests=requests,mutations=writes;const auto spoken=replies.size();
    check(reinterpret_cast<int(__cdecl*)(int,int,int,int,int)>(queueCode.at(0x86e920))(0,3,0,1,action.entry)==1,
        "original Djose player request enters the native queue");
    reinterpret_cast<void(__cdecl*)(int,int)>(queueCode.at(0x865390))(3,0);
    const auto pc=read<uintptr_t>(w+0x12c+0x4c+0x18);
    check(queueStarts==starts+1 && pc==ptr(chunk)+0x1000+(intercepted?action.returnPc:action.entryPc),
        "Djose native queued wrapper is intercepted only while the solver owns the sequence");
    if(intercepted) check(read<uint8_t>(pc)==0x3c && requests==utilityRequests && writes==mutations && replies.size()==spoken,
        "queued wrapper returns before its original sphere helper request without restarting the solver");
    Event returned;returned.args[0]=3;fire(0x469170,returned,true,false);
    reinterpret_cast<void(__cdecl*)(int,int,int)>(queueCode.at(0x869170))(3,-1,0);
    fire(0x469170,returned,false,true);
    check(!read<uintptr_t>(w+0x90) && !read<uintptr_t>(w+0x80),"original RET releases the suppressed Djose request normally");
}
void djoseSolverFixtures() {
    const auto& t=trialUtilities[2];const auto& c=puzzleContracts[2];const auto& endingAction=puzzleCompletions[2].first;
    for(const auto& action:c.interactions) {
        loadSolver(t,c,action);puzzleCommand(L"on");natural(action);complete(3);pumpPuzzleSolver();
        if(initPc!=t.treasure.pc || !utilityBusy()) std::fwprintf(stderr,L"Djose fixture worker%u pc%u requests%u last=%ls\n",action.worker,initPc,requests,replies.empty()?L"":replies.back().c_str());
        check(initPc==t.treasure.pc && utilityBusy(),"Djose activation starts the original destruction reveal");
        const auto ownedPC=ptr(target)+0x12c+7*0x4c+0x18;
        auto repeated=[&] {
            const auto accepted=requests,mutations=writes;const auto spoken=replies.size();
            put(physical,0x12c+0x4c+0x18,uintptr_t{0});dialogue=1;natural(action);dialogue=0;
            check(read<uintptr_t>(ptr(physical)+0x12c+0x4c+0x18)==ptr(chunk)+0x1000+action.returnPc,
                "Djose queued player interaction cannot run an ordinary take/place prompt during solving");
            complete(3);
            check(requests==accepted && writes==mutations && replies.size()==spoken && utilityBusy(),
                "Djose repeated input preserves the owned request and original objective");
        };
        repeated();
        check(read<uintptr_t>(ownedPC)==ptr(chunk)+0x1000+t.treasure.pc,"Djose queued interaction retains the owned reveal PC");
        if(action.worker>=48 && action.worker<=55) djoseQueuedPlayer(action,true);
        setValue(t.ready.low,t.ready.high,t.ready.value,t.ready.element);complete(1);pumpPuzzleSolver();
        check(initPc==t.collect.pc,"Djose reveal advances to its original chest continuation");
        repeated();if(action.worker>=48 && action.worker<=55) djoseQueuedPlayer(action,true);
        setValue(0x01000a70,1,t.mask);complete(2);pumpPuzzleSolver();
        if(action.destruction) {
            check(puzzleArmed() && !utilityBusy() && !warps,"Djose destruction-only objective remains in the chamber");
        } else {
            check(!puzzleArmed() && initPc==endingAction.pc && utilityBusy(),"Djose ordinary objective continues to the native completion exit");
            repeated();if(action.worker>=48 && action.worker<=55) djoseQueuedPlayer(action,true);
            complete(4);
        }
        puzzleCommand(L"off");put(physical,0x12c+0x4c+0x18,uintptr_t{0});natural(action);
        check(read<uintptr_t>(ptr(physical)+0x12c+0x4c+0x18)==0,"Djose opt-out restores ordinary native interactions");
        if(action.worker>=48 && action.worker<=55) djoseQueuedPlayer(action,false);
    }
    // Internal scripted calls, other entries and stale contexts retain their
    // original ownership. A protected sequence cannot consume them as input.
    const auto& action=c.interactions.front();loadSolver(t,c,action);puzzleCommand(L"on");natural(action);complete(3);pumpPuzzleSolver();
    const auto pc=ptr(physical)+0x12c+0x4c+0x18;
    put(physical,0x12c+0x4c+0x18,uintptr_t{0});natural(action,43);
    check(!read<uintptr_t>(pc),"Djose does not intercept an internal helper's request");
    auto other=action;other.entry=3;natural(other);check(!read<uintptr_t>(pc),"Djose does not intercept another native entry");
    ++epoch;natural(action);check(!read<uintptr_t>(pc),"Djose pending guard retires with the map generation");resetPuzzleSolver();
    puts("PASS: all19 Djose interactions retain reveal/chest/completion ownership, objective and opt-out; original native enqueue/dispatch/init/RET replay covers all8 sphere/recess wrappers. Interpreter animation/UI remains simulated.");
}
}
