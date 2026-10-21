namespace {
void finishTrialBonus(const TrialUtility& t) {
    if(std::string(t.script)!="bvyt1100") return;
    check(utilityBusy() && initPc==79849 && !utilityValue(utilityScript(),0x0c00002a,1),
        "Bevelle runs the original branch chest after destruction treasure, before marking it or leaving");
    const auto before=requests;complete(5);
    check(utilityValue(utilityScript(),0x0c00002a,1)==1 && requests==before,
        "Bevelle records the original caller marker only after the owned chest RET");
    pumpPuzzleSolver();
}
void bevelleTreasureFixtures() {
    const auto& t=trialUtilities[4];const auto& c=puzzleContracts[4];const auto& i=c.interactions.front();
    const auto& end=puzzleCompletions[4];
    const auto begin=[&] {
        loadSolver(t,c,i);setValue(0x01000a70,1,t.mask);puzzleCommand(L"on");
        natural(i);complete(read<uint16_t>(ptr(target)+0x2e)==i.worker?1:3);
        pumpPuzzleSolver();pumpPuzzleSolver();
        check(initPc==end.bonus.pc && requests==1 && utilityBusy() && puzzleArmed(),
            "already collected destruction treasure still leads to the missing branch chest");
    };
    begin();const auto pendingWrites=writes;pumpPuzzleSolver();pumpPuzzleSolver();
    check(requests==1 && writes==pendingWrites && !warps && !utilityValue(utilityScript(),0x0c00002a,1),
        "pending branch chest dialogue cannot duplicate requests, mark collection early or advance the story");
    puzzleCommand(L"off");complete(5);pumpPuzzleSolver();
    check(utilityValue(utilityScript(),0x0c00002a,1)==1 && requests==1 && !warps && !puzzleArmed(),
        "Off lets the current chest finish and retain its collection marker without continuing to the exit");
    puzzleCommand(L"on");natural(i);complete(3);pumpPuzzleSolver();pumpPuzzleSolver();
    check(requests==2 && initPc==end.later.pc && !puzzleArmed(),
        "re-enabling after Off skips the collected chest and starts only the native exit");complete(4);

    begin();puzzleCompleted(ptr(bonusWorkerBytes),ptr(bonusSignalBytes),false,true);pumpPuzzleSolver();
    check(!puzzleArmed() && !utilityValue(utilityScript(),0x0c00002a,1) && requests==1 && !warps,
        "interrupted branch chest never records collection or proceeds to story completion");
    loadSolver(t,c,i);setValue(0x01000a70,1,t.mask);setValue(0x0c00002a,1,1);
    puzzleCommand(L"on");natural(i);complete(3);pumpPuzzleSolver();pumpPuzzleSolver();
    check(requests==1 && initPc==end.later.pc,"both previously opened chests are skipped before the native exit");complete(4);

    begin();const auto beforeEpoch=writes;++epoch;updateUtilityContext();
    puzzleCompleted(ptr(bonusWorkerBytes),ptr(bonusSignalBytes),true,true);pumpPuzzleSolver();
    check(writes==beforeEpoch && !utilityValue(utilityScript(),0x0c00002a,1) && requests==1 && !warps,
        "generation change retires the pending chest without a stale marker or exit");resetPuzzleSolver();
    begin();const auto beforeAllocation=writes;utility_fixture_resources::prepareUtilityResources(ptr(chunk),chunk.data(),chunk.size());
    complete(5);pumpPuzzleSolver();
    check(writes==beforeAllocation && !utilityValue(utilityScript(),0x0c00002a,1) && requests==1 && !warps && !puzzleArmed(),
        "same-address replacement allocation cannot inherit an earlier chest completion marker");resetPuzzleSolver();
    begin();puzzleCompleted(ptr(target),ptr(signal),true,true);puzzleCompleted(ptr(bonusWorkerBytes),ptr(physicalSignal),true,true);
    check(!utilityValue(utilityScript(),0x0c00002a,1) && requests==1 && utilityBusy(),
        "another worker or signal cannot acknowledge the pending branch chest");finishTrialBonus(t);complete(4);
    puts("PASS: Bevelle destruction treasure then original optional chest then exit; collected-state skips, owned RET marker, queued dialogue, Off/re-enable, interruption, generation/allocation and worker/signal guards; native item grant/UI effects remain simulated.");
}
}
