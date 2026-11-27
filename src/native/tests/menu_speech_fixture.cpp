// Synthetic memory only: exercise production menu hooks, copied plans and workers.
#include "../events.hpp"
#include "../profile.hpp"
#include "../settings.hpp"
#include "../text_plan.hpp"
#include "../selection_details.hpp"
#include "../copied_controls.hpp"
#include "../navigation.hpp"
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
unsigned testEpoch=1,activeWindows=0;
struct Listener {Handler enter,leave;};
std::map<unsigned,Listener> listeners;
std::vector<std::pair<std::wstring,bool>> said;
unsigned generation() {return testEpoch;}
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
void helpSceneChanged() {}
void pauseNavAudio() {}
void updateNavAudio(Point,bool) {}
Point playerPosition() {return {};}
void refreshMinigameAudio() {}
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
void event(unsigned id,uintptr_t arg,const std::function<void()>& change={},unsigned parent=0) {
    auto listener=listeners.at(id);Event e;e.rva=id;e.args[0]=arg;e.epoch=generation();e.parentRva=parent;
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
unsigned uiLanguage=1;
unsigned __cdecl menuLanguage() {check(owner,"menu locale getter stays on native owner");return uiLanguage;}
unsigned __cdecl activeWindowMask() {return activeWindows;}
uintptr_t __cdecl record(unsigned,uintptr_t* base) {if(base)*base=address(0x2100000);return address(0x2101000);}
uintptr_t __cdecl gear(unsigned) {return address(0x2100000);}
uintptr_t __cdecl equipment(unsigned,uintptr_t*) {return 0;}
void itemFocus(unsigned slot=win,int phase=0) {
    set<int>(slot+0x28,phase);
    event(0x4c6da0,address(slot),[=]{set<int>(slot+0x28,2);});
}
void items(unsigned count=3,unsigned kind=0,unsigned slot=win,int phase=1) {
    std::memset(reinterpret_cast<void*>(address(slot)),0,0x100);
    scene=kind==2?2:3;set<uint16_t>(slot+0x30,static_cast<uint16_t>((count+1)/2));
    set<int8_t>(slot+0x45,0);set<uint8_t>(slot+0x42,1);set<uint8_t>(slot+0x43,static_cast<uint8_t>(kind));
    set<int>(slot+0x94,count);
    for(unsigned i=0;i<4;++i) {
        set<uint16_t>(0x1197730+i*4,0x2001);set<uint8_t>(0x1197732+i*4,3);set<uint8_t>(0x1197733+i*4,1);
    }
    itemFocus(slot,phase);
}
void openOnce() {
    setSetting(Setting::MenuIndex,0);setSetting(Setting::AutoDetail,0);
    event(0x4aa0b0,3,[]{activeWindows=1u<<3;});
    auto n=said.size();items(3,0,win,0);check(said.size()==n+1,"opening Items speaks the first row");
    itemFocus();check(said.size()==n+1,"same opening callback stays silent");
    event(0x4aa0b0,19,[]{activeWindows|=1u<<19;});
    event(0x4aad20,19,[]{activeWindows&=~(1u<<19);});
    itemFocus();check(said.size()==n+1,"an overlay leaves the Items opening intact");
    event(0x4aad20,3,[]{activeWindows=1u<<1;});
    event(0x4aa0b0,3,[]{activeWindows=1u<<3;});
    itemFocus();check(said.size()==n+2,"immediate close and reopen on the same window and row speaks");
    itemFocus();check(said.size()==n+2,"the second opening is read once too");
    event(0x4c6da0,address(win),[]{set<int>(win+0x58,1);});
    event(0x4c6da0,address(win),[]{set<int>(win+0x58,0);});
    check(said.size()==n+4,"moving to and from an equally named row preserves both focus events");
    itemFocus();n=said.size();
    event(0x4c6da0,address(win),[]{set<int>(win+0x28,0x11);});
    itemFocus();check(said.size()==n+1,"returning from accepted item use can read the same row");
    set<int>(win+0x28,0x13);
    event(0x4c6da0,address(win),[]{set<int8_t>(win+0x45,-1);set<int>(win+0x28,0x14);});
    set<int8_t>(win+0x45,0);
    itemFocus();check(said.size()==n+2,"accepted grid cancel allows refocus in the same parent");
    items(3,0,win+0x100,0);check(said.size()==n+3,"replacement window can read an identical row");
    ++testEpoch;
    itemFocus(win+0x100);check(said.size()==n+4,"old resource generation cannot suppress a fresh event");
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
    // The confirm handler opens the loading status before returning. Its old
    // cursor must not announce again or automatically read the new prompt.
    for(const auto handler:{0x2efd90u,0x2f00e0u,0x2f06f0u}) for(const int automatic:{0,1}) {
        setSetting(Setting::AutoDetail,automatic);
        set<int>(0x8e72f0,1);event(0x302fe0,0);event(handler,0);
        const auto n=said.size();
        event(handler,0,[]{event(0x302f70,0);});
        check(said.size()==n+1 && said.back().first==L"Load",
            "confirm return after a status popup emits only the status, never stale Yes or duplicate detail");
        event(handler,0);check(said.size()==n+1,"the next idle handler cannot revive the retired answer");
    }
    setSetting(Setting::AutoDetail,0);
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
    struct LanguageCase {unsigned id;const wchar_t* indexed;};
    const LanguageCase locales[]={
        {0,L"\u7a7a\u304d, 1\u756a\u76ee\uff08\u51681\u4ef6\uff09"}, {1,L"Empty, 1 of 1"},
        {2,L"Vide, 1 sur 1"}, {3,L"Vac\u00edo, 1 de 1"}, {4,L"Leer, 1 von 1"}, {5,L"Vuoto, 1 di 1"},
        {9,L"\u7a7a, \u7b2c1\u9879\uff0c\u51711\u9879"}, {10,L"\ube44\uc5b4 \uc788\uc74c, 1\ubc88\uc9f8 (\uc804\uccb4 1\uac1c)"},
        {255,L"Empty, 1 of 1"}
    };
    setSetting(Setting::MenuIndex,1);
    for(const auto& locale:locales) {
        uiLanguage=locale.id;items(1);
        event(0x4c6da0,address(win),[]{set<uint8_t>(0x1197732,0);});
        expect(locale.indexed,"blank cell with a valid index uses the UI language");
    }
    uiLanguage=1;
}
void availability() {
    setSetting(Setting::AutoDetail,0);setSetting(Setting::MenuIndex,0);setSetting(Setting::UnavailableCue,0);
    items(1);auto n=said.size();
    event(0x486de0,0x80000003,{},0x4c6da0);
    check(said.size()==n,"field rejection cue defaults Off");
    event(0x4c6da0,address(win),[]{set<uint8_t>(0x1197733,0);});
    expect(L"Potion, 3, Unavailable","native field label works independently of cue setting");
    n=said.size();event(0x4c6da0,address(win));
    check(said.size()==n,"unchanged availability stays silent");
    event(0x4c6da0,address(win),[]{set<uint8_t>(0x1197733,1);});
    expect(L"Potion, 3","availability can change on the same row");
    struct Locale {unsigned id;const wchar_t* label;};
    const Locale locales[]={
        {0,L"\u4f7f\u7528\u3067\u304d\u307e\u305b\u3093"},{1,L"Unavailable"},{2,L"Indisponible"},
        {3,L"No disponible"},{4,L"Nicht verf\u00fcgbar"},{5,L"Non disponibile"},
        {9,L"\u65e0\u6cd5\u4f7f\u7528"},{10,L"\uc0ac\uc6a9\ud560 \uc218 \uc5c6\uc74c"},{255,L"Unavailable"}
    };
    setSetting(Setting::UnavailableCue,1);
    for(const auto locale:locales) {
        uiLanguage=locale.id;items(1,2);
        event(0x4c6da0,address(win),[]{set<uint8_t>(0x1197733,0);});
        check(said.back().first==std::wstring(L"Potion, ")+locale.label,"grey Abilities use the game UI locale");
        n=said.size();event(0x486de0,0x80000003,{},0x4c6da0);
        check(said.size()==n+1 && said.back().first==locale.label && said.back().second,
            "active grid rejection speaks a localized short cue");
    }
    uiLanguage=1;setSetting(Setting::MenuIndex,1);items(1);
    event(0x4c6da0,address(win),[]{set<uint8_t>(0x1197733,0);});
    expect(L"Potion, 3, Unavailable, 1 of 1","availability preserves quantity and index");
    event(0x4c6da0,address(win),[]{set<uint8_t>(0x1197732,0);});expect(L"Empty, 1 of 1","blank rows keep their separate label");
    n=said.size();
    event(0x486de0,0x80000001,{},0x4c6da0);
    event(0x486de0,0x80000003,{},0x464180);
    event(0x486de0,0x80000003,{},0);
    scene=0;event(0x486de0,0x80000003,{},0x4c6da0);
    check(said.size()==n,"other sounds, script and unrelated buzzers, and inactive field menus stay silent");
    setSetting(Setting::MenuIndex,0);setSetting(Setting::UnavailableCue,0);
}
}
int main(int argc,char** argv) {
    testBase=reinterpret_cast<uintptr_t>(VirtualAlloc(nullptr,0x2400000,MEM_COMMIT|MEM_RESERVE,PAGE_EXECUTE_READWRITE));
    check(testBase!=0,"isolated memory allocated");
    stub(0x3852b0,language);stub(0x241290,menuLanguage);stub(0x4aa410,activeWindowMask);
    stub(0x390a40,record);stub(0x39a4c0,record);stub(0x3abe10,gear);stub(0x3abbf0,equipment);
    encoded(0x2100000,L"Potion");encoded(0x2100040,L"Restores HP");set<uint16_t>(0x2101008,0x40);
    strcpy_s(reinterpret_cast<char*>(address(0x8eb690)),256,"Save");strcpy_s(reinterpret_cast<char*>(address(0x8eb250)),256,"Load");
    strcpy_s(reinterpret_cast<char*>(address(0x8eb890)),64,"Yes");strcpy_s(reinterpret_cast<char*>(address(0x8eba90)),64,"No");
    installGameContext();installFieldMenus();installSystemMenus();
    const std::string mode=argc>1?argv[1]:"all";
    if(mode=="all" || mode=="open-once") openOnce();
    if(mode=="all" || mode=="empty") emptyItems();
    if(mode=="all" || mode=="slots") slots();
    if(mode=="all" || mode=="yes-no") yesNo();
    if(mode=="all" || mode=="index") indexing();
    if(mode=="all" || mode=="availability") availability();
    std::printf("PASS: menu speech (%s), %u checks; synthetic fixtures only\n",mode.c_str(),checks);
    VirtualFree(reinterpret_cast<void*>(testBase),0,MEM_RELEASE);return 0;
}
