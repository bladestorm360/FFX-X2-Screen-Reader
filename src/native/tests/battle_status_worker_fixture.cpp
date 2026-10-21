#include "../events.hpp"
#include "../ready_game_text.hpp"
#include "../settings.hpp"
#include "../speech_domains.hpp"
#include "../speech_queue.hpp"
#include <atomic>
#include <cstdio>
#include <cstdlib>
#include <cstring>

namespace {
thread_local bool nativeOwner=false;
std::atomic<unsigned> nativeAllocations{0},nativeFrees{0};
std::atomic<bool> holdWorker{true};
HANDLE entered=nullptr,resume=nullptr,sinkSignal=nullptr;
uint8_t actors[31][0xf90]{};
uint8_t playerNames[20][40]{};
uintptr_t actorArray=reinterpret_cast<uintptr_t>(actors);
struct Hook {unsigned rva;ffx::Handler enter,leave;};
Hook hooks[8]{};unsigned hookCount=0;
bool showMaximumHp=false,showMaximumMp=false;
void require(bool ok,const char* why) {if(!ok) {std::fprintf(stderr,"FAIL: %s\n",why);std::exit(1);}}
void encode(uint8_t* out,const wchar_t* value) {
    constexpr wchar_t alphabet[]=L"0123456789 !\"#$%&'()*+,-./:;<=>?ABCDEFGHIJKLMNOPQRSTUVWXYZ[\\]^_\u2018abcdefghijklmnopqrstuvwxyz";
    while(*value) {const auto item=std::wcschr(alphabet,*value++);require(item!=nullptr,"fixture glyph");*out++=static_cast<uint8_t>(0x30+item-alphabet);}*out=0;
}
void held() {require(WaitForSingleObject(entered,3000)==WAIT_OBJECT_0,"worker held before drain");}
void release() {nativeOwner=false;holdWorker=false;SetEvent(resume);}
Hook find(unsigned rva) {for(unsigned i=0;i<hookCount;++i) if(hooks[i].rva==rva)return hooks[i];require(false,"installed hook exists");return {};}
void apply(unsigned actorId,unsigned flag,bool setting) {
    const uintptr_t actor=reinterpret_cast<uintptr_t>(actors[actorId]);
    uint8_t* header=actors[actorId]+0x774;
    header[0]=0;header[1]=1;header[2]=0;header[3]=42;
    uint8_t* packet=header+0x18;uint16_t flags=static_cast<uint16_t>(setting?flag:0);
    std::memcpy(packet+0x14,&flags,sizeof(flags));
    ffx::Event e;e.rva=0x38f0b0;e.args[0]=0;e.args[1]=42;e.args[2]=actorId;e.args[4]=0;
    nativeOwner=true;find(e.rva).enter(e);
    if(setting) actors[actorId][0x606]|=static_cast<uint8_t>(flag);else actors[actorId][0x606]&=static_cast<uint8_t>(~flag);
    header[0]=1;
    find(e.rva).leave(e);nativeOwner=false;
    require(actor==reinterpret_cast<uintptr_t>(actors[actorId]),"actor identity stable");
}
std::wstring take() {
    const auto deadline=GetTickCount64()+3000;ffx::speechq::Utterance result;
    while(!ffx::speechq::take(result)) {const auto now=GetTickCount64();if(now>=deadline)break;WaitForSingleObject(sinkSignal,static_cast<DWORD>(deadline-now));}
    require(!result.text.empty(),"bounded sink wait receives output");return result.text;
}
void resetActor() {std::memset(actors,0,sizeof(actors));for(unsigned i=0;i<31;++i) actors[i][0xdc8]=1;}
}
void* operator new(size_t n) {if(nativeOwner)++nativeAllocations;if(auto p=std::malloc(n?n:1))return p;throw std::bad_alloc();}
void* operator new[](size_t n) {return ::operator new(n);}
void operator delete(void* p) noexcept {if(nativeOwner&&p)++nativeFrees;std::free(p);}
void operator delete[](void* p) noexcept {::operator delete(p);}
void operator delete(void* p,size_t) noexcept {::operator delete(p);}
void operator delete[](void* p,size_t) noexcept {::operator delete(p);}

