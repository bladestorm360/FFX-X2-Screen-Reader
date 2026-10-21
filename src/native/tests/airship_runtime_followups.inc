namespace {
uint16_t __cdecl interactionEntry(unsigned,unsigned,uintptr_t);
uint8_t __cdecl interactionPriority(unsigned,unsigned);
std::vector<uintptr_t> airshipWorkers;
uintptr_t __cdecl airshipWorker(int index) { return index>=0 && static_cast<size_t>(index)<airshipWorkers.size()?airshipWorkers[index]:0; }
uintptr_t __cdecl unboundGuard(uintptr_t,int,int) { return 0; }
void airshipRuntimeFixtures() {
    struct Room {const char* name;std::vector<unsigned> doors,elevators;};
    const Room rooms[]={
        {"hiku0000",{26,27},{29,30}}, {"hiku0200",{20,22,23,24},{}},
        {"hiku0500",{23,25,26,27,28,29},{}}, {"hiku2000",{14,15},{17,18}},
        {"bvyt0900",{30},{}},{"stbv0000",{26},{}}
    };
    constexpr unsigned chunk=0x2410000,ctrl=0x2430000,workers=0x2440000;
    stub(0x476360,interactionEntry);stub(0x4767e0,interactionPriority);stub(0x46a7c0,airshipWorker);stub(0x46c500,unboundGuard);
    set<uintptr_t>(0xf26ae8,address(ctrl)); set<uintptr_t>(ctrl+0x1f8,0); set<uintptr_t>(ctrl+0x48,address(chunk));
    set<uintptr_t>(0x1fc44e4,0); set<unsigned>(0x1fc44e0,0);
    for(const auto& room:rooms) {
        const auto path=(ffx_fixture::evidenceRoot()/"offline-output/FFX/20261003_audit/files/ffx_ps2/ffx/master/jppc/event/obj").string()+"/"+std::string(room.name,2)+"/"+room.name+"/"+room.name+".ebp";
        std::ifstream source(path,std::ios::binary); check(source.good(),"original airship EBP exists");
        unsigned offset[11]{};source.seekg(4);source.read(reinterpret_cast<char*>(offset),sizeof(offset));
        source.seekg(0,std::ios::end);unsigned end=static_cast<unsigned>(source.tellg());
        for(unsigned i=1;i<11;++i) if(offset[i] && offset[i]>=offset[0] && offset[i]<end) {end=offset[i];break;}
        check(offset[0]==64 && end>64 && end-64<0x20000,"airship ATEL chunk bounds");
        source.seekg(64);source.read(reinterpret_cast<char*>(address(chunk)),end-64);check(source.gcount()==end-64,"complete real Airship script");
        auto script=readScript(address(chunk)); const auto catalog=navigationCatalog(script);check(catalog!=nullptr,"real Airship script is hash qualified");
        airshipWorkers.clear();set<uint16_t>(ctrl+0xc,static_cast<uint16_t>(script.workers.size()));
        for(unsigned i=0;i<script.workers.size();++i) {
            const auto off=workers+i*0x1000;ZeroMemory(reinterpret_cast<void*>(address(off)),0x1000);
            airshipWorkers.push_back(address(off));set<uintptr_t>(off,address(chunk+script.workers[i].header));set<uintptr_t>(off+4,address(chunk));
            set<uint16_t>(off+0x2e,static_cast<uint16_t>(i));set<uint16_t>(off+0x34,0x80);
            set<float>(off+0x558,static_cast<float>(i*10));set<float>(off+0x558+0x30,10);set<float>(off+0x558+0x38,10);
            set<float>(off+0x558+0x1c,30);set<float>(off+0x558+0x40,35);set<float>(off+0x558+0x3c,3.1415927f);
        }
        for(const auto index:room.doors) set<uint8_t>(workers+index*0x1000+0xab,std::string(room.name)=="bvyt0900"?0x20:0x10);
        for(const auto index:room.elevators) set<uint8_t>(workers+index*0x1000+0xab,1);
        resetNavigationScripts();std::vector<Entity> entities;scanExits({},entities);
        check(entities.size()==room.doors.size()+room.elevators.size(),"real Airship exposes every enabled door and elevator with no ambient targets");
        for(auto& entity:entities) {
            const auto expected=navigationTarget(catalog,entity.worker);
            if(!expected || entity.name.find(expected->label)!=0 || entity.inactive) std::fwprintf(stderr,L"Passage fixture: %hs worker%d label %ls expected %ls inactive%d\n",room.name,entity.worker,entity.name.c_str(),expected?expected->label:L"missing",entity.inactive);
            check(expected && entity.name.find(expected->label)==0 && !entity.inactive,"same-map exit label and valid native geometry survive discovery");
            const bool elevator=std::find(room.elevators.begin(),room.elevators.end(),entity.worker)!=room.elevators.end();
            if(elevator) check(entity.category==L"Exit" && entity.usefulEntries.contains(2) && entity.zone.talk && entity.name.find(L"Confirm to ride")!=std::wstring::npos,"elevator appears among exits and routes to its actual Confirm interaction");
            else check(entity.category==L"Exit" && (std::string(room.name)=="bvyt0900"?entity.usefulEntries.contains(6):entity.zone.crossing),"passage retains native crossing or inside callback geometry");
            set<uint8_t>(workers+entity.worker*0x1000+0xab,0);refreshNavigationTarget(entity);check(entity.inactive,"cutscene disable retires target");
            set<uint8_t>(workers+entity.worker*0x1000+0xab,elevator?1:std::string(room.name)=="bvyt0900"?0x20:0x10);refreshNavigationTarget(entity);check(!entity.inactive,"native re-enable restores target without stale exit cache");
        }
        for(const auto index:room.doors) set<uint8_t>(workers+index*0x1000+0xab,0);
        entities.clear();scanExits({},entities);check(entities.size()==room.elevators.size(),"disabled door never hides the remaining elevator");
        for(const auto index:room.elevators) set<uint8_t>(workers+index*0x1000+0xab,0);
        entities.clear();scanExits({},entities);check(entities.empty(),"zero available interactions during cutscene are not invented");
        if(!room.elevators.empty()) {
            // Real hash-qualified EV and resource service; live poses and the
            // two separated floor states below are controlled test boundaries.
            fixturePhysicalPart(chunk,end-64);static nav::Scene copied;std::memset(&copied,0,sizeof(copied));
            copied.epoch=generation();copied.allowed=copied.control=copied.controlled=true;copied.partCount=1;copied.count=static_cast<unsigned>(script.workers.size());
            check(nativeAtelSpan(address(chunk),copied.parts[0].span),"original lift part has an owned resource identity");
            const unsigned deck=room.elevators.front()-4,up=room.elevators.front();
            for(unsigned i=0;i<copied.count;++i) {copied.workers[i].valid=true;copied.workers[i].part=0;copied.workers[i].local=copied.workers[i].global=i;}
            Entity exit,lift;exit.worker=deck;lift.worker=up;exit.category=lift.category=L"Exit";
            exit.name=L"Deck exit";lift.name=L"Elevator up. Confirm to ride";exit.id=lift.id=0xffff;
            for(auto* e:{&exit,&lift}) {e->zone.valid=true;e->zone.shape=3;e->zone.width=e->zone.depth=2;e->zone.height=5;e->zone.center=e->position={12,e==&exit?50.:0.,0};}
            Mesh floor;floor.openPlane=floor.fixedHeight=true;floor.planeY=0;copied.player={0,0,0};
            std::vector<Entity> staged{exit,lift};testStatusOwner=false;nav::stageAirshipPassages(copied,floor,staged);
            check(staged.size()==1 && staged[0].worker==static_cast<int>(up) && staged[0].name.find(L"Confirm to ride")!=std::wstring::npos,"lower floor exposes the actual reachable lift instead of an unreachable deck trigger");
            check(!navigationRouteOwned(floor,copied.player,staged[0],staged).empty(),"path/beacon/filter/Auto walk share the same real lift worker and zone");
            floor.planeY=50;copied.player.y=50;staged={exit,lift};nav::stageAirshipPassages(copied,floor,staged);
            check(staged.size()==2 && staged[0].worker==static_cast<int>(deck),"connected upper deck retains its actual exit");
            floor.planeY=0;copied.player.y=0;lift.inactive=true;staged={exit,lift};nav::stageAirshipPassages(copied,floor,staged);
            check(staged.size()==2,"disabled lift never becomes a proposed route");testStatusOwner=true;
        }
    }
    resetNavigationScripts();stub(0x46a7c0,navigationWorker);set<uintptr_t>(0xf26ae8,0);
    std::puts("PASS: all20 real Airship/Via door/elevator/progression callbacks, live cutscene disable/re-enable and correct crossing/Confirm geometry.");
}
}
