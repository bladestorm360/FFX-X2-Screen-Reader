#include "runtime.hpp"
#include <cstdio>
#include <share.h>
#include <atomic>
#include <cstring>
#include <algorithm>
#include <new>
#include "settings.hpp"
#include "log_history.hpp"
#include "prism_backend.hpp"
#include "nonblocking_snapshots.hpp"
#include "speech_queue.hpp"
#include "process_storage.hpp"

namespace ffx {
#ifdef FFX_SPEECH_WAKE_TEST
void speechBeforeWaitForTest();
void logBeforeWriteForTest(const char* message);
void logBeforeWaitForTest();
#endif
namespace {
size_t readyWideLength(const wchar_t* text) noexcept {
    __try {return wcsnlen_s(text,speechq::CharacterLimit+1);}
    __except(EXCEPTION_EXECUTE_HANDLER) {return speechq::CharacterLimit+1;}
}
using LogPacket=LogCapture;
static_assert(std::is_trivially_copyable<LogPacket>::value, "log packet must be POD");
constexpr size_t LogByteLimit=1024*1024;
static_assert(std::atomic<size_t>::is_always_lock_free, "log byte reservation must not wait");
async::TryCaptureQueue<LogPacket,4096>* logQueueStorage=nullptr;
std::atomic<async::TryCaptureQueue<LogPacket,4096>*> logQueue{nullptr};
std::atomic<HANDLE> logWake{nullptr};
std::atomic<size_t> logQueuedBytes{0};
std::atomic<unsigned> logDropped{0};
std::atomic<bool> ready{false},starting{false};
std::atomic<FILE*> logfile{nullptr};
ProcessStorage<std::wstring> directoryStorage;
std::wstring& directory() noexcept {return directoryStorage.get();}
HANDLE initialized = nullptr;
using BackendInfo=SpeechBackendSnapshot;
static_assert(std::is_trivially_copyable<BackendInfo>::value, "backend snapshot must be POD");
static_assert(sizeof(BackendInfo) % sizeof(std::uint32_t) == 0, "backend snapshot word alignment");
async::AtomicSnapshot<BackendInfo> backendSnapshot;
std::atomic<int> requestedBackend{-1};
std::atomic<HANDLE> speechWake{nullptr};
void wakeSpeech() noexcept {
    const HANDLE event=speechWake.load(std::memory_order_acquire);
    if(event) SetEvent(event);
}
bool copyBackendText(wchar_t (&target)[128],const std::wstring& source) {
    if(source.size()>=_countof(target)) return false;
    std::wmemcpy(target,source.c_str(),source.size()+1);
    return true;
}
bool snapshotBackend(PrismSpeechBackend& backend,BackendInfo& current,bool list=false) {
    if(list) {
        const auto choices=backend.choices();
        if(choices.size()>current.rows.size()) return false;
        BackendInfo next{};
        if(!copyBackendText(next.status,backend.name())) return false;
        next.count=static_cast<int>(choices.size());
        next.ready=true;
        for(size_t i=0;i<choices.size();++i) {
            next.rows[i].preference=choices[i].preference;
            if(!copyBackendText(next.rows[i].name,choices[i].name)) return false;
        }
        current=next;
    } else {
        if(!copyBackendText(current.status,backend.name())) return false;
        current.ready=true;
    }
    backendSnapshot.publish(current);
    return true;
}

DWORD WINAPI logThread(void*) {
    for (;;) {
        auto* queue=logQueue.load(std::memory_order_acquire);
        LogPacket packet{};
        if(queue) while(queue->tryPop(packet)) {
            logQueuedBytes.fetch_sub(packet.byteCost,std::memory_order_acq_rel);
            char message[4096]{};
            formatLogCapture(packet,message,sizeof(message));
#ifdef FFX_SPEECH_WAKE_TEST
            ffx::logBeforeWriteForTest(message);
#endif
            const auto file=logfile.load(std::memory_order_acquire);
            if(file) {
                FILETIME local{};SYSTEMTIME timestamp{};
                FileTimeToLocalFileTime(&packet.timestamp,&local);FileTimeToSystemTime(&local,&timestamp);
                char prefix[32]{};
                sprintf_s(prefix,"[%02u:%02u:%02u.%03u] ",timestamp.wHour,timestamp.wMinute,
                    timestamp.wSecond,timestamp.wMilliseconds);
                fputs(prefix,file);fputs(message,file);fputs("\n",file);fflush(file);
            }
        }
        if(const auto lost=logDropped.exchange(0,std::memory_order_relaxed)) {
            if(const auto file=logfile.load(std::memory_order_acquire)) {
                fprintf(file,"log backpressure rejected=%u\n",lost);fflush(file);
            }
        }
#ifdef FFX_SPEECH_WAKE_TEST
        ffx::logBeforeWaitForTest();
#endif
        const HANDLE event=logWake.load(std::memory_order_acquire);
        if(event) WaitForSingleObject(event,INFINITE);
    }
}

DWORD WINAPI speechThread(void*) {
    PrismSpeechBackend backend;
    int appliedPreference=setting(Setting::SpeechBackend);
    BackendInfo currentBackendInfo{};
    ready=backend.open(directory(),appliedPreference);
    if(ready) (void)snapshotBackend(backend,currentBackendInfo,true);
    recordLog("speech worker=%lu Prism=%s",GetCurrentThreadId(),ready?"ready":"unavailable");
    starting=false;SetEvent(initialized);
    if(!ready) return 0;
    unsigned stopped=0;
    for(;;) {
        if(requestedBackend.load(std::memory_order_acquire)<0 &&
            speechq::silenceRequest()==stopped && !speechq::pending()) {
#ifdef FFX_SPEECH_WAKE_TEST
            ffx::speechBeforeWaitForTest();
#endif
            const HANDLE event=speechWake.load(std::memory_order_acquire);
            if(event) WaitForSingleObject(event,INFINITE);
            continue;
        }
        const int requested=requestedBackend.exchange(-1,std::memory_order_acq_rel);
        const int preference=requested>=0?requested:setting(Setting::SpeechBackend);
        if(preference!=appliedPreference) {
            if(backend.choose(preference)) {
                appliedPreference=preference;setSetting(Setting::SpeechBackend,preference);
                (void)snapshotBackend(backend,currentBackendInfo);
                (void)backend.output(L"Speech backend: "+backend.name(),true);
            } else {
                setSetting(Setting::SpeechBackend,appliedPreference);
                (void)backend.output(L"Requested speech backend unavailable. Keeping "+backend.name(),true);
            }
            continue;
        }
        const auto stop=speechq::silenceRequest();
        if(stop!=stopped) {
            stopped=stop;
            if(speechq::silenceGeneration()==generation())
                recordLog("input silence worker=%lu accepted=%d",GetCurrentThreadId(),backend.stop());
            continue;
        }
        speechq::Utterance item;
        if(!speechq::take(item)) continue;
        const auto& h=item.header;
        if(!speechq::current(h) || ((h.flags&speechq::Mod)==0 && !setting(Setting::Speech))) {
            recordLog("discard seq=%u gen=%u current=%u",h.sequence,h.generation,generation());continue;
        }
        const int size=WideCharToMultiByte(CP_UTF8,0,item.text.c_str(),-1,nullptr,0,nullptr,nullptr);
        if(size>0) {
            std::string utf8(static_cast<size_t>(size),'\0');
            WideCharToMultiByte(CP_UTF8,0,item.text.c_str(),-1,&utf8[0],size,nullptr,nullptr);
            recordLog("text seq=%u interrupt=%d %s",h.sequence,(h.flags&speechq::Interrupt)!=0,utf8.c_str());
        }
        const int mode=(h.flags&speechq::Voiced)?setting(Setting::VoicedDialogue):2;
        if(mode==0) {recordLog("dialogue suppressed seq=%u",h.sequence);continue;}
        const auto submitted=GetTickCount64();
        const bool success=backend.output(item.text,(h.flags&speechq::Interrupt)!=0,mode==1);
        recordLog("output seq=%u gen=%u owner=%lu accepted=%d mode=%d queue-ms=%llu prism-ms=%llu",
            h.sequence,h.generation,h.owner,success,mode,submitted-h.queuedAt,GetTickCount64()-submitted);
    }
}
}

#ifdef FFX_SPEECH_WAKE_TEST
size_t queuedLogBytesForTest() noexcept {return logQueuedBytes.load();}
unsigned droppedLogsForTest() noexcept {return logDropped.load();}
#endif
bool copySpeechBackendSnapshot(SpeechBackendSnapshot& info) {
    return backendSnapshot.tryRead(info) && info.ready && info.count>=0 && static_cast<unsigned>(info.count)<=info.rows.size();
}
std::vector<SpeechBackendChoice> speechBackendChoices() {
    BackendInfo info{};
    if(!backendSnapshot.tryRead(info) || !info.ready) return {};
    std::vector<SpeechBackendChoice> choices;
    choices.reserve(static_cast<size_t>(info.count));
    for(int i=0;i<info.count;++i) choices.push_back({info.rows[i].preference,info.rows[i].name});
    return choices;
}
std::wstring speechBackendStatus() {
    BackendInfo info{};
    if(!backendSnapshot.tryRead(info) || !info.ready) return {};
    return info.status;
}
void selectSpeechBackend(int preference) {
    if(!ready || preference<0 || preference>15) return;
    requestedBackend.store(preference,std::memory_order_release);
    wakeSpeech();
}
void submitLogCapture(const LogCapture& packet) noexcept {
    if (!logfile.load()) return;
    auto* queue=logQueue.load(std::memory_order_acquire);
    const HANDLE event=logWake.load(std::memory_order_acquire);
    if(!queue || !event) return;
    const size_t reserved=logQueuedBytes.fetch_add(packet.byteCost,std::memory_order_acq_rel)+packet.byteCost;
    if(reserved>LogByteLimit || !queue->tryPush(packet)) {
        logQueuedBytes.fetch_sub(packet.byteCost,std::memory_order_acq_rel);
        logDropped.fetch_add(1,std::memory_order_relaxed);
        return;
    }
    SetEvent(event);
}

void prepareSpeechStartup() {starting=true;}
void cancelSpeechStartup() {starting=false;}
bool startSpeech(HMODULE module, const wchar_t* logName) {
    prepareSpeechStartup();
    directoryStorage.prepare();
    struct FailedStartup {bool handedOff=false;~FailedStartup(){if(!handedOff) cancelSpeechStartup();}} startup;
    // Called by the proxy startup worker (or isolated launcher startup).
    // The proxy is process-lifetime: do not unload code under a live worker.
    HMODULE pinned;
    if (!GetModuleHandleExW(GET_MODULE_HANDLE_EX_FLAG_FROM_ADDRESS | GET_MODULE_HANDLE_EX_FLAG_PIN,
        reinterpret_cast<LPCWSTR>(&speechThread), &pinned)) return false;
    loadSettings();
    if(!startSettingsWriter(module)) return false;
    wchar_t path[MAX_PATH]{};
    if (!GetModuleFileNameW(module, path, MAX_PATH)) return false;
    directory() = path;
    directory().resize(directory().find_last_of(L"\\/") + 1);
    logfile = openLogWithHistory(directory(),logName);
    if(logfile) {
        logQueueStorage=new(std::nothrow) async::TryCaptureQueue<LogPacket,4096>();
        HANDLE wake=CreateEventW(nullptr,FALSE,FALSE,nullptr);
        if(!logQueueStorage || !wake) return false;
        logQueue.store(logQueueStorage,std::memory_order_release);
        logWake.store(wake,std::memory_order_release);
        const auto logger=CreateThread(nullptr,0,logThread,nullptr,0,nullptr);
        if(!logger) return false; // published queue/event remain alive for racing producers
        CloseHandle(logger);
        logCaptureHandler.store(&submitLogCapture,std::memory_order_release);
    }
    initialized = CreateEventW(nullptr, TRUE, FALSE, nullptr);
    if (!initialized) return false;
    HANDLE wake=CreateEventW(nullptr,FALSE,FALSE,nullptr);
    if(!wake) return false;
    speechWake.store(wake,std::memory_order_release);
    if(!speechq::start(module,wake)) return false;
    HANDLE worker = CreateThread(nullptr, 0, speechThread, nullptr, 0, nullptr);
    if (!worker) return false; // published wake remains process lifetime for racing producers
    startup.handedOff=true;
    WaitForSingleObject(initialized, INFINITE); // bootstrap is outside DllMain
    CloseHandle(worker);
    CloseHandle(initialized);
    return ready;
}

bool outputDialogueAt(const std::wstring& text,bool interrupt,const char* producer,bool voiced,unsigned epoch) {
    if(!ready && !starting) return false;
    return speechq::output(text.data(),text.size(),interrupt,producer,voiced,epoch);
}
bool outputDialogue(const std::wstring& text,bool interrupt,const char* producer,bool voiced) {
    return outputDialogueAt(text,interrupt,producer,voiced,generation());
}
bool outputAt(const std::wstring& text,bool interrupt,const char* producer,unsigned epoch) {
    return outputDialogueAt(text,interrupt,producer,false,epoch);
}
bool output(const wchar_t* text,bool interrupt,const char* producer) {
    if(!text || (!ready && !starting)) return false;
    const auto length=readyWideLength(text);
    if(length>speechq::CharacterLimit) return false;
    return speechq::output(text,length,interrupt,producer,false,generation());
}
bool output(const std::wstring& text,bool interrupt,const char* producer) {
    return outputDialogue(text,interrupt,producer,false);
}
void protectSpeech(bool value) {speechq::protect(value);}
void protectTutorialSpeech(bool value) {speechq::tutorial(value);}
void interruptSpeechForInput() {speechq::interrupt();}
void captureSpeech(bool value) {speechq::modal(value);}
void repeatSpeech(bool) {speechq::repeat();}
unsigned generation() {return speechq::generation();}
void invalidate(const char* event) {speechq::invalidate(event);}
}

extern "C" int __cdecl FfxNative_Output(const wchar_t* text,int interrupt) {
    if(!text || (!ffx::ready && !ffx::starting)) return 0;
    // Copy bounded ready UTF16 into owned storage; no temporary string/heap
    // allocation and no borrowed pointer outlives this call.
    const auto length=ffx::readyWideLength(text);
    return ffx::speechq::output(text,length,interrupt!=0,"api",false,ffx::generation())?1:0;
}
extern "C" int __cdecl FfxNative_SpeechReady() { return ffx::ready ? 1 : 0; }
