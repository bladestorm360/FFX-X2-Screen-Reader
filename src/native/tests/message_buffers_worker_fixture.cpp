#include "../events.hpp"
#include "../field_controls.hpp"
#include "../blitz_review.hpp"
#include "../message_text.hpp"
#include "../text_plan.hpp"
#include "../speech_queue.hpp"
#include "../speech_domains.hpp"
#include "../settings.hpp"
#include "../selection_details.hpp"
#include "../navigation.hpp"
#include "../utility_memory.hpp"
#include "../minigame_memory.hpp"
// This host isolates copied message ownership; Jecht voice state is supplied
// by the shared neutral test adapter, not a real game/native voice producer.
#include "test_jecht_caption.hpp"
#include <atomic>
#include <cstdio>
#include <cstdlib>
#include <cstring>

namespace ffx { std::atomic<int> scene{0}; }
namespace {
thread_local bool nativeOwner=false;
std::atomic<unsigned> nativeAllocations{0},nativeFrees{0};
std::atomic<bool> holdWorker{true};
HANDLE entered=nullptr,resumeWorker=nullptr,wakeSink=nullptr;
HANDLE markerDrained=nullptr;
uint8_t windows[8][256]{},cursors[8][32]{},controller[8]{},sources[8][256]{};
int titleValue=0;
uintptr_t controllerPointer=0;
bool poisonUtility=false;
std::atomic<bool> resourcePending{false};
std::atomic<bool> resourceRejected{false};
ffx::AtelPreparedHandler preparedHandler=nullptr;
uintptr_t windowTable[8]{},cursorTable[8]{};
struct Hook { unsigned rva; ffx::Handler enter,leave; } hooks[32]{};
unsigned hookCount=0;
void (*hotkeyCallback)(bool)=nullptr;
bool (*hotkeyAvailable)()=nullptr;
std::vector<std::wstring> spoken;
void drained(const ffx::speechq::Header&,const std::wstring&) { SetEvent(markerDrained); }
void check(bool value,const char* why) { if(!value) { std::fprintf(stderr,"FAIL: %s\n",why); std::exit(1); } }
Hook findHook(unsigned rva) { for(unsigned i=0;i<hookCount;++i) if(hooks[i].rva==rva) return hooks[i]; check(false,"expected message hook");return {}; }
void held() { check(WaitForSingleObject(entered,3000)==WAIT_OBJECT_0,"preparation worker reached held barrier"); }
void holdBeforeNextDrain() {
    holdWorker=true;ResetEvent(entered);ResetEvent(resumeWorker);
    const unsigned marker=0x4d425546;
    check(ffx::speechq::captureDomain(31,&marker,sizeof(marker),nullptr,0,ffx::generation()),"barrier wake marker captured");
    held();
}
void release() { nativeOwner=false;holdWorker=false;SetEvent(resumeWorker); }
void drainMarker() {
    ResetEvent(markerDrained);const unsigned marker=0x4d425546;
    check(ffx::speechq::captureDomain(31,&marker,sizeof(marker),nullptr,0,ffx::generation()),"ordered worker marker captured");
    check(WaitForSingleObject(markerDrained,3000)==WAIT_OBJECT_0,"worker processed ordered marker");
}
std::wstring take() {
    const auto deadline=GetTickCount64()+3000;ffx::speechq::Utterance item;
    while(!ffx::speechq::take(item)) { const auto now=GetTickCount64();if(now>=deadline) break;WaitForSingleObject(wakeSink,static_cast<DWORD>(deadline-now)); }
    check(!item.text.empty(),"actual speech queue produced an utterance");spoken.push_back(item.text);return item.text;
}
constexpr wchar_t alphabet[]=L"0123456789 !\"#$%&'()*+,-./:;<=>?ABCDEFGHIJKLMNOPQRSTUVWXYZ[\\]^_‘abcdefghijklmnopqrstuvwxyz";
void encode(uint8_t* target,const wchar_t* text,bool field=false) {
    unsigned i=0;if(field){target[i++]=9;target[i++]=0x30;}
    while(*text) { if(*text==L'\n') {target[i++]=3;++text;continue;}const auto p=std::wcschr(alphabet,*text++);check(p!=nullptr,"known fixture alphabet glyph");target[i++]=static_cast<uint8_t>(0x30+(p-alphabet)); }
    target[i]=0;
}
void clearOutputs(){spoken.clear();}
void eventFor(unsigned slot,bool opening=false) {
    auto hook=findHook(0x4b8980);ffx::Event e;e.rva=0x4b8980;e.args[0]=slot;
    e.before[0]=opening?1:0;e.before[1]=0;e.before[2]=0;e.before[3]=reinterpret_cast<intptr_t>(windows[slot]);
    nativeOwner=true;hook.enter(e);hook.leave(e);nativeOwner=false;
}
}
void* operator new(size_t n) { if(nativeOwner)++nativeAllocations;if(auto p=std::malloc(n?n:1))return p;throw std::bad_alloc(); }
void* operator new[](size_t n) { return ::operator new(n); }
void operator delete(void* p) noexcept { if(nativeOwner&&p)++nativeFrees;std::free(p); }
void operator delete[](void* p) noexcept { ::operator delete(p); }
void operator delete(void* p,size_t) noexcept { ::operator delete(p); }
void operator delete[](void* p,size_t) noexcept { ::operator delete(p); }

