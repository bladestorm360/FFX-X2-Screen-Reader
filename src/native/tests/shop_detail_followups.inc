// Production shop events/TextPlan receiver/I/U, with controlled native getter
// boundaries matching the user's screenshot. No game or personal save is read.
namespace {
unsigned shopCharacterReads=0;
uintptr_t __cdecl shopSavedCharacter(unsigned id) {
    check(testStatusOwner,"shop character getter stays on Native");++shopCharacterReads;
    return savedCharacter(id);
}
uintptr_t __cdecl shopEquipmentName(unsigned id,unsigned,int,uintptr_t) {
    check(testStatusOwner,"shop gear-name getter stays on Native");
    return address(id==0x7a01?0x240b000:0x21000c0);
}
uintptr_t __cdecl shopPrice(unsigned) {return 825;}
void shopDetailFixtures() {
    constexpr unsigned window=0x2217000,piece=0x2101040,currentPiece=0x2310800;
    const auto priorScene=scene.load();const int priorAuto=setting(Setting::AutoDetail),priorExtra=setting(Setting::AutoExtraInfo);
    std::array<uint8_t,0x40> oldPiece{},oldCurrent{};std::array<uint8_t,0x128> oldCharacters{};
    std::array<uint8_t,0xa0> oldText{};
    std::memcpy(oldPiece.data(),reinterpret_cast<void*>(address(piece)),oldPiece.size());
    std::memcpy(oldCurrent.data(),reinterpret_cast<void*>(address(currentPiece)),oldCurrent.size());
    std::memcpy(oldCharacters.data(),reinterpret_cast<void*>(fixtureAddress(0xd3205c)),oldCharacters.size());
    std::memcpy(oldText.data(),reinterpret_cast<void*>(address(0x2100060)),oldText.size());
    encoded(0x2100060,"First Strike");encoded(0x2100080,"Strength +5%");encoded(0x240b000,"Baroque Sword");
    stub(0x3853f0,shopSavedCharacter);stub(0x3a0c70,shopEquipmentName);
    stub(0x4d9140,gearRecord);
    stub(0x4d9340,shopPrice);stub(0x4d93e0,shopPrice);
    set<uint16_t>(piece,0x7a01);set<uint8_t>(piece+4,0);set<uint8_t>(piece+5,0);set<uint8_t>(piece+0xb,2);
    for(unsigned i=0;i<4;++i) set<uint16_t>(piece+0xe+i*2,i?0xff:2);
    set<uint16_t>(currentPiece,0x7a00);set<uint8_t>(currentPiece+0xb,1);
    for(unsigned i=0;i<4;++i) set<uint16_t>(currentPiece+0xe+i*2,i?0xff:1);
    set<unsigned>(0xd3205c+0x1c,520);set<unsigned>(0xd3205c+0x24,520);
    set<unsigned>(0xd3205c+0x20,12);set<unsigned>(0xd3205c+0x28,12);set<uint8_t>(0xd3205c+0x2d,254);
    set<unsigned>(0xd3205c+0x94+0x1c,475);set<unsigned>(0xd3205c+0x94+0x24,475);
    set<unsigned>(0xd3205c+0x94+0x20,84);set<unsigned>(0xd3205c+0x94+0x28,84);set<uint8_t>(0xd3205c+0x94+0x2d,254);
    set<uintptr_t>(0x146ab68,address(0x2110000));set<uint16_t>(0x2110000,2);
    set<uint16_t>(0x2110002,1);set<uint16_t>(0x2110004,2);
    set<uint16_t>(0x1197730,1);set<uint16_t>(0x1197734,2);
    set<uint16_t>(window+0x30,2);set<int8_t>(window+0x45,0);scene=0x15;
    const std::wstring primary=L"Strength +5%, 1 empty slot. Compared character: Tidus";
    const std::wstring body=L"HP 520/520, MP 12/12. Current equipment: Sonic Steel, First Strike";
    const std::wstring extra=L"Compared character: Tidus. "+body;
    double nativeTotal=0,nativeMaximum=0,usableTotal=0;
    const auto manualExtra=[&] {
        const auto found=chords.find('U');check(found!=chords.end() && found->second.second(),"shop U owns only its visible current context");
        found->second.first(false);drainSpeechDomainsForTest();
    };
    for(unsigned buy=0;buy<2;++buy) for(unsigned automatic=0;automatic<2;++automatic) for(unsigned extras=0;extras<2;++extras) {
        const unsigned rva=buy?0x4d77c0:0x4d7990;
        setSetting(Setting::AutoDetail,automatic);setSetting(Setting::AutoExtraInfo,extras);
        set<uint8_t>(0x146ab60,0);set<int>(window+0x28,1);set<int16_t>(window+0x48,0);
        const auto before=said.size();event(rva,address(window),[=]{set<int>(window+0x28,2);});
        nativeTotal+=fixtureNativeEventMs;nativeMaximum=std::max(nativeMaximum,fixtureNativeEventMs);usableTotal+=fixtureUsableEventMs;
        check(said.size()==before+1+automatic+extras,"buy/sell automatic primary and extra output are independent");
        check(said[before].first==L"Baroque Sword, 825 Gil","native item name and price stay first");
        if(automatic) check(said[before+1].first==primary,"Autodetail orders selected modifiers before compared character without HP/current gear");
        if(extras) check(said.back().first==(automatic?body:extra),"automatic extra pane follows primary and includes only its displayed context");
        const auto getters=shopCharacterReads;
        for(unsigned repeat=0;repeat<2;++repeat) {
            auto count=said.size();keys['I'](false);expect(primary.c_str(),"I repeats primary while either automatic setting is On or Off");
            check(said.size()==count+1,"each manual I remains a separate valid output");
            count=said.size();manualExtra();expect(extra.c_str(),"U repeats the visible extra pane independent of Autodetail");
            check(said.size()==count+1,"each manual U remains a separate valid output");
        }
        check(shopCharacterReads==getters,"manual I/U use owned pane text without reentering character getters");
        event(rva,address(window),[=]{set<int>(window+0x28,0);});
        check(!chords['U'].second(),"leaving the list retires its extra pane and restores ordinary U ownership");
    }
    setSetting(Setting::AutoDetail,1);setSetting(Setting::AutoExtraInfo,1);
    set<uint8_t>(0x146ab60,0);set<int>(window+0x28,1);
    event(0x4d77c0,address(window),[=]{set<int>(window+0x28,2);});
    event(0x4d77c0,address(window),[]{set<uint8_t>(0x146ab60,1);});
    keys['I'](false);expect(L"Strength +5%, 1 empty slot. Compared character: Yuna","primary uses displayed comparison character, not selected gear owner");
    manualExtra();expect(L"Compared character: Yuna. HP 475/475, MP 84/84. Current equipment: Sonic Steel, First Strike","U follows the native comparison owner and its vitals");
    const auto getters=shopCharacterReads;
    event(0x4d77c0,address(window),[]{set<uint8_t>(0x146ab60,255);});
    keys['I'](false);expect(L"Strength +5%, 1 empty slot","hidden comparison owner is absent from the primary description");
    manualExtra();expect(L"No extra information is displayed for this selection.","U never reads an undisplayed pane");
    check(shopCharacterReads==getters,"hidden pane never requests character values");
    event(0x4d77c0,address(window),[]{set<uint8_t>(0x146ab60,0);});
    const auto beforeStale=said.size();chords['U'].first(false);
    selectionDetail(L"Different screen",false);drainSpeechDomainsForTest();
    check(said.size()==beforeStale && !chords['U'].second(),"queued U is canceled when another selected context takes ownership");
    event(0x4d77c0,address(window),[=]{set<int>(window+0x28,0);});
    std::memcpy(reinterpret_cast<void*>(address(piece)),oldPiece.data(),oldPiece.size());
    std::memcpy(reinterpret_cast<void*>(address(currentPiece)),oldCurrent.data(),oldCurrent.size());
    std::memcpy(reinterpret_cast<void*>(fixtureAddress(0xd3205c)),oldCharacters.data(),oldCharacters.size());
    std::memcpy(reinterpret_cast<void*>(address(0x2100060)),oldText.data(),oldText.size());
    stub(0x3853f0,savedCharacter);stub(0x3a0c70,gear);stub(0x4d9340,price);stub(0x4d93e0,price);
    set<uint8_t>(0x146ab60,0);scene=priorScene;setSetting(Setting::AutoDetail,priorAuto);setSetting(Setting::AutoExtraInfo,priorExtra);
    std::puts("PASS: native buy/sell shop primary/modifier/character order, all automatic-setting combinations, independent I/U repeats, displayed-only extra pane and stale context cancellation.");
    std::printf("METRIC shop eight selection episodes native aggregate=%.3fms max=%.3fms capture-to-owned-output aggregate=%.3fms; controlled getter/queue host, not game/frame/backend/audible responsiveness.\n",nativeTotal,nativeMaximum,usableTotal);
}
}
