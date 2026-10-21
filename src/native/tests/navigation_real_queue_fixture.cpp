// Actual speech capture/preparation plus production navigation over retained EVs.
// Native engine/input/audio endpoints remain controlled fixtures; no game/SDK.
#define FFX_NAVIGATION_REAL_QUEUE_TEST
#include "navigation_service_fixture.cpp"
#include "../speech_queue.hpp"
#include <atomic>
#include <algorithm>
namespace {
HANDLE entered=nullptr,resume=nullptr,progress=nullptr,sinkSignal=nullptr;
std::atomic<bool> hold{true};
std::atomic<unsigned> maintenancePasses{0};
void ownerEventReal() {ffx::Event e;e.rva=0x225930;for(const auto& listener:navigationListeners[e.rva]) if(listener.after) listener.after(e);}
void waited() {check(WaitForSingleObject(entered,3000)==WAIT_OBJECT_0,"actual preparation barrier");}
void holdNext() {ResetEvent(entered);ResetEvent(resume);hold=true;ffx::speechq::notifyPreparation();waited();ResetEvent(progress);}
void resumeWork() {hold=false;SetEvent(resume);}
void prepared() {ffx::drainReachabilityForTest();maintenancePasses.fetch_add(1,std::memory_order_release);SetEvent(progress);}
void marker(const ffx::speechq::Header& header,const std::wstring&) {ffx::speechq::emitOwned(L"Speech tail marker",header,"nav-tail",false,false);}
std::vector<ffx::speechq::Utterance> take(unsigned expected) {
    std::vector<ffx::speechq::Utterance> result;const auto deadline=GetTickCount64()+5000;
    while(result.size()<expected && GetTickCount64()<deadline) {
        ownerEventReal();ffx::speechq::Utterance item;
        while(ffx::speechq::take(item)) result.push_back(std::move(item));
        if(result.size()<expected) WaitForSingleObject(progress,10);
    }
    if(result.size()!=expected) {std::fprintf(stderr,"Actual queue output mismatch expected=%u actual=%zu; maintenance=%u slots=%u waiting=%d nativeEntries=%u names=%u foreign=%u\n",expected,result.size(),maintenancePasses.load(),nav::navigationSceneSlotsForTest(),nav::navigationDataPending(),nativeEntries,areaGetterCalls,foreignReads);for(const auto& item:result) std::fwprintf(stderr,L"output: %ls\n",item.text.c_str());}
    check(result.size()==expected,"complete expected actual-queue output count");return result;
}
void discard() {ffx::speechq::Utterance item;while(ffx::speechq::take(item)) {}}
}
namespace ffx::speechq {
void captureBeforeDrainForTest() {if(hold.load(std::memory_order_acquire)) {SetEvent(entered);WaitForSingleObject(resume,INFINITE);}}
}
int main() {try {
    using namespace ffx;nativeOwner=true;
    entered=CreateEventW(nullptr,TRUE,FALSE,nullptr);resume=CreateEventW(nullptr,TRUE,FALSE,nullptr);
    progress=CreateEventW(nullptr,FALSE,FALSE,nullptr);sinkSignal=CreateEventW(nullptr,FALSE,FALSE,nullptr);
    check(entered && resume && progress && sinkSignal,"actual queue events");
    check(startAtelResources(),"owned resource bootstrap");installAtelResources();installNavigation();
    speechq::registerDomain(30,marker,nullptr);speechq::registerDomainMaintenance(31,prepared);
    check(speechq::start(GetModuleHandleW(nullptr),sinkSignal),"actual preparation thread without real backend");waited();
    prepare("cdsp0000");setScene();nameTableSize=16;
    const auto letters=L"0123456789 !\"#$%&'()*+,-./:;<=>?ABCDEFGHIJKLMNOPQRSTUVWXYZ[\\]^_‘abcdefghijklmnopqrstuvwxyz";
    const auto* area=L"Copied area";for(unsigned i=0;area[i];++i) areaBytes[i]=static_cast<uint8_t>(0x30+std::wcschr(letters,area[i])-letters);
    cameraPointer=reinterpret_cast<uintptr_t>(cameraBytes.data());put(cameraBytes.data()+0x40,1.f);put(actorBodies[0].bytes.data()+0x4fc,300.f);
    auto start=std::chrono::steady_clock::now();navigationKeys.at(VK_OEM_3)(false);ownerEventReal();resumeWork();
    auto cold=take(2);const auto coldMs=std::chrono::duration<double,std::milli>(std::chrono::steady_clock::now()-start).count();
    check(!cold[0].text.empty() && !foreignReads,"real cold complete scan and Native name handback");
    holdNext();const auto queriesBefore=maintenancePasses.load();
    for(unsigned i=0;i<4;++i) {navigationKeys.at('P')(false);ownerEventReal();}
    // Four owned snapshots survive engine body replacement; only copied data
    // can be processed after the actual worker is released.
    for(auto& body:actorBodies) std::fill(body.bytes.begin(),body.bytes.end(),static_cast<uint8_t>(0xff));
    resumeWork();auto equal=take(4);for(unsigned i=1;i<equal.size();++i) check(equal[i].text==equal[0].text,"four equal independent real-queue requests");
    check(maintenancePasses.load()>queriesBefore && !foreignReads,"actual Worker never reads overwritten engine state");
    holdNext();setScene();put(actorBodies[0].bytes.data()+0x4fc,300.f);
    for(unsigned i=0;i<4;++i) {navigationKeys.at('P')(false);ownerEventReal();}
    resetNavigation();speechq::interrupt();navigationKeys.at(VK_OEM_3)(false);resumeWork();check(WaitForSingleObject(progress,3000)==WAIT_OBJECT_0,"canceled frames retire before new collection");
    auto recovered=take(2);check(!recovered[0].text.empty(),"cancel retires old captures and fresh command recovers");discard();
    // Maximum declared engine counts, complete publication and a small speech
    // marker behind the graph work. Time includes queue, computation and sink.
    holdNext();triangleCount=32767;static std::vector<std::array<int16_t,8>> triangles(32767);
    static std::vector<std::array<int16_t,4>> vertices(32767);static std::array<uint8_t,12> root{};
    put(root.data()+0xa,static_cast<int16_t>(32767));rootPointer=reinterpret_cast<uintptr_t>(root.data());
    for(unsigned i=0;i<triangles.size();++i) {auto& t=triangles[i];t={0,1,2,static_cast<int16_t>(i?i-1:-1),static_cast<int16_t>(i+1<triangles.size()?i+1:-1),-1,0,0};}
    vertices[0]={0,0,0,0};vertices[1]={10,0,0,0};vertices[2]={0,0,10,0};
    trianglesPointer=reinterpret_cast<uintptr_t>(triangles.data());verticesPointer=reinterpret_cast<uintptr_t>(vertices.data());geometryScale=1;
    const unsigned originalCount=static_cast<unsigned>(workers.size());workers.resize(500);
    for(unsigned i=originalCount;i<workers.size();++i) {workers[i]=workers[i%originalCount];put(workers[i].data()+0x2e,static_cast<uint16_t>(i%originalCount));}
    put(controller.data()+0xc,static_cast<uint16_t>(500));setScene();put(actorBodies[0].bytes.data()+0x4fc,300.f);
    start=std::chrono::steady_clock::now();observe=true;navigationKeys.at(VK_OEM_3)(false);ownerEventReal();observe=false;
    const auto nativeUs=std::chrono::duration<double,std::micro>(std::chrono::steady_clock::now()-start).count();
    const unsigned data=0;check(speechq::captureDomain(30,data,generation()),"small voice behind large graph packet");resumeWork();
    auto large=take(3);const auto largeMs=std::chrono::duration<double,std::milli>(std::chrono::steady_clock::now()-start).count();
    check(std::any_of(large.begin(),large.end(),[](const auto& u) {return u.text==L"Speech tail marker";}),"small voice survives large pipeline");
    check(!foreignReads && !allocations && !frees,"real transport Native heap0/0 and Worker getter/read0");
    // Deliberately stall preparation beyond the production250ms effect guard.
    // This host delay is not normal latency and introduces no game-thread wait.
    holdNext();fixtureAutoWalk=true;const auto origin=playerPosition();const Point goal{origin.x+2,origin.y,origin.z+2};
    const auto startsBefore=starts,refusalsBefore=refusals;UtilityText prefix(L"Fixed target. ");
    check(queuePuzzleGuidePoint(goal,-1,false,L"Interact with the guide target.",prefix),"typed point admitted on actual transport");ownerEventReal();
    const auto capturedAt=GetTickCount64();Sleep(300);resumeWork();auto aged=take(2);
    check(starts==startsBefore && refusals>refusalsBefore,"old copied point cannot start movement after250ms guard");
    const auto agedMs=GetTickCount64()-capturedAt;
    holdNext();check(queuePuzzleGuidePoint(goal,-1,false,L"Interact with the guide target.",prefix),"fresh point admitted after aged refusal");ownerEventReal();resumeWork();take(1);
    const auto deadline=GetTickCount64()+3000;while(starts==startsBefore && GetTickCount64()<deadline) {ownerEventReal();WaitForSingleObject(progress,1);}
    check(starts==startsBefore+1,"fresh current point recovers and starts on Native owner");
    std::printf("METRIC deliberate held data-age refusal %llums; production effect limit250ms. Fresh point recovery verified.\n",static_cast<unsigned long long>(agedMs));
    std::printf("PASS: real preparation/capture navigation FIFO, Native names, source overwrite, cancellation/recovery, four equal actions, 500 workers/32767 triangles and speech tail.\nMETRIC cold whole request %.3fms; large Native %.3fus, whole scan plus speech tail %.3fms. Controlled native/effect/audio endpoints; no gameplay/backend/audible claim.\n",coldMs,nativeUs,largeMs);
    holdNext();return 0;
}catch(const std::exception& e) {observe=false;std::fprintf(stderr,"FAIL: real navigation fixture %s\n",e.what());return 1;}}
