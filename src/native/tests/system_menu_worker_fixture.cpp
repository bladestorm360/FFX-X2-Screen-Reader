#include "../selection_details.hpp"
#include "../system_menu_capture.hpp"
#include "../speech_queue.hpp"
#include "../events.hpp"
#include "../settings.hpp"
#include <atomic>
#include <cstdio>
#include <cstdlib>
namespace {
thread_local bool nativeCapture=false;
std::atomic<unsigned> allocations{0},destructions{0},idle{0};
std::atomic<bool> hold{true};
HANDLE entered=nullptr,resume=nullptr;
void check(bool ok,const char* message) {if(!ok) {std::fprintf(stderr,"FAIL: %s\n",message);std::exit(1);}}
void held() {check(WaitForSingleObject(entered,3000)==WAIT_OBJECT_0,"preparation held");}
void release() {hold=false;SetEvent(resume);}
void blockAgain() {ResetEvent(entered);ResetEvent(resume);hold=true;}
void waitIdle(unsigned before) {
    const auto deadline=GetTickCount64()+3000;
    while(idle.load()==before && GetTickCount64()<deadline) Sleep(1);
    check(idle.load()!=before,"maintenance follows the complete drain");
}
std::wstring take() {
    const auto deadline=GetTickCount64()+3000;ffx::speechq::Utterance result;
    while(!ffx::speechq::take(result) && GetTickCount64()<deadline) Sleep(1);
    check(!result.text.empty(),"SDK sink receives expected owned output");return result.text;
}
void observedIdle() {++idle;}
void noOp(const ffx::speechq::Header&,const std::wstring&) {}
void encoded(ffx::ReadySystemText& text,const wchar_t* value) {
    constexpr wchar_t alphabet[]=L"0123456789 !\"#$%&'()*+,-./:;<=>?ABCDEFGHIJKLMNOPQRSTUVWXYZ[\\]^_\u2018abcdefghijklmnopqrstuvwxyz";
    text.length=0;
    while(*value) {
        if(*value==L'\n') text.bytes[text.length++]=3;
        else {const auto c=std::wcschr(alphabet,*value);check(c!=nullptr,"fixture glyph");text.bytes[text.length++]=static_cast<uint8_t>(0x30+c-alphabet);}
        ++value;
    }
    text.bytes[text.length++]=0;
}
}
void* operator new(size_t n) {if(nativeCapture) ++allocations;if(auto p=std::malloc(n?n:1)) return p;throw std::bad_alloc();}
void* operator new[](size_t n) {return ::operator new(n);}
void operator delete(void* p) noexcept {if(nativeCapture && p) ++destructions;std::free(p);}
void operator delete[](void* p) noexcept {::operator delete(p);}
void operator delete(void* p,size_t) noexcept {::operator delete(p);}
void operator delete[](void* p,size_t) noexcept {::operator delete(p);}
namespace ffx {
unsigned generation() {return speechq::generation();}
int setting(Setting s) {return s==Setting::Speech?1:0;}
void prepareCopiedControlSnapshot(CopiedControls& p) {p={};}
OwnedTextContext copiedControlContext(const CopiedControls&) {check(!nativeCapture,"encoded formatting stays on preparation");return {};}
bool copy(uintptr_t source,void* target,size_t count) {std::memcpy(target,reinterpret_cast<const void*>(source),count);return true;}
std::wstring clean(std::wstring text) {check(!nativeCapture,"clean stays on preparation");return text;}
std::wstring join(const std::vector<std::wstring>& parts,const wchar_t* separator) {
    std::wstring result;for(const auto& part:parts) if(!part.empty()) {if(!result.empty()) result+=separator;result+=part;}return result;
}
bool readCopiedTextDetail() {return false;}
void resetCopiedTextDetail() {}
namespace speechq {
void captureBeforeDrainForTest() {if(hold.load()) {SetEvent(entered);WaitForSingleObject(resume,INFINITE);}}
}
}
int main() {
    entered=CreateEventW(nullptr,TRUE,FALSE,nullptr);resume=CreateEventW(nullptr,TRUE,FALSE,nullptr);
    const auto backendWake=CreateEventW(nullptr,FALSE,FALSE,nullptr);
    check(entered && resume && backendWake,"fixture barriers");
    ffx::installSelectionDetails();ffx::installSystemMenuCapture();
    ffx::speechq::registerDomain(31,noOp,nullptr);ffx::speechq::registerDomainMaintenance(31,observedIdle);
    check(ffx::speechq::start(GetModuleHandleW(nullptr),backendWake),"preparation starts without a real SDK");held();
    char area[64]="Area",time[64]="01:23";
    ffx::SystemMenuPacket packet;
    nativeCapture=true;
    ffx::prepareSystemMenuPacket(packet,ffx::SystemMenuKind::SlotPopulation);packet.index=7;
    ffx::copySystemAscii(reinterpret_cast<uintptr_t>(area),packet.text[0],64);
    ffx::copySystemAscii(reinterpret_cast<uintptr_t>(time),packet.text[1],64);
    check(ffx::commitSystemMenuPacket(packet),"owned slot labels capture");std::memset(area,0,sizeof(area));std::memset(time,0,sizeof(time));
    packet={};ffx::prepareSystemMenuPacket(packet,ffx::SystemMenuKind::SlotFocus);packet.index=7;packet.detail=ffx::beginSelectionDetail();
    check(ffx::commitSystemMenuPacket(packet) && ffx::readSelectionDetail(),"focus/manual publication independent of preparation");
    nativeCapture=false;release();
    check(take()==L"Slot 7: Area, 01:23" && take()==L"Slot 7: Area, 01:23","worker cache uses owned labels and supplies current detail");
    blockAgain();const unsigned wake=0;
    check(ffx::speechq::captureDomain(31,&wake,sizeof(wake),nullptr,0,ffx::generation()),"next barrier wake");held();
    packet={};ffx::prepareSystemMenuPacket(packet,ffx::SystemMenuKind::Config,true);packet.joinLabel=true;
    encoded(packet.text[0],L"Sound");encoded(packet.text[1],L"Stereo");encoded(packet.text[2],L"Sound\n-\nDisplayed description");
    nativeCapture=true;packet.detail=ffx::beginSelectionDetail();
    check(ffx::commitSystemMenuPacket(packet) && ffx::readSelectionDetail(),"config/detail captured");nativeCapture=false;packet.text={};release();
    check(take()==L"Sound: Stereo" && take()==L"Displayed description","owned Config decode and compound detail are preserved");
    const std::wstring label=L"Equal displayed detail";
    blockAgain();const unsigned trigger=0;
    check(ffx::speechq::captureDomain(31,&trigger,sizeof(trigger),nullptr,0,ffx::generation()),"barrier wake");held();
    nativeCapture=true;ffx::selectionDetail(label,false);check(ffx::readSelectionDetail(),"captured manual read accepted");ffx::resetSelectionDetail();
    check(!ffx::readSelectionDetail(),"reset immediately removes native detail availability");nativeCapture=false;
    const auto before=idle.load();release();waitIdle(before);check(!ffx::speechq::pending(),"reset retires queued stale manual reads");
    blockAgain();check(ffx::speechq::captureDomain(31,&trigger,sizeof(trigger),nullptr,0,ffx::generation()),"second barrier wake");held();
    nativeCapture=true;unsigned accepted=0;
    while(accepted<2048 && ffx::speechq::captureDomain(31,&trigger,sizeof(trigger),nullptr,0,ffx::generation())) ++accepted;
    check(accepted>=2047 && accepted<=2048,"metadata is full, accounting for a possible unconsumed wake record");
    check(!ffx::speechq::captureDomain(31,&trigger,sizeof(trigger),nullptr,0,ffx::generation()),"full capture rejects without waiting");
    ffx::resetSelectionDetail();ffx::resetSystemMenuCapture();check(!ffx::readSelectionDetail(),"lost reset marker cannot restore availability");nativeCapture=false;
    const auto full=idle.load();release();waitIdle(full);
    check(allocations.load()==0 && destructions.load()==0 && !ffx::speechq::pending(),"capture/read/reset pressure has no native heap or owned destruction");
    packet={};ffx::prepareSystemMenuPacket(packet,ffx::SystemMenuKind::SlotFocus);packet.index=7;
    nativeCapture=true;packet.detail=ffx::beginSelectionDetail();
    check(ffx::commitSystemMenuPacket(packet) && ffx::readSelectionDetail(),"fresh focus after rejected reset marker");nativeCapture=false;
    check(take()==L"Slot 7 empty" && take()==L"Slot 7 empty","maintenance clears the old slot cache without a reset marker");
    check(allocations.load()==0 && destructions.load()==0,"fresh focus is also native-heap-free");
    blockAgain();check(ffx::speechq::captureDomain(31,&trigger,sizeof(trigger),nullptr,0,ffx::generation()),"resource barrier wake");held();
    char fresh[64]="Fresh",clock[64]="01:23";
    nativeCapture=true;ffx::speechq::invalidate("resource-start");
    packet={};ffx::prepareSystemMenuPacket(packet,ffx::SystemMenuKind::SlotPopulation);packet.index=9;
    ffx::copySystemAscii(reinterpret_cast<uintptr_t>(fresh),packet.text[0],64);ffx::copySystemAscii(reinterpret_cast<uintptr_t>(clock),packet.text[1],64);
    check(ffx::commitSystemMenuPacket(packet),"fresh post-load slot arrives before native owner reset");
    ffx::resetSystemMenuCapture();
    packet={};ffx::prepareSystemMenuPacket(packet,ffx::SystemMenuKind::SlotFocus);packet.index=9;packet.detail=ffx::beginSelectionDetail();
    check(ffx::commitSystemMenuPacket(packet) && ffx::readSelectionDetail(),"fresh post-reset focus and detail are captured");nativeCapture=false;release();
    check(take()==L"Slot 9: Fresh, 01:23" && take()==L"Slot 9: Fresh, 01:23","late owner reset preserves fresh same-resource data");
    check(allocations.load()==0 && destructions.load()==0,"resource reset does not allocate or destroy native owned storage");
    puts("PASS: owned system-menu slot/cache/config details, caller reuse, held-worker independence, zero native heap and reset-pressure retirement.");
    return 0;
}
