namespace {
void swimmingProgressionFixtures() {
    constexpr unsigned actor=0x2240000,ctrl=0x2250000,worker=0x2251000,signal=0x2251800,chunk=0x2374000;
    std::ifstream ebp(ffx_fixture::evidenceRoot()/"offline-output/FFX/20261003_audit/files/ffx_ps2/ffx/master/jppc/event/obj/cd/cdsp0000/cdsp0000.ebp",std::ios::binary);
    check(ebp.good(),"actual retained salvage ship EBP exists");
    unsigned offset=0,end=0; ebp.seekg(4); ebp.read(reinterpret_cast<char*>(&offset),4); ebp.read(reinterpret_cast<char*>(&end),4);
    check(offset==64 && end>offset && end-offset<0x20000,"native EBP ATEL chunk bounds");
    ebp.seekg(offset); ebp.read(reinterpret_cast<char*>(address(chunk)),end-offset);
    check(ebp.gcount()==end-offset,"complete real surface/deck script is loaded into isolated fixture memory");
    fixturePhysicalPart(chunk,end-offset);
    const auto script=readScript(address(chunk)); const auto catalog=navigationCatalog(script);
    check(catalog && script.name=="cdsp0000" && script.checksum==0xf5f6d993 && script.workers[30].entries[6]==60444,
        "actual hash-qualified controller matches retained transition evidence");
    stub(0x46a7c0,reinterpret_cast<void*>(navigationWorker));
    set<uintptr_t>(0x1fc44e4,address(actor)); set<unsigned>(0x1fc44e0,2); set<uintptr_t>(0xf00740,address(actor));
    set<uint16_t>(actor+2,1); set<uint8_t>(actor+0x185,2); set<uint8_t>(actor+0x186,2); set<uint8_t>(actor+0x182,2);
    set<float>(actor+0xc,-192); set<float>(actor+0x10,59); set<float>(actor+0x14,79); set<int16_t>(actor+0x824,0);
    set<uintptr_t>(0xf26ae8,address(ctrl)); set<uint8_t>(ctrl,2); set<uint16_t>(ctrl+0xc,1);
    set<uintptr_t>(ctrl+0x1f8,0); set<uintptr_t>(ctrl+0x48,address(chunk));
    set<uintptr_t>(worker,address(chunk+script.workers[30].header)); set<uintptr_t>(worker+4,address(chunk));
    set<uint16_t>(worker+0x2e,30); set<uint8_t>(worker+0x32,1); set<uintptr_t>(worker+0x90,address(signal));
    set<uint16_t>(signal+8,6); set<uint8_t>(signal+0xe,1);
    const auto code=chunk+read<unsigned>(address(chunk+0x30));
    set<uintptr_t>(worker+0x12c+0x4c+0x18,address(code+60510));
    set<uint16_t>(0xd2ca90,71); set<uint16_t>(0xd2ca90+0xe,20);
    set<uint8_t>(swimming_rva::DiveEnabled,1); set<uint8_t>(0xd2a8e0,0);
    // Match the actual native input conditions with no current scheduler
    // signal. Navigation must remain available between ATEL slices.
    set<uintptr_t>(worker+0x90,0); set<uint8_t>(actor+0x185,0); set<uint8_t>(actor+0x182,0);
    set<uint8_t>(0x1685f70,0); resetSphereGrid(); scene=0;
    set<uint16_t>(0x2314000,1024*4); mapFixtureName="Salvage Ship - Surface";
    announceCurrentMap();drainSpeechDomainsForTest();
    check(activeField() && said.back().first==L"Salvage Ship - Surface",
        "loaded primary field chunk remains active and named with auxiliary +1F8 clear");
    mapFixtureName="Salvage Ship - Underwater";
    resetNavigation(); std::vector<Entity> found; scanExits(playerPosition(),found);
    check(found.size()==1 && found[0].swimDepth && found[0].worker==0 && found[0].category==L"Exit" && found[0].name==L"Exit to Salvage Ship - Underwater",
        "active invisible controller produces one correctly named exit with no ship actors");
    auto goal=found[0]; const auto route=navigationRoute(readMesh(),playerPosition(),goal);
    check(route.size()==2 && route.back().y==251 && route.back().x==-192 && route.back().z==79,
        "depth goal works without collision data and does not invent a horizontal destination");
    check(!destinationUnreachable({},playerPosition(),goal),"unreachable filter preserves the active depth exit");
    check(!navigationTargetReached(goal,{-192,250,79}) && navigationTargetReached(goal,{-192,250.01,79}),
        "arrival uses native strict Y greater than 250, never horizontal proximity");
    for(unsigned i=0;i<8;++i) {
        keys[VK_OEM_3](false);navigationFixturePump();
        if(said[said.size()-2].first==L"1 All") break;
        keys[VK_OEM_6](true);navigationFixturePump();
    }
    check(said.back().first.find(L"Exit to Salvage Ship - Underwater. Dive first. Hold ")==0 &&
        said.back().first.find(L"85 percent of the descent remains")!=std::wstring::npos,
        "entity list describes actual Dive action rather than zero steps here");
    set<int>(0xf01a48,0); set<uintptr_t>(0xf01a44,0); setSetting(Setting::AutoWalk,1);
    const auto before=autoStarts; keys[VK_OEM_5](false);navigationFixturePump();
    check(autoStarts==before+1 && said.back().first.find(L"Dive first. Hold ")==0,
        "normal route command starts Auto walk for depth transition without a floor route");
    setSetting(Setting::AutoWalk,0);
    set<float>(actor+0x10,200);
    check(swimmingGoalInstruction().find(L"23 percent of the descent remains")!=std::wstring::npos,"descent guidance updates from actual player depth");
    for(unsigned guard=0;guard<5;++guard) {
        if(guard==0) set<uint8_t>(actor+0x186,0);
        if(guard==1) set<uint8_t>(swimming_rva::DiveEnabled,0);
        if(guard==2) set<uint8_t>(ctrl,0);
        if(guard==3) set<uint16_t>(0xd2ca90,288);
        if(guard==4) set<uint16_t>(0xd2ca90+0xe,21);
        refreshNavigationTarget(goal);
        found.clear(); scanExits(playerPosition(),found);
        if(guard<2) {
            check(!goal.inactive && !goal.swimReady && found.size()==1,"known Underwater exit remains listed before native Dive is ready");
            check(!swimmingGoalActive(goal) && navigationRoute(readMesh(),playerPosition(),goal).empty(),"unready Dive cannot start automatic movement");
        } else check(goal.inactive && found.empty(),"unrelated room/area or disabled control retires the exit");
        set<uint8_t>(actor+0x186,2); set<uint16_t>(0xd2ca90+0xe,20);
        set<uint8_t>(swimming_rva::DiveEnabled,1); set<uint8_t>(ctrl,2); set<uint16_t>(0xd2ca90,71);
        set<uintptr_t>(worker+0x12c+0x4c+0x18,address(code+60510));
    }
    Entity changed; check(!configureSwimmingGoal(changed,"unknown",30,address(worker)) &&
        !configureSwimmingGoal(changed,"cdsp0000",29,address(worker)),"unverified scripts/controllers cannot produce depth goals");
    refreshNavigationTarget(goal); check(!goal.inactive && goal.swimReady,"restored native control makes the linked exit routable again");
    realSwimmingFloor(goal);
    set<uint16_t>(0xd2ca90+0xe,19);set<uint8_t>(actor+0x186,0);
    found.clear();scanExits(playerPosition(),found);
    check(found.size()==1 && !found[0].swimReady && found[0].name==L"Exit to Salvage Ship - Underwater",
        "Deck can list the real linked Underwater exit and explain Dive first without starting assistance");
    resetNavigation(); mapFixtureName="Besaid Village";
    std::puts("PASS: real salvage ship depth exit discovery, native goal/label, filter, route/Auto walk dispatch and live producer guards.");
}
}
