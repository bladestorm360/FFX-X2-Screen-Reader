namespace {
unsigned fixtureGil=16563;
unsigned __cdecl currentGil() { return fixtureGil; }
void activeBattleFixtures() {
    set<uint8_t>(0x2120000+0xf90+0xdc8,1);
    set<unsigned>(0x2120000+0xf90+0x6e4,999);
    keys['H'](false); drainSpeechDomainsForTest(); expect(L"Tidus HP 100 (50%) MP 17. Status: None","H never reads other live party slots");
    stub(0x395560,reinterpret_cast<void*>(overdriveCurrent)); stub(0x3955a0,reinterpret_cast<void*>(overdriveMaximum));
    set<uint8_t>(0x2120000+0x5bc,33); set<uint8_t>(0x2120000+0x5bd,150);
    check(chords['O'].second(),"battle O owns its context"); chords['O'].first(false);
    drainSpeechDomainsForTest(); expect(L"Tidus Overdrive 22%","O converts the native bar current/max ratio to percentage");
    set<uint8_t>(0x2120000+0x5bc,150); chords['O'].first(false); drainSpeechDomainsForTest(); expect(L"Tidus Overdrive 100%","full native gauge is 100 percent");
    set<uint8_t>(0x2120000+0x5bc,0); chords['O'].first(false); drainSpeechDomainsForTest(); expect(L"Tidus Overdrive 0%","empty native gauge is zero percent");
    const unsigned aeon=0x2120000+8*0xf90;
    encoded(aeon+0x540,"Valefor"); set<uint8_t>(aeon+0xdc8,1);
    set<unsigned>(aeon+0x5d0,400);set<unsigned>(aeon+0x6e4,9999); set<unsigned>(aeon+0x594,800);
    set<unsigned>(aeon+0x5d4,45);set<unsigned>(aeon+0x6e8,999); set<unsigned>(aeon+0x598,60);
    set<uint16_t>(aeon+0x606,8); set<uint8_t>(aeon+0x613,3);
    set<uint8_t>(0xf3c918,8); keys['H'](false); drainSpeechDomainsForTest();
    expect(L"Valefor HP 400 (50%) MP 45. Status: poison, haste","active summon HP MP and statuses use native turn owner");
    set<uint8_t>(aeon+0x5bc,25); set<uint8_t>(aeon+0x5bd,100); chords['O'].first(false); drainSpeechDomainsForTest();
    expect(L"Valefor Overdrive 25%","O reads the active summon rather than a party member");
    auto quiet=said.size(); set<uint8_t>(0xf3c918,18); keys['H'](false);
    check(said.size()==quiet,"enemy turn H is silent");
    chords['O'].first(false); check(said.size()==quiet,"enemy turn O is silent");
    set<uint8_t>(0xf3c918,0); set<uint8_t>(0xf3c911,0); chords['O'].first(false);
    check(said.size()==quiet,"no active command turn O is silent");
    keys['H'](false); check(said.size()==quiet,"no active command turn H is silent");
    set<uint8_t>(0xf3c911,4); set<uint8_t>(0x2120000+0xdc8,0); chords['O'].first(false);
    check(said.size()==quiet,"inactive character O is silent");
    keys['H'](false); check(said.size()==quiet,"inactive character H is silent");
    set<uint8_t>(0x2120000+0xdc8,1);
    set<uint8_t>(0x1fcc092,255); keys['H'](false);
    check(said.size()==quiet,"missing active battle window H is silent"); set<uint8_t>(0x1fcc092,0);
    set<uint8_t>(0x2120000+0x5bd,0); chords['O'].first(false); check(said.size()==quiet,"missing gauge maximum is silent");
    set<uint8_t>(0xd2a8e0,0); check(!chords['O'].second(),"field context cannot capture battle O"); chords['O'].first(false);
    check(said.size()==quiet,"outside battle O does not read any meter");
    keys['H'](false); check(said.size()==quiet,"outside battle H is silent"); set<uint8_t>(0xd2a8e0,1);
    set<uint8_t>(0xf3c918,0); set<uint8_t>(0x2120000+0xf90+0xdc8,0);
    for(unsigned kind=2;kind<=5;++kind) {
        const auto n=said.size(); Event e; e.args[0]=18; e.args[1]=kind; e.args[2]=static_cast<uintptr_t>(-1);
        listeners[0x39fa20][0].leave(e);
        drainSpeechDomainsForTest();
        if(kind==3) { check(said.size()==n+1,"true miss has one dedicated announcement"); const auto expected=L"Tidus: "+battleGraphicText(BattleGraphic::Miss); expect(expected.c_str(),"nonnumeric miss does not become a healing amount"); }
        else if(kind==4) { check(said.size()==n+1,"accepted guard graphic has one dedicated announcement"); const auto expected=L"Tidus: "+battleGraphicText(BattleGraphic::Guard); expect(expected.c_str(),"guard uses the native locale-specific label"); }
        else check(said.size()==n,"overkill and gauge graphics are never healing amounts");
    }
    { const auto n=said.size(); Event rejected; rejected.args[0]=18; rejected.args[1]=4; rejected.result=1;
      listeners[0x39fa20][0].leave(rejected); drainSpeechDomainsForTest();
      check(said.size()==n,"rejected graphic registration remains silent"); }
    Event hpDamage; hpDamage.args[2]=1; listeners[0x39fa20][0].leave(hpDamage); drainSpeechDomainsForTest();
    expect(L"Tidus: 1 damage","one HP damage is still a valid numeric event");
    hpDamage.args[2]=static_cast<uintptr_t>(-1); listeners[0x39fa20][0].leave(hpDamage); drainSpeechDomainsForTest();
    expect(L"Tidus: healed for 1","real one HP restoration remains valid");
    keys['G'](false); expect(L"16563 Gil","G uses current native wallet getter");
}
void phasedResultsFixtures() {
    constexpr unsigned result=0x1f10ea0;
    std::memset(reinterpret_cast<void*>(address(result)),0,0x1b0);
    set<uint8_t>(result,1); set<unsigned>(result+0x38,20); set<unsigned>(result+0x80,300);
    set<unsigned>(result+0xcc,618); set<uint8_t>(result+0xd0,1);
    set<uint16_t>(result+0xd4,1); set<uint8_t>(result+0xe4,2);
    set<uint8_t>(result+0xd1,1); set<uint16_t>(result+0xec,7);
    set<uint8_t>(result+0xd2,1); set<uint8_t>(0xd3205c+0x3b,2);
    resetBattle();set<uint8_t>(0x1469ed8,1);write<int16_t>(address(0x1469ee6),0);
    const int automatic=setting(Setting::AutoDetail);setSetting(Setting::AutoDetail,1);
    const auto n=said.size();event(0x4bdb30,0);set<int>(0x1469ec8,0xc);event(0x4ba620,0);
    check(said.size()==n+2,"first results phase automatically reads its displayed AP buffer");
    expect(L"Tidus. AP earned: 300. Tidus. AP: 300. Sphere Level: 2. Next AP: 151","AP earned and current displayed native amount are retained");
    auto count=said.size();event(0x4ba620,0);check(said.size()==count,"idle controller does not reread the reward page");
    set<unsigned>(0x1469ed4,618);set<int>(0x1469ec8,0x18);event(0x4ba620,0);
    check(said.back().first.find(L"Gil: 618. Total Gil: 16563. Potion x2. Key")!=std::wstring::npos,"second phase buffers gil and native rewards");
    count=said.size();event(0x4ba620,0);check(said.size()==count,"native gil chunks do not repeat phase announcement");
    resetBattle();event(0x4bdb30,0);set<int>(0x1469ec8,0xc);event(0x4ba620,0);
    check(said.size()==count+2,"next result event preserves identical rewards as a separate page event");
    resetBattle();count=said.size();event(0x4ba620,0);check(said.size()==count,"ordinary controller work outside results cannot announce rewards");
    setSetting(Setting::AutoDetail,automatic);
}
}
