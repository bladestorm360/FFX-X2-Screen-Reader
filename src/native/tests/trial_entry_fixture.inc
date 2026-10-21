namespace {
void loadSalvageShip() {
    const auto& mac=trialUtilities[3];load(mac,mac.treasure);resetPuzzleSolver();
    std::ifstream stream("cdsp0000_atel.bin",std::ios::binary|std::ios::ate);
    check(stream.good() && stream.tellg()==78272,"original Salvage Ship ATEL extent");
    const auto bytes=static_cast<unsigned>(stream.tellg());stream.seekg(0);
    stream.read(reinterpret_cast<char*>(chunk.data()),bytes);check(stream.good(),"complete archived Salvage Ship ATEL");
    const auto header=read<unsigned>(ptr(chunk)+0x38);
    check(header==0xc8 && read<uint16_t>(ptr(chunk)+header+2)==35,"original Salvage Ship worker0 header and declaration count");
    declarations.clear();const auto table=read<unsigned>(ptr(chunk)+header+0x14);
    for(unsigned i=0;i<35;++i) declarations.push_back({read<unsigned>(ptr(chunk)+table+i*8),read<unsigned>(ptr(chunk)+table+i*8+4)});
    check(std::find(declarations.begin(),declarations.end(),std::make_pair(0x280u,1u))==declarations.end(),
        "source regression must not invent the missing initialization selector declaration");
    put(root,0,ptr(chunk)+header);put(root,4,ptr(chunk));put(controller,0xc,static_cast<uint16_t>(1));
    utility_fixture_resources::prepareUtilityResources(ptr(chunk),chunk.data(),bytes);
    check(utilityScript().name=="cdsp0000","test warp runs from actual Salvage Ship declarations, not Macalania declarations");
}
void trialEntryFixtures() {
    for(int progress:{0,95,1529,1530}) {
        loadSalvageShip();setValue(0x20000a00,1,progress);delayedActions=true;
        const auto destination=prepareUtilityWarp(L"macalania-test");
        check(destination.macalaniaTest && destination.room==239 && destination.spawn==1 && !destination.force,
            "explicit Macalania test warp chooses the native return entrance");
        performUtilityWarp(destination);
        check(!writes && !warps && pendingActions.size()==1 && utilityValue(utilityScript(),0x20000a00,1)==progress,
            "test warp parsing and queuing never change game state");
        auto action=std::move(pendingActions.back());pendingActions.clear();
        check(action.prepare(action) && !writes && !warps,"test warp preflight stays read-only for story progress");
        action.invoke(action);
        check(writes==1 && warps==1 && nextRoom==239 && nextSpawn==1 && storyAtWarp==1550,
            "test warp changes exactly the story declaration immediately before the native transition");
        delayedActions=false;
    }
    for(int progress:{1531,1542,1550,3000}) {
        loadSalvageShip();setValue(0x20000a00,1,progress);const auto previousReplies=replies.size();
        warpCommand(L"macalania-test");
        check(replies.size()==previousReplies+1 && replies.back().find(L"Load an early save")!=std::wstring::npos &&
            !writes && !warps && !(read<uint8_t>(ptr(controller))&0x80) && utilityValue(utilityScript(),0x20000a00,1)==progress,
            "test warp refuses later progress before flags or story writes");
        warpCommand(L"macalania");check(!writes && storyAtWarp==progress && nextSpawn==0,
            "ordinary Macalania warp continues preserving all story progress");
    }
    for(auto text:{L"macalania-test 0",L"macalania-test 1",L"force macalania-test"})
        check(!prepareUtilityWarp(text).refusal.empty(),"test warp rejects force and entrance overrides");
    loadSalvageShip();globals[0x280]=4;warpCommand(L"macalania-test");
    check(replies.back().find(L"normal early-save initialization")!=std::wstring::npos && !writes && !warps,
        "test warp reads the undeclared global selector and reports its specific refusal before native writes");
    for(uintptr_t owner:{uintptr_t{0},ptr(globals)+1,UINTPTR_MAX-1}) {
        loadSalvageShip();put(controller,0x2c,owner);warpCommand(L"macalania-test");
        check(replies.back().find(L"story memory could not be verified")!=std::wstring::npos && !writes && !warps && !(read<uint8_t>(ptr(controller))&0x80),
            "unqualified global owners are refused without flags, story changes or an unsafe selector read");
    }
    loadSalvageShip();const auto table=read<unsigned>(read<uintptr_t>(ptr(root))+0x14);
    put(chunk,table,0x20000a01u);utility_fixture_resources::prepareUtilityResources(ptr(chunk),chunk.data(),78272);
    warpCommand(L"macalania-test");
    check(replies.back().find(L"story declaration is not ready")!=std::wstring::npos && !writes && !warps,
        "missing source story declaration produces a specific deferred refusal instead of a swallowed preparation exception");
    loadSalvageShip();setValue(0x20000a00,1,95);warpCommand(L"macalania-test");
    check(writes==1 && warps==1 && storyAtWarp==1550 && nextRoom==239 && nextSpawn==1,
        "real executor path successfully transitions from original Salvage Ship with no selector declaration");
    loadSalvageShip();delayedActions=true;warpCommand(L"macalania-test");
    auto cancelled=std::move(pendingActions.back());pendingActions.clear();++epoch;updateUtilityContext();
    check(!cancelled.prepare(cancelled) && !writes && !warps,"stale Macalania testing warp cancels before any story write");
    cancelled.complete(cancelled,false);delayedActions=false;
    loadSalvageShip();battle=true;warpCommand(L"macalania-test");battle=false;
    check(!writes && !warps,"Macalania testing setup is unavailable in battle");
    dialogue=1;warpCommand(L"macalania-test");dialogue=0;check(!writes && !warps,"Macalania testing setup waits for native dialogue closure");

    for(int progress:{0,95,1529,1530}) {
        loadSalvageShip();setValue(0x20000a00,1,progress);
        check(std::find(declarations.begin(),declarations.end(),std::make_pair(0x010000d8u,3u))==declarations.end(),
            "Gagazet regression uses the original source without a cave-state declaration");
        globals[0xd7]=0x5a;globals[0xd8]=3;globals[0xd9]=0xc7;globals[0xda]=1;globals[0xdb]=0xa5;
        const auto originalGlobals=globals;delayedActions=true;
        const auto destination=prepareUtilityWarp(L"gagazet-test");
        check(destination.gagazetTest && !destination.macalaniaTest && !destination.force && destination.room==272 && destination.spawn==0,
            "Gagazet setup enters the dry cave before its native swimming-party event");
        check(performUtilityWarp(destination).find(L"Prepare swimming party")!=std::wstring::npos,
            "queued setup explains the required native party event");
        check(globals==originalGlobals && !writes && !warps && pendingActions.size()==1,
            "queuing Gagazet setup preserves all source globals");
        auto action=std::move(pendingActions.back());pendingActions.clear();
        check(action.prepare(action) && globals==originalGlobals && !writes && !warps,
            "Gagazet preflight does not reset puzzle state or alter story");
        action.invoke(action);delayedActions=false;
        auto expectedGlobals=originalGlobals;
        expectedGlobals[0xd8]=expectedGlobals[0xd9]=expectedGlobals[0xda]=0;
        const uint16_t testProgress=2625;memcpy(expectedGlobals.data()+0xa00,&testProgress,sizeof(testProgress));
        check(globals==expectedGlobals && writes==1 && warps==1 && nextRoom==272 && nextSpawn==0 && storyAtWarp==2625 && !requests,
            "Gagazet setup changes only three cave bytes and story at native transition, without solving or requesting a puzzle");
    }
    for(auto text:{L"gagazet-test 0",L"gagazet-test 1",L"force gagazet-test",L"gagazet-test extra"})
        check(!prepareUtilityWarp(text).refusal.empty(),"Gagazet setup rejects force and entrance overrides");
    for(int progress:{1531,2625,2680,3000}) {
        loadSalvageShip();setValue(0x20000a00,1,progress);globals[0xd8]=3;globals[0xd9]=0xc7;globals[0xda]=1;
        const auto beforeGlobals=globals;warpCommand(L"gagazet-test");
        check(replies.back().find(L"Load an early save")!=std::wstring::npos && !writes && !warps && globals==beforeGlobals,
            "Gagazet testing cannot reset existing later progress");
        warpCommand(L"272 0");check(!writes && warps==1 && storyAtWarp==progress && globals==beforeGlobals,
            "ordinary Gagazet warp preserves cave flags and story");
    }
    for(uintptr_t owner:{uintptr_t{0},ptr(globals)+1,UINTPTR_MAX-1}) {
        loadSalvageShip();put(controller,0x2c,owner);globals[0xd8]=3;const auto beforeGlobals=globals;
        warpCommand(L"gagazet-test");
        check(!writes && !warps && globals==beforeGlobals && !(read<uint8_t>(ptr(controller))&0x80),
            "Gagazet setup rejects an unqualified globals owner before any cave write");
    }
    loadSalvageShip();globals[0x280]=4;globals[0xd8]=3;const auto selectorGlobals=globals;warpCommand(L"gagazet-test");
    check(!writes && !warps && globals==selectorGlobals,"Gagazet setup preserves nonstandard initialization state");
    loadSalvageShip();globals[0xd8]=3;const auto staleGlobals=globals;delayedActions=true;warpCommand(L"gagazet-test");
    auto staleGagazet=std::move(pendingActions.back());pendingActions.clear();++epoch;updateUtilityContext();
    check(!staleGagazet.prepare(staleGagazet) && !writes && !warps && globals==staleGlobals,
        "stale Gagazet setup cancels before cave and story writes");
    staleGagazet.complete(staleGagazet,false);delayedActions=false;
    loadSalvageShip();globals[0xd8]=3;const auto blockedGlobals=globals;battle=true;warpCommand(L"gagazet-test");battle=false;
    dialogue=1;warpCommand(L"gagazet-test");dialogue=0;
    check(!writes && !warps && globals==blockedGlobals,"Gagazet setup waits for field control and dialogue closure");
    auto conflicting=prepareUtilityWarp(L"gagazet-test");conflicting.macalaniaTest=true;
    check(performUtilityWarp(conflicting).find(L"Invalid")!=std::wstring::npos && !writes && !warps,
        "test destinations cannot be combined");
    puts("PASS: Gagazet explicit early-save setup from original Salvage Ship changes only story and three cave bytes at native transition; dry-cave entry retains native swimming-party and puzzle scripts; ordinary, stale, later, invalid-owner, battle and dialogue contexts preserve progress.");

    const auto& bevelle=trialUtilities[4];const auto& contract=puzzleContracts[4];
    const auto rail=std::find_if(contract.interactions.begin(),contract.interactions.end(),[](const auto& a){return a.worker==39 && a.entry==6;});
    check(rail!=contract.interactions.end() && rail->entryPc==116381 && rail->returnPc==116484,"Bevelle contract uses the actual rail entry edge rather than an empty talk entry");
    loadSolver(bevelle,contract,*rail);put(chunk,0x900,uint8_t{2});utility_fixture_resources::prepareUtilityResources(ptr(chunk),chunk.data(),chunk.size());
    check(puzzleCommand(L"on").find(L"push the rail pedestal")!=std::wstring::npos,"Bevelle solver names the reachable starting action");
    natural(*rail);check(read<uintptr_t>(ptr(physical)+0x12c+0x4c+0x18)==ptr(chunk)+0x1000+rail->returnPc && !requests && !writes,
        "Bevelle rail crossing intercepts before its original player lock and ride requests");
    complete(3);pumpPuzzleSolver();check(requests==1 && initPc==bevelle.treasure.pc,"Bevelle rail trigger starts original solver reveal after normal RET");
    setValue(bevelle.ready.low,bevelle.ready.high,bevelle.ready.value,bevelle.ready.element);complete(1);pumpPuzzleSolver();
    check(requests==2 && initPc==bevelle.collect.pc,"Bevelle rail trigger advances to original chest collection");
    setValue(0x01000a70,1,bevelle.mask);complete(1);pumpPuzzleSolver();
    finishTrialBonus(bevelle);
    check(initPc==puzzleCompletions[4].later.pc && !puzzleArmed() && !warps,"Bevelle rail activation keeps the complete-trial objective");complete(4);
    loadSolver(bevelle,contract,*rail);natural(*rail);
    check(!read<uintptr_t>(ptr(physical)+0x12c+0x4c+0x18) && !writes && !requests,"solver Off preserves ordinary rail boarding");resetPuzzleSolver();
    puts("PASS: Macalania explicit early-save testing warp from original Salvage Ship ATEL (35 declarations, no selector) owns its single story write at native transition, reports specific refusals, rejects invalid owners/later/forced/stale contexts and preserves ordinary warps; Bevelle original type2 W39:6 entrance crossing reaches reveal/chest/completion and preserves Off.");
}
}
