// Exercise the production event readers with native catalog/getter substitutes.
namespace {
unsigned sphereFixtureQuantity=99;
uintptr_t __cdecl sphereCatalogRecord(unsigned id,uintptr_t* strings) {
    id&=0xfff;
    const int row=id>=0x46 && id<=0x4a?static_cast<int>(id-0x46):
        id>=0x51 && id<=0x54?static_cast<int>(id-0x51+5):-1;
    if(row<0) return 0;
    if(strings) *strings=address(0x2342000+row*0x40);
    return address(0x2341000+row*0x80);
}
uintptr_t __cdecl sphereCatalogDefinition(unsigned id,uintptr_t table,uintptr_t* strings) {
    if(table==address(0x2310b00)) return id<9?address(0x2343000+id*0x20):0;
    if(id>=12) return 0;
    if(strings) *strings=address(0x2345000+id*0x80);
    return address(0x2344000+id*0x20);
}
unsigned __cdecl sphereCatalogQuantity(unsigned id) { return (id&0xfff)==0x46?sphereFixtureQuantity:0; }
void sphereReaderFixtures() {
    const char* materials[]={"Power Sphere","Mana Sphere","Speed Sphere","Ability Sphere","Fortune Sphere",
        "Lv 1 Key Sphere","Lv 2 Key Sphere","Lv 3 Key Sphere","Lv 4 Key Sphere"};
    const unsigned masks[]={5,2,8,0x400,0x10,0,0,0,0};
    for(unsigned row=0;row<9;++row) {
        encoded(0x2342000+row*0x40,materials[row]);
        set<int8_t>(0x2341000+row*0x80+0x5d,static_cast<int8_t>(row));
        set<uint16_t>(0x2343000+row*0x20+0xa,static_cast<uint16_t>(masks[row]));
        set<uint8_t>(0x2343000+row*0x20+0xd,static_cast<uint8_t>(row<5?0:row-4));
    }
    stub(0x390a40,sphereCatalogRecord); stub(0x3ab890,sphereCatalogDefinition); stub(0x390500,sphereCatalogQuantity);
    constexpr unsigned inventory=pool+8*0x340;
    set<uintptr_t>(0x1686108,address(pool)); set<uint8_t>(inventory+0x23,1);
    set<int16_t>(inventory+0x18,0); set<int16_t>(inventory+0x1e,2);
    set<uint16_t>(inventory+0x48,0x2046); set<uint16_t>(inventory+0x54,0x2047);
    for(unsigned owned: {99u,98u}) {
        resetSphereGrid(); sphereFixtureQuantity=owned;
        set<int16_t>(pool+0x2700,8); set<uintptr_t>(pool+0x2704,address(0x6560d0));
        set<uint8_t>(inventory+0x44,0);
        auto before=said.size(); event(0x6598a0,0);
        check(said.size()==before+1 && said.back().first==L"Power Sphere, "+std::to_wstring(owned),
            "enabled Power Sphere at 99 or 98 never announces unavailable");
        before=said.size(); event(0x6583e0,0); check(said.size()==before,"inventory idle remains silent");
    }
    set<uint8_t>(inventory+0x50,1); set<int16_t>(inventory+0x18,1);
    auto before=said.size(); event(0x6583e0,0);
    check(said.size()==before+2 && said[before].first==L"Mana Sphere, 0" && said.back().first==L"Unavailable",
        "native disabled entry announces unavailability after its name and quantity");
    check(!said.back().second,"unavailability queues after the selected sphere");
    constexpr unsigned grid=0x2320000;
    std::memset(reinterpret_cast<void*>(address(grid)),0,0x12000);
    set<uintptr_t>(0x1f05834,address(grid)); set<uintptr_t>(0x16860e0,address(0x2310400));
    set<uintptr_t>(0x16860e4,address(0x2310b00)); set<int16_t>(grid+2,12);
    set<int16_t>(pool+0x2700,-1); set<uint8_t>(0x1685f70,1);
    const char* names[]={"Strength Node","HP Node","Magic Node","Agility Node","Cheer","Luck Node",
        "Lock 1","Lock 2","Lock 3","Lock 4","Empty","Strength Node"};
    const unsigned flags[]={4,1,2,8,0x400,0x10,0x8000,0x8000,0x8000,0x8000,0,4};
    const unsigned types[]={2,2,2,2,2,2,0x12,0x11,0,0x10,1,2};
    const unsigned expected[]={0,0,1,2,3,4,5,6,7,8};
    resetSphereGrid(); setSetting(Setting::AutoDetail,0);
    for(int key:{VK_F2,VK_F3,VK_F4,VK_F5,static_cast<int>('O'),static_cast<int>('S')}) check(!contexts.at(key)(),"grid shortcuts have no closed-grid context");
    for(unsigned row=0;row<12;++row) {
        encoded(0x2345000+row*0x80,names[row]);
        set<uint16_t>(0x2344000+row*0x20+0x10,static_cast<uint16_t>(flags[row]));
        set<uint8_t>(0x2344000+row*0x20+0x16,static_cast<uint8_t>(types[row]));
        set<int16_t>(grid+0x80e+row*0x28,static_cast<int16_t>(row));
    }
    GridSnapshot snapshot; check(sphereSnapshot(snapshot,true),"snapshot with native sphere catalog");
    for(unsigned row=0;row<10;++row) {
        set<int16_t>(grid+0x112fc,static_cast<int16_t>(row)); event(0x648d70,0);
        const unsigned material=expected[row];
        const auto name=recordName(0x390a40,material<5?0x2046+material:0x2051+material-5);
        if(said.back().first!=snapshot.nodes[row].name+L", requires 1 "+name+L", node "+std::to_wstring(row+1)) std::fwprintf(stderr,L"Sphere row %u actual [%s] expected name [%s] material [%s]\n",row,said.back().first.c_str(),snapshot.nodes[row].name.c_str(),name.c_str());
        check(said.back().first==snapshot.nodes[row].name+L", requires 1 "+name+L", node "+std::to_wstring(row+1),
            "stat, ability, fortune and every lock node announce the native material without owning it");
        check(sphereNode(static_cast<int>(row),true).sphere==snapshot.nodes[row].sphere,
            "node announcements and planner share material resolution");
    }
    set<int16_t>(grid+0x112fc,10); event(0x648d70,0); expect(L"Empty, node 11","empty nodes have no activation material");
    set<uint8_t>(grid+0x829+11*0x28,1); set<int16_t>(grid+0x112fc,11); event(0x648d70,0);
    expect(L"Strength Node, activated for Tidus, node 12","activated node does not request another sphere");
    event(0x4a9810,0,[=]{set<uint8_t>(grid+0x115bc,1);});
    expect(L"Strength Node, requires 1 Power Sphere, node 12","material requirement follows active character activation bits");
    for(int key:{VK_F2,VK_F3,VK_F4,VK_F5,static_cast<int>('O'),static_cast<int>('S')}) check(contexts.at(key)(),"native grid selection establishes all grid shortcut contexts");
    const int assist=setting(Setting::OverdriveAssist); keys['O'](false);
    check(setting(Setting::OverdriveAssist)==assist,"O orientation cannot toggle Overdrive auto-success");
    set<uint8_t>(0x1685f70,0); event(0x653de0,0);
    check(!contexts.at('O')() && !contexts.at(VK_F2)(),"native closing event releases grid key contexts");
    resetSphereGrid(); resetSelectionDetail();
    stub(0x390a40,record); stub(0x3ab890,nodeDefinition); stub(0x390500,qty);
}
}
