#include "external_fixture.hpp"
#include "../reachability.hpp"
#include "../reachability_shared.hpp"
#include "../settings.hpp"
#include <algorithm>
#include <atomic>
#include <cstdio>
#include <cstdlib>
#include <map>
#include <thread>
#include <chrono>
#include <new>
using namespace ffx;
thread_local bool observingNative=false;
unsigned nativeAllocations=0,nativeFrees=0;
void* operator new(size_t size) {if(observingNative) ++nativeAllocations;if(auto p=std::malloc(size?size:1)) return p;throw std::bad_alloc{};}
void* operator new[](size_t size) {return ::operator new(size);}
void operator delete(void* p) noexcept {if(p && observingNative) ++nativeFrees;std::free(p);}
void operator delete[](void* p) noexcept {::operator delete(p);}
void operator delete(void* p,size_t) noexcept {::operator delete(p);}
void operator delete[](void* p,size_t) noexcept {::operator delete(p);}
namespace {
std::vector<uint8_t> memory(0x2500000);
DWORD nativeOwner=0;
unsigned epoch=1,changes=0;
int enabled=1;
std::atomic<bool> stop{false},block{false};
std::atomic<unsigned> searches{0},wrongThread{0};
HANDLE entered=nullptr,release=nullptr;
std::map<unsigned,Handler> before;
void check(bool condition,const char* message) { if(!condition) { std::fprintf(stderr,"FAIL: %s\n",message);std::exit(1); } }
template<class T> void put(unsigned rva,T value) { std::memcpy(memory.data()+rva,&value,sizeof(value)); }
void pump(unsigned n=1) { for(unsigned i=0;i<n;++i) { advanceReachabilityForTest();Sleep(1); } }
void waitHidden(std::vector<Entity>& list,bool hidden) {
    const auto deadline=GetTickCount64()+5000;
    while(list[0].unreachable!=hidden && GetTickCount64()<deadline) pump();
    check(list[0].unreachable==hidden,"filter result ready");
}
void waitComplete() {
    const auto deadline=GetTickCount64()+5000;
    while(!reachabilityCompleteForTest() && GetTickCount64()<deadline) pump();
    check(reachabilityCompleteForTest(),"worker request complete");pump(5);
}
void geometry() {
    put<uintptr_t>(0xf01a44,address(0x2200000));put<int>(0xf01a48,30000);
    put<uintptr_t>(0xf01a4c,address(0x2300000));put<float>(0xf01a50,1);
    put<uintptr_t>(0xf01a54,address(0x2400100));put<int16_t>(0x240010a,6);
    RawNavigationTriangle a{};a.vertices[0]=0;a.vertices[1]=1;a.vertices[2]=2;
    for(auto& v:a.neighbours) v=-1;
    RawNavigationTriangle b=a;b.vertices[0]=3;b.vertices[1]=4;b.vertices[2]=5;
    for(unsigned i=0;i<30000;++i) put(0x2200000+i*16,i==1?b:a);
    const int16_t points[6][3]={{0,0,0},{10,0,0},{0,0,10},{20,0,20},{30,0,20},{20,0,30}};
    for(unsigned i=0;i<6;++i) { RawNavigationVertex p{};std::copy(points[i],points[i]+3,p.coordinates);put(0x2300000+i*8,p); }
}
}
namespace ffx {
std::atomic<int> scene{0};
uintptr_t address(unsigned rva) { return reinterpret_cast<uintptr_t>(memory.data()+rva); }
bool copy(uintptr_t p,void* d,size_t n) {
    if(GetCurrentThreadId()!=nativeOwner) { ++wrongThread;return false; }
    const auto base=reinterpret_cast<uintptr_t>(memory.data());
    if(n>0x10000 || p<base || p-base>memory.size() || n>memory.size()-(p-base)) return false;
    std::memcpy(d,reinterpret_cast<const void*>(p),n);return true;
}
uintptr_t playerActor() { return address(0x2100000); }
Point playerPosition() { return {read<float>(playerActor()+0xc),read<float>(playerActor()+0x10),read<float>(playerActor()+0x14)}; }
bool nativeGameThread() { return GetCurrentThreadId()==nativeOwner; }
unsigned generation() { return epoch; }
bool readyAtelIdentity(uintptr_t,AtelIdentity&) {return false;} // This host has no loaded field script.
bool readyAtelWorkerHeader(uintptr_t,unsigned,unsigned&) {std::abort();}
NativeInteractionFacts captureNativeInteractionFacts(uintptr_t,EntryMask,const AtelSpan&) {std::abort();}
bool inBattle() { return false; }
int setting(Setting id) { return id==Setting::HideUnreachable?enabled:0; }
bool hook(unsigned rva,Handler first,Handler) { if(first) before[rva]=first;return true; }
bool hookSnapshot(unsigned rva,Handler first,Handler last) {return hook(rva,first,last);}
void navigationReachabilityChanged() { ++changes; }
void log(const char*,...) {}
bool fixtureDestination(const Mesh&,Point,const Entity&);
bool destinationUnreachable(const Mesh& mesh,Point p,const Entity& e) {
    ++searches;
    if(GetCurrentThreadId()==nativeOwner) ++wrongThread;
    if(block.exchange(false)) { SetEvent(entered);check(WaitForSingleObject(release,5000)==WAIT_OBJECT_0,"blocked worker released"); }
    return fixtureDestination(mesh,p,e);
}
}
#include "collision_native_fixture.inc"
int main() {
    nativeOwner=GetCurrentThreadId();entered=CreateEventW(nullptr,TRUE,FALSE,nullptr);release=CreateEventW(nullptr,TRUE,FALSE,nullptr);
    check(entered && release,"fixture synchronization");
    put<uint16_t>(0x2100002,1);put<int16_t>(0x2100824,0);put<float>(0x210000c,2);put<float>(0x2100010,0);put<float>(0x2100014,2);
    put<uintptr_t>(0xf00740,playerActor());put<uintptr_t>(0xf26ae8,address(0x2400000));put<uint8_t>(0x2400000,2);
    geometry();installReachability();
    std::thread worker([] { while(!stop) { drainReachabilityForTest();Sleep(1); }drainReachabilityForTest(); });
    std::vector<Entity> list(2);list[0].id=1;list[0].worker=3;list[0].position={22,0,22};list[0].category=L"Object";
    list[1].id=2;list[1].worker=4;list[1].position={200,0,200};list[1].category=L"Exit";
    block=true;requestReachability(list,playerPosition());
    const auto deadline=GetTickCount64()+5000;
    while(WaitForSingleObject(entered,0)!=WAIT_OBJECT_0 && GetTickCount64()<deadline) pump();
    check(WaitForSingleObject(entered,0)==WAIT_OBJECT_0,"route runs on worker");
    // Native owner continues while a real route worker is deliberately blocked.
    enabled=0;pump();check(!list[0].unreachable && !list[1].unreachable,"Off retains all targets without waiting");
    enabled=1;
    for(unsigned i=0;i<40;++i) { requestReachability(list,playerPosition());advanceReachabilityForTest(); }
    check(WaitForSingleObject(release,0)==WAIT_TIMEOUT,"native queue pressure never waits for worker");
    resetReachability();SetEvent(release);pump(30);
    requestReachability(list,playerPosition());waitHidden(list,true);
    check(!list[1].unreachable,"unknown boundary exit retained");
    waitComplete();const auto count=searches.load();
    list[0].unreachable=false;requestReachability(list,playerPosition());waitHidden(list,true);waitComplete();
    check(searches==count,"unchanged geometry, floor, gate, player and target reuse route result");
    // An actual gate mutation invalidates the context and cached route verdict.
    put<unsigned>(0xf01998+49*4,2);pump();check(!list[0].unreachable,"gate change retires hidden verdict");
    requestReachability(list,playerPosition());waitHidden(list,true);check(searches>count,"changed gate recomputed");
    // Native resource replacement invalidates both raw geometry and verdicts.
    Event e;
    const auto oldRequest=reach::latest.load();
    std::thread foreignMesh([&] {before.at(0x43ea00)(e);advanceReachabilityForTest();});foreignMesh.join();
    check(reach::latest.load()!=oldRequest && list[0].unreachable,"foreign mesh replacement publishes retirement without mutating owner targets");
    pump();check(!list[0].unreachable,"owner consumes foreign mesh retirement");
    requestReachability(list,playerPosition());list[0].unreachable=true;
    std::thread foreignReset([] {resetReachability();});foreignReset.join();
    check(list[0].unreachable,"foreign reset does not clear owner targets");
    pump();check(!list[0].unreachable,"owner consumes foreign reset retirement");
    before.at(0x43ea00)(e);check(!list[0].unreachable,"owner mesh replacement retires verdict");
    put<float>(0x210000c,22);put<float>(0x2100014,22);put<int16_t>(0x2100824,1);
    requestReachability(list,playerPosition());pump(200);check(!list[0].unreachable,"new controlled floor retained");
    ++epoch;pump();check(!list[0].unreachable,"stale generation retained");
    list[0].position={2,0,2};put<float>(0x210000c,2);put<float>(0x2100014,2);put<int16_t>(0x2100824,0);
    requestReachability(list,playerPosition());waitComplete();check(!list[0].unreachable,"reachable target remains listed");
    list[0].id=7;list[0].position={22,0,22};block=true;ResetEvent(entered);ResetEvent(release);
    requestReachability(list,playerPosition());const auto targetDeadline=GetTickCount64()+5000;
    while(WaitForSingleObject(entered,0)!=WAIT_OBJECT_0 && GetTickCount64()<targetDeadline) pump();
    check(WaitForSingleObject(entered,0)==WAIT_OBJECT_0,"changed target worker blocked");
    list[0].position={2,0,2};SetEvent(release);waitComplete();
    check(!list[0].unreachable,"late verdict cannot hide a moved target");
    // The actual native actor can move without a refreshed mod-owned Entity.
    // The final owner check must refuse its old worker verdict in that case.
    put<uintptr_t>(0x1fc44e4,address(0x2000000));put<unsigned>(0x1fc44e0,2);
    list[0].actor=1;list[0].position={22,0,22};
    put<uint16_t>(0x2000880,7);put<uint16_t>(0x2000882,1);put<unsigned>(0x20010a4,0);
    put<float>(0x200088c,22);put<float>(0x2000890,0);put<float>(0x2000894,22);
    block=true;ResetEvent(entered);ResetEvent(release);
    requestReachability(list,playerPosition());
    check(WaitForSingleObject(entered,5000)==WAIT_OBJECT_0,"native actor stale verdict worker held");
    put<float>(0x200088c,2);put<float>(0x2000894,2);SetEvent(release);waitComplete();
    check(!list[0].unreachable,"fresh native actor identity/pose prevents stale hidden verdict even when Entity is unchanged");
    list[0].actor=-1;list[0].position={2,0,2};
    // Accepted post-physics movement retires the old request and captures the
    // complete new pose in one native call. An unchanged commit keeps its ID.
    put<float>(0x210000c,3);
    const auto previousRequest=reach::latest.load();
    observingNative=true;refreshReachabilityAfterMovement(list);observingNative=false;
    check(reach::latest.load()!=previousRequest,"post-physics pose starts fresh reachability");
    const auto movementRequest=reach::latest.load();
    observingNative=true;refreshReachabilityAfterMovement(list);observingNative=false;
    check(reach::latest.load()==movementRequest,"unchanged post-physics pose preserves in-flight request");
    waitComplete();check(!list[0].unreachable,"post-physics reachable result uses current pose");
    // Measure the actual capture -> worker algorithms -> native commit, without
    // the intentional held-worker delay. This fixture floor is synthetic and
    // does not qualify real level collection, audible speech or live latency.
    std::vector<Entity> large(512);
    for(unsigned i=0;i<large.size();++i) {large[i].id=i+20;large[i].worker=i;large[i].category=L"Object";large[i].position={22,0,22};}
    const auto measure=[&](const char* label,std::vector<Entity>& targets) {
        const auto start=std::chrono::steady_clock::now();
        observingNative=true;requestReachability(targets,playerPosition());observingNative=false;
        const auto ready=std::chrono::steady_clock::now();
        auto firstUsable=ready;bool usable=false;
        const auto until=GetTickCount64()+20000;
        while(!reachabilityCompleteForTest() && GetTickCount64()<until) {
            observingNative=true;advanceReachabilityForTest();observingNative=false;
            if(!usable && targets.front().unreachable) {firstUsable=std::chrono::steady_clock::now();usable=true;}Sleep(1);
        }
        check(reachabilityCompleteForTest(),"whole bounded target batch finishes");
        observingNative=true;advanceReachabilityForTest();observingNative=false;
        const auto end=std::chrono::steady_clock::now();
        for(const auto& target:targets) check(target.unreachable,"complete batch verdict committed without per-input assembly");
        std::printf("METRIC %s targets=%u triangles=30000 capture-ms=%.3f first-commit-ms=%.3f complete-and-commit-ms=%.3f native-new=%u native-delete=%u\n",label,static_cast<unsigned>(targets.size()),
            std::chrono::duration<double,std::milli>(ready-start).count(),std::chrono::duration<double,std::milli>((usable?firstUsable:end)-start).count(),std::chrono::duration<double,std::milli>(end-start).count(),nativeAllocations,nativeFrees);
    };
    before.at(0x43ea00)(e);measure("cold",large);measure("warm",large);
    check(!nativeAllocations && !nativeFrees,"native snapshot, cancellation and result commit allocate and free no C++ owners");
    std::vector<Entity> tooMany(513);requestReachability(tooMany,playerPosition());
    check(!reachabilityChecking(),"whole oversized batch is rejected without partial hidden targets");
    resetReachability();besaidCollisionFixtures(list);
    put<uintptr_t>(0xf00740,0);pump();check(!reachabilityChecking(),"control loss retires request");
    resetReachability();stop=true;worker.join();CloseHandle(entered);CloseHandle(release);
    check(!wrongThread && changes,"worker never reads native memory and filter commits on owner");
    std::puts("PASS: async reachability, blocked worker, pressure, cache, floor/gate/resource/epoch invalidation and unknown exit retention.");
}
