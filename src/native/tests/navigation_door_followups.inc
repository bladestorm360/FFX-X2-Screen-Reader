namespace {
unsigned doorState=1;
unsigned __cdecl collisionBlocked(uintptr_t player,int index) {
    check(player==address(0x2246000),"native collision getter uses controlled actor");
    const auto tri=read<uintptr_t>(address(0xf01a44));
    return (read<unsigned>(tri+index*16+12)&127)==53?doorState:0;
}
unsigned __cdecl collisionState(unsigned group) { check(group==53,"native door state getter group"); return doorState; }
void doorFixtures() {
    const auto oldActors=read<uintptr_t>(address(0x1fc44e4)),oldControlled=read<uintptr_t>(address(0xf00740));
    const auto oldCount=read<unsigned>(address(0x1fc44e0));
    const auto oldCollisionRoot=read<uintptr_t>(address(0xf01a54));
    const auto oldVertexCount=oldCollisionRoot?read<int16_t>(oldCollisionRoot+0xA):0;
    set<uintptr_t>(0x1fc44e4,address(0x2246000)); set<unsigned>(0x1fc44e0,1);
    set<uintptr_t>(0xf00740,address(0x2246000));
    stub(0x43e4e0,reinterpret_cast<void*>(collisionBlocked)); stub(0x43e960,reinterpret_cast<void*>(collisionState));
    constexpr unsigned tris=0x2330000,verts=0x2330100;
    set<int>(0xf01a48,2); set<uintptr_t>(0xf01a44,address(tris)); set<uintptr_t>(0xf01a4c,address(verts)); set<float>(0xf01a50,1);
    fixtureCollisionVertexCount(4);
    const int16_t points[][3]={{0,0,0},{10,0,0},{0,0,10},{10,0,10}};
    const int16_t records[][6]={{0,1,2,1,-1,-1},{1,3,2,0,-1,-1}};
    for(unsigned i=0;i<4;++i) for(unsigned j=0;j<3;++j) set<int16_t>(verts+i*8+j*2,points[i][j]);
    for(unsigned i=0;i<2;++i) for(unsigned j=0;j<6;++j) set<int16_t>(tris+i*16+j*2,records[i][j]);
    set<unsigned>(tris+12,0); set<unsigned>(tris+28,53); doorState=1; check(write<unsigned>(address(0xf01998)+53*4,1),"copied native blocked alias");
    auto mesh=readMesh(); check(path(mesh,0,1).empty(),"closed dynamic door blocks connected floor");
    Event opened; opened.args[0]=53; opened.args[1]=0;
    const auto collisionSerial=navCollisionSerial();
    auto& wall=listeners[0x43e9e0][0]; wall.enter(opened); doorState=0; check(write<unsigned>(address(0xf01998)+53*4,0),"copied native open alias"); wall.leave(opened);
    check(navCollisionSerial()==collisionSerial+1,"actual wall-change callback immediately retires copied movement serial");
    wall.enter(opened);wall.leave(opened);check(navCollisionSerial()==collisionSerial+1,"unchanged native wall setter does not trigger repeated replans");
    mesh=readMesh(); check(path(mesh,0,1).size()==2,"door mutation updates native passability without changing raw polygon group");
    check(mesh.triangles[1].collisionGroup==53,"live collision mesh retains the native mutable group identity");
    InteractionZone recess; recess.valid=true; recess.shape=1; recess.center={8,0,8}; recess.height=5; recess.range=2;
    check(!interactionRoute(mesh,{1,0,1},recess).empty(),"opened door exposes reachable wall recess");
    Entity exit; exit.category=L"Exit"; exit.position=recess.center; exit.zone=recess;
    check(!destinationUnreachable(mesh,{1,0,1},exit),"native open gate retains an enabled exit");
    doorState=1; check(write<unsigned>(address(0xf01998)+53*4,1),"copied native blocked alias"); mesh=readMesh(); check(interactionRoute(mesh,{1,0,1},recess).empty(),"closing door restores blocked recess without stale floor cache");
    check(destinationUnreachable(mesh,{1,0,1},exit),"filter verifies exit removal using live native gate state and retained group");
    doorState=0; check(write<unsigned>(address(0xf01998)+53*4,0),"copied native open alias"); mesh=readMesh(); check(!destinationUnreachable(mesh,{1,0,1},exit),"native reopening restores the enabled exit without cached validation");
    set<int>(0xf01a48,0); set<uintptr_t>(0xf01a54,oldCollisionRoot);
    if(oldCollisionRoot) check(write<int16_t>(oldCollisionRoot+0xA,static_cast<int16_t>(oldVertexCount)),"restore native collision vertex count");
    set<uintptr_t>(0x1fc44e4,oldActors); set<uintptr_t>(0xf00740,oldControlled); set<unsigned>(0x1fc44e0,oldCount);
}
}
