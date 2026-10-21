namespace {
unsigned __cdecl swimMapCollision(uintptr_t,int index) {
    const auto tri=read<uintptr_t>(address(0xf01a44));
    const auto group=read<unsigned>(tri+index*16+12)&127;
    return group==2 || group==15; // Native 43E4E0 static player cases; swim gates open.
}
void realSwimmingFloor(const Entity& goal) {
    constexpr unsigned triangles=0x2500000,vertices=0x2510000,actor=0x2240000;
    const auto directory=(ffx_fixture::evidenceRoot()/"offline-output/open_issue_repairs_20261012/swim/cdsp00_0").string()+"/";
    auto load=[&](const char* name,unsigned destination,unsigned bytes) {
        std::ifstream file(std::string(directory)+name,std::ios::binary);check(file.good(),"original Surface floor fixture exists");
        file.read(reinterpret_cast<char*>(address(destination)),bytes);check(file.gcount()==bytes,"complete extracted native collision data");
    };
    load("triangles.bin",triangles,1739*16);load("vertices.bin",vertices,959*8);
    set<int>(0xf01a48,1739);set<uintptr_t>(0xf01a44,address(triangles));set<uintptr_t>(0xf01a4c,address(vertices));
    fixtureCollisionVertexCount(959);
    set<float>(0xf01a50,47.246630859375f);set<int16_t>(actor+0x824,347);
    set<float>(actor+0xc,-192);set<float>(actor+0x14,79);set<float>(actor+0x10,59);
    stub(0x43e4e0,swimMapCollision);const auto mesh=readMesh();
    check(mesh.swimmingFloor && mesh.floorY.size()==1739 && mesh.triangles[347].valid,"native Surface component preserves original floor depth while projecting swimming height");
    const auto route=navigationRoute(mesh,playerPosition(),goal);
    check(route.size()>=3 && route.back().y==251 && route.back().z<79 && distanceXZ(route.back(),playerPosition())>1,
        "real Surface routes south to sufficient depth before its actual Underwater map exit");
    const auto endpoint=route[route.size()-2]; const int at=findTriangle(mesh,endpoint);
    check(at>=0,"deep approach remains on the actual connected swimming floor");
    const auto& t=mesh.triangles[at]; const auto a=t.points[0],b=t.points[1],c=t.points[2];
    const double det=(b.z-c.z)*(a.x-c.x)+(c.x-b.x)*(a.z-c.z);
    const double u=((b.z-c.z)*(endpoint.x-c.x)+(c.x-b.x)*(endpoint.z-c.z))/det;
    const double v=((c.z-a.z)*(endpoint.x-c.x)+(a.x-c.x)*(endpoint.z-c.z))/det;
    const auto floor=u*mesh.floorY[at][0]+v*mesh.floorY[at][1]+(1-u-v)*mesh.floorY[at][2];
    check(floor>=252+goal.swimClearance-.001,"native interpolated endpoint floor allows the real depth threshold and body clearance");
    set<float>(actor+0x10,150);const auto dived=navigationRoute(mesh,playerPosition(),goal);
    if(dived.size()<3) std::fprintf(stderr,"cached swim route points: %zu\n",dived.size());
    check(dived.size()>=3 && std::abs(dived[1].y-150)<.000001 && dived.back().y==251,"cached Surface geometry still supplies a correct horizontal route after diving");
    auto shallow=mesh;for(auto& heights:shallow.floorY) heights={170,170,170};
    check(navigationRoute(shallow,playerPosition(),goal).empty(),"a shallow floor cannot invent an exit route by holding Dive in place");
    set<int>(0xf01a48,0);set<uintptr_t>(0xf01a44,0);stub(0x43e4e0,clearCollision);
    std::puts("PASS: original1739-triangle Surface geometry, actual shallow starting floor, connected southward deep-water route, Dive clearance and cached-height handling.");
}
}
