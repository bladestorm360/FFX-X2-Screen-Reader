namespace {
void trialPendingInteraction(const PuzzleInteraction& action,bool busy) {
    const unsigned index=read<uint16_t>(ptr(target)+0x2e)==action.worker?1:3;
    const auto w=worker(index),head=read<uintptr_t>(w+0x80),current=read<uintptr_t>(w+0x90);
    const auto accepted=requests,mutations=writes;const auto spoken=replies.size();
    write<uintptr_t>(w+0x12c+0x4c+0x18,0);dialogue=1;natural(action);dialogue=0;
    check(read<uintptr_t>(w+0x12c+0x4c+0x18)==ptr(chunk)+0x1000+action.returnPc,
        "every trial retires only the queued player entry at its original RET during an active solver sequence");
    complete(index);write<uintptr_t>(w+0x80,head);write<uintptr_t>(w+0x90,current);
    check(requests==accepted && writes==mutations && replies.size()==spoken && utilityBusy()==busy,
        "queued input preserves the trial objective, exact owned script, writes and speech");
}
void trialPendingForeignRequest(const PuzzleInteraction& action) {
    const unsigned index=read<uint16_t>(ptr(target)+0x2e)==action.worker?1:3;
    const auto w=worker(index),head=read<uintptr_t>(w+0x80),current=read<uintptr_t>(w+0x90);
    const auto pc=w+0x12c+0x4c+0x18;write<uintptr_t>(pc,0);natural(action,88);
    check(!read<uintptr_t>(pc),"active solver preserves internal non-player requests in every trial");
    auto other=action;other.entry=3;natural(other);check(!read<uintptr_t>(pc),"active solver preserves unmatched native entries in every trial");
    write<uintptr_t>(w+0x80,head);write<uintptr_t>(w+0x90,current);
}
void allTrialInterceptFixtures() {
    unsigned interactions=0,gateCases=0;
    for(unsigned n=0;n<_countof(trialUtilities);++n) for(const auto& action:puzzleContracts[n].interactions) {
        const auto& t=trialUtilities[n];const auto& c=puzzleContracts[n];const auto& end=puzzleCompletions[n];
        loadSolver(t,c,action);puzzleCommand(L"on");natural(action);
        const unsigned activation=read<uint16_t>(ptr(target)+0x2e)==action.worker?1:3;complete(activation);
        if(n==3) {
            trialPendingInteraction(action,false);
        }
        pumpPuzzleSolver();check(utilityBusy() && initPc==t.treasure.pc,"each trial retains its own exact reveal initialization");
        trialPendingForeignRequest(action);trialPendingInteraction(action,true);
        setValue(c.carried.low,c.carried.high,c.destruction,c.carried.element);
        setValue(t.ready.low,t.ready.high,t.ready.value,t.ready.element);complete(1);pumpPuzzleSolver();
        check(utilityBusy() && initPc==t.collect.pc && initY==t.collect.regY,"each trial retains its own exact collection initialization");
        trialPendingInteraction(action,true);
        setValue(0x01000a70,1,t.mask);complete(t.collect.worker==t.treasure.worker?1:2);pumpPuzzleSolver();
        if(n==4) {trialPendingInteraction(action,true);finishTrialBonus(t);}
        const bool stays=(n==5 || action.destruction);
        if(stays) check(puzzleArmed() && !utilityBusy() && !warps,"treasure-only objective remains unchanged by queued input");
        else {
            check(!puzzleArmed() && utilityBusy() && initPc==(n==0?end.first.pc:end.later.pc) && !warps,"ordinary solve retains its original completion after disarming new activations");
            trialPendingInteraction(action,true);complete(end.first.worker==end.later.worker?4:5);
        }
        puzzleCommand(L"off");write<uintptr_t>(worker(activation)+0x12c+0x4c+0x18,0);natural(action);
        check(!read<uintptr_t>(worker(activation)+0x12c+0x4c+0x18),"every trial restores native player interactions after opt-out");
        ++interactions;
        if(n==5) {
            loadSolver(t,c,action);setValue(0x20000a00,1,2768);puzzleCommand(L"on");natural(action);complete(activation);pumpPuzzleSolver();
            check(requests==1 && utilityBusy() && initPc==end.first.pc && !puzzleArmed(),"Zanarkand first visit runs only the original gate action");
            trialPendingInteraction(action,true);
            setValue(0x20000a00,1,2775);trialPendingInteraction(action,true);
            complete(4);pumpPuzzleSolver();
            check(requests==1 && !utilityBusy() && !puzzleArmed() && !warps && !utilityValue(utilityScript(),0x01000a70,1),
                "Zanarkand native gate write and queued input preserve treasure lock and the encounter route");++gateCases;
        }
    }
    resetPuzzleSolver();std::printf("PASS: all6 trial intercept matrix: %u interactions through reveal/chest/completion or goal selection, shared-worker owned initialization, internal-source exclusions, opt-out; %u Zanarkand first-visit gate cases. Native UI/animation remains simulated.\n",interactions,gateCases);
}
}
