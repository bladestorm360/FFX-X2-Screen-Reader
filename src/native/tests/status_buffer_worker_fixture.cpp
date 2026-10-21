#include "../events.hpp"
#include "../ready_game_text.hpp"
#include "../selection_details.hpp"
#include "../settings.hpp"
#include "../speech_queue.hpp"
#include "../sphere_grid.hpp"
#include "../status_review.hpp"
#include "../text_plan.hpp"
#include <atomic>
#include <cstdio>
#include <cstdlib>
#include <cstring>

namespace {
thread_local bool nativeOwner=false;
std::atomic<unsigned> nativeAllocations{0},nativeFrees{0};
std::atomic<bool> holdWorker{true};
HANDLE entered=nullptr,resume=nullptr,sinkSignal=nullptr;
uint8_t records[64][96]{};
struct Hook {unsigned rva;ffx::Handler enter,leave;};
struct Key {unsigned vk;void(*callback)(bool);bool(*available)();};
Hook hooks[8]{};unsigned hookCount=0;
Key keys[8]{};unsigned keyCount=0;
int autoDetailSetting=0;
void require(bool ok,const char* why) {if(!ok) {std::fprintf(stderr,"FAIL: %s\n",why);std::exit(1);}}
void encode(uint8_t* out,const wchar_t* value) {
    constexpr wchar_t alphabet[]=L"0123456789 !\"#$%&'()*+,-./:;<=>?ABCDEFGHIJKLMNOPQRSTUVWXYZ[\\]^_\u2018abcdefghijklmnopqrstuvwxyz";
    while(*value) {const auto item=std::wcschr(alphabet,*value++);require(item!=nullptr,"fixture glyph");*out++=static_cast<uint8_t>(0x30+item-alphabet);}*out=0;
}
void held() {require(WaitForSingleObject(entered,3000)==WAIT_OBJECT_0,"worker held before drain");}
void release() {nativeOwner=false;holdWorker=false;SetEvent(resume);}
void nativeMoveDown() {
    for(unsigned i=0;i<keyCount;++i) if(keys[i].vk==VK_DOWN) {
        require(keys[i].available && keys[i].available(),"Native Down availability uses current context");
        keys[i].callback(false);return;
    }
    require(false,"Native Down registered");
}
void nativeMoveUp() {
    for(unsigned i=0;i<keyCount;++i) if(keys[i].vk==VK_UP) {
        require(keys[i].available && keys[i].available(),"Native Up availability uses current context");
        keys[i].callback(false);return;
    }
    require(false,"Native Up registered");
}
std::wstring take() {
    const auto deadline=GetTickCount64()+3000;ffx::speechq::Utterance result;
    while(!ffx::speechq::take(result)) {const auto now=GetTickCount64();if(now>=deadline)break;WaitForSingleObject(sinkSignal,static_cast<DWORD>(deadline-now));}
    require(!result.text.empty(),"bounded speech sink receives output");return result.text;
}
void drainAvailable() {ffx::speechq::Utterance result;while(ffx::speechq::take(result)) {}}
void appendGridPage(unsigned character,int page,const wchar_t* title) {
    auto batch=ffx::beginStatusRows(character,page,true,true);
    ffx::TextPlan heading;ffx::prepareTextPlan(heading,"status-buffer-title",false);
    ffx::addStatusRow(batch,heading,ffx::planLiteral(heading,title));
    for(unsigned i=0;i<64;++i) {
        ffx::TextPlan row;ffx::prepareTextPlan(row,"status-buffer-row",false);
        const auto name=ffx::planEncoded(row,reinterpret_cast<uintptr_t>(records[i]));
        const auto detail=ffx::planLiteral(row,L"Ability detail");
        ffx::addStatusRow(batch,row,name,detail,true);
    }
    require(batch.count==65,"complete page has title and 64 visible rows");
    require(ffx::finishStatusRows(batch),"complete grid page accepted");
}
void rejectOverCapacityPage() {
    auto batch=ffx::beginStatusRows(0,9,true,true);
    for(unsigned i=0;i<66;++i) {
        ffx::TextPlan row;ffx::prepareTextPlan(row,"status-buffer-overflow",false);
        ffx::addStatusRow(batch,row,ffx::planLiteral(row,L"Overflow row"));
    }
    require(!batch.valid&&batch.count==65,"66th row invalidates the entire fixed-capacity batch");
    require(!ffx::finishStatusRows(batch),"over-capacity batch rejects as a whole");
    require(!ffx::statusReviewAvailable(),"rejected batch publishes no partial page context");
}
}

