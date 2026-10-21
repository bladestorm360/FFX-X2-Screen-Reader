namespace {
unsigned deniedSphereCalls=0;
uintptr_t __cdecl deniedSphereGetter(unsigned,uintptr_t,uintptr_t*) {++deniedSphereCalls;return 0;}
void sphereOwnedFixtures() {
    resetSphereGrid();drainSpeechDomainsForTest();setSetting(Setting::AutoDetail,0);
    constexpr unsigned grid=0x2320000;std::vector<uint8_t> savedGrid(0x12000);std::memcpy(savedGrid.data(),reinterpret_cast<const void*>(address(grid)),savedGrid.size());
    set<uint8_t>(0x1685f70,1);set<int16_t>(grid+2,3);set<int16_t>(grid+4,2);set<uint8_t>(grid+0x115bc,0);set<int16_t>(grid+0x112fc,0);
    for(unsigned i=0;i<3;++i) {set<int16_t>(grid+0x80e+i*0x28,static_cast<int16_t>(i==2?1:0));set<int16_t>(grid+0x808+i*0x28,static_cast<int16_t>(i*10));set<int16_t>(grid+0x80a+i*0x28,0);}
    for(unsigned i=0;i<2;++i) {set<int16_t>(grid+0xa808+i*0x14,static_cast<int16_t>(i));set<int16_t>(grid+0xa80a+i*0x14,static_cast<int16_t>(i+1));}
    set<uint16_t>(0x2310100+0x12,1);set<int16_t>(grid+0x110cc,0);
    std::vector<uint8_t> preparedGrid(0x12000);std::memcpy(preparedGrid.data(),reinterpret_cast<const void*>(address(grid)),preparedGrid.size());
    GridSnapshot baseline;check(sphereSnapshot(baseline,true),"original native snapshot baseline");
    using Clock=std::chrono::steady_clock;nativeHeapFixture::allocations=nativeHeapFixture::frees=0;nativeHeapFixture::observing=true;
    auto begin=Clock::now();const bool captured=captureSphereFactsForTest();const auto cold=std::chrono::duration<double,std::micro>(Clock::now()-begin).count();
    nativeHeapFixture::observing=false;check(captured && !nativeHeapFixture::allocations && !nativeHeapFixture::frees,"actual copied Sphere Native capture owns no heap allocation or free");
    begin=Clock::now();nativeHeapFixture::observing=true;check(captureSphereFactsForTest(),"warm static catalog capture");nativeHeapFixture::observing=false;
    const auto warm=std::chrono::duration<double,std::micro>(Clock::now()-begin).count();check(!nativeHeapFixture::allocations && !nativeHeapFixture::frees,"warm live Sphere delta capture owns no heap allocation or free");
    const auto before=said.size();begin=Clock::now();openSpherePlanner(false);check(spherePlannerActive(),"pending copied planner immediately reserves modal input");
    const unsigned getters[]={0x385070,0x3853f0,0x3852b0,0x3ab890,0x390a40,0x39a4c0,0x390500,0x38fd40};std::array<std::array<uint8_t,7>,8> code{};
    for(unsigned i=0;i<_countof(getters);++i) {check(copy(address(getters[i]),code[i].data(),7),"save affine getter fixture");stub(getters[i],reinterpret_cast<void*>(deniedSphereGetter));}
    std::memset(reinterpret_cast<void*>(address(grid)),0xa5,0x12000);drainSpeechDomainsForTest();
    const auto usable=std::chrono::duration<double,std::milli>(Clock::now()-begin).count();
    testStatusOwner=false;GridSnapshot owned;const bool restored=sphereOwnedGridForTest(owned);testStatusOwner=true;
    check(restored && !deniedSphereCalls && owned.nodes.size()==baseline.nodes.size() && owned.links.size()==baseline.links.size(),"owned planner processes copied full layout with no affine getter after source overwrite");
    for(unsigned i=0;i<owned.nodes.size();++i) check(owned.nodes[i].name==baseline.nodes[i].name && owned.nodes[i].description==baseline.nodes[i].description && owned.nodes[i].sphere==baseline.nodes[i].sphere && owned.nodes[i].ability==baseline.nodes[i].ability,"copied node/text/material semantics match original snapshot");
    check(said.size()>before && spherePlannerActive(),"owned ability menu publishes usable output");
    for(unsigned i=0;i<_countof(getters);++i) {std::memcpy(reinterpret_cast<void*>(address(getters[i])),code[i].data(),7);FlushInstructionCache(GetCurrentProcess(),reinterpret_cast<void*>(address(getters[i])),7);}
    std::memcpy(reinterpret_cast<void*>(address(grid)),preparedGrid.data(),preparedGrid.size());set<uint8_t>(0x1685f70,1);
    spherePlannerKey(VK_ESCAPE);drainSpeechDomainsForTest();check(!spherePlannerActive(),"owned planner closes through copied input");
    // Equal user actions retain separate results; held frames recover through
    // atomic invalidation even when the capture marker itself is discarded.
    const auto equals=said.size();for(unsigned i=0;i<4;++i) sphereOrientationKey(false);drainSpeechDomainsForTest();check(said.size()==equals+4,"four equal owned orientation requests retain four outputs");
    sphereOrientationKey(false);clearSphereTargets();speechq::fixture::pending.clear();
    // Admit the fresh command before maintenance, testing that retirement runs
    // ahead of it and cannot clear its newly opened menu afterwards.
    openSpherePlanner(false);drainSpeechDomainsForTest();check(spherePlannerActive(),"same-generation reset retires old ownership before the fresh menu, without erasing it");
    spherePlannerKey(VK_ESCAPE);drainSpeechDomainsForTest();check(!spherePlannerActive(),"fresh menu remains independently closable");
    const auto recovered=said.size();sphereOrientationKey(false);drainSpeechDomainsForTest();check(said.size()==recovered+1,"fresh orientation recovers after discarded reset packet");
    // Production maximum fixed transport shape; live catalog/poses are
    // controlled adapters, and the original snapshot is the semantic baseline.
    set<int16_t>(grid+2,1024);set<int16_t>(grid+4,1023);set<int16_t>(grid+0x112fc,0);set<uint8_t>(grid+0x115bc,0);
    for(unsigned i=0;i<1024;++i) {set<int16_t>(grid+0x80e+i*0x28,static_cast<int16_t>(i==1023?1:0));set<int16_t>(grid+0x808+i*0x28,static_cast<int16_t>(i));set<int16_t>(grid+0x80a+i*0x28,0);}
    for(unsigned i=0;i<1023;++i) {set<int16_t>(grid+0xa808+i*0x14,static_cast<int16_t>(i));set<int16_t>(grid+0xa80a+i*0x14,static_cast<int16_t>(i+1));set<uint8_t>(grid+0xa814+i*0x14,1);}
    clearSphereTargets();drainSpeechDomainsForTest();nativeHeapFixture::observing=true;begin=Clock::now();const bool large=captureSphereFactsForTest();const auto largeCold=std::chrono::duration<double,std::milli>(Clock::now()-begin).count();nativeHeapFixture::observing=false;
    check(large && !nativeHeapFixture::allocations && !nativeHeapFixture::frees,"1024-node/1023-link cold capture owns no Native heap");
    nativeHeapFixture::observing=true;begin=Clock::now();const bool largeWarmOkay=captureSphereFactsForTest();const auto largeWarm=std::chrono::duration<double,std::milli>(Clock::now()-begin).count();nativeHeapFixture::observing=false;
    check(largeWarmOkay && !nativeHeapFixture::allocations && !nativeHeapFixture::frees,"1024-node warm capture owns no Native heap");
    begin=Clock::now();openSpherePlanner(false);drainSpeechDomainsForTest();const auto largeUsable=std::chrono::duration<double,std::milli>(Clock::now()-begin).count();check(spherePlannerActive(),"maximum copied shape produces usable ability menu");spherePlannerKey(VK_ESCAPE);drainSpeechDomainsForTest();
    std::printf("METRIC maximum1024/1023 Native cold %.3fms warm %.3fms request-to-usable %.3fms; controlled catalog/chain, no actual thread/SDK/game timing.\n",largeCold,largeWarm,largeUsable);
    std::printf("PASS: owned Sphere capture/menu/routes, Native heap0/0, source reuse/getter denial, equal events and reset recovery. METRIC cold %.3fus warm %.3fus request-to-usable %.3fms (semantic domain, no game/SDK/audible claim).\n",cold,warm,usable);
    std::memcpy(reinterpret_cast<void*>(address(grid)),savedGrid.data(),savedGrid.size());resetSphereGrid();drainSpeechDomainsForTest();set<uint8_t>(0x1685f70,0);
}
}
