namespace {
void viaSwimmingFixtures() {
    constexpr unsigned chunk=0x2410000,ctrl=0x2430000,workers=0x2440000,actor=0x2240000;
    constexpr unsigned triangles=0x2500000,vertices=0x2510000;
    // User-authorized actual tester save; no game load or guessed live pointers.
    std::array<uint8_t,26880> saved{};
    const auto savePath=ffx_fixture::repositoryRoot().parent_path()/"Saves/Dylan/ViaPurifico-20261012/ffx_050";
    std::ifstream save(savePath,std::ios::binary);
    check(save.good(),"Dylan's exact approved Via save retained");
    save.read(reinterpret_cast<char*>(saved.data()),saved.size());check(save.gcount()==saved.size(),"complete actual tester save");
    const auto savedWord=[&](unsigned offset) {uint16_t value;std::memcpy(&value,saved.data()+64+offset,2);return value;};
    check(savedWord(0)==209 && savedWord(0xe)==244 && savedWord(0xba)==209 && savedWord(0xb8)==1 && savedWord(0xbec)==2220,
        "tester save confirms room209, native area244, saved entrance1 and story2220");
    const auto priorStory=read<uint16_t>(address(0xd2ca90)+0xbec);
    const auto priorUnderwater=read<uint16_t>(address(0xd2ca90)+0x28);
    set<uint16_t>(0xd2ca90+0xbec,savedWord(0xbec));set<uint16_t>(0xd2ca90+0x28,savedWord(0x28));
    const auto ebp=ffx_fixture::evidenceRoot()/"offline-output/FFX/20261003_audit/files/ffx_ps2/ffx/master/jppc/event/obj/st/stbv0000/stbv0000.ebp";
    std::ifstream source(ebp,std::ios::binary);check(source.good(),"actual Via underwater EBP exists");
    unsigned begin=0,end=0;source.seekg(4);source.read(reinterpret_cast<char*>(&begin),4);source.read(reinterpret_cast<char*>(&end),4);
    check(begin==64 && end>begin && end-begin<0x20000,"real Via script bounds");
    source.seekg(begin);source.read(reinterpret_cast<char*>(address(chunk)),end-begin);
    check(source.gcount()==end-begin,"whole actual Via underwater chunk");
    fixturePhysicalPart(chunk,end-begin);
    const auto script=readScript(address(chunk));const auto catalog=navigationCatalog(script);
    check(catalog && script.name=="stbv0000" && script.checksum==0xada03b86,"actual Via underwater identity");
    stub(0x46a7c0,airshipWorker);stub(0x476360,interactionEntry);stub(0x4767e0,interactionPriority);stub(0x46c500,unboundGuard);
    stub(0x4b7f10,mapTable);stub(0x46c3c0,mapName);set<uint16_t>(0x2314000,1024*4);
    set<uintptr_t>(0xf26ae8,address(ctrl));set<uint8_t>(ctrl,2);set<uint16_t>(ctrl+0xc,static_cast<uint16_t>(script.workers.size()));
    set<uintptr_t>(ctrl+0x48,address(chunk));set<uintptr_t>(ctrl+0x1f8,0);
    set<uintptr_t>(0x1fc44e4,address(actor));set<unsigned>(0x1fc44e0,3);set<uintptr_t>(0xf00740,address(actor));
    set<uint16_t>(actor+2,1);set<uint8_t>(actor+0x186,2);set<unsigned>(actor+0x198,0);
    set<float>(actor+0xc,1649.4752f);set<float>(actor+0x10,-54.9421f);set<float>(actor+0x14,-1761.332f);
    airshipWorkers.clear();
    for(unsigned i=0;i<script.workers.size();++i) {
        const auto off=workers+i*0x1000;ZeroMemory(reinterpret_cast<void*>(address(off)),0x1000);
        airshipWorkers.push_back(address(off));set<uintptr_t>(off,address(chunk+script.workers[i].header));set<uintptr_t>(off+4,address(chunk));
        set<uint16_t>(off+0x2e,static_cast<uint16_t>(i));set<uint16_t>(off+0x34,0x80);set<uint8_t>(off+0xaa,1);
    }
    const auto line=[&](unsigned i,Point a,Point b,float radius,unsigned channel) {
        const auto off=workers+i*0x1000+0x558;
        for(unsigned j=0;j<3;++j) {const double av[]={a.x,a.y,a.z},bv[]={b.x,b.y,b.z};set<float>(off+j*4,static_cast<float>(av[j]));set<float>(off+0x30+j*4,static_cast<float>(bv[j]));}
        set<float>(off+0x1c,radius);set<float>(off+0x48,radius);set<uint8_t>(workers+i*0x1000+0xab,static_cast<uint8_t>(channel));
    };
    line(25,{1237.3505859375,-129.9998779296875,-1509.93701171875},{1221.75244140625,-129.9998779296875,-1369.982421875},20,0x20);
    line(26,{61.9183349609375,-14.939315795898438,-349.960693359375},{87.8355941772461,-14.939315795898438,-210.09506225585938},10,0x10);
    auto load=[&](const char* name,unsigned destination,unsigned bytes) {
        std::ifstream file((ffx_fixture::evidenceRoot()/"offline-output/open_issue_repairs_20261012/via_swim").string()+"/"+name,std::ios::binary);
        check(file.good(),"actual Via swim geometry exists");file.read(reinterpret_cast<char*>(address(destination)),bytes);check(file.gcount()==bytes,"complete Via swim geometry");
    };
    load("triangles.bin",triangles,86*16);load("vertices.bin",vertices,85*8);
    set<int>(0xf01a48,86);set<uintptr_t>(0xf01a44,address(triangles));set<uintptr_t>(0xf01a4c,address(vertices));set<float>(0xf01a50,17.3559326171875f);
    fixtureCollisionVertexCount(85);
    set<uint8_t>(0xd2a8e0,0);scene=0;set<uint16_t>(0xd2ca90,209);mapFixtureName="Bevelle - Via Purifico";
    for(unsigned area:{244u,245u}) for(unsigned spawn:{0u,1u,2u}) {
        set<uint16_t>(0xd2ca90+0xe,static_cast<uint16_t>(area));set<uint8_t>(0xd2ca90+0xc,static_cast<uint8_t>(spawn));
        announceCurrentMap();drainSpeechDomainsForTest();
        check(activeField() && said.back().first==L"Bevelle - Via Purifico" && lastArea==area,"both real Via underwater regions keep their native name with the auxiliary chunk absent");
    }
    set<uint16_t>(0xd2ca90,savedWord(0));set<uint16_t>(0xd2ca90+0xe,savedWord(0xe));
    set<uint8_t>(0xd2ca90+0xc,static_cast<uint8_t>(savedWord(0xb8)));
    announceCurrentMap();drainSpeechDomainsForTest();
    check(activeField() && said.back().first==L"Bevelle - Via Purifico","actual saved Via context resolves its native map name");
    set<int16_t>(actor+0x824,0);auto initial=readMesh();const auto floor=findTriangle(initial,playerPosition());check(floor>=0,"actual Via entrance has a native swim floor");
    set<int16_t>(actor+0x824,static_cast<int16_t>(floor));const auto mesh=readMesh();
    resetNavigationScripts();std::vector<Entity> entities;scanExits(playerPosition(),entities);
    check(entities.size()==2,"real Via boss approach and progress exit both remain discoverable");
    const auto boss=std::find_if(entities.begin(),entities.end(),[](const auto& e){return e.worker==25;});
    const auto exit=std::find_if(entities.begin(),entities.end(),[](const auto& e){return e.worker==26;});
    check(boss!=entities.end() && boss->category==L"Event" && boss->zone.range==20 && !boss->zone.crossing,"boss callback6 uses the native line-region radius rather than a zero-width point");
    check(exit!=entities.end() && exit->category==L"Exit" && exit->zone.crossing,"post-boss Common010C progression exit retains its actual crossing callback");
    auto route=navigationRoute(mesh,playerPosition(),*boss);
    if(route.empty()) std::fprintf(stderr,"Via debug floor %d valid %zu direct %zu boss shape %u range %.2f height %.2f start %.2f %.2f %.2f\n",floor,std::count_if(mesh.triangles.begin(),mesh.triangles.end(),[](const auto& t){return t.valid;}),routePoints(mesh,playerPosition(),boss->zone.center).size(),boss->zone.shape,boss->zone.range,boss->zone.height,playerPosition().x,playerPosition().y,playerPosition().z);
    check(route.size()>=2,"actual Via entrance supplies a boss approach at its initial native depth");
    for(double y:{-55.0,-90.0,-110.0,-125.0,-140.0}) {
        auto origin=playerPosition();origin.y=y;const auto continued=navigationRoute(mesh,origin,*boss);
        check(!continued.empty() && continued.back().y==y,"cached Via swimming geometry survives native depth changes without snapping to old Y");
        check(!destinationUnreachable(mesh,origin,*boss),"reachability filter retains the reachable swimming event at changing depth");
    }
    auto approach=route.back();approach.y=-55;check(!navigationTargetReached(*boss,approach),"horizontal approach cannot claim boss activation at the wrong native height");
    approach.y=-120;check(navigationTargetReached(*boss,approach),"boss arrival still requires the real native height and range");
    set<uint8_t>(workers+25*0x1000+0xab,0);set<float>(actor+0xc,998);set<float>(actor+0x10,-30);set<float>(actor+0x14,-1474);
    entities.clear();scanExits(playerPosition(),entities);check(entities.size()==1 && entities[0].worker==26,"after boss disables its boundary the required exit remains listed");
    route=navigationRoute(mesh,playerPosition(),entities[0]);check(route.size()>=2,"actual Via corridor supplies the post-boss exit approach");
    for(unsigned i:{16u,17u}) {
        const auto off=workers+i*0x1000,body=actor+(i-15)*0x880;set<uintptr_t>(off+0x9c,address(body));set<uint16_t>(body,0x50ab);set<uint16_t>(body+2,1);set<unsigned>(body+0x824,0);
        const Point p=i==16?Point{1623.0029296875,-84.99993896484375,-1707.5}:Point{1301,-84.99993896484375,-1365};
        for(unsigned j=0;j<3;++j) {const double values[]={p.x,p.y,p.z};set<float>(body+0xc+j*4,static_cast<float>(values[j]));set<float>(off+0x558+j*4,static_cast<float>(values[j]));}
        set<float>(off+0x558+0x2c,20);set<float>(off+0x558+0x40,20);set<float>(off+0x558+0x3c,3.1415927f);set<uint8_t>(off+0xab,1);
    }
    entities.clear();scanExits(playerPosition(),entities);
    check(entities.size()==3 && std::count_if(entities.begin(),entities.end(),[](const auto& e){return e.category==L"Save Sphere";})==2,"Via save spheres remain distinct bound underwater actors alongside its exit");
    for(const auto& target:entities) check(!navigationRoute(mesh,playerPosition(),target).empty(),"both native save-sphere positions and progression exit have connected horizontal swim approaches");
    set<uint8_t>(workers+26*0x1000+0xab,0);entities.clear();scanExits(playerPosition(),entities);check(entities.size()==2,"disabled progress callback cannot be invented");
    resetNavigationScripts();set<int>(0xf01a48,0);set<uintptr_t>(0xf01a44,0);set<uintptr_t>(0xf26ae8,0);set<uintptr_t>(0x1fc44e4,0);set<uintptr_t>(0xf00740,0);set<unsigned>(0x1fc44e0,0);
    set<uint16_t>(0xd2ca90+0xbec,priorStory);set<uint16_t>(0xd2ca90+0x28,priorUnderwater);
    stub(0x46a7c0,navigationWorker);mapFixtureName="Besaid Village";
    puts("PASS: actual Via underwater script and86-triangle floor, both region names, boss radius, cached depth changes, native arrival guards, post-boss exit and two save spheres.");
}
}
