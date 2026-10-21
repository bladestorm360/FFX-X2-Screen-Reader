#define WIN32_LEAN_AND_MEAN
#define DIRECTINPUT_VERSION 0x0800
#include <windows.h>
#include <dinput.h>
#include <cstdio>
int main() {
    const auto proxy = LoadLibraryW(L"dinput8.dll");
    if (!proxy) { fprintf(stderr, "proxy load failed: %lu\n", GetLastError()); return 1; }
    using Create = HRESULT(WINAPI*)(HINSTANCE, DWORD, REFIID, LPVOID*, LPUNKNOWN);
    const auto create = reinterpret_cast<Create>(GetProcAddress(proxy, "DirectInput8Create"));
    const auto features = reinterpret_cast<unsigned(__cdecl*)()>(GetProcAddress(proxy, "FfxNative_Features"));
    const auto ready = reinterpret_cast<int(__cdecl*)()>(GetProcAddress(proxy, "FfxNative_SpeechReady"));
    if (!create || !features || !ready || features() || ready()) return 1;
    IDirectInput8W* input = nullptr;
    const auto result = create(GetModuleHandleW(nullptr), DIRECTINPUT_VERSION, IID_IDirectInput8W, reinterpret_cast<void**>(&input), nullptr);
    const auto deadline=GetTickCount64()+10000;
    while(!ready() && GetTickCount64()<deadline) Sleep(1); // isolated fixture only
    if (FAILED(result) || !input || features() || !ready()) {
        fprintf(stderr, "forwarding/bootstrap failed: hr=%lx features=%u ready=%d\n", result, features(), ready()); return 1;
    }
    input->Release();
    if (GetModuleHandleW(L"frida-gadget.dll") != nullptr) return 1;
    puts("Proxy fixture passed: exact DirectInput ABI/forwarding, deferred bootstrap, unsupported executable guarded, no Gadget loaded.");
    return 0;
}
