// Isolated fake-Win32 tests for the production Sphere Grid number-entry dialog.
// No native game reads, desktop windows, worker threads, or game process are used.
#include "../events.hpp"
#include "../settings.hpp"
#include "../mod_menu.hpp"
#include "../status_review.hpp"
#include <array>
#include <algorithm>
#include <cstdio>
#include <cstdlib>
#include <cwchar>

namespace ffx { extern bool nativeOwner; extern unsigned fixtureEpoch; }
namespace {
HWND game=reinterpret_cast<HWND>(0x100),dialog=reinterpret_cast<HWND>(0x200),fakeEdit=reinterpret_cast<HWND>(0x201),
    goButton=reinterpret_cast<HWND>(0x202),cancelButton=reinterpret_cast<HWND>(0x203),external=reinterpret_cast<HWND>(0x300);
HWND fg=game,currentFocus=nullptr;
bool returnHeld=false,failRegister=false,failCreate=false,failEdit=false;
bool visible=false;
unsigned windowCreates=0,childCreates=0,focusCalls=0,foregroundCalls=0,wakes=0,threads=0,copyCalls=0,outputs=0;
WNDPROC registered=nullptr;
std::array<wchar_t,16> editText{};
std::array<std::wstring,64> speech{};
void check(bool ok,const char* why) {if(!ok){std::fprintf(stderr,"FAIL: %s\n",why);std::exit(1);}}
HWND WINAPI getForeground(){return fg;}
HWND WINAPI ancestor(HWND h,UINT){return h;}
HWND WINAPI getFocus(){return currentFocus;}
HWND WINAPI setFocus(HWND h){++focusCalls;currentFocus=h;return h;}
BOOL WINAPI setForeground(HWND h){++foregroundCalls;fg=h;return TRUE;}
SHORT WINAPI getKeyState(int key){return key==VK_RETURN&&returnHeld?static_cast<SHORT>(0x8000):0;}
BOOL WINAPI show(HWND,int mode){visible=mode!=SW_HIDE;return TRUE;}
BOOL WINAPI isWindow(HWND h){return h==game||h==dialog;}
BOOL WINAPI post(HWND,UINT,WPARAM,LPARAM){return TRUE;}
ATOM WINAPI registerClass(const WNDCLASSW* cls){check(!ffx::nativeGameThread(),"class registration stays off Native owner");registered=cls->lpfnWndProc;return failRegister?0:1;}
DWORD WINAPI lastError(){return ERROR_ACCESS_DENIED;}
LRESULT WINAPI send(HWND h,UINT message,WPARAM,LPARAM){if(h==fakeEdit&&message==EM_SETLIMITTEXT) return 0;return 0;}
int WINAPI getText(HWND h,LPWSTR out,int cap){check(h==fakeEdit,"submission reads the edit control");size_t n=(std::min)(std::wcslen(editText.data()),static_cast<size_t>(cap-1));std::wmemcpy(out,editText.data(),n);out[n]=0;return static_cast<int>(n);}
BOOL WINAPI setText(HWND h,LPCWSTR text){check(h==fakeEdit,"dialog clears its edit control");wcsncpy_s(editText.data(),editText.size(),text,_TRUNCATE);return TRUE;}
HWND WINAPI create(DWORD,LPCWSTR kind,LPCWSTR text,DWORD style,int,int,int,int,HWND parent,HMENU,HINSTANCE,LPVOID){
    check(!ffx::nativeGameThread(),"dialog/control creation stays off Native owner");
    if(style&WS_CHILD){++childCreates;if(wcscmp(kind,L"EDIT")==0){check(wcscmp(text,L"")==0,"number input starts empty");return failEdit?nullptr:fakeEdit;}if(wcscmp(kind,L"BUTTON")==0){check(wcscmp(text,L"&Go to node")==0||wcscmp(text,L"&Cancel")==0,"Go and Cancel buttons have explicit accessible labels");return wcscmp(text,L"&Go to node")==0?goButton:cancelButton;}if(wcscmp(kind,L"STATIC")==0)check(wcscmp(text,L"&Node number")==0,"edit has a descriptive label");return reinterpret_cast<HWND>(0x204);}
    ++windowCreates;check(wcscmp(kind,L"FFXSphereNodeEntry")==0&&wcscmp(text,L"Sphere Grid: go to node")==0,"dialog class and title are labelled");if(failCreate)return nullptr;
    check(parent==nullptr&&registered,"standard top-level dialog registered before create");check(registered(dialog,WM_CREATE,0,0)==0,"dialog creates labelled controls before use");return dialog;
}
BOOL WINAPI setEvent(HANDLE){++wakes;return TRUE;}
HANDLE WINAPI createEvent(LPSECURITY_ATTRIBUTES,BOOL manual,BOOL,LPCWSTR){check(!manual,"wake event is auto-reset");return reinterpret_cast<HANDLE>(0x400);}
HANDLE WINAPI createThread(LPSECURITY_ATTRIBUTES,SIZE_T,LPTHREAD_START_ROUTINE,LPVOID,DWORD,LPDWORD){++threads;return reinterpret_cast<HANDLE>(0x500);}
BOOL WINAPI closeHandle(HANDLE){return TRUE;}
BOOL WINAPI modulePin(DWORD,LPCWSTR,HMODULE* out){*out=reinterpret_cast<HMODULE>(0x600);return TRUE;}
HMODULE WINAPI fakeGetModule(HANDLE){return reinterpret_cast<HMODULE>(0x600);}
HCURSOR WINAPI cursor(HINSTANCE,LPCWSTR){return reinterpret_cast<HCURSOR>(0x700);}
HGDIOBJ WINAPI stock(int){return reinterpret_cast<HGDIOBJ>(0x800);}
LRESULT WINAPI defProc(HWND,UINT,WPARAM,LPARAM){return 0;}
BOOL WINAPI isDialogMessage(HWND,LPMSG){return FALSE;}
void setNumber(const wchar_t* value){wcsncpy_s(editText.data(),editText.size(),value,_TRUNCATE);}
}