namespace ffx {
void installBattleStatus();
void resetBattleStatus();
void queueBattleActiveStats(unsigned,uintptr_t);
unsigned generation() {return speechq::generation();}
int setting(Setting s) {
    if(s==Setting::ShowMaximumHp)return showMaximumHp;
    if(s==Setting::ShowMaximumMp)return showMaximumMp;
    return s==Setting::Speech?1:0;
}
uintptr_t address(unsigned rva) {
    require(nativeOwner,"engine address resolution stays on native owner");
    if(rva==0xd334cc)return reinterpret_cast<uintptr_t>(&actorArray);
    if(rva==0x394030) return reinterpret_cast<uintptr_t>(+[](unsigned id)->uintptr_t {require(nativeOwner,"actor getter stays on native owner");return id<31?reinterpret_cast<uintptr_t>(actors[id]):0;});
    if(rva==0x385070) return reinterpret_cast<uintptr_t>(+[](unsigned id)->uintptr_t {require(nativeOwner,"player name getter stays on native owner");return id<20?reinterpret_cast<uintptr_t>(playerNames[id]):0;});
    require(rva==0x3b09c0,"only optional action owner getter is requested");
    return reinterpret_cast<uintptr_t>(+[](unsigned)->uintptr_t {require(nativeOwner,"stub action getter stays on native owner");return 0;});
}
bool copy(uintptr_t from,void* to,size_t n) {require(nativeOwner,"native memory copies stay on owner");std::memcpy(to,reinterpret_cast<const void*>(from),n);return true;}
void prepareCopiedControlSnapshot(CopiedControls& p) {p={};}
OwnedTextContext copiedControlContext(const CopiedControls&) {require(!nativeOwner,"text context is built on worker");return {};}
std::wstring join(const std::vector<std::wstring>& parts,const wchar_t* separator) {require(!nativeOwner,"status formatting stays on worker");std::wstring out;for(const auto& part:parts)if(!part.empty()){if(!out.empty())out+=separator;out+=part;}return out;}
bool inBattle() {require(nativeOwner,"battle state is read on native owner");return true;}
uint8_t equippedItem(unsigned,bool) {require(false,"unused equipment boundary");return 255;}
bool readCopiedTextDetail() {return false;}
void resetCopiedTextDetail() {}
bool statusAnimationTarget(const Event&,unsigned&,unsigned&) {require(nativeOwner,"animation target boundary stays on native owner");return false;}
bool hook(unsigned rva,Handler enter,Handler leave) {require(nativeOwner,"hook installation boundary");require(hookCount<8,"fixed hook table capacity");hooks[hookCount++]={rva,enter,leave};return true;}
namespace speechq {void captureBeforeDrainForTest() {if(holdWorker.load()){SetEvent(entered);WaitForSingleObject(resume,INFINITE);}}}
}

