// Original mod adapters load separately retained game inputs from outside Git.
// Native region/distance bodies execute offline; CRT trig and script scheduling
// are controlled boundaries. No game process or game-owned memory is involved.
namespace {
std::vector<uint8_t> baajInput(const char* name) {
    char configured[MAX_PATH]{};const auto length=GetEnvironmentVariableA("FFX_BAAJ_INPUTS",configured,MAX_PATH);
    check(length<MAX_PATH,"external Baaj fixture directory is bounded");
    const auto base=length?std::filesystem::path(configured):ffx_fixture::repositoryRoot().parent_path()/"Analysis/FFX/Summon_20261020/inputs";
    std::ifstream file(base/name,std::ios::binary);check(file.good(),"external Baaj fixture input exists");
    return std::vector<uint8_t>(std::istreambuf_iterator<char>(file),{});
}
uintptr_t baajReplay(unsigned rva,const char* name) {
    const auto bytes=baajInput((std::string(name)+".bin").c_str());
    const auto fixes=baajInput((std::string(name)+".fixups").c_str());
    check(!bytes.empty() && bytes.size()<4096 && fixes.size()%12==0,"bounded external native replay input");
    const auto code=nativeReplayAddress(rva);std::memcpy(reinterpret_cast<void*>(code),bytes.data(),bytes.size());
    for(size_t at=0;at<fixes.size();at+=12) {
        unsigned fix[3];std::memcpy(fix,fixes.data()+at,sizeof(fix));check(fix[0]+4<=bytes.size(),"native fixup stays in body");
        const auto value=nativeReplayAddress(fix[1])-(fix[2]?code+fix[0]+4:0);
        std::memcpy(reinterpret_cast<void*>(code+fix[0]),&value,4);
    }
    FlushInstructionCache(GetCurrentProcess(),reinterpret_cast<void*>(code),bytes.size());return code;
}
__declspec(naked) void baajSin() {
    __asm {
        fsin
        ret
    }
}
__declspec(naked) void baajCos() {
    __asm {
        fcos
        ret
    }
}
void baajTrigStub(unsigned rva,void* function) {
    // Intrinsic CRT entry points preserve general registers. Branch without
    // the ordinary adapter's EAX trampoline, retaining their x87 contract.
    const auto code=nativeReplayAddress(rva);write<uint8_t>(code,0x68);write<uintptr_t>(code+1,reinterpret_cast<uintptr_t>(function));write<uint8_t>(code+5,0xc3);
    FlushInstructionCache(GetCurrentProcess(),reinterpret_cast<void*>(code),6);
}
unsigned baajEntered=0,baajLeft=0;
uint8_t baajSavedFlag=0;
uint8_t baajActivatedCount=0;
uintptr_t __cdecl baajSavedVariable(uintptr_t,int variable,int element) {
    // Controlled getter boundary for the seals' verified declaration0. The
    // original bytecode/proof is exercised; no native variable getter is run.
    return element==0 && (variable==0 || variable==2)?reinterpret_cast<uintptr_t>(variable==0?&baajSavedFlag:&baajActivatedCount):0;
}
void __cdecl baajScheduled(unsigned,unsigned,unsigned channel) {if(channel==5) ++baajEntered;else if(channel==6) ++baajLeft;else check(false,"type6 schedules only entry/leave");}
uint16_t __cdecl baajEntry(unsigned type,unsigned channel,uintptr_t table) {
    if(type!=6) return interactionEntry(type,channel,table);
    if(table) return read<uint16_t>(table+channel*2,0xffff);
    // Factual default callback metadata, not a copied native implementation.
    const uint16_t entries[]={2,2,0xffff,2,2,2,3};return channel<7?entries[channel]:0xffff;
}
void baajNavigationFixtures() {
    constexpr unsigned chunk=0x2410000,controller=0x2430000,workerBase=0x2440000;
    const auto originalController=read<uintptr_t>(address(0xf26ae8));
    auto ebp=baajInput("bjyt1000.ebp");check(ebp.size()>64,"retained Baaj EV has header");
    unsigned offsets[11];std::memcpy(offsets,ebp.data()+4,sizeof(offsets));
    unsigned end=static_cast<unsigned>(ebp.size());for(unsigned i=1;i<11;++i) if(offsets[i]>offsets[0] && offsets[i]<end) end=offsets[i];
    check(offsets[0]==64 && end>64 && end-64<0x20000,"retained Baaj ATEL extent bounded");
    std::memcpy(reinterpret_cast<void*>(address(chunk)),ebp.data()+64,end-64);
    const auto script=readScript(address(chunk));const auto catalog=navigationCatalog(script);
    check(catalog && script.name=="bjyt1000" && script.workers.size()==37,"actual hash-qualified Baaj script");
    stub(0x476360,baajEntry);stub(0x4767e0,interactionPriority);stub(0x46a7c0,airshipWorker);stub(0x46c500,unboundGuard);
    set<uintptr_t>(0xf26ae8,address(controller));set<uint16_t>(controller+0xc,37);set<uintptr_t>(controller+0x48,address(chunk));
    set<uintptr_t>(controller+0x1f8,0);set<uintptr_t>(0x1fc44e4,0);set<unsigned>(0x1fc44e0,0);
    airshipWorkers.clear();
    for(unsigned i=0;i<37;++i) {
        const auto w=workerBase+i*0x1000;ZeroMemory(reinterpret_cast<void*>(address(w)),0x1000);airshipWorkers.push_back(address(w));
        set<uintptr_t>(w,address(chunk+script.workers[i].header));set<uintptr_t>(w+4,address(chunk));set<uint16_t>(w+0x2e,static_cast<uint16_t>(i));
    }
    const float x[]={25,25,25,-25,-25,-25},z[]={-30,0,30,30,0,-30};
    const wchar_t* names[]={L"Kilika seal",L"Djose seal",L"Bevelle seal",L"Besaid seal",L"Macalania seal",L"Zanarkand seal"};
    fixturePhysicalPart(chunk,end-64);static nav::Scene copied;ZeroMemory(&copied,sizeof(copied));
    copied.epoch=generation();copied.count=37;copied.actorCount=1;copied.playerIndex=0;copied.partCount=1;copied.allowed=true;
    check(nativeAtelSpan(address(chunk),copied.parts[0].span),"Baaj prepared allocation identity");
    std::ifstream save(ffx_fixture::repositoryRoot().parent_path()/"Saves/Dylan/Summon/ffx_000",std::ios::binary);check(save.good(),"named Summon save exists");
    save.seekg(64+0x1ec);save.read(reinterpret_cast<char*>(copied.saved.bytes.data()),copied.saved.bytes.size());
    check(save.gcount()==copied.saved.bytes.size(),"complete bounded saved-fact bank copied");copied.saved.valid=true;
    check(copied.saved.bytes[0xa70]==0x3e,"provided save records five prerequisite flags, not all six");
    for(unsigned i=0;i<6;++i) {
        const unsigned local=30+i,w=workerBase+local*0x1000,g=w+0x284;const auto target=navigationTarget(catalog,local);
        check(target && target->identity==3 && std::wstring(target->label)==names[i] && read<uint8_t>(address(chunk+script.workers[local].header))==6,"six authored seal identities match original type6 workers");
        set<uint16_t>(w+0x34,0x80);set<uint8_t>(w+0xab,0x7f);set<int16_t>(g+0x28,-1);
        set<float>(g,x[i]);set<float>(g+4,0);set<float>(g+8,z[i]);set<float>(g+0x30,9);set<float>(g+0x38,9);set<float>(g+0x1c,10);
        auto& fact=copied.workers[local];fact.valid=true;fact.actor=-1;fact.part=0;fact.local=fact.global=local;fact.header=script.workers[local].header;fact.type=6;fact.position={x[i],0,z[i]};
        fact.interactions=captureNativeInteractionFacts(address(w),target->entries,copied.parts[0].span);
        check(fact.interactions.channels==0x20 && fact.interactions.entries[5]==2,"native capture selects type6 entry channel5, not default aliases");
        std::memcpy(fact.geometry.data(),reinterpret_cast<void*>(address(g)),fact.geometry.size());
    }
    const auto scan=[&]() {
        testStatusOwner=false;std::vector<Entity> entities;
        const bool ready=nav::scanNavigationScene(copied,entities,[](int){return std::wstring{};},[](int){return std::wstring{};});testStatusOwner=true;
        check(ready && entities.size()==6,"all six seals survive actual owned catalog/callback filtering");return entities;
    };
    auto entities=scan();unsigned inactive=0;
    for(const auto& entity:entities) {inactive+=entity.inactive;check(entity.zone.valid && entity.zone.crossing && !entity.zone.talk,"owned seal routes use walk-in geometry");}
    if(inactive!=1 || entities[3].name!=L"Besaid seal" || !entities[3].inactive)
        for(const auto& entity:entities) std::fwprintf(stderr,L"Baaj guard worker%d %ls inactive%d\n",entity.worker,entity.name.c_str(),entity.inactive);
    check(inactive==1 && entities[3].name==L"Besaid seal" && entities[3].inactive,"real saved guard keeps Besaid listed but inactive");
    // Each label is cross-qualified against the matching temple treasure write,
    // not a guessed ordinal. Exercise every missing flag against real Baaj code.
    constexpr unsigned treasureMasks[]={2,4,16,1,8,32};
    for(unsigned missing=0;missing<6;++missing) {
        copied.saved.bytes[0xa70]=static_cast<uint8_t>(0x3f^treasureMasks[missing]);
        const auto oneMissing=scan();unsigned unavailable=0;
        for(unsigned row=0;row<oneMissing.size();++row) {
            unavailable+=oneMissing[row].inactive;
            if(oneMissing[row].name!=names[row] || oneMissing[row].inactive!=(row==missing))
                std::fwprintf(stderr,L"Baaj matrix missingMask%u row%u worker%d label%ls inactive%d expectedInactive%d\n",treasureMasks[missing],row,oneMissing[row].worker,oneMissing[row].name.c_str(),oneMissing[row].inactive,row==missing);
            check(oneMissing[row].name==names[row] && oneMissing[row].inactive==(row==missing),"each original treasure flag disables only its correctly named seal");
        }
        check(unavailable==1,"one missing temple treasure produces exactly one inactive seal");
    }
    copied.saved.bytes[0xa70]=0x3e;
    baajSavedFlag=0x3e;stub(0x46c500,baajSavedVariable);
    for(unsigned flags=0;flags<64;++flags) {
        copied.saved.bytes[0xa70]=baajSavedFlag=static_cast<uint8_t>(flags);
        const auto allStates=scan();resetNavigationScripts();std::vector<Entity> nativeStates;scanExits({},nativeStates);
        check(nativeStates.size()==6,"native guard retains all six seal identities in every prerequisite combination");
        for(unsigned row=0;row<6;++row) {
            const bool unavailable=(flags&treasureMasks[row])==0;
            check(allStates[row].inactive==unavailable && nativeStates[row].inactive==unavailable,
                "all64 treasure combinations agree between native and owned guards for each named seal");
        }
    }
    auto unknownIdentity=script;unknownIdentity.checksum^=1;
    check(callbackAvailableOwned(unknownIdentity,34,2,[](unsigned){return std::optional<int>(0);}),
        "Macalania cleanup fact cannot apply to an unqualified script identity");
    copied.saved.bytes[0xa70]=baajSavedFlag=0x3e;
    resetNavigationScripts();std::vector<Entity> legacy;scanExits({},legacy);
    check(legacy.size()==6 && legacy[3].inactive,"native indexed guard and owned saved guard agree on the supplied prerequisite");
    auto indexed=script;const auto guard=indexed.workers[33].entries[2];
    check(indexed.code[guard]==0xae && indexed.code[guard+3]==0xa2,"real seal prefix uses indexed scalar variable");
    indexed.code[guard+1]=1; // Unmodeled array element must remain available/unknown.
    check(callbackAvailableOwned(indexed,33,2,[](unsigned){return std::optional<int>(0x3e);}),"nonzero indexed saved load does not invent scalar unavailability");
    copied.saved.bytes[0xa70]=0x3f;entities=scan();for(const auto& entity:entities) check(!entity.inactive,"all-prerequisite copied state enables all six without changing save");
    baajSavedFlag=0x3f;resetNavigationScripts();legacy.clear();scanExits({},legacy);check(legacy.size()==6,"legacy discovery agrees on all six region labels");
    for(const auto& entity:legacy) check(!entity.inactive,"native indexed guard accepts all-prerequisite state");
    const auto native=baajReplay(0x467bb0,"type6");baajReplay(0x48c330,"distance_squared");
    baajTrigStub(0x5498c8,baajSin);baajTrigStub(0x5498c2,baajCos);replayStub(0x476510,baajScheduled);
    const auto minusOne=baajInput("negative_one.bin");check(minusOne.size()==8,"retained native double constant");
    std::memcpy(reinterpret_cast<void*>(nativeReplayAddress(0x792340)),minusOne.data(),8);
    write<uintptr_t>(nativeReplayAddress(0xf26ae8),address(controller));
    Mesh floor;floor.openPlane=floor.fixedHeight=true;floor.planeY=0;unsigned comparisons=0;
    const auto nativeEntry=[&](unsigned w,Point point) {
        set<uint16_t>(w+0x34,0x80);baajEntered=baajLeft=0;
        for(unsigned offset:{0x218u,0x228u}) {set<float>(controller+offset,static_cast<float>(point.x));set<float>(controller+offset+4,static_cast<float>(point.y));set<float>(controller+offset+8,static_cast<float>(point.z));}
        reinterpret_cast<void(__cdecl*)(uintptr_t)>(native)(address(w));++comparisons;return baajEntered;
    };
    for(unsigned i=0;i<6;++i) {
        const unsigned local=30+i,w=workerBase+local*0x1000,g=w+0x284;
        for(float angle:{0.f,.73f,1.5707963f}) {
            set<float>(g+0x34,angle);std::memcpy(copied.workers[local].geometry.data(),reinterpret_cast<void*>(address(g)),0x4c);
            const auto owned=nav::sceneInteractionZone(copied.workers[local],0x20,0),live=interactionZone(address(w),0x20);
            check(owned.valid && live.valid && owned.angle==live.angle && owned.width==live.width,"native refresh and copied decoder share one region contract");
            const auto route=interactionRoute(floor,{},owned);check(!route.empty() && inInteractionZone(owned,route.back()),"production route ends strictly inside seal region");
            const auto accepted=nativeEntry(w,route.back());
            if(accepted!=1) std::fprintf(stderr,"Baaj native W%u angle%g endpoint %.4f %.4f %.4f center%g %g flags%x enter%u leave%u\n",local,angle,route.back().x,route.back().y,route.back().z,x[i],z[i],read<uint16_t>(address(w+0x34)),baajEntered,baajLeft);
            check(accepted==1,"original1027-byte native region accepts actual production route endpoint");
            for(int dx=-9;dx<=9;dx+=3) for(int dz=-9;dz<=9;dz+=3) {
                Point point{x[i]+dx,0,z[i]+dz};if(inInteractionZone(owned,point)) check(nativeEntry(w,point)==1,"every accepted sampled endpoint dispatches original native entry");
            }
        }
        set<int16_t>(g+0x28,0);check(!interactionZone(address(w),0x20).valid,"another-worker reference is not a player destination");set<int16_t>(g+0x28,-1);
        set<uint16_t>(w+0x34,0);Entity refreshed=entities[i];refreshNavigationTarget(refreshed);check(refreshed.inactive,"native seal disable retires selected target");
        copied.workers[local].interactions={};
    }
    entities=scan();for(const auto& entity:entities) check(entity.inactive,"disabled seals stay listed as inactive without a false route");
    copied.workers[30].geometry.fill(0);check(!nav::sceneInteractionZone(copied.workers[30],0x20,0).valid,"collapsed/missing region data refuses");
    // Exercise actual native observation of the two diagnostic scalars. The
    // original script/declaration/resource services are real; the getter's
    // values and actor storage remain explicit controlled host boundaries.
    const auto oldActors=read<uintptr_t>(address(0x1fc44e4)),oldControlled=read<uintptr_t>(address(0xf00740));
    const auto oldActorCount=read<unsigned>(address(0x1fc44e0));const auto oldRoom=read<uint16_t>(address(0xd2ca90));
    const auto savedBank=address(0xd2ca90)+0x1ec;std::vector<uint8_t> oldBank(copied.saved.bytes.size());std::memcpy(oldBank.data(),reinterpret_cast<void*>(savedBank),oldBank.size());
    std::memcpy(reinterpret_cast<void*>(savedBank),copied.saved.bytes.data(),copied.saved.bytes.size());
    constexpr unsigned testActor=0x23c0000;ZeroMemory(reinterpret_cast<void*>(address(testActor)),0x880);
    set<uint16_t>(testActor,1);set<uint16_t>(testActor+2,1);set<int16_t>(testActor+0x824,0);
    set<uintptr_t>(0x1fc44e4,address(testActor));set<unsigned>(0x1fc44e0,1);set<uintptr_t>(0xf00740,address(testActor));set<uint16_t>(0xd2ca90,196);
    set<uintptr_t>(controller+0x2c,savedBank);set<uint8_t>(controller,2);set<uint16_t>(controller+0x1a,0);
    stub(0x46c500,baajSavedVariable);baajSavedFlag=static_cast<uint8_t>(copied.saved.bytes[0xa70]);baajActivatedCount=5;
    static nav::Scene observed;check(nav::captureNavigationScene(observed,1,false),"actual Baaj diagnostic scene captures");
    check(observed.baajFactsReady && observed.baajTreasureFlags==baajSavedFlag && observed.baajActivatedCount==5 && observed.saved.valid,
        "qualified native treasure/counter facts and copied saved-bank validity stay distinct");
    set<uintptr_t>(controller+0x2c,0);check(nav::captureNavigationScene(observed,2,false) && !observed.saved.valid && observed.baajFactsReady,
        "unknown copied bank does not erase the independently captured native diagnostic facts");
    std::memcpy(reinterpret_cast<void*>(savedBank),oldBank.data(),oldBank.size());set<uint16_t>(0xd2ca90,oldRoom);
    set<uintptr_t>(0x1fc44e4,oldActors);set<unsigned>(0x1fc44e0,oldActorCount);set<uintptr_t>(0xf00740,oldControlled);
    resetNavigationScripts();stub(0x46a7c0,navigationWorker);stub(0x476360,interactionEntry);stub(0x46c500,unboundGuard);set<uintptr_t>(0xf26ae8,originalController);
    std::printf("PASS: six real Baaj seals, provided/all prerequisite states, inactive retention, native handback, shared type6 geometry and %u original native endpoint comparisons. Native scheduling/CRT/floor are controlled offline boundaries.\n",comparisons);
}
}
