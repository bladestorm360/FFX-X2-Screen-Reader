namespace {
uintptr_t __cdecl caveCrossingWorker(int index) {return index>=35 && index<=37?address(0x2450000+(index-35)*0x1000):0;}
uint16_t __cdecl caveCrossingEntry(unsigned,unsigned channel,uintptr_t) {return channel==2?2:0;}
uint8_t __cdecl caveCrossingPriority(unsigned,unsigned channel) {return channel==2?0:255;}
uintptr_t __cdecl exitNameGetter(unsigned index,unsigned) {
    lastArea=index;
    const char* name=lastArea==19?"Salvage Ship - Deck":lastArea==20?"Salvage Ship - Surface":
        (lastArea==23 || lastArea==24)?"Underwater Ruins":lastArea==322?"Salvage Ship - Underwater":"Native dungeon";
    return encoded(0x2314010,name);
}
AtelScript retainedExitScript(const char* name,const char* group="cd",unsigned* bytes=nullptr) {
    const auto file=(ffx_fixture::evidenceRoot()/"offline-output/FFX/20261003_audit/files/ffx_ps2/ffx/master/jppc/event/obj").string()+"/"+group+"/"+name+"/"+name+".ebp";
    std::ifstream source(file,std::ios::binary); check(source.good(),"retained exit EBP exists");
    unsigned offsets[11]{}; source.seekg(4); source.read(reinterpret_cast<char*>(offsets),sizeof(offsets));
    source.seekg(0,std::ios::end); const auto length=static_cast<unsigned>(source.tellg());
    const unsigned start=offsets[0]; unsigned end=length;
    for(unsigned i=1;i<11;++i) if(offsets[i] && offsets[i]>=start && offsets[i]<=length) { end=offsets[i]; break; }
    check(start==64 && end>start && end-start<0x70000,"actual exit script chunk bounds include sparse EBP chunks");
    source.seekg(start); source.read(reinterpret_cast<char*>(address(0x2390000)),end-start);
    check(source.gcount()==end-start,"whole retained exit script loaded");
    if(bytes) *bytes=end-start;
    auto script=readScript(address(0x2390000));
    check(navigationCatalog(script)!=nullptr,"exit name table shares the actual hash-qualified catalog");
    return script;
}
void caveCrossingCapture(const AtelScript& cave,unsigned bytes) {
    constexpr unsigned actor=0x2240000,controller=0x2250000,chunk=0x2390000;
    const unsigned globals[]={0x1fc44e4,0x1fc44e0,0xf00740,0xf26ae8,0xf01a48,0xf01a44};
    std::array<unsigned,6> previous{};
    for(unsigned i=0;i<6;++i) previous[i]=read<unsigned>(address(globals[i]));
    std::array<uint8_t,0x880> actorBefore{};std::array<uint8_t,0x220> controllerBefore{};
    std::memcpy(actorBefore.data(),reinterpret_cast<void*>(address(actor)),actorBefore.size());
    std::memcpy(controllerBefore.data(),reinterpret_cast<void*>(address(controller)),controllerBefore.size());
    const unsigned getters[]={0x46a7c0,0x476360,0x4767e0};std::array<std::array<uint8_t,7>,3> beforeGetters{};
    for(unsigned i=0;i<3;++i) std::memcpy(beforeGetters[i].data(),reinterpret_cast<void*>(address(getters[i])),7);
    fixturePhysicalPart(chunk,bytes);
    set<uintptr_t>(0x1fc44e4,address(actor));set<unsigned>(0x1fc44e0,1);set<uintptr_t>(0xf00740,address(actor));
    set<uintptr_t>(0xf26ae8,address(controller));set<uint8_t>(controller,2);set<uintptr_t>(controller+0x48,address(chunk));
    set<uint16_t>(actor+2,1);set<uint8_t>(actor+0x186,2);set<int16_t>(actor+0x824,45);
    set<float>(actor+0xc,178.5891f);set<float>(actor+0x10,88.16031f);set<float>(actor+0x14,-351.6188f);
    set<int>(0xf01a48,0);set<uintptr_t>(0xf01a44,0);
    stub(getters[0],caveCrossingWorker);stub(getters[1],caveCrossingEntry);stub(getters[2],caveCrossingPriority);
    for(unsigned i=0;i<3;++i) {
        const unsigned w=0x2450000+i*0x1000;
        set<uintptr_t>(w,address(chunk+cave.workers[35+i].header));set<uintptr_t>(w+4,address(chunk));
        set<uint16_t>(w+0x34,0x80);set<uint8_t>(w+0xab,4);
    }
    nav::NativeGeometryContext context;
    check(nav::captureNativeGeometry(context,1) && context.waterBake==1 && context.waterCrossings==7,
        "original prepared cave identity and three actual callback facts select cross-water connections");
    set<uint8_t>(0x2451000+0xab,0);
    check(nav::captureNativeGeometry(context,1) && context.waterCrossings==5,"disabled shore callback retires its route connection");
    set<uint8_t>(0x2451000+0xab,4);set<uintptr_t>(0x2452000+4,address(chunk+4));
    check(nav::captureNativeGeometry(context,1) && context.waterCrossings==3,"foreign shoreline worker cannot adopt a native crossing");
    set<uintptr_t>(0x2452000+4,address(chunk));
    const auto begin=std::chrono::steady_clock::now();nativeHeapFixture::allocations=nativeHeapFixture::frees=0;nativeHeapFixture::observing=true;
    bool ready=true;for(unsigned i=0;i<100;++i) ready=nav::captureNativeGeometry(context,i+1)&&ready;
    nativeHeapFixture::observing=false;
    check(ready && !nativeHeapFixture::allocations && !nativeHeapFixture::frees,"bounded native cross-water observations never allocate or free");
    const double ms=std::chrono::duration<double,std::milli>(std::chrono::steady_clock::now()-begin).count();
    const auto code=chunk+read<unsigned>(address(chunk+0x30));const auto original=read<uint8_t>(address(code));set<uint8_t>(code,original^1);
    fixturePhysicalPart(chunk,bytes);
    check(nav::captureNativeGeometry(context,1) && !context.waterBake && !context.waterCrossings,"changed prepared cave script refuses original crossing bake");
    set<uint8_t>(code,original);fixturePhysicalPart(chunk,bytes);
    for(unsigned i=0;i<3;++i) std::memcpy(reinterpret_cast<void*>(address(getters[i])),beforeGetters[i].data(),7);
    std::memcpy(reinterpret_cast<void*>(address(actor)),actorBefore.data(),actorBefore.size());
    std::memcpy(reinterpret_cast<void*>(address(controller)),controllerBefore.data(),controllerBefore.size());
    for(unsigned i=0;i<6;++i) set<unsigned>(globals[i],previous[i]);
    std::printf("PASS: original cave resource identity, native shoreline callback/owner guards and zero-heap observations; 100 controlled native captures %.3fms (SDK getter endpoints simulated).\n",ms);
}
void exitLabelFixtures() {
    stub(0x46c3c0,reinterpret_cast<void*>(exitNameGetter)); set<uint16_t>(0x2314000,1024*4);
    const auto underwater=retainedExitScript("cdsp0100"),ruins=retainedExitScript("cdsp0700");
    check(navigationExitName(underwater,5,64,L"Exit")==L"Exit 1 to Underwater Ruins","underwater passage names its actual destination");
    check(navigationExitName(underwater,6,71,L"Exit")==L"Exit 2 to Salvage Ship","multiple-area room uses native common location without guessing area from entrance");
    const auto passage=navigationExitName(ruins,28,380,L"Exit");
    if(passage!=L"Exit 1 to Underwater Ruins") std::fwprintf(stderr,L"Ruins passage: [%s], checksum %x\n",passage.c_str(),ruins.checksum);
    check(passage==L"Exit 1 to Underwater Ruins","same-named dungeon passage retains its stable identity");
    check(navigationExitName(ruins,37,71,L"Exit")==L"Exit 2 to Salvage Ship" &&
        navigationExitName(ruins,38,71,L"Exit")==L"Exit 3 to Salvage Ship","two same-destination passages remain distinct even when arrival points match");
    const auto original=navigationExitName(ruins,38,71,L"Exit");
    check(navigationExitName(ruins,38,71,L"Exit")==original,"identity survives repeated scans and other exits disappearing");
    AtelScript unknown; unknown.name="changed"; unknown.checksum=ruins.checksum;
    check(navigationExitName(unknown,37,-1,L"Exit")==L"Exit 38","different script cannot adopt hashed catalog destinations");
    check(navigationExitName(unknown,2,-1,L"Exit to the antechamber")==L"Exit 3 to the antechamber","unresolved destination preserves the authored trial role");
    const auto winno=retainedExitScript("swin0200","sw");
    const auto winnoName=[](int area) {return area==91?std::wstring(L"S.S. Winno - Deck"):std::wstring(L"Zanarkand");};
    check(navigationExitName(winno,34,191,L"Exit",winnoName)==L"Exit 1 to Jecht Shot challenge","original Jecht trigger names its challenge role instead of the later flashback room");
    auto changedWinno=winno;changedWinno.checksum^=1;
    check(navigationExitName(changedWinno,34,191,L"Exit",winnoName)!=L"Exit 1 to Jecht Shot challenge","unverified Winno script cannot adopt the challenge correction");
    const auto gagazet=retainedExitScript("mtgz0700","mt");
    const auto gagazetCatalog=navigationCatalog(gagazet);
    const auto firstTrial=navigationTarget(gagazetCatalog,32),secondTrial=navigationTarget(gagazetCatalog,33);
    check(firstTrial && firstTrial->category==2 && firstTrial->room==-1 && firstTrial->entries.contains(2) &&
        navigationEntityName(firstTrial,0,false)==L"First Trial of Gagazet: Wakka's ball target",
        "original W32 entry2 is a named first-trial trigger, not its nested late-story reward warp");
    check(secondTrial && secondTrial->category==2 && secondTrial->entries.contains(2) &&
        navigationEntityName(secondTrial,0,false)==L"Second Trial of Gagazet: coloured openings",
        "original W33 entry2 is a distinct second-trial trigger");
    for(unsigned worker:{29u,30u,31u}) check(navigationTarget(gagazetCatalog,worker)->category==1,
        "all three original water-to-cave exits keep their identity and connectivity");
    auto changedGagazet=gagazet;changedGagazet.checksum^=1;
    check(!navigationCatalog(changedGagazet),"changed Gagazet script cannot adopt the trial-label correction");
    const auto sphereScript=retainedExitScript("mtgz0800","mt");
    const auto sphereCatalog=navigationCatalog(sphereScript);
    const auto sphereObject=navigationTarget(sphereCatalog,9);
    check(sphereObject && sphereObject->category==6 && sphereObject->room==-1 && sphereObject->entries.contains(2) &&
        navigationEntityName(sphereObject,0,false)==L"Yuna's sphere",
        "original mtgz0800 worker9 entry2 names Yuna's revealed dropped sphere");
    check(navigationTarget(sphereCatalog,18)->room==272 && navigationTarget(sphereCatalog,19)->room==312 &&
        navigationTarget(sphereCatalog,18)->category==1 && navigationTarget(sphereCatalog,19)->category==1,
        "sphere target does not change either original summit exit");
    auto changedSphere=sphereScript;changedSphere.checksum^=1;
    check(!navigationCatalog(changedSphere),"changed mtgz0800 script cannot adopt the sphere overlay");
    const auto dome=retainedExitScript("dome0600","do");
    const auto domeCatalog=navigationCatalog(dome);
    const auto staircase1=navigationTarget(domeCatalog,25),staircase2=navigationTarget(domeCatalog,26);
    check(staircase1 && staircase1->category==2 && staircase1->room==-1 && staircase1->entries.contains(2) &&
        navigationEntityName(staircase1,0,false)==L"Staircase 1" &&
        staircase2 && staircase2->category==2 && staircase2->room==-1 && staircase2->entries.contains(2) &&
        navigationEntityName(staircase2,0,false)==L"Staircase 2",
        "original dome0600 W25 and W26 entry2 remain distinct gated staircase events");
    check(navigationTarget(domeCatalog,24)->category==1 && navigationTarget(domeCatalog,24)->room==319 &&
        navigationTarget(domeCatalog,15)->category==4,
        "staircase events retain the original room exit and type-only chest target");
    auto changedDome=dome;changedDome.checksum^=1;
    check(!navigationCatalog(changedDome),"changed dome0600 script cannot adopt the staircase overlays");
    unsigned caveBytes=0;const auto cave=retainedExitScript("mtgz0600","mt",&caveBytes);
    const auto swimmingParty=navigationTarget(navigationCatalog(cave),33);
    check(swimmingParty && swimmingParty->category==2 && swimmingParty->room==-1 && swimmingParty->entries.contains(2) &&
        navigationEntityName(swimmingParty,0,false)==L"Prepare swimming party",
        "original dry-cave crossing names the native swimming-party setup event");
    caveCrossingCapture(cave,caveBytes);
    stub(0x46c3c0,reinterpret_cast<void*>(mapName));
    std::puts("PASS: real underwater/ruins destination names, distinct dungeon passages, stable exit IDs and changed-script guards.");
}
}
