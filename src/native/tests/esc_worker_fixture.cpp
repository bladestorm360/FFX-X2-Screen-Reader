#include "../esc_capture.hpp"
#include "../selection_details.hpp"
#include "../speech_queue.hpp"
#include "../events.hpp"
#include "../settings.hpp"
#include <atomic>
#include <cstdio>
#include <cstdlib>
#include <cstring>

namespace {
thread_local bool nativeCapture=false;
std::atomic<unsigned> allocations{0},destructions{0};
std::atomic<unsigned> drains{0};
std::atomic<bool> hold{true};
HANDLE entered=nullptr,resume=nullptr;
unsigned configPointer=0;
unsigned config[128]{};
const char* names[]={"Enter","Space","J","K"};
void check(bool ok,const char* message) {if(!ok) {std::fprintf(stderr,"FAIL: %s\n",message);std::exit(1);}}
void resolver(unsigned id,char* out,unsigned cap) {
    const char* value="";
    switch(id) {
    case 0x100: value="Quit";break; case 0x101: value="Yes";break; case 0x102: value="No";break;
    case 0x200: value="Confirm";break; case 0x201: value="Resolution";break;
    case 0x235: value="Fullscreen";break; case 0x23c: value="On";break;
    case 0x290: value="Press $key$ to activate";break;
    }
    if(cap) {unsigned i=0;while(i+1<cap && value[i]) {out[i]=value[i];++i;}out[i]=0;}
}
uintptr_t keyboardName(unsigned id) {return reinterpret_cast<uintptr_t>(names[id<4?id:0]);}
uintptr_t padName(unsigned id) {return reinterpret_cast<uintptr_t>(names[id<4?id:0]);}
void held() {check(WaitForSingleObject(entered,3000)==WAIT_OBJECT_0,"preparation held");}
void release() {hold=false;SetEvent(resume);}
void blockAgain() {
    ResetEvent(entered);ResetEvent(resume);hold=true;
    const unsigned wake=0;
    check(ffx::speechq::captureDomain(31,&wake,sizeof(wake),nullptr,0,ffx::generation()),"barrier wake");
    held();nativeCapture=true;
}
void drained() {++drains;}
void awaitDrain(unsigned before) {
    const auto deadline=GetTickCount64()+3000;
    while(drains.load()==before && GetTickCount64()<deadline) Sleep(1);
    check(drains.load()!=before,"worker finishes the queued batch");
}
std::wstring take() {
    const auto deadline=GetTickCount64()+3000;ffx::speechq::Utterance result;
    while(!ffx::speechq::take(result) && GetTickCount64()<deadline) Sleep(1);
    check(!result.text.empty(),"worker emits expected owned text");return result.text;
}
}
void* operator new(size_t n) {if(nativeCapture) ++allocations;if(auto p=std::malloc(n?n:1)) return p;throw std::bad_alloc();}
void* operator new[](size_t n) {return ::operator new(n);}
void operator delete(void* p) noexcept {if(nativeCapture && p) ++destructions;std::free(p);}
void operator delete[](void* p) noexcept {::operator delete(p);}
void operator delete(void* p,size_t) noexcept {::operator delete(p);}
void operator delete[](void* p,size_t) noexcept {::operator delete(p);}

namespace ffx {
uintptr_t address(unsigned rva) {
    if(rva==0x2414a0) return reinterpret_cast<uintptr_t>(&resolver);
    if(rva==0x5af0) return reinterpret_cast<uintptr_t>(&keyboardName);
    if(rva==0x5b10) return reinterpret_cast<uintptr_t>(&padName);
    if(rva==0x1efb504) return reinterpret_cast<uintptr_t>(&configPointer);
    return 0;
}
bool copy(uintptr_t source,void* target,size_t count) {
    if(source==reinterpret_cast<uintptr_t>(&configPointer)) {std::memcpy(target,&configPointer,count);return true;}
    const auto base=reinterpret_cast<uintptr_t>(config);
    if(source>=base && source-base+count<=sizeof(config)) {std::memcpy(target,reinterpret_cast<const void*>(source),count);return true;}
    std::memcpy(target,reinterpret_cast<const void*>(source),count);return true;
}
unsigned generation() {return speechq::generation();}
int setting(Setting s) {return s==Setting::Speech?1:0;}
std::wstring clean(std::wstring text) {check(!nativeCapture,"text cleanup stays on preparation");return text;}
std::wstring join(const std::vector<std::wstring>& parts,const wchar_t* separator) {
    check(!nativeCapture,"joining stays on preparation");
    std::wstring out;for(const auto& part:parts) if(!part.empty()) {if(!out.empty()) out+=separator;out+=part;}return out;
}
bool readCopiedTextDetail() {return false;}
void resetCopiedTextDetail() {}
namespace speechq {void captureBeforeDrainForTest() {if(hold.load()) {SetEvent(entered);WaitForSingleObject(resume,INFINITE);}}}
}