void* operator new(size_t n) {if(nativeOwner)++nativeAllocations;if(auto p=std::malloc(n?n:1))return p;throw std::bad_alloc();}
void* operator new[](size_t n) {return ::operator new(n);}
void operator delete(void* p) noexcept {if(nativeOwner&&p)++nativeFrees;std::free(p);}
void operator delete[](void* p) noexcept {::operator delete(p);}
void operator delete(void* p,size_t) noexcept {::operator delete(p);}
void operator delete[](void* p,size_t) noexcept {::operator delete(p);}

namespace ffx {
std::atomic<int> scene{6};
void resetStatusReview();
unsigned generation() {return speechq::generation();}
int setting(Setting s) {return s==Setting::Speech?1:s==Setting::AutoDetail?autoDetailSetting:0;}
bool sphereGridOpen() {require(nativeOwner,"grid-open query stays on Native owner");return true;}
bool helpMenuContext() {require(nativeOwner,"Help context stays on Native owner");return true;}
bool battleResultsReviewActive() {require(nativeOwner,"reward context stays on Native owner");return true;}
void refreshBattleResultReview() {require(nativeOwner,"reward refresh stays on Native owner");}
bool escMenuActive() {require(nativeOwner,"Escape context stays on Native owner");return false;}
uint8_t equippedItem(unsigned,bool) {require(false,"status fixture does not request equipment");return 255;}
bool hook(unsigned rva,Handler enter,Handler leave) {
    require(nativeOwner,"fixed hook registration boundary stays on Native owner");require(hookCount<8,"fixed hook registry capacity");hooks[hookCount++]={rva,enter,leave};return true;
}
void hotkeyWhen(unsigned vk,void(*callback)(bool),bool(*available)()) {
    require(nativeOwner,"fixed key registration boundary stays on Native owner");require(keyCount<8,"fixed key registry capacity");keys[keyCount++]={vk,callback,available};
}
void installGridStatus() {require(nativeOwner,"Grid Status install stub runs at bootstrap");}
uintptr_t address(unsigned rva) {
    require(nativeOwner,"engine getter address requested on Native owner");
    if(rva==0x3909c0 || rva==0x39a4c0) return reinterpret_cast<uintptr_t>(+[](unsigned id,uintptr_t* base)->uintptr_t {
        require(nativeOwner,"record getter stays on Native owner");if(id>=64)return 0;if(base)*base=reinterpret_cast<uintptr_t>(records[id]);return reinterpret_cast<uintptr_t>(records[id]);
    });
    require(false,"only record-name getter is used");return 0;
}
bool copy(uintptr_t from,void* to,size_t n) {require(nativeOwner,"native bytes copied before returning to caller");std::memcpy(to,reinterpret_cast<const void*>(from),n);return true;}
void prepareCopiedControlSnapshot(CopiedControls& p) {p={};}
OwnedTextContext copiedControlContext(const CopiedControls&) {require(!nativeOwner,"ready text decodes on preparation worker");return {};}
std::wstring join(const std::vector<std::wstring>& parts,const wchar_t* separator) {
    require(!nativeOwner,"status page joins on preparation worker");std::wstring out;
    for(const auto& part:parts)if(!part.empty()){if(!out.empty())out+=separator;out+=part;}return out;
}
bool readCopiedTextDetail() {return false;}
void resetCopiedTextDetail() {}
namespace speechq {void captureBeforeDrainForTest() {if(holdWorker.load()){SetEvent(entered);WaitForSingleObject(resume,INFINITE);}}}
}

