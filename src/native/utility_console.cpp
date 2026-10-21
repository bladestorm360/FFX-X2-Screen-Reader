#include "utilities.hpp"
#include "utility_context.hpp"
#include "nonblocking_snapshots.hpp"
#include "process_storage.hpp"
#include <atomic>
#include <cstring>
#ifdef FFX_DEV_UTILITIES
namespace ffx {
namespace {
constexpr UINT ConsumeRequests=WM_APP+1,ConsumeReplies=WM_APP+2,HideConsole=WM_APP+3;
constexpr int CommandId=100,RunId=101,HelpId=102,ResultsId=103;
struct Request {UtilityContext token;};
struct Reply {unsigned length=0;wchar_t text[47996]{};};
async::TryCaptureQueue<Request,8> requests;
async::TryCaptureQueue<Reply,8> replies;
async::AtomicSnapshot<UtilityContext> displayed;
std::atomic<unsigned> lostReplies{0};
std::atomic<HWND> readyWindow{nullptr};
HANDLE wakeEvent=nullptr; // Published on bootstrap, process lifetime thereafter.
ProcessStorage<std::wstring> historyStorage;
// UI owner alone accesses these controls/session/string data.
HWND commandEdit=nullptr,resultEdit=nullptr;
UtilityContext sessionContext;
WNDPROC originalEdit=nullptr;
bool returnAfterEnter=false;
std::wstring& history() {return historyStorage.get();}
HWND foregroundRoot() {return GetAncestor(GetForegroundWindow(),GA_ROOT);}
bool sameContext(const UtilityContext& a,const UtilityContext& b) {
    return a.revision==b.revision && a.epoch==b.epoch && a.window==b.window;
}
bool messageContext(const UtilityContext& token,WPARAM revision,LPARAM epoch) {
    return token.revision==static_cast<unsigned>(revision) && token.epoch==static_cast<unsigned>(epoch);
}
void hideConsole(HWND hwnd) {
    const auto game=utilityContext().window;
    if(foregroundRoot()==hwnd && game && game==sessionContext.window && IsWindow(game)) SetForegroundWindow(game);
    returnAfterEnter=false;ShowWindow(hwnd,SW_HIDE);sessionContext={};displayed.publish({});
}
void refresh() {
    if(!resultEdit) return;
    SetWindowTextW(resultEdit,history().c_str());
    SendMessageW(resultEdit,EM_SETSEL,history().size(),history().size());
    SendMessageW(resultEdit,EM_SCROLLCARET,0,0);
}
void consumeReplies() {
    Reply reply;
    while(replies.tryPop(reply)) {
        if(reply.length>_countof(reply.text)) continue;
        history().append(reply.text,reply.length);history()+=L"\r\n\r\n";
        if(history().size()>48000) history().erase(0,history().size()-48000);
    }
    const auto lost=lostReplies.exchange(0,std::memory_order_acq_rel);
    if(lost) recordLog("utility console history pressure count=%u",lost);
    refresh();
}
void submit(HWND hwnd) {
    wchar_t text[512]{};GetWindowTextW(commandEdit,text,_countof(text));if(!*text) return;
    const auto token=utilityContext();
    if(foregroundRoot()!=hwnd || !sameContext(token,sessionContext) || !utilityContextValid(token)) {
        hideConsole(hwnd);utilityReply(L"Command cancelled because the game context changed.");return;
    }
    history()+=L"> "+std::wstring(text)+L"\r\n";SetWindowTextW(commandEdit,L"");
    const bool queued=queueUtilityContext(text,token);consumeReplies();
    if(queued) {returnAfterEnter=(GetKeyState(VK_RETURN)&0x8000)!=0;if(!returnAfterEnter) hideConsole(hwnd);}
}
LRESULT CALLBACK editProc(HWND hwnd,UINT message,WPARAM wp,LPARAM lp) {
    if(message==WM_KEYDOWN && wp==VK_RETURN) {submit(GetParent(hwnd));return 0;}
    if(message==WM_KEYUP && wp==VK_RETURN && returnAfterEnter) {hideConsole(GetParent(hwnd));return 0;}
    if(message==WM_KEYDOWN && wp==VK_ESCAPE) {hideConsole(GetParent(hwnd));return 0;}
    return CallWindowProcW(originalEdit,hwnd,message,wp,lp);
}
void consumeRequests(HWND& hwnd);
LRESULT consoleProcImpl(HWND hwnd,UINT message,WPARAM wp,LPARAM lp) {
    switch(message) {
    case WM_CREATE: {
        const auto instance=GetModuleHandleW(nullptr);
        const auto control=[&](const wchar_t* type,const wchar_t* text,DWORD style,int id,int x,int y,int w,int h) {
            const auto child=CreateWindowExW(!std::wcscmp(type,L"EDIT")?WS_EX_CLIENTEDGE:0,type,text,
                WS_CHILD|WS_VISIBLE|style,x,y,w,h,hwnd,reinterpret_cast<HMENU>(static_cast<intptr_t>(id)),instance,nullptr);
            if(child) SendMessageW(child,WM_SETFONT,reinterpret_cast<WPARAM>(GetStockObject(DEFAULT_GUI_FONT)),TRUE);
            return child;
        };
        control(L"STATIC",L"&Command",0,104,12,12,100,22);
        commandEdit=control(L"EDIT",L"",WS_TABSTOP|ES_AUTOHSCROLL,CommandId,12,36,600,26);
        if(commandEdit) SendMessageW(commandEdit,EM_SETLIMITTEXT,500,0);
        originalEdit=commandEdit?reinterpret_cast<WNDPROC>(SetWindowLongPtrW(commandEdit,GWLP_WNDPROC,reinterpret_cast<LONG_PTR>(editProc))):nullptr;
        control(L"BUTTON",L"&Run command",WS_TABSTOP|BS_DEFPUSHBUTTON,RunId,12,72,140,30);
        control(L"BUTTON",L"&Help",WS_TABSTOP,HelpId,164,72,100,30);
        control(L"BUTTON",L"Return to &game",WS_TABSTOP,IDCANCEL,276,72,140,30);
        control(L"STATIC",L"&Results (Tab to read previous replies)",0,105,12,114,600,22);
        resultEdit=control(L"EDIT",L"",WS_TABSTOP|ES_MULTILINE|ES_READONLY|ES_AUTOVSCROLL|WS_VSCROLL,ResultsId,12,140,600,280);
        return commandEdit && resultEdit && originalEdit?0:-1;
    }
    case WM_COMMAND:
        switch(LOWORD(wp)) {
        case RunId:case IDOK:submit(hwnd);return 0;
        case HelpId:utilityReply(executeUtility(L"help"));return 0;
        case IDCANCEL:hideConsole(hwnd);return 0;
        }
        break;
    case WM_CLOSE:hideConsole(hwnd);return 0;
    case WM_SIZE: {
        const int w=LOWORD(lp),h=HIWORD(lp);
        if(commandEdit) MoveWindow(commandEdit,12,36,w-24,26,TRUE);
        if(resultEdit) MoveWindow(resultEdit,12,140,w-24,h-152,TRUE);
        return 0;
    }
    case WM_KEYDOWN:if(wp==VK_ESCAPE) {hideConsole(hwnd);return 0;}break;
    case ConsumeRequests:consumeRequests(hwnd);return 0;
    case ConsumeReplies:consumeReplies();return 0;
    case HideConsole:
        if(messageContext(sessionContext,wp,lp) && !utilityContextValid(sessionContext)) hideConsole(hwnd);
        return 0;
    }
    return DefWindowProcW(hwnd,message,wp,lp);
}
LRESULT CALLBACK consoleProc(HWND hwnd,UINT message,WPARAM wp,LPARAM lp) {
    try {return consoleProcImpl(hwnd,message,wp,lp);}
    catch(...) {recordLog("utility console message failed message=%x",message);hideConsole(hwnd);return message==WM_CREATE?-1:0;}
}
bool createConsole(HWND& hwnd) {
    const auto instance=GetModuleHandleW(nullptr);
    WNDCLASSW cls{};cls.lpfnWndProc=consoleProc;cls.hInstance=instance;cls.lpszClassName=L"FFXScreenReaderUtilities";
    cls.hCursor=LoadCursorW(nullptr,MAKEINTRESOURCEW(32512));cls.hbrBackground=reinterpret_cast<HBRUSH>(COLOR_WINDOW+1);
    if(!RegisterClassW(&cls) && GetLastError()!=ERROR_CLASS_ALREADY_EXISTS) {
        recordLog("utility console registration failed error=%lu",GetLastError());return false;
    }
    hwnd=CreateWindowExW(WS_EX_CONTROLPARENT,cls.lpszClassName,L"FFX accessibility utilities",WS_OVERLAPPEDWINDOW,
        CW_USEDEFAULT,CW_USEDEFAULT,650,490,nullptr,nullptr,instance,nullptr);
    if(!hwnd) {recordLog("utility console creation failed error=%lu",GetLastError());return false;}
    // WM_CREATE and control/subclass initialization have already completed.
    readyWindow.store(hwnd,std::memory_order_release);
    utilityReply(executeUtility(L"help"));consumeReplies();return true;
}
void consumeRequests(HWND& hwnd) {
    Request request;
    while(requests.tryPop(request)) {
        const auto token=request.token;
        if(!utilityContextValid(token,true)) continue;
        if(!hwnd && !createConsole(hwnd)) {utilityReply(L"Could not open the utility console.");continue;}
        refresh();ShowWindow(hwnd,SW_SHOWNOACTIVATE);
        if(!utilityContextValid(token,true)) {hideConsole(hwnd);continue;}
        sessionContext=token;SetForegroundWindow(hwnd);
        if(!utilityContextValid(token) || foregroundRoot()!=hwnd) {hideConsole(hwnd);continue;}
        displayed.publish(token);SetFocus(commandEdit);
        output(L"FFX utilities. Enter a command, or type help. Escape returns to the game.",true,"utility-console");
    }
}
void finishedWindow(HWND hwnd) {
    readyWindow.store(nullptr,std::memory_order_release);displayed.publish({});
    if(hwnd && IsWindow(hwnd)) DestroyWindow(hwnd);
    commandEdit=resultEdit=nullptr;originalEdit=nullptr;returnAfterEnter=false;sessionContext={};
}
void serviceWake(HWND& hwnd) {consumeReplies();consumeRequests(hwnd);consumeReplies();}
DWORD WINAPI consoleThread(void*) {
    historyStorage.prepare();
    HWND hwnd=nullptr;
    for(;;) {
        WaitForSingleObject(wakeEvent,INFINITE);
        try {
            // Reply-only wakes update retained history without creating a UI.
            serviceWake(hwnd);
            if(!hwnd) continue;
            MSG message{};bool escaping=false;
            while(GetMessageW(&message,nullptr,0,0)>0) {
                // This also recovers accepted packets after a PostMessage quota
                // failure while other UI messages continue to be dispatched.
                serviceWake(hwnd);
                if(message.message==WM_KEYUP && message.wParam==VK_RETURN && returnAfterEnter) {hideConsole(hwnd);continue;}
                if(message.message==WM_KEYDOWN && message.wParam==VK_ESCAPE) {escaping=true;continue;}
                if(message.message==WM_KEYUP && message.wParam==VK_ESCAPE && escaping) {escaping=false;hideConsole(hwnd);continue;}
                if(!IsDialogMessageW(hwnd,&message)) {TranslateMessage(&message);DispatchMessageW(&message);}
            }
        } catch(...) {recordLog("utility console UI cycle failed");}
        finishedWindow(hwnd);hwnd=nullptr;
        // History and queued packets remain owned across UI creation retries.
    }
}
void wake(UINT message) {
    if(wakeEvent) SetEvent(wakeEvent);
    if(const auto hwnd=readyWindow.load(std::memory_order_acquire)) PostMessageW(hwnd,message,0,0);
}
}
void startUtilityConsole() {
    if(wakeEvent) return;
    HMODULE pin=nullptr;
    if(!GetModuleHandleExW(GET_MODULE_HANDLE_EX_FLAG_FROM_ADDRESS|GET_MODULE_HANDLE_EX_FLAG_PIN,
        reinterpret_cast<LPCWSTR>(&consoleThread),&pin)) return;
    const auto event=CreateEventW(nullptr,FALSE,FALSE,nullptr);if(!event) return;
    wakeEvent=event;const auto thread=CreateThread(nullptr,0,consoleThread,nullptr,0,nullptr);
    if(thread) CloseHandle(thread);else {wakeEvent=nullptr;CloseHandle(event);recordLog("utility console dispatcher unavailable error=%lu",GetLastError());}
}
bool utilityConsoleActive() {
    const auto hwnd=readyWindow.load(std::memory_order_acquire);return hwnd && foregroundRoot()==hwnd;
}
void openUtilityConsole() {
    const auto token=utilityContext();if(!utilityContextValid(token,true)) return;
    if(!wakeEvent) {utilityReply(L"Could not open the utility console.");return;}
    if(!requests.tryPush({token})) {utilityReply(L"Utility console request queue is full.");return;}
    wake(ConsumeRequests);
}
void invalidateUtilityConsole() {
    UtilityContext token;
    if(!displayed.tryRead(token) || !token.revision) return;
    if(const auto hwnd=readyWindow.load(std::memory_order_acquire)) PostMessageW(hwnd,HideConsole,token.revision,token.epoch);
}
void consoleReplyReady(const wchar_t* text,size_t length) {
    if(!text) return;
    Reply reply;reply.length=static_cast<unsigned>((std::min)(length,size_t{47996}));
    std::memcpy(reply.text,text+length-reply.length,reply.length*sizeof(wchar_t));
    if(!replies.tryPush(reply)) lostReplies.fetch_add(1,std::memory_order_relaxed);
    wake(ConsumeReplies);
}
void consoleReply(const std::wstring& text) {consoleReplyReady(text.data(),text.size());}
}
#else
namespace ffx {
bool utilityConsoleActive() {return false;}
void openUtilityConsole() {}
void invalidateUtilityConsole() {}
void consoleReply(const std::wstring&) {}
void startUtilityConsole() {}
void consoleReplyReady(const wchar_t*,size_t) {}
}
#endif
