// Production assistance/resource qualification plus original native float and
// typed operand readers. Engine ownership, VM scope/PC and rendering/inventory
// are controlled dependencies; this does not execute the full game interpreter.
#include "../events.hpp"
#include "../native_worker_index.hpp"
#define address fixtureUtilityAddress
#define main utilityBaselineEntry
#include "utility_resource_fixture.cpp"
#undef main
#undef address
#include "../sin_collection.hpp"
#include "../settings.hpp"
#include <cmath>
namespace {
unsigned room=324,nativeFailure=0;bool focused=true,battle=false,menu=false,console=false,missingWorker=false,wrongLevel=false;
std::atomic<int> collectionSetting{0};
std::map<unsigned,uintptr_t> originalFunctions;
std::array<std::array<unsigned,8>,3> jumpTables{};
double unsignedCorrection=4294967296.0;
uintptr_t __cdecl originalVariable(uintptr_t worker,int variable,int element);
void __cdecl diagnosticNoop() {}
uintptr_t __cdecl ownedWorkerGetter(int index) {return missingWorker && index==24?0:workerGetter(index);}
unsigned __cdecl unexpectedInteger(uintptr_t,int,int) {check(false,"float replay entered integer fallback");return 0;}
uintptr_t dependency(unsigned va) {
    if(va==0x86c500) return reinterpret_cast<uintptr_t>(&originalVariable);
    if(va==0x86c6c0) return reinterpret_cast<uintptr_t>(&unexpectedInteger);
    if(va==0x1326ae8) return ffx::fixtureUtilityAddress(0xf26ae8);
    if(va==0x1327098) return reinterpret_cast<uintptr_t>(&nativeFailure);
    if(va==0xb11f1c) return reinterpret_cast<uintptr_t>(&unsignedCorrection);
    if(va==0x88c7a0) return reinterpret_cast<uintptr_t>(&diagnosticNoop);
    const unsigned tables[]={0x86c64c,0x86c57c,0x86c358};
    for(unsigned i=0;i<3;++i) if(va==tables[i]) return reinterpret_cast<uintptr_t>(jumpTables[i].data());
    return originalFunctions.at(va);
}
template<size_t N> void body(const char* name,unsigned va,uint64_t hash) {
    const auto input=ffx_fixture::load<N>((std::string("offline-output/sin_collection_20261020/")+name+".bin").c_str(),hash);
    const auto at=originalFunctions.at(va);std::memcpy(reinterpret_cast<void*>(at),input.bytes,N);
    std::ifstream fixes(ffx_fixture::evidenceRoot()/(std::string("offline-output/sin_collection_20261020/")+name+"-fixups.txt"));
    unsigned offset=0,target=0,relative=0;
    while(fixes>>offset>>target>>relative) {check(offset+4<=N,"original native relocation extent");const auto value=dependency(target)-(relative?at+offset+4:0);put(reinterpret_cast<void*>(at+offset),value);}
    FlushInstructionCache(GetCurrentProcess(),reinterpret_cast<void*>(at),N);
}
void prepareNativeReaders() {
    const unsigned bases[]={0x86c5a0,0x86c500,0x86c270};const char* names[]={"floatGet","variablePointer","variableBase"};
    for(unsigned va:{0x86c5a0u,0x86e3d0u,0x86c500u,0x86c270u}) {const auto p=VirtualAlloc(nullptr,4096,MEM_RESERVE|MEM_COMMIT,PAGE_EXECUTE_READWRITE);check(p!=nullptr,"original native reader allocation");originalFunctions[va]=reinterpret_cast<uintptr_t>(p);}
    for(unsigned t=0;t<3;++t) {
        const auto n=t==1?8u:7u;std::ifstream table(ffx_fixture::evidenceRoot()/(std::string("offline-output/sin_collection_20261020/")+names[t]+"-table.bin"),std::ios::binary);
        check(static_cast<bool>(table.read(reinterpret_cast<char*>(jumpTables[t].data()),n*4)),"original native switch table");
        for(unsigned i=0;i<n;++i) {check(jumpTables[t][i]>=bases[t] && jumpTables[t][i]<bases[t]+256,"switch targets original bounded function");jumpTables[t][i]=originalFunctions.at(bases[t])+jumpTables[t][i]-bases[t];}
    }
    body<171>("floatGet",0x86c5a0,0xac7f6c644b619dd3ull);body<123>("pushVariable",0x86e3d0,0x3d16a0a638f5296cull);
    body<124>("variablePointer",0x86c500,0xbc286cfbc11d22bbull);body<232>("variableBase",0x86c270,0x5a3d70a69f96dfd6ull);
}
uintptr_t __cdecl originalVariable(uintptr_t worker,int variable,int element) {
    const auto pointer=reinterpret_cast<uintptr_t(__cdecl*)(uintptr_t,int,int)>(originalFunctions.at(0x86c500))(worker,variable,element);
    Event e;e.rva=0x46c500;e.epoch=generation();e.parentRva=0x464180;e.args[0]=worker;e.args[1]=variable;e.args[2]=element;e.result=pointer;
    listeners.at(0x46c500).after(e);return e.result;
}
uintptr_t rawVariable(unsigned local,int variable) {return reinterpret_cast<uintptr_t(__cdecl*)(uintptr_t,int,int)>(originalFunctions.at(0x86c500))(workerGetter(local),variable,0);}
float readPickup(unsigned local,int delta=0,unsigned entry=6) {
    const auto w=workerGetter(local);static std::array<uint8_t,32> sig{};put(sig.data()+8,static_cast<uint16_t>(entry));put(sig.data()+14,static_cast<uint8_t>(wrongLevel?0:1));
    put(reinterpret_cast<void*>(w+0x90),reinterpret_cast<uintptr_t>(sig.data()));put(reinterpret_cast<void*>(w+0x32),uint8_t{1});
    put(reinterpret_cast<void*>(w+0x12c+0x4c+0x18),code+0xbeda+596*(local-24)+delta);
    Event vm;vm.epoch=generation();vm.args[0]=w;listeners.at(0x464180).before(vm);
    std::array<uint8_t,0x70> stack{};
    reinterpret_cast<void(__cdecl*)(uintptr_t,void*,int,int)>(originalFunctions.at(0x86e3d0))(w,stack.data(),207+3*(local-24),0);
    listeners.at(0x464180).after(vm);
    check(word<int>(reinterpret_cast<uintptr_t>(stack.data()))==1 && stack[0x54]==2,"original typed PUSHV preserves float stack ABI");
    return word<float>(reinterpret_cast<uintptr_t>(stack.data()+4));
}
void setCounts(unsigned a,unsigned b) {put(reinterpret_cast<void*>(rawVariable(0,26)),static_cast<uint8_t>(a));put(reinterpret_cast<void*>(rawVariable(0,27)),static_cast<uint8_t>(b));}
}
namespace ffx {
std::atomic<int> scene{0};
uintptr_t address(unsigned rva) {
    if(rva==0xd2ca90) return reinterpret_cast<uintptr_t>(&room);
    if(rva==0x46a7c0) return reinterpret_cast<uintptr_t>(&ownedWorkerGetter);
    if(rva==0x46c500) return reinterpret_cast<uintptr_t>(&originalVariable);
    return fixtureUtilityAddress(rva);
}
bool hook(unsigned rva,Handler before,Handler after) {listeners[rva]={before,after};return true;}
bool utilityBusy() {return false;}
int setting(Setting id) {return id==Setting::SinCollectionAssist?collectionSetting.load():0;}
void setSetting(Setting id,int value) {check(id==Setting::SinCollectionAssist,"collection changes only its own preference");collectionSetting=value;}
bool gameFocused() {return focused;}bool inBattle() {return battle;}bool modMenuActive() {return menu;}bool utilityConsoleActive() {return console;}
}
int main() {try {
    using namespace ffx;check(startAtelResources(),"resource bootstrap");installAtelResources();installSinCollection();prepareNativeReaders();
    // All ten retained Sin room mappings expose controls, including both
    // Nucleus aliases. No script identity, pending preparation or battle can
    // hide the toggle; native ownership and non-Sin boundaries still apply.
    for(unsigned candidate:{203u,204u,296u,322u,324u,325u,326u,327u,341u,384u}) {
        room=candidate;check(sinCollectionActive(),"all Sin room mappings expose collection controls before resource readiness");
        battle=true;check(sinCollectionActive(),"battle does not remove Sin setting availability");battle=false;
    }
    for(unsigned candidate:{0u,196u,202u,205u,295u,323u,328u,340u,342u,383u,385u}) {
        room=candidate;check(!sinCollectionActive(),"non-Sin room cannot reveal the setting");
        sinCollectionCommand(L"on");check(collectionSetting==0,"non-Sin On cannot change the preference");
    }
    // Original preparation resources never supply Nucleus counters or pickup
    // assistance. Explicit On persists while native scope resets.
    for(unsigned i=0;i<2;++i) {
        room=i?327:204;prepare(i?"sins0900":"sins0400");put(controller.data()+0x48,chunk);
        check(sinCollectionActive() && !sinCollectionArmed(),"both exact preparation maps expose the default-Off menu");
        const auto getterCount=getterCalls;
        sinCollectionCommand(L"on");check(sinCollectionArmed() && getterCalls==getterCount,"preparation On never reads Nucleus counters");
        check(renderUtilityText(sinCollectionCommand(L"status")).find(L"Ready for The Nucleus")!=std::wstring::npos && getterCalls==getterCount,"preparation status uses no foreign collection declaration");
        resetSinCollection();check(sinCollectionArmed(),"native reset preserves the armed preference before entering");
        room=196;check(!sinCollectionActive(),"correct preparation script in a non-Sin room does not reveal the setting");room=i?327:204;
        sinCollectionCommand(L"off");check(!sinCollectionArmed(),"preparation Off clears the preference");
    }
    room=327;prepare("sins0900",true,true);put(controller.data()+0x48,chunk);check(sinCollectionActive(),"preparation identity is independent of safe menu availability");
    prepare("sins0900");put(controller.data()+0x48,chunk);sinCollectionCommand(L"on");resetSinCollection();
    room=324;prepare("sins0500");put(controller.data()+0x48,chunk);setCounts(0,0);
    check(sinCollectionArmed(),"pre-armed preference survives the actual allocation/generation transition into Nucleus");
    put(reinterpret_cast<void*>(rawVariable(24,207)),90.f);
    allocations=frees=0;observe=true;const auto coldBegan=std::chrono::steady_clock::now();const auto coldRead=readPickup(24);observe=false;
    check(coldRead==0 && !allocations && !frees,"first qualified pre-armed pickup needs no F8 or native heap work");
    std::printf("METRIC first pre-armed original pickup %.3fus, heap0/0; controlled native SDK endpoints.\n",std::chrono::duration<double,std::micro>(std::chrono::steady_clock::now()-coldBegan).count());
    battle=true;check(readPickup(24)==90 && collectionSetting==1,"native battle pauses pickup assistance without clearing the preference");
    resetSinCollection();++epoch;battle=false;check(readPickup(24)==0,"fresh post-battle interpreter scope automatically resumes assistance");
    sinCollectionCommand(L"off");
    put(controller.data()+0x48,chunk);setCounts(0,0);check(sinCollectionActive() && !sinCollectionArmed(),"qualified Nucleus defaults Off");
    for(unsigned local=24;local<=33;++local) put(reinterpret_cast<void*>(rawVariable(local,207+3*(local-24))),90.f);
    check(readPickup(24)==90,"Off preserves original computed distance through original float ABI");sinCollectionCommand(L"on");check(sinCollectionArmed(),"explicit action arms only current exact resource");
    for(unsigned alias:{324u,341u}) {
        room=alias;
        for(unsigned local=24;local<=33;++local) {check(readPickup(local)==0,"both Nucleus aliases assist all ten original pickup predicates");check(word<float>(rawVariable(local,207+3*(local-24)))==90,"underlying computed distance is not overwritten");}
    }
    // A global controller may contain foreign workers in addition to this
    // resource's44. Requalify by actual native global membership, not equality.
    workers.resize(45);put(controller.data()+0xc,uint16_t{45});invalidateNativeWorkerIndex();
    check(readPickup(24)==0,"additional foreign global worker does not disable exact Nucleus assistance");
    workers.resize(500);put(controller.data()+0xc,uint16_t{500});invalidateNativeWorkerIndex();
    allocations=frees=0;observe=true;const auto largeBegan=std::chrono::steady_clock::now();const auto largeRead=readPickup(24);observe=false;
    check(largeRead==0 && !allocations && !frees,"maximum loaded controller requalifies without native heap work");
    std::printf("METRIC cold maximum500-global-worker original pickup %.3fus, heap0/0; controlled native getter.\n",std::chrono::duration<double,std::micro>(std::chrono::steady_clock::now()-largeBegan).count());
    workers.resize(44);put(controller.data()+0xc,uint16_t{44});invalidateNativeWorkerIndex();
    check(readPickup(24,3)==90 && readPickup(24,-12)==90 && readPickup(24,0,7)==90,"adjacent/read-write/cleanup sites retain native behavior");
    missingWorker=true;check(readPickup(24)==90,"removed global worker refuses assistance");missingWorker=false;invalidateNativeWorkerIndex();
    wrongLevel=true;check(readPickup(24)==90,"signal level must belong to the active thread");wrongLevel=false;
    const auto header=word<uintptr_t>(workerGetter(24)),declarations=chunk+word<unsigned>(header+0x14),high=declarations+207*8+4;
    put(reinterpret_cast<void*>(high),2u);check(readPickup(24)==90,"changed live declaration refuses prepared original facts");put(reinterpret_cast<void*>(high),1u);
    for(bool* guard:{&menu,&console,&battle}) {*guard=true;check(readPickup(24)==90,"context gate pauses assistance");*guard=false;}
    focused=false;check(readPickup(24)==90,"foreign focus pauses assistance");focused=true;
    room=204;check(sinCollectionActive() && readPickup(24)==90,"Sin preparation exposes controls but refuses Nucleus assistance");room=324;
    nativeOwner=false;check(!sinCollectionActive() && !sinCollectionArmed(),"foreign owner cannot enter native qualification");nativeOwner=true;
    sinCollectionCommand(L"off");check(!sinCollectionArmed() && readPickup(24)==90,"Off is immediate and restores native predicate");
    for(unsigned a=0;a<=5;++a) for(unsigned b=0;b<=5;++b) {setCounts(a,b);const auto text=renderUtilityText(sinCollectionCommand(L"status"));check(text.find(std::to_wstring(a+b)+L" of 10")!=std::wstring::npos,"both original native collection counters feed status");}
    sinCollectionCommand(L"on");check(!sinCollectionArmed(),"completed native collection cannot be rearmed");
    setCounts(0,0);sinCollectionCommand(L"on");allocations=frees=0;observe=true;const auto began=std::chrono::steady_clock::now();
    bool okay=true;for(unsigned i=0;i<1000;++i) okay=okay && readPickup(24+i%10)==0;observe=false;
    check(okay && !allocations && !frees,"warm original reader/actual assistance path has Native heap0/0");
    std::printf("METRIC original float/PUSHV plus actual assistance1000 %.3fms, heap0/0; controlled VM scope/PC and resource getters.\n",std::chrono::duration<double,std::milli>(std::chrono::steady_clock::now()-began).count());
    ++epoch;check(sinCollectionArmed() && readPickup(24)==0,"graphics generation retains preference and requalifies live native ownership");
    resetSinCollection();check(sinCollectionArmed(),"explicit native reset preserves only the saved preference");
    check(word<float>(originalVariable(workerGetter(24),207,0))==90,"reset clears borrowed interpreter scope before any later raw getter");
    prepare("sins0500",false);put(controller.data()+0x48,chunk);
    put(reinterpret_cast<void*>(rawVariable(24,207)),90.f);
    check(sinCollectionActive() && readPickup(24)==90,"pending resource keeps controls visible while refusing assistance");
    nativeOwner=false;drainAtelResourcesForTest();nativeOwner=true;setCounts(0,0);
    check(readPickup(24)==0,"prepared resource resumes assistance");prepare("sins0500",true,true);put(controller.data()+0x48,chunk);
    put(reinterpret_cast<void*>(rawVariable(24,207)),90.f);
    check(sinCollectionActive() && readPickup(24)==90,"modified resource keeps controls visible but cannot assist");
    std::puts("PASS: all ten Sin room mappings, non-Sin hiding, both Nucleus aliases, extra global workers, original City/Tower preparation, saved pre-arm through entry/battle/reset, no foreign counters, all ten original float/PUSHV predicates and Off/context/site/counter/pending/changed-resource guards. Full VM/rewards/animation/gameplay remain user-side.");
    return 0;
}catch(const std::exception& e) {observe=false;std::fprintf(stderr,"FAIL Sin collection: %s\n",e.what());return 1;}}
