// Original mod host around retained, hash-qualified airship scripts. Native
// actor/worker/saved-state storage is controlled; no game process is accessed.
namespace {
uintptr_t __cdecl cidSavedVariable(uintptr_t worker,int index,int element) {
    if(index<0 || element) return 0;
    const auto header=read<uintptr_t>(worker),chunk=read<uintptr_t>(worker+4);
    if(static_cast<unsigned>(index)>=read<uint16_t>(header+2)) return 0;
    const auto row=chunk+read<unsigned>(header+0x14)+index*8;
    const auto declaration=read<unsigned>(row);
    return ((declaration>>25)&7)==0?address(0xd2ca90)+0x1ec+(declaration&0xffffff):0;
}
void airshipCidFixtures() {
    struct Map {const char* name;unsigned cid,wakka;};
    const Map maps[]={{"hiku0800",9,3},{"hiku0801",8,3},{"hiku1600",7,3},{"hiku1900",8,2}};
    constexpr unsigned chunk=0x21c0000,controller=0x2430000,workers=0x2440000,actors=0x23c0000;
    const auto previousController=read<uintptr_t>(address(0xf26ae8)),previousActors=read<uintptr_t>(address(0x1fc44e4)),previousPlayer=read<uintptr_t>(address(0xf00740));
    const auto previousCount=read<unsigned>(address(0x1fc44e0));const auto previousRoom=read<uint16_t>(address(0xd2ca90));
    const auto saved=address(0xd2ca90)+0x1ec;std::vector<uint8_t> previousSaved(0x68c0-0x1ec);
    std::memcpy(previousSaved.data(),reinterpret_cast<void*>(saved),previousSaved.size());
    stub(0x46a7c0,airshipWorker);stub(0x46c500,cidSavedVariable);stub(0x476360,interactionEntry);stub(0x4767e0,interactionPriority);
    for(const auto& map:maps) {
        const auto path=ffx_fixture::evidenceRoot()/"offline-output/FFX/20261003_audit/files/ffx_ps2/ffx/master/jppc/event/obj/hi"/map.name/(std::string(map.name)+".ebp");
        std::ifstream input(path,std::ios::binary);check(input.good(),"retained original Cid map exists");
        std::vector<uint8_t> bytes((std::istreambuf_iterator<char>(input)),{});unsigned offsets[11];std::memcpy(offsets,bytes.data()+4,sizeof(offsets));
        unsigned end=static_cast<unsigned>(bytes.size());for(unsigned i=1;i<11;++i) if(offsets[i]>offsets[0] && offsets[i]<end) end=offsets[i];
        check(offsets[0]==64 && end>64 && end-64<0x40000,"bounded original airship part");
        std::memcpy(reinterpret_cast<void*>(address(chunk)),bytes.data()+64,end-64);
        const auto script=readScript(address(chunk));const auto catalog=navigationCatalog(script);check(catalog && script.name==map.name,"exact Cid map code identity");
        fixturePhysicalPart(chunk,end-64);ZeroMemory(reinterpret_cast<void*>(address(controller)),0x238);
        set<uintptr_t>(0xf26ae8,address(controller));set<uint8_t>(controller,2);set<uint16_t>(controller+0xc,static_cast<uint16_t>(script.workers.size()));
        set<uintptr_t>(controller+0x48,address(chunk));set<uintptr_t>(controller+0x2c,saved);set<uint16_t>(0xd2ca90,263);
        set<uintptr_t>(0x1fc44e4,address(actors));set<unsigned>(0x1fc44e0,3);set<uintptr_t>(0xf00740,address(actors));
        ZeroMemory(reinterpret_cast<void*>(address(actors)),3*0x880);ZeroMemory(reinterpret_cast<void*>(saved),previousSaved.size());
        for(unsigned i=0;i<3;++i) {const auto a=actors+i*0x880;set<uint16_t>(a,i==0?1:i==1?0x2101:5);set<uint16_t>(a+2,1);set<int16_t>(a+0x824,0);}
        airshipWorkers.clear();
        for(unsigned i=0;i<script.workers.size();++i) {
            const auto w=workers+i*0x1000;ZeroMemory(reinterpret_cast<void*>(address(w)),0x1000);airshipWorkers.push_back(address(w));
            set<uintptr_t>(w,address(chunk+script.workers[i].header));set<uintptr_t>(w+4,address(chunk));set<uint16_t>(w+0x2e,static_cast<uint16_t>(i));
        }
        for(const auto local:{map.cid,map.wakka}) {
            const auto w=workers+local*0x1000;set<uint16_t>(w+0x34,0x80);set<uint8_t>(w+0xaa,1);set<uint8_t>(w+0xab,1);
            set<uintptr_t>(w+0x9c,address(actors+(local==map.cid?1:2)*0x880));
            set<float>(w+0x558+0x30,4);set<float>(w+0x558+0x38,4);set<float>(w+0x558+0x1c,30);set<float>(w+0x558+0x40,20);set<float>(w+0x558+0x3c,3.14159f);
        }
        const auto pose=[&](unsigned slot,Point p,unsigned local) {
            const auto a=actors+slot*0x880,g=workers+local*0x1000+0x558;
            for(const auto base:{a+0xc,g}) {set<float>(base,static_cast<float>(p.x));set<float>(base+4,static_cast<float>(p.y));set<float>(base+8,static_cast<float>(p.z));}
        };
        const auto scan=[&]() {
            static nav::Scene observation;check(nav::captureNavigationScene(observation,1,false),"actual native capture accepts ready controlled Cid scene");
            std::vector<Entity> out;testStatusOwner=false;const auto ready=nav::scanNavigationScene(observation,out,[](int){return std::wstring{};},[](int i){return i==4?L"Custom Wakka":L"";});testStatusOwner=true;
            check(ready,"owned Cid scene is ready");return out;
        };
        const auto expectCid=[&](const std::vector<Entity>& rows,unsigned slot,Point p) {
            unsigned count=0;for(const auto& row:rows) {
                if(row.id==0x2101) {++count;check(row.name==L"Cid" && row.category==L"NPC" && row.actor==static_cast<int>(slot) && distanceXZ(row.position,p)<.001,"Cid keeps name/class/current actor and live pose");}
                if(row.id==5) check(row.name==L"Custom Wakka","actual Wakka cannot inherit Cid from shared dialogue");
            }check(count==1,"one actual Cid remains in list across story/interaction changes");
        };
        pose(1,{20,0,30},map.cid);pose(2,{-20,0,40},map.wakka);
        for(unsigned story:{0u,1290u,2440u,3010u,4000u}) {
            write<uint16_t>(saved+0xa00,static_cast<uint16_t>(story));const auto rows=scan();expectCid(rows,1,{20,0,30});
            if(std::strcmp(map.name,"hiku1900")==0 && !story) for(const auto& row:rows) if(row.id==0x2101) check(row.inactive,"no-op story Talk stays inactive rather than being invented");
            resetNavigationScripts();std::vector<Entity> native;scanExits({},native);unsigned count=0;
            for(const auto& row:native) if(row.id==0x2101) {++count;check(row.name==L"Cid" && distanceXZ(row.position,{20,0,30})<.001,"native discovery agrees on Cid body and current pose");}
            check(count==1,"native discovery also retains present Cid through story guards");
        }
        const bool selectionCase=std::strcmp(map.name,"hiku1900")==0;
        const auto selectedCid=[&](unsigned local,unsigned slot,Point p) {
            navigationFixturePump();Entity selected;testStatusOwner=false;
            const auto ready=navigationSelectedForTest(selected);testStatusOwner=true;
            check(ready && selected.id==0x2101 && selected.name==L"Cid" && selected.worker==static_cast<int>(local) &&
                selected.actor==static_cast<int>(slot) && distanceXZ(selected.position,p)<.001,
                "actual navigation selection follows Cid's current owner and pose instead of falling back to Wakka");
        };
        if(selectionCase) {resetNavigation();keys[VK_OEM_3](false);selectedCid(map.cid,1,{20,0,30});}
        pose(1,{70,-5,10},map.cid);expectCid(scan(),1,{70,-5,10});
        if(selectionCase) {refreshNavigation();selectedCid(map.cid,1,{70,-5,10});}
        set<uint8_t>(workers+map.cid*0x1000+0xab,0);const auto silent=scan();expectCid(silent,1,{70,-5,10});
        for(const auto& row:silent) if(row.id==0x2101) check(row.inactive,"native disabled Talk does not remove actor or become a fabricated interaction");
        set<uint16_t>(actors+0x880+2,0);for(const auto& row:scan()) check(row.id!=0x2101,"disabled Cid body leaves no ghost destination");
        set<uint16_t>(actors+0x880+2,1);set<uint16_t>(actors+0x880,5);set<uint16_t>(actors+2*0x880,0x2101);
        set<uintptr_t>(workers+map.cid*0x1000+0x9c,address(actors+2*0x880));set<uintptr_t>(workers+map.wakka*0x1000+0x9c,address(actors+0x880));
        pose(2,{90,0,25},map.cid);expectCid(scan(),2,{90,0,25});
        if(selectionCase) {
            // Transfer to the other worker as well as another actor slot. The
            // previous static Wakka record must now identify actual Cid.
            set<uintptr_t>(workers+map.cid*0x1000+0x9c,address(actors+0x880));
            set<uintptr_t>(workers+map.wakka*0x1000+0x9c,address(actors+2*0x880));
            pose(2,{110,0,15},map.wakka);refreshNavigation();selectedCid(map.wakka,2,{110,0,15});resetNavigation();
        }
    }
    std::memcpy(reinterpret_cast<void*>(saved),previousSaved.data(),previousSaved.size());set<uint16_t>(0xd2ca90,previousRoom);
    set<uintptr_t>(0xf26ae8,previousController);set<uintptr_t>(0x1fc44e4,previousActors);set<unsigned>(0x1fc44e0,previousCount);set<uintptr_t>(0xf00740,previousPlayer);
    resetNavigationScripts();stub(0x46a7c0,navigationWorker);stub(0x46c500,unboundGuard);
    std::puts("PASS: four original Cid airship variants, story0/1290/2440/3010/4000, actual capture/owned/native discovery, Wakka identity, live movement/rebinding and disabled-actor rejection. No game process used.");
}
}
