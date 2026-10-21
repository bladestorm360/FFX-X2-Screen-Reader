namespace {
void reportedEventRuntimeFixtures() {
    constexpr unsigned chunk=0x2410000,ctrl=0x2430000,workers=0x2440000;
    constexpr unsigned actors=0x2240000,actor=actors+0x880,tables=0x2480000;
    const auto oldController=read<uintptr_t>(address(0xf26ae8));
    const auto oldActors=read<uintptr_t>(address(0x1fc44e4));
    const auto oldCapacity=read<unsigned>(address(0x1fc44e0));
    const auto oldWorkers=airshipWorkers;
    std::array<uint8_t,0x880> originalActor{};
    std::memcpy(originalActor.data(),reinterpret_cast<void*>(address(actor)),originalActor.size());
    const uint16_t oldControllerCount=oldController?read<uint16_t>(oldController+0xc):uint16_t{0};
    const auto oldControllerChunk=oldController?read<uintptr_t>(oldController+0x48):0;

    stub(0x46a7c0,airshipWorker);stub(0x476360,interactionEntry);
    stub(0x4767e0,interactionPriority);stub(0x46c500,unboundGuard);
    set<uintptr_t>(0xf26ae8,address(ctrl));set<uintptr_t>(ctrl+0x1f8,0);
    set<uintptr_t>(ctrl+0x48,address(chunk));
    set<uintptr_t>(0x1fc44e4,address(actors));set<unsigned>(0x1fc44e0,10);

    const auto run=[&](const char* room,const char* group,unsigned sphereWorker,
                       const std::vector<unsigned>& crossingWorkers) {
        const auto path=(ffx_fixture::evidenceRoot()/"offline-output/FFX/20261003_audit/files/ffx_ps2/ffx/master/jppc/event/obj").string()+
            "/"+group+"/"+room+"/"+room+".ebp";
        std::ifstream source(path,std::ios::binary);check(source.good(),"retained reported-event EBP exists");
        unsigned offsets[11]{};source.seekg(4);source.read(reinterpret_cast<char*>(offsets),sizeof(offsets));
        source.seekg(0,std::ios::end);unsigned end=static_cast<unsigned>(source.tellg());
        for(unsigned i=1;i<11;++i) if(offsets[i] && offsets[i]>=offsets[0] && offsets[i]<end) {end=offsets[i];break;}
        check(offsets[0]==64 && end>64 && end-64<0x70000,"reported-event ATEL chunk bounds");
        source.seekg(64);source.read(reinterpret_cast<char*>(address(chunk)),end-64);
        check(source.gcount()==end-64,"complete retained reported-event script loaded");
        const auto script=readScript(address(chunk));
        const auto catalog=navigationCatalog(script);check(catalog!=nullptr,"retained reported-event script is hash qualified");

        airshipWorkers.clear();set<uint16_t>(ctrl+0xc,static_cast<uint16_t>(script.workers.size()));
        for(unsigned i=0;i<script.workers.size();++i) {
            const auto off=workers+i*0x1000;ZeroMemory(reinterpret_cast<void*>(address(off)),0x1000);
            airshipWorkers.push_back(address(off));
            set<uintptr_t>(off,address(chunk+script.workers[i].header));set<uintptr_t>(off+4,address(chunk));
            set<uint16_t>(off+0x2e,static_cast<uint16_t>(i));set<uint16_t>(off+0x34,0x80);
            for(const auto geometry:{0x558u,0x284u}) {
                set<float>(off+geometry,100.f+static_cast<float>(i));set<float>(off+geometry+4,0);
                set<float>(off+geometry+8,200.f);set<float>(off+geometry+0x1c,20);
                set<float>(off+geometry+0x30,120);set<float>(off+geometry+0x34,0);
                set<float>(off+geometry+0x38,220);set<float>(off+geometry+0x3c,3.1415927f);
                set<float>(off+geometry+0x40,35);set<float>(off+geometry+0x48,50);
            }
        }
        if(sphereWorker<script.workers.size()) {
            const auto off=workers+sphereWorker*0x1000;
            set<uint8_t>(off+0xaa,1);set<uint8_t>(off+0xab,1);
            set<uintptr_t>(off+0x9c,address(actor));
            set<uint16_t>(actor,0x5045);set<uint16_t>(actor+2,1);
            set<uint8_t>(actor+0x186,2);set<int16_t>(actor+0x824,45);
            set<float>(actor+0xc,100);set<float>(actor+0x10,0);set<float>(actor+0x14,200);
            set<float>(off+0x558,100);set<float>(off+0x558+4,0);set<float>(off+0x558+8,200);
            set<float>(off+0x558+0x2c,30);set<float>(off+0x558+0x3c,3.1415927f);
            set<float>(ctrl+0x1f0,15);
        }
        // Type-5 workers use native channel 4 for the line crossing. Their
        // original scripts bind entry 2 through the worker callback table.
        // Endpoints below are controlled finite fixture poses; the report
        // record contains no exact endpoint coordinates.
        for(const auto index:crossingWorkers) {
            const auto off=workers+index*0x1000,table=tables+index*0x20;
            for(unsigned channel=0;channel<7;++channel)set<uint16_t>(table+channel*2,0xffff);
            set<uint16_t>(table+4*2,2);set<uintptr_t>(off+0x44,address(table));
            set<uint8_t>(off+0xab,0x10);
        }

        resetNavigationScripts();std::vector<Entity> found;scanExits({},found);
        const size_t expected=(sphereWorker<script.workers.size()?1u:0u)+crossingWorkers.size();
        check(found.size()==expected,"only the enabled reported event workers are discovered");
        for(auto& entity:found) {
            const auto* target=navigationTarget(catalog,static_cast<unsigned>(entity.worker));
            check(target && entity.usefulEntries.contains(2) && !entity.inactive,
                "reported event keeps its authored useful entry 2 and active native gate");
            check(entity.name.find(target->label)==0,"reported event discovery keeps its authored label");
            if(static_cast<unsigned>(entity.worker)==sphereWorker) {
                check(entity.name==L"Yuna's sphere" && entity.zone.valid && entity.zone.talk,
                    "linked sphere actor has a live native talk range");
            } else {
                check(entity.name==L"Staircase 1" || entity.name==L"Staircase 2",
                    "native staircase crossing retains its distinct authored event name");
            }
            set<uint8_t>(workers+entity.worker*0x1000+0xab,0);
            refreshNavigationTarget(entity);
            check(entity.inactive,"native interaction disable retires the reported event");
            set<uint8_t>(workers+entity.worker*0x1000+0xab,
                static_cast<unsigned>(entity.worker)==sphereWorker?1:0x10);
            refreshNavigationTarget(entity);
            check(!entity.inactive,"native interaction re-enable restores the reported event");
        }
        for(const auto index:crossingWorkers) set<uint8_t>(workers+index*0x1000+0xab,0);
        if(sphereWorker<script.workers.size())set<uint8_t>(workers+sphereWorker*0x1000+0xab,0);
        found.clear();scanExits({},found);
        check(found.empty(),"zero native event channels never create an active reported target");
    };

    run("mtgz0800","mt",9,{});
    run("dome0600","do",~0u,{25,26});

    resetNavigationScripts();airshipWorkers=oldWorkers;
    stub(0x46a7c0,reinterpret_cast<void*>(navigationWorker));
    stub(0x476360,reinterpret_cast<void*>(interactionEntry));
    stub(0x4767e0,reinterpret_cast<void*>(interactionPriority));
    stub(0x46c500,unboundGuard);
    set<uintptr_t>(0xf26ae8,oldController);
    if(oldController) {set<uint16_t>(oldController+0xc,oldControllerCount);set<uintptr_t>(oldController+0x48,oldControllerChunk);}
    set<uintptr_t>(0x1fc44e4,oldActors);set<unsigned>(0x1fc44e0,oldCapacity);
    std::memcpy(reinterpret_cast<void*>(address(actor)),originalActor.data(),originalActor.size());
    std::puts("PASS: retained sphere and Dome event workers honor live native interaction gates; actor and crossing poses are controlled fixture dependencies.");
}
}
