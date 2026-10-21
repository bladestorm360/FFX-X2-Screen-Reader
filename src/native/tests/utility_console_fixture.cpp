// Actual console transport and feedback with copied tokens and fake Win32 UI.
// No fixtureGame, desktop window, registry, speech SDK or real console thread is used.
#include "../utilities.hpp"
#include <array>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <new>
thread_local bool observingNative=false;
unsigned nativeNew=0,nativeDelete=0;
void* operator new(size_t n) {if(observingNative) ++nativeNew;if(auto p=std::malloc(n?n:1)) return p;throw std::bad_alloc{};}
void* operator new[](size_t n) {return ::operator new(n);}
void operator delete(void* p) noexcept {if(p && observingNative) ++nativeDelete;std::free(p);}
void operator delete[](void* p) noexcept {::operator delete(p);}
void operator delete(void* p,size_t) noexcept {::operator delete(p);}
void operator delete[](void* p,size_t) noexcept {::operator delete(p);}
namespace {
HWND fixtureGame=reinterpret_cast<HWND>(0x100),console=reinterpret_cast<HWND>(0x200),external=reinterpret_cast<HWND>(0x300),foreground=fixtureGame;
ffx::UtilityContext fixtureContext{1,1,fixtureGame};
bool visible=false,registrationFails=false,creationFails=false,threadFails=false,changeOnFocus=false,changeOnRefresh=false;
unsigned focusCalls=0,created=0,threads=0,destroyed=0,submitted=0,helpCalls=0,spoken=0,postedCount=0;
const wchar_t* command=L"where";
WNDPROC registered=nullptr;
struct Posted {HWND hwnd;UINT message;WPARAM revision;LPARAM epoch;};
std::array<Posted,128> posted{};
std::array<std::array<wchar_t,512>,64> said{};
void check(bool ok,const char* why) {if(!ok) {std::fprintf(stderr,"FAIL: %s\n",why);std::exit(1);}}
HWND WINAPI fg() {return foreground;}
HWND WINAPI ancestor(HWND h,UINT) {return h;}
BOOL WINAPI isWindow(HWND h) {return h!=nullptr;}
BOOL WINAPI activate(HWND h) {++focusCalls;foreground=h;if(changeOnFocus) {++fixtureContext.revision;changeOnFocus=false;}return TRUE;}
BOOL WINAPI show(HWND,int mode) {visible=mode!=SW_HIDE;return TRUE;}
HWND WINAPI focus(HWND h) {return h;}
BOOL WINAPI post(HWND h,UINT m,WPARAM w,LPARAM l) {check(postedCount<posted.size(),"bounded fake message recorder");posted[postedCount++]={h,m,w,l};return TRUE;}
int WINAPI getText(HWND,LPWSTR out,int n) {const auto len=(std::min)(std::wcslen(command),static_cast<size_t>(n-1));std::wmemcpy(out,command,len);out[len]=0;return static_cast<int>(len);}
BOOL WINAPI setText(HWND,LPCWSTR) {if(changeOnRefresh) {++fixtureContext.revision;changeOnRefresh=false;}return TRUE;}
LRESULT WINAPI send(HWND,UINT,WPARAM,LPARAM) {return 0;}
SHORT WINAPI keyState(int) {return 0;}
ATOM WINAPI registerClass(const WNDCLASSW* cls) {check(!observingNative,"UI registration belongs to dispatcher");registered=cls->lpfnWndProc;return registrationFails?0:1;}
DWORD WINAPI lastError() {return ERROR_ACCESS_DENIED;}
LRESULT CALLBACK oldProc(HWND,UINT,WPARAM,LPARAM) {return 0;}
LONG_PTR WINAPI subclass(HWND,int,LONG_PTR) {return reinterpret_cast<LONG_PTR>(&oldProc);}
HWND WINAPI create(DWORD,LPCWSTR,LPCWSTR,DWORD style,int,int,int,int,HWND,HMENU,HINSTANCE,LPVOID) {
    check(!observingNative,"window/control creation belongs to UI owner");
    if(style&WS_CHILD) return console;
    ++created;if(creationFails) return nullptr;
    check(registered && registered(console,WM_CREATE,0,0)==0,"WM_CREATE finishes controls before ready publication");return console;
}
BOOL WINAPI destroy(HWND) {++destroyed;return TRUE;}
BOOL WINAPI getMessage(LPMSG,HWND,UINT,UINT) {return 0;}
HANDLE WINAPI createThread(LPSECURITY_ATTRIBUTES,SIZE_T,LPTHREAD_START_ROUTINE,LPVOID,DWORD,LPDWORD) {++threads;return threadFails?nullptr:reinterpret_cast<HANDLE>(0x500);}
BOOL WINAPI closeHandle(HANDLE) {return TRUE;}
HANDLE WINAPI createEvent(LPSECURITY_ATTRIBUTES,BOOL manual,BOOL,LPCWSTR) {check(!manual,"dispatcher uses auto-reset wake without self retry loop");return reinterpret_cast<HANDLE>(0x400);}
BOOL WINAPI eventSet(HANDLE) {return TRUE;}
DWORD WINAPI wait(HANDLE,DWORD) {check(!observingNative,"Native never waits for dispatcher");return WAIT_OBJECT_0;}
BOOL WINAPI modulePin(DWORD,LPCWSTR,HMODULE* module) {*module=reinterpret_cast<HMODULE>(0x600);return TRUE;}
}
namespace ffx {
UtilityContext utilityContext() {return fixtureContext;}
bool utilityContextValid(const UtilityContext& t,bool focusRequired) {return t.revision && t.revision==fixtureContext.revision && t.epoch==fixtureContext.epoch && t.window==fixtureContext.window && (!focusRequired || foreground==fixtureGame);}
bool queueUtilityContext(const std::wstring&,const UtilityContext& t) {check(utilityContextValid(t),"submission copied valid token");++submitted;return true;}
std::wstring executeUtility(const std::wstring&) {check(!observingNative,"Native never formats/executes console help");++helpCalls;return L"Help";}
bool output(const wchar_t* text,bool,const char*) {check(spoken<said.size() && std::wcslen(text)<512,"bounded fake ready speech recorder");wcscpy_s(said[spoken++].data(),512,text);return true;}
void log(const char*,...) {}
}
#define GetForegroundWindow fg
#define GetAncestor ancestor
#define IsWindow isWindow
#define SetForegroundWindow activate
#define ShowWindow show
#define SetFocus focus
#define PostMessageW post
#define GetWindowTextW getText
#define SetWindowTextW setText
#define SendMessageW send
#define GetKeyState keyState
#define RegisterClassW registerClass
#define GetLastError lastError
#define CreateWindowExW create
#define DestroyWindow destroy
#define GetMessageW getMessage
#define CreateThread createThread
#define CloseHandle closeHandle
#define CreateEventW createEvent
#define SetEvent eventSet
#define WaitForSingleObject wait
#define GetModuleHandleExW modulePin
#undef SetWindowLongPtrW
#define SetWindowLongPtrW subclass
#include "../utility_console.cpp"
#include "../utility_feedback.cpp"
int main() {
    using namespace ffx;
    startUtilityConsole();check(threads==1 && !created && !spoken,"bootstrap starts only dispatcher without UI/help/focus");
    historyStorage.prepare();HWND hwnd=nullptr;
    observingNative=true;consoleReplyReady(L"Equal response",14);consoleReplyReady(L"Equal response",14);observingNative=false;
    serviceWake(hwnd);
    check(!hwnd && !created && !helpCalls && !spoken && history()==L"Equal response\r\n\r\nEqual response\r\n\r\n","reply-only wake preserves equal history without UI or unsolicited help");
    observingNative=true;openUtilityConsole();observingNative=false;
    check(!created && !spoken,"native request never creates UI or help");serviceWake(hwnd);
    check(hwnd==console && visible && foreground==console && helpCalls==1 && spoken==2 && sameContext(sessionContext,fixtureContext),"first valid request creates controls, records help, shows/focuses then announces");
    check(!std::wcscmp(said[0].data(),L"Help"),"first-open help ordering retained");
    submit(console);check(submitted==1 && !visible && foreground==fixtureGame,"valid submission returns input to captured fixtureGame");
    observingNative=true;openUtilityConsole();observingNative=false;++fixtureContext.revision;const auto priorFocus=focusCalls;
    serviceWake(hwnd);check(!visible && focusCalls==priorFocus,"stale queued show cannot activate console");
    foreground=fixtureGame;observingNative=true;openUtilityConsole();observingNative=false;foreground=external;
    serviceWake(hwnd);check(!visible && foreground==external && focusCalls==priorFocus,"show never steals external foreground");
    foreground=fixtureGame;observingNative=true;openUtilityConsole();observingNative=false;changeOnRefresh=true;
    serviceWake(hwnd);check(!visible && foreground==fixtureGame,"reentrant refresh context change invalidates focus handoff");
    foreground=fixtureGame;observingNative=true;openUtilityConsole();observingNative=false;changeOnFocus=true;
    serviceWake(hwnd);check(!visible && foreground==fixtureGame && !sessionContext.revision,"reentrant activation context change hides and restores captured fixtureGame");
    foreground=fixtureGame;observingNative=true;openUtilityConsole();observingNative=false;serviceWake(hwnd);
    const auto old=sessionContext;++fixtureContext.revision;observingNative=true;invalidateUtilityConsole();observingNative=false;
    check(postedCount && posted[postedCount-1].message==HideConsole && posted[postedCount-1].revision==old.revision,"Native invalidation posts bounded displayed token without lock");
    consoleProc(console,HideConsole,old.revision,old.epoch);check(!visible && foreground==fixtureGame,"fixtureContext invalid session hides");
    observingNative=true;openUtilityConsole();observingNative=false;serviceWake(hwnd);
    consoleProc(console,HideConsole,old.revision,old.epoch);check(visible && sameContext(sessionContext,fixtureContext),"old hide cannot close a reopened session");
    foreground=external;const auto returning=focusCalls;hideConsole(console);check(!visible && foreground==external && focusCalls==returning,"return never steals external focus");
    foreground=fixtureGame;observingNative=true;openUtilityConsole();observingNative=false;serviceWake(hwnd);
    ++fixtureContext.epoch;const auto submittedBefore=submitted;submit(console);check(submitted==submittedBefore && !visible && foreground==fixtureGame,"stale submission cancels and returns captured input");
    const auto historyBefore=history();finishedWindow(hwnd);hwnd=nullptr;
    registrationFails=true;observingNative=true;openUtilityConsole();observingNative=false;serviceWake(hwnd);
    check(!hwnd && history().find(historyBefore)==0 && threads==1,"creation failure retains history and single bootstrap dispatcher");
    registrationFails=false;creationFails=true;observingNative=true;openUtilityConsole();observingNative=false;serviceWake(hwnd);check(!hwnd,"window failure leaves new-request retry available");
    creationFails=false;observingNative=true;openUtilityConsole();observingNative=false;serviceWake(hwnd);check(hwnd==console && visible,"later explicit request retries creation successfully");
    hideConsole(hwnd);finishedWindow(hwnd);hwnd=nullptr;
    std::wstring longReply(50000,L'x');longReply.back()=L'z';
    observingNative=true;consoleReplyReady(longReply.data(),longReply.size());observingNative=false;serviceWake(hwnd);
    check(history().size()==48000 && history()[47995]==L'z' && history().substr(47996)==L"\r\n\r\n","bounded tail is exactly previous 48000-character history trimming");
    const auto helpBefore=helpCalls;observingNative=true;
    for(unsigned i=0;i<9;++i) consoleReplyReady(L"same",4);
    observingNative=false;serviceWake(hwnd);check(!hwnd && helpCalls==helpBefore,"reply pressure is logged without recursive help or UI creation");
    check(!nativeNew && !nativeDelete && threads==1,"actual Native open/invalidation/reply uses no C++ owners or thread creation");
    std::puts("PASS: actual prestarted console/ready-feedback transports; reply-only silence, equal/history tail, UI controls/focus/reentrant/stale guards, retry/history lifetime, Native new/delete=0/0. Win32/UI loop boundaries mocked.");
}
