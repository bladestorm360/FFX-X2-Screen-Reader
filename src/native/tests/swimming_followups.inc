namespace {
unsigned entranceSeen=999;
int __cdecl swimmingMapIndex(uintptr_t chunk,unsigned entrance) {
    check(chunk==address(0x2370000),"native map getter receives the current field chunk");
    entranceSeen=entrance; return entrance==1?20:19;
}
void swimmingFixtures() {
    constexpr unsigned actor=0x2240000,ctrl=0x2250000,worker=0x2251000;
    set<uintptr_t>(0x1fc44e4,address(actor)); set<unsigned>(0x1fc44e0,2); set<uintptr_t>(0xf00740,address(actor));
    set<uint16_t>(actor+2,1); set<uint8_t>(actor+0x186,2); set<uint8_t>(actor+0x182,0); set<int16_t>(actor+0x824,0);
    set<float>(actor+0xc,5); set<float>(actor+0x10,-50); set<float>(actor+0x14,5);
    set<int>(0xf01a48,3); set<uintptr_t>(0xf01a44,address(0x2360000)); set<uintptr_t>(0xf01a4c,address(0x2361000)); set<float>(0xf01a50,1);
    fixtureCollisionVertexCount(7);
    stub(0x43e4e0,reinterpret_cast<void*>(clearCollision));
    const int16_t triangleData[][8]={{0,1,2,1,-1,-1,0,0},{1,3,2,0,-1,-1,0,0},{4,5,6,-1,-1,-1,0,0}};
    const int16_t vertexData[][4]={{0,0,0,0},{100,0,0,0},{0,0,100,0},{100,0,100,0},
        {0,100,0,0},{100,100,0,0},{0,100,100,0}};
    memcpy(reinterpret_cast<void*>(address(0x2360000)),triangleData,sizeof(triangleData));
    memcpy(reinterpret_cast<void*>(address(0x2361000)),vertexData,sizeof(vertexData));
    set<uint8_t>(actor+0x185,0);set<uint8_t>(actor+0x182,2);
    auto mesh=readMesh();
    check(mesh.fixedHeight && !mesh.openPlane && mesh.triangles[0].valid && mesh.triangles[1].valid && !mesh.triangles[2].valid,
        "swimming projects only the native current collision component");
    auto route=routePoints(mesh,playerPosition(),{95,-50,95});
    check(!route.empty() && route.back().y==-50,"swimming can cross mesh triangles without snapping to the floor");
    set<uint8_t>(actor+0x186,0); mesh=readMesh();
    check(!mesh.fixedHeight && mesh.triangles[2].valid,"ordinary movement retains distinct floor heights");
    set<uint8_t>(actor+0x186,2); set<int>(0xf01a48,0); set<uintptr_t>(0xf01a44,0);
    check(readMesh().openPlane,"verified swimming without collision data uses native free movement");
    set<uint8_t>(actor+0x186,0); check(!readMesh().openPlane,"missing walking collision data cannot invent a route");
    set<uint8_t>(actor+0x186,2);
    set<uintptr_t>(0xf26ae8,address(ctrl)); set<uint8_t>(ctrl,2); set<uint16_t>(ctrl+0xa,0); set<uint16_t>(ctrl+0xc,1);
    set<uintptr_t>(worker+4,address(0x2371000)); // An unrelated worker's loaded chunk.
    set<uintptr_t>(ctrl+0x1f8,0); set<uintptr_t>(ctrl+0x48,address(0x2370000)); set<unsigned>(0x2370000+4,0x80); set<unsigned>(0x2370000+8,0xc0);
    set<uint16_t>(0x2370000+0x1e,0x8002); set<unsigned>(0x2370000+0x28,0xc0);
    set<uint16_t>(0x2370000+0xc0,19); set<uint16_t>(0x2370000+0xc2,20); set<uint16_t>(0xd2ca90+0xe,20);
    fixturePhysicalPart(0x2370000,0x1000);
    set<uint8_t>(0xd2ca90+0xc,1); set<uint16_t>(0x2314000,21*4);
    stub(0x46bd20,reinterpret_cast<void*>(swimmingMapIndex)); keys['M'](false);
    drainSpeechDomainsForTest();
    check(lastArea==20,"swimming map uses native published area, independent of geometry MapNo and arrival-point number");
    constexpr unsigned window=0x2330000,stream=0x2340000,cursor=0x2330200;
    resetMessages(); set<uintptr_t>(0x14676b0,address(window)); set<uintptr_t>(0x1468a50,address(cursor));
    set<uint16_t>(window+0x2c,2); set<uint16_t>(window+0x10,0); set<uintptr_t>(window+0x94,address(stream)); set<uint8_t>(cursor+1,0);
    set<uint8_t>(stream,9); set<uint8_t>(stream+1,0x30); encoded(stream+2,"Return to ship\nDive");
    const auto spoken=said.size(); event(0x4b8980,0);
    check(said.size()==spoken+1 && said.back().first==L"Return to ship Dive","swimming controls have one field HUD speech owner");
    check(!chords['T'].second(),"nonblocking swimming HUD cannot become dialogue replay text");
    check(fieldControlsCapturesKey(VK_DOWN,true) && dispatchFieldControlsKey(VK_DOWN,true),"Ctrl arrows review the visible native swimming controls");
    expect(L"Dive","swimming control buffer retains native line order");
    dispatchFieldControlsKey(VK_RIGHT,true); expect(L"Return to ship. Dive","Ctrl right reads all visible swimming controls");
    set<uint16_t>(window+0x2c,0); check(!fieldControlsCapturesKey(VK_DOWN,true),"closed swimming HUD releases its review keys");
    resetMessages();
    std::puts("PASS: native swimming collision component, height, free movement, entrance name and control prompt review.");
}
}