namespace ffx {
unsigned generation(){return speechq::generation();}
int setting(Setting s){return s==Setting::Speech?1:0;}
bool output(const wchar_t* text,bool interrupt,const char* producer) {return speechq::output(text,std::wcslen(text),interrupt,producer,false,generation());}
uintptr_t address(unsigned rva) {
    check(nativeOwner,"native address getter owner");
    if(rva==0x14676b0)return reinterpret_cast<uintptr_t>(windowTable);
    if(rva==0x1468a50)return reinterpret_cast<uintptr_t>(cursorTable);
    if(rva==0xf26ae8)return reinterpret_cast<uintptr_t>(&controllerPointer);
    if(rva==0xefbbf8)return reinterpret_cast<uintptr_t>(&titleValue);
    if(rva==0x4b6610)return reinterpret_cast<uintptr_t>(+[](unsigned,uintptr_t)->uintptr_t{check(nativeOwner,"message substitution getter owner");return 0;});
    check(false,"unexpected Native address getter");return 0;
}
bool copy(uintptr_t from,void* to,size_t size){check(nativeOwner,"bounded Native copy owner");std::memcpy(to,reinterpret_cast<const void*>(from),size);return true;}
bool hook(unsigned r,Handler enter,Handler leave){check(!nativeOwner,"hook bootstrap outside Native callback");check(hookCount<32,"hook table bound");hooks[hookCount++]={r,enter,leave};return true;}
bool hookSnapshot(unsigned,Handler,Handler){return false;} bool hookAddress(uintptr_t,unsigned,Handler,Handler){return false;}
void hotkeyChordWhen(unsigned,bool,void(*callback)(bool),bool(*available)(),bool){hotkeyCallback=callback;hotkeyAvailable=available;}
bool inBattle(){check(nativeOwner,"battle getter Native owner");return false;}
bool swimmingPlane(){check(nativeOwner,"swimming getter Native owner");return true;}
bool sphereGridOpen(){check(nativeOwner,"grid getter Native owner");return false;}
void protectTutorialSpeech(bool){}
void installHelp(){} void resetHelp(){} void installButtonGlyphs(){}
void installDialogueVoice(){} void resetDialogueVoice(){} void installMovieDialogue(bool=false){} void resetMovieDialogue(){}
void submitDialogueOwned(std::wstring text,const speechq::Header& header,const char* producer,bool,bool) { speechq::emitOwned(std::move(text),header,producer,false,true); }
bool readCopiedTextDetail(){return false;} void resetCopiedTextDetail(){}
std::wstring buttonGlyph(uint8_t){check(false,"legacy glyph path is outside this fixture");return {};}
std::vector<std::wstring> decode(uintptr_t,size_t,TextMode){check(false,"legacy decoding path is outside this fixture");return {};}
std::wstring clean(std::wstring){check(false,"legacy cleanup path is outside this fixture");return {};}
uint8_t equippedItem(unsigned,bool){check(false,"unused equipment getter");return 255;}
void prepareCopiedControlSnapshot(CopiedControls& c){check(nativeOwner,"control snapshot Native owner");c={};}
OwnedTextContext copiedControlContext(const CopiedControls&){check(!nativeOwner,"message decoding belongs to preparation worker");return {};}
std::wstring join(const std::vector<std::wstring>& values,const wchar_t* separator){check(!nativeOwner,"message join belongs to preparation worker");std::wstring out;for(const auto& v:values)if(!v.empty()){if(!out.empty())out+=separator;out+=v;}return out;}
UtilityScript utilityScript(){check(nativeOwner,"Blitz utility lookup Native owner");UtilityScript s;s.name="bltz0200";s.length=113266;s.workers=7;s.chunk=1;s.allocationSerial=1;return s;}
bool captureUtilityScript(UtilityScript& s) noexcept {s=utilityScript();return true;}
unsigned scriptChecksum(const UtilityScript&){check(nativeOwner&&!resourcePending,"utility checksum only after preparation");return 0x58a5c0a5;}
uintptr_t utilityWorker(const UtilityScript&,unsigned local){check(nativeOwner&&!resourcePending&&local==1,"utility worker only when ready, Native owner and exact index");static uintptr_t header=0x1234;return reinterpret_cast<uintptr_t>(&header);}
bool captureUtilityWorker(const UtilityScript& s,unsigned local,uintptr_t& w) noexcept {if(resourcePending) return false;w=utilityWorker(s,local);return true;}
AtelPreparation atelPreparation(uintptr_t){return resourcePending?AtelPreparation::Pending:AtelPreparation::Ready;}
bool nativeAtelSpan(uintptr_t chunk,AtelSpan& span){check(nativeOwner,"exact resource span captured by Native owner");span={chunk,generation(),1,200000};return true;}
bool readyAtelIdentity(uintptr_t chunk,AtelIdentity& identity){if(resourcePending)return false;identity.span={chunk,generation(),1,200000};identity.length=113266;identity.workers=7;identity.checksum=resourceRejected?0u:0x58a5c0a5;std::memcpy(identity.name,"bltz0200",9);return true;}
bool onAtelPrepared(AtelPreparedHandler handler){preparedHandler=handler;return true;}
int utilityValue(const UtilityScript&,unsigned low,unsigned high,unsigned element){check(nativeOwner,"utility value getter Native owner");if(low==0x56000000&&high==1)return 0;if(low==0x5d0003f4&&high==16)return 7;if(low==0x010013ce&&high==60)return poisonUtility?9:3;if(low==0x5d000340&&high==6)return 100;switch(low){case 0x5d000040:return (poisonUtility?900:123)+static_cast<int>(element);case 0x5d000300:return (poisonUtility?600:60)+static_cast<int>(element);case 0x5d000180:return (poisonUtility?120:12)+static_cast<int>(element);case 0x5d000100:return (poisonUtility?140:14)+static_cast<int>(element);case 0x5d0001c0:return (poisonUtility?160:16)+static_cast<int>(element);case 0x5d000240:return (poisonUtility?180:18)+static_cast<int>(element);case 0x5d000200:return (poisonUtility?200:20)+static_cast<int>(element);case 0x5d000280:return (poisonUtility?220:22)+static_cast<int>(element);default:check(false,"known utility binding only");return 0;}}
MinigameScript captureMinigameScript(uintptr_t){check(false,"fallback minigame detector should not run");return {};}
bool captureUtilityValue(const UtilityScript& s,unsigned low,unsigned high,unsigned element,int& value) noexcept {try {value=utilityValue(s,low,high,element);return true;}catch(...) {return false;}}
}

