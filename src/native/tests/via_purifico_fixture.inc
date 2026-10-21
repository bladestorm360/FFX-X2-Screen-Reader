namespace {
void loadVia() {
    auto t=trialUtilities[0];t.script="bvyt0900";t.length=37649;
    load(t,viaChests[0].action);resetPuzzleSolver();declarations.clear();
    for(unsigned id:{6u,8u,9u,10u,11u,13u}) declaration(id,1);
    viaLoaded=true;
    const unsigned locals[]={0,11,12,13,14,15,16,17,42,43,36,38,40,37,39,41};
    for(unsigned i=0;i<16;++i) {
        viaWorkers[i].assign(0xb58,0);viaSignals[i].assign(32,0);
        const unsigned h=0xb00+i*0x30,table=0x38000+i*64;
        put(chunk,0x38+locals[i]*4,h);put(chunk,h,static_cast<uint8_t>(i==8 || i==9 || i>=10?3:1));
        put(chunk,h+2,static_cast<uint16_t>(declarations.size()));put(chunk,h+0x14,0x200u);
        put(chunk,h+8,static_cast<uint16_t>(16));put(chunk,h+0x20,table);
        if(i>0 && i<8) put(chunk,table+8,viaChests[i-1].action.entryPc);
        put(viaWorkers[i],0,ptr(chunk)+h);put(viaWorkers[i],4,ptr(chunk));put(viaWorkers[i],0x2e,static_cast<uint16_t>(locals[i]));
        put(viaWorkers[i],0x88,ptr(viaSignals[i]));
    }
    // Last metadata header remains below the 1000 code start.
    put(controller,0xc,static_cast<uint16_t>(16));fixturePlayer={};
    utility_fixture_resources::prepareUtilityResources(ptr(chunk),chunk.data(),chunk.size());
}
void viaFixtures() {
    loadVia();check(viaPurificoActive(),"Via guide accepts the exact real archived script");
    check(viaCommand(L"step").find(L"chest 1")!=std::wstring::npos,"manual guide starts with accessible treasure");
    setValue(6,1,99);check(viaCommand(L"expected").find(L"prepare the relocation")!=std::wstring::npos,"guide uses actual four accessible chest bits");
    setValue(10,1,1);check(viaCommand(L"expected").find(L"eastern relocation")!=std::wstring::npos,"first glyph leads to native second glyph");
    setValue(11,1,1);setValue(9,1,4);
    check(viaCommand(L"expected").find(L"west")!=std::wstring::npos,"east panel selects west toward the western branch");
    setValue(9,1,2);check(viaCommand(L"step").find(L"north")!=std::wstring::npos,"west panel selects north after relocation");
    setValue(9,1,0);check(viaCommand(L"step").find(L"west")!=std::wstring::npos,"relocated northwest panel selects west into sealed chamber");
    setValue(9,1,5);check(viaCommand(L"expected").find(L"chest 3")!=std::wstring::npos,"sealed chamber guides the first of three chests");
    setValue(6,1,127);check(viaCommand(L"expected").find(L"east")!=std::wstring::npos,"all treasure bits lead to east return from sealed chamber");
    setValue(9,1,0);check(viaCommand(L"step").find(L"Exit 1")!=std::wstring::npos,"manual guide finishes at required exit");
    loadVia();setValue(6,1,0);viaCommand(L"on");check(viaCollecting() && !requests,"collection waits for owner pump and starts without forged grant bits");
    for(unsigned i=0;i<7;++i) {
        pumpPuzzleSolver();check(initPc==viaChests[i].action.pc,"each original chest entry is selected exactly");
        const auto prior=requests;pumpPuzzleSolver();check(requests==prior,"pending dialogue never duplicates collection");
        setValue(6,1,(1<<(i+1))-1);complete(i+1);
    }
    pumpPuzzleSolver();check(!viaCollecting() && requests==7 && !writes && !warps,"all seven original chests collected without state writes or exit warp");
    const auto prior=requests;viaCommand(L"on");pumpPuzzleSolver();check(requests==prior,"already-collected chests never requested again");
    loadVia();setValue(6,1,3);viaCommand(L"on");pumpPuzzleSolver();check(initPc==viaChests[2].action.pc,"partly collected dungeon resumes at first missing chest");
    viaScriptCompleted(false,true);check(!viaCollecting(),"interrupted native collection stops automatic sequence");
    loadVia();viaCommand(L"on");++epoch;pumpPuzzleSolver();check(!viaCollecting() && !requests,"map change cannot run an old collection request");
    loadVia();chunk[0x1000]^=1;utility_fixture_resources::prepareUtilityResources(ptr(chunk),chunk.data(),chunk.size());resetViaPurifico();check(!viaPurificoActive(),"newly prepared changed bytecode refuses Via services");
    loadVia();setValue(6,1,99);setValue(10,1,1);setValue(11,1,1);setValue(9,1,0);setValue(13,1,1);setValue(8,1,1);
    viaCommand(L"expected");fixturePlayer={-360,0,1100};Event e;e.args[0]=ptr(viaWorkers[0]);e.args[1]=1;e.args[3]=2;
    // Root declaration1 is actual direction low8; run the actual write boundary.
    const auto count=replies.size();fire(0x4713a0,e,true,false);setValue(8,1,2);fire(0x4713a0,e,false,true);
    check(replies.size()==count+1 && replies.back().find(L"west")!=std::wstring::npos,"wanted native direction edge gives nearby manual warp cue");
    fire(0x4713a0,e);check(replies.size()==count+1,"same native direction assignment creates no extra cue");
    setValue(8,1,1);fixturePlayer={};fire(0x4713a0,e);check(replies.size()==count+1,"distant panel does not issue an unusable cue");
    puts("PASS: Via native collection bits, seven original chest callbacks, interruption/staleness/identity guards, manual relocation graph and nearby warp cues.");
    viaLoaded=false;resetViaPurifico();fixturePlayer={};
}
}
