// Native bootstrap: DirectInput's first export call executes outside loader lock.
#include "../native/events.hpp"
#include <unknwn.h>
#include <exception>

static HMODULE selfModule;
static INIT_ONCE initialized = INIT_ONCE_STATIC_INIT;
using CreateInput = HRESULT(WINAPI*)(HINSTANCE, DWORD, REFIID, LPVOID*, LPUNKNOWN);
static CreateInput realCreate;

static DWORD WINAPI bootstrap(void*) {
    try { if (ffx::startSpeech(selfModule)) ffx::startReader();else ffx::cancelSpeechStartup(); }
    catch(const std::exception& error) { ffx::cancelSpeechStartup();ffx::recordLog("native bootstrap refused: %s",error.what()); }
    catch(...) { ffx::cancelSpeechStartup();ffx::recordLog("native bootstrap refused: unexpected exception"); }
    return 0;
}

static BOOL CALLBACK initialize(PINIT_ONCE, PVOID, PVOID*) {
    wchar_t path[MAX_PATH]{};
    if (!GetSystemDirectoryW(path, MAX_PATH) || wcscat_s(path, L"\\dinput8.dll")) return FALSE;
    const auto real = LoadLibraryW(path);
    if (!real) return FALSE;
    realCreate = reinterpret_cast<CreateInput>(GetProcAddress(real, "DirectInput8Create"));
    if (!realCreate) return FALSE;
    // Keep signature extraction, files, speech/backend initialization and audio
    // away from the native caller. The worker never owns a native COM object.
    HMODULE pinned=nullptr;
    if(GetModuleHandleExW(GET_MODULE_HANDLE_EX_FLAG_FROM_ADDRESS|GET_MODULE_HANDLE_EX_FLAG_PIN,
        reinterpret_cast<LPCWSTR>(&bootstrap),&pinned)) {
        selfModule=pinned;
        const auto worker=CreateThread(nullptr,0,bootstrap,nullptr,0,nullptr);
        if(worker) CloseHandle(worker);
    }
    return TRUE;
}

extern "C" HRESULT WINAPI proxy_DirectInput8Create(HINSTANCE instance, DWORD version,
    REFIID iid, LPVOID* result, LPUNKNOWN outer) {
    if (!InitOnceExecuteOnce(&initialized, initialize, nullptr, nullptr) || !realCreate) return E_FAIL;
    const auto status = realCreate(instance, version, iid, result, outer);
    // Input readers rediscover ready devices on native keyboard/pad updates;
    // a factory object created before worker readiness is never retained.
    return status;
}

BOOL APIENTRY DllMain(HMODULE module, DWORD reason, LPVOID) {
    if (reason == DLL_PROCESS_ATTACH) {
        selfModule = module;
        DisableThreadLibraryCalls(module);
    }
    return TRUE;
}
