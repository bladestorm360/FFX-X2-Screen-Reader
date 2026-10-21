// Included by reader_fixture.cpp: exercise the production listeners and stubs.
namespace {
uint8_t __cdecl fieldParty(unsigned row) { return static_cast<uint8_t>(row?0:6); }
uint8_t __cdecl invalidFieldParty(unsigned) { return 255; }
uintptr_t __cdecl customizedName(unsigned) { return address(0x2105900); }
uintptr_t __cdecl liveCustomizeRecord(unsigned id,uintptr_t*) {return id>=0x5000 && id<0x5004?address(0x2106000)+(id-0x5000)*22:0;}
uintptr_t __cdecl liveCustomizeName(unsigned id) {return address(id==0x5002?0x2106200:id==0x5003?0x2106240:0x2106280);}
void fieldMenuFixtures() {
    setSetting(Setting::AutoDetail,0); setSetting(Setting::ShowMaximumHp,0);
    std::memset(reinterpret_cast<void*>(address(win)),0,0x100);
    scene=2; set<int>(win+0x28,1); set<int16_t>(win+0x48,0);
    set<int>(win+0x58,0); set<int>(win+0x5c,0); set<uint8_t>(0x146a351,1);
    stub(0x4a9a80,fieldParty);
    set<unsigned>(0xd3205c+6*0x94+0x1c,75); set<unsigned>(0xd3205c+6*0x94+0x24,300);
    event(0x4c7400,address(win),[]{set<int>(win+0x28,4);});
    expect((playerName(6)+L": HP 75 (25%)").c_str(),"spell initial target uses saved native party ID and percentage");
    auto n=said.size(); event(0x4c7400,address(win),[]{set<int>(win+0x28,5);});
    check(said.size()==n,"initial-to-active target phase has one producer");
    event(0x4c7400,address(win),[]{set<unsigned>(0xd3205c+6*0x94+0x1c,150);});
    expect((playerName(6)+L": HP 150 (50%)").c_str(),"same target refreshed after native heal event");
    event(0x4c7400,address(win),[]{set<int16_t>(win+0x48,1);});
    check(said.back().first.find(playerName(0)+L": HP ")==0,"spell target follows actual native order");
    event(0x4c7400,address(win),[]{set<uint8_t>(0x146a351,0);});
    check(said.back().first.find(L"All party members. "+playerName(6)+L": HP 150 (50%). "+playerName(0))==0,
        "all-party spell target keeps native ordering");
    setSetting(Setting::ShowMaximumHp,1);
    event(0x4c7400,address(win),[]{set<uint8_t>(0x146a351,1); set<int16_t>(win+0x48,0);});
    expect((playerName(6)+L": HP 150/300").c_str(),"field HP maximum preference uses central formatter");
    scene=3; set<unsigned>(0xd3205c+6*0x94+0x20,9); set<unsigned>(0xd3205c+6*0x94+0x28,18);
    event(0x4c7400,address(win),[]{set<int>(win+0x5c,1);});
    expect((playerName(6)+L": MP 9/18").c_str(),"item MP-only renderer semantics preserved");
    event(0x4c7400,address(win),[]{set<int>(win+0x28,9);});
    event(0x4c7400,address(win),[]{set<int>(win+0x28,4);});
    expect((playerName(6)+L": MP 9/18").c_str(),"target reopening retains current resource values");
    stub(0x4a9a80,invalidFieldParty); n=said.size();
    event(0x4c7400,address(win),[]{set<int16_t>(win+0x48,1);});
    check(said.size()==n,"invalid native party ID is rejected");
    stub(0x4a9a80,fieldParty);
    n=said.size(); scene=4; event(0x4c7400,address(win),[]{set<int16_t>(win+0x48,1);});
    check(said.size()==n,"unrelated menu cannot produce field target vitals");
    stub(0x4a9a80,orderedParty); setSetting(Setting::ShowMaximumHp,0);

    scene=9; set<int16_t>(win+0x48,0); set<uint16_t>(win+0x30,1);
    set<uintptr_t>(0x146a9f8,address(0x2105980)); set<uint16_t>(0x2105980,0);
    stub(0x3abe10,customizedName); encoded(0x2105900,"Old gear name");
    set<int>(0x146aa28,12); n=said.size();
    event(0x4d5800,address(win),[]{set<int>(0x146aa28,6);});
    check(said.size()==n,"customization cancel does not announce a changed name");
    set<int>(0x146aa28,12);
    event(0x4d5800,address(win),[]{encoded(0x2105900,"Fresh customized name"); set<int>(0x146aa28,10);});
    expect(L"Fresh customized name","successful customization reads fresh name despite unchanged ID and row");
    n=said.size(); event(0x4d5800,address(win));
    check(said.size()==n,"customization result idle does not repeat");
    scene=4; set<int>(0x146aa28,12);
    event(0x4d5800,address(win),[]{set<int>(0x146aa28,10);});
    check(said.size()==n,"unrelated scene does not announce customization");
    // Original 7ABD30 retains empty slots in the native table; 8D6730 hides
    // them by ownership/flag, even though their old names still exist.
    scene=9;stub(0x3abbf0,liveCustomizeRecord);stub(0x3abe10,liveCustomizeName);
    encoded(0x2106200,"Current armor");encoded(0x2106240,"Current weapon");encoded(0x2106280,"Sold old weapon");
    std::memset(reinterpret_cast<void*>(address(0x2106000)),0,88);
    set<uint8_t>(0x2106000+2*22+2,1);set<uint8_t>(0x2106000+3*22+2,1);
    set<uint8_t>(0x2106000+3*22+3,2);
    set<int>(win+0x28,1);set<int8_t>(win+0x45,0);set<uint16_t>(win+0x30,3);
    set<uintptr_t>(win+0x10,address(0x4d6830));set<int16_t>(win+0x48,0);
    set<uint16_t>(0x2105980,0x5000);set<uint16_t>(0x2105982,0x5002);set<uint16_t>(0x2105984,0x5003);
    event(0x4b4460,address(win),[]{set<int>(win+0x28,2);});expect(L"Empty","sold slot cannot read its retained old equipment name");
    check(readSelectionDetail(),"sold slot owns its empty detail context");
    expect(L"No native description is available for this selection.","sold slot retires stale equipment details too");
    event(0x4b4460,address(win),[]{set<int16_t>(win+0x48,1);});expect(L"Current armor","Customize resolves current row through the native table");
    event(0x4b4460,address(win),[]{set<int16_t>(win+0x48,2);});expect(L"Empty","native hidden gear remains undisplayed");
    event(0x4b4460,address(win),[]{set<uint8_t>(0x2106000+3*22+3,4);});expect(L"Current weapon","visible uncustomizable gear is retained, matching the native renderer");
    event(0x4b4460,address(win),[]{set<uint16_t>(0x2105984,0x5002);});expect(L"Current armor","same row follows a rebuilt inventory index after sale");
    stub(0x3abbf0,gearRecord);
    stub(0x3abe10,slotName);
    scene=0;
}
}