int main() {
    entered=CreateEventW(nullptr,TRUE,FALSE,nullptr);resume=CreateEventW(nullptr,TRUE,FALSE,nullptr);
    const auto backendWake=CreateEventW(nullptr,FALSE,FALSE,nullptr);
    check(entered&&resume&&backendWake,"fixture barriers");
    configPointer=static_cast<unsigned>(reinterpret_cast<uintptr_t>(config));
    config[0x60/4]=0;config[0x60/4+1]=1;config[0x60/4+2]=2;config[0x60/4+3]=3;
    ffx::installSelectionDetails();ffx::installEscCapture();
    ffx::speechq::registerDomainMaintenance(31,drained);
    check(ffx::speechq::start(GetModuleHandleW(nullptr),backendWake),"worker starts without SDK");held();

    nativeCapture=true;
    ffx::EscPacket p;ffx::prepareEscPacket(p,ffx::EscCaptureKind::Popup);
    ffx::copyEscLocalized(0x100,p.text[0]);ffx::copyEscLocalized(0x102,p.text[1]);
    char borrowed[]="Quit";ffx::copyReadyAscii(reinterpret_cast<uintptr_t>(borrowed),p.text[0],512);
    // Restore the same exact synthetic localized value through the production helper.
    ffx::copyEscLocalized(0x100,p.text[0]);
    nativeCapture=true;check(ffx::commitEscPacket(p),"quit packet accepted");p={};nativeCapture=false;
    release();check(take()==L"Quit" && take()==L"No","separate quit prompt and initial focus survive caller reuse");

    blockAgain();ffx::Binding row{0,1,0x200};
    ffx::EscPacket keyboard;ffx::prepareEscPacket(keyboard,ffx::EscCaptureKind::BindingFocus);keyboard.pad=false;
    ffx::copyEscBinding(row,false,keyboard.bindings[0],0);nativeCapture=true;check(ffx::commitEscPacket(keyboard),"keyboard focus accepted");nativeCapture=false;
    release();check(take()==L"Confirm: Enter","keyboard primary key is single focus");
    blockAgain();ffx::prepareEscPacket(keyboard,ffx::EscCaptureKind::BindingFocus);keyboard.pad=false;
    ffx::copyEscBinding(row,false,keyboard.bindings[0],1);nativeCapture=true;check(ffx::commitEscPacket(keyboard),"keyboard alternate focus accepted");nativeCapture=false;
    release();check(take()==L"Confirm: Space","keyboard alternate key is single focus");
    blockAgain();ffx::EscPacket changes;ffx::prepareEscPacket(changes,ffx::EscCaptureKind::BindingChanges);changes.count=1;changes.pad=false;
    ffx::copyEscBinding(row,false,changes.bindings[0],2);nativeCapture=true;check(ffx::commitEscPacket(changes),"keyboard changed row accepted");nativeCapture=false;
    release();check(take()==L"Confirm: Enter, Space","changed keyboard row has both keys");
    blockAgain();ffx::EscPacket pad;ffx::prepareEscPacket(pad,ffx::EscCaptureKind::BindingFocus);pad.pad=true;
    ffx::copyEscBinding(row,true,pad.bindings[0]);nativeCapture=true;check(ffx::commitEscPacket(pad),"controller focus accepted");nativeCapture=false;
    release();check(take()==L"Confirm: Enter","controller focus has one button");

    blockAgain();ffx::EscPacket special;ffx::prepareEscPacket(special,ffx::EscCaptureKind::SpecialFocus);
    ffx::copyEscLocalized(0x200,special.text[0]);ffx::copyEscLocalized(0x290,special.text[1]);
    ffx::copyEscKey(2,false,special.bindings[0].first);
    check(ffx::commitEscPacket(special),"Special Features ready controls accepted without waiting for held worker");
    config[0x60/4+2]=3;special={};nativeCapture=false;release();
    check(take()==L"Confirm. Press J to activate","Special Features owns localized template and configured key before native source reuse");
    blockAgain();ffx::prepareEscPacket(special,ffx::EscCaptureKind::SpecialControls);
    ffx::copyEscLocalized(0x290,special.text[1]);ffx::copyEscKey(2,false,special.bindings[0].first);
    check(ffx::commitEscPacket(special)&&ffx::commitEscPacket(special),"repeated Special Features control requests accepted");
    nativeCapture=false;release();
    check(take()==L"Press K to activate"&&take()==L"Press K to activate","control help respects remapping and preserves equal explicit reads");

    blockAgain();ffx::EscPacket audio;ffx::prepareEscPacket(audio,ffx::EscCaptureKind::AudioChanged);audio.value=37;
    ffx::copyEscLocalized(0x200,audio.text[0]);nativeCapture=true;check(ffx::commitEscPacket(audio),"audio change accepted");nativeCapture=false;
    release();check(take()==L"Confirm: 37","audio setting value");

    blockAgain();ffx::EscPacket video;ffx::prepareEscPacket(video,ffx::EscCaptureKind::VideoChanged);video.index=0;video.value=(1920u<<16)|1080u;
    ffx::copyEscLocalized(0x201,video.text[0]);nativeCapture=true;check(ffx::commitEscPacket(video),"resolution event accepted");nativeCapture=false;
    release();check(take()==L"Resolution: 1920 by 1080","resolution uses width by height");
    blockAgain();video={};ffx::prepareEscPacket(video,ffx::EscCaptureKind::VideoChanged);video.index=5;video.value=2;
    ffx::copyEscLocalized(0x201,video.text[0]);nativeCapture=true;check(ffx::commitEscPacket(video),"AA multiplier accepted");nativeCapture=false;
    release();check(take()==L"Resolution: 4 x","AA value maps to multiplier");
    blockAgain();video={};ffx::prepareEscPacket(video,ffx::EscCaptureKind::VideoChanged);video.index=1;video.value=0;
    ffx::copyEscLocalized(0x201,video.text[0]);ffx::copyEscVideo(1,0,video.text[1]);nativeCapture=true;check(ffx::commitEscPacket(video),"localized video enum accepted");nativeCapture=false;
    release();check(take()==L"Resolution: Fullscreen","video enum is localized from ready copy");

    blockAgain();ffx::EscPacket setting;ffx::prepareEscPacket(setting,ffx::EscCaptureKind::SettingFocus);setting.index=0;setting.value=1;setting.automatic=true;
    ffx::copyEscLocalized(0x200,setting.text[0]);char borrowedDetail[]="Resolution";
    ffx::copyReadyAscii(reinterpret_cast<uintptr_t>(borrowedDetail),setting.text[2],512);setting.detail=ffx::beginSelectionDetail();
    nativeCapture=true;check(ffx::commitEscPacket(setting),"automatic settings detail accepted");std::memset(borrowedDetail,0,sizeof(borrowedDetail));nativeCapture=false;release();
    check(take()==L"Confirm: 1" && take()==L"Resolution","automatic detail follows its headline");
    blockAgain();const unsigned wake=0;check(ffx::speechq::captureDomain(31,&wake,sizeof(wake),nullptr,0,ffx::generation()),"manual-read barrier wake");held();
    nativeCapture=true;check(ffx::readSelectionDetail(),"manual detail request accepted");nativeCapture=false;release();
    check(take()==L"Resolution","manual I path uses current selection buffer transport");

    blockAgain();ffx::EscPacket repeated;ffx::prepareEscPacket(repeated,ffx::EscCaptureKind::AudioChanged);repeated.value=37;
    ffx::copyEscLocalized(0x200,repeated.text[0]);nativeCapture=true;
    check(ffx::commitEscPacket(repeated)&&ffx::commitEscPacket(repeated),"equal accepted events remain distinct");nativeCapture=false;release();
    check(take()==L"Confirm: 37"&&take()==L"Confirm: 37","equal events preserve both outputs");

    blockAgain();const auto pressureStart=GetTickCount64();unsigned accepted=0;
    nativeCapture=true;
    while(accepted<2048 && ffx::commitEscPacket(repeated)) ++accepted;
    check(accepted>0 && accepted<2048,"held-worker capture reaches bounded pressure");
    check(GetTickCount64()-pressureStart<100,"capture pressure remains immediate");
    check(allocations.load()==0&&destructions.load()==0,"pressure rejection has no native heap activity");
    ffx::resetEscCapture();nativeCapture=false;const auto pressureDrain=drains.load();release();awaitDrain(pressureDrain);

    blockAgain();ffx::EscPacket stale;ffx::prepareEscPacket(stale,ffx::EscCaptureKind::AudioChanged);stale.value=99;ffx::copyEscLocalized(0x200,stale.text[0]);
    const auto before=drains.load();nativeCapture=true;check(ffx::commitEscPacket(stale),"stale event queued behind held worker");ffx::resetEscCapture();nativeCapture=false;release();
    awaitDrain(before);ffx::speechq::Utterance staleOutput;
    check(!ffx::speechq::take(staleOutput),"reset cancels stale queued packet");
    check(allocations.load()==0&&destructions.load()==0,"native publication has no allocation or destruction");
    puts("PASS: ESC ready copies, ordering, focus/change binding forms, video/audio values, manual/automatic details, distinct equal events and reset cancellation.");
    return 0;
}
