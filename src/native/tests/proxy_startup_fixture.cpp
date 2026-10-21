// Exercises real proxy bootstrap and system forwarding with injected failures.
// This is an isolated test process; no game is launched or attached.
#define WIN32_LEAN_AND_MEAN
#define DIRECTINPUT_VERSION 0x0800
#include "../events.hpp"
#include <dinput.h>
#include <stdexcept>
#include <cstdio>
#include <cstdlib>
#include <atomic>
namespace {
 bool failSpeech=false;std::atomic<unsigned> failures{0},readerCalls{0};
 HANDLE speechStarted=nullptr,releaseSpeech=nullptr,completed=nullptr;
 DWORD owner=0,caller=0;
}
namespace ffx {
bool startSpeech(HMODULE,const wchar_t*) {
    owner=GetCurrentThreadId();SetEvent(speechStarted);
    if(WaitForSingleObject(releaseSpeech,5000)!=WAIT_OBJECT_0) std::abort();
    if(failSpeech) throw std::runtime_error("synthetic speech bootstrap failure");
    return true;
}
void cancelSpeechStartup() {}
unsigned startReader() { ++readerCalls; throw std::out_of_range("synthetic unverified Installed RVA 464180"); }
void startupLog(const LogCapture&) noexcept { ++failures;SetEvent(completed); }
void observeDirectInput(uintptr_t) { std::abort(); }
}
extern "C" unsigned __cdecl FfxNative_Features() { return 0; }
extern "C" HRESULT WINAPI proxy_DirectInput8Create(HINSTANCE,DWORD,REFIID,LPVOID*,LPUNKNOWN);
int main(int argc,char**) {
    failSpeech=argc>1;caller=GetCurrentThreadId();
    speechStarted=CreateEventW(nullptr,TRUE,FALSE,nullptr);
    releaseSpeech=CreateEventW(nullptr,TRUE,FALSE,nullptr);
    completed=CreateEventW(nullptr,TRUE,FALSE,nullptr);
    if(!speechStarted || !releaseSpeech || !completed) return 1;
    ffx::logCaptureHandler.store(ffx::startupLog,std::memory_order_release);
    IDirectInput8W* input=nullptr;
    const auto result=proxy_DirectInput8Create(GetModuleHandleW(nullptr),DIRECTINPUT_VERSION,IID_IDirectInput8W,
        reinterpret_cast<void**>(&input),nullptr);
    const bool asynchronous=WaitForSingleObject(speechStarted,5000)==WAIT_OBJECT_0 && owner!=caller;
    SetEvent(releaseSpeech);
    if(WaitForSingleObject(completed,5000)!=WAIT_OBJECT_0) return 1;
    if(FAILED(result) || !input || !asynchronous || failures!=1 || readerCalls!=(failSpeech?0u:1u)) {
        std::fprintf(stderr,"FAIL: startup exception did not preserve system DirectInput forwarding\n"); return 1;
    }
    input->Release();CloseHandle(speechStarted);CloseHandle(releaseSpeech);CloseHandle(completed);
    std::puts(failSpeech?"PASS: speech bootstrap exception is contained and native DirectInput still forwards.":
        "PASS: missing-reader-address exception is contained and native DirectInput still forwards.");
}
