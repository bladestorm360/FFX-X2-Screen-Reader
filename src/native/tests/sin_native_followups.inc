// Original mod adapter; EBP and native geometry remain separately retained.
// Native getters/worker storage are controlled, not a live game observation.
namespace {
void sinNativeFixtures() {
    constexpr unsigned chunk=0x21c0000,controller=0x2430000,workers=0x2440000;
    const auto oldController=read<uintptr_t>(address(0xf26ae8));const auto oldRoom=read<uint16_t>(address(0xd2ca90));
    const auto file=ffx_fixture::evidenceRoot()/"offline-output/FFX/20261003_audit/files/ffx_ps2/ffx/master/jppc/event/obj/si/sins0200/sins0200.ebp";
    std::ifstream source(file,std::ios::binary);check(source.good(),"original sins0200 input exists");
    std::vector<uint8_t> bytes((std::istreambuf_iterator<char>(source)),{});unsigned offsets[11];std::memcpy(offsets,bytes.data()+4,sizeof(offsets));
    unsigned end=static_cast<unsigned>(bytes.size());for(unsigned i=1;i<11;++i) if(offsets[i]>offsets[0] && offsets[i]<end) end=offsets[i];
    check(offsets[0]==64 && end-64<0x40000,"original Sin chunk extent");std::memcpy(reinterpret_cast<void*>(address(chunk)),bytes.data()+64,end-64);
    fixturePhysicalPart(chunk,end-64);const auto script=readScript(address(chunk));check(script.name=="sins0200" && script.checksum==0xb5ed9bb4,"exact Sin allocation identity");
    ZeroMemory(reinterpret_cast<void*>(address(controller)),0x238);set<uintptr_t>(controller+0x48,address(chunk));set<uint16_t>(0xd2ca90,203);
    set<uintptr_t>(0xf26ae8,address(controller));stub(0x46a7c0,airshipWorker);stub(0x476360,interactionEntry);stub(0x4767e0,interactionPriority);
    airshipWorkers.clear();for(unsigned i=0;i<script.workers.size();++i) {
        const auto w=workers+i*0x1000;airshipWorkers.push_back(address(w));ZeroMemory(reinterpret_cast<void*>(address(w)),0x1000);
        set<uintptr_t>(w,address(chunk+script.workers[i].header));set<uintptr_t>(w+4,address(chunk));set<uint16_t>(w+0x2e,static_cast<uint16_t>(i));
    }
    std::ifstream facts(ffx_fixture::evidenceRoot()/"offline-output/sin_routes_20261020/events.txt");
    for(unsigned i=0;i<4;++i) {
        DirectedCrossing e;facts>>e.a.x>>e.a.y>>e.a.z>>e.b.x>>e.b.y>>e.b.z>>e.landing.x>>e.landing.y>>e.landing.z>>e.height;
        const auto w=workers+(17+i)*0x1000,g=w+0x558;set<uint16_t>(w+0x34,0x80);set<uint8_t>(w+0xaa,1);set<uint8_t>(w+0xab,16);
        set<float>(g,static_cast<float>(e.a.x));set<float>(g+4,static_cast<float>(e.a.y));set<float>(g+8,static_cast<float>(e.a.z));
        set<float>(g+0x30,static_cast<float>(e.b.x));set<float>(g+0x34,static_cast<float>(e.b.y));set<float>(g+0x38,static_cast<float>(e.b.z));set<float>(g+0x1c,40);
    }check(!facts.fail(),"retained Sin edge facts with controlled height");
    const auto captured=nav::captureRouteEvents(address(controller));check(captured.ready && captured.count==4,"actual prepared native capture retains four active ledges");
    nativeHeapFixture::allocations=nativeHeapFixture::frees=0;const auto began=std::chrono::steady_clock::now();nativeHeapFixture::observing=true;
    bool captures=true;for(unsigned i=0;i<1000;++i) {const auto next=nav::captureRouteEvents(address(controller));captures=captures && next.ready && next.count==4;}
    nativeHeapFixture::observing=false;
    check(captures && !nativeHeapFixture::allocations && !nativeHeapFixture::frees,"four-ledged ready capture has no native heap work");
    std::printf("METRIC Sin ready Native capture1000 %.3fms, heap0/0; controlled getter adapters, no game/backend/audible claim.\n",std::chrono::duration<double,std::milli>(std::chrono::steady_clock::now()-began).count());
    testStatusOwner=false;check(!nav::captureRouteEvents(address(controller)).ready,"foreign owner cannot enter route-event getters");testStatusOwner=true;
    check(captured.rows[2].landing.x==91.16837310791016 && captured.rows[2].a.x>128,"script direction and current geometry agree");
    for(unsigned bits=0;bits<16;++bits) {
        unsigned count=0;for(unsigned i=0;i<4;++i) {const bool enabled=(bits&(1u<<i))!=0;count+=enabled;set<uint8_t>(workers+(17+i)*0x1000+0xab,enabled?16:0);}
        const auto active=nav::captureRouteEvents(address(controller));check(active.ready && active.count==count,"all16 native enabled subsets retire/reinstate exactly their own constraints");
        check(nav::sameRouteEvents(active,captured)==(bits==15),"cache compatibility includes native event changes");
    }
    const auto w=workers+19*0x1000;set<uint16_t>(w+0x34,0);check(nav::captureRouteEvents(address(controller)).count==3,"disabled native worker removes its edge");set<uint16_t>(w+0x34,0x80);
    const auto table=address(0x23b0000);for(unsigned i=0;i<7;++i) write<uint16_t>(table+i*2,0xffff);write<uint16_t>(table+8,5);set<uintptr_t>(w+0x44,table);
    check(nav::captureRouteEvents(address(controller)).count==3,"remap to return-only entry retires the native constraint");set<uintptr_t>(w+0x44,0);
    const auto originalX=read<float>(address(w+0x558));set<float>(w+0x558,originalX+1);
    check(!nav::sameRouteEvents(captured,nav::captureRouteEvents(address(controller))),"moved edge invalidates routing without changed floor bytes");set<float>(w+0x558,originalX);
    set<uintptr_t>(w,address(chunk+script.workers[18].header));check(!nav::captureRouteEvents(address(controller)).ready,"foreign worker header cannot supply a false route");set<uintptr_t>(w,address(chunk+script.workers[19].header));
    set<uintptr_t>(controller+0x48,address(chunk+16));check(!nav::captureRouteEvents(address(controller)).ready,"pending/changed resource cannot reuse stale event facts");set<uintptr_t>(controller+0x48,address(chunk));
    // Actual copied intention -> Native guard. Keep player/target/floor fixed
    // and change only one callback between preparation and movement dispatch.
    const auto oldPlayer=read<uintptr_t>(address(0xf00740)),oldActors=read<uintptr_t>(address(0x1fc44e4));const auto oldCapacity=read<unsigned>(address(0x1fc44e0));
    constexpr unsigned actor=0x23c0000;set<uintptr_t>(0xf00740,address(actor));set<uintptr_t>(0x1fc44e4,address(actor));set<unsigned>(0x1fc44e0,1);
    set<uint16_t>(actor,1);set<uint16_t>(actor+2,1);set<float>(actor+0xc,91.1683731f);set<float>(actor+0x10,14.1166182f);set<float>(actor+0x14,-40.3403664f);
    set<uint8_t>(controller,2);set<uint16_t>(controller+0xc,48);const auto saved=address(0xd2ca90)+0x1ec;set<uintptr_t>(controller+0x2c,saved);
    static nav::Scene intent;intent.controller=address(controller);intent.primary=address(chunk);intent.epoch=generation();intent.partCount=1;
    check(nativeAtelSpan(address(chunk),intent.parts[0].span),"Sin intention owns a ready original resource");intent.saved.valid=1;std::memcpy(intent.saved.bytes.data(),reinterpret_cast<void*>(saved),intent.saved.bytes.size());
    intent.geometry.events=captured;intent.player=playerPosition();Entity point;point.position={0,0,100};point.name=L"Fixture Sin goal";
    fixtureRoute=true;fixtureLeg={};fixtureLeg.target.worker=-1;fixtureLeg.final=point.position;nav::startNavigationIntents();
    const auto queue=[&] {intent.capturedAt=GetTickCount64();intent.collisionSerial=navCollisionSerial();testStatusOwner=false;const bool admitted=nav::submitNavigationIntent(intent,point,nav::Intent::Start,0,nav::navigationIntentRevision());testStatusOwner=true;check(admitted,"copied Sin movement intention admitted");};
    const auto starts=autoStarts;queue();nav::dispatchNavigationIntents();check(autoStarts==starts+1,"unchanged Sin callback facts permit guarded movement start");
    queue();set<uint8_t>(w+0xab,0);nav::dispatchNavigationIntents();check(autoStarts==starts+1,"changed Sin callback cancels queued start before Native movement");set<uint8_t>(w+0xab,16);
    queue();navCollisionRevision.fetch_add(2);nav::dispatchNavigationIntents();check(autoStarts==starts+1,"gate change and return cancel queued start with unchanged final state");
    queue();nav::dispatchNavigationIntents();check(autoStarts==starts+2,"fresh collision serial permits a new guarded start");
    fixtureRoute=false;set<uintptr_t>(0xf00740,oldPlayer);set<uintptr_t>(0x1fc44e4,oldActors);set<unsigned>(0x1fc44e0,oldCapacity);
    static nav::Scene noFloor;noFloor.geometryValid=true;noFloor.geometry.swimming=true;noFloor.geometry.events=captured;
    testStatusOwner=false;const auto missing=nav::decodeNavigationSceneMesh(noFloor);testStatusOwner=true;
    check(!missing.events.ready && routePoints(missing,{0,0,0},{10,0,10}).empty(),"actual scene decoder cannot replace missing qualified Sin floor with an open plane");
    set<uint16_t>(0xd2ca90,322);const auto other=nav::captureRouteEvents(address(controller));check(other.ready && !other.kind && !other.count,"other room never inherits Sin constraints");
    set<uintptr_t>(0xf26ae8,oldController);set<uint16_t>(0xd2ca90,oldRoom);stub(0x46a7c0,navigationWorker);
    std::puts("PASS: original Sin native ownership, all16 ledge states, disable/remap/movement/resource/room guards and cache identity. Native motion remains user-side.");
}
}
