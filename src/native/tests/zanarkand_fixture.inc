// Native dome0700 phase boundaries, not a simulated universal trial recipe.
namespace {
void zanarkandFixtures() {
    const auto& t=trialUtilities[5];const auto& c=puzzleContracts[5];
    const auto& i=c.interactions.front();const auto& end=puzzleCompletions[5];
    check(end.first.worker==39 && end.first.entry==47 && !end.exit.worker,
        "Zanarkand completion contract cannot take the lift past its encounter route");
    for(int progress:{0,2768,2774}) {
        loadSolver(t,c,i);setValue(0x20000a00,1,progress);
        check(puzzleCommand(L"on").find(L"first-visit automatic solver on")!=std::wstring::npos,"native first-visit solver selection");
        check(trialCommand(L"treasure").find(L"locked")!=std::wstring::npos && !requests && !writes,"first visit cannot reveal treasure through developer command");
        check(trialCommand(L"collect").find(L"locked")!=std::wstring::npos && !requests && !writes,"first visit cannot collect treasure through developer command");
        natural(i);complete(3);pumpPuzzleSolver();
        check(requests==1 && initPc==end.first.pc && !warps && !puzzleArmed(),"first-visit solver invokes only normal gate, not reveal/chest/lift");
        check(utilityValue(utilityScript(),0x01000a70,1)==0 && utilityValue(utilityScript(),0x20000a00,1)==progress,
            "first-visit preparation never grants treasure or synthesizes story progress");
        setValue(0x20000a00,1,2775); // original gate's verified write, outside fixture request stub
        complete(4);pumpPuzzleSolver();
        check(requests==1 && !warps && replies.back().find(L"re-enter")!=std::wstring::npos && replies.back().find(L"Cloister of Trials")!=std::wstring::npos,"normal gate return explains the confirmed leave/re-enter route");
    }
    for(int progress:{2775,2776,2780,2785,2870,2874}) {
        loadSolver(t,c,i);setValue(0x20000a00,1,progress);
        const auto before=replies.size();puzzleCommand(L"on");natural(i);pumpPuzzleSolver();
        check(!puzzleArmed() && !requests && !writes && !warps && replies.size()==before,"gate-open/story sequence cannot arm a completion or treasure solver");
        const auto finishText=renderUtilityText(trialCommand(L"finish"));
        check((finishText.find(L"native exit")!=std::wstring::npos || finishText.find(L"Cloister of Trials")!=std::wstring::npos) && !requests && !warps && !writes,"developer finish cannot skip native Zanarkand encounters");
    }
    for(int progress:{2875,3000,3120,3121}) {
        loadSolver(t,c,i);setValue(0x20000a00,1,progress);
        check(puzzleCommand(L"on").find(L"return-visit Destruction Sphere solver on")!=std::wstring::npos,"native return phase selects treasure-only solver");
        natural(i);complete(3);pumpPuzzleSolver();
        check(initPc==t.treasure.pc && requests==2 && !warps,"return solver uses original treasure reveal and chest visibility request");
        setValue(t.ready.low,t.ready.high,t.ready.value,t.ready.element);complete(1);pumpPuzzleSolver();
        check(initPc==t.collect.pc,"return reveal schedules original collection");
        setValue(0x01000a70,1,t.mask);complete(2);pumpPuzzleSolver();
        check(requests==3 && !warps && puzzleArmed() && utilityValue(utilityScript(),0x20000a00,1)==progress,"return solver collects and stays without gate/lift/story writes");
        puzzleCommand(L"off");
    }
    loadSolver(t,c,i);setValue(0x20000a00,1,2768);resetOwnedPuzzleGuide();
    check(fixturePuzzleGuideSync(OwnedGuideAction::Step).find(L"First-visit trial. Step 1 of 11")!=std::wstring::npos,"first-visit guide contains only normal trial");
    for(unsigned n=0;n<30;++n)fixturePuzzleGuideSync(OwnedGuideAction::Next);
    check(fixturePuzzleGuideSync(OwnedGuideAction::Step).find(L"Step 11 of 11")!=std::wstring::npos,"first-visit guide cannot advance into locked return instructions");
    setValue(0x20000a00,1,2775);
    check(fixturePuzzleGuideSync(OwnedGuideAction::Step).find(L"re-enter")!=std::wstring::npos,"gate-open guide explains the confirmed leave/re-enter route");
    setValue(0x20000a00,1,2780);
    check(fixturePuzzleGuideSync(OwnedGuideAction::Next).find(L"native story and encounter sequence")!=std::wstring::npos,"story sequence has no return-puzzle guide");
    setValue(0x20000a00,1,2875);
    check(fixturePuzzleGuideSync(OwnedGuideAction::Step).find(L"Return-visit treasure. Step 1 of 4")!=std::wstring::npos,"same-map native phase change selects separate return guide");
    check(fixturePuzzleGuideSync(OwnedGuideAction::Previous).find(L"Step 1 of 4")!=std::wstring::npos,"return guide cannot move backwards into first-visit trial");
    for(unsigned n=0;n<10;++n)fixturePuzzleGuideSync(OwnedGuideAction::Next);
    check(fixturePuzzleGuideSync(OwnedGuideAction::Step).find(L"Step 4 of 4")!=std::wstring::npos,"return guide ends at treasure chest");
    resetOwnedPuzzleGuide();
    std::puts("PASS: native Zanarkand first visit/gate/story/return boundaries; separate bounded manual guides; no treasure before unlock or automatic encounter/lift shortcut.");
}
}
