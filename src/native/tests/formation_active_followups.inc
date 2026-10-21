namespace {
std::array<uint8_t,8> formationRows{};
std::array<uint8_t,3> formationRoster{};
uint8_t __cdecl fixtureFormationRow(int row) {
    return row>=0 && row<8?formationRows[row]:255;
}
uintptr_t __cdecl fixtureFormationParty(int* count) {
    if(count) *count=3;
    return address(0x24b0000);
}
void formationActiveFixtures() {
    const int oldScene=scene.load();
    const auto oldDetail=setting(Setting::AutoDetail);
    const auto oldPhase=read<int>(address(0x2200000)+0x1c,-1);
    const auto oldMainPhase=read<int>(address(0x1471508),-1);
    const auto oldSecondPhase=read<int>(address(0x147150c),-1);
    const auto oldFirstRow=read<int>(address(0x147151c),-1);
    const auto oldSecondRow=read<int>(address(0x1471520),-1);
    uint8_t priorRoster[3]{};
    check(copy(address(0x24b0000),priorRoster,sizeof(priorRoster)),"formation roster fixture memory is readable");
    const auto priorRows=formationRows;

    // This fixture owns only the selection getter stub; the roster function
    // stub is installed by the host at 385330 and returns the three bytes above.
    stub(0x4a9ab0,reinterpret_cast<void*>(fixtureFormationRow));
    formationRows={0,1,2,1,0,1,2,0};
    formationRoster={0,1,3};
    check(write<uint8_t>(address(0x24b0000),formationRoster[0]) &&
          write<uint8_t>(address(0x24b0001),formationRoster[1]) &&
          write<uint8_t>(address(0x24b0002),formationRoster[2]),"formation roster bytes are installed");
    set<int>(0x1471508,0x0e);set<int>(0x147150c,0x14);
    set<int>(0x147151c,0);set<int>(0x1471520,0);
    set<int>(win+0x1c,5);setSetting(Setting::AutoDetail,0);scene=2;
    auto expectedName=playerName(0);
    event(0x4e0340,address(win),[]{set<int>(win+0x1c,0x0e);});
    check(said.back().first==expectedName+L", active party","first front-party member is identified as active");
    event(0x4e0340,address(win),[]{set<int>(win+0x1c,0x14);});
    check(said.back().first==expectedName+L", active party","swap phase reports actual front-party membership");
    set<int>(win+0x1c,0x0e);
    keys['I'](false);const auto description=said.back().first;
    check(!description.empty() && description!=expectedName+L", active party","Formation I retains its native description");

    // A roster swap at an unchanged cursor and character must refresh the label.
    event(0x4e0340,address(win),[]{
        formationRoster={2,1,3};
        for(unsigned i=0;i<3;++i) set<uint8_t>(0x24b0000+i,formationRoster[i]);
    });
    check(said.back().first==expectedName+L", reserve","same selection refreshes when native party membership changes");
    keys['I'](false);check(said.back().first==description,"membership label does not replace native manual description");

    // The fourth UI row is active because membership comes from native roster,
    // even though that row index lies beyond the three active positions.
    event(0x4e0340,address(win),[]{set<int>(0x147151c,3);});
    check(said.back().first==playerName(1)+L", active party","non-first-three menu row follows actual native membership");
    event(0x4e0340,address(win),[]{
        formationRoster={0,255,255};for(unsigned i=0;i<3;++i)set<uint8_t>(0x24b0000+i,formationRoster[i]);
        set<int>(0x147151c,2);
    });
    check(said.back().first==playerName(2)+L", reserve","empty roster entries remain valid and do not invent a member");
    event(0x4e0340,address(win),[]{
        formationRoster={0,20,3};for(unsigned i=0;i<3;++i)set<uint8_t>(0x24b0000+i,formationRoster[i]);
    });
    check(said.back().first==playerName(2),"invalid native roster ID omits active/reserve classification");

    // Restore the exact fixture-owned state and its existing party getter.
    stub(0x4a9ab0,party);
    for(unsigned i=0;i<3;++i)set<uint8_t>(0x24b0000+i,priorRoster[i]);
    formationRows=priorRows;
    set<int>(win+0x1c,oldPhase);set<int>(0x1471508,oldMainPhase);set<int>(0x147150c,oldSecondPhase);
    set<int>(0x147151c,oldFirstRow);set<int>(0x1471520,oldSecondRow);
    setSetting(Setting::AutoDetail,oldDetail);scene=oldScene;
}
}
