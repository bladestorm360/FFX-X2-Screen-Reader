/*
 * dinput8_proxy.cpp — Minimal dinput8.dll proxy for FFX/FFX-2 HD Remaster.
 *
 * Forwards DirectInput8Create to the real dinput8.dll and loads
 * frida-gadget.dll which runs the mod script (ffx_sr.js).
 */

#define WIN32_LEAN_AND_MEAN
#include <windows.h>

static HMODULE g_real_dinput8 = nullptr;
static FARPROC g_proc_DirectInput8Create = nullptr;

static void load_real_dinput8() {
    wchar_t sys_dir[MAX_PATH];
    GetSystemDirectoryW(sys_dir, MAX_PATH);
    wcscat_s(sys_dir, L"\\dinput8.dll");
    g_real_dinput8 = LoadLibraryW(sys_dir);
    if (g_real_dinput8)
        g_proc_DirectInput8Create = GetProcAddress(g_real_dinput8, "DirectInput8Create");
}

extern "C" __declspec(naked) void __stdcall proxy_DirectInput8Create() {
    __asm { jmp dword ptr [g_proc_DirectInput8Create] }
}

BOOL APIENTRY DllMain(HMODULE hModule, DWORD reason, LPVOID lpReserved) {
    if (reason == DLL_PROCESS_ATTACH) {
        DisableThreadLibraryCalls(hModule);
        load_real_dinput8();

        // Load Frida Gadget — it reads its .config file and runs ffx_sr.js
        wchar_t gadget_path[MAX_PATH];
        GetModuleFileNameW(hModule, gadget_path, MAX_PATH);
        wchar_t* sep = wcsrchr(gadget_path, L'\\');
        if (sep) wcscpy_s(sep + 1, MAX_PATH - (sep - gadget_path + 1), L"frida-gadget.dll");
        LoadLibraryW(gadget_path);
    }
    else if (reason == DLL_PROCESS_DETACH) {
        if (g_real_dinput8) FreeLibrary(g_real_dinput8);
    }
    return TRUE;
}
