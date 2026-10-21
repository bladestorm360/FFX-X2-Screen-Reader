// Production completion adapter + real prepared EV resources and original x86
// typed variable base/pointer/setter. VM event scheduling, request dispatch,
// focus and renderer/animation are controlled boundaries, not gameplay proof.
#include "../events.hpp"
#include "../native_worker_index.hpp"
namespace ffx {uintptr_t fixtureUtilityAddress(unsigned);}
#define address fixtureUtilityAddress
#define deferNativeAction unusedDeferredAction
#define main utilityBaselineEntry
#include "utility_resource_fixture.cpp"
#undef main
#undef deferNativeAction
#undef address
#include "../minigame_completion.hpp"
#include "../native_worker_index.hpp"
#include "../mod_menu.hpp"
#include <iomanip>
#include "atel_queue_bytes.inc"
namespace {
using namespace ffx;
bool focused=true,menu=false,battle=false,console=false,queueFull=false,realQueue=false;
std::vector<std::array<uint8_t,96>> pools;
std::array<uint8_t,0x1000> descriptors{};
void prepareOriginalQueue();
void initializePools(bool halted);
std::array<int,static_cast<unsigned>(Setting::Count)> preferences{};
std::array<unsigned,static_cast<unsigned>(Setting::Count)> revisions{};
NativeActionQueue pending;
std::vector<std::array<uint8_t,32>> signals;
std::array<uint8_t,0x10000> saved{};
unsigned room=290,nativeFailure=0,mutations=0,requestCount=0,dispatchCount=0;
struct Receipt {unsigned local=0,entry=0;};
std::array<Receipt,8> receipts{};
std::map<unsigned,uintptr_t> functions;
std::array<std::array<unsigned,8>,3> tables{};
uintptr_t __cdecl variablePointer(uintptr_t w,int index,int element) {
    check(nativeOwner,"original pointer getter remains native-owned");
    return reinterpret_cast<uintptr_t(__cdecl*)(uintptr_t,int,int)>(functions.at(0x86c500))(w,index,element);
}
void __cdecl nativeSetter(uintptr_t w,int index,int element,int v,int limited) {
    check(nativeOwner,"original typed setter remains native-owned");++mutations;
    reinterpret_cast<void(__cdecl*)(uintptr_t,int,int,int,int)>(functions.at(0x8713a0))(w,index,element,v,limited);
}
int __cdecl nativeRequest(int source,int target,int resultSlot,int level,int entry) {
    check(nativeOwner && source>=0 && static_cast<unsigned>(source)<workers.size(),"native request source is loaded");
    check(resultSlot==0 && level==7 && requestCount<receipts.size(),"native request ABI and work bound");
    check(target>=0 && static_cast<unsigned>(target)<workers.size(),"native request global target bound");
    receipts[requestCount++]={read<uint16_t>(workerGetter(target)+0x2e,0xffff),static_cast<unsigned>(entry)};
    return realQueue?reinterpret_cast<int(__cdecl*)(int,int,int,int,int)>(functions.at(0x86e920))(source,target,resultSlot,level,entry):1;
}
void __cdecl nativeDispatch(int target,int level) {
    check(nativeOwner && target>=0 && static_cast<unsigned>(target)<workers.size() && (level==-1 || level==10),"native dispatch ABI");++dispatchCount;
    if(realQueue) {
        reinterpret_cast<void(__cdecl*)(int,int)>(functions.at(0x865390))(target,level);
        const auto w=workerGetter(target),sig=read<uintptr_t>(w+0x90);const auto header=read<uintptr_t>(w);
        check(sig && read<uint16_t>(sig+8)==receipts[dispatchCount-1].entry,"original scheduler actually starts requested finish entry");
        const auto pc=read<unsigned>(chunk+read<unsigned>(header+0x20)+read<uint16_t>(sig+8)*4);
        check(read<uintptr_t>(w+0x12c+(read<uint8_t>(sig+14)&15u)*0x4c+0x18)==code+pc,"original native initializer selects exact original script entry PC");
    }
}
void __cdecl noop() {}
int __cdecl unpaused() {return 0;}
void __cdecl initializeObserved(uintptr_t w,uintptr_t sig) {
    reinterpret_cast<void(__cdecl*)(uintptr_t,uintptr_t)>(functions.at(0x869040))(w,sig);
}
void __cdecl setupThread(uintptr_t w,uintptr_t,unsigned) {
    const auto type=read<uint8_t>(read<uintptr_t>(w));check(type!=1,"finish endpoints do not load actor-owned thread stacks");
}
void __cdecl unexpectedQueueCall() {check(false,"unmodeled original scheduler dependency");}

uintptr_t dependency(unsigned va) {
    if(va==0x1326ae8) return ffx::fixtureUtilityAddress(0xf26ae8);
    if(va==0x1327098) return reinterpret_cast<uintptr_t>(&nativeFailure);
    if(va==0x88c7a0) return reinterpret_cast<uintptr_t>(&noop);
    const unsigned locations[]={0x86c358,0x86c57c,0x871480};
    for(unsigned i=0;i<3;++i) if(va==locations[i]) return reinterpret_cast<uintptr_t>(tables[i].data());
    if(va==0x86a7c0) return reinterpret_cast<uintptr_t>(&workerGetter);
    if(va==0x780d60) return reinterpret_cast<uintptr_t>(&unpaused);
    if(va==0x869040) return reinterpret_cast<uintptr_t>(&initializeObserved);
    if(va==0x8636f0) return reinterpret_cast<uintptr_t>(&setupThread);
    if(va==0x8725f0 || va==0x862c00 || va==0x862c60 || va==0x872800 || va==0x863980) return reinterpret_cast<uintptr_t>(&unexpectedQueueCall);
    if(va>=0x1325b60 && va<=0x1325d5c) return reinterpret_cast<uintptr_t>(descriptors.data())+va-0x1325b60;
    return functions.at(va);
}
template<size_t N> void body(const char* name,unsigned va,uint64_t hash) {
    const auto bytes=ffx_fixture::load<N>((std::string("offline-output/minigame_completion_20261020/")+name+".bin").c_str(),hash);
    const auto at=functions.at(va);memcpy(reinterpret_cast<void*>(at),bytes.bytes,N);
    std::ifstream in(ffx_fixture::evidenceRoot()/(std::string("offline-output/minigame_completion_20261020/")+name+"-fixups.txt"));
    unsigned offset=0,target=0,relative=0;
    while(in>>offset>>target>>relative) {check(offset+4<=N,"original relocation bound");put(reinterpret_cast<void*>(at+offset),dependency(target)-(relative?at+offset+4:0));}
    FlushInstructionCache(GetCurrentProcess(),reinterpret_cast<void*>(at),N);
}
void originalFunctions() {
    for(unsigned va:{0x86c270u,0x86c500u,0x8713a0u}) {
        const auto p=VirtualAlloc(nullptr,4096,MEM_COMMIT|MEM_RESERVE,PAGE_EXECUTE_READWRITE);check(p!=nullptr,"original typed native code allocation");functions[va]=reinterpret_cast<uintptr_t>(p);
    }
    const char* names[]={"base","pointer","setter"};const unsigned bases[]={0x86c270,0x86c500,0x8713a0};
    for(unsigned t=0;t<3;++t) {
        const auto count=t==1?8u:7u;
        std::ifstream in(ffx_fixture::evidenceRoot()/(std::string("offline-output/minigame_completion_20261020/")+names[t]+"-table.bin"),std::ios::binary);
        check(static_cast<bool>(in.read(reinterpret_cast<char*>(tables[t].data()),count*4)),"original native switch table");
        for(unsigned i=0;i<count;++i) {check(tables[t][i]>=bases[t] && tables[t][i]<bases[t]+256,"original native table range");tables[t][i]=functions.at(bases[t])+tables[t][i]-bases[t];}
    }
    body<232>("base",0x86c270,6502477297937211350ull);
    body<124>("pointer",0x86c500,13558206506731381435ull);
    body<224>("setter",0x8713a0,3825552034105480475ull);
}
template<size_t N,size_t M> void queueBody(unsigned va,const unsigned char (&bytes)[N],const std::array<AtelReplayFixup,M>& fixes) {
    const auto at=functions.at(va);memcpy(reinterpret_cast<void*>(at),bytes,N);
    for(const auto& f:fixes) put(reinterpret_cast<void*>(at+f.offset),dependency(f.va)-(f.relative?at+f.offset+4:0));
    FlushInstructionCache(GetCurrentProcess(),reinterpret_cast<void*>(at),N);
}
void prepareOriginalQueue() {
    for(unsigned va:{0x86e920u,0x86e990u,0x865390u,0x869040u,0x86dba0u,0x86dbc0u,0x86dbe0u,0x8715f0u}) {
        const auto p=VirtualAlloc(nullptr,4096,MEM_COMMIT|MEM_RESERVE,PAGE_EXECUTE_READWRITE);check(p!=nullptr,"original native queue allocation");functions[va]=reinterpret_cast<uintptr_t>(p);
    }
    queueBody(0x86e920,queueRequestBody,queueRequestFixups);queueBody(0x86e990,queueInsertBody,queueInsertFixups);
    queueBody(0x865390,queueDispatchBody,queueDispatchFixups);queueBody(0x869040,queueInitBody,queueInitFixups);
    queueBody(0x86dba0,queueAfterBody,queueAfterFixups);queueBody(0x86dbc0,queueBeforeBody,queueBeforeFixups);
    queueBody(0x86dbe0,queueAppendBody,queueAppendFixups);queueBody(0x8715f0,queueAckBody,queueAckFixups);
}
void initializePools(bool halted) {
    pools.clear();pools.resize(workers.size());
    for(unsigned i=0;i<workers.size();++i) {
        const auto w=workerGetter(i),p=reinterpret_cast<uintptr_t>(pools[i].data());
        put(reinterpret_cast<void*>(w+0x34),uint16_t{14});put(reinterpret_cast<void*>(w+0x88),p);
        for(unsigned j=0;j<3;++j) {put(reinterpret_cast<void*>(p+j*32),j==2?uintptr_t{0}:p+(j+1)*32);put(reinterpret_cast<void*>(p+j*32+4),j?p+(j-1)*32:w+0x88);}
        if(halted) {
            put(reinterpret_cast<void*>(w+0x80),p);put(reinterpret_cast<void*>(w+0x90),p);put(reinterpret_cast<void*>(w+0x88),p+32);
            put(reinterpret_cast<void*>(p),uintptr_t{0});put(reinterpret_cast<void*>(p+4),w+0x80);put(reinterpret_cast<void*>(p+36),w+0x88);
            put(reinterpret_cast<void*>(p+8),uint16_t{1});put(reinterpret_cast<void*>(p+10),uint16_t{0xffff});put(reinterpret_cast<void*>(p+14),uint8_t{0x20});
        }
    }
}
struct Case {Setting setting;unsigned hash,local,entry,pc,index,initial1,initial2;std::string file;};
Setting settingId(const std::string& s) {
    return s=="ChocoboAssist"?Setting::ChocoboAssist:s=="RemiemAssist"?Setting::RemiemAssist:s=="ButterflyAssist"?Setting::ButterflyAssist:Setting::BlitzAssist;
}
void releasePending(bool execute=true) {
    while(!pending.empty()) {
        auto a=pending.take();bool okay=false;
        if(execute && a.epoch==generation() && a.prepare(a)) {a.invoke(a);okay=true;}
        if(a.complete) a.complete(a,okay);if(a.retire) a.retire(a);
    }
}
void load(const Case& c,bool drain=true,bool changed=false) {
    releasePending(false);resetMinigameCompletion();invalidateNativeWorkerIndex();
    if(!ev.empty()) {Event e;e.args[0]=reinterpret_cast<uintptr_t>(ev.data());listeners.at(0x48d490).before(e);}
    std::ifstream input(ffx_fixture::evidenceRoot()/"offline-output/FFX/20261003_audit/files"/c.file,std::ios::binary);
    check(input.good(),"original complete archived minigame EV");ev.assign(std::istreambuf_iterator<char>(input),{});
    const auto parent=reinterpret_cast<uintptr_t>(ev.data());chunk=parent+word<unsigned>(parent+4);
    code=chunk+word<unsigned>(chunk+0x30);codeLength=word<unsigned>(chunk);
    if(changed) *reinterpret_cast<uint8_t*>(code)^=1;
    ++epoch;controller={};saved={};
    const unsigned count=word<uint16_t>(chunk+0x34);workers.clear();workers.resize(count);signals.clear();signals.resize(count);
    for(unsigned i=0;i<count;++i) {
        put(workers[i].data(),chunk+word<unsigned>(chunk+0x38+i*4));put(workers[i].data()+4,chunk);
        put(workers[i].data()+0x2e,static_cast<uint16_t>(i));put(workers[i].data()+0x34,uint16_t{2});
    }
    put(controller.data()+0xc,static_cast<uint16_t>(count));put(controller.data()+0xa,static_cast<uint16_t>(c.local));
    put(controller.data()+0x2c,reinterpret_cast<uintptr_t>(saved.data()));
    Event loadEvent;loadEvent.args[0]=12;loadEvent.nativeStack=0x910000;loadEvent.epoch=epoch;listeners.at(0x48d230).before(loadEvent);
    Event allocation;allocation.caller=0x48d2a4;allocation.edi=ev.size();allocation.args[0]=(allocation.edi+0x7ffu)&~0x7ffu;allocation.epoch=epoch;allocation.result=parent;
    listeners.at(0x2871c0).before(allocation);listeners.at(0x2871c0).after(allocation);loadEvent.result=parent;listeners.at(0x48d230).after(loadEvent);
    if(drain) {nativeOwner=false;drainAtelResourcesForTest();nativeOwner=true;}
    initializePools(false);
    preferences={};revisions={};focused=true;menu=battle=console=queueFull=false;mutations=requestCount=dispatchCount=0;
}
UtilityScript script(const Case& c) {UtilityScript s;check(captureUtilityScript(s),"actual captured original script");s.worker=workerGetter(c.local);s.header=read<uintptr_t>(s.worker);return s;}
void set(const Case& c,unsigned low,int v) {
    // Fixture setup also seeds pending resources, independently of the mod's
    // prepared-declaration gate that the actual completion must still pass.
    auto s=script(c);const auto rows=s.chunk+read<unsigned>(s.header+0x14);const auto count=read<uint16_t>(s.header+2);
    for(unsigned i=0;i<count;++i) if(read<unsigned>(rows+i*8)==low && read<unsigned>(rows+i*8+4)==1) {nativeSetter(s.worker,i,0,v,0);return;}
    fprintf(stderr,"Missing fixture seed declaration low=%x file=%s\n",low,c.file.c_str());check(false,"original seed declaration");
}
int get(const Case& c,unsigned low) {auto s=script(c);return utilityValue(s,low,1);}
void setup(const Case& c,unsigned course=4) {
    if(c.setting==Setting::ChocoboAssist) {
        set(c,0x0c0000bc,course);set(c,0x0d0000c9,0);set(c,0x0c0000e1,1);
    } else if(c.setting==Setting::ButterflyAssist) {set(c,0x0c000041,1);set(c,0x0c000042,0);}
    else if(c.setting==Setting::BlitzAssist) {set(c,0x1452,0);set(c,0x1453,3);set(c,0x1454,0);set(c,0x5c000148,3000);}
    mutations=requestCount=dispatchCount=0;
}
void emit(const Case& c,unsigned pc=0,unsigned entry=0,bool scoped=true,bool observer=true,int chosen=1) {
    if(!pc) pc=c.pc;if(!entry) entry=c.entry;
    const auto w=workerGetter(c.local);auto& sig=signals[c.local];sig={};
    put(sig.data()+8,static_cast<uint16_t>(entry));put(sig.data()+14,uint8_t{1});put(reinterpret_cast<void*>(w+0x90),reinterpret_cast<uintptr_t>(sig.data()));
    put(reinterpret_cast<void*>(w+0x32),uint8_t{1});put(reinterpret_cast<void*>(w+0x12c+0x4c+0x18),code+pc);
    Event vm;vm.epoch=epoch;vm.args[0]=w;
    if(observer && scoped) listeners.at(0x464180).before(vm);
    Event e;e.rva=0x4713a0;e.parentRva=0x464180;e.epoch=epoch;e.args[0]=w;e.args[1]=c.index;e.args[3]=static_cast<uintptr_t>(chosen);
    reinterpret_cast<void(__cdecl*)(uintptr_t,int,int,int,int)>(functions.at(0x8713a0))(w,c.index,0,static_cast<int>(e.args[3]),0);
    if(observer) listeners.at(0x4713a0).after(e);
    if(observer && scoped) listeners.at(0x464180).after(vm);
}
void enable(const Case& c) {setSetting(c.setting,1);}
void success(const Case& c,unsigned course=4,int chosen=1) {
    const auto writesBefore=mutations;emit(c,0,0,true,true,chosen);check(pending.size()==1 && mutations==writesBefore,"native observation queues without mutation");
    releasePending();check(mutations>writesBefore && mutations-writesBefore<=8,"bounded original typed writes enter only during invocation");
    if(c.setting==Setting::ChocoboAssist) {
        check(requestCount==(course==4?2u:1u) && receipts[0].local==109 && receipts[0].entry==2,"all courses enter original player finish");
        if(course==4) check(receipts[1].local==108 && get(c,0x0c0000d3)==1,"Catcher enters original opponent finish with strictly negative raw-time inputs");
        check(get(c,0x0c0000b0)==0 && get(c,0x0c0000b1)==0 && get(c,0x0c0000b2)==0,"actual timer result operands");
    } else if(c.setting==Setting::RemiemAssist) {
        const auto flags=get(c,0x01000a9b);unsigned expected=5;
        const unsigned masks[]={8,4,16,32,64};
        if(flags&2) for(unsigned i=0;i<5;++i) if(!(flags&masks[i])) {expected=i+1;break;}
        check(requestCount==1 && receipts[0].local==102 && receipts[0].entry==2 && get(c,0x0c00000a)==0 && get(c,0x0c00000b)==static_cast<int>(expected),"first victory/next unearned native chest tier and zero-pole original finish");
    } else if(c.setting==Setting::ButterflyAssist) {
        check(requestCount==1 && receipts[0].entry==6 && receipts[0].local==(workers.size()==55?36u:25u) && get(c,0x0c000042)==7,"original counter display receives current course's seventh pickup");
        for(unsigned low=0x0c000043;low<=0x0c000049;++low) check(get(c,low)==1,"all seven original pickup guards prevent count8");
    } else check(!requestCount && get(c,0x1452)>get(c,0x1453) && get(c,0x1454)==1 && get(c,0x5c000144)==get(c,0x5c000148),"native final-period expiration and player lead");
    check(dispatchCount==requestCount,"each accepted request reaches native dispatch");
    const auto done=mutations;emit(c);releasePending();check(mutations==done && pending.empty(),"same attempt is one-shot");
}
}
namespace ffx {
std::atomic<int> scene{0};
uintptr_t address(unsigned rva) {
    if(rva==0x46c500) return reinterpret_cast<uintptr_t>(&variablePointer);
    if(rva==0x4713a0) return reinterpret_cast<uintptr_t>(&nativeSetter);
    if(rva==0x46e920) return reinterpret_cast<uintptr_t>(&nativeRequest);
    if(rva==0x465390) return reinterpret_cast<uintptr_t>(&nativeDispatch);
    if(rva==0xd2ca90) return reinterpret_cast<uintptr_t>(&room);
    return fixtureUtilityAddress(rva);
}
bool utilityBusy() {return false;}
bool hook(unsigned rva,Handler before,Handler after) {listeners[rva]={before,after};return true;}
bool deferNativeAction(NativeAction a) {return !queueFull && pending.push(a);}
int setting(Setting id) {return preferences[static_cast<unsigned>(id)];}
unsigned settingRevision(Setting id) {return revisions[static_cast<unsigned>(id)];}
void setSetting(Setting id,int v) {const auto i=static_cast<unsigned>(id);if(preferences[i]!=v) ++revisions[i];preferences[i]=v;}
bool gameFocused() {return focused;}bool inBattle() {return battle;}bool modMenuActive() {return menu;}bool utilityConsoleActive() {return console;}
}
int main() {try {
    using namespace ffx;check(startAtelResources(),"resource bootstrap");installAtelResources();installMinigameCompletion();originalFunctions();prepareOriginalQueue();
    std::ifstream in(ffx_fixture::evidenceRoot()/"offline-output/minigame_completion_20261020/cases.txt");check(in.good(),"independently verified original cases");
    std::vector<Case> cases;std::string kind;Case c{};
    while(in>>kind>>c.hash>>c.local>>c.entry>>c.pc>>c.index>>c.initial1>>c.initial2>>std::quoted(c.file)) {c.setting=settingId(kind);cases.push_back(c);}
    check(cases.size()==9,"all catalogued original variants");
    for(const auto& test:cases) {
        load(test);check(scriptChecksum(script(test))==test.hash,"prepared exact native code identity");setup(test);
        check(minigameAssistAvailable(test.setting),"pre-activity switch available");emit(test);releasePending();check(!mutations && !requestCount,"default Off leaves native behavior");enable(test);
        emit(test,test.pc+3);emit(test,0,test.entry+1);emit(test,0,0,false);check(pending.empty(),"adjacent PC, wrong entry and missing interpreter refuse");
        emit(test);setSetting(test.setting,0);setSetting(test.setting,1);releasePending();check(!mutations,"Off/On round trip cancels queued completion");
        for(bool* gate:{&menu,&battle,&console}) {emit(test);check(pending.size()==1,"fresh live event accepted");*gate=true;releasePending();*gate=false;check(!mutations,"changed native context refuses pending completion");}
        emit(test);focused=false;releasePending();focused=true;check(!mutations,"lost focus cancels pending work");
        queueFull=true;emit(test);queueFull=false;check(pending.empty() && !mutations,"full action queue has no side effects");
        auto s=script(test);const auto row=s.chunk+read<unsigned>(s.header+0x14)+test.index*8+4;const auto old=read<unsigned>(row);write<unsigned>(row,2);emit(test);check(pending.empty(),"changed live declaration refused");write<unsigned>(row,old);
        emit(test);cancelMinigameCompletion();releasePending();check(!mutations,"open/closed context retires queued work permanently");
        if(test.setting!=Setting::BlitzAssist) {
            const auto local=test.setting==Setting::ChocoboAssist?109u:test.setting==Setting::RemiemAssist?102u:workers.size()==55?36u:25u;
            const auto w=workerGetter(local);const auto flags=read<uint16_t>(w+0x34);write<uint16_t>(w+0x34,flags&~2u);
            emit(test);releasePending();check(!mutations,"disabled native target refuses all writes");write<uint16_t>(w+0x34,flags);
            const auto free=read<uintptr_t>(w+0x88);write<uintptr_t>(w+0x88,0);emit(test);releasePending();check(!mutations,"full native target queue refuses before writes");write<uintptr_t>(w+0x88,free);
        }
        success(test);
        load(test,false);setup(test);enable(test);emit(test);releasePending();check(!mutations,"pending resource refuses assistance");
        nativeOwner=false;drainAtelResourcesForTest();nativeOwner=true;success(test);
        load(test,true,true);setup(test);enable(test);emit(test);releasePending();check(!mutations,"changed code rejects old completion facts");
        load(test);setup(test);enable(test);emit(test);++epoch;releasePending();check(!mutations,"generation change retires queued work");
        if(test.setting==Setting::ChocoboAssist) for(unsigned course=1;course<=4;++course) {load(test);setup(test,course);enable(test);success(test,course);}
        if(test.setting==Setting::RemiemAssist) for(unsigned mask=0;mask<64;++mask) {
            load(test);setup(test);set(test,0x01000a9b,static_cast<int>(mask*2));mutations=0;enable(test);success(test);
            check(get(test,0x01000a9b)==static_cast<int>(mask*2),"assist never grants native first-win/prize flags");
        }
        if(test.setting==Setting::ButterflyAssist) for(int story:{0,1454,1455,2875,2876,9999}) {
            load(test);setup(test);set(test,0x20000a00,story);mutations=0;enable(test);success(test);
            check(get(test,0x20000a00)==story,"hunt assistance never rewrites story phase");
        }
        if(test.setting==Setting::BlitzAssist) {
            for(int chosen:{-1,0,1,2,3,255}) {
                load(test);setup(test);enable(test);
                if(chosen>=0 && chosen<=2) success(test,4,chosen);
                else {emit(test,0,0,true,true,chosen);releasePending();check(!mutations,"cancel/invalid action return cannot trigger a win");}
            }
            for(int story:{535,560,575,582,9999}) {
                load(test);setup(test);set(test,0x20000a00,story);mutations=0;enable(test);success(test);
                check(get(test,0x20000a00)==story,"Blitzball assistance leaves native story progression alone");
            }
            for(int half=0;half<=1;++half) for(int own:{0,3,98,99}) for(int opponent:{0,3,98,99}) {
                load(test);setup(test);set(test,0x1452,own);set(test,0x1453,opponent);set(test,0x1454,half);mutations=0;enable(test);success(test);
            }
            load(test);setup(test);enable(test);set(test,0x5c000148,0);mutations=0;emit(test);releasePending();check(!mutations,"invalid clock bound refuses score mutation");
        }
    }
    for(const auto& test:cases) if(test.setting!=Setting::BlitzAssist) for(bool halted:{false,true}) {
        load(test);setup(test);enable(test);initializePools(halted);realQueue=true;success(test);realQueue=false;
    }
    // Compare the same original event and typed setter without the observer,
    // default-Off, and a handled On attempt. Native finish-entry readiness is
    // measured separately; visible results still depend on the game's VM.
    const auto test=cases.back();
    for(bool large:{false,true}) {
        load(test);setup(test);enable(test);
        if(large) {workers.resize(500);put(controller.data()+0xc,uint16_t{500});invalidateNativeWorkerIndex();}
        initializePools(false);realQueue=true;
        allocations=frees=0;observe=true;const auto begin=std::chrono::steady_clock::now();emit(test);releasePending();observe=false;
        check(!allocations && !frees && dispatchCount==1,"cold original finish dispatch has no native heap work");
        printf("METRIC %s event through first original finish-entry initialization %.3fus; heap0/0.\n",large?"cold500-worker":"cold normal",std::chrono::duration<double,std::micro>(std::chrono::steady_clock::now()-begin).count());
        realQueue=false;
    }
    for(unsigned mode=0;mode<3;++mode) {
        setSetting(test.setting,mode==1?0:1);
        allocations=frees=0;observe=true;auto begin=std::chrono::steady_clock::now();for(unsigned i=0;i<1000;++i) emit(test,0,0,true,mode!=0);observe=false;
        check(!allocations && !frees && pending.empty(),"warm native event has no backlog or native heap");
        printf("METRIC %s callbacks1000 %.3fms; heap0/0.\n",mode==0?"original baseline":mode==1?"default-Off":"handled On",std::chrono::duration<double,std::milli>(std::chrono::steady_clock::now()-begin).count());
    }
    puts("PASS: nine original activity variants, four courses, halves/score bounds, default-Off, exact native data, cancellation, one-shot and refusal matrix. Original typed readers/setter; original request/list/scheduler/init replay from idle and halted endpoints; VM scheduling, animations and gameplay effects controlled.");return 0;
} catch(const std::exception& e) {observe=false;fprintf(stderr,"FAIL: %s\n",e.what());return 1;}}
