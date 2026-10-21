#include "../copied_text.hpp"
#include "../settings.hpp"
#include <atomic>
#include <mutex>
#include <cstdio>
#include <cstdlib>
#include <cstring>
namespace {
std::atomic<DWORD> nativeOwner{0};
std::atomic<unsigned> allocations{0},wrongOwner{0},outputs{0};
std::atomic<bool> held{true};
HANDLE entered=nullptr,releaseWorker=nullptr;
std::mutex resultMutex;
std::vector<std::wstring> result;
void check(bool value,const char* why) {if(!value) {std::fprintf(stderr,"FAIL: %s\n",why);std::exit(1);}}
void waitOutputs(unsigned count) {
    const auto limit=GetTickCount64()+3000;
    while(outputs.load()<count && GetTickCount64()<limit) Sleep(1);
    check(outputs.load()>=count,"worker completes accepted requests");
}
void encode(ffx::CopiedHelpPacket& p,const wchar_t* text) {
    constexpr wchar_t alphabet[]=L"0123456789 !\"#$%&'()*+,-./:;<=>?ABCDEFGHIJKLMNOPQRSTUVWXYZ[\\]^_\u2018abcdefghijklmnopqrstuvwxyz";
    p.text={};p.length=0;
    while(*text) {
        const auto c=std::wcschr(alphabet,*text++);check(c!=nullptr,"fixture glyph mapping");
        p.text[p.length++]=static_cast<uint8_t>(0x30+c-alphabet);
    }
    p.text[p.length++]=0;
}
}
void* operator new(size_t n) {if(nativeOwner==GetCurrentThreadId()) ++allocations;if(auto p=std::malloc(n?n:1)) return p;throw std::bad_alloc();}
void* operator new[](size_t n) {return ::operator new(n);}
void operator delete(void* p) noexcept {std::free(p);}
void operator delete[](void* p) noexcept {std::free(p);}
void operator delete(void* p,size_t) noexcept {std::free(p);}
void operator delete[](void* p,size_t) noexcept {std::free(p);}
namespace ffx {
unsigned generation() {return 1;}
int setting(Setting) {return 0;}
int textEncodingSnapshot() {return 0;}
void prepareCopiedControlNames() {}
void prepareCopiedControlSnapshot(CopiedControls& p) {p={};}
OwnedTextContext copiedControlContext(const CopiedControls&) {
    if(nativeOwner==GetCurrentThreadId()) ++wrongOwner;
    if(held.load()) {SetEvent(entered);WaitForSingleObject(releaseWorker,INFINITE);}
    return {};
}
std::wstring join(const std::vector<std::wstring>& parts,const wchar_t* separator) {
    std::wstring text;
    for(const auto& part:parts) {if(!text.empty()) text+=separator;text+=part;}
    return text;
}
bool outputAt(const std::wstring& text,bool,const char*,unsigned) {
    if(nativeOwner==GetCurrentThreadId()) ++wrongOwner;
    {std::lock_guard<std::mutex> lock(resultMutex);result.push_back(text);}
    ++outputs;return true;
}
}
int main() {
    entered=CreateEventW(nullptr,TRUE,FALSE,nullptr);releaseWorker=CreateEventW(nullptr,TRUE,FALSE,nullptr);
    check(entered && releaseWorker,"fixture barriers");
    check(ffx::startCopiedText(GetModuleHandleW(nullptr)),"synthetic copied-text owner starts");
    nativeOwner=GetCurrentThreadId();
    ffx::CopiedHelpPacket p;ffx::prepareCopiedHelp(p);p.batch=1;
    check(ffx::commitCopiedHelp(p),"begin accepted");p.kind=ffx::CopiedHelpKind::Part;encode(p,L"Alpha");
    check(ffx::commitCopiedHelp(p),"first rendered part accepted");
    check(WaitForSingleObject(entered,3000)==WAIT_OBJECT_0,"worker decoding held");
    encode(p,L"Beta");check(ffx::commitCopiedHelp(p),"native second part independent of held decoder");
    p.text.fill(0);p.kind=ffx::CopiedHelpKind::Complete;p.parts=2;
    check(ffx::commitCopiedHelp(p) && ffx::readCopiedTextDetail(),"native completion and manual read remain nonblocking");
    p.kind=ffx::CopiedHelpKind::Begin;p.batch=999;
    unsigned accepted=0;for(unsigned i=0;i<1024 && ffx::commitCopiedHelp(p);++i) ++accepted;
    check(accepted==1021,"exact1024 physical capacity includes three ready requests");
    check(allocations.load()==0,"native publication/rejection allocates nothing");
    held=false;SetEvent(releaseWorker);waitOutputs(2);
    {std::lock_guard<std::mutex> lock(resultMutex);check(result[0]==L"Alpha Beta" && result[1]==L"Alpha Beta","owned caller reuse and manual FIFO preserve the complete page");}
    // A read after the pressure burst acknowledges all preceding records.
    const auto limit=GetTickCount64()+3000;
    while(!ffx::commitCopiedHelp(ffx::CopiedHelpPacket{p}) && GetTickCount64()<limit) Sleep(1);
    check(ffx::readCopiedTextDetail(),"detail remains readable after pressure");waitOutputs(3);
    ResetEvent(entered);ResetEvent(releaseWorker);held=true;
    ffx::prepareCopiedHelp(p);p.batch=2;p.kind=ffx::CopiedHelpKind::Begin;check(ffx::commitCopiedHelp(p),"new page begins");
    p.kind=ffx::CopiedHelpKind::Part;encode(p,L"Cancelled");check(ffx::commitCopiedHelp(p),"new part queued");
    check(WaitForSingleObject(entered,3000)==WAIT_OBJECT_0,"second held decoder");
    p.kind=ffx::CopiedHelpKind::Complete;p.parts=1;check(ffx::commitCopiedHelp(p),"cancelled completion captured");
    ffx::resetCopiedTextDetail();held=false;SetEvent(releaseWorker);Sleep(30);
    check(outputs.load()==3 && wrongOwner.load()==0 && allocations.load()==0,"reset cancels the page; all decoding/output belongs to worker");
    ResetEvent(entered);ResetEvent(releaseWorker);held=true;
    ffx::prepareCopiedHelp(p);p.batch=3;p.kind=ffx::CopiedHelpKind::Begin;check(ffx::commitCopiedHelp(p),"warning barrier page");
    p.kind=ffx::CopiedHelpKind::Part;encode(p,L"Barrier");check(ffx::commitCopiedHelp(p),"warning barrier part");
    check(WaitForSingleObject(entered,3000)==WAIT_OBJECT_0,"warning decoder held");
    ffx::CopiedTextPacket warning;warning.kind=ffx::CopiedTextKind::IntroWarning;warning.epoch=1;
    const char* ready="Autosave fixture\nSave donn\xc3\xa9" "es";
    warning.length[0]=static_cast<unsigned>(std::strlen(ready));std::memcpy(warning.text[0].data(),ready,warning.length[0]);
    check(ffx::commitCopiedText(warning) && ffx::commitCopiedText(warning),"two distinct warnings captured without native decoding");
    warning.text[0].fill(0);ffx::resetCopiedTextDetail();
    check(allocations.load()==0,"warning copy/publication/reset has no native allocation");
    held=false;SetEvent(releaseWorker);waitOutputs(5);
    {std::lock_guard<std::mutex> lock(resultMutex);check(result[3]==L"Autosave fixture\nSave donn\u00e9es" && result[4]==result[3],"warning UTF8/newlines survive source reuse, selection reset and equal distinct delivery");}
    check(wrongOwner.load()==0 && allocations.load()==0,"warning formatting/output stays on actual copied-text worker");
    puts("PASS: copied Help/autosave owner, held-decoder independence, zero producer heap, FIFO/source reuse, UTF8, exact capacity and retirement.");
    return 0;
}
