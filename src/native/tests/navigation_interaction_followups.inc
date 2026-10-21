namespace {
uint16_t __cdecl interactionEntry(unsigned type,unsigned channel,uintptr_t table) {
    if(table) return read<uint16_t>(table+channel*2,0xffff);
    const uint16_t actor[]={2,3,4,5,0xffff,0xffff,0xffff};
    const uint16_t zone[]={2,3,0xffff,5,4,6,7};
    return channel<7?(type==1?actor[channel]:zone[channel]):0xffff;
}
uint8_t __cdecl interactionPriority(unsigned type,unsigned channel) {
    return channel>=7 || (type==1?channel>=4:channel==2)?255:1;
}
void prepareInteractionFixture(unsigned worker,unsigned controller) {
    stub(0x476360,reinterpret_cast<void*>(interactionEntry));
    stub(0x4767e0,reinterpret_cast<void*>(interactionPriority));
    set<uint16_t>(worker+0x34,0x80); set<uint8_t>(worker+0xaa,1); set<uint8_t>(worker+0xab,1);
    set<float>(worker+0x558+0x3c,3.1415927f);
    set<float>(worker+0x558,120); set<float>(worker+0x558+0x2c,50);
    set<float>(worker+0x558+0x40,35); set<float>(controller+0x1f0,15);
}
uintptr_t facingActor=0; float facingAngle=999;
void __cdecl nativeFace(uintptr_t actor,float angle) { facingActor=actor; facingAngle=angle; }
unsigned __cdecl clearCollision(uintptr_t,int) { return 0; }
void navigationInteractionFixtures() {
    constexpr unsigned worker=0x2251000,npc=0x2240880,header=0x2252200;
    const auto initialEntryCount=read<uint16_t>(address(header+8)); set<uint16_t>(header+8,8);
    set<uint16_t>(npc+2,1); set<uint8_t>(worker+0xab,1);
    unsigned channels=0;
    check(interactionEntries(address(worker),4,&channels)==std::vector<unsigned>{2} && channels==1,
        "native talk flag maps to useful entry 2");
    auto zone=interactionZone(address(worker),channels);
    check(zone.valid && zone.range==50 && zone.shape==1,"native talk zone includes controlled character reach");
    check(!inInteractionZone(zone,{70,0,0}) && inInteractionZone(zone,{70.2,0,0}),"strict native range excludes its outer boundary");
    set<float>(0x2240000+0xc,80); set<uintptr_t>(0x1f11430,address(0x2311000)); set<float>(0x2311040,1);
    keys[VK_OEM_5](false); expect(L"Villager: within interaction range. Face north","Backslash gives camera-relative facing from inside the strict native talk zone");
    set<float>(0x2311040,.381f); set<float>(0x2311048,-.925f);
    keys[VK_OEM_5](false); expect(L"Villager: within interaction range. Face west","oblique facing speaks only the compass direction without a degree suffix");
    set<float>(0x2311040,1); set<float>(0x2311048,0);
    stub(0x42b230,reinterpret_cast<void*>(nativeFace));
    const auto controlled=read<uintptr_t>(address(0xf00740)); set<uintptr_t>(0xf00740,address(0x2240000));
    setSetting(Setting::AutoWalk,1); keys[VK_OEM_5](false);navigationFixturePump();
    check(facingActor==address(0x2240000) && std::abs(facingAngle)<.001,"already-in-range auto walk uses the native facing setter on the controlled character");
    Entity pushTarget; pushTarget.worker=7;
    configurePedestalTarget(pushTarget,"bsyt0100",7);
    const unsigned radiusFields[]={0x5c,0x64,0x4e4}; float playerBounds[3],targetBounds[3];
    for(unsigned i=0;i<3;++i) {
        playerBounds[i]=read<float>(address(0x2240000+radiusFields[i]));
        targetBounds[i]=read<float>(address(npc+radiusFields[i]));
        set<float>(0x2240000+radiusFields[i],i==2?1.f:4.f);
        set<float>(npc+radiusFields[i],i==2?1.f:3.f);
    }
    set<uint8_t>(worker+0xab,5);
    refreshTargetZone(pushTarget,address(worker),1);
    check(pushTarget.pushControl==1 && !pushTarget.inactive && pushTarget.zone.contactRadius==7 && !pushTarget.zone.talk,
        "physical push target reads summed native radii and the enabled contact channel");
    set<uint8_t>(worker+0xab,1); refreshTargetZone(pushTarget,address(worker),1);
    check(pushTarget.inactive,"an active confirm channel cannot claim pedestal pushing is enabled");
    configurePedestalTarget(pushTarget,"bsyt0100",44);
    check(pushTarget.pushControl==0,"the Besaid sphere recess remains an ordinary talk target");
    configurePedestalTarget(pushTarget,"unknown",7);
    check(pushTarget.pushControl==0,"unverified script identity cannot receive pedestal handling");
    configurePedestalTarget(pushTarget,"klyt1000",6); set<uint8_t>(worker+0xab,5);
    refreshTargetZone(pushTarget,address(worker),1);
    check(pushTarget.pushControl==2 && pushTarget.zone.contactRadius==7,"Kilika uses the same physical push geometry");
    for(unsigned i=0;i<3;++i) {
        set<float>(0x2240000+radiusFields[i],playerBounds[i]); set<float>(npc+radiusFields[i],targetBounds[i]);
    }
    set<uint8_t>(worker+0xab,1);
    setSetting(Setting::AutoWalk,0); set<uintptr_t>(0xf00740,controlled);
    stub(0x43e4e0,reinterpret_cast<void*>(clearCollision));
    const auto oldCount=read<int>(address(0xf01a48));
    const auto oldTriangles=read<uintptr_t>(address(0xf01a44)),oldVertices=read<uintptr_t>(address(0xf01a4c));
    const auto oldScale=read<float>(address(0xf01a50));
    const auto oldCollisionRoot=read<uintptr_t>(address(0xf01a54));
    const auto oldVertexCount=oldCollisionRoot?read<int16_t>(oldCollisionRoot+0xA):0;
    set<int>(0xf01a48,1); set<uintptr_t>(0xf01a44,address(0x2350000)); set<uintptr_t>(0xf01a4c,address(0x2351000)); set<float>(0xf01a50,1);
    fixtureCollisionVertexCount(3);
    const int16_t triangle[]={0,1,2,-1,-1,-1},vertices[]={0,0,0,0,100,0,0,0,0,0,100,0};
    memcpy(reinterpret_cast<void*>(address(0x2350000)),triangle,sizeof(triangle));
    memcpy(reinterpret_cast<void*>(address(0x2351000)),vertices,sizeof(vertices));
    set<float>(0x2240000+0xc,0); set<float>(worker+0x558,20); set<float>(worker+0x558+0x40,5);
    set<float>(0x2250000+0x1f0,0); setSetting(Setting::AutoWalk,1); setSetting(Setting::AutoWalkPath,0);
    auto spoken=said.size(); auto starts=autoStarts; keys[VK_OEM_5](false);navigationFixturePump();
    check(said.size()==spoken && autoStarts==starts+1,"path narration off suppresses directions and still starts walking");
    setSetting(Setting::AutoWalkPath,1); keys[VK_OEM_5](false);navigationFixturePump();
    check(said.size()>spoken && autoStarts==starts+2,"path narration on retains spoken directions before starting");
    setSetting(Setting::AutoWalk,0); setSetting(Setting::AutoWalkPath,0); spoken=said.size(); keys[VK_OEM_5](false);navigationFixturePump();
    check(said.size()>spoken && autoStarts==starts+2,"ordinary path requests always speak when auto walk is off");
    set<int>(0xf01a48,oldCount); set<uintptr_t>(0xf01a44,oldTriangles); set<uintptr_t>(0xf01a4c,oldVertices); set<float>(0xf01a50,oldScale);
    set<uintptr_t>(0xf01a54,oldCollisionRoot);
    if(oldCollisionRoot) check(write<int16_t>(oldCollisionRoot+0xA,static_cast<int16_t>(oldVertexCount)),"restore native collision vertex count");
    set<float>(worker+0x558,120); set<float>(worker+0x558+0x40,35); set<float>(0x2250000+0x1f0,15);
    set<float>(npc+0xc,60000); set<float>(worker+0x558,60000);
    std::vector<Entity> distant; scanExits({},distant);
    check(distant.size()==1 && distant[0].position.x==60000,"finite native coordinates beyond 50000 remain discoverable");
    set<float>(worker+0x558+0x40,20000);
    check(interactionZone(address(worker),1).valid,"finite native interaction ranges beyond 10000 remain valid");
    set<float>(npc+0xc,120); set<float>(worker+0x558,120); set<float>(worker+0x558+0x40,35);
    set<float>(0x2240000+0xc,0); set<uintptr_t>(0x1f11430,0);
    event(0x45c140,address(worker),[=]{set<uint8_t>(worker+0xab,0);});
    keys[VK_OEM_5](false); expect(L"No entity selected","disabled story talk disappears on native flag mutation");
    event(0x45bf70,address(worker),[=]{set<uint8_t>(worker+0xab,1);});
    keys[VK_OEM_5](false);navigationFixturePump(); check(said.back().first.find(L"collision data unavailable")!=std::wstring::npos,
        "native enabling event restores useful interaction");
    set<uintptr_t>(worker+0x44,address(0x2253000)); set<uint16_t>(0x2253000,3);
    check(interactionEntries(address(worker),4).empty(),"current request remapping cannot activate an unrelated/no-op catalog entry");
    const auto originalCount=read<uint16_t>(address(header+8)); set<uint16_t>(header+8,64);
    set<uint16_t>(0x2253000,40); EntryMask late{0u}; late.words[1]=1u<<8;
    check(interactionEntries(address(worker),late)==std::vector<unsigned>{40},"remapped useful callback above 31 remains available");
    check(interactionEntries(address(worker),4).empty(),"late no-op callback is still excluded by its actual catalog mask");
    set<uint16_t>(header+8,originalCount);
    set<uintptr_t>(worker+0x44,0); set<uint16_t>(worker+0x34,0x280);
    check(interactionEntries(address(worker),4).empty(),"native actor talk inhibition suppresses inaccessible party NPC");
    set<uint16_t>(worker+0x34,0x80); set<uint8_t>(header,2); set<uint8_t>(worker+0xab,0x10);
    check(interactionEntries(address(worker),0x10)==std::vector<unsigned>{4},"line exit uses enabled crossing channel 4");
    navigationRefreshFixtures(worker,header,npc);
    set<uint16_t>(worker+0x34,0x180);
    check(interactionEntries(address(worker),0x10)==std::vector<unsigned>{4},"inside-rectangle movement history cannot remove a live exit");
    set<uint16_t>(worker+0x34,0x80);
    check(interactionEntries(address(worker),4).empty(),"an exit's no-op talk channel cannot keep a disabled crossing listed");
    set<uint8_t>(worker+0xab,0); check(interactionEntries(address(worker),0x10).empty(),"disabled story exit is omitted");
    set<uint8_t>(header,1); set<uint8_t>(worker+0xab,1); set<uint16_t>(npc,0x5001);
    set<uintptr_t>(worker+0x44,address(0x2253000)); set<uint16_t>(0x2253000,3);
    std::vector<Entity> saves; scanExits({},saves);
    check(saves.size()==1 && saves[0].category==L"Save Sphere","native save sphere survives a remapped interaction callback");
    set<uint16_t>(npc,0x50ab); saves.clear(); scanExits({},saves);
    check(saves.size()==1 && saves[0].category==L"Save Sphere" && saves[0].name==L"Save Sphere",
        "underwater save sphere retains save category/name and remapped callbacks");
    set<uint8_t>(worker+0xab,0); saves.clear(); scanExits({},saves);
    check(saves.empty(),"a genuinely disabled sphere is still removed by live state");
    set<uint8_t>(worker+0xab,1); set<uint16_t>(npc,0x5002); set<uint16_t>(0x2253000,2);
    saves.clear(); scanExits({},saves); check(saves.size()==1 && saves[0].category==L"Treasure Chest","native chest remains available before collection");
    set<uint16_t>(0x2253000,7); saves.clear(); scanExits({},saves);
    check(saves.empty(),"same-count collected chest disappears through live callback remapping");
    set<uintptr_t>(worker+0x44,0); set<uint16_t>(npc,0x2002); set<uint16_t>(npc+2,0);
    set<uint16_t>(header+8,initialEntryCount);
    navigationGuardFixtures(worker);
    resetNavigation();
}
void visibleEquipmentFixtures() {
    constexpr unsigned piece=0x2101040;
    set<uint8_t>(piece+0xb,1); set<uint16_t>(piece+0xe,1); set<uint16_t>(piece+0x10,2);
    check(equipmentDetails(address(piece))==L"Potion, Ether","all four renderer slots are scanned even before native normalization");
    set<uint8_t>(piece+4,7); check(shopEquipmentDescription(address(piece)).empty(),"invalid comparison owner cannot narrate hidden data");
    set<uint8_t>(piece+4,0); set<uint8_t>(0xd3205c+0x2d,255);
    check(shopEquipmentDescription(address(piece))==L"Tidus. HP 100/200, MP 17/30. Current equipment: None. Selected equipment: Potion, Ether","empty current panel has explicit context");
    set<uint8_t>(0x146ab60,1);
    set<unsigned>(0xd3205c+0x94+0x1c,520); set<unsigned>(0xd3205c+0x94+0x24,520);
    set<unsigned>(0xd3205c+0x94+0x20,12); set<unsigned>(0xd3205c+0x94+0x28,12);
    set<uint8_t>(0xd3205c+0x94+0x2d,254);
    check(shopEquipmentDescription(address(piece))==L"Yuna. HP 520/520, MP 12/12. Current equipment: Sonic Steel, Potion. Selected equipment: Potion, Ether","shop describes the displayed comparison character rather than the selected gear owner");
    set<uint8_t>(0x146ab60,0);
    set<uint8_t>(0xd3205c+0x2d,254); set<uint8_t>(piece+0xb,2);
    set<uint16_t>(piece+0xe,2); set<uint16_t>(piece+0x10,0xff);
}
}
