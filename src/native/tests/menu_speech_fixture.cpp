// Synthetic memory only: exercise production menu hooks, copied plans and workers.
#include "../events.hpp"
#include "../profile.hpp"
#include "../settings.hpp"
#include "../text_plan.hpp"
#include "../selection_details.hpp"
#include "../copied_controls.hpp"
#include "ready_speech_adapter.hpp"
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <functional>
#include <map>

namespace ffx {
uintptr_t testBase=0;
std::atomic<int> scene{0};
bool testForeground=true,owner=true;
struct Listener {Handler enter,leave;};
std::map<unsigned,Listener> listeners;
std::vector<std::pair<std::wstring,bool>> said;
unsigned generation() {return 1;}
bool nativeGameThread() {return owner;}
bool nativeResourceLoading() noexcept {return false;}
bool hook(unsigned id,Handler enter,Handler leave) {listeners[id]={enter,leave};return true;}
void hotkey(unsigned,void(*)(bool)) {}
bool output(const std::wstring& text,bool interrupt,const char*) {
    if(owner) {std::fprintf(stderr,"FAIL: speech ran on native owner\n");std::exit(1);}
    if(!text.empty()) said.emplace_back(text,interrupt);return true;
}
bool outputDialogue(const std::wstring& text,bool interrupt,const char* producer,bool) {return output(text,interrupt,producer);}
void log(const char*,...) {}
void prepareCopiedControlSnapshot(CopiedControls& controls) {controls=CopiedControls{};}
OwnedTextContext copiedControlContext(const CopiedControls&) {return {};}
bool readCopiedTextDetail() {return false;}
void resetCopiedTextDetail() {}
void installSaveLoadMenu() {}
void installFieldDetails() {}
void installAeonAttributes() {}
void installFieldTargets() {}
void installStatusReview() {}
void resetStatusReview() {}
void installShops() {}
void installConfirmations() {}
void installEscSettings() {}
void installEscDetails() {}
void resetEscDetails() {}
void installNaming() {}
void resetNaming() {}
bool announceEscSetting(uintptr_t) {return false;}
bool escMenuActive() {return false;}
bool readEscControls() {return false;}
bool readHelpControls() {return false;}
bool readEnteredName() {return false;}
bool readBattleSummonDetail() {return false;}
bool readSphereDescription() {return false;}
bool statusReviewAvailable() {return false;}
bool battleResultsReviewActive() {return false;}
void refreshBattleResultReview() {}
void announceIncreaseRequirements(int) {}
}
#define drainSpeechDomainsForTest fixtureDrainDomains
#include "test_speech_domains.hpp"
#undef drainSpeechDomainsForTest
namespace ffx {
void drainSpeechDomainsForTest() {const bool wasOwner=owner;owner=false;fixtureDrainDomains();owner=wasOwner;}
}
using namespace ffx;
namespace {
constexpr unsigned win=0x2200000;
unsigned checks=0;
void check(bool ok,const char* why) {++checks;if(!ok) {std::fprintf(stderr,"FAIL: %s\n",why);std::exit(1);}}
template<class T> void set(unsigned off,T value) {check(write<T>(address(off),value),"synthetic write");}
void drain() {owner=false;drainSpeechDomainsForTest();owner=true;}
void expect(const wchar_t* expected,const char* why) {check(!said.empty() && said.back().first==expected,why);}
void event(unsigned id,uintptr_t arg,const std::function<void()>& change={}) {
    auto listener=listeners.at(id);Event e;e.rva=id;e.args[0]=arg;e.epoch=generation();
    if(listener.enter) listener.enter(e);if(change) change();if(listener.leave) listener.leave(e);drain();
}
void encoded(unsigned off,const wchar_t* text) {
    static const wchar_t charset[]=L"0123456789ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz";
    auto* dst=reinterpret_cast<uint8_t*>(address(off));
    for(;*text;++text,++dst) {
        if(*text==L' ') {*dst=0x3a;continue;}
        const auto* found=std::wcschr(charset,*text);check(found!=nullptr,"synthetic game text encodes");
        const auto i=found-charset;*dst=static_cast<uint8_t>(i<10?0x30+i:i<36?0x50+i-10:0x70+i-36);
    }
    *dst=0;
}
void stub(unsigned off,void* function) {
    auto* code=reinterpret_cast<uint8_t*>(address(off));code[0]=0xb8;
    const auto target=reinterpret_cast<uintptr_t>(function);std::memcpy(code+1,&target,4);code[5]=0xff;code[6]=0xe0;
    FlushInstructionCache(GetCurrentProcess(),code,7);
}
unsigned __cdecl language() {return 0;}
uintptr_t __cdecl record(unsigned,uintptr_t* base) {if(base)*base=address(0x2100000);return address(0x2101000);}
uintptr_t __cdecl gear(unsigned) {return address(0x2100000);}
uintptr_t __cdecl equipment(unsigned,uintptr_t*) {return 0;}
void items(unsigned count=3,unsigned kind=0) {
    std::memset(reinterpret_cast<void*>(address(win)),0,0x100);
    scene=kind==2?2:3;set<int>(win+0x28,1);set<uint16_t>(win+0x30,static_cast<uint16_t>((count+1)/2));
    set<int8_t>(win+0x45,0);set<uint8_t>(win+0x42,1);set<uint8_t>(win+0x43,static_cast<uint8_t>(kind));
    set<int>(win+0x94,count);
    for(unsigned i=0;i<4;++i) {set<uint16_t>(0x1197730+i*4,0x2001);set<uint8_t>(0x1197732+i*4,3);}
    event(0x4c6da0,address(win),[]{set<int>(win+0x28,2);});
}
void emptyItems() {
    setSetting(Setting::MenuIndex,0);setSetting(Setting::AutoDetail,1);items();expect(L"Restores HP","populated item has automatic detail");
    auto n=said.size();event(0x4c6da0,address(win),[]{set<uint8_t>(0x1197732,0);});
    check(said.size()==n+1,"zero quantity speaks once without automatic stale detail");expect(L"Empty","zero quantity announces Empty");
    check(readSelectionDetail(),"empty item retains detail context");drain();expect(L"No native description is available for this selection.","zero quantity retires prior detail");
    n=said.size();event(0x4c6da0,address(win));check(said.size()==n,"idle empty item stays silent");
    event(0x4c6da0,address(win),[]{set<int>(win+0x58,1);set<uint8_t>(0x1197736,0);});expect(L"Empty","adjacent empty cell speaks despite equal text");
    setSetting(Setting::AutoDetail,0);items();event(0x4c6da0,address(win),[]{set<int16_t>(win+0x48,1);set<int>(win+0x58,1);});
    expect(L"Empty","odd final row trailing cell announces Empty");
    check(readSelectionDetail(),"trailing cell retains detail context");drain();expect(L"No native description is available for this selection.","trailing cell clears prior detail");
    n=said.size();event(0x4c6da0,address(win),[]{set<int16_t>(win+0x48,2);});check(said.size()==n,"outside native row count stays silent");
    n=said.size();items(0);check(said.size()==n,"zero-row list stays silent");
    event(0x4c6da0,address(win),[]{set<uint16_t>(win+0x30,1);});expect(L"Empty","native blank row with zero items speaks");
    items(1,2);event(0x4c6da0,address(win),[]{set<uint8_t>(0x1197732,0);});expect(L"Empty","blank Abilities entry speaks");
}
void slots() {
    setSetting(Setting::AutoDetail,0);
    for(const auto popup:{0x302fe0u,0x302f70u}) {
        event(popup,0);event(0x302db0,3);expect(L"Slot 3 empty","initial slot speaks");
        auto n=said.size();event(0x302db0,3);check(said.size()==n,"idle slot stays silent");
        event(popup,0);n=said.size();event(0x302db0,3);
        check(said.size()==n+1,"same save/load slot speaks after reopening");expect(L"Slot 3 empty","reopened slot label");
    }
}
void yesNo() {
    for(const auto popup:{0x302fe0u,0x302f70u}) for(const auto cursor:{0,1}) {
        set<int>(0x8e72f0,cursor);event(popup,0);event(0x2efd90,0);
        expect(cursor?L"Yes":L"No","initial answer speaks");
        auto n=said.size();event(0x2efd90,0);check(said.size()==n,"idle answer stays silent");
        event(popup,0);n=said.size();event(0x2efd90,0);
        check(said.size()==n+1,"same answer speaks for both reopened popup types");
        expect(cursor?L"Yes":L"No","reopened answer label");
    }
}
void indexing() {
    setSetting(Setting::AutoDetail,0);setSetting(Setting::MenuIndex,1);items();expect(L"Potion, 3, 1 of 3","Items uses cell position and total count");
    event(0x4c6da0,address(win),[]{set<int>(win+0x58,1);});expect(L"Potion, 3, 2 of 3","second column has next position");
    event(0x4c6da0,address(win),[]{set<int16_t>(win+0x48,1);set<int>(win+0x58,0);});expect(L"Potion, 3, 3 of 3","odd list final item index");
    event(0x4c6da0,address(win),[]{set<int>(win+0x58,1);});expect(L"Empty","trailing blank never claims an out-of-range index");
    setSetting(Setting::MenuIndex,0);items(1);expect(L"Potion, 3","index Off preserves speech");
    setSetting(Setting::MenuIndex,1);items(1,2);expect(L"Potion, 1 of 1","Abilities indexes a single entry");
    scene=4;set<uintptr_t>(win+0x10,address(0x4cf9c0));set<uint16_t>(win+0x30,3);set<int>(win+0x28,1);
    event(0x4b4460,address(win),[]{set<int>(win+0x28,2);});expect(L"Potion, 1 of 3","generic equipment list uses native row count");
    event(0x4b4460,address(win),[]{set<int16_t>(win+0x48,2);});expect(L"Potion, 3 of 3","generic last row index");
    auto n=said.size();event(0x4b4460,address(win),[]{set<uint16_t>(win+0x30,0);});check(said.size()==n,"generic zero-row list stays silent");
    event(0x4b4460,address(win),[]{set<uint16_t>(win+0x30,1);set<int16_t>(win+0x48,0);});expect(L"Potion, 1 of 1","generic single-row list index");
    set<uint16_t>(win+0x30,3);
    setSetting(Setting::MenuIndex,0);event(0x4b4460,address(win),[]{set<int16_t>(win+0x48,1);});expect(L"Potion","generic index Off preserves name");
}
}
int main(int argc,char** argv) {
    testBase=reinterpret_cast<uintptr_t>(VirtualAlloc(nullptr,0x2400000,MEM_COMMIT|MEM_RESERVE,PAGE_EXECUTE_READWRITE));
    check(testBase!=0,"isolated memory allocated");
    stub(0x3852b0,language);stub(0x390a40,record);stub(0x39a4c0,record);stub(0x3abe10,gear);stub(0x3abbf0,equipment);
    encoded(0x2100000,L"Potion");encoded(0x2100040,L"Restores HP");set<uint16_t>(0x2101008,0x40);
    strcpy_s(reinterpret_cast<char*>(address(0x8eb690)),256,"Save");strcpy_s(reinterpret_cast<char*>(address(0x8eb250)),256,"Load");
    strcpy_s(reinterpret_cast<char*>(address(0x8eb890)),64,"Yes");strcpy_s(reinterpret_cast<char*>(address(0x8eba90)),64,"No");
    installFieldMenus();installSystemMenus();
    const std::string mode=argc>1?argv[1]:"all";
    if(mode=="all" || mode=="empty") emptyItems();
    if(mode=="all" || mode=="slots") slots();
    if(mode=="all" || mode=="yes-no") yesNo();
    if(mode=="all" || mode=="index") indexing();
    std::printf("PASS: menu speech (%s), %u checks; synthetic fixtures only\n",mode.c_str(),checks);
    VirtualFree(reinterpret_cast<void*>(testBase),0,MEM_RELEASE);return 0;
}
