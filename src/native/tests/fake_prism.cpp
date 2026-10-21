#define PRISM_BUILDING
#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#include "../prism_headers/prism.h"
#include <mutex>
#include <string>
#include <vector>

struct Record { std::wstring text; DWORD thread; bool interrupt; bool braille; };
struct Silence { DWORD thread; int outputCount; };
struct PrismContext { bool alive=true; };
struct PrismBackend { PrismBackendId id=PRISM_BACKEND_NVDA; bool initialized=false; };
static std::mutex guard;
static std::vector<Record> records;
static std::vector<Silence> silences;
static std::vector<int> lifecycle;
static bool brailleAvailable=true, initFailure=false, nvdaAvailable=true, jawsAvailable=false;
static uint64_t configuredFeatures=0;
static bool featuresConfigured=false;
static uint32_t configuredVersion=0x1203;
static int initCalls=0, shutdownCalls=0, initializeCalls=0, freeCalls=0, outputCalls=0, speakCalls=0, brailleCalls=0, stopCalls=0;
static DWORD lastLifecycleThread=0;
static HANDLE blocked=nullptr,release=nullptr;
static constexpr uint64_t defaultFeatures=PRISM_BACKEND_IS_SUPPORTED_AT_RUNTIME|PRISM_BACKEND_SUPPORTS_SPEAK|PRISM_BACKEND_SUPPORTS_BRAILLE|PRISM_BACKEND_SUPPORTS_OUTPUT|PRISM_BACKEND_SUPPORTS_STOP;
static std::wstring fromUtf8(const char* text) {
 if(!text)return {}; int n=MultiByteToWideChar(CP_UTF8,MB_ERR_INVALID_CHARS,text,-1,nullptr,0); if(n<=1)return {};
 std::wstring v(static_cast<size_t>(n),L'\0'); if(!MultiByteToWideChar(CP_UTF8,MB_ERR_INVALID_CHARS,text,-1,v.data(),n))return {}; v.pop_back(); return v;
}
static bool record(const char* text,bool interrupt,bool braille,bool shouldBlock) {
 auto copied=fromUtf8(text); if(copied.empty())return false;
 if(shouldBlock) { HANDLE started=nullptr,resume=nullptr; {std::lock_guard<std::mutex> l(guard);started=blocked;resume=release;} if(started&&resume){SetEvent(started);WaitForSingleObject(resume,INFINITE);} }
 std::lock_guard<std::mutex> l(guard); if(braille&&!brailleAvailable)return false;
 records.push_back({std::move(copied),GetCurrentThreadId(),interrupt,braille}); return true;
}
extern "C" PRISM_MALLOC PrismContext* __cdecl prism_init(PrismConfig* cfg) { std::lock_guard<std::mutex> l(guard); ++initCalls; lastLifecycleThread=GetCurrentThreadId(); lifecycle.push_back(1); return cfg?nullptr:new PrismContext{}; }
extern "C" __declspec(dllexport) void __cdecl prism_shutdown(PrismContext* c) { std::lock_guard<std::mutex> l(guard); ++shutdownCalls;lastLifecycleThread=GetCurrentThreadId();lifecycle.push_back(2);delete c; }
extern "C" __declspec(dllexport) uint32_t __cdecl prism_version() { return configuredVersion; }
extern "C" PRISM_MALLOC PrismBackend* __cdecl prism_registry_create(PrismContext* c,PrismBackendId id) {
 if(!c||!c->alive)return nullptr; if(id==PRISM_BACKEND_NVDA&&!nvdaAvailable)return nullptr; if(id==PRISM_BACKEND_JAWS&&!jawsAvailable)return nullptr;
 if(id!=PRISM_BACKEND_NVDA&&id!=PRISM_BACKEND_SAPI&&id!=PRISM_BACKEND_JAWS)return nullptr; return new PrismBackend{id,false};
}
extern "C" PRISM_MALLOC PrismBackend* __cdecl prism_registry_create_best(PrismContext* c) { if(!c||!c->alive||!nvdaAvailable)return nullptr;return new PrismBackend{PRISM_BACKEND_NVDA,true}; }
extern "C" __declspec(dllexport) PrismError __cdecl prism_backend_initialize(PrismBackend* b) { std::lock_guard<std::mutex> l(guard);++initializeCalls;lastLifecycleThread=GetCurrentThreadId();lifecycle.push_back(3);if(!b)return PRISM_ERROR_INVALID_PARAM;if(initFailure)return PRISM_ERROR_BACKEND_NOT_AVAILABLE;if(b->initialized)return PRISM_ERROR_ALREADY_INITIALIZED;b->initialized=true;return PRISM_OK; }
extern "C" __declspec(dllexport) void __cdecl prism_backend_free(PrismBackend* b) { std::lock_guard<std::mutex> l(guard);++freeCalls;lastLifecycleThread=GetCurrentThreadId();lifecycle.push_back(4);delete b; }
extern "C" __declspec(dllexport) const char* __cdecl prism_backend_name(PrismBackend* b) { if(!b)return "";return b->id==PRISM_BACKEND_SAPI?"SAPI":b->id==PRISM_BACKEND_JAWS?"JAWS":"NVDA"; }
extern "C" __declspec(dllexport) uint64_t __cdecl prism_backend_get_features(PrismBackend* b) { std::lock_guard<std::mutex> l(guard);return b&&b->initialized?(featuresConfigured?configuredFeatures:defaultFeatures):0; }
extern "C" __declspec(dllexport) PrismError __cdecl prism_backend_output(PrismBackend* b,const char* t,bool i) { {std::lock_guard<std::mutex> l(guard);++outputCalls;}return b&&b->initialized&&record(t,i,false,true)?PRISM_OK:PRISM_ERROR_SPEAK_FAILURE; }
extern "C" __declspec(dllexport) PrismError __cdecl prism_backend_speak(PrismBackend* b,const char* t,bool i) { {std::lock_guard<std::mutex> l(guard);++speakCalls;}return b&&b->initialized&&record(t,i,false,true)?PRISM_OK:PRISM_ERROR_SPEAK_FAILURE; }
extern "C" __declspec(dllexport) PrismError __cdecl prism_backend_braille(PrismBackend* b,const char* t) { {std::lock_guard<std::mutex> l(guard);++brailleCalls;}return b&&b->initialized&&record(t,false,true,false)?PRISM_OK:PRISM_ERROR_NOT_IMPLEMENTED; }
extern "C" __declspec(dllexport) PrismError __cdecl prism_backend_stop(PrismBackend* b) { std::lock_guard<std::mutex> l(guard);++stopCalls;lastLifecycleThread=GetCurrentThreadId();lifecycle.push_back(5);if(!b||!b->initialized)return PRISM_ERROR_NOT_INITIALIZED;silences.push_back({GetCurrentThreadId(),static_cast<int>(records.size())});return PRISM_OK; }
extern "C" __declspec(dllexport) void __cdecl Test_Block(HANDLE a,HANDLE b) { std::lock_guard<std::mutex> l(guard);blocked=a;release=b; }
extern "C" __declspec(dllexport) void __cdecl Test_BrailleAvailable(bool v) { std::lock_guard<std::mutex> l(guard);brailleAvailable=v; }
extern "C" __declspec(dllexport) bool __cdecl Test_Braille(int i) { std::lock_guard<std::mutex> l(guard);return i>=0&&i<(int)records.size()&&records[i].braille; }
extern "C" __declspec(dllexport) bool __cdecl Test_Interrupt(int i) { std::lock_guard<std::mutex> l(guard);return i>=0&&i<(int)records.size()&&records[i].interrupt; }
extern "C" __declspec(dllexport) int __cdecl Test_Count() { std::lock_guard<std::mutex> l(guard);return (int)records.size(); }
extern "C" __declspec(dllexport) DWORD __cdecl Test_Read(int i,wchar_t* t,unsigned n) { std::lock_guard<std::mutex> l(guard);if(i<0||i>=(int)records.size())return 0;wcsncpy_s(t,n,records[i].text.c_str(),_TRUNCATE);return records[i].thread; }
extern "C" __declspec(dllexport) int __cdecl Test_SilenceCount() { std::lock_guard<std::mutex> l(guard);return (int)silences.size(); }
extern "C" __declspec(dllexport) DWORD __cdecl Test_SilenceThread() { std::lock_guard<std::mutex> l(guard);return silences.empty()?0:silences.back().thread; }
extern "C" __declspec(dllexport) int __cdecl Test_SilenceOutputCount() { std::lock_guard<std::mutex> l(guard);return silences.empty()?-1:silences.back().outputCount; }
extern "C" __declspec(dllexport) void __cdecl Test_SetFeatures(uint64_t v) { std::lock_guard<std::mutex> l(guard);configuredFeatures=v;featuresConfigured=true; }
extern "C" __declspec(dllexport) void __cdecl Test_SetInitFailure(bool v) { std::lock_guard<std::mutex> l(guard);initFailure=v; }
extern "C" __declspec(dllexport) void __cdecl Test_SetVersion(uint32_t v) { std::lock_guard<std::mutex> l(guard);configuredVersion=v; }
extern "C" __declspec(dllexport) void __cdecl Test_SetAvailability(bool nvda,bool jaws) { std::lock_guard<std::mutex> l(guard);nvdaAvailable=nvda;jawsAvailable=jaws; }
extern "C" __declspec(dllexport) int __cdecl Test_Counter(int n) { std::lock_guard<std::mutex> l(guard);int a[]={initCalls,shutdownCalls,initializeCalls,freeCalls,outputCalls,speakCalls,brailleCalls,stopCalls};return n>=0&&n<8?a[n]:-1; }
extern "C" __declspec(dllexport) DWORD __cdecl Test_LastLifecycleThread() { std::lock_guard<std::mutex> l(guard);return lastLifecycleThread; }
extern "C" __declspec(dllexport) int __cdecl Test_Lifecycle(int i) { std::lock_guard<std::mutex> l(guard);return i>=0&&i<(int)lifecycle.size()?lifecycle[i]:0; }
extern "C" __declspec(dllexport) int __cdecl Test_LifecycleCount() { std::lock_guard<std::mutex> l(guard);return (int)lifecycle.size(); }




