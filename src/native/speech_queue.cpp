#include "speech_queue.hpp"
#include "speech_domains.hpp"
#include "settings.hpp"
#include "process_storage.hpp"
#include <atomic>
#include <deque>
#include <mutex>
#include <cstring>

namespace ffx::speechq {
#ifdef FFX_SPEECH_WAKE_TEST
void captureBeforeDrainForTest();
#endif
namespace {
// This mutex is shared only by the preparation and backend workers. Native
// producers never enter the SDK deque or destroy its strings.
std::mutex sinkMutex;
using Sink=std::deque<Utterance>;
ProcessStorage<Sink> sinkStorage;
Sink& sink() noexcept {return sinkStorage.get();}
struct PreparationStorage { std::wstring lastSpeech; Utterance focus; };
ProcessStorage<PreparationStorage> preparationStorage;
PreparationStorage& preparation() noexcept {return preparationStorage.get();}
std::atomic<unsigned> sinkCount{0},mapEpoch{1},inputEpoch{0},modalEpoch{0},tutorialEpoch{0};
std::atomic<bool> protectedQueue{false},modalActive{false},tutorialActive{false};
std::atomic<unsigned> stopRequest{0},stopMap{0};
std::atomic<HANDLE> prepareWake{nullptr},sinkWake{nullptr};
unsigned lostTutorialEnd=0;
std::atomic<unsigned> lostEnds{0};
void wake(HANDLE event) noexcept {if(event) SetEvent(event);}
void wakePreparation() noexcept {wake(prepareWake.load(std::memory_order_acquire));}
void wakeBackend() noexcept {wake(sinkWake.load(std::memory_order_acquire));}
Header header(unsigned flags,const char* producer) {
    Header result{};result.flags=flags;result.generation=mapEpoch.load();result.cancellation=inputEpoch.load();
    result.modal=modalEpoch.load();result.tutorial=tutorialEpoch.load();result.budgetEpoch=currentBudgetEpoch();
    result.owner=GetCurrentThreadId();result.queuedAt=GetTickCount64();
    result.context=(setting(Setting::Speech)?SpeechEnabled:0u)|(protectedQueue.load()?QueueProtected:0u)|
        (modalActive.load()?ModalOpen:0u)|(tutorialActive.load()?TutorialOpen:0u);
    if(domainsProtectSpeech(result)) result.context|=QueueProtected;
    if(producer) {
        const auto length=strnlen_s(producer,sizeof(result.producer)-1);
        std::memcpy(result.producer,producer,length);result.producer[length]=0;
    }
    return result;
}
void retireSink() {
    std::lock_guard<std::mutex> lock(sinkMutex);
    for(auto item=sink().begin();item!=sink().end();) {
        if(!current(item->header)) {release(item->header);item=sink().erase(item);sinkCount.fetch_sub(1);}
        else ++item;
    }
    if(!preparation().focus.text.empty() && !current(preparation().focus.header)) preparation().focus={};
}
bool enqueue(Utterance item) {
    if(!current(item.header)) {release(item.header);return false;}
    if(!item.header.charged) {
        item.header.budgetEpoch=currentBudgetEpoch();
        if(!reserve(item.header,static_cast<unsigned>(item.text.size()))) return false;
    }
    try {
        std::lock_guard<std::mutex> lock(sinkMutex);
        if(!current(item.header)) {release(item.header);return false;}
        sink().push_back(std::move(item));sinkCount.fetch_add(1,std::memory_order_release);
    } catch(...) {release(item.header);throw;}
    wakeBackend();return true;
}
void flushFocus() {
    if(preparation().focus.text.empty()) return;
    auto item=std::move(preparation().focus);preparation().focus={};
    item.header.flags&=~Deferred;item.header.flags&=~Interrupt;
    (void)enqueue(std::move(item));
}
void process(Header& captureHeader,std::wstring text) {
    if(captureHeader.flags&Control) {
        if(captureHeader.control==TutorialEnd) {if(current(captureHeader)) flushFocus();}
        else if(captureHeader.control==GenerationRecord) {
            advanceDomains(captureHeader.generation);
            recordLog("generation=%u event=%s owner=%lu",captureHeader.generation,captureHeader.producer,captureHeader.owner);
        } else dispatchDomain(captureHeader,text);
        return;
    }
    if(domainsProtectSpeech(captureHeader)) captureHeader.flags&=~Interrupt;
    if(captureHeader.flags&Repeat) {
        text=preparation().lastSpeech.empty()?L"Nothing to repeat":preparation().lastSpeech;
        captureHeader.flags=Mod|(captureHeader.flags&Interrupt);
        (void)enqueue({std::move(text),captureHeader});return;
    }
    // Match producer-time repeat semantics even when this accepted line was
    // subsequently retired before preparation. Tutorial focus was not the
    // legacy repeat buffer and remains separate.
    if((captureHeader.flags&(Mod|Deferred))==0) preparation().lastSpeech=text;
    if(!current(captureHeader)) {release(captureHeader);return;}
    if(captureHeader.flags&Deferred) {
        if(!preparation().focus.text.empty() && preparation().focus.header.tutorial!=captureHeader.tutorial) flushFocus();
        if((captureHeader.flags&Detail) && !preparation().focus.text.empty()) {
            if(text.size()+2<CharacterLimit-preparation().focus.text.size()) preparation().focus.text+=L". "+text;
        } else preparation().focus={std::move(text),captureHeader};
        return;
    }
    if(!preparation().focus.text.empty() && preparation().focus.header.tutorial!=captureHeader.tutorial) flushFocus();
    auto item=Utterance{std::move(text),captureHeader};captureHeader.charged=false;
    (void)enqueue(std::move(item));
}
DWORD WINAPI prepareThread(void*) {
    for(;;) {
#ifdef FFX_SPEECH_WAKE_TEST
        captureBeforeDrainForTest();
#endif
        try {
            retireSink();Header captureHeader{};std::wstring text;
            while(pop(captureHeader,text)) {
                try {retireSink();process(captureHeader,std::move(text));}
                catch(...) {release(captureHeader);throw;}
            }
            // Retire even if the bounded capture queue rejected its marker.
            // Monotonic advancement preserves fresh same-generation voices.
            advanceDomains(generation());
            maintainDomains();
            const auto lost=lostEnds.load();
            if(lost!=lostTutorialEnd && !capturesPending()) {lostTutorialEnd=lost;flushFocus();}
        } catch(...) {recordLog("speech preparation rejected: allocation or processing failure");}
        const HANDLE event=prepareWake.load(std::memory_order_acquire);
        if(event) WaitForSingleObject(event,INFINITE);
    }
}
}
unsigned generation() noexcept {return mapEpoch.load(std::memory_order_acquire);}
void notifyPreparation() noexcept {wakePreparation();}
unsigned silenceRequest() noexcept {return stopRequest.load(std::memory_order_acquire);}
unsigned silenceGeneration() noexcept {return stopMap.load(std::memory_order_acquire);}
bool pending() noexcept {return sinkCount.load(std::memory_order_acquire)!=0;}
bool current(const Header& item) noexcept {
    if(item.generation!=generation() || item.cancellation!=inputEpoch.load()) return false;
    if((item.flags&(Mod|Protected))==0 && (item.modal!=modalEpoch.load() || modalActive.load())) return false;
    return true;
}
bool take(Utterance& item) {
    std::lock_guard<std::mutex> lock(sinkMutex);
    if(sink().empty()) return false;
    item=std::move(sink().front());sink().pop_front();sinkCount.fetch_sub(1);
    release(item.header);item.header.charged=false;return true;
}
bool output(const wchar_t* text,size_t length,bool interrupt,const char* producer,bool voiced,unsigned epoch) {
    if(!text || !length || epoch!=generation()) return false;
    unsigned flags=(interrupt?Interrupt:0u)|(voiced?Voiced:0u);
    const bool mod=producer && std::strncmp(producer,"mod-",4)==0;
    if(mod) flags|=Mod;
    if(!mod && !setting(Setting::Speech)) return false;
    auto item=header(flags,producer);
    const bool protectedNow=(item.context&QueueProtected)!=0;
    if(protectedNow) flags&=~Interrupt;
    if(protectedNow && producer && (!std::strcmp(producer,"battle-formation") || !std::strcmp(producer,"battle-message"))) flags|=Protected;
    if(!mod && modalActive.load() && (flags&Protected)==0) return false;
    if(tutorialActive.load() && producer && (!std::strncmp(producer,"sphere-",7) || !std::strcmp(producer,"selection-description"))) {
        flags|=Deferred;flags&=~Interrupt;
        if(!std::strcmp(producer,"selection-description")) flags|=Detail;
    }
    item.flags=flags;item.generation=epoch;
    const bool accepted=capture(item,text,length);
    // A rejected writer may have been the ordering barrier for an earlier
    // ready record. Wake on either result so retirement cannot lose a wake.
    wakePreparation();
    return accepted;
}
bool captureDomain(unsigned domain,const void* data,unsigned bytes,const wchar_t* text,unsigned length,unsigned epoch) {
    if(domain>=DomainLimit || bytes%sizeof(wchar_t) || epoch!=generation()) return false;
    auto item=header(Control,"native-domain");item.control=DomainBase+domain;item.generation=epoch;
    const bool accepted=captureParts(item,static_cast<const wchar_t*>(data),bytes/sizeof(wchar_t),text,length);
    wakePreparation();return accepted;
}
bool emitOwned(std::wstring text,const Header& anchor,const char* producer,bool voiced,bool interrupt) {
    if(text.empty()) return false;
    auto item=anchor;item.control=None;item.charged=false;item.length=static_cast<unsigned>(text.size());
    unsigned flags=(interrupt?Interrupt:0u)|(voiced?Voiced:0u);
    const bool mod=producer && std::strncmp(producer,"mod-",4)==0;
    if(mod) flags|=Mod;
    if(!mod && !(anchor.context&SpeechEnabled)) return false;
    if((anchor.context&QueueProtected) || domainsProtectSpeech(anchor)) {
        flags&=~Interrupt;
        if(producer && (!std::strcmp(producer,"battle-formation") || !std::strcmp(producer,"battle-message"))) flags|=Protected;
    }
    if(!mod && (anchor.context&ModalOpen) && !(flags&Protected)) return false;
    if((anchor.context&TutorialOpen) && producer && (!std::strncmp(producer,"sphere-",7) || !std::strcmp(producer,"selection-description"))) {
        flags|=Deferred;flags&=~Interrupt;if(!std::strcmp(producer,"selection-description")) flags|=Detail;
    }
    item.flags=flags;std::memset(item.producer,0,sizeof(item.producer));
    if(producer) std::memcpy(item.producer,producer,strnlen_s(producer,sizeof(item.producer)-1));
    const bool ready=current(item);process(item,std::move(text));return ready;
}
void protect(bool value) noexcept {if(!value) resetDomainProtection();protectedQueue.store(value);}
void modal(bool value) noexcept {
    if(value) modalEpoch.fetch_add(1);
    modalActive.store(value);wakePreparation();
}
void tutorial(bool value) {
    tutorialActive.store(value);
    if(!value) {
        auto item=header(Control,"tutorial-end");item.control=TutorialEnd;
        tutorialEpoch.fetch_add(1);
        if(!capture(item,nullptr,0)) lostEnds.fetch_add(1);
        wakePreparation();
    }
}
void interrupt() noexcept {
    const auto request=inputEpoch.fetch_add(1)+1;
    (void)nextBudgetEpoch();protectedQueue=false;resetDomainProtection();tutorialActive=false;tutorialEpoch.fetch_add(1);
    stopMap.store(generation());stopRequest.store(request,std::memory_order_release);
    wakePreparation();wakeBackend();
}
void repeat() {
    (void)capture(header(Repeat|Mod|(protectedQueue.load()?0u:Interrupt),"mod-repeat"),nullptr,0);wakePreparation();
}
void invalidate(const char* event) {
    mapEpoch.fetch_add(1);(void)nextBudgetEpoch();protectedQueue=false;resetDomainProtection();tutorialActive=false;tutorialEpoch.fetch_add(1);
    modalActive=false;modalEpoch.fetch_add(1);
    auto item=header(Control,event);item.control=GenerationRecord;
    (void)capture(item,nullptr,0);wakePreparation();wakeBackend();
}
bool start(HMODULE module,HANDLE backendWake) {
    // The caller is the speech bootstrap owner, before either worker can use
    // this process-lifetime deque. Native attach/detach never allocates/frees it.
    sinkStorage.prepare();
    preparationStorage.prepare();
    HMODULE pinned=nullptr;
    if(!GetModuleHandleExW(GET_MODULE_HANDLE_EX_FLAG_FROM_ADDRESS|GET_MODULE_HANDLE_EX_FLAG_PIN,
        reinterpret_cast<LPCWSTR>(&prepareThread),&pinned)) return false;
    (void)module;sinkWake.store(backendWake,std::memory_order_release);
    const auto event=CreateEventW(nullptr,FALSE,FALSE,nullptr);
    if(!event) return false;
    prepareWake.store(event,std::memory_order_release);
    const auto thread=CreateThread(nullptr,0,prepareThread,nullptr,0,nullptr);
    if(!thread) return false; // published storage/event are process lifetime
    CloseHandle(thread);return true;
}
}
