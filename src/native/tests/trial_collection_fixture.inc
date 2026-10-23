// Exercise production Auto with the real six temple contracts. Native reward
// UI/animation are modeled; missing-bit cases deliberately inject a lost marker
// after a completed chest, rather than claim to reproduce the old save's cause.
namespace {
unsigned beginTrialCollection(unsigned n,unsigned flags,bool destruction=false) {
    const auto& t=trialUtilities[n];const auto& c=puzzleContracts[n];
    auto it=std::find_if(c.interactions.begin(),c.interactions.end(),[&](const auto& i){return i.destruction==destruction;});
    if(it==c.interactions.end() && destruction) it=c.interactions.begin();
    check(it!=c.interactions.end(),"requested native trial interaction exists");
    loadSolver(t,c,*it);setValue(0x01000a70,1,flags);
    setValue(t.ready.low,t.ready.high,t.ready.value,t.ready.element);
    // Some trials identify this goal through the carried sphere rather than
    // a dedicated destruction-labelled interaction. Exercise that real route.
    if(destruction) setValue(c.carried.low,c.carried.high,c.destruction,c.carried.element);
    puzzleCommand(L"on");natural(*it);complete(read<uint16_t>(ptr(target)+0x2e)==it->worker?1:3);
    pumpPuzzleSolver();pumpPuzzleSolver();
    check(utilityBusy() && initPc==t.collect.pc && requests==1,"missing temple flag starts its original chest even when other temples are complete");
    return t.collect.worker==t.treasure.worker?1:2;
}
void trialCollectionFixtures() {
    // The retained Baaj save has 0x3E: Besaid missing and Zanarkand present.
    // Test the reported Besaid case first, then a Zanarkand-missing counterpart.
    for(unsigned n:{0u,5u}) {
        const auto& t=trialUtilities[n];const unsigned before=0x3f&~t.mask;
        const auto index=beginTrialCollection(n,before);const auto mutations=writes;
        complete(index);
        check(utilityValue(utilityScript(),0x01000a70,1)==0x3f && writes==mutations+1,
            "Besaid then Zanarkand final chest RET commits the missing Baaj flag while preserving the other five");
        pumpPuzzleSolver();resetPuzzleSolver();
    }
    unsigned cases=0;
    for(unsigned n=0;n<_countof(trialUtilities);++n) for(unsigned flags=0;flags<64;++flags) {
        const auto& t=trialUtilities[n];const unsigned before=flags|0xc0;
        if(before&t.mask) {
            const auto& c=puzzleContracts[n];const auto& i=c.interactions.front();
            loadSolver(t,c,i);setValue(0x01000a70,1,before);puzzleCommand(L"on");natural(i);
            complete(read<uint16_t>(ptr(target)+0x2e)==i.worker?1:3);pumpPuzzleSolver();
            check(!requests && utilityValue(utilityScript(),0x01000a70,1)==static_cast<int>(before),
                "already collected temple never repeats its destruction reward or changes other flags");
        } else {
            const auto index=beginTrialCollection(n,before);const auto mutations=writes;
            complete(index);
            check(utilityValue(utilityScript(),0x01000a70,1)==static_cast<int>(before|t.mask) && writes==mutations+1,
                "every missing temple commits only its own flag across all64 combinations and preserves reserved bits");
        }
        resetPuzzleSolver();++cases;
    }
    for(unsigned n=0;n<_countof(trialUtilities);++n) {
        const auto& t=trialUtilities[n];const auto before=0x3f&~t.mask;
        auto index=beginTrialCollection(n,before);auto mutations=writes;
        puzzleCompleted(worker(index),ptr(physicalSignal),true,true);
        puzzleCompleted(worker(index==1?2:1),ptr(index==1?signal:helperSignal),true,true);
        check(utilityValue(utilityScript(),0x01000a70,1)==static_cast<int>(before) && writes==mutations && utilityBusy(),
            "foreign worker/signal returns cannot commit an active temple flag");
        puzzleCommand(L"off");complete(index);pumpPuzzleSolver();
        check(utilityValue(utilityScript(),0x01000a70,1)==0x3f && !puzzleArmed() && requests==1 && !warps,
            "Off allows exactly the running chest to commit its sphere flag without completing or exiting the trial");
        index=beginTrialCollection(n,before);mutations=writes;
        puzzleCompleted(worker(index),ptr(index==1?signal:helperSignal),false,true);
        check(utilityValue(utilityScript(),0x01000a70,1)==static_cast<int>(before) && writes==mutations && !puzzleArmed(),
            "interrupted chest cannot fabricate collection");resetPuzzleSolver();
        index=beginTrialCollection(n,before);mutations=writes;
        complete(index,false);
        check(utilityValue(utilityScript(),0x01000a70,1)==static_cast<int>(before) && writes==mutations && !puzzleArmed(),
            "native return before the chest's verified final RET cannot fabricate collection");resetPuzzleSolver();
        index=beginTrialCollection(n,before);mutations=writes;
        ++epoch;updateUtilityContext();complete(index);
        check(utilityValue(utilityScript(),0x01000a70,1)==static_cast<int>(before) && writes==mutations,
            "generation changes cannot commit an earlier chest's flag");resetPuzzleSolver();
        index=beginTrialCollection(n,before);mutations=writes;
        utility_fixture_resources::prepareUtilityResources(ptr(chunk),chunk.data(),chunk.size());complete(index);
        check(utilityValue(utilityScript(),0x01000a70,1)==static_cast<int>(before) && writes==mutations && !puzzleArmed(),
            "same-address replacement allocation cannot commit an earlier chest's flag");resetPuzzleSolver();
        index=beginTrialCollection(n,before);setValue(0x01000a70,1,0x3f);mutations=writes;complete(index);
        check(writes==mutations && utilityValue(utilityScript(),0x01000a70,1)==0x3f,
            "normal original chest flag needs no second setter");resetPuzzleSolver();
        index=beginTrialCollection(n,before,true);complete(index);pumpPuzzleSolver();
        if(n==4) finishTrialBonus(t);
        check(utilityValue(utilityScript(),0x01000a70,1)==0x3f && puzzleArmed() && !utilityBusy() && !warps,
            "Destruction Sphere interactions retain the verified flag and stay in the trial");resetPuzzleSolver();
    }
    std::printf("PASS: Besaid0x3E then Zanarkand0x1F; all6 temples/all64 combinations (%u cases), reserved bits, original committed-state no-op, exact worker/signal, Off, interruption and generation/allocation guards. Missing-bit fallback is fault injection; native item UI/animation remains modeled.\n",cases);
}
}