namespace ffx {
bool nativeGameThread(){return nativeOwner;}
bool gameFocused(){return fg==game;}
void setInputDialogWindow(HWND value){check(!nativeOwner && value==dialog,"UI owner registers its actual input window before game focus changes");}
bool sphereGridOpen(){return true;}
bool modMenuActive(){return false;}
bool statusReviewAvailable(){return false;}
void hotkeyChordWhen(unsigned,bool,void(*)(bool),bool(*)(),bool){}
bool hook(unsigned,Handler,Handler){return true;}
uintptr_t address(unsigned){return 0;}
bool copy(uintptr_t,void*,size_t){++copyCalls;return false;}
unsigned generation(){return fixtureEpoch;}
bool deferNativeAction(NativeAction){return true;}
bool output(const wchar_t* text,bool,const char*){++outputs;if(text&&speech.size()>outputs-1)speech[outputs-1]=text;return true;}
bool output(const std::wstring& text,bool interrupt,const char* producer){return output(text.c_str(),interrupt,producer);}
void recordLog(const char*,...){ }
bool nativeOwner=true;unsigned fixtureEpoch=3;
}

#define GetForegroundWindow getForeground
#define GetAncestor ancestor
#define GetFocus getFocus
#define SetFocus setFocus
#define SetForegroundWindow setForeground
#define GetKeyState getKeyState
#define ShowWindow show
#define IsWindow isWindow
#define PostMessageW post
#define GetWindowTextW getText
#define SetWindowTextW setText
#define SendMessageW send
#define CreateWindowExW create
#define RegisterClassW registerClass
#define GetLastError lastError
#define CreateEventW createEvent
#define CreateThread createThread
#define CloseHandle closeHandle
#define GetModuleHandleExW modulePin
#define GetModuleHandleW fakeGetModule
#define LoadCursorW cursor
#define GetStockObject stock
#define DefWindowProcW defProc
#define IsDialogMessageW isDialogMessage
#define SetEvent setEvent
#define MsgWaitForMultipleObjects(...) WAIT_OBJECT_0
#define PeekMessageW(...) FALSE
#define TranslateMessage(...) TRUE
#define DispatchMessageW(...) 0
#define GetCurrentThreadId() 1
#include "../sphere_jump.cpp"

namespace {
void publish(unsigned revision=7,unsigned epoch=3){ffx::Context c;c.revision=revision;c.epoch=epoch;c.mode=0x645010;c.grid=0x111100;c.game=game;c.count=300;c.character=2;ffx::published.publish(c);}
void openNative(){check(ffx::nativeGameThread(),"fixture owner set Native for request");ffx::openSphereJump(false);}
void consume(){check(!ffx::nativeGameThread(),"fixture owner set UI for dispatcher");HWND hwnd=ffx::window.load();ffx::consumeOpens(hwnd);}
void clearCommands(){ffx::Request r;while(ffx::commands.tryPop(r)){} }
ffx::Request takeCommand(){ffx::Request r{};check(ffx::commands.tryPop(r),"one native request queued");return r;}
}

