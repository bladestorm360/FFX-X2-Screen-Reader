#include "prism_backend.hpp"
#include <string>
namespace ffx {
namespace {
constexpr PrismBackendId backendIds[]={0,PRISM_BACKEND_NVDA,PRISM_BACKEND_JAWS,PRISM_BACKEND_SAPI,
    PRISM_BACKEND_ONE_CORE,PRISM_BACKEND_UIA,PRISM_BACKEND_SYSTEM_ACCESS,PRISM_BACKEND_WINDOW_EYES,
    PRISM_BACKEND_PC_TALKER,PRISM_BACKEND_SENSE_READER,PRISM_BACKEND_ZDSR,PRISM_BACKEND_ZOOM_TEXT,
    PRISM_BACKEND_BOY_PC_READER,PRISM_BACKEND_ORCA,PRISM_BACKEND_SPEECH_DISPATCHER,PRISM_BACKEND_SPIEL};
bool canOutput(uint64_t flags) { return (flags&(PRISM_BACKEND_SUPPORTS_OUTPUT|PRISM_BACKEND_SUPPORTS_SPEAK|PRISM_BACKEND_SUPPORTS_BRAILLE))!=0; }
std::wstring wide(const char* value) {
    if(!value) return {};
    const auto length=MultiByteToWideChar(CP_UTF8,MB_ERR_INVALID_CHARS,value,-1,nullptr,0);
    if(length<=1) return {};
    std::wstring result(length,L'\0');
    if(!MultiByteToWideChar(CP_UTF8,MB_ERR_INVALID_CHARS,value,-1,result.data(),length)) return {};
    result.pop_back();return result;
}
}
PrismSpeechBackend::~PrismSpeechBackend() { close(); }
void PrismSpeechBackend::close() {
    if(backend && backend_free) backend_free(backend);
    backend=nullptr;features=0;currentName.clear();
    if(context && shutdown) shutdown(context);
    context=nullptr;
    if(module) FreeLibrary(module);
    module=nullptr;
}
bool PrismSpeechBackend::open(const std::wstring& directory,int preference) {
    close();
    module=LoadLibraryExW((directory+L"PrismRuntime\\prism.dll").c_str(),nullptr,
        LOAD_LIBRARY_SEARCH_DLL_LOAD_DIR|LOAD_LIBRARY_SEARCH_SYSTEM32);
    if(!module) return false;
#define LOAD(name) name=reinterpret_cast<decltype(name)>(GetProcAddress(module,"prism_" #name)); if(!name) { close();return false; }
    LOAD(init);LOAD(shutdown);LOAD(version);LOAD(registry_create);LOAD(registry_create_best);
    LOAD(backend_initialize);LOAD(backend_free);LOAD(backend_name);LOAD(backend_get_features);
    LOAD(backend_output);LOAD(backend_speak);LOAD(backend_braille);LOAD(backend_stop);
#undef LOAD
    if(version()!=0x1203) { close();return false; }
    context=init(nullptr); // null config: no availability callback/polling thread
    if(!context) { close();return false; }
    if(choose(preference) || (preference && choose(0))) return true;
    close();return false;
}
bool PrismSpeechBackend::choose(int preference) {
    if(!context || preference<0 || preference>=static_cast<int>(_countof(backendIds))) return false;
    // create_best already initializes; an explicit new instance requires initialize.
    auto candidate=preference?registry_create(context,backendIds[preference]):registry_create_best(context);
    if(!candidate) return false;
    if(preference && backend_initialize(candidate)!=PRISM_OK) { backend_free(candidate);return false; }
    const auto flags=backend_get_features(candidate);
    if(!canOutput(flags)) { backend_free(candidate);return false; }
    auto copied=wide(backend_name(candidate));
    if(backend) { (void)stop();backend_free(backend); }
    backend=candidate;features=flags;currentName=std::move(copied);return true;
}
std::vector<SpeechBackendChoice> PrismSpeechBackend::choices() {
    std::vector<SpeechBackendChoice> result{{0,L"Automatic"}};
    if(!context) return result;
    for(int i=1;i<static_cast<int>(_countof(backendIds));++i) {
        auto candidate=registry_create(context,backendIds[i]);
        if(!candidate) continue;
        if(backend_initialize(candidate)==PRISM_OK && canOutput(backend_get_features(candidate))) {
            const auto name=wide(backend_name(candidate));
            if(!name.empty()) result.push_back({i,name});
        }
        backend_free(candidate);
    }
    return result;
}
bool PrismSpeechBackend::output(const std::wstring& text,bool interrupt,bool brailleOnly) {
    if(!backend || text.empty()) return false;
    const auto size=WideCharToMultiByte(CP_UTF8,WC_ERR_INVALID_CHARS,text.c_str(),-1,nullptr,0,nullptr,nullptr);
    if(size<=0) return false;
    std::string utf8(size,'\0');
    if(!WideCharToMultiByte(CP_UTF8,WC_ERR_INVALID_CHARS,text.c_str(),-1,utf8.data(),size,nullptr,nullptr)) return false;
    if(brailleOnly) return (features&PRISM_BACKEND_SUPPORTS_BRAILLE) && backend_braille(backend,utf8.c_str())==PRISM_OK;
    if(features&PRISM_BACKEND_SUPPORTS_OUTPUT) return backend_output(backend,utf8.c_str(),interrupt)==PRISM_OK;
    bool success=false;
    if(features&PRISM_BACKEND_SUPPORTS_SPEAK) success=backend_speak(backend,utf8.c_str(),interrupt)==PRISM_OK;
    if(features&PRISM_BACKEND_SUPPORTS_BRAILLE) {
        const bool accepted=backend_braille(backend,utf8.c_str())==PRISM_OK;
        success=(features&PRISM_BACKEND_SUPPORTS_SPEAK)?success && accepted:accepted;
    }
    return success;
}
bool PrismSpeechBackend::stop() { return backend && (features&PRISM_BACKEND_SUPPORTS_STOP) && backend_stop(backend)==PRISM_OK; }
}
