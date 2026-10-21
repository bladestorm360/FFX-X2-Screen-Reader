#include "../battle_sensor.hpp"
#include "../message_text.hpp"
#include "../ready_game_text.hpp"
#include "../settings.hpp"
#include "../speech_domains.hpp"
#include "../speech_queue.hpp"
#include <atomic>
#include <algorithm>
#include <cstdio>
#include <cstdlib>
#include <cstring>

namespace ffx { void installTextPlans(); void installBattleSensor(); }
namespace {
thread_local bool nativeOwner=false;
std::atomic<unsigned> allocations{0},frees{0};
unsigned textGetterCalls=0; LARGE_INTEGER requestAt{},firstOutputAt{},coldStart{},coldEnd{};
std::atomic<bool> hold{true};
 HANDLE entered=nullptr,resume=nullptr,wake=nullptr;LARGE_INTEGER warmStart{},warmEnd{};
uint8_t actors[31][0xf90]{}, players[20][40]{}, windowBytes[0xf0]{};
uint8_t currentSlot=0, activeBattle=1, languageValue=0;
uint16_t selectedTarget=20; int battleFlag=0;
uint8_t renderWindow[32]{}; uintptr_t renderWindows[8]{};
uintptr_t actorArray=reinterpret_cast<uintptr_t>(actors);
uint8_t help[0x4001]{}, poison[40]{}, haste[40]{}, scan[40]{};
uint8_t oversizedMessage[0x1002]{},largeMessage[0x1000]{},macroMiddle[0x1000]{},macroDeep[0x1000]{},macroLeaf[0x1000]{};
struct Hook {unsigned rva;ffx::Handler enter,leave;}; Hook hooks[8]{};unsigned hookCount=0;
void check(bool ok,const char* s){if(!ok){std::fprintf(stderr,"FAIL: %s\n",s);std::exit(1);}}
void encode(uint8_t* out,const wchar_t* s){constexpr wchar_t a[]=L"0123456789 !\"#$%&'()*+,-./:;<=>?ABCDEFGHIJKLMNOPQRSTUVWXYZ[\\]^_‘abcdefghijklmnopqrstuvwxyz";while(*s){if(*s==L'\n'){++s;*out++=3;continue;}auto p=std::wcschr(a,*s++);check(p!=nullptr,"fixture glyph");*out++=static_cast<uint8_t>(0x30+p-a);}*out=0;}
Hook find(unsigned r){for(unsigned i=0;i<hookCount;++i)if(hooks[i].rva==r)return hooks[i];check(false,"production hook");return {};}
void held(){check(WaitForSingleObject(entered,3000)==WAIT_OBJECT_0,"worker barrier entered");}
void release(){nativeOwner=false;hold=false;SetEvent(resume);}
std::wstring take(){auto until=GetTickCount64()+3000;ffx::speechq::Utterance u;while(!ffx::speechq::take(u)){auto now=GetTickCount64();if(now>=until)break;WaitForSingleObject(wake,static_cast<DWORD>(until-now));}check(!u.text.empty(),"speech sink output");return u.text;}
}
void* operator new(size_t n){if(nativeOwner)++allocations;if(auto p=std::malloc(n?n:1))return p;throw std::bad_alloc();}
void* operator new[](size_t n){return ::operator new(n);}
void operator delete(void* p)noexcept{if(nativeOwner&&p)++frees;std::free(p);}
void operator delete[](void* p)noexcept{::operator delete(p);}
void operator delete(void* p,size_t)noexcept{::operator delete(p);}
void operator delete[](void* p,size_t)noexcept{::operator delete(p);}
namespace ffx {
std::atomic<int> scene{0};
unsigned generation(){return speechq::generation();} int setting(Setting s){return s==Setting::Speech?1:0;}
uintptr_t address(unsigned r){
 check(nativeOwner,"address lookup on native owner");
 if(r==0x1fcc092)return reinterpret_cast<uintptr_t>(&currentSlot);
 if(r==0xf3c910)return reinterpret_cast<uintptr_t>(windowBytes);
 if(r==0xf3d1b4)return reinterpret_cast<uintptr_t>(&selectedTarget);
 if(r==0xf3d6a0)return reinterpret_cast<uintptr_t>(&battleFlag);
 if(r==0xf3c8fe)return reinterpret_cast<uintptr_t>(&activeBattle);
 if(r==0x14676b0)return reinterpret_cast<uintptr_t>(renderWindows);
 if(r==0xd334cc)return reinterpret_cast<uintptr_t>(&actorArray);
 if(r==0x394030)return reinterpret_cast<uintptr_t>(+[](unsigned id)->uintptr_t{check(nativeOwner,"actor getter owner");return id<31?reinterpret_cast<uintptr_t>(actors[id]):0;});
 if(r==0x385070)return reinterpret_cast<uintptr_t>(+[](unsigned id)->uintptr_t{check(nativeOwner,"player getter owner");return id<20?reinterpret_cast<uintptr_t>(players[id]):0;});
 if(r==0x38f810)return reinterpret_cast<uintptr_t>(+[](unsigned,int)->uintptr_t{check(nativeOwner,"help getter owner");++textGetterCalls;return reinterpret_cast<uintptr_t>(help);});
 if(r==0x4b6610)return reinterpret_cast<uintptr_t>(+[](unsigned,uintptr_t token)->uintptr_t{check(nativeOwner,"message macro getter owner");++textGetterCalls;if(token==reinterpret_cast<uintptr_t>(largeMessage)+4093)return reinterpret_cast<uintptr_t>(macroMiddle);if(token==reinterpret_cast<uintptr_t>(macroMiddle)+4093)return reinterpret_cast<uintptr_t>(macroDeep);if(token==reinterpret_cast<uintptr_t>(macroDeep)+4093)return reinterpret_cast<uintptr_t>(macroLeaf);return 0;});
 if(r==0x3852b0)return reinterpret_cast<uintptr_t>(+[]()->unsigned{check(nativeOwner,"language getter owner");++textGetterCalls;return languageValue;});
 if(r==0x38fd40)return reinterpret_cast<uintptr_t>(+[](int,unsigned id,unsigned)->uintptr_t{check(nativeOwner,"menu getter owner");++textGetterCalls;if(id==0x1009)return reinterpret_cast<uintptr_t>(poison);if(id==0x101d)return reinterpret_cast<uintptr_t>(haste);if(id==0x1020||id==0x1021)return reinterpret_cast<uintptr_t>(scan);return reinterpret_cast<uintptr_t>(help);});
 if(r==0xf3d1b4)return reinterpret_cast<uintptr_t>(&selectedTarget);
 std::fprintf(stderr,"unexpected engine address %08x\n",r);check(false,"unexpected engine address");return 0;
}
bool copy(uintptr_t a,void* b,size_t n){check(nativeOwner,"copy on native owner");std::memcpy(b,reinterpret_cast<void*>(a),n);return true;}
bool inBattle(){check(nativeOwner,"battle check on owner");return true;}
bool hook(unsigned r,Handler e,Handler l){check(nativeOwner,"hook bootstrap owner");hooks[hookCount++]={r,e,l};return true;}
bool hookSnapshot(unsigned,Handler,Handler){return false;} bool hookAddress(uintptr_t,unsigned,Handler,Handler){return false;}
void prepareCopiedControlSnapshot(CopiedControls& c){c={};}
OwnedTextContext copiedControlContext(const CopiedControls&){check(!nativeOwner,"format context on worker");return {};}
std::wstring join(const std::vector<std::wstring>& v,const wchar_t* sep){check(!nativeOwner,"join on worker");std::wstring o;for(auto&s:v)if(!s.empty()){if(!o.empty())o+=sep;o+=s;}return o;}
bool readCopiedTextDetail(){return false;} void resetCopiedTextDetail(){}
uint8_t equippedItem(unsigned,bool){check(false,"unused equipment getter");return 255;}
std::wstring buttonGlyph(uint8_t){check(false,"legacy message-page button decoding unreachable");return {};}
std::vector<std::wstring> decode(uintptr_t,size_t,TextMode){check(false,"legacy message-page decoding unreachable");return {};}
std::wstring clean(std::wstring){check(false,"legacy message-page cleanup unreachable");return {};}
namespace speechq {void captureBeforeDrainForTest(){if(hold){SetEvent(entered);WaitForSingleObject(resume,INFINITE);}}
}
}
using namespace ffx;
int main(){
 windowBytes[2]=1;windowBytes[8]=0;*reinterpret_cast<uint16_t*>(windowBytes+0xd8)=0;*reinterpret_cast<uint16_t*>(windowBytes+0xda)=16;
 renderWindows[0]=reinterpret_cast<uintptr_t>(renderWindow);
 actors[20][0xdc8]=1;*reinterpret_cast<uint16_t*>(actors[20]+0x616)=1;actors[20][0x5dd]=1;
 
 encode(poison,L"Poison");encode(haste,L"Haste");encode(scan,L"Scan");encode(help,L"Stone Gaze: weak to Fire\nAvoid petrification");
 // The producer's real ranges are 1006+permanent-index, 1012+timed-index,
 // and 1020+extra-index. Select 1009 Poison, 101D Haste, and two distinct
 // 1020-range effects whose native names are intentionally equal.
 *reinterpret_cast<uint16_t*>(actors[20]+0x606)=static_cast<uint16_t>(1u<<3);
 actors[20][0x608+11]=1;
 *reinterpret_cast<uint16_t*>(actors[20]+0x616)=3;
 entered=CreateEventW(nullptr,TRUE,FALSE,nullptr);resume=CreateEventW(nullptr,TRUE,FALSE,nullptr);wake=CreateEventW(nullptr,FALSE,FALSE,nullptr);
 check(entered&&resume&&wake,"barrier handles");
 nativeOwner=true;installTextPlans();installBattleSensor();nativeOwner=false;
 check(hookCount==1,"battle sensor event hook installed");check(speechq::start(GetModuleHandleW(nullptr),wake),"real speech worker started");held();
 QueryPerformanceCounter(&requestAt);QueryPerformanceCounter(&coldStart);nativeOwner=true;highlightBattleSensor(reinterpret_cast<uintptr_t>(windowBytes));QueryPerformanceCounter(&coldEnd);QueryPerformanceCounter(&warmStart);highlightBattleSensor(reinterpret_cast<uintptr_t>(windowBytes));QueryPerformanceCounter(&warmEnd);nativeOwner=false;
 nativeOwner=true;
 check(battleSensorAvailable()&&battleSensorCapturesKey(VK_LEFT,true),"sensor owns review keys while preparation is pending");
 check(dispatchBattleSensorKey(VK_LEFT,true),"left key captured");check(dispatchBattleSensorKey(VK_RIGHT,true),"right key captured");check(dispatchBattleSensorKey(VK_RIGHT,true),"repeat right captured");check(dispatchBattleSensorKey(VK_UP,true),"up key captured");check(dispatchBattleSensorKey(VK_UP,true),"second up key captured");
 auto getterCountAfterCapture=textGetterCalls;
 // Four 4 KiB native source segments exercise the message reader's per-source
 // bound and the 16 KiB aggregate capture budget through nested substitutions.
 uint8_t a[40]{};encode(a,L"A");
 auto segment=[&](uint8_t* out){std::fill_n(out,4093,a[0]);out[4093]=18;out[4094]=0;out[4095]=0;};
 segment(largeMessage);segment(macroMiddle);segment(macroDeep);std::fill_n(macroLeaf,0xfff,a[0]);macroLeaf[0xfff]=0;
 auto queueMessage=[&](uintptr_t source){TextPlan p;prepareTextPlan(p,"sensor-message-fixture");p.headline=planMessage(p,0,source,true);check(p.valid&&p.headline,"message source captured");check(commitTextPlan(p),"message event committed atomically");};
 nativeOwner=true;queueMessage(reinterpret_cast<uintptr_t>(largeMessage));help[0]=0;queueMessage(reinterpret_cast<uintptr_t>(help));encode(help,L"Marker");queueMessage(reinterpret_cast<uintptr_t>(help));
 std::memset(oversizedMessage,0x51,sizeof(oversizedMessage));oversizedMessage[0x1001]=0;
 TextPlan tooLong;prepareTextPlan(tooLong,"source-message-overflow");tooLong.headline=planMessage(tooLong,0,reinterpret_cast<uintptr_t>(oversizedMessage));
 check(!tooLong.valid&&!commitTextPlan(tooLong),"actual nonterminated source exceeding 4 KiB rejects whole message rather than publishing prefix");
 TextPlan overflow;prepareTextPlan(overflow,"sensor-message-overflow");std::array<wchar_t,2048> fill{};fill.fill(L'X');for(unsigned i=0;i<64;++i)planLiteral(overflow,fill.data());check(!overflow.valid&&!commitTextPlan(overflow),"overflow rejects the complete event before publication");
 std::memset(largeMessage,0,sizeof(largeMessage));std::memset(macroMiddle,0,sizeof(macroMiddle));std::memset(macroDeep,0,sizeof(macroDeep));std::memset(macroLeaf,0,sizeof(macroLeaf));getterCountAfterCapture=textGetterCalls;nativeOwner=false;
 nativeOwner=false;
 std::memset(help,0,sizeof(help));std::memset(poison,0,sizeof(poison));std::memset(haste,0,sizeof(haste));std::memset(scan,0,sizeof(scan));actors[20][0x5dd]=0;actors[20][0x608+11]=0;*reinterpret_cast<uint16_t*>(actors[20]+0x606)=0;*reinterpret_cast<uint16_t*>(actors[20]+0x616)=0;
 release();
 auto first=take();QueryPerformanceCounter(&firstOutputAt);auto last=take(), repeatLast=take(), previous=take();
 auto status=take();auto large=take(),marker=take();ffx::speechq::Utterance noExtra;check(!speechq::take(noExtra),"empty source emits no partial speech before FIFO marker");
 if(first!=L"Weakness: Fire"||last!=L"Avoid petrification"||repeatLast!=L"Avoid petrification"||previous!=L"Stone Gaze: weak to Fire"||status!=L"Status: Poison, Haste, Scan, Scan")
  std::fwprintf(stderr,L"outputs=[%ls][%ls][%ls][%ls][%ls]\n",first.c_str(),last.c_str(),repeatLast.c_str(),previous.c_str(),status.c_str());
 check(first==L"Weakness: Fire","captured weakness survives native source overwrite");
 check(last==L"Avoid petrification"&&repeatLast==L"Avoid petrification","right-end review and repeated equal output are preserved");
 check(previous==L"Stone Gaze: weak to Fire","up reviews the previous description line");
 check(status==L"Status: Poison, Haste, Scan, Scan","actual 1009/101D/1020 getter IDs retain equal distinct status names after source overwrite");
 check(large.size()==16377&&std::count(large.begin(),large.end(),L' ')==3&&std::all_of(large.begin(),large.end(),[](wchar_t c){return c==L'A'||c==L' ';}),"held nested message reaches 16 KiB capture budget after source overwrite");
 check(marker==L"Marker","FIFO marker follows the silent empty source after source overwrite");
 check(textGetterCalls==getterCountAfterCapture,"review reuses worker-owned copies without native text getters");
 check(allocations.load()==0&&frees.load()==0,"native capture allocation counters");
 nativeOwner=true;resetBattleSensor();nativeOwner=false;check(!battleSensorAvailable(),"reset immediately clears sensor ownership");
 LARGE_INTEGER frequency{};QueryPerformanceFrequency(&frequency);const double requestMs=frequency.QuadPart?1000.0*(firstOutputAt.QuadPart-requestAt.QuadPart)/frequency.QuadPart:0;
 const double warmMs=frequency.QuadPart?1000.0*(warmEnd.QuadPart-warmStart.QuadPart)/frequency.QuadPart:0;
 const double coldMs=frequency.QuadPart?1000.0*(coldEnd.QuadPart-coldStart.QuadPart)/frequency.QuadPart:0;
 std::printf("PASS: actual battle_sensor/message_text/text_plan/ready_text/speech queue; native capture alloc/free=%u/%u; cold Native collection %.3f ms, warm duplicate guard %.3f ms, fixture request-to-first-sink %.3f ms; held 16 KiB/nested/empty/overflow cases. Timings are fixture-only, not gameplay/backend latency. Fake native addresses and ready memory only; no live game or backend.\n",allocations.load(),frees.load(),coldMs,warmMs,requestMs);
}