int main(){
    using namespace ffx;
    // Startup follows the normal event/hotkey bootstrap but the thread stub never executes.
    nativeOwner=true;installSphereJump();check(threads==1&&windowCreates==0,"bootstrap starts bounded UI dispatcher without creating a window");
    publish();fg=game;nativeOwner=true;openNative();check(windowCreates==0&&copyCalls==0&&wakes==1,"Native open queues POD and performs no game dereference or UI creation");
    nativeOwner=false;consume();check(windowCreates==1&&childCreates==4,"UI consumer creates the labelled EDIT and two buttons plus label");
    check(fg==dialog&&currentFocus==fakeEdit&&!speech[0].empty()&&speech[0].find(L"Node number")!=std::wstring::npos,"successful open foregrounds dialog, focuses edit and announces instructions");
    const unsigned readsBefore=copyCalls;setNumber(L"42");dialogProc(dialog,WM_COMMAND,IDOK,0);auto req=takeCommand();
    check(req.context.revision==7&&req.context.epoch==3&&req.context.grid==0x111100&&req.context.mode==0x645010&&req.context.game==game&&req.context.count==300&&req.context.character==2&&req.number==42,"Go submits exact copied context and one-based number");
    check(copyCalls==readsBefore&&fg==game,"UI submission never dereferences game memory and hide returns owned foreground");

    // Reopen and cover invalid numbers without closing the dialog.
    nativeOwner=true;openNative();nativeOwner=false;consume();
    for(const wchar_t* invalid:{L"",L"0",L"301",L"12x",L"12345"}){setNumber(invalid);dialogProc(dialog,WM_COMMAND,IDOK,0);check(visible&&fg==dialog&&!speech[outputs-1].empty(),"invalid or empty value keeps dialog open and requests correction");}
    setNumber(L"8");returnHeld=true;dialogProc(dialog,WM_COMMAND,IDOK,0);Request empty{};check(!commands.tryPop(empty),"Enter held submission waits for release");
    check(pendingNumber==8&&returnOnRelease,"Enter submission records bounded pending number");releaseEntry(dialog);req=takeCommand();check(req.number==8,"Enter release queues the pending number");

    nativeOwner=true;openNative();nativeOwner=false;consume();setNumber(L"9");returnHeld=true;dialogProc(dialog,WM_COMMAND,IDCANCEL,0);check(pendingNumber==0&&returnOnRelease,"Cancel while Return held clears pending submission until release");const unsigned beforeCancel=commands.tryPop(req)?1:0;check(beforeCancel==0,"Cancel before release queues nothing");releaseEntry(dialog);check(!commands.tryPop(req),"release after Cancel still queues nothing");
    nativeOwner=true;openNative();nativeOwner=false;consume();setNumber(L"10");dialogProc(dialog,WM_COMMAND,IDOK,0);check(pendingNumber==10,"Return submission remains pending until key release");dialogProc(dialog,WM_CLOSE,0,0);check(!commands.tryPop(req)&&!pendingNumber,"hiding while Return is held cancels without queueing");returnHeld=false;

    nativeOwner=true;openNative();nativeOwner=false;consume();const auto old=session;publish(8,3);dialogProc(dialog,CloseStale,0,0);check(!visible&&fg==game,"stale revision closes dialog and restores owned game focus");
    fixtureEpoch=4;publish(8,4);fg=game;nativeOwner=true;openNative();nativeOwner=false;consume();check(visible,"new generation opens with its copied token");fixtureEpoch=5;dialogProc(dialog,CloseStale,0,0);check(!visible&&fg==game,"stale generation closes dialog and restores owned game focus");fixtureEpoch=3;
    publish(9,3);fg=game;nativeOwner=true;openNative();nativeOwner=false;consume();fg=external;setNumber(L"4");dialogProc(dialog,WM_COMMAND,IDOK,0);check(!commands.tryPop(req)&&fg==external,"focus theft cancels without queuing or stealing focus");
    (void)old;

    // Saturate open and selection queues and ensure the existing items survive.
    publish(10,3);fg=game;nativeOwner=true;for(int i=0;i<4;++i)check(opens.tryPush({Context{10,3,0x111100,0x645010,game,300,2},0}),"fill bounded open queue");const auto queued=wakes;openSphereJump(false);check(wakes==queued&&outputs&&speech[outputs-1].find(L"unavailable")!=std::wstring::npos,"saturated open queue reports busy and does not overwrite");Request open{};for(int i=0;i<4;++i)check(opens.tryPop(open)&&open.context.revision==10,"open queue retained original entries");
    nativeOwner=true;openNative();nativeOwner=false;consume();clearCommands();for(int i=0;i<4;++i)check(commands.tryPush({Context{10,3,0x111100,0x645010,game,300,2},i+1}),"fill bounded command queue");setNumber(L"21");dialogProc(dialog,WM_COMMAND,IDOK,0);check(commands.tryPop(req)&&req.number==1,"queue saturation did not overwrite oldest request");
    check(speech[outputs-1].find(L"busy")!=std::wstring::npos&&fg==dialog,"saturated command submission reports busy and leaves entry available");clearCommands();

    // Creation failures leave requests retryable; hide restores focus only while dialog owns foreground.
    resetSphereJump();publish(11,3);window.store(nullptr);failCreate=true;fg=game;nativeOwner=true;openNative();nativeOwner=false;consume();check(windowCreates>=2&&window.load()==nullptr,"failed window creation attempted and left retry available");failCreate=false;nativeOwner=true;openNative();nativeOwner=false;consume();check(fg==dialog&&currentFocus==fakeEdit,"later explicit request retries dialog creation");
    fg=external;const auto focusBefore=focusCalls;hide(dialog);check(fg==external&&focusCalls==focusBefore,"hide does not return focus when another window is foreground");
    std::puts("PASS: fake-Win32 Sphere Jump UI lifecycle, labels, focus, submission, validation, Return-release cancellation, stale/focus guards, queue saturation and creation retry. Game memory, window manager and dispatcher execution are simulated.");
}
