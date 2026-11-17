#include "mod_state.hpp"
#include "runtime.hpp"
#include <cwchar>

namespace ffx {
void toggleMod() {
    const bool enabled=!modEnabled.load();
    modEnabled.store(enabled);
    invalidate("mod-toggle");
    modLifetimeEpoch=generation();modToggleSerial.fetch_add(1);
    interruptSpeechForInput();
    output(enabled?L"Screen reader mod on":L"Screen reader mod off",true,"mod-toggle");
}
namespace {
bool held=false,wasDown=false;
LRESULT CALLBACK keyboardEvent(int code,WPARAM message,LPARAM data) {
    if(code==HC_ACTION) {
        const auto& key=*reinterpret_cast<const KBDLLHOOKSTRUCT*>(data);
        if(key.vkCode==VK_F12) {
            DWORD process=0;GetWindowThreadProcessId(GetForegroundWindow(),&process);
            const bool down=message==WM_KEYDOWN || message==WM_SYSKEYDOWN;
            if(modToggleKey(key.vkCode,down,(GetAsyncKeyState(VK_CONTROL)&0x8000)!=0,
                (GetAsyncKeyState(VK_SHIFT)&0x8000)!=0,process==GetCurrentProcessId())) return 1;
        }
    }
    return CallNextHookEx(nullptr,code,message,data);
}
DWORD WINAPI toggleThread(void*) {
    // Independent of game addresses and message hooks, including in FFX-2.
    HMODULE module=nullptr;
    GetModuleHandleExW(GET_MODULE_HANDLE_EX_FLAG_FROM_ADDRESS|GET_MODULE_HANDLE_EX_FLAG_UNCHANGED_REFCOUNT,
        reinterpret_cast<LPCWSTR>(&keyboardEvent),&module);
    const auto hook=SetWindowsHookExW(WH_KEYBOARD_LL,keyboardEvent,module,0);
    if(!hook) {
        recordLog("mod toggle keyboard hook failed error=%lu",GetLastError());return 1;
    }
    MSG message{};
    while(GetMessageW(&message,nullptr,0,0)>0) {TranslateMessage(&message);DispatchMessageW(&message);}
    return 0; // Process lifetime; toggling never removes this hook.
}
}
bool modToggleKey(unsigned vk,bool down,bool control,bool shift,bool focused) {
    if(vk!=VK_F12) return false;
    if(!down) {const bool captured=held;held=wasDown=false;return captured;}
    if(wasDown) return held;
    wasDown=true;
    if(!focused || !control || !shift) return false;
    held=true;
    if(modToggleOnOwner) modToggleRequests.fetch_add(1);
    else toggleMod();
    return true;
}
void startModToggle() {
    wchar_t path[MAX_PATH]{};GetModuleFileNameW(nullptr,path,MAX_PATH);
    const auto slash=std::wcsrchr(path,L'\\');const auto name=slash?slash+1:path;
    if(_wcsicmp(name,L"FFX.exe") && _wcsicmp(name,L"FFX-2.exe") &&
       _wcsicmp(name,L"FFX2.exe") && _wcsicmp(name,L"FFX&X-2_Will.exe")) return;
    const auto worker=CreateThread(nullptr,0,toggleThread,nullptr,0,nullptr);
    if(worker) CloseHandle(worker);
    else recordLog("mod toggle worker unavailable error=%lu",GetLastError());
}
}
