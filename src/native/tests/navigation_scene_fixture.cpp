#include <atomic>
// Actual production scene capture/resource leases/proofs/labels/geometry, with
// affine getter adapters and retained EV data. No game or SDK is executed.
#include "../navigation_scene.hpp"
#include "../settings.hpp"
#include "../navigation_observations.hpp"
#ifdef FFX_NAVIGATION_SERVICE_TEST
#define deferNativeAction fixtureComponentDefer
#endif
#define address fixtureUtilityAddress
#define fixtureMain utilityBaselineMain
#pragma push_macro("main")
#undef main
#define main utilityBaselineEntry
#include "utility_resource_fixture.cpp"
#undef main
#pragma pop_macro("main")
#undef fixtureMain
#undef address
#ifdef FFX_NAVIGATION_SERVICE_TEST
#undef deferNativeAction
#endif
namespace {
std::array<ffx::nav::ActorBody,128> actorBodies{};
ffx::nav::Scene sceneCapture;
uintptr_t actorsPointer=0,controlledPointer=0,geometryPointer=0,cameraPointer=0,trianglesPointer=0,verticesPointer=0,rootPointer=0;
unsigned actorCapacity=128;int triangleCount=0;float geometryScale=1;
std::array<uint8_t,0x68c0> savedData{};
std::array<unsigned,65> collision{};
unsigned nativeEntries=0;bool huntFixture=false,leavingFixture=false;
std::array<uint8_t,40> personBytes{};
std::array<uint8_t,256> areaBytes{};uint16_t nameTableSize=0xfffc;unsigned areaGetterCalls=0;
#ifdef FFX_NAVIGATION_REAL_QUEUE_TEST
std::atomic<unsigned> manualInputRevision{1};std::atomic<bool> fixtureAutoWalk{false};
#else
unsigned manualInputRevision=1;
#endif
std::map<unsigned,std::vector<Listener>> navigationListeners;std::map<unsigned,void(*)(bool)> navigationKeys;
std::vector<std::wstring> navigationSaid;
uintptr_t __cdecl nameTableGetter(unsigned) {if(!nativeOwner) ++foreignReads;return reinterpret_cast<uintptr_t>(&nameTableSize);}
uintptr_t __cdecl areaNameGetter(unsigned,unsigned) {if(!nativeOwner) ++foreignReads;++areaGetterCalls;return reinterpret_cast<uintptr_t>(areaBytes.data());}
uintptr_t __cdecl personGetter(unsigned) {if(!nativeOwner) ++foreignReads;return reinterpret_cast<uintptr_t>(personBytes.data());}
uint8_t __cdecl priorityGetter(unsigned,unsigned) {if(!nativeOwner) ++foreignReads;return 0;}
uint16_t __cdecl interactionGetter(unsigned,unsigned channel,uintptr_t) {if(!nativeOwner) ++foreignReads;++nativeEntries;return static_cast<uint16_t>(huntFixture && channel==2?4:channel+1);}
void setScene() {
    actorsPointer=reinterpret_cast<uintptr_t>(actorBodies.data());controlledPointer=actorsPointer;put(controller.data()+0x48,chunk);
    put(controller.data()+0x2c,reinterpret_cast<uintptr_t>(savedData.data()+0x1ec));put(controller.data(),static_cast<uint8_t>(2));
    put(savedData.data(),static_cast<uint16_t>(71));put(savedData.data()+0xe,static_cast<uint16_t>(20));
    for(unsigned i=0;i<actorBodies.size();++i) {auto& b=actorBodies[i].bytes;b={};put(b.data(),static_cast<uint16_t>(i?0x5001:1));put(b.data()+2,static_cast<uint16_t>(1));
        put(b.data()+0xc,static_cast<float>(i+1));put(b.data()+0x10,0.f);put(b.data()+0x14,static_cast<float>(i+1));put(b.data()+0x824,static_cast<int16_t>(0));
        put(b.data()+0x5c,1.f);put(b.data()+0x64,1.f);put(b.data()+0x4e4,1.f);}
    actorBodies[0].bytes[0x186]=2;
    for(unsigned i=0;i<workers.size();++i) {
        auto& w=workers[i];put(w.data()+0x34,static_cast<uint16_t>(0x80));w[0xaa]=1;w[0xab]=0x7f;
        put(w.data()+0x9c,actorsPointer+((i%127)+1)*0x880);
        for(unsigned at:{0x558u,0x284u}) {put(w.data()+at,static_cast<float>(i+1));put(w.data()+at+4,0.f);put(w.data()+at+8,static_cast<float>(i+1));
            put(w.data()+at+0x1c,10.f);put(w.data()+at+0x2c,10.f);put(w.data()+at+0x3c,3.f);put(w.data()+at+0x40,10.f);put(w.data()+at+0x48,10.f);}
    }
}
}
namespace ffx {
std::atomic<int> scene{0};
uintptr_t address(unsigned rva) {
    if(!nativeOwner) {++foreignReads;return 0;}
    if(rva==0x1fc44e4) return reinterpret_cast<uintptr_t>(&actorsPointer);
    if(rva==0x1fc44e0) return reinterpret_cast<uintptr_t>(&actorCapacity);
    if(rva==0xf00740) return reinterpret_cast<uintptr_t>(&controlledPointer);
    if(rva==0xf0077d) {static uint8_t value=1;return reinterpret_cast<uintptr_t>(&value);}
    if(rva==0xd2ca90) return reinterpret_cast<uintptr_t>(savedData.data());
    if(rva==0xf01a48) return reinterpret_cast<uintptr_t>(&triangleCount);
    if(rva==0xf01a44) return reinterpret_cast<uintptr_t>(&trianglesPointer);
    if(rva==0xf01a4c) return reinterpret_cast<uintptr_t>(&verticesPointer);
    if(rva==0xf01a54) return reinterpret_cast<uintptr_t>(&rootPointer);
    if(rva==0xf01a50) return reinterpret_cast<uintptr_t>(&geometryScale);
    if(rva==0xf01998) return reinterpret_cast<uintptr_t>(collision.data());
    if(rva==0x1f11430) return reinterpret_cast<uintptr_t>(&cameraPointer);
    if(rva==0x4b7f10) return reinterpret_cast<uintptr_t>(&nameTableGetter);
    if(rva==0x46c3c0) return reinterpret_cast<uintptr_t>(&areaNameGetter);
    if(rva==0x385070) return reinterpret_cast<uintptr_t>(&personGetter);
    if(rva==0x4767e0) return reinterpret_cast<uintptr_t>(&priorityGetter);
    if(rva==0x476360) return reinterpret_cast<uintptr_t>(&interactionGetter);
    return fixtureUtilityAddress(rva);
}
bool butterflyNavigationLeaving(const AtelSpan&) {return leavingFixture;}
int setting(Setting id) {
#ifdef FFX_NAVIGATION_REAL_QUEUE_TEST
    return id==Setting::Speech || (id==Setting::AutoWalk && fixtureAutoWalk.load())?1:0;
#else
    (void)id;return 1;
#endif
}
unsigned autoWalkInputRevision() {return manualInputRevision;}
bool inBattle() {return false;}bool gameFocused() {return true;}bool modMenuActive() {return false;}bool utilityConsoleActive() {return false;}
#ifndef FFX_NAVIGATION_SERVICE_TEST
bool nativeReaderResourcesReady() noexcept {return true;}
#endif
void prepareCopiedControlSnapshot(CopiedControls& controls) {controls={};}
OwnedTextContext copiedControlContext(const CopiedControls&) {return {};}
unsigned pedestalConfirmMask() {return 0x20;}
PedestalNavigationState capturePedestalNavigationState() {return {};}
void preparePedestalZone(Entity&,uintptr_t) {}
void refreshSwimmingGoal(Entity&) {}
std::wstring nativeAreaName(int) {++foreignReads;return {};}
std::wstring playerName(int) {++foreignReads;return {};}
#ifdef FFX_NAVIGATION_REAL_QUEUE_TEST
bool output(const std::wstring& text,bool interrupt,const char* producer) {return speechq::output(text.data(),text.size(),interrupt,producer,false,generation());}
bool output(const wchar_t* text,bool interrupt,const char* producer) {return speechq::output(text,std::wcslen(text),interrupt,producer,false,generation());}
#else
bool output(const std::wstring& text,bool,const char*) {navigationSaid.push_back(text);return true;}
bool output(const wchar_t* text,bool,const char*) {navigationSaid.emplace_back(text);return true;}
#endif
bool outputDialogue(const std::wstring& text,bool interrupt,const char* producer,bool) {return output(text,interrupt,producer);}
bool hook(unsigned rva,Handler before,Handler after) {navigationListeners[rva].push_back({before,after});return true;}
std::wstring join(const std::vector<std::wstring>& words,const wchar_t* separator) {std::wstring text;for(const auto& word:words) {if(!text.empty()) text+=separator;text+=word;}return text;}
}
#define hookSnapshot fixtureUnusedHookSnapshot
#ifndef FFX_NAVIGATION_REAL_QUEUE_TEST
#include "test_speech_domains.hpp"
#endif
#undef hookSnapshot
#ifndef FFX_NAVIGATION_SERVICE_TEST
namespace ffx::nav {unsigned navigationIntentRevision() {return 1;}}
#endif
#ifndef FFX_NAVIGATION_REAL_QUEUE_TEST
namespace ffx::speechq {void notifyPreparation() noexcept {}}
#endif
namespace {
std::vector<ffx::nav::QueryObservation> acceptedQueries;
unsigned acceptedScenes=0;
void acceptQuery(const ffx::speechq::Header&,const ffx::nav::QueryObservation& q) {acceptedQueries.push_back(q);}
void acceptScene(const ffx::speechq::Header&,const ffx::nav::SceneObservation& s) {
    ++acceptedScenes;check(s.scene.count>0,"observer delivers owned scene");
    for(const auto& q:acceptedQueries) check(s.requested>=q.requested,"scene demand identifies facts captured after each admitted request");
    ffx::nav::navigationWaitingForData(false);
}
void resetObserver(unsigned) {}
}
#ifndef FFX_NAVIGATION_REAL_QUEUE_TEST
int main() {try {
    using namespace ffx;check(startAtelResources(),"scene resource bootstrap");installAtelResources();
    double maxCapture=0,maxReady=0;unsigned total=0;
    for(const char* name:{"cdsp0000","stbv0000","bsyt0100","klyt1000","djyt0600","mcyt0500","bvyt1100","dome0700"}) {
        prepare(name);setScene();observe=true;const auto begin=std::chrono::steady_clock::now();
        check(nav::captureNavigationScene(sceneCapture,1,true),"actual scene capture");
        const auto captured=std::chrono::steady_clock::now();observe=false;
        const auto originalPosition=sceneCapture.player;for(auto& b:actorBodies) std::fill(b.bytes.begin(),b.bytes.end(),static_cast<uint8_t>(0xff));
        for(auto& w:workers) std::fill(w.begin()+0x558,w.begin()+0x5a4,static_cast<uint8_t>(0xff));
        nativeOwner=false;std::vector<Entity> entities;
        check(nav::scanNavigationScene(sceneCapture,entities,[](int) {return std::wstring(L"Copied native area");},[](int) {return std::wstring(L"Copied native person");}),"owned scene proof and labels ready");
        check(nav::scenePersonName(sceneCapture,0,OwnedTextContext{}).empty(),"copied native empty person name remains empty");
        const auto mesh=nav::decodeNavigationSceneMesh(sceneCapture);check(mesh.openPlane && mesh.fixedHeight && mesh.planeY==originalPosition.y,"actual owned open swimming plane");
        check(!nav::captureNavigationScene(sceneCapture,1,false),"worker cannot enter native scene capture");
        for(const auto& e:entities) check(std::isfinite(e.position.x) && std::isfinite(e.position.y) && std::isfinite(e.position.z),"source reuse cannot change copied entities");
        nativeOwner=true;const auto ready=std::chrono::steady_clock::now();total+=static_cast<unsigned>(entities.size());
        maxCapture=(std::max)(maxCapture,std::chrono::duration<double,std::micro>(captured-begin).count());
        maxReady=(std::max)(maxReady,std::chrono::duration<double,std::milli>(ready-begin).count());
    }
    for(const char* name:{"mcfr0100","mcfr0200"}) {
        prepare(name);setScene();huntFixture=true;put(controller.data()+0x2c,static_cast<uintptr_t>(0));
        for(auto& actor:actorBodies) {auto& b=actor.bytes;put(b.data(),static_cast<uint16_t>(0x5008));put(b.data()+0x354,1.f);put(b.data()+0x358,1.f);
            put(b.data()+0x334,.1f);put(b.data()+0x340,.3f);put(b.data()+0x34c,1.f);}
        for(unsigned active=0;active<2;++active) {
            scalar=active;observe=true;check(nav::captureNavigationScene(sceneCapture,1,false),"real course copied Native scene");observe=false;
            nativeOwner=false;std::vector<Entity> found;check(nav::scanNavigationScene(sceneCapture,found,[](int) {return std::wstring{};},[](int) {return std::wstring{};}),"owned real course scene");
            unsigned blue=0,starter=0;for(const auto& e:found) {blue+=e.butterfly && !e.butterflyStart && !e.hazardous;starter+=e.butterflyStart && !e.inactive;}
            check(active?blue==7 && starter==0:blue==0 && starter==1,"seven current blue targets and actual inactive/active starter guards");nativeOwner=true;
        }
        leavingFixture=true;observe=true;check(nav::captureNavigationScene(sceneCapture,1,false),"copied Native course exit edge");observe=false;
        nativeOwner=false;std::vector<Entity> found;check(nav::scanNavigationScene(sceneCapture,found,[](int) {return std::wstring{};},[](int) {return std::wstring{};}),"owned exiting course scene");
        for(const auto& e:found) check(!e.butterfly || (e.butterflyStart && e.inactive),"original course exit cannot keep hunt target/start active");nativeOwner=true;leavingFixture=huntFixture=false;
    }
    put(controller.data()+0x1a,static_cast<uint16_t>(4));const auto ownerCalls=workerCalls;
    observe=true;check(!nav::captureNavigationScene(sceneCapture,1,false),"segmented controller cannot supply field scene facts");observe=false;
    check(workerCalls==ownerCalls,"wrong field controller refuses before worker getters");put(controller.data()+0x1a,static_cast<uint16_t>(0));
    prepare("cdsp0000");setScene();geometryScale=0;rootPointer=0x1234;
    observe=true;check(nav::captureNavigationScene(sceneCapture,1,true) && sceneCapture.geometryValid,"empty swimming geometry needs no scale or dereferenced retired root");observe=false;
    nativeOwner=false;check(nav::decodeNavigationSceneMesh(sceneCapture).openPlane,"original collision-free swimming remains available without scale");
    check(decodeOwnedNavigationMesh({}, {},0,nav::CollisionSnapshot{},true,0,{1,0,1}).openPlane,"shared decoder preserves collision-free swimming before scale qualification");nativeOwner=true;geometryScale=1;rootPointer=0;
    prepare("stbv0000");const auto originalWorkers=workers;workers.resize(500);
    for(unsigned i=0;i<workers.size();++i) workers[i]=originalWorkers[i%originalWorkers.size()];
    put(controller.data()+0xc,static_cast<uint16_t>(500));setScene();
    std::vector<RawNavigationTriangle> triangles(32767);std::vector<RawNavigationVertex> vertices(32767);std::array<uint8_t,12> root{};
    for(auto& triangle:triangles) {triangle.vertices[0]=0;triangle.vertices[1]=1;triangle.vertices[2]=2;for(auto& next:triangle.neighbours) next=-1;}
    vertices[1].coordinates[0]=10;vertices[2].coordinates[2]=10;
    triangleCount=32767;trianglesPointer=reinterpret_cast<uintptr_t>(triangles.data());verticesPointer=reinterpret_cast<uintptr_t>(vertices.data());rootPointer=reinterpret_cast<uintptr_t>(root.data());put(root.data()+0xa,static_cast<int16_t>(32767));
    observe=true;const auto largeBegin=std::chrono::steady_clock::now();check(nav::captureNavigationScene(sceneCapture,1,true) && sceneCapture.geometryValid,"complete500/32767 scene captured in one observation");
    const auto largeCaptured=std::chrono::steady_clock::now();observe=false;
    nativeOwner=false;std::vector<Entity> largeEntities;check(nav::scanNavigationScene(sceneCapture,largeEntities,[](int) {return std::wstring{};},[](int) {return std::wstring{};}),"owned maximum scene");
    const auto largeMesh=nav::decodeNavigationSceneMesh(sceneCapture);check(largeMesh.triangles.size()==32767 && largeMesh.fixedHeight && largeMesh.triangles[0].valid,"maximum typed mesh uses actual controlled swimming component");
    nativeOwner=true;const auto largeReady=std::chrono::steady_clock::now();
    double rapidMax=0;for(unsigned n=0;n<32;++n) {observe=true;const auto begin=std::chrono::steady_clock::now();check(nav::captureNavigationScene(sceneCapture,1,false),"rapid whole-scene capture");
        observe=false;rapidMax=(std::max)(rapidMax,std::chrono::duration<double,std::micro>(std::chrono::steady_clock::now()-begin).count());}
    observe=true;put(controller.data()+0xc,static_cast<uint16_t>(501));check(!nav::captureNavigationScene(sceneCapture,1,false),"invalid501 worker count refuses before getters");
    put(controller.data()+0xc,static_cast<uint16_t>(500));triangleCount=32768;check(nav::captureNavigationScene(sceneCapture,1,true) && !sceneCapture.geometryValid,"invalid signed triangle bound never copies unqualified geometry");observe=false;
    prepare("stbv0000",false);setScene();observe=true;check(nav::captureNavigationScene(sceneCapture,1,false),"pending-resource scene still copies ready Native facts");observe=false;
    nativeOwner=false;std::vector<Entity> pendingEntities;check(!nav::scanNavigationScene(sceneCapture,pendingEntities,[](int) {return std::wstring{};},[](int) {return std::wstring{};}) && pendingEntities.empty(),"pending resource never invokes a Native parser or yields partial targets");
    drainAtelResourcesForTest();check(nav::scanNavigationScene(sceneCapture,pendingEntities,[](int) {return std::wstring{};},[](int) {return std::wstring{};}),"same copied scene becomes usable on exact prepared resource completion");nativeOwner=true;
    std::printf("METRIC500/32767 capture %.3fus; capture-to-ready %.3fms; rapid32 capture max %.3fus. Largest counts synthetic with repeated real local headers; ready bytes copied in one observation.\n",
        std::chrono::duration<double,std::micro>(largeCaptured-largeBegin).count(),std::chrono::duration<double,std::milli>(largeReady-largeBegin).count(),rapidMax);
    check(!foreignReads && !allocations && !frees,"Native scene heap0/0 and worker engine reads0");
    // This semantic domain adapter allocates on capture and is excluded from
    // Native heap/timing claims above. The actual queue has its separate held
    // worker/pressure/affinity fixtures. Here it tests ticket and event protocol.
    nav::startNavigationObservations(acceptQuery,acceptScene,resetObserver);
    for(unsigned i=0;i<5;++i) check(nav::submitNavigationQuery(nav::Query::Distance),"equal independent requests admitted");
    check(nav::captureRequestedNavigationScene(false),"native owner services whole request burst");
    for(unsigned i=1;i<4;++i) check(nav::captureRequestedNavigationScene(false,true),"held worker capture slot admitted");
    check(nav::navigationSceneSlotsForTest()==4 && !nav::captureRequestedNavigationScene(false,true),"held observer pressure never waits or overwrites a borrowed scene");
    nativeOwner=false;drainSpeechDomainsForTest();nativeOwner=true;
    check(acceptedQueries.size()==5 && acceptedScenes==4 && !nav::navigationSceneSlotsForTest(),"equal actions retained FIFO and all capture slots recovered");
    for(unsigned i=1;i<acceptedQueries.size();++i) check(acceptedQueries[i].serial>acceptedQueries[i-1].serial,"independent request sequence preserved");
    check(nav::captureRequestedNavigationScene(false,true),"post-pressure recovery capture");nav::resetNavigationObservations();
    nativeOwner=false;speechq::fixture::pending.clear();drainSpeechDomainsForTest();nativeOwner=true;
    check(!nav::navigationSceneSlotsForTest(),"same-generation discarded packet retires only on preparation owner");
    check(nav::captureRequestedNavigationScene(false,true),"fresh same-generation scene survives canceled old packet");
    nativeOwner=false;drainSpeechDomainsForTest();nativeOwner=true;check(acceptedScenes==5,"only fresh revision delivered");
    check(nav::captureRequestedNavigationScene(false,true),"queued pre-generation scene");++epoch;nav::resetNavigationObservations();
    nativeOwner=false;speechq::fixture::pending.clear();drainSpeechDomainsForTest();nativeOwner=true;
    check(!nav::navigationSceneSlotsForTest(),"generation discard cannot leak scene pool");
    std::printf("PASS: scene observation protocol retains five equal requests, four held frames, pressure/recovery and canceled-generation slot retirement. Fake domain transport only; no Native-heap/audio claim for protocol adapter.\n");
    std::printf("PASS: production copied scenes, ten real EV files plus500/32767/pending, primitive guards, immutable labels, source reuse and open swimming mesh. Native heap0/0; worker engine reads0.\nMETRIC max Native capture %.3fus, capture-to-ready %.3fms, entities %u, affine entries %u. Synthetic live poses/bindings/controls; no whole-game/audible claim.\n",maxCapture,maxReady,total,nativeEntries);return 0;
} catch(const std::exception& e) {observe=false;std::fprintf(stderr,"FAIL: scene fixture exception %s\n",e.what());return 1;}}

#endif
