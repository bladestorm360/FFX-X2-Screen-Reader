#include "external_fixture.hpp"
// Production bounded capture, real Surface geometry, and the actual owned
// decoder/ray implementation. Native facts are controlled; no game or SDK.
#include "../wall_cues.hpp"
#include "../navigation_capture.hpp"
#include "../settings.hpp"
#include <atomic>
#include <thread>
#include <chrono>
#include <fstream>
#include <map>
#include <cstdio>
#include <cstdlib>
namespace {
using namespace ffx;
std::vector<uint8_t> memory(0x2500000);
DWORD nativeOwner=0;unsigned epoch=1;
std::atomic<unsigned> foreignReads{0},nativeAllocations{0},nativeFrees{0},copies{0},searches{0};
thread_local bool measureNative=false;
std::atomic<bool> stop{false},hold{false};
HANDLE entered=nullptr,resume=nullptr,workWake=nullptr;
std::map<unsigned,Handler> before;
void check(bool ok,const char* why) {if(!ok) {std::fprintf(stderr,"FAIL: %s\n",why);std::exit(1);}}
template<class T> void put(unsigned at,T value) {std::memcpy(memory.data()+at,&value,sizeof(value));}
void load(const char* name,unsigned at,unsigned size) {
    std::ifstream file((ffx_fixture::evidenceRoot()/"offline-output/open_issue_repairs_20261012/swim/cdsp00_0").string()+"/"+name,std::ios::binary);
    check(file.good(),"retained actual Surface mesh exists");file.read(reinterpret_cast<char*>(memory.data()+at),size);
    check(file.gcount()==size,"complete actual Surface mesh");
}
void surface() {
    load("triangles.bin",0x2200000,1739*16);load("vertices.bin",0x2300000,959*8);
    put<uintptr_t>(0xf01a44,address(0x2200000));put<int>(0xf01a48,1739);
    put<uintptr_t>(0xf01a4c,address(0x2300000));put<float>(0xf01a50,47.246630859375f);
    put<uintptr_t>(0xf01a54,address(0x2400100));put<int16_t>(0x240010a,959);
    put<int16_t>(0x2100824,347);put<float>(0x210000c,-192);put<float>(0x2100010,59);put<float>(0x2100014,79);
    put<uint8_t>(0x2100186,2);
}
double capture() {
    const auto start=std::chrono::steady_clock::now();measureNative=true;captureWallCues();measureNative=false;
    return std::chrono::duration<double,std::micro>(std::chrono::steady_clock::now()-start).count();
}
double ready() {
    const auto start=std::chrono::steady_clock::now();
    while(!wallCuesReadyForTest() && std::chrono::steady_clock::now()-start<std::chrono::seconds(5)) Sleep(0);
    check(wallCuesReadyForTest(),"owned wall result becomes ready");
    return std::chrono::duration<double,std::milli>(std::chrono::steady_clock::now()-start).count();
}
}
void* operator new(size_t size) {if(measureNative) ++nativeAllocations;auto* p=std::malloc(size?size:1);if(!p) throw std::bad_alloc();return p;}
void* operator new[](size_t size) {return ::operator new(size);}
void operator delete(void* p) noexcept {if(p && measureNative) ++nativeFrees;std::free(p);}
void operator delete[](void* p) noexcept {::operator delete(p);}
void operator delete(void* p,size_t) noexcept {::operator delete(p);}
void operator delete[](void* p,size_t) noexcept {::operator delete(p);}
namespace ffx {
std::atomic<int> scene{0};
uintptr_t address(unsigned rva) {if(GetCurrentThreadId()!=nativeOwner) ++foreignReads;return reinterpret_cast<uintptr_t>(memory.data()+rva);}
bool copy(uintptr_t source,void* destination,size_t size) {
    if(GetCurrentThreadId()!=nativeOwner) {++foreignReads;return false;}
    const auto base=reinterpret_cast<uintptr_t>(memory.data());
    if(size>0x10000 || source<base || source-base>memory.size() || size>memory.size()-(source-base)) return false;
    if(measureNative && (source==base+0x2200000 || source==base+0x2300000)) ++copies;
    std::memcpy(destination,reinterpret_cast<const void*>(source),size);return true;
}
bool nativeGameThread() {return GetCurrentThreadId()==nativeOwner;}
bool nativeAtelSpan(uintptr_t,AtelSpan&) {return false;} // This host tests geometry, not field ATEL activation.
bool readyAtelIdentity(uintptr_t,AtelIdentity&) {return false;}
bool readyAtelWorkerHeader(uintptr_t,unsigned,unsigned&) {return false;}
NativeInteractionFacts captureNativeInteractionFacts(uintptr_t,EntryMask,const AtelSpan&) {
    check(false,"geometry-only host must not enter the optional Gagazet script branch");return {};
}
unsigned generation() {return epoch;}
bool inBattle() {check(nativeGameThread(),"battle getter retained on Native");return false;}
int setting(Setting s) {check(nativeGameThread(),"settings captured on Native");return s==Setting::Walls?1:s==Setting::WallRange?20:0;}
bool hookSnapshot(unsigned rva,Handler first,Handler) {before[rva]=first;return true;}
void log(const char*,...) {}
bool output(const wchar_t*,bool,const char*) {check(nativeGameThread(),"pressure literal comes from Native");return true;}
void setButtonGlyphReader(ButtonGlyphReader) {}
unsigned fixtureWallRays(const Mesh&,Point,Point,Point,double);
unsigned nearbyWalls(const Mesh& mesh,Point p,Point f,Point r,double range) {
    check(!measureNative,"ray computation never runs in measured Native capture");
    ++searches;
    if(hold.exchange(false)) {check(!nativeGameThread(),"held ray work belongs to Worker");SetEvent(entered);check(WaitForSingleObject(resume,5000)==WAIT_OBJECT_0,"test-only Worker barrier released");}
    return fixtureWallRays(mesh,p,f,r,range);
}
}
int main() {
    nativeOwner=GetCurrentThreadId();entered=CreateEventW(nullptr,TRUE,FALSE,nullptr);resume=CreateEventW(nullptr,TRUE,FALSE,nullptr);workWake=CreateEventW(nullptr,FALSE,FALSE,nullptr);
    check(entered && resume && workWake,"fixture barriers and production event wake");
    put<uintptr_t>(0x1fc44e4,address(0x2100000));put<unsigned>(0x1fc44e0,1);put<uintptr_t>(0xf00740,address(0x2100000));
    put<uint16_t>(0x2100002,1);put<uintptr_t>(0xf26ae8,address(0x2400000));put<uint8_t>(0x2400000,2);
    put<uintptr_t>(0x1f11430,address(0x2402000));put<float>(0x2402040,1);put<float>(0x2402048,0);
    surface();installWallCues();setWallCueWakeForTest(workWake);
    std::array<uint8_t,32> rejectedBuffer{};
    check(!copyReadyRegion(address(0x2200000),rejectedBuffer.data(),64,32) &&
        !copyReadyRegion(address(0x2200000),rejectedBuffer.data(),1,0x800001) &&
        !copyReadyRegion(UINTPTR_MAX-15,rejectedBuffer.data(),32,32),"bulk captures reject extent mismatch, oversized work and pointer overflow before reading");
    Point forward,right;check(cameraBasis(forward,right),"actual Native camera basis");
    const auto old=readMesh();const auto expected=nearbyWalls(old,playerPosition(),forward,right,20*11.13);
    const auto coldBegin=std::chrono::steady_clock::now();const auto cold=capture();
    // Destroy the source after capture and before starting its worker.
    std::memset(memory.data()+0x2200000,0,1739*16);
    std::thread worker([] {SetThreadPriority(GetCurrentThread(),THREAD_PRIORITY_BELOW_NORMAL);while(!stop) {WaitForSingleObject(workWake,INFINITE);drainWallCuesForTest();}});
    const auto coldReady=ready();const auto coldTotal=std::chrono::duration<double,std::milli>(std::chrono::steady_clock::now()-coldBegin).count();
    check(copiedWallCues()==expected,"actual Surface wall mask equals original production Native reader despite source overwrite");
    const auto oldCopies=copies.load();const auto warmBegin=std::chrono::steady_clock::now();const auto warm=capture();const auto warmReady=ready();
    const auto warmTotal=std::chrono::duration<double,std::milli>(std::chrono::steady_clock::now()-warmBegin).count();
    check(copies==oldCopies && copiedWallCues()==expected,"warm copied-data cache requires no physical mesh recopy");
    hold=true;ResetEvent(entered);ResetEvent(resume);capture();check(WaitForSingleObject(entered,5000)==WAIT_OBJECT_0,"actual Worker computation held");
    double maximum=0;for(unsigned i=0;i<12;++i) maximum=(std::max)(maximum,capture());
    check(wallCuePressureForTest()>0 && !copiedWallCues(),"held Worker has bounded pressure, no Native wait and no stale wall result");
    cancelWallCues();SetEvent(resume);Sleep(20);check(!copiedWallCues(),"cancel invalidates late held results");
    capture();ready();check(copiedWallCues()==expected,"capacity recovers after held Worker retirement");
    put<int>(0xf01a48,32768);capture();check(!wallCuesReadyForTest() && !copiedWallCues(),"oversized Native mesh cancels cues without reading a guessed region");
    surface();Event reset;before.at(0x43ea00)(reset);capture();ready();
    put<float>(0x2100010,150);const auto divedOld=readMesh();const auto divedExpected=nearbyWalls(divedOld,playerPosition(),forward,right,20*11.13);
    capture();ready();check(copiedWallCues()==divedExpected,"real swim component wall rays follow live depth over owned cached geometry");
    ++epoch;check(!copiedWallCues(),"new graphics generation excludes old results");capture();ready();
    // Maximum signed native triangle count, small real-layout vertex bank.
    RawNavigationTriangle triangle{};triangle.vertices[1]=1;triangle.vertices[2]=2;for(auto& n:triangle.neighbours)n=-1;
    for(unsigned i=0;i<32767;++i) put(0x2200000+i*16,triangle);
    const RawNavigationVertex a{{0,0,0,0}},b{{100,0,0,0}},c{{0,0,100,0}};
    put(0x2300000,a);put(0x2300000+8,b);put(0x2300000+16,c);
    put<int>(0xf01a48,32767);put<int16_t>(0x240010a,3);put<float>(0xf01a50,1);
    put<uint8_t>(0x2100186,0);put<int16_t>(0x2100824,0);put<float>(0x210000c,5);put<float>(0x2100010,0);put<float>(0x2100014,5);
    before.at(0x43ea00)(reset);const auto largeExpected=nearbyWalls(readMesh(),playerPosition(),forward,right,20*11.13);
    const auto largeBegin=std::chrono::steady_clock::now();const auto large=capture();const auto largeReady=ready();
    const auto largeTotal=std::chrono::duration<double,std::milli>(std::chrono::steady_clock::now()-largeBegin).count();
    check(copiedWallCues()==largeExpected,"maximum32767 triangle wall result preserves production reader semantics");
    const auto warmCopies=copies.load();double rapidMax=0;
    for(unsigned i=0;i<3;++i) {put<float>(0x210000c,static_cast<float>(6+i));rapidMax=(std::max)(rapidMax,capture());}
    ready();check(copies==warmCopies,"rapid current poses reuse immutable physical geometry");
    const auto beforeMapCopies=copies.load();
    put<int>(0xf30804,382);capture();
    check(!copiedWallCues() && copies==beforeMapCopies,"airship map cancels field walls before any geometry capture");
    put<int>(0xf30804,0);capture();ready();
    check(copies==beforeMapCopies,"leaving the airship map can resume the existing owned geometry cache");
    // Same coordinate-reader flag, with no renderer-room special case. Native
    // header/phase are controlled here; original hiku2100 is in reader_fixture.
    constexpr unsigned searchChunk=0x2000000;
    put<uintptr_t>(0x2400048,address(searchChunk));put<unsigned>(searchChunk,24785);
    put<uint16_t>(searchChunk+0x34,41);put<unsigned>(searchChunk+0x20,0x150f8);
    put<unsigned>(searchChunk+0x30,0xe6c);put<unsigned>(searchChunk+0xc,0x100);
    std::memcpy(memory.data()+searchChunk+0x100,"hiku2100",8);put<int>(searchChunk+0x15248,1);
    capture();check(!copiedWallCues() && copies==beforeMapCopies,"active coordinate-reader search cancels walls before geometry even outside renderer room382");
    put<int>(searchChunk+0x15248,0);capture();ready();
    check(copies==beforeMapCopies,"search exit restores existing owned wall geometry without a reload");
    stop=true;SetEvent(workWake);worker.join();check(!foreignReads && !nativeAllocations && !nativeFrees,"actual Native capture heap0/0 and Worker engine reads0");
    std::printf("PASS: production owned wall capture/decoder/rays, actual Surface parity, warm cache, source reuse, held pressure/recovery, cancellation, bounds and live depth.\nMETRIC cold Native %.3fus, first ready %.3fms; warm %.3fus/%.3fms; held max Native %.3fus; engine copies%u, searches%u. Controlled host, no audible/game claim.\n",cold,coldReady,warm,warmReady,maximum,copies.load(),searches.load());
    std::printf("METRIC maximum32767 triangle capture %.3fus/ready wait %.3fms; actual request-to-ready %.3fms; rapid warm capture max %.3fus. Surface request-to-ready cold %.3fms (includes explicit source overwrite and delayed Worker creation), warm %.3fms. No real SDL delivery in this host.\n",large,largeReady,largeTotal,rapidMax,coldTotal,warmTotal);
}
