#include "../native/runtime.hpp"
static HMODULE module;
static INIT_ONCE once = INIT_ONCE_STATIC_INIT;
static DWORD WINAPI bootstrap(void*) {
    try {if(!ffx::startSpeech(module,L"FFX_Launcher_SR.log")) ffx::cancelSpeechStartup();}
    catch(...) {ffx::cancelSpeechStartup();}
    return 0;
}
static BOOL CALLBACK initialize(PINIT_ONCE,PVOID,PVOID*) {
    HMODULE pinned=nullptr;
    if(!GetModuleHandleExW(GET_MODULE_HANDLE_EX_FLAG_FROM_ADDRESS|GET_MODULE_HANDLE_EX_FLAG_PIN,
        reinterpret_cast<LPCWSTR>(&bootstrap),&pinned)) return FALSE;
    module=pinned;ffx::prepareSpeechStartup();
    const auto worker=CreateThread(nullptr,0,bootstrap,nullptr,0,nullptr);
    if(!worker) {ffx::cancelSpeechStartup();return FALSE;}
    CloseHandle(worker);return TRUE;
}
extern "C" int __cdecl LauncherSpeech_Start() {
    return InitOnceExecuteOnce(&once, initialize, nullptr, nullptr) ? 1 : 0;
}
extern "C" void __cdecl LauncherSpeech_Capture(int paused) { ffx::captureSpeech(paused!=0); }
BOOL APIENTRY DllMain(HMODULE handle, DWORD reason, LPVOID) {
    if (reason == DLL_PROCESS_ATTACH) {
        module = handle;
        DisableThreadLibraryCalls(handle);
    }
    return TRUE;
}
