#include "../runtime.hpp"
#include "../mod_state.hpp"
#include <cstdio>
#include <cstdlib>

namespace {unsigned resets=0,stops=0,outputs=0;std::wstring last;}
namespace ffx {
void invalidate(const char*) {++resets;}
unsigned generation() {return resets+1;}
void interruptSpeechForInput() {++stops;}
bool output(const wchar_t* text,bool,const char*) {++outputs;last=text;return true;}
}
static void check(bool ok,const char* reason) {if(!ok) {std::fprintf(stderr,"FAIL: %s\n",reason);std::exit(1);}}
int main() {
    using namespace ffx;
    check(modEnabled,"starts enabled without saved preference");
    check(!modToggleKey(VK_F12,true,false,false,true),"plain NVDA F12 passes through");
    check(!modToggleKey(VK_F12,true,true,true,true) && modEnabled,"adding modifiers to a held F12 cannot synthesize a toggle");
    check(!modToggleKey(VK_F12,false,true,true,true),"never swallow an unowned release");
    check(!modToggleKey(VK_F12,true,true,true,false),"other foreground applications keep the chord");
    modToggleKey(VK_F12,false,true,true,false);
    check(modToggleKey(VK_F12,true,true,true,true) && !modEnabled,"shared proxy chord disables without game addresses");
    check(last==L"Screen reader mod off" && stops==1 && resets==1,"disable cancels speech and invalidates history before feedback");
    check(modToggleKey(VK_F12,true,true,true,true) && outputs==1,"held chord never repeats");
    check(modToggleKey(VK_F12,false,false,false,false),"paired chord release drains even after focus/modifier loss");
    check(modToggleKey(VK_F12,true,true,true,true) && modEnabled && last==L"Screen reader mod on","same chord enables from off");
    modToggleKey(VK_F12,false,true,true,true);
    modToggleOnOwner=[](unsigned requests) {if(!requests) return;check(requests==1,"one owner request");toggleMod();};
    const auto before=outputs;
    modToggleKey(VK_F12,true,true,true,true);
    check(modEnabled && outputs==before,"keyboard worker never resets native state");
    serviceModToggle();
    check(!modEnabled && outputs==before+1,"input owner performs queued toggle");
    serviceModToggle();check(outputs==before+1,"request consumed once");
    std::puts("PASS: shared proxy toggle chord, default on, off/on feedback, no repeat, focus/key-up pairing and native owner handoff.");
}
