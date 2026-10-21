#pragma once
#include "speech_backends.hpp"
#include "prism_headers/prism.h"
namespace ffx {
class PrismSpeechBackend {
    HMODULE module=nullptr;
    PrismContext* context=nullptr;
    PrismBackend* backend=nullptr;
    uint64_t features=0;
    std::wstring currentName;
#define PRISM_FUNCTION(name) decltype(&prism_##name) name=nullptr
    PRISM_FUNCTION(init); PRISM_FUNCTION(shutdown); PRISM_FUNCTION(version);
    PRISM_FUNCTION(registry_create); PRISM_FUNCTION(registry_create_best);
    PRISM_FUNCTION(backend_initialize); PRISM_FUNCTION(backend_free);
    PRISM_FUNCTION(backend_name); PRISM_FUNCTION(backend_get_features);
    PRISM_FUNCTION(backend_output); PRISM_FUNCTION(backend_speak);
    PRISM_FUNCTION(backend_braille); PRISM_FUNCTION(backend_stop);
#undef PRISM_FUNCTION
public:
    ~PrismSpeechBackend();
    bool open(const std::wstring& directory,int preference=0);
    bool choose(int preference);
    bool output(const std::wstring&,bool interrupt,bool brailleOnly=false);
    bool stop();
    std::vector<SpeechBackendChoice> choices();
    std::wstring name() const { return currentName; }
    uint64_t capabilities() const { return features; }
    void close();
};
}
