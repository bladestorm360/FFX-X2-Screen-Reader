#include "sphere_jump.hpp"
#include "sphere_grid.hpp"
#include "events.hpp"
#include "mod_menu.hpp"
#include "status_review.hpp"
#include "settings.hpp"
#include "nonblocking_snapshots.hpp"
#include <cwchar>

namespace ffx {
namespace {
constexpr UINT CloseStale=WM_APP+1;
struct Context {unsigned revision=0,epoch=0;uintptr_t grid=0,mode=0;HWND game=nullptr;int count=0,character=-1;};
struct Request {Context context;int number=0;};
async::AtomicSnapshot<Context> published;
async::TryCaptureQueue<Request,4> opens,commands;
std::atomic<HWND> window{nullptr};
HANDLE wakeEvent=nullptr;
// Native owner owns current. UI owner owns session, edit and returnOnRelease.
Context current;unsigned serial=0;
Context session;HWND edit=nullptr;bool returnOnRelease=false;int pendingNumber=0;
HWND foreground() {return GetAncestor(GetForegroundWindow(),GA_ROOT);}
bool same(const Context& a,const Context& b) {return a.revision && a.revision==b.revision && a.epoch==b.epoch && a.epoch==generation();}
bool valid(const Context& token) {Context latest;return published.tryRead(latest) && latest.mode && same(token,latest);}
uintptr_t gridMode(uintptr_t grid) {
    const auto callback=read<uintptr_t>(grid+0x115a8);
    for(const auto rva:{0x645010u,0x644ef0u,0x6452d0u}) if(callback==address(rva)) return callback;
    return 0;
}
void captureContext(Event&) {
    if(!nativeGameThread()) return;
    Context next;next.epoch=generation();next.game=current.game;
    const auto grid=read<uintptr_t>(address(0x1f05834)),pool=read<uintptr_t>(address(0x1686108));
    const bool focused=gameFocused();
    if(focused) next.game=foreground();
    const bool gridOpen=sphereGridOpen() && !statusReviewAvailable();
    next.grid=gridOpen?grid:0;
    if(gridOpen && !modMenuActive() && grid && pool &&
        read<int16_t>(pool+0x2700,-1)<0 && !read<uintptr_t>(grid+0x115b0) &&
        (focused || (window.load() && foreground()==window.load()))) {
        next.grid=grid;next.count=read<int16_t>(grid+2);next.character=read<uint8_t>(grid+0x115bc,255);
        if(next.count>0 && next.count<=1024 && next.character<7) next.mode=gridMode(grid);
    }
    if(next.epoch!=current.epoch || next.grid!=current.grid || next.mode!=current.mode || next.count!=current.count ||
        next.character!=current.character || next.game!=current.game) next.revision=++serial;
    else next.revision=current.revision;
    const bool changed=next.revision!=current.revision;
    current=next;published.publish(current);
    if(changed) if(const auto hwnd=window.load()) PostMessageW(hwnd,CloseStale,0,0);
}
bool eligible(const Context& token,int number) {
    if(!valid(token) || !gameFocused() || number<1 || number>token.count) return false;
    const auto grid=read<uintptr_t>(address(0x1f05834)),pool=read<uintptr_t>(address(0x1686108));
    if(!sphereGridOpen() || modMenuActive() || statusReviewAvailable() || grid!=token.grid || !pool ||
        read<int16_t>(pool+0x2700,-1)>=0 || read<uintptr_t>(grid+0x115b0) || gridMode(grid)!=token.mode ||
        read<int16_t>(grid+2)!=token.count || read<uint8_t>(grid+0x115bc,255)!=token.character) return false;
    const auto node=grid+0x808+(number-1)*0x28;
    if(read<int16_t>(node+6,-1)<0) return false;
    // Exactly A45000's native Move/Use target predicate. Browse permits locks;
    // Move/Use retain their native reachable/activation flags and confirmations.
    return token.mode==address(0x645010) || (read<uint8_t>(node+0x22)&1)!=0;
}
void consumeCommands(Event&) {
    if(!nativeGameThread() || !gameFocused()) return;
    Request request;if(!commands.tryPop(request)) return;
    NativeAction action;action.epoch=request.context.epoch;
    action.prepare=[request] {return eligible(request.context,request.number);};
    action.invoke=[number=request.number] {
        using Select=void(__cdecl*)(int,float);
        reinterpret_cast<Select>(address(0x648d20))(number-1,0.5f);
    };
    action.complete=[](bool success) {if(!success) output(L"Node selection cancelled. The grid changed or that node is unavailable in the current mode.",true,"sphere-jump");};
    if(!deferNativeAction(action)) output(L"Node selection is busy. Try F6 again.",true,"sphere-jump");
}
void hide(HWND hwnd) {
    const bool owned=foreground()==hwnd;
    ShowWindow(hwnd,SW_HIDE);returnOnRelease=false;pendingNumber=0;
    if(owned && session.game && IsWindow(session.game)) SetForegroundWindow(session.game);
    session={};
}
bool parse(const wchar_t* text,int count,int& number) {
    number=0;if(!text || !*text) return false;
    for(unsigned i=0;text[i];++i) {if(i>=4 || text[i]<L'0' || text[i]>L'9') return false;number=number*10+text[i]-L'0';}
    return number>=1 && number<=count;
}
void submit(HWND hwnd) {
    if(!valid(session) || foreground()!=hwnd) {hide(hwnd);return;}
    wchar_t text[16]{};int number=0;GetWindowTextW(edit,text,_countof(text));
    if(!parse(text,session.count,number)) {output(L"Enter a node number from 1 to "+std::to_wstring(session.count)+L".",true,"sphere-jump");SetFocus(edit);return;}
    returnOnRelease=(GetKeyState(VK_RETURN)&0x8000)!=0;
    if(returnOnRelease) {pendingNumber=number;return;}
    if(!commands.tryPush({session,number})) {output(L"Node selection is busy. Try again.",true,"sphere-jump");return;}
    hide(hwnd);
}
void releaseEntry(HWND hwnd) {
    if(pendingNumber && valid(session) && foreground()==hwnd && !commands.tryPush({session,pendingNumber})) {
        returnOnRelease=false;pendingNumber=0;output(L"Node selection is busy. Try again.",true,"sphere-jump");return;
    }
    hide(hwnd);
}
LRESULT dialogMessage(HWND hwnd,UINT message,WPARAM wp,LPARAM lp) {
    switch(message) {
    case WM_CREATE: {
        const auto instance=GetModuleHandleW(nullptr);
        const auto control=[&](const wchar_t* kind,const wchar_t* text,DWORD style,int id,int x,int y,int width,int height) {
            const auto child=CreateWindowExW(0,kind,text,WS_CHILD|WS_VISIBLE|style,x,y,width,height,hwnd,reinterpret_cast<HMENU>(static_cast<intptr_t>(id)),instance,nullptr);
            if(child) SendMessageW(child,WM_SETFONT,reinterpret_cast<WPARAM>(GetStockObject(DEFAULT_GUI_FONT)),TRUE);
            return child;
        };
        control(L"STATIC",L"&Node number",0,100,12,12,260,20);
        edit=control(L"EDIT",L"",WS_TABSTOP|WS_BORDER|ES_AUTOHSCROLL,101,12,36,260,26);
        if(edit) SendMessageW(edit,EM_SETLIMITTEXT,4,0);
        control(L"BUTTON",L"&Go to node",WS_TABSTOP|BS_DEFPUSHBUTTON,IDOK,12,78,124,30);
        control(L"BUTTON",L"&Cancel",WS_TABSTOP,IDCANCEL,148,78,124,30);
        return edit?0:-1;
    }
    case WM_COMMAND:if(LOWORD(wp)==IDOK) {submit(hwnd);return 0;}if(LOWORD(wp)==IDCANCEL) {
        pendingNumber=0;returnOnRelease=(GetKeyState(VK_RETURN)&0x8000)!=0;if(!returnOnRelease) hide(hwnd);return 0;
    }break;
    case WM_CLOSE:hide(hwnd);return 0;
    case CloseStale:if(!valid(session)) hide(hwnd);return 0;
    }
    return DefWindowProcW(hwnd,message,wp,lp);
}
LRESULT CALLBACK dialogProc(HWND hwnd,UINT message,WPARAM wp,LPARAM lp) {
    try {return dialogMessage(hwnd,message,wp,lp);}
    catch(...) {recordLog("sphere number entry message failed=%x",message);hide(hwnd);return message==WM_CREATE?-1:0;}
}
void consumeOpens(HWND& hwnd) {
    Request request;
    for(unsigned n=0;n<4 && opens.tryPop(request);++n) {
        if(!valid(request.context) || foreground()!=request.context.game) continue;
        if(!hwnd) {
            const auto instance=GetModuleHandleW(nullptr);WNDCLASSW cls{};cls.lpfnWndProc=dialogProc;cls.hInstance=instance;
            cls.lpszClassName=L"FFXSphereNodeEntry";cls.hCursor=LoadCursorW(nullptr,MAKEINTRESOURCEW(32512));cls.hbrBackground=reinterpret_cast<HBRUSH>(COLOR_WINDOW+1);
            if(!RegisterClassW(&cls) && GetLastError()!=ERROR_CLASS_ALREADY_EXISTS) {output(L"Could not open node entry.",true,"sphere-jump");continue;}
            hwnd=CreateWindowExW(WS_EX_CONTROLPARENT,cls.lpszClassName,L"Sphere Grid: go to node",WS_OVERLAPPED|WS_CAPTION|WS_SYSMENU,
                CW_USEDEFAULT,CW_USEDEFAULT,306,162,nullptr,nullptr,instance,nullptr);
            if(!hwnd) {output(L"Could not open node entry.",true,"sphere-jump");continue;}
            setInputDialogWindow(hwnd);window.store(hwnd,std::memory_order_release);
        }
        session=request.context;SetWindowTextW(edit,L"");returnOnRelease=false;pendingNumber=0;
        ShowWindow(hwnd,SW_SHOWNOACTIVATE);SetForegroundWindow(hwnd);
        if(!valid(session) || foreground()!=hwnd) {hide(hwnd);continue;}
        SetFocus(edit);output(L"Node number. Enter a number, then Enter. Escape cancels.",true,"sphere-jump");
    }
}
DWORD WINAPI uiThread(void*) {
    HWND hwnd=nullptr;bool escaping=false;
    for(;;) {
        const auto result=MsgWaitForMultipleObjects(1,&wakeEvent,FALSE,INFINITE,QS_ALLINPUT);
        if(result==WAIT_OBJECT_0) try {consumeOpens(hwnd);} catch(...) {recordLog("sphere number entry UI failed");if(hwnd) hide(hwnd);}
        MSG message{};
        while(PeekMessageW(&message,nullptr,0,0,PM_REMOVE)) {
            if(hwnd && message.message==WM_KEYUP && message.wParam==VK_RETURN && returnOnRelease) {
                releaseEntry(hwnd);continue;
            }
            if(hwnd && message.message==WM_KEYDOWN && message.wParam==VK_RETURN && GetFocus()==edit) {if(!returnOnRelease) submit(hwnd);continue;}
            if(hwnd && message.message==WM_KEYDOWN && message.wParam==VK_ESCAPE) {escaping=true;continue;}
            if(hwnd && message.message==WM_KEYUP && message.wParam==VK_ESCAPE && escaping) {escaping=false;hide(hwnd);continue;}
            if(!hwnd || !IsDialogMessageW(hwnd,&message)) {TranslateMessage(&message);DispatchMessageW(&message);}
        }
    }
}
}
bool sphereJumpContext() {Context token;return published.tryRead(token) && token.grid && token.epoch==generation();}
void openSphereJump(bool) {
    Context token;if(!nativeGameThread() || !gameFocused() || !published.tryRead(token) || !token.grid || token.epoch!=generation()) return;
    if(!token.mode) {output(L"Close the current grid menu or wait for the cursor to stop, then press F6.",true,"sphere-jump");return;}
    if(!wakeEvent || !opens.tryPush({token,0})) {output(L"Node entry is unavailable. Try F6 again.",true,"sphere-jump");return;}
    SetEvent(wakeEvent);
}
void resetSphereJump() {if(!nativeGameThread()) return;current={};current.revision=++serial;published.publish(current);if(const auto hwnd=window.load()) PostMessageW(hwnd,CloseStale,0,0);}
void installSphereJump() {
    hook(0x225930,captureContext,consumeCommands);
    hotkeyChordWhen(VK_F6,false,openSphereJump,sphereJumpContext);
#ifndef FFX_READER_TEST
    HMODULE pin=nullptr;
    if(!GetModuleHandleExW(GET_MODULE_HANDLE_EX_FLAG_FROM_ADDRESS|GET_MODULE_HANDLE_EX_FLAG_PIN,reinterpret_cast<LPCWSTR>(&uiThread),&pin)) return;
    wakeEvent=CreateEventW(nullptr,FALSE,FALSE,nullptr);if(!wakeEvent) return;
    if(const auto thread=CreateThread(nullptr,0,uiThread,nullptr,0,nullptr)) CloseHandle(thread);
    else {CloseHandle(wakeEvent);wakeEvent=nullptr;}
#endif
}
#ifdef FFX_READER_TEST
bool sphereJumpForTest(const wchar_t* text) {Context token;int number=0;return published.tryRead(token) && parse(text,token.count,number) && commands.tryPush({token,number});}
#endif
}
