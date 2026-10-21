namespace {
void sphereMovementFixtures() {
    resetSphereGrid(); setSetting(Setting::AutoDetail,1); set<uint8_t>(0x1685f70,1);
    const unsigned originalCallback=extendedAddresses.at(0x656160);
    if(nativeProfile==NativeProfile::Installed) extendedAddresses[0x656160]=0x656150;
    constexpr unsigned choices=pool+11*0x340,grid=0x2320000;
    set<uintptr_t>(0x1686108,address(pool)); set<int16_t>(pool+0x2700,11);
    set<uintptr_t>(pool+0x2704,address(0x656160)); set<uint8_t>(choices+0x23,1);
    set<int16_t>(choices+0x1e,2); set<int16_t>(choices+0x18,0);
    encoded(0x2105800,"Move to this location"); set<uint8_t>(0x2105800+21,0x4f); set<uint8_t>(0x2105800+22,0);
    encoded(0x2105840,"Quit Sphere Grid");
    set<uintptr_t>(pool+9*0x340+0x40,address(0x2105800));
    set<uintptr_t>(pool+10*0x340+0x40,address(0x2105840));
    set<uintptr_t>(choices+0x40,address(0x21001a0)); set<uintptr_t>(choices+0x4c,address(0x21001c0));
    auto before=said.size(); event(0x6583e0,0);
    check(said.size()==before+2 && said[before].first==L"Move to this location?" && said.back().first==L"Yes",
        "movement callback selects native slot 9 prompt and actual Yes choice");
    check(said[before].second && !said.back().second,"movement prompt precedes queued initial focus");
    keys['I'](false); expect(L"Move to this location?","I reads movement prompt rather than grid/action detail");
    before=said.size(); event(0x6583e0,0); check(said.size()==before,"movement prompt idle silence");
    event(0x6583e0,0,[=]{set<int16_t>(choices+0x18,1);}); expect(L"No","movement choice navigation");
    check(said.back().second,"moved choice interrupts");
    event(0x6583e0,0,[=]{set<int16_t>(pool+0x2700,-1);});
    before=said.size();
    event(0x6583e0,0,[=]{set<int16_t>(pool+0x2700,11);set<int16_t>(choices+0x18,0);});
    check(said.size()==before+2 && said[before].first==L"Move to this location?","same movement confirmation reopens audibly");
    event(0x6583e0,0,[=]{set<uintptr_t>(pool+0x2704,address(0x656060));});
    expect(L"Yes","exit after movement still announces initial focus");
    check(said[said.size()-2].first==L"Quit Sphere Grid","callback identity distinguishes shared slot 11 confirmations");
    keys['I'](false); expect(L"Quit Sphere Grid","I reads actual exit prompt");
    setSetting(Setting::AutoDetail,0); set<int16_t>(pool+0x2700,-1);
    set<uintptr_t>(0x1f05834,address(grid)); set<int16_t>(grid+2,2);
    set<int16_t>(grid+0x80e,0); set<int16_t>(grid+0x80e+0x28,0);
    set<uint8_t>(grid+0x115bc,0); set<int16_t>(grid+0x11630,0); set<int16_t>(grid+0x110cc,1);
    set<uint8_t>(0xd3205c+0x3b,30);
    Event move; move.rva=0x656160; move.args[2]=0; move.args[3]=1;
    auto listener=listeners.at(move.rva).front(); listener.enter(move);
    set<uint8_t>(0xd3205c+0x3b,28); listener.leave(move);
    expect(L"Tidus moved to Strength Node, node 2. Sphere Levels 30 to 28","confirmed movement announces actual node and level change");
    check(said.back().second,"movement result becomes immediate latest speech");
    // No/cancel and unchanged position cannot claim movement.
    for(auto choice: {0u,1u}) {
        before=said.size(); move={}; move.rva=0x656160; move.args[2]=choice; move.args[3]=1;
        set<int16_t>(grid+0x110cc,0); listener.enter(move); listener.leave(move);
        check(said.size()==before,"No or unchanged position does not announce moved");
    }
    before=said.size(); move={}; move.args[3]=0; set<int16_t>(grid+0x110cc,1);
    listener.enter(move); listener.leave(move); check(said.size()==before,"cancel cannot announce moved");
    set<uint8_t>(grid+0x115bc,1); set<int16_t>(grid+0x1111c,1);
    move={}; move.args[3]=1; listener.enter(move); listener.leave(move);
    expect(L"Yuna moved to Strength Node, node 2","movement follows actual character position without inventing level cost");
    extendedAddresses[0x656160]=originalCallback;
    set<uint8_t>(0x1685f70,0); resetSphereGrid(); resetSelectionDetail();
    // Native movement-preview start, before the possibly long route animation.
    set<uint8_t>(0x1685f70,1); set<uintptr_t>(grid+0x115b0,0); set<int16_t>(grid+0x110cc,0);
    Event prepare; prepare.rva=0x648c80; prepare.args[0]=0; prepare.args[1]=1;
    auto starter=listeners.at(prepare.rva).front(); before=said.size(); starter.enter(prepare);
    check(said.size()==before+1 && said.back().first==L"Preparing to move" && said.back().second,"movement preview announces preparation immediately");
    before=said.size(); set<uintptr_t>(grid+0x115b0,0x1234); starter.enter(prepare);
    check(said.size()==before,"busy native preview does not repeat preparation");
    set<uintptr_t>(grid+0x115b0,0); prepare.args[1]=0; starter.enter(prepare);
    check(said.size()==before,"same node does not claim a new move");
    prepare.args[0]=7; prepare.args[1]=1; starter.enter(prepare);
    check(said.size()==before,"invalid character does not announce preparation");
    set<uint8_t>(0x1685f70,0); prepare.args[0]=0; starter.enter(prepare);
    check(said.size()==before,"closed grid does not announce preparation"); resetSphereGrid();
    std::puts("PASS: movement/exit native prompts, queued Yes/No, reopening, shifted callback and confirmed character destination/levels.");
}
}