int main() {
    for(unsigned i=0;i<64;++i) {wchar_t name[32]{};swprintf_s(name,L"Ability %02u",i+1);encode(records[i],name);}
    entered=CreateEventW(nullptr,TRUE,FALSE,nullptr);resume=CreateEventW(nullptr,TRUE,FALSE,nullptr);sinkSignal=CreateEventW(nullptr,FALSE,FALSE,nullptr);
    require(entered&&resume&&sinkSignal,"worker barrier and sink handles");
    ffx::installSelectionDetails();ffx::installTextPlans();
    require(ffx::setting(ffx::Setting::Speech)==1&&ffx::setting(ffx::Setting::AutoDetail)==0,"Speech On and AutoDetail Off fixture defaults");
    nativeOwner=true;ffx::installStatusReview();nativeOwner=false;
    require(hookCount==4&&keyCount==6,"status review installs actual Native callbacks and keys");
    require(ffx::speechq::start(GetModuleHandleW(nullptr),sinkSignal),"speech preparation starts with synthetic backend");held();

    LARGE_INTEGER frequency{};QueryPerformanceFrequency(&frequency);LARGE_INTEGER start{},captured{};
    nativeOwner=true;rejectOverCapacityPage();nativeOwner=false;
    QueryPerformanceCounter(&start);
    nativeOwner=true;appendGridPage(0,1,L"Tidus. Skills");nativeOwner=false;QueryPerformanceCounter(&captured);
    nativeOwner=true;nativeMoveDown();require(ffx::readSelectionDetail(),"manual read accepted while real status worker is held");nativeOwner=false;
    for(auto& record:records)std::memset(record,0,sizeof(record));
    require(nativeAllocations.load()==0&&nativeFrees.load()==0,"Native page capture, installed Down and manual read perform no heap allocation or free");
    release();
    const auto title=take();require(title==L"Tidus. Skills","grid title survives encoded source reuse");
    LARGE_INTEGER first{};QueryPerformanceCounter(&first);
    require(take()==L"Ability 01","Native Down reads original selected encoded row after source overwrite");
    require(take()==L"Ability detail","manual selected-row detail survives source overwrite and FIFO");

    ResetEvent(entered);ResetEvent(resume);holdWorker=true;nativeOwner=true;
    for(unsigned i=0;i<64;++i) {wchar_t name[32]{};swprintf_s(name,L"Ability %02u",i+1);encode(records[i],name);}
    std::memcpy(records[1],records[0],sizeof(records[0]));
    appendGridPage(0,2,L"Page A");appendGridPage(0,3,L"Page B");
    nativeMoveDown();nativeMoveDown();require(ffx::readSelectionDetail(),"manual request captures the replacement page context");held();
    ffx::resetStatusReview();require(!ffx::statusReviewAvailable(),"actual resetStatusReview closes current context immediately while worker is held");
    appendGridPage(0,4,L"Fresh page");nativeMoveDown();require(ffx::readSelectionDetail(),"fresh post-reset manual request is accepted in its own context");nativeOwner=false;
    require(nativeAllocations.load()==0&&nativeFrees.load()==0,"all held status capture, replace, reset and manual requests remain native-heap-free");
    release();
    require(take()==L"Page A"&&take()==L"Page B"&&take()==L"Ability 01"&&take()==L"Ability 01",
        "complete replaced pages and equal row/review events retain original FIFO");
    // The old manual request's token was superseded by the fresh native page.
    // Selection-detail's established stale-token guard must reject that read.
    require(take()==L"Fresh page"&&take()==L"Ability 01"&&take()==L"Ability detail","post-reset read uses fresh page context with no stale output");
    drainAvailable();
    require(!ffx::speechq::pending(),"all status and manual commands drain without stale queued output");

    // Same production receiver, current Help cancellation guard and large page.
    ResetEvent(entered);ResetEvent(resume);holdWorker=true;
    nativeOwner=true;
    auto help=ffx::beginStatusRows(0,1,false,true);help.help=true;help.present=true;help.nativeArrows=1;help.focusRow=~0u;
    for(unsigned i=0;i<128;++i) {
        ffx::TextPlan row;ffx::prepareTextPlan(row,"help-large",false);
        row.context[2]=10;row.context[3]=(127-i)*3;row.context[4]=1;
        ffx::addStatusRow(help,row,ffx::planLiteral(row,i==127?L"Heading":L"Visible Help paragraph"));
    }
    require(ffx::finishStatusRows(help),"128-line Help page accepted with real worker held");
    for(unsigned i=0;i<keyCount;++i) if(keys[i].vk==VK_DOWN) require(!keys[i].available(),"native Help menu keeps its directional navigation");
    require(ffx::readSelectionDetail(),"Help manual page read accepted while held");nativeOwner=false;held();release();
    require(take()==L"Heading","Help headline follows visible y/x order on the worker");
    const auto helpText=take();require(helpText.find(L"Heading\nVisible Help paragraph")==0 && helpText.size()==7+127*22+127,"Help I owns visible order and every repeated paragraph without text deduplication");

    // Help content arrows use only the buffered row while I keeps the complete page.
    // Hold preparation across open/Down; the same page owns the newer arrow
    // detail token. Later manual I must still obey its current-selection guard.
    ResetEvent(entered);ResetEvent(resume);holdWorker=true;autoDetailSetting=1;nativeOwner=true;
    auto helpContent=ffx::beginStatusRows(0,3,false,true);helpContent.help=true;helpContent.present=true;helpContent.nativeArrows=0;
    ffx::TextPlan helpHeading;ffx::prepareTextPlan(helpHeading,"help-content",false);
    helpHeading.context[2]=0;helpHeading.context[3]=0;helpHeading.context[4]=1;
    ffx::addStatusRow(helpContent,helpHeading,ffx::planLiteral(helpHeading,L"Help heading"));
    ffx::TextPlan helpParagraph1;ffx::prepareTextPlan(helpParagraph1,"help-content",false);
    helpParagraph1.context[2]=0;helpParagraph1.context[3]=10;helpParagraph1.context[4]=1;
    ffx::addStatusRow(helpContent,helpParagraph1,ffx::planLiteral(helpParagraph1,L"Paragraph one"));
    ffx::TextPlan helpParagraph2;ffx::prepareTextPlan(helpParagraph2,"help-content",false);
    helpParagraph2.context[2]=0;helpParagraph2.context[3]=20;helpParagraph2.context[4]=1;
    ffx::addStatusRow(helpContent,helpParagraph2,ffx::planLiteral(helpParagraph2,L"Paragraph two"));
    require(ffx::finishStatusRows(helpContent),"three-row Help content page accepted with AutoDetail On");
    nativeMoveDown();nativeOwner=false;held();
    require(nativeAllocations.load()==0&&nativeFrees.load()==0,"Help open, Down, I and Up remain native-heap-free while the worker is held");
    release();
    require(take()==L"Help heading"&&take()==L"Paragraph one\nParagraph two",
        "AutoDetail opening speaks Help heading and both paragraphs");
    require(take()==L"Paragraph one","Help Down speaks only the selected paragraph");
    nativeOwner=true;require(ffx::readSelectionDetail(),"Help I accepted after buffered Down");nativeOwner=false;
    require(take()==L"Help heading\nParagraph one\nParagraph two","Help I after Down reads the complete page");
    ResetEvent(entered);ResetEvent(resume);holdWorker=true;nativeOwner=true;nativeMoveUp();nativeOwner=false;held();release();
    require(take()==L"Help heading","Help Up back to heading does not replay full-page AutoDetail");
    drainAvailable();require(!ffx::speechq::pending(),"Help arrow/detail sequence drains without replay output");
    autoDetailSetting=0;require(ffx::setting(ffx::Setting::AutoDetail)==0,"fixture restores AutoDetail Off for subsequent cases");

    // Multiline Help rows split into individual arrow rows. The controls footer
    // stays available to Shift+I but is excluded from opening, arrows and I.
    ResetEvent(entered);ResetEvent(resume);holdWorker=true;autoDetailSetting=1;nativeOwner=true;
    auto helpLines=ffx::beginStatusRows(0,5,false,true);helpLines.help=true;helpLines.present=true;helpLines.nativeArrows=0;
    ffx::TextPlan lineHeading;ffx::prepareTextPlan(lineHeading,"help-lines",false);
    lineHeading.context[2]=0;lineHeading.context[3]=0;lineHeading.context[4]=1;
    ffx::addStatusRow(helpLines,lineHeading,ffx::planLiteral(lineHeading,L"Help heading"));
    ffx::TextPlan multiline;ffx::prepareTextPlan(multiline,"help-lines",false);
    multiline.context[2]=0;multiline.context[3]=10;multiline.context[4]=1;
    ffx::addStatusRow(helpLines,multiline,ffx::planLiteral(multiline,L"Line one\nLine two"));
    ffx::TextPlan footer;ffx::prepareTextPlan(footer,"help-lines",false);footer.context[5]=ffx::HelpControls;
    ffx::addStatusRow(helpLines,footer,ffx::planLiteral(footer,L"X: Exit"));
    require(ffx::finishStatusRows(helpLines),"multiline Help page with controls footer accepted");
    nativeMoveDown();require(ffx::readHelpControls(),"Shift+I controls read accepted while the real worker is held");
    require(ffx::readSelectionDetail(),"I full-page read accepted while the real worker is held");
    nativeOwner=false;held();
    require(nativeAllocations.load()==0&&nativeFrees.load()==0,"multiline Help open, Down, Shift+I and I remain native-heap-free while held");
    release();
    require(take()==L"Help heading"&&take()==L"Line one\nLine two","Help opening and AutoDetail exclude controls and retain full-page newline separators");
    require(take()==L"Line one","Help arrow reads the first split line, excluding controls");
    require(take()==L"X: Exit","Shift+I reads the captured controls footer");
    require(take()==L"Help heading\nLine one\nLine two","I reads the entire Help page with multiline row split and controls excluded");
    drainAvailable();require(!ffx::speechq::pending(),"multiline Help commands drain without footer or stale output");
    autoDetailSetting=0;

    // Missing footer reports the exact native boundary fallback.
    ResetEvent(entered);ResetEvent(resume);holdWorker=true;nativeOwner=true;
    auto noControls=ffx::beginStatusRows(0,6,false,true);noControls.help=true;noControls.present=true;
    ffx::TextPlan noControlsText;ffx::prepareTextPlan(noControlsText,"help-no-controls",false);
    ffx::addStatusRow(noControls,noControlsText,ffx::planLiteral(noControlsText,L"Only Help row"));
    require(ffx::finishStatusRows(noControls)&&ffx::readHelpControls(),"Help controls fallback read accepted while held");
    nativeOwner=false;held();release();
    require(take()==L"Only Help row"&&take()==L"No controls displayed.","missing Help controls footer emits exact fallback");
    drainAvailable();

    ResetEvent(entered);ResetEvent(resume);holdWorker=true;nativeOwner=true;
    auto cancelled=ffx::beginStatusRows(0,2,false,true);cancelled.help=true;cancelled.present=true;
    ffx::TextPlan body;ffx::prepareTextPlan(body,"help-cancel",false);
    ffx::addStatusRow(cancelled,body,ffx::planLiteral(body,L"Retired Help page"));
    ffx::TextPlan cancelledFooter;ffx::prepareTextPlan(cancelledFooter,"help-cancel",false);cancelledFooter.context[5]=ffx::HelpControls;
    ffx::addStatusRow(cancelled,cancelledFooter,ffx::planLiteral(cancelledFooter,L"X: Exit"));
    require(ffx::finishStatusRows(cancelled)&&ffx::readHelpControls(),"captured Help page and controls read can wait behind worker");held();
    ffx::closeHelpReview();ffx::resetSelectionDetail();nativeOwner=false;release();
    Sleep(30);ffx::speechq::Utterance stale;require(!ffx::speechq::take(stale),"closed Help cannot emit its pending page");

    // Quiet battle refresh replaces values without replaying the page and keeps
    // the existing row cursor in the real ordered worker protocol.
    nativeOwner=true;auto reward=ffx::beginStatusRows(0,0,false,true);reward.battle=reward.present=true;
    ffx::TextPlan rewardTitle;ffx::prepareTextPlan(rewardTitle,"reward-page",false);ffx::addStatusRow(reward,rewardTitle,ffx::planLiteral(rewardTitle,L"AP"));
    ffx::TextPlan oldValue;ffx::prepareTextPlan(oldValue,"reward-value",false);ffx::addStatusRow(reward,oldValue,ffx::planLiteral(oldValue,L"AP: 300"));
    ffx::TextPlan earned;ffx::prepareTextPlan(earned,"reward-earned",false);earned.context[6]=1;ffx::addStatusRow(reward,earned,ffx::planLiteral(earned,L"Gil earned: 90"));
    ffx::TextPlan drop;ffx::prepareTextPlan(drop,"reward-drop",false);ffx::addStatusRow(reward,drop,ffx::planLiteral(drop,L"Ether x3"));
    require(ffx::finishStatusRows(reward),"reward page accepted");nativeMoveDown();nativeOwner=false;
    require(take()==L"AP"&&take()==L"Gil earned: 90"&&take()==L"AP: 300","mandatory earned total speaks with Autodetail Off, optional drop stays silent, and cursor is ordered");
    nativeOwner=true;auto updated=ffx::beginStatusRows(0,0,false,false,false);updated.battle=true;
    ffx::TextPlan newTitle;ffx::prepareTextPlan(newTitle,"reward-page",false);ffx::addStatusRow(updated,newTitle,ffx::planLiteral(newTitle,L"AP"));
    ffx::TextPlan newValue;ffx::prepareTextPlan(newValue,"reward-value",false);ffx::addStatusRow(updated,newValue,ffx::planLiteral(newValue,L"AP: 0"));
    require(ffx::finishStatusRows(updated)&&ffx::readSelectionDetail(),"quiet Confirm refresh followed by I is admitted");nativeOwner=false;
    require(take()==L"AP. AP: 0","quiet refresh updates current visible page before I");
    nativeOwner=true;nativeMoveDown();nativeOwner=false;require(take()==L"AP","quiet refresh preserves cursor rather than resetting to the first row");
    require(nativeAllocations.load()==0&&nativeFrees.load()==0,"large Help and reward refresh keep native heap allocation/free at zero");

    const double collectionUs=1e6*(captured.QuadPart-start.QuadPart)/frequency.QuadPart;
    const double firstMs=1e3*(first.QuadPart-start.QuadPart)/frequency.QuadPart;
    std::printf("PASS: status_review production batch API, TextPlan and ready-name pipeline with synthetic Native/getter/hooks/backend. Initial 65-row collection %.1f us; request-to-first output %.2f ms. Synthetic protocol evidence only; no game or real backend performance qualification.\n",collectionUs,firstMs);
    return 0;
}
