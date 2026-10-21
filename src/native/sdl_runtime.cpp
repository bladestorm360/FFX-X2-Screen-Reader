#include "runtime.hpp"
#include "sdl_runtime.hpp"
#include <mutex>
namespace ffx::sdl {
#define FFX_SDL_DEFINE(name) decltype(&::name) name=nullptr;
FFX_SDL_FUNCTIONS(FFX_SDL_DEFINE)
#undef FFX_SDL_DEFINE
bool load() {
    static std::once_flag once;
    static bool available=false;
    std::call_once(once,[] {
        wchar_t path[MAX_PATH]{};
        const DWORD length=GetModuleFileNameW(nullptr,path,MAX_PATH);
        if(!length || length>=MAX_PATH) return;
        auto slash=wcsrchr(path,L'\\'); if(!slash) return;
        wcscpy_s(slash+1,MAX_PATH-static_cast<size_t>(slash+1-path),L"SDL3.dll");
#if !defined(FFX_READER_TEST) || defined(FFX_SDL_OWNER_TEST)
        // Decline an observed host/other-mod instance rather than steal its owner.
        if(GetModuleHandleW(L"SDL3.dll")) {recordLog("SDL3 already loaded; exclusive mod backend declined");return;}
#endif
        const auto module=LoadLibraryExW(path,nullptr,LOAD_LIBRARY_SEARCH_DLL_LOAD_DIR|LOAD_LIBRARY_SEARCH_SYSTEM32);
        if(!module) { recordLog("SDL3 unavailable error=%lu",GetLastError()); return; }
        bool valid=true;
#define FFX_SDL_BIND(name) name=reinterpret_cast<decltype(name)>(GetProcAddress(module,#name)); valid=valid && name;
        FFX_SDL_FUNCTIONS(FFX_SDL_BIND)
#undef FFX_SDL_BIND
        if(!valid || SDL_GetVersion()!=SDL_VERSIONNUM(3,4,4) || !SDL_IsMainThread()) {
            recordLog("SDL3 unsupported exports/version"); return;
        }
#if !defined(FFX_READER_TEST) || defined(FFX_SDL_OWNER_TEST)
        if(SDL_WasInit(0)) {recordLog("SDL3 already initialized; exclusive mod backend declined");return;}
#endif
        SDL_SetMainReady();
        if(!SDL_IsMainThread()) {recordLog("SDL3 initialization owner changed; backend declined");return;}
        available=true;
        recordLog("SDL3 loaded version=%d x86",SDL_GetVersion());
        // Loaded modules are process lifetime, including while audio calls back.
    });
    return available;
}
}
