// Four exact original City touch callbacks, through actual native capture and
// owned/legacy discovery. Geometry/script inputs remain externally retained;
// scheduler, getters and shape enablement are controlled component boundaries.
namespace {
void sinPlatformFixtures() {
    constexpr unsigned chunk=0x21c0000,controller=0x2430000,workerBase=0x2440000,actor=0x23c0000;
    const auto oldController=read<uintptr_t>(address(0xf26ae8)),oldActors=read<uintptr_t>(address(0x1fc44e4)),oldPlayer=read<uintptr_t>(address(0xf00740));
    const auto oldCount=read<unsigned>(address(0x1fc44e0));const auto oldRoom=read<uint16_t>(address(0xd2ca90));
    std::ifstream input(ffx_fixture::evidenceRoot()/"offline-output/FFX/20261003_audit/files/ffx_ps2/ffx/master/jppc/event/obj/si/sins0400/sins0400.ebp",std::ios::binary);
    check(input.good(),"original City script exists");std::vector<uint8_t> ebp((std::istreambuf_iterator<char>(input)),{});
    unsigned offsets[11];std::memcpy(offsets,ebp.data()+4,sizeof(offsets));unsigned end=static_cast<unsigned>(ebp.size());
    for(unsigned i=1;i<11;++i) if(offsets[i]>offsets[0] && offsets[i]<end) end=offsets[i];
    check(offsets[0]==64 && end-64<0x40000,"City allocation extent");std::memcpy(reinterpret_cast<void*>(address(chunk)),ebp.data()+64,end-64);
    fixturePhysicalPart(chunk,end-64);const auto script=readScript(address(chunk));const auto catalog=navigationCatalog(script);
    check(catalog && script.name=="sins0400" && script.checksum==0x01aaeb94 && script.workers.size()==100,"exact original City catalog/resource");
    const unsigned locals[]={42,43,59,60};
    const wchar_t* labels[]={L"First moving platform, stop 1",L"First moving platform, stop 2",L"Second moving platform, stop 1",L"Second moving platform, stop 2"};
    for(unsigned i=0;i<4;++i) {const auto target=navigationTarget(catalog,locals[i]);check(target && target->category==6 && target->identity==2 && target->entries.words==EntryMask(0x10).words && std::wcscmp(target->label,labels[i])==0,"exact touch-only platform target identity");}
    auto foreign=script;foreign.checksum^=1;check(!navigationCatalog(foreign),"foreign checksum cannot inherit platform stops");
    ZeroMemory(reinterpret_cast<void*>(address(controller)),0x238);set<uintptr_t>(controller+0x48,address(chunk));set<uint16_t>(controller+0xc,100);set<uint8_t>(controller,2);
    set<uintptr_t>(controller+0x2c,address(0xd2ca90)+0x1ec);set<uint16_t>(0xd2ca90,204);set<uintptr_t>(0xf26ae8,address(controller));
    set<uintptr_t>(0x1fc44e4,address(actor));set<unsigned>(0x1fc44e0,1);set<uintptr_t>(0xf00740,address(actor));
    ZeroMemory(reinterpret_cast<void*>(address(actor)),0x880);set<uint16_t>(actor,1);set<uint16_t>(actor+2,1);
    stub(0x46a7c0,airshipWorker);stub(0x476360,interactionEntry);stub(0x4767e0,interactionPriority);
    airshipWorkers.clear();for(unsigned i=0;i<100;++i) {
        const auto worker=workerBase+i*0x1000;airshipWorkers.push_back(address(worker));ZeroMemory(reinterpret_cast<void*>(address(worker)),0x1000);
        set<uintptr_t>(worker,address(chunk+script.workers[i].header));set<uintptr_t>(worker+4,address(chunk));set<uint16_t>(worker+0x2e,static_cast<uint16_t>(i));
    }
    // Skip externally retained entrance/exit/chest facts and read the four
    // original Common0057/0059/005A boarding rectangles, never copied bytecode.
    std::ifstream facts(ffx_fixture::evidenceRoot()/"offline-output/sin_later_maps_20261020/city_facts_v2.txt");
    double ignored=0;unsigned count=0;facts>>ignored>>count;for(unsigned i=0;i<count*3;++i) facts>>ignored;
    facts>>count;for(unsigned i=0;i<count*9;++i) facts>>ignored;
    facts>>count;for(unsigned i=0;i<count*4;++i) facts>>ignored;
    facts>>count;check(count==4,"four retained original boarding rectangles");
    std::array<Point,4> positions;
    for(unsigned i=0;i<4;++i) {
        unsigned local=0;double x=0,y=0,z=0,width=0,depth=0,height=0,angle=0;facts>>local>>x>>y>>z>>width>>depth>>height>>angle;
        check(local==locals[i],"original platform ordering");positions[i]={x,y,z};const auto w=workerBase+local*0x1000,g=w+0x558;
        set<uint16_t>(w+0x34,0x80);set<uint8_t>(w+0xab,16);
        set<float>(g,static_cast<float>(x));set<float>(g+4,static_cast<float>(y));set<float>(g+8,static_cast<float>(z));
        set<float>(g+0x30,static_cast<float>(width));set<float>(g+0x38,static_cast<float>(depth));set<float>(g+0x1c,static_cast<float>(height));set<float>(g+0x34,static_cast<float>(angle));
    }check(facts.good(),"ready original platform rectangle facts");
    const auto scan=[] {
        static nav::Scene copied;check(nav::captureNavigationScene(copied,1,false),"actual City native scene capture");
        std::vector<Entity> found;testStatusOwner=false;const bool ready=nav::scanNavigationScene(copied,found,[](int){return std::wstring{};},[](int){return std::wstring{};});testStatusOwner=true;
        check(ready,"City copied callbacks/targets ready");return found;
    };
    for(unsigned bits=0;bits<16;++bits) {
        for(unsigned i=0;i<4;++i) set<uint16_t>(workerBase+locals[i]*0x1000+0x34,bits&(1u<<i)?0x80:0);
        const auto owned=scan();resetNavigationScripts();std::vector<Entity> legacy;scanExits({},legacy);
        const std::vector<Entity>* sources[]={&owned,&legacy};
        for(unsigned i=0;i<4;++i) for(const auto* rows:sources) {
            unsigned seen=0;for(const auto& e:*rows) if(e.worker==static_cast<int>(locals[i])) {
                ++seen;check(e.name==labels[i] && e.category==L"Object" && e.zone.valid && e.zone.crossing && !e.zone.talk && e.zone.shape==3 && distanceXZ(e.zone.center,positions[i])<.001,"boarding target keeps actual owner/name/touch rectangle");
            }check(seen==((bits>>i)&1u),"only currently enabled native platform stops are listed");
        }
    }
    for(unsigned i=0;i<4;++i) set<uint16_t>(workerBase+locals[i]*0x1000+0x34,0x80);
    const auto w=workerBase+locals[0]*0x1000,g=w+0x558;set<float>(g,static_cast<float>(positions[0].x+10));
    const auto moved=scan();bool tracked=false;for(const auto& e:moved) if(e.worker==42) tracked=std::abs(e.zone.center.x-positions[0].x-10)<.001;check(tracked,"fresh native geometry moves the boarding target");
    constexpr unsigned callbacks=0x2430280;for(unsigned c=0;c<7;++c) set<uint16_t>(callbacks+c*2,5);set<uintptr_t>(w+0x44,address(callbacks));
    const auto remapped=scan();for(const auto& e:remapped) check(e.worker!=42,"unrelated remapped callback cannot retain a platform target");
    set<uintptr_t>(0xf26ae8,oldController);set<uintptr_t>(0x1fc44e4,oldActors);set<unsigned>(0x1fc44e0,oldCount);set<uintptr_t>(0xf00740,oldPlayer);set<uint16_t>(0xd2ca90,oldRoom);
    stub(0x46a7c0,navigationWorker);resetNavigationScripts();
    std::puts("PASS: original City four moving-platform stops, all16 enablement states in native/owned discovery, touch-only geometry, live pose, remapped callback and foreign-identity guards. Ride VM/gameplay unexecuted.");
}
}