using namespace ffx;
int main() {
    resetActor();encode(actors[0]+0x540,L"Auron");encode(playerNames[0],L"Auron");
    entered=CreateEventW(nullptr,TRUE,FALSE,nullptr);resume=CreateEventW(nullptr,TRUE,FALSE,nullptr);sinkSignal=CreateEventW(nullptr,FALSE,FALSE,nullptr);
    require(entered&&resume&&sinkSignal,"barrier and sink handles");
    nativeOwner=true;installBattleStatus();nativeOwner=false;require(hookCount==4,"four production battle-status hooks installed");
    require(speechq::start(GetModuleHandleW(nullptr),sinkSignal),"real speech queue starts without PRISM");held();
    apply(0,8,true);apply(0,8,false);
    uint8_t* calcPacket=actors[0]+0x774+0x18;uint8_t counter[16]{};unsigned serial=42;std::memcpy(counter+12,&serial,4);
    Event calc;calc.rva=0x38e680;calc.args[0]=0;calc.args[2]=0;calc.args[3]=reinterpret_cast<uintptr_t>(actors[0]);calc.args[6]=reinterpret_cast<uintptr_t>(calcPacket);calc.args[8]=reinterpret_cast<uintptr_t>(counter);
    nativeOwner=true;find(calc.rva).enter(calc);actors[0][0x65e]=1;find(calc.rva).leave(calc);nativeOwner=false;
    apply(0,0,false);
    require(nativeAllocations.load()==0&&nativeFrees.load()==0,"status and calculation events publish with no native heap work");

    resetActor();encode(actors[0]+0x540,L"Auron");encode(playerNames[0],L"Auron");
    calc={};calc.rva=0x38e680;calc.args[0]=0;calc.args[2]=0;calc.args[3]=reinterpret_cast<uintptr_t>(actors[0]);calc.args[6]=reinterpret_cast<uintptr_t>(actors[0]+0x774+0x18);calc.args[8]=reinterpret_cast<uintptr_t>(counter);
    nativeOwner=true;find(calc.rva).enter(calc);actors[0][0x65e]=1;find(calc.rva).leave(calc);nativeOwner=false;
    nativeOwner=true;resetBattleStatus();nativeOwner=false;apply(0,0,false);
    // H must own the committed values and the per-request preference, even if
    // native memory is reused while the real production worker is held.
    encode(actors[4]+0x540,L"Wakka");encode(playerNames[4],L"Wakka");
    const auto stat=[&](unsigned offset,int value){std::memcpy(actors[4]+offset,&value,sizeof(value));};
    stat(0x594,4618);stat(0x598,150);stat(0x5d0,329);stat(0x5d4,10);
    stat(0x6e4,9236);stat(0x6e8,300);
    nativeOwner=true;queueBattleActiveStats(4,reinterpret_cast<uintptr_t>(actors[4]));nativeOwner=false;
    stat(0x5d0,4618);stat(0x5d4,150);showMaximumHp=showMaximumMp=true;
    nativeOwner=true;queueBattleActiveStats(4,reinterpret_cast<uintptr_t>(actors[4]));nativeOwner=false;
    stat(0x594,9236);stat(0x59c,4618);stat(0x5d0,6116);
    nativeOwner=true;queueBattleActiveStats(4,reinterpret_cast<uintptr_t>(actors[4]));nativeOwner=false;
    std::memset(actors[4],0,sizeof(actors[4]));std::memset(playerNames[4],0,40);
    std::memset(actors[0]+0x540,0,40);std::memset(playerNames[0],0,40);
    release();
    require(take()==L"Auron: poison"&&take()==L"Auron: poison cured"&&take()==L"Auron: Cheer 1","copied names and distinct ordered status and calculated-buff events survive source overwrite");
    require(take()==L"Wakka HP 329 (7%) MP 10. Status: None","H preserves committed current values and gauge preference across worker delay");
    require(take()==L"Wakka HP 4618/4618 MP 150/150. Status: None","fresh H captures restoration despite unchanged forecast");
    require(take()==L"Wakka HP 6116/9236 MP 150/150. Status: None","H captures effective raised maximum rather than base capacity");
    while(WaitForSingleObject(sinkSignal,0)==WAIT_OBJECT_0) {}
    require(WaitForSingleObject(sinkSignal,350)==WAIT_TIMEOUT&&!speechq::pending(),"reset suppresses stale prediction without a fabricated Miss");
    require(nativeAllocations.load()==0&&nativeFrees.load()==0,"all native status capture and event work remains allocation-free");
    std::printf("PASS: actual battle_status/ready-text/text-plan/speech pipeline, copied H totals/preferences, stale forecasts, restoration and effective raised maximum through a held worker/source overwrite. Fake actor getters, hooks and backend; native capture alloc/free zero. No PRISM, live game, animation or audible timing claim.\n");
}
