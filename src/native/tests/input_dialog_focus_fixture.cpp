// Production focus-policy regression using only synthetic HWND/PID mappings.
// settings.cpp is compiled normally; no window, process, game, or writer exists.
#include "../settings.hpp"
#include "native_heap_counter.hpp"
#include <cstdio>
#include <cstdlib>

namespace {
HWND gameRoot=reinterpret_cast<HWND>(0x100),dialogRoot=reinterpret_cast<HWND>(0x200),
    dialogChild=reinterpret_cast<HWND>(0x201),foreignWindow=reinterpret_cast<HWND>(0x300);
HWND foreground=gameRoot;
constexpr DWORD ownPid=77,foreignPid=88;
unsigned windowQueries=0,pidQueries=0;
void check(bool value,const char* message){if(!value){std::fprintf(stderr,"FAIL: %s\n",message);std::exit(1);}}
HWND WINAPI fakeForeground(){++windowQueries;return foreground;}
HWND WINAPI fakeAncestor(HWND window,UINT){if(window==dialogChild)return dialogRoot;return window;}
DWORD WINAPI fakeWindowThreadProcessId(HWND window,LPDWORD pid){++pidQueries;*pid=window==foreignWindow?foreignPid:ownPid;return 1;}
DWORD WINAPI fakeCurrentProcessId(){return ownPid;}
}

#define GetForegroundWindow fakeForeground
#define GetAncestor fakeAncestor
#define GetWindowThreadProcessId fakeWindowThreadProcessId
#define GetCurrentProcessId fakeCurrentProcessId
#include "../settings.cpp"

namespace ffx { void recordLog(const char*,...){} }

int main(){
    using namespace ffx;
    setInputDialogWindow(nullptr);
    foreground=gameRoot;
    check(gameFocused()&&!inputDialogActive(),"same-process game root is focused when no dialog is registered");
    foreground=foreignWindow;
    check(!gameFocused(),"foreground window in a foreign process is rejected");

    setInputDialogWindow(dialogRoot);
    foreground=dialogRoot;
    check(inputDialogActive()&&!gameFocused(),"registered same-process dialog root is active and excluded from game focus");
    foreground=dialogChild;
    check(GetAncestor(GetForegroundWindow(),GA_ROOT)==dialogRoot&&inputDialogActive()&&!gameFocused(),"dialog child resolves to registered root and is excluded");

    foreground=gameRoot;
    check(!inputDialogActive()&&gameFocused(),"hidden dialog does not block game focus when game root is foreground");
    setInputDialogWindow(nullptr);
    foreground=dialogRoot;
    check(!inputDialogActive()&&gameFocused(),"clearing dialog registration restores ordinary same-process focus behavior");

    nativeHeapFixture::allocations=0;nativeHeapFixture::frees=0;nativeHeapFixture::observing=true;
    volatile unsigned focusedCount=0;
    for(unsigned i=0;i<1000;++i)if(gameFocused())++focusedCount;
    nativeHeapFixture::observing=false;
    check(focusedCount==1000&&nativeHeapFixture::allocations==0&&nativeHeapFixture::frees==0,
        "1000 production focus queries perform no C++ heap allocation");
    check(windowQueries>=1000&&pidQueries>=1000,"production process identity and foreground APIs were exercised");
    std::puts("PASS: production gameFocused excludes a registered same-process dialog root and its child; foreign-process rejection, hidden/cleared behavior, and 1000 allocation-free queries. HWND, process identity, and foreground are mocked.");
}
