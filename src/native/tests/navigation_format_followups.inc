namespace {
unsigned __cdecl formatFloorBlocked(uintptr_t,int) { return 0; }
uintptr_t guardedVertexPage=0;unsigned guardedVertexFaults=0;
LONG CALLBACK vertexFaultObserver(PEXCEPTION_POINTERS exception) {
    if(exception->ExceptionRecord->ExceptionCode==EXCEPTION_ACCESS_VIOLATION &&
       exception->ExceptionRecord->NumberParameters>=2 && exception->ExceptionRecord->ExceptionInformation[1]==guardedVertexPage) ++guardedVertexFaults;
    return EXCEPTION_CONTINUE_SEARCH;
}
void navigationFormatFixtures() {
    const auto bent=routeDirectionsText({{0,0,0},{10,0,10},{15,0,20}},1,true);
    check(bent.find(L", ")!=std::wstring::npos,"same-octant bend keeps its segment instead of one false straight distance");
    const auto straight=routeDirectionsText({{0,0,0},{10,0,10},{20,0,20}},1,true);
    check(straight.find(L", ")==std::wstring::npos,"collinear subdivisions retain concise combined distance");
    check(join({routeSegmentText(.6,L"south",true),routeSegmentText(1.4,L"west",true),routeSegmentText(2.3,L"north",true)})==L"0.6 south, 1.4 west, 2.3 north","requested partial path wording has no repeated unit word");
    check(join({routeSegmentText(.6,L"south",false),routeSegmentText(1.4,L"west",false),routeSegmentText(2.3,L"north",false)})==L"1 south, 1 west, 2 north","whole path numbers round without changing direction order");
    check(routeSegmentText(.4,L"west",false)==L"1 west","whole path distances have a minimum of one rather than a confusing zero");
    constexpr unsigned triangles=0x2315000,vertices=0x2315100;
    const auto oldCount=read<int>(address(0xf01a48));
    const auto oldTriangles=read<uintptr_t>(address(0xf01a44)),oldVertices=read<uintptr_t>(address(0xf01a4c)),oldCamera=read<uintptr_t>(address(0x1f11430));
    const auto oldScale=read<float>(address(0xf01a50));
    const auto oldCollisionRoot=read<uintptr_t>(address(0xf01a54));
    const auto oldVertexCount=oldCollisionRoot?read<int16_t>(oldCollisionRoot+0xA):0;
    const int16_t indices[]={0,1,2,-1,-1,-1};
    const int16_t coords[]={-1000,0,-1000,0,1000,0,-1000,0,0,0,1000,0};
    memcpy(reinterpret_cast<void*>(address(triangles)),indices,sizeof(indices));
    memcpy(reinterpret_cast<void*>(address(vertices)),coords,sizeof(coords));
    set<unsigned>(triangles+12,0); set<int>(0xf01a48,1);
    set<uintptr_t>(0xf01a44,address(triangles)); set<uintptr_t>(0xf01a4c,address(vertices)); set<float>(0xf01a50,1);
    fixtureCollisionVertexCount(3);
    stub(0x43e4e0,reinterpret_cast<void*>(formatFloorBlocked));
    set<uintptr_t>(0x1f11430,address(0x2311000)); set<float>(0x2311040,1);
    setSetting(Setting::PartialSteps,0); keys[VK_OEM_5](false);
    expect(L"6 north","strict interaction route uses old concise whole wording by default");
    setSetting(Setting::PartialSteps,1); keys[VK_OEM_5](false);
    expect(L"6.3 north","same strict interaction route switches to concise decimal wording");
    setSetting(Setting::PartialSteps,0);
    // Actual protected fixture page proves no attempt to read vertex3 beyond
    // the native three-vertex bound; a readable large fake image cannot mask it.
    guardedVertexPage=address(0x25f2000);guardedVertexFaults=0;
    const auto guardedVertices=guardedVertexPage-sizeof(coords);
    std::memcpy(reinterpret_cast<void*>(guardedVertices),coords,sizeof(coords));
    set<uintptr_t>(0xf01a4c,guardedVertices);set<int16_t>(triangles+4,3);
    DWORD priorProtection=0;
    const auto observer=AddVectoredExceptionHandler(1,vertexFaultObserver);check(observer!=nullptr,"fixture vertex fault observer");
    check(VirtualProtect(reinterpret_cast<void*>(guardedVertexPage),4096,PAGE_NOACCESS,&priorProtection)!=0,"protect fixture page beyond exact vertices");
    const auto malformed=readMesh();
    check(malformed.triangles.size()==1 && !malformed.triangles[0].valid && guardedVertexFaults==0,"out-of-range vertex cannot enlarge native reads");
    set<int>(0xf01a48,32768);check(readMesh().triangles.empty(),"native signed-short triangle bound");set<int>(0xf01a48,1);
    fixtureCollisionVertexCount(0);check(readMesh().triangles.empty(),"nonempty floor requires exact native vertices");fixtureCollisionVertexCount(3);
    DWORD restored=0;check(VirtualProtect(reinterpret_cast<void*>(guardedVertexPage),4096,priorProtection,&restored)!=0,"restore fixture page protection");
    check(RemoveVectoredExceptionHandler(observer)!=0,"remove fixture fault observer");guardedVertexPage=0;
    set<int16_t>(triangles+4,2);
    set<int>(0xf01a48,oldCount); set<uintptr_t>(0xf01a44,oldTriangles); set<uintptr_t>(0xf01a4c,oldVertices);
    set<uintptr_t>(0xf01a54,oldCollisionRoot);
    if(oldCollisionRoot) check(write<int16_t>(oldCollisionRoot+0xA,static_cast<int16_t>(oldVertexCount)),"restore native collision vertex count");
    set<float>(0xf01a50,oldScale); set<uintptr_t>(0x1f11430,oldCamera);
}
}