namespace ffx::speechq {
void captureBeforeDrainForTest(){if(holdWorker.load(std::memory_order_acquire)){SetEvent(entered);WaitForSingleObject(resumeWorker,INFINITE);}}
}
using namespace ffx;
int main() {
    for(unsigned i=0;i<8;++i){windowTable[i]=reinterpret_cast<uintptr_t>(windows[i]);cursorTable[i]=reinterpret_cast<uintptr_t>(cursors[i]);}
    controllerPointer=reinterpret_cast<uintptr_t>(controller);
    windowTable[0]=reinterpret_cast<uintptr_t>(windows[0]);cursorTable[0]=reinterpret_cast<uintptr_t>(cursors[0]);
    *reinterpret_cast<uint16_t*>(windows[0]+0x2c)=2;*reinterpret_cast<uintptr_t*>(windows[0]+0x94)=reinterpret_cast<uintptr_t>(sources[0]);
    cursors[0][1]=0;controller[0]=2;
    entered=CreateEventW(nullptr,TRUE,FALSE,nullptr);resumeWorker=CreateEventW(nullptr,TRUE,FALSE,nullptr);wakeSink=CreateEventW(nullptr,FALSE,FALSE,nullptr);markerDrained=CreateEventW(nullptr,TRUE,FALSE,nullptr);check(entered&&resumeWorker&&wakeSink&&markerDrained,"worker barrier handles");
    speechq::registerDomain(31,drained,nullptr);
    check(speechq::start(GetModuleHandleW(nullptr),wakeSink),"actual speech preparation worker starts");
    nativeOwner=false;installTextPlans();installSelectionDetails();installMessages();
    check(hookCount>=9&&hotkeyCallback&&hotkeyAvailable,"message hooks and T hotkey installed");

    // NPC caption and T repeat are both queued while the real worker is held.
    holdBeforeNextDrain();encode(sources[0],L"Dialogue one");eventFor(0);
    nativeOwner=true;check(hotkeyAvailable(),"T remains available during pending preparation");hotkeyCallback(false);nativeOwner=false;
    std::memset(sources[0],0,sizeof(sources[0]));release();
    const auto caption=take(),repeat=take();check(caption==L"Dialogue one"&&repeat==L"Dialogue one","captured caption and manual repeat survive overwrite and preserve the owned guard copy");
    drainMarker();speechq::Utterance extraCaption;check(!speechq::take(extraCaption),"caption event and T repeat produce exactly two outputs");
    holdBeforeNextDrain();encode(sources[0],L"Dialogue one");eventFor(0);std::memset(sources[0],0,sizeof(sources[0]));release();
    // A duplicate subtitle/page is filtered only by the established last-dialogue guard.
    drainMarker();speechq::Utterance noDuplicate;check(!speechq::take(noDuplicate),"reinstalled identical page produces no extra output under the existing dialogue guard");
    holdBeforeNextDrain();encode(sources[0],L"Another page");eventFor(0);std::memset(sources[0],0,sizeof(sources[0]));release();check(take()==L"Another page","different page becomes current dialogue");
    holdBeforeNextDrain();encode(sources[0],L"Dialogue one");eventFor(0);std::memset(sources[0],0,sizeof(sources[0]));release();check(take()==L"Dialogue one","first page can speak again after a different page");
    nativeOwner=true;Event close;close.args[0]=0;findHook(0x4b8510).leave(close);check(!hotkeyAvailable(),"closing the Native window invalidates T immediately");nativeOwner=false;

    // Field-control capture is a copied POD candidate; two queued reviews use one owned worker buffer.
    holdBeforeNextDrain();encode(sources[0],L"9:30\nFirst line\nSecond line",true);eventFor(0);nativeOwner=true;
    const auto pendingField=fieldControlsForSource(0,reinterpret_cast<uintptr_t>(sources[0]));
    check(pendingField.candidate && classifyFieldControls(pendingField)==FieldControlsClassification::Pending,"native HUD classification copies metadata while decoding is held");
    check(fieldControlsCapturesKey(VK_UP,true),"field review chord captured while worker is held");
    check(dispatchFieldControlsKey(VK_DOWN,true)&&dispatchFieldControlsKey(VK_RIGHT,true)&&dispatchFieldControlsKey(VK_RIGHT,true),"field review commands queued without reading text");
    std::memset(sources[0],0,sizeof(sources[0]));release();
    const auto fieldPrompt=take(),fieldLine=take(),fieldAll=take(),fieldAllAgain=take();
    check(fieldPrompt==L"9:30 First line Second line"&&fieldLine==L"First line","field prompt and selected second source line use copied field buffer");
    check(fieldAll==L"9:30. First line. Second line"&&fieldAllAgain==fieldAll,"right review joins copied lines and repeats equal output");
    nativeOwner=true;check(classifyFieldControls(pendingField)==FieldControlsClassification::Nonblocking,"owned HUD classification is published for the exact copied page serial");nativeOwner=false;
    nativeOwner=true;resetMessages();nativeOwner=false;
    holdBeforeNextDrain();encode(sources[0],L"10:00\nStale field",true);eventFor(0);nativeOwner=true;dispatchFieldControlsKey(VK_RIGHT,true);resetMessages();nativeOwner=false;release();
    drainMarker();speechq::Utterance stale;check(!speechq::take(stale),"reset prevents queued stale field capture from reviving ownership or speech");
    nativeOwner=true;check(!fieldControlsCapturesKey(VK_RIGHT,true),"reset clears field native ownership immediately");nativeOwner=false;

    // Blitz uses the verified utility identity and copied scalar snapshot; all review strings stay worker-owned.
    nativeOwner=true;resetMessages();nativeOwner=false;
    for(unsigned slot=1;slot<=3;++slot){windowTable[slot]=reinterpret_cast<uintptr_t>(windows[slot]);cursorTable[slot]=reinterpret_cast<uintptr_t>(cursors[slot]);*reinterpret_cast<uint16_t*>(windows[slot]+0x2c)=2;*reinterpret_cast<uintptr_t*>(windows[slot]+0x94)=reinterpret_cast<uintptr_t>(sources[slot]);cursors[slot][1]=0;}
    holdBeforeNextDrain();
    sources[1][0]=16;sources[1][1]=0x30;encode(sources[1]+2,L"Choice");cursors[1][1]=1;
    encode(sources[2],L"Tidus");encode(sources[3],L"Venom Tackle\n30 HP");
    eventFor(1);eventFor(2);eventFor(3);
    nativeOwner=true;check(blitzCapturesKey(VK_LEFT,true),"Blitz review owns Ctrl arrows while worker is held");
    dispatchBlitzKey(VK_LEFT,true);dispatchBlitzKey(VK_LEFT,true);dispatchBlitzKey(VK_DOWN,true);dispatchBlitzKey(VK_DOWN,true);dispatchBlitzKey(VK_RIGHT,true);dispatchBlitzKey(VK_RIGHT,true);
    std::memset(sources[1],0,sizeof(sources[1]));std::memset(sources[2],0,sizeof(sources[2]));std::memset(sources[3],0,sizeof(sources[3]));poisonUtility=true;nativeOwner=false;
    release();
    check(take()==L"Choice","visible native selector speaks its encoded choice");
    check(take()==L"Venom Tackle 30 HP","native copied move window retains its displayed text");
    const auto blitzName=take(),blitzNameAgain=take(),blitzLevel=take(),blitzHp=take(),blitzAll=take(),blitzAllAgain=take();
    check(blitzName==L"Tidus"&&blitzNameAgain==blitzName,"repeated Ctrl Left keeps the player name buffer");
    check(blitzLevel==L"Level: 3"&&blitzHp==L"HP: 123","queued Ctrl Down steps through the copied level and stat rows");
    check(blitzAll==blitzAllAgain,"equal complete Blitz review outputs are both preserved");
    const wchar_t* ordered[]={L"HP: 123",L"Speed: 60",L"Endurance: 12",L"Attack: 14",L"Pass: 16",L"Block: 18",L"Shot: 20",L"Catch: 22",L"Venom Tackle",L"30 HP"};size_t previous=0;
    for(const auto* value:ordered){const auto at=blitzAll.find(value,previous);check(at!=std::wstring::npos,"joined Blitz review includes actual stats and move in order");previous=at+std::wcslen(value);}
    check(blitzAll.find(L"900")==std::wstring::npos,"Blitz review uses Native utility facts captured before overwrite");

    // A captured page stays owned while its exact ATEL resource is pending.
    holdBeforeNextDrain();resourcePending=true;
    sources[1][0]=16;sources[1][1]=0x30;encode(sources[1]+2,L"Pending choice");cursors[1][1]=1;
    encode(sources[2],L"Original name");encode(sources[3],L"Original move\n20 HP");
    eventFor(1);eventFor(2);eventFor(3);
    nativeOwner=true;check(!blitzCapturesKey(VK_LEFT,true),"pending exact resource cannot publish review controls");nativeOwner=false;
    std::memset(sources[1],0,sizeof(sources[1]));std::memset(sources[2],0,sizeof(sources[2]));std::memset(sources[3],0,sizeof(sources[3]));
    release();drainMarker();
    speechq::Utterance earlyBlitz;check(!speechq::take(earlyBlitz),"pending Blitz messages produce no early speech");
    nativeOwner=true;check(!blitzCapturesKey(VK_LEFT,true),"review remains unavailable while retained pages await preparation");nativeOwner=false;
    resourcePending=false;check(preparedHandler!=nullptr,"Blitz preparation callback registered");preparedHandler();drainMarker();
    check(take()==L"Pending choice" && take()==L"Original move 20 HP","resource wake preserves the original selector and move speech order");
    nativeOwner=true;check(blitzCapturesKey(VK_LEFT,true)&&dispatchBlitzKey(VK_RIGHT,true),"ready resource enables query over retained pages");nativeOwner=false;
    drainMarker();const auto retainedBlitz=take();
    check(retainedBlitz.find(L"Original name")!=std::wstring::npos&&retainedBlitz.find(L"Original move")!=std::wstring::npos&&retainedBlitz.find(L"Pending choice")==std::wstring::npos,"prepared review retains the original name and move; selector speech remains outside the existing review buffer");

    holdBeforeNextDrain();resourcePending=true;encode(sources[2],L"Canceled pending page");eventFor(2);
    std::memset(sources[2],0,sizeof(sources[2]));release();drainMarker();
    nativeOwner=true;resetMessages();nativeOwner=false;
    resourcePending=false;preparedHandler();drainMarker();
    speechq::Utterance canceledBlitz;check(!speechq::take(canceledBlitz),"message reset retires retained pending pages and their late ticket releases cannot revive speech");

    // A matching header/name is only a candidate. Rejected owned code must
    // restore the ordinary producer and T review, using the original bytes.
    holdBeforeNextDrain();resourcePending=true;encode(sources[2],L"Ordinary changed script");eventFor(2);
    nativeOwner=true;check(!hotkeyAvailable(),"pending name-only candidate cannot claim ordinary T review");nativeOwner=false;
    std::memset(sources[2],0,sizeof(sources[2]));release();drainMarker();resourceRejected=true;resourcePending=false;preparedHandler();
    check(take()==L"Ordinary changed script","rejected code qualification restores original copied ordinary dialogue");
    nativeOwner=true;check(hotkeyAvailable(),"rejected candidate restores native window T ownership");hotkeyCallback(false);nativeOwner=false;
    check(take()==L"Ordinary changed script","T repeats rejected candidate's original owned ordinary text");resourceRejected=false;

    holdBeforeNextDrain();nativeOwner=true;resetMessages();nativeOwner=false;
    sources[1][0]=16;sources[1][1]=0x30;sources[1][2]=0;eventFor(1);
    nativeOwner=true;auto fresh=captureBlitzSnapshot(true,false);check(fresh.valid,"fresh Blitz selector retains verified utility identity");dispatchBlitzKey(VK_RIGHT,true);std::memset(sources[1],0,sizeof(sources[1]));nativeOwner=false;release();
    check(take()==L"No Blitzball details are displayed.","fresh selector clears stale stats and empty review is explicit");
    // Exercise the real speech cancellation performed by input before its
    // queued T/arrow review, rather than bypassing cancellation in a direct key.
    drainMarker();nativeOwner=true;resetMessages();nativeOwner=false;
    holdBeforeNextDrain();encode(sources[0],L"Canceled caption remains current");eventFor(0);
    speechq::interrupt();nativeOwner=true;check(hotkeyAvailable(),"current caption owns T after input cancellation");hotkeyCallback(false);nativeOwner=false;
    std::memset(sources[0],0,sizeof(sources[0]));release();
    check(take()==L"Canceled caption remains current","fresh T reads prepared copied buffer while the canceled original caption stays silent");
    drainMarker();speechq::Utterance canceledOriginal;check(!speechq::take(canceledOriginal),"only the fresh T repeat is emitted");
    holdBeforeNextDrain();encode(sources[0],L"First line\nSecond line",true);eventFor(0);
    speechq::interrupt();nativeOwner=true;check(!hotkeyAvailable(),"field-control installation releases dialogue T immediately");
    check(dispatchFieldControlsKey(VK_DOWN,true),"fresh field arrow owns pending copied lines after cancellation");nativeOwner=false;
    std::memset(sources[0],0,sizeof(sources[0]));release();check(take()==L"Second line","fresh arrow reads its owned row without reviving the canceled field headline");
    drainMarker();check(!speechq::take(canceledOriginal),"canceled field headline remains silent");
    check(nativeAllocations.load()==0&&nativeFrees.load()==0,"Native callbacks make no C++ new/delete calls");
    std::printf("PASS: actual message/text-plan/field/Blitz capture, held preparation worker, real speech queue; Native C++ new/delete=%u/%u. Fake addresses, controller/swimming/battle and utility facts; no PRISM voice-policy/backend claim.\n",nativeAllocations.load(),nativeFrees.load());
}
