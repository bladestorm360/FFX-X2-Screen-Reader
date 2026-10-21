#include "../runtime.hpp"
#include "../settings.hpp"
#include "../wakka.hpp"
#include "../speech_backends.hpp"
#include "../speech_domains.hpp"
#include <atomic>
#include <mutex>
#include <cstring>
namespace ffx {
namespace speechq {
std::atomic<HANDLE> captureHoldEntered{nullptr},captureHoldResume{nullptr};
void captureBeforeDrainForTest() {
    const auto entered=captureHoldEntered.load(),resume=captureHoldResume.load();
    if(entered && resume) {SetEvent(entered);WaitForSingleObject(resume,INFINITE);}
}
}
std::atomic<unsigned> domainCopies{0};
std::atomic<DWORD> domainOwner{0};
std::atomic<unsigned> domainResetEpoch{0};
void resetDomainFixture(unsigned epoch) {domainResetEpoch=epoch;}
struct DomainFixture {unsigned id,guard;};
void processDomainFixture(const speechq::Header& header,const std::wstring& owned) {
    if(owned.size()*sizeof(wchar_t)<sizeof(DomainFixture)) return;
    DomainFixture data;memcpy(&data,owned.data(),sizeof(data));
    const auto text=owned.substr(sizeof(data)/sizeof(wchar_t));
    if(data.id==29 && data.guard==0x534e4150 && text==L"Owned domain payload") ++domainCopies;
    domainOwner=GetCurrentThreadId();
    speechq::emitOwned(text,header,"domain-fixture",false,false);
}
size_t queuedLogBytesForTest() noexcept;
unsigned droppedLogsForTest() noexcept;
std::atomic<HANDLE> speechWaitEntered{nullptr},speechWaitResume{nullptr};
std::atomic<HANDLE> logHoldEntered{nullptr},logHoldResume{nullptr},logCopiesDone{nullptr};
std::atomic<HANDLE> logWaitEntered{nullptr},logWaitResume{nullptr},logLateDone{nullptr};
std::mutex logFixtureMutex;
std::vector<std::string> logFixtureRows;
std::atomic<DWORD> logFixtureOwner{0};
void logBeforeWriteForTest(const char* message) {
    if(std::strncmp(message,"LOGFIXTURE.",11)!=0) return;
    logFixtureOwner=GetCurrentThreadId();
    if(!std::strcmp(message,"LOGFIXTURE.HOLD")) {
        const auto entered=logHoldEntered.load(),resume=logHoldResume.load();
        if(entered && resume) {SetEvent(entered);WaitForSingleObject(resume,INFINITE);}
    }
    if(!std::strncmp(message,"LOGFIXTURE.COPY",15) || !std::strcmp(message,"LOGFIXTURE.EQUAL")) {
        std::lock_guard<std::mutex> lock(logFixtureMutex);logFixtureRows.emplace_back(message);
    }
    if(!std::strcmp(message,"LOGFIXTURE.COPIES.DONE")) {if(const auto done=logCopiesDone.load()) SetEvent(done);}
    if(!std::strcmp(message,"LOGFIXTURE.LATE")) {if(const auto done=logLateDone.load()) SetEvent(done);}
}
void logBeforeWaitForTest() {
    const auto entered=logWaitEntered.load(),resume=logWaitResume.load();
    if(entered && resume) {SetEvent(entered);WaitForSingleObject(resume,INFINITE);}
}
void speechBeforeWaitForTest() {
    const auto entered=speechWaitEntered.load(),resume=speechWaitResume.load();
    if(entered && resume) {SetEvent(entered);WaitForSingleObject(resume,INFINITE);}
}
}
#include <cstdio>
#include <thread>
#include <stdexcept>
static void require(bool condition, const char* message) { if (!condition) throw std::runtime_error(message); }
int main() {
    try {
        const uint8_t encoded[] = {0x50, 0x07, 0x71, 0x51, 0x00}; // A, style payload, B
        auto parts = ffx::decode(reinterpret_cast<uintptr_t>(encoded), sizeof(encoded));
        require(parts.size() == 1 && parts[0] == L"AB", "control payload leaked into text");
        const uint8_t separators[] = {0x50, 0x0a, 0x71, 0x51, 0x03, 0x52, 0};
        parts = ffx::decode(reinterpret_cast<uintptr_t>(separators), sizeof(separators));
        require(parts.size() == 3 && parts[0] == L"A" && parts[1] == L"B" && parts[2] == L"C", "segment order");
        require(ffx::decode(0, 256).empty(), "null memory");
        const uint8_t prefix[] = {0x2b};
        require(ffx::decode(reinterpret_cast<uintptr_t>(prefix), 1).empty(), "truncated multibyte character");
        auto pages = static_cast<uint8_t*>(VirtualAlloc(nullptr, 8192, MEM_RESERVE | MEM_COMMIT, PAGE_READWRITE));
        require(pages != nullptr, "test allocation"); DWORD old;
        VirtualProtect(pages + 4096, 4096, PAGE_NOACCESS, &old);
        pages[4094] = 0x50; pages[4095] = 0;
        parts = ffx::decode(reinterpret_cast<uintptr_t>(pages + 4094), 256);
        require(parts.size() == 1 && parts[0] == L"A", "terminator at unreadable page boundary");
        pages[4095] = 0x04;
        parts = ffx::decode(reinterpret_cast<uintptr_t>(pages + 4094), 256);
        require(parts.size() == 1 && parts[0] == L"A", "truncated opcode at page boundary");
        VirtualFree(pages, 0, MEM_RELEASE);
        require(ffx::startSpeech(GetModuleHandleW(nullptr), L"runtime_test.log"), "fake Prism bootstrap (PrismRuntime\\prism.dll)");
        auto prism = GetModuleHandleW(L"prism.dll");
        auto count = reinterpret_cast<int(__cdecl*)()>(GetProcAddress(prism, "Test_Count"));
        auto record = reinterpret_cast<DWORD(__cdecl*)(int, wchar_t*, unsigned)>(GetProcAddress(prism, "Test_Read"));
        require(count && record, "test exports");
        std::wstring source = L"Copied before return";
        require(ffx::output(source, true, "test.copy"), "enqueue copy"); source = L"Changed";
        std::vector<std::thread> producers;
        for (int i = 0; i < 4; ++i) producers.emplace_back([] {
            for (int j = 0; j < 100; ++j) require(ffx::output(L"Repeated valid event", false, "test.repeat"), "enqueue repeat");
        });
        for (auto& producer : producers) producer.join();
        const auto deadline = GetTickCount64() + 10000;
        while (count() < 401 && GetTickCount64() < deadline) Sleep(1);
        require(count() == 401, "lost or duplicated speech event");
        wchar_t text[256]{};
        const auto owner = record(0, text, 256);
        require(owner && owner != GetCurrentThreadId() && wcscmp(text, L"Copied before return") == 0, "copied text / worker ownership");
        for (int i = 1; i < 401; ++i) {
            require(record(i, text, 256) == owner && wcscmp(text, L"Repeated valid event") == 0, "multiple speech owners / text deduplication");
        }
        const auto generation = ffx::generation(); ffx::invalidate("test.map-teardown");
        require(ffx::generation() == generation + 1, "map generation");
        require(ffx::output(L"New generation", true, "test.map"), "generation output");
        while (count() < 402 && GetTickCount64() < deadline) Sleep(1);
        require(count() == 402, "new generation lost");
        ffx::captureSpeech(true);
        require(!ffx::output(L"Suppressed game event",true,"test.modal"),"modal game speech suppressed");
        require(ffx::output(L"Settings still accessible",true,"mod-menu"),"modal settings speech allowed");
        ffx::captureSpeech(false); ffx::setSetting(ffx::Setting::Speech,0);
        require(!ffx::output(L"Disabled game event",true,"test.disabled"),"disabled game speech suppressed");
        ffx::repeatSpeech(false);
        while(count()<404 && GetTickCount64()<deadline) Sleep(1);
        require(count()==404,"mod speech or repeat lost when game speech disabled");
        record(403,text,256); require(wcscmp(text,L"New generation")==0,"repeat preserves last copied game event");
        ffx::setSetting(ffx::Setting::Speech,1);
        auto braille=reinterpret_cast<bool(__cdecl*)(int)>(GetProcAddress(prism,"Test_Braille"));
        require(braille!=nullptr,"braille test export");
        auto waitCount=[&](int expected) {
            auto limit=GetTickCount64()+5000;
            while(count()<expected && GetTickCount64()<limit) Sleep(1);
            require(count()==expected,"dialogue output count");
        };
        ffx::setSetting(ffx::Setting::VoicedDialogue,0);
        require(ffx::outputDialogue(L"Suppressed voiced line",false,"event-dialogue",true),"suppressed line copied");
        // Ordered sentinel proves the worker processed the suppressed line.
        ffx::output(L"Menu still spoken",false,"window-focus"); waitCount(405);
        record(404,text,256); require(wcscmp(text,L"Menu still spoken")==0 && !braille(404),"voiced policy cannot silence menus");
        ffx::outputDialogue(L"Suppressed repeat",false,"npc-dialogue",true);
        ffx::repeatSpeech(false); waitCount(406);
        record(405,text,256); require(wcscmp(text,L"Suppressed repeat")==0 && !braille(405),"explicit repeat speaks suppressed line");
        ffx::setSetting(ffx::Setting::VoicedDialogue,1);
        ffx::outputDialogue(L"Braille voiced line",false,"event-dialogue",true); waitCount(407);
        require(record(406,text,256)==owner && braille(406) && wcscmp(text,L"Braille voiced line")==0,"braille only shares worker without Tolk_Output");
        ffx::outputDialogue(L"Unvoiced line",false,"npc-dialogue",false); waitCount(408);
        require(!braille(407),"unvoiced dialogue remains speech in braille mode");
        ffx::setSetting(ffx::Setting::VoicedDialogue,2);
        ffx::outputDialogue(L"Speak voiced line",false,"npc-dialogue",true); waitCount(409);
        require(!braille(408),"speak anyway uses normal speech and braille");
        auto brailleAvailable=reinterpret_cast<void(__cdecl*)(bool)>(GetProcAddress(prism,"Test_BrailleAvailable"));
        require(brailleAvailable!=nullptr,"braille availability export"); brailleAvailable(false);
        ffx::setSetting(ffx::Setting::VoicedDialogue,1);
        ffx::outputDialogue(L"No braille device",false,"event-dialogue",true);
        ffx::output(L"Availability sentinel",false,"window-focus"); waitCount(410);
        record(409,text,256); require(wcscmp(text,L"Availability sentinel")==0 && !braille(409),"unavailable braille must not speak voiced line");
        brailleAvailable(true);
        ffx::setSetting(ffx::Setting::VoicedDialogue,0);
        auto block=reinterpret_cast<void(__cdecl*)(HANDLE,HANDLE)>(GetProcAddress(prism,"Test_Block"));
        require(block!=nullptr,"blocking screen-reader fixture export");
        HANDLE submitted=CreateEventW(nullptr,TRUE,FALSE,nullptr),resume=CreateEventW(nullptr,TRUE,FALSE,nullptr);
        block(submitted,resume);
        ffx::output(L"Slow external screen reader",false,"test.slow");
        require(WaitForSingleObject(submitted,2000)==WAIT_OBJECT_0,"worker entered blocked Tolk call");
        HANDLE finished=CreateEventW(nullptr,TRUE,FALSE,nullptr);
        std::thread gameCallback([&] {
            ffx::output(L"Old queued line",false,"test.old");
            ffx::invalidate("test.nonblocking-load"); ffx::captureSpeech(true);
            ffx::output(L"New mod event",false,"mod-test");
            ffx::recordLog("read-only load callback returned while Tolk blocked");
            SetEvent(finished);
        });
        const bool independent=WaitForSingleObject(finished,1000)==WAIT_OBJECT_0;
        SetEvent(resume); gameCallback.join();
        require(independent,"game callbacks must not wait for screen-reader output");
        waitCount(412);
        record(411,text,256); require(wcscmp(text,L"New mod event")==0,"load cancels old queued generation without blocking Tolk");
        block(nullptr,nullptr); ffx::captureSpeech(false);
        CloseHandle(submitted); CloseHandle(resume); CloseHandle(finished);
        auto interruptFlag=reinterpret_cast<bool(__cdecl*)(int)>(GetProcAddress(prism,"Test_Interrupt"));
        require(interruptFlag!=nullptr,"interruption test export");
        ffx::setSetting(ffx::Setting::VoicedDialogue,2); ffx::protectSpeech(true);
        submitted=CreateEventW(nullptr,TRUE,FALSE,nullptr); resume=CreateEventW(nullptr,TRUE,FALSE,nullptr);
        block(submitted,resume); ffx::output(L"Enemies: Melusine, Kusariqqu, Aerouge",false,"battle-formation");
        require(WaitForSingleObject(submitted,2000)==WAIT_OBJECT_0,"formation worker entered Tolk");
        ffx::output(L"Pre-emptive strike!",false,"battle-message");
        ffx::captureSpeech(true); ffx::output(L"Manual read",true,"mod-test");
        SetEvent(resume); waitCount(415); block(nullptr,nullptr);
        ffx::captureSpeech(false); ffx::output(L"Attack",true,"battle-command");
        ffx::protectSpeech(false); waitCount(416);
        for(int i=412;i<416;++i) require(!interruptFlag(i),"protected battle queue cannot cancel opening speech");
        record(412,text,256); require(wcscmp(text,L"Enemies: Melusine, Kusariqqu, Aerouge")==0,"formation ordering");
        record(413,text,256); require(wcscmp(text,L"Pre-emptive strike!")==0,"native opening banner queues second");
        ffx::output(L"After battle",true,"window-focus"); waitCount(417);
        require(interruptFlag(416),"normal interrupts restored after battle");
        ffx::protectSpeech(true); ffx::invalidate("protected teardown");
        ffx::output(L"After teardown",true,"window-focus"); waitCount(418);
        require(interruptFlag(417),"resource generation clears speech protection");
        ffx::captureSpeech(true); ffx::protectSpeech(true);
        require(ffx::output(L"Enemies: Larva",true,"battle-formation"),"formation accepted during mod menu");
        require(ffx::output(L"Ambush!",true,"battle-message"),"opening banner accepted during mod menu");
        ffx::output(L"Menu",true,"mod-test"); waitCount(421);
        for(int i=418;i<421;++i) require(!interruptFlag(i),"mod menu cannot interrupt opening speech");
        record(418,text,256); require(wcscmp(text,L"Enemies: Larva")==0,"modal formation precedes banner");
        ffx::captureSpeech(false); ffx::protectSpeech(false);
        CloseHandle(submitted); CloseHandle(resume);
        submitted=CreateEventW(nullptr,TRUE,FALSE,nullptr); resume=CreateEventW(nullptr,TRUE,FALSE,nullptr);
        block(submitted,resume); ffx::output(L"Backpressure fixture",false,"test.block");
        require(WaitForSingleObject(submitted,2000)==WAIT_OBJECT_0,"blocked Tolk queue fixture");
        for(int i=0;i<1024;++i) require(ffx::output(L"Repeated event",false,"test.bound"),"distinct events retained to queue budget");
        require(!ffx::output(L"Overflow",false,"test.bound"),"event budget refuses overload without blocking game");
        ffx::invalidate("event-budget-reset");
        const std::wstring large(4096,L'x');
        for(int i=0;i<128;++i) require(ffx::output(large,false,"test.bytes"),"copied text retained to character budget");
        require(!ffx::output(L"Overflow",false,"test.bytes"),"text budget bounds memory while Tolk stalls");
        ffx::invalidate("text-budget-reset"); ffx::output(L"Recovered",false,"test.recovered");
        SetEvent(resume); waitCount(423); block(nullptr,nullptr);
        CloseHandle(submitted); CloseHandle(resume);
        auto silenceCount=reinterpret_cast<int(__cdecl*)()>(GetProcAddress(prism,"Test_SilenceCount"));
        auto silenceThread=reinterpret_cast<DWORD(__cdecl*)()>(GetProcAddress(prism,"Test_SilenceThread"));
        auto silenceOutputCount=reinterpret_cast<int(__cdecl*)()>(GetProcAddress(prism,"Test_SilenceOutputCount"));
        require(silenceCount && silenceThread && silenceOutputCount,"Tolk silence test exports");
        submitted=CreateEventW(nullptr,TRUE,FALSE,nullptr); resume=CreateEventW(nullptr,TRUE,FALSE,nullptr);
        block(submitted,resume); ffx::protectSpeech(true);
        ffx::output(L"Slow battle opening",false,"battle-formation");
        require(WaitForSingleObject(submitted,2000)==WAIT_OBJECT_0,"blocked opening call");
        ffx::output(L"Older queued banner",false,"battle-message");
        finished=CreateEventW(nullptr,TRUE,FALSE,nullptr);
        std::thread controllerInput([&] {
            ffx::interruptSpeechForInput();
            ffx::output(L"Current character status",true,"battle-active-stats");
            ffx::output(L"Queued status details",false,"battle-status");
            SetEvent(finished);
        });
        const bool inputIndependent=WaitForSingleObject(finished,1000)==WAIT_OBJECT_0;
        SetEvent(resume); controllerInput.join(); require(inputIndependent,"controller interruption cannot wait for Tolk");
        waitCount(426); block(nullptr,nullptr);
        require(silenceCount()==1 && silenceThread()==owner && silenceOutputCount()==424,"silence executes on the single Tolk worker before current action");
        record(424,text,256); require(wcscmp(text,L"Current character status")==0 && interruptFlag(424),"deliberate input releases battle protection and supersedes older pending output");
        record(425,text,256); require(wcscmp(text,L"Queued status details")==0 && !interruptFlag(425),"new action details retain normal queue ordering");
        CloseHandle(submitted); CloseHandle(resume); CloseHandle(finished);
        ffx::protectTutorialSpeech(true);
        ffx::output(L"Learn Cheer. Adjacent nodes can be activated.",false,"npc-dialogue"); waitCount(427);
        ffx::output(L"Power Sphere",true,"sphere-list");
        ffx::output(L"Ability Sphere",true,"sphere-list");
        ffx::output(L"Activate an ability node",false,"selection-description");
        Sleep(30); require(count()==427,"forced grid focus cannot interrupt a protected tutorial caption");
        ffx::protectTutorialSpeech(false); waitCount(428);
        record(427,text,256); require(wcscmp(text,L"Ability Sphere. Activate an ability node")==0 && !interruptFlag(427),
            "latest forced focus and its description follow the complete tutorial caption");
        ffx::output(L"Power Sphere",true,"sphere-list"); waitCount(429);
        require(interruptFlag(428),"ordinary focus remains interruptible outside tutorial narration");
        ffx::protectTutorialSpeech(true); ffx::output(L"Old forced focus",true,"sphere-node");
        ffx::interruptSpeechForInput(); ffx::protectTutorialSpeech(false);
        ffx::output(L"Current manual focus",true,"sphere-node"); waitCount(430);
        record(429,text,256); require(wcscmp(text,L"Current manual focus")==0,"manual input cancels deferred focus and restores normal speech");
        const auto choiceCount=count(); ffx::captureSpeech(true);
        require(!ffx::output(L"Paused battle event",false,"battle-action"),"choice menu continues to capture ordinary game speech");
        const wchar_t* choices[]={L"Match reels. Fire Shot",L"Ice Shot",L"Fire Shot",L"Aurochs Shot"};
        for(unsigned i=0;i<_countof(choices);++i) {
            require(ffx::output(choices[i],true,ffx::WakkaChoiceProducer),"paused Wakka highlight passes the real modal speech filter");
            waitCount(choiceCount+i+1); record(choiceCount+i,text,256);
            require(wcscmp(text,choices[i])==0 && interruptFlag(choiceCount+i),"each browsed outcome reaches Tolk_Output including repeated valid choices");
        }
        ffx::captureSpeech(false);
        const auto backendList=ffx::speechBackendChoices();
        require(backendList.size()==3 && backendList[0].name==L"Automatic" && backendList[1].name==L"NVDA" && backendList[2].name==L"SAPI","complete ordered fake backend list copied without truncation");
        submitted=CreateEventW(nullptr,TRUE,FALSE,nullptr);resume=CreateEventW(nullptr,TRUE,FALSE,nullptr);
        block(submitted,resume);ffx::output(L"Held backend metadata fixture",false,"mod-test");
        require(WaitForSingleObject(submitted,2000)==WAIT_OBJECT_0,"backend output deliberately held for metadata checks");
        finished=CreateEventW(nullptr,TRUE,FALSE,nullptr);bool copiedMetadata=false;
        std::thread metadataInput([&] {
            ffx::selectSpeechBackend(3);
            copiedMetadata=ffx::speechBackendChoices().size()==3 && !ffx::speechBackendStatus().empty();SetEvent(finished);
        });
        const bool metadataIndependent=WaitForSingleObject(finished,1000)==WAIT_OBJECT_0;
        SetEvent(resume);metadataInput.join();require(metadataIndependent && copiedMetadata,"metadata read and backend request cannot wait for a held SDK worker");
        block(nullptr,nullptr);
        auto awaitStatus=[&](const wchar_t* expected) {const auto until=GetTickCount64()+3000;while(ffx::speechBackendStatus()!=expected && GetTickCount64()<until)Sleep(1);require(ffx::speechBackendStatus()==expected,"copied backend status becomes current");};
        awaitStatus(L"SAPI");
        auto afterChoices=ffx::speechBackendChoices();require(afterChoices.size()==backendList.size(),"status update retains the full startup choice list");
        for(size_t i=0;i<afterChoices.size();++i)require(afterChoices[i].preference==backendList[i].preference && afterChoices[i].name==backendList[i].name,"backend list order/labels unchanged on selection");
        CloseHandle(submitted);CloseHandle(resume);CloseHandle(finished);
        // Exact lost-wake interval: Native request lands after the worker's
        // empty-state check and before its auto-reset event wait.
        submitted=CreateEventW(nullptr,TRUE,FALSE,nullptr);resume=CreateEventW(nullptr,TRUE,FALSE,nullptr);
        ffx::speechWaitResume=resume;ffx::speechWaitEntered=submitted;ffx::selectSpeechBackend(3);
        require(WaitForSingleObject(submitted,2000)==WAIT_OBJECT_0,"speech worker held outside queue lock before wait");
        ffx::selectSpeechBackend(0);SetEvent(resume);awaitStatus(L"NVDA");
        ffx::speechWaitEntered=nullptr;ffx::speechWaitResume=nullptr;
        // Keep the published fixture handles valid to process exit as well.
        require(ffx::speechBackendChoices().size()==3,"atomic wake preserves available backend metadata");
        puts("PASS: owned backend metadata, held-SDK request/read independence and exact pre-wait lost-wake regression.");
        submitted=CreateEventW(nullptr,TRUE,FALSE,nullptr);resume=CreateEventW(nullptr,TRUE,FALSE,nullptr);
        finished=CreateEventW(nullptr,TRUE,FALSE,nullptr);
        ffx::logHoldEntered=submitted;ffx::logHoldResume=resume;ffx::logCopiesDone=finished;
        ffx::recordLog("LOGFIXTURE.HOLD");
        require(WaitForSingleObject(submitted,2000)==WAIT_OBJECT_0,"logger worker held before FILE write");
        char copiedLog[64]="owned log source";
        ffx::recordLog("LOGFIXTURE.COPY %s",copiedLog);strcpy_s(copiedLog,"mutated after enqueue");
        ffx::recordLog("LOGFIXTURE.EQUAL");ffx::recordLog("LOGFIXTURE.EQUAL");ffx::recordLog("LOGFIXTURE.COPIES.DONE");
        SetEvent(resume);require(WaitForSingleObject(finished,2000)==WAIT_OBJECT_0,"owned log sentinels drain in accepted order");
        {
            std::lock_guard<std::mutex> lock(ffx::logFixtureMutex);
            require(ffx::logFixtureRows.size()==3 && ffx::logFixtureRows[0]=="LOGFIXTURE.COPY owned log source" &&
                ffx::logFixtureRows[1]=="LOGFIXTURE.EQUAL" && ffx::logFixtureRows[2]=="LOGFIXTURE.EQUAL",
                "logger owns copied source and preserves equal distinct events");
        }
        require(ffx::logFixtureOwner.load()!=GetCurrentThreadId(),"logger processing stays off the producer");
        ffx::logHoldEntered=nullptr;ffx::logHoldResume=nullptr;ffx::logCopiesDone=nullptr;
        // Deliberately fill the byte/event budgets while the logger cannot write.
        submitted=CreateEventW(nullptr,TRUE,FALSE,nullptr);resume=CreateEventW(nullptr,TRUE,FALSE,nullptr);
        finished=CreateEventW(nullptr,TRUE,FALSE,nullptr);ffx::logHoldEntered=submitted;ffx::logHoldResume=resume;
        ffx::recordLog("LOGFIXTURE.HOLD");require(WaitForSingleObject(submitted,2000)==WAIT_OBJECT_0,"pressure logger held");
        std::thread logProducer([&] {
            std::string large(4080,'Q');
            for(unsigned i=0;i<5000;++i)ffx::recordLog("LOGPRESSURE %u",i);
            for(unsigned i=0;i<2000;++i)ffx::recordLog("%s",large.c_str());
            SetEvent(finished);
        });
        const bool logIndependent=WaitForSingleObject(finished,1000)==WAIT_OBJECT_0;
        require(ffx::queuedLogBytesForTest()<=1024*1024 && ffx::droppedLogsForTest()>0,"event pressure retains bounded bytes and records rejections");
        SetEvent(resume);logProducer.join();require(logIndependent,"full logger budgets never wait for held disk worker");
        ffx::logHoldEntered=nullptr;ffx::logHoldResume=nullptr;
        // Auto-reset wake must retain a publication in the exact pre-wait gap.
        submitted=CreateEventW(nullptr,TRUE,FALSE,nullptr);resume=CreateEventW(nullptr,TRUE,FALSE,nullptr);
        finished=CreateEventW(nullptr,TRUE,FALSE,nullptr);
        ffx::logWaitEntered=submitted;ffx::logWaitResume=resume;ffx::logLateDone=finished;
        ffx::recordLog("LOGFIXTURE.WAKE");
        require(WaitForSingleObject(submitted,5000)==WAIT_OBJECT_0,"logger held in exact pre-wait interval");
        ffx::recordLog("LOGFIXTURE.LATE");SetEvent(resume);
        require(WaitForSingleObject(finished,2000)==WAIT_OBJECT_0,"logger publication wakes after pre-wait interval");
        ffx::logWaitEntered=nullptr;ffx::logWaitResume=nullptr;ffx::logLateDone=nullptr;
        submitted=CreateEventW(nullptr,TRUE,FALSE,nullptr);resume=CreateEventW(nullptr,TRUE,FALSE,nullptr);
        finished=CreateEventW(nullptr,TRUE,FALSE,nullptr);ffx::logHoldEntered=submitted;ffx::logHoldResume=resume;
        ffx::recordLog("LOGFIXTURE.HOLD");require(WaitForSingleObject(submitted,2000)==WAIT_OBJECT_0,"byte-budget logger held");
        std::thread byteProducer([&] {
            std::string large(4080,'R');for(unsigned i=0;i<2000;++i)ffx::recordLog("%s",large.c_str());SetEvent(finished);
        });
        const bool bytesIndependent=WaitForSingleObject(finished,1000)==WAIT_OBJECT_0;
        const auto queuedBytes=ffx::queuedLogBytesForTest();
        require(queuedBytes>1024*1024-4096 && queuedBytes<=1024*1024 && ffx::droppedLogsForTest()>0,"byte pressure caps the exact 1MB budget");
        SetEvent(resume);byteProducer.join();require(bytesIndependent,"byte-budget rejection never waits for held disk worker");
        ffx::logHoldEntered=nullptr;ffx::logHoldResume=nullptr;
        // Published fixture events remain alive through process exit.
        puts("PASS: owned equal log captures, held-disk pressure independence, logger ownership and exact pre-wait wake.");
        submitted=CreateEventW(nullptr,TRUE,FALSE,nullptr);resume=CreateEventW(nullptr,TRUE,FALSE,nullptr);
        finished=CreateEventW(nullptr,TRUE,FALSE,nullptr);
        ffx::speechq::captureHoldEntered=submitted;ffx::speechq::captureHoldResume=resume;
        ffx::output(L"Wake preparation fixture",false,"mod-test");
        require(WaitForSingleObject(submitted,2000)==WAIT_OBJECT_0,"preparation worker deliberately held");
        const int firstHeld=count();bool heldAccepted=false,heldGeneration=false;
        std::thread captureInput([&] {
            const auto before=ffx::generation();ffx::invalidate("held-preparation-resource");
            heldGeneration=ffx::generation()==before+1;ffx::interruptSpeechForInput();
            std::wstring owned=L"Held preparation snapshot";
            wchar_t raw[64]=L"Raw caption copied before reuse";
            heldAccepted=ffx::output(owned,false,"mod-test") && FfxNative_Output(raw,0)!=0;
            owned=L"changed after publish";wcscpy_s(raw,L"changed raw source");SetEvent(finished);
        });
        const bool captureIndependent=WaitForSingleObject(finished,1000)==WAIT_OBJECT_0;
        SetEvent(resume);captureInput.join();
        require(captureIndependent && heldAccepted && heldGeneration,"native output, input cancellation and resource invalidation never wait for preparation or SDK queues");
        const auto heldLimit=GetTickCount64()+5000;bool firstOwned=false,secondOwned=false;
        while(GetTickCount64()<heldLimit && (!firstOwned || !secondOwned)) {
            for(int i=firstHeld;i<count();++i) {
                record(i,text,256);
                if(!wcscmp(text,L"Held preparation snapshot")) firstOwned=true;
                if(!wcscmp(text,L"Raw caption copied before reuse")) secondOwned=true;
            }
            if(!firstOwned || !secondOwned) Sleep(1);
        }
        require(firstOwned && secondOwned,"held preparation consumes owned native/API snapshots after source mutation");
        ffx::speechq::captureHoldEntered=nullptr;ffx::speechq::captureHoldResume=nullptr;
        puts("PASS: held preparation worker cannot delay native capture, resource epoch publication or input cancellation; native/API source buffers remain owned.");
        {
            auto entered=CreateEventW(nullptr,TRUE,FALSE,nullptr),resume=CreateEventW(nullptr,TRUE,FALSE,nullptr);
            require(entered && resume,"domain fixture preparation events");
            ffx::speechq::registerDomain(4,ffx::processDomainFixture,ffx::resetDomainFixture);
            ffx::speechq::captureHoldEntered=entered;ffx::speechq::captureHoldResume=resume;
            require(ffx::output(L"hold domain preparation",false,"domain-fixture"),"wake domain preparation hold");
            require(WaitForSingleObject(entered,1000)==WAIT_OBJECT_0,"domain preparation held");
            ffx::DomainFixture data{29,0x534e4150};std::wstring text=L"Owned domain payload";
            const auto begin=GetTickCount64();
            require(ffx::speechq::captureDomain(4,data,ffx::generation(),text),"capture owned metadata plus ready text");
            require(ffx::speechq::captureDomain(4,data,ffx::generation(),text),"capture equal distinct domain event");
            data={0,0};text.assign(L"reused caller buffers");
            require(GetTickCount64()-begin<200,"domain capture never waits for preparation");
            ffx::speechq::captureHoldEntered=nullptr;ffx::speechq::captureHoldResume=nullptr;SetEvent(resume);
            const auto end=GetTickCount64()+1000;
            while(ffx::domainCopies<2 && GetTickCount64()<end) Sleep(1);
            require(ffx::domainCopies==2 && ffx::domainOwner!=GetCurrentThreadId(),"worker processes both owned metadata/text snapshots after source reuse");
            puts("PASS: ordered owned speech domains, equal distinct metadata/text, held preparation independence and worker-only processing.");
            ResetEvent(entered);ResetEvent(resume);
            ffx::speechq::captureHoldEntered=entered;ffx::speechq::captureHoldResume=resume;
            require(ffx::output(L"hold domain retirement",false,"domain-fixture"),"wake retirement hold");
            require(WaitForSingleObject(entered,1000)==WAIT_OBJECT_0,"retirement preparation held");
            unsigned accepted=0;
            for(unsigned i=0;i<2200;++i) if(ffx::speechq::captureDomain(4,data,ffx::generation())) ++accepted;
            require(accepted>1900 && accepted<=2048,"fill bounded metadata storage while worker held");
            ffx::invalidate("domain marker pressure");const auto resetEpoch=ffx::generation();
            ffx::speechq::captureHoldEntered=nullptr;ffx::speechq::captureHoldResume=nullptr;SetEvent(resume);
            const auto retiredBy=GetTickCount64()+2000;
            while(ffx::domainResetEpoch!=resetEpoch && GetTickCount64()<retiredBy) Sleep(1);
            require(ffx::domainResetEpoch==resetEpoch,"owned domains retire despite rejected full-queue generation marker");
            puts("PASS: full capture pressure cannot retain old-generation audio domain state after reset.");
        }
        puts("Native runtime tests passed: copied speech, repeated events, bounded queues, nonblocking input cancellation, one Prism worker, ordered current-action details and paused reel-choice highlights.");
        return 0;
    } catch (const std::exception& error) { fprintf(stderr, "%s\n", error.what()); return 1; }
}
