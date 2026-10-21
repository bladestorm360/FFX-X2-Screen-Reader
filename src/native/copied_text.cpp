#include "copied_text.hpp"
#include "events.hpp"
#include "settings.hpp"
#include "input_device.hpp"
#include "owned_text_decoder.hpp"
#include "nonblocking_snapshots.hpp"
#include "process_storage.hpp"
#include "ready_text.hpp"
#include <atomic>
namespace ffx {
namespace {
async::TryCaptureQueue<CopiedTextPacket,32> captures;
async::TryCaptureQueue<CopiedHelpPacket,1024> helpCaptures;
std::atomic<unsigned> revision{1},detailRevision{0},detailEpoch{0},dropped{0};
std::atomic<unsigned> helpSerial{1};
std::atomic<bool> helpDetail{false};
HANDLE wake=nullptr;
struct WorkerText {
    std::wstring detail;
    std::vector<std::wstring> help;
    unsigned revision=0,epoch=0,serial=0,batch=0,parts=0;
};
ProcessStorage<WorkerText> storage;
WorkerText& state() noexcept {return storage.get();}
void process(const CopiedTextPacket& p) {
    if(p.epoch!=generation() || (p.kind!=CopiedTextKind::IntroWarning && p.revision!=revision.load())) return;
    if(p.kind==CopiedTextKind::IntroWarning) {
        if(p.length[0] && p.length[0]<=1023)
            outputAt(decodeReadyAscii(p.text[0].data(),p.length[0]),true,"autosave-warning",p.epoch);
        return;
    }
    if(p.kind==CopiedTextKind::ReadDetail) {
        if(state().revision==p.revision && state().epoch==p.epoch)
            outputAt(state().detail.empty()?L"No displayed description is available.":state().detail,true,"selection-description-hotkey",p.epoch);
        return;
    }
    if(p.kind==CopiedTextKind::IntroControls) {
        outputAt(L"Press B or Menu/Start for the title menu, or let the intro play.",true,"mod-intro",p.epoch);return;
    }
    const auto c=copiedControlContext(p);
    std::array<std::wstring,3> lines;
    for(unsigned i=0;i<3;++i) if(p.length[i] && p.length[i]<=p.text[i].size())
        lines[i]=join(decodeOwnedBytes(p.text[i].data(),p.length[i],OwnedTextMode::Terminated,c),L" ");
    if(p.epoch!=generation() || p.revision!=revision.load()) return;
    std::wstring headline=lines[0];
    if(!lines[1].empty()) { if(!headline.empty()) headline+=L". "; headline+=lines[1]; }
    if(!headline.empty()) outputAt(headline,p.interrupt,"monster-arena",p.epoch);
    state().detail=lines[2]; state().revision=p.revision; state().epoch=p.epoch;
    if(p.automatic && !state().detail.empty()) outputAt(state().detail,false,"selection-description",p.epoch);
}
void processHelp(const CopiedHelpPacket& p) {
    if(p.epoch!=generation() || p.revision!=revision.load() || p.serial!=helpSerial.load()) return;
    auto& s=state();
    if(p.kind==CopiedHelpKind::ReadDetail) {
        if(s.revision==p.revision && s.epoch==p.epoch)
            outputAt(s.detail.empty()?L"No displayed description is available.":s.detail,true,"selection-description-hotkey",p.epoch);
        return;
    }
    if(p.kind==CopiedHelpKind::Begin) {
        s.help.clear();s.parts=0;s.serial=p.serial;s.batch=p.batch;return;
    }
    if(s.serial!=p.serial || s.batch!=p.batch) return;
    if(p.kind==CopiedHelpKind::Part) {
        if(s.parts>=512 || !p.length || p.length>p.text.size()) return;
        ++s.parts;
        auto line=join(decodeOwnedBytes(p.text.data(),p.length,OwnedTextMode::Terminated,copiedControlContext(p)),L" ");
        if(!line.empty()) s.help.push_back(std::move(line));
        return;
    }
    if(p.kind!=CopiedHelpKind::Complete || p.parts!=s.parts) return;
    auto page=join(s.help,L" ");s.help.clear();s.batch=0;
    if(p.epoch!=generation() || p.revision!=revision.load() || p.serial!=helpSerial.load()) return;
    // An accepted empty native page has no undisplayed fallback narration.
    if(page.empty()) return;
    s.detail=page;s.revision=p.revision;s.epoch=p.epoch;
    outputAt(page,true,"help-page",p.epoch);
}
void drain() {
    CopiedTextPacket p;while(captures.tryPop(p)) process(p);
    CopiedHelpPacket h;while(helpCaptures.tryPop(h)) processHelp(h);
    auto& s=state();
    if(s.revision!=revision.load() || s.epoch!=generation()) {s.detail.clear();s.revision=0;s.epoch=0;}
    if(s.serial!=helpSerial.load()) {s.help.clear();s.serial=0;s.batch=0;s.parts=0;}
}
DWORD WINAPI worker(void*) {
    for(;;) {
        drain();
        if(const auto lost=dropped.exchange(0)) recordLog("copied text backpressure rejected=%u",lost);
        WaitForSingleObject(wake,INFINITE); // Dedicated worker; native producers never wait.
    }
}
}
void prepareCopiedControls(CopiedTextPacket& p) {
    p.epoch=generation(); p.revision=revision.load();
    prepareCopiedControlSnapshot(p);
    p.automatic=setting(Setting::AutoDetail)!=0;
}
void prepareCopiedText(CopiedTextPacket& p) {prepareCopiedControls(p);p.polish=textEncodingSnapshot()==1;}
bool commitCopiedText(const CopiedTextPacket& p) {
    if(p.epoch!=generation() || (p.kind!=CopiedTextKind::IntroWarning && p.revision!=revision.load())) return false;
    if(!captures.tryPush(p)) { ++dropped; return false; }
    if(p.kind==CopiedTextKind::Arena) {helpDetail=false;detailEpoch=p.epoch;detailRevision=p.revision;}
    if(wake) SetEvent(wake);
    return true;
}
void resetCopiedTextDetail() {++revision;detailRevision=0;detailEpoch=0;if(wake) SetEvent(wake);}
unsigned resetCopiedHelpCapture() {
    const auto next=helpSerial.fetch_add(1)+1;if(wake) SetEvent(wake);return next;
}
void prepareCopiedHelp(CopiedHelpPacket& p) {
    p.epoch=generation();p.revision=revision.load();p.serial=helpSerial.load();prepareCopiedControlSnapshot(p);
}
bool commitCopiedHelp(const CopiedHelpPacket& p) {
    if(p.epoch!=generation() || p.revision!=revision.load() || p.serial!=helpSerial.load()) return false;
    if(!helpCaptures.tryPush(p)) {++dropped;return false;}
    if(p.kind==CopiedHelpKind::Complete && p.parts) {helpDetail=true;detailEpoch=p.epoch;detailRevision=p.revision;}
    if(wake) SetEvent(wake);return true;
}
bool readCopiedTextDetail() {
    const auto current=revision.load();
    if(detailEpoch!=generation() || detailRevision!=current) return false;
    if(helpDetail.load()) {
        CopiedHelpPacket p;prepareCopiedHelp(p);p.kind=CopiedHelpKind::ReadDetail;
        commitCopiedHelp(p);return true;
    }
    CopiedTextPacket p; p.kind=CopiedTextKind::ReadDetail; p.epoch=generation(); p.revision=current;
    commitCopiedText(p); return true; // A rejected request never falls through to stale native detail.
}
bool startCopiedText(HMODULE module) {
    HMODULE pinned=nullptr;
    if(!GetModuleHandleExW(GET_MODULE_HANDLE_EX_FLAG_FROM_ADDRESS|GET_MODULE_HANDLE_EX_FLAG_PIN,
        reinterpret_cast<LPCWSTR>(&startCopiedText),&pinned)) return false;
    (void)module;prepareCopiedControlNames();storage.prepare();
    wake=CreateEventW(nullptr,FALSE,FALSE,nullptr); if(!wake) return false;
    const auto thread=CreateThread(nullptr,0,worker,nullptr,0,nullptr);
    if(!thread) { CloseHandle(wake); wake=nullptr; return false; }
    CloseHandle(thread); return true;
}
#ifdef FFX_READER_TEST
void drainCopiedTextForTest() {prepareCopiedControlNames();storage.prepare();drain();}
#endif
}
