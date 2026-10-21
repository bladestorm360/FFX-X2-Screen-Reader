namespace {
uintptr_t __cdecl messageSubstitution(unsigned,uintptr_t);
uintptr_t __cdecl summonSubstitution(unsigned slot,uintptr_t token) {
    check(slot==0 && read<uint8_t>(token)==0x13,"summon help resolves the renderer's actor-name token");
    return names(read<uint8_t>(token+1)-0x30);
}
void battleSwitchFixtures() {
    const auto combatDetail=setting(Setting::CombatDetail),autoDetail=setting(Setting::AutoDetail);
    const auto maxHp=setting(Setting::ShowMaximumHp); setSetting(Setting::ShowMaximumHp,0);
    setSetting(Setting::CombatDetail,0); setSetting(Setting::AutoDetail,0);
    constexpr unsigned actor=0x2120000,reserve=actor+0xf90;
    const auto priorHp=read<int>(address(actor)+0x5d0),priorMp=read<int>(address(actor)+0x5d4);
    encoded(reserve+0x540,"Yuna");
    set<uint8_t>(actor+0x10,1); set<uint8_t>(reserve+0x10,1);
    set<uint8_t>(reserve+0xdc8,0);
    set<int>(actor+0x5d0,41); set<int>(actor+0x5d4,3);
    set<int>(reserve+0x5d0,321); set<int>(reserve+0x5d4,9);
    set<int>(reserve+0x594,500);
    // Saved field stats and the pre-effect cache deliberately disagree.
    set<int>(reserve+0x6e4,999); set<int>(reserve+0x6e8,888);
    const auto saved=0xd3205c+0x94;
    const auto savedHp=read<unsigned>(fixtureAddress(saved+0x1c)),savedMp=read<unsigned>(fixtureAddress(saved+0x20));
    set<unsigned>(saved+0x1c,777); set<unsigned>(saved+0x20,666);
    set<uint16_t>(0xf3c916,10); set<uint8_t>(0xf3c918,0);
    set<uintptr_t>(0xf3c930,address(0x2211400)); set<unsigned>(0xf3c934,2); set<unsigned>(0xf3c954,1);
    set<uint16_t>(0x2211400,0); set<uint16_t>(0x2211402,1); set<uint8_t>(0xf3c952,1);
    resetBattle(); auto spoken=said.size(); event(0x4a2100,address(0xf3c910));
    check(said.size()==spoken+1 && said.back().second,"switch highlight has one interrupting name and vitals utterance with detail settings off");
    expect(L"Yuna, HP 321 (64%), MP 9","switch highlight reads rendered reserve values and bar percentage rather than owner, saved data or pre-effect cache");
    keys['H'](false); expect(L"Tidus HP 41 (21%) MP 3. Status: None","H retains active-turn identity and committed values while Change is open");
    readSelectionDetail(); expect(L"HP 321 (64%), MP 9","manual selection details use the same current battle values");
    setSetting(Setting::ShowMaximumHp,1);
    check(battleSelectionVitals(1)==L"HP 321/500, MP 9","maximum number preference uses actual battle maximum, not saved HP");
    keys['H'](false); expect(L"Tidus HP 41/200 MP 3. Status: None","maximum HP preference leaves current-only MP unchanged");
    setSetting(Setting::ShowMaximumHp,0);
    setSetting(Setting::CombatDetail,1); setSetting(Setting::AutoDetail,1);
    spoken=said.size(); event(0x4a2100,address(0xf3c910),[]{set<uint8_t>(0xf3c952,0);});
    check(said.size()==spoken+1,"detail settings do not add a second switch-vitals producer");
    expect(L"Tidus, HP 41 (21%), MP 3","Change cursor movement uses highlighted row rather than window owner");
    set<int>(reserve+0x5d0,0); set<int>(reserve+0x5d4,0);
    event(0x4a2100,address(0xf3c910),[]{set<uint8_t>(0xf3c952,1);});
    expect(L"Yuna, HP 0 (0%), MP 0","zero displayed HP and MP retain the correct bar percentage");
    set<uint8_t>(reserve+0x10,0); set<uint8_t>(reserve+0x11,1);
    check(battleSelectionVitals(1).empty(),"initialization is the native byte flag, not neighboring actor data");
    set<uint8_t>(reserve+0x10,1); set<uint8_t>(reserve+0x11,0);
    const auto aeon=actor+8*0xf90;
    set<uint8_t>(aeon+0x10,1); set<int>(aeon+0x5d0,380); set<int>(aeon+0x5d4,37);
    set<int>(aeon+0x594,800);
    set<uint16_t>(0xf3c916,5); set<uint16_t>(0x2211400,8); set<uint8_t>(0xf3c952,0);
    encoded(0xf3f4f8,"Summon the aeon Valefor.");
    resetBattle(); spoken=said.size(); event(0x49cd50,address(0xf3c910));
    check(said.size()==spoken+2 && said[spoken].first==L"Valefor","ordinary Summon retains native name and configured automatic detail");
    expect(L"Summon the aeon Valefor. HP 380 (48%), MP 37","Summon detail includes native help and displayed statistics");
    setSetting(Setting::CombatDetail,0); setSetting(Setting::AutoDetail,0);
    keys['I'](false);
    expect(L"Summon the aeon Valefor. HP 380 (48%), MP 37","I includes help and statistics with automatic detail disabled");
    encoded(0xf3f4f8,"Fresh native summon help."); set<int>(aeon+0x5d4,36); keys['I'](false);
    expect(L"Fresh native summon help. HP 380 (48%), MP 36","I reads help and values freshly even after the highlight event");
    stub(0x4b6610,reinterpret_cast<void*>(summonSubstitution));
    encoded(0xf3f4f8,"Summon the aeon ");
    const auto token=address(0xf3f4f8)+16;
    write<uint8_t>(token,0x13); write<uint8_t>(token+1,0x38);
    write<uint8_t>(token+2,0x48); write<uint8_t>(token+3,0);
    keys['I'](false);
    expect(L"Summon the aeon Valefor. HP 380 (48%), MP 36","actual encoded name macro is retained in full help");
    encoded(0x2100040,"Renamed aeon"); keys['I'](false);
    expect(L"Summon the aeon Renamed aeon. HP 380 (48%), MP 36","renamed aeon help follows the native getter instead of a hardcoded name");
    encoded(0x2100040,"Valefor");
    stub(0x4b6610,reinterpret_cast<void*>(messageSubstitution));
    setSetting(Setting::CombatDetail,1); setSetting(Setting::AutoDetail,1);
    const char* names[]={"Cindy","Sandy","Mindy"};
    for(unsigned i=0;i<3;++i) {
        const auto member=actor+(15+i)*0xf90;
        encoded(member+0x540,names[i]); set<uint8_t>(member+0x10,1);
        set<int>(member+0x5d0,100+i); set<int>(member+0x5d4,20+i);
        set<int>(member+0x594,200);
    }
    encoded(0xf3f4f8,"Summon the Magus Sisters.");
    set<uint16_t>(0x2211402,0x30ff); event(0x49cd50,address(0xf3c910),[]{set<uint8_t>(0xf3c952,1);});
    check(said[said.size()-2].first==recordName(0x39a4c0,0x30ff),"special Summon group name uses the full native record ID");
    expect(L"Summon the Magus Sisters. Cindy, HP 100 (50%), MP 20. Sandy, HP 101 (51%), MP 21. Mindy, HP 102 (51%), MP 22","special Summon includes help and all three displayed actors");
    check(hpReadout(500,200)==L"HP 500 (100%)" && hpReadout(-2,200)==L"HP -2 (0%)","bar percentage stays within the native bar range");
    check(hpReadout(7,0)==L"HP 7","absent maximum does not invent a percentage");
    check(characterDescription(1).find(L"HP 777/")==0 && characterDescription(1).find(L", Sphere Levels ")!=std::wstring::npos,
        "saved field and shop descriptions retain their existing source and format");
    set<unsigned>(saved+0x1c,savedHp); set<unsigned>(saved+0x20,savedMp);
    set<uint16_t>(0xf3c916,0); set<uint8_t>(0xf3c952,0);
    setSetting(Setting::CombatDetail,combatDetail); setSetting(Setting::AutoDetail,autoDetail);
    setSetting(Setting::ShowMaximumHp,maxHp);
    set<int>(actor+0x5d0,priorHp);set<int>(actor+0x5d4,priorMp);
    resetBattle();
}
}
