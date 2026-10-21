#include "../ready_game_text.hpp"
#include "../speech_queue.hpp"
#include "../events.hpp"
#include "../settings.hpp"
#include "../utility_text.hpp"
#include <atomic>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <cwchar>
namespace {
thread_local bool nativeCapture=false;
std::atomic<unsigned> allocations{0},destructions{0};
std::atomic<bool> hold{true};
HANDLE entered=nullptr,resume=nullptr,sinkSignal=nullptr;
std::array<std::array<wchar_t,256>,16> consoleHistory{};
std::array<unsigned,16> consoleLengths{};
unsigned consoleCount=0;
uint8_t text[2048]{},record[32]{},character[64]{};
uint8_t* reusedGear=nullptr;
void check(bool ok,const char* message) {if(!ok) {std::fprintf(stderr,"FAIL: %s\n",message);std::exit(1);}}
void encoded(uint8_t* target,const wchar_t* value) {
    constexpr wchar_t alphabet[]=L"0123456789 !\"#$%&'()*+,-./:;<=>?ABCDEFGHIJKLMNOPQRSTUVWXYZ[\\]^_\u2018abcdefghijklmnopqrstuvwxyz";
    while(*value) {const auto found=std::wcschr(alphabet,*value++);check(found!=nullptr,"fixture glyph");*target++=static_cast<uint8_t>(0x30+found-alphabet);}*target=0;
}
uintptr_t __cdecl getter(unsigned,uintptr_t* base) {
    check(nativeCapture,"native record getter stays on caller");
    encoded(text,L"Potion");encoded(text+256,L"Restores HP");
    if(reusedGear) {std::memset(reusedGear+0xe,0,8);reusedGear=nullptr;}
    if(base) *base=reinterpret_cast<uintptr_t>(text);
    return reinterpret_cast<uintptr_t>(record);
}
unsigned __cdecl language() {check(nativeCapture,"native language getter stays on caller");return 0;}
uintptr_t __cdecl saved(unsigned) {check(nativeCapture,"saved getter stays on caller");return reinterpret_cast<uintptr_t>(character);}
void held() {check(WaitForSingleObject(entered,3000)==WAIT_OBJECT_0,"preparation held");}
void release() {nativeCapture=false;hold=false;SetEvent(resume);}
void armHold() {ResetEvent(entered);ResetEvent(resume);hold=true;}
std::wstring take() {
    const auto deadline=GetTickCount64()+3000;ffx::speechq::Utterance result;
    while(!ffx::speechq::take(result)) {
        const auto now=GetTickCount64();if(now>=deadline) break;
        WaitForSingleObject(sinkSignal,static_cast<DWORD>(deadline-now));
    }
    check(!result.text.empty(),"owned first usable output");return result.text;
}
}
void* operator new(size_t n) {if(nativeCapture) ++allocations;if(auto p=std::malloc(n?n:1)) return p;throw std::bad_alloc();}
void* operator new[](size_t n) {return ::operator new(n);}
void operator delete(void* p) noexcept {if(nativeCapture && p) ++destructions;std::free(p);}
void operator delete[](void* p) noexcept {::operator delete(p);}
void operator delete(void* p,size_t) noexcept {::operator delete(p);}
void operator delete[](void* p,size_t) noexcept {::operator delete(p);}
namespace ffx {
unsigned generation() {return speechq::generation();}
int setting(Setting s) {return s==Setting::Speech?1:0;}
uint8_t equippedItem(unsigned,bool) {check(false,"equipment-index getter not used by these recipes");return 255;}
uintptr_t address(unsigned rva) {
    check(nativeCapture,"address resolution stays on native caller");
    if(rva==0x390a40 || rva==0x39a4c0) return reinterpret_cast<uintptr_t>(&getter);
    if(rva==0x3852b0) return reinterpret_cast<uintptr_t>(&language);
    if(rva==0x3853f0) return reinterpret_cast<uintptr_t>(&saved);
    check(false,"unexpected native getter");return 0;
}
bool copy(uintptr_t source,void* target,size_t count) {check(nativeCapture,"native bytes stay on caller");std::memcpy(target,reinterpret_cast<const void*>(source),count);return true;}
void prepareCopiedControlSnapshot(CopiedControls& p) {p={};}
OwnedTextContext copiedControlContext(const CopiedControls&) {check(!nativeCapture,"decode context stays on worker");OwnedTextContext result;result.primaryDodge=L"Primary dodge";result.glyphs[0x31]=L"Confirm";result.glyphs[0x48]=L"Right";return result;}
std::wstring join(const std::vector<std::wstring>& parts,const wchar_t* separator) {
    check(!nativeCapture,"joining stays on worker");std::wstring out;
    for(const auto& part:parts) if(!part.empty()) {if(!out.empty()) out+=separator;out+=part;}return out;
}
bool readCopiedTextDetail() {return false;}
void resetCopiedTextDetail() {}
void consoleReplyReady(const wchar_t* line,size_t length) {
    check(!nativeCapture && line && length<consoleHistory[0].size() && consoleCount<consoleHistory.size(),
        "utility console history receives bounded copied output on preparation owner");
    consoleLengths[consoleCount]=static_cast<unsigned>(length);
    std::wmemcpy(consoleHistory[consoleCount].data(),line,length);consoleHistory[consoleCount][length]=0;++consoleCount;
}
namespace speechq {void captureBeforeDrainForTest() {if(hold.load()) {SetEvent(entered);WaitForSingleObject(resume,INFINITE);}}}
}
int main() {
    const uint16_t descriptionOffset=256;std::memcpy(record+8,&descriptionOffset,2);
    const unsigned hp=100,maxHp=200,mp=17,maxMp=30;
    std::memcpy(character+0x1c,&hp,4);std::memcpy(character+0x24,&maxHp,4);
    std::memcpy(character+0x20,&mp,4);std::memcpy(character+0x28,&maxMp,4);character[0x3b]=4;
    entered=CreateEventW(nullptr,TRUE,FALSE,nullptr);resume=CreateEventW(nullptr,TRUE,FALSE,nullptr);
    const auto backendWake=CreateEventW(nullptr,FALSE,FALSE,nullptr);
    sinkSignal=backendWake;
    check(entered && resume && backendWake,"barriers");ffx::installSelectionDetails();ffx::installTextPlans();ffx::installUtilityText();
    check(ffx::speechq::start(GetModuleHandleW(nullptr),backendWake),"preparation starts without real SDK");held();
    nativeCapture=true;ffx::TextPlan p;ffx::prepareTextPlan(p,"items");
    p.headline=ffx::planCountedName(p,ffx::planRecordName(p,0x390a40,1),3);
    ffx::planSelectionDetail(p,ffx::planRecordDescription(p,0x390a40,1),true);
    check(ffx::commitTextPlan(p) && ffx::commitTextPlan(p),"equal distinct plans publish while worker held");
    ffx::TextPlan append;ffx::prepareTextPlan(append,"field-requirement",false);
    append.description=ffx::planLiteral(append,L"More");append.append=true;append.detail=ffx::currentSelectionDetailToken();
    check(ffx::commitTextPlan(append) && ffx::readSelectionDetail(),"append/manual stays in event order");
    std::memset(text,0,sizeof(text));p={};release();
    check(take()==L"Potion, 3" && take()==L"Restores HP" && take()==L"Potion, 3" && take()==L"Restores HP" &&
        take()==L"Restores HP. More","worker owns source reuse, exact formatting, equal events and detail FIFO");
    nativeCapture=true;ffx::TextPlan stats;ffx::prepareTextPlan(stats,"character");
    stats.headline=ffx::planCharacterDescription(stats,0);check(ffx::commitTextPlan(stats),"character POD plan");nativeCapture=false;
    check(take()==L"HP 100/200, MP 17/30, Sphere Levels 4","saved values retain exact displayed formatting");
    uint8_t gear[32]{};gear[0xb]=3;const uint16_t ability=0x2001,unused=0xff;
    std::memcpy(gear+0xe,&ability,2);std::memcpy(gear+0x10,&ability,2);std::memcpy(gear+0x12,&unused,2);std::memcpy(gear+0x14,&unused,2);
    nativeCapture=true;ffx::TextPlan equipment;ffx::prepareTextPlan(equipment,"gear");
    reusedGear=gear;equipment.headline=ffx::planEquipmentDetails(equipment,reinterpret_cast<uintptr_t>(gear));
    check(ffx::commitTextPlan(equipment),"equipment ready names and slot POD");nativeCapture=false;
    check(take()==L"Potion, Potion, 1 empty slot","four ability IDs copied before native getters can reuse the record");
    nativeCapture=true;ffx::TextPlan percent;ffx::prepareTextPlan(percent,"vitals");
    percent.headline=ffx::planVitals(percent,100,200,false,false);check(ffx::commitTextPlan(percent),"HP percent recipe");nativeCapture=false;
    check(take()==L"HP 100 (50%)","HP percentage preserves the existing maximum-HP preference");
    nativeCapture=true;ffx::TextPlan maximum;ffx::prepareTextPlan(maximum,"vitals");
    maximum.headline=ffx::planVitals(maximum,100,200,false,true);check(ffx::commitTextPlan(maximum),"HP maximum recipe");nativeCapture=false;
    check(take()==L"HP 100/200","opt-in HP maximum");
    nativeCapture=true;ffx::TextPlan mpPlan;ffx::prepareTextPlan(mpPlan,"vitals");
    mpPlan.headline=ffx::planVitals(mpPlan,17,30,true,false);check(ffx::commitTextPlan(mpPlan),"displayed MP recipe");nativeCapture=false;
    check(take()==L"MP 17/30","native MP target display remains distinct from HP preference");
    nativeCapture=true;ffx::TextPlan prefix;ffx::prepareTextPlan(prefix,"prefix");
    prefix.headline=ffx::planPrefix(prefix,0,ffx::planLiteral(prefix,L"Value"),L". ");
    prefix.headline=ffx::planSuffix(prefix,prefix.headline,0,L". ");check(ffx::commitTextPlan(prefix),"empty prefix/suffix recipe");nativeCapture=false;
    check(take()==L"Value","empty optional title/detail adds no punctuation");
    nativeCapture=true;ffx::TextPlan party;ffx::prepareTextPlan(party,"party");ffx::TextRef rows=0;
    encoded(text,L"Potion");
    for(unsigned i=0;i<18;++i) {
        const auto name=ffx::planEncoded(party,reinterpret_cast<uintptr_t>(text),40);
        const auto resource=ffx::planVitals(party,100+i,200,false,false);
        const auto line=ffx::planConcat(party,ffx::planConcat(party,name,ffx::planLiteral(party,L": ")),resource);
        rows=ffx::planPair(party,ffx::TextPlanOp::Periods,rows,line);
    }
    party.headline=ffx::planConcat(party,ffx::planLiteral(party,L"All party members. "),rows);
    check(ffx::commitTextPlan(party),"largest supported18-row party recipe fits complete capacity");nativeCapture=false;
    auto expected=std::wstring(L"All party members. ");
    for(unsigned i=0;i<18;++i) {if(i) expected+=L". ";expected+=L"Potion: HP "+std::to_wstring(100+i)+L" ("+std::to_wstring(((100+i)*100+100)/200)+L"%)";}
    check(take()==expected,"all party members keep order, distinct equal names and exact HP values");
    nativeCapture=true;ffx::TextPlan page;ffx::prepareTextPlan(page,"attributes");
    page.headline=ffx::planLiteral(page,L"Attributes");
    const auto pageStats=ffx::planLiteral(page,L"HP: 100"),learned=ffx::planLiteral(page,L"Potion");
    ffx::addTextPlanOutput(page,pageStats,"stats");ffx::addTextPlanOutput(page,learned,"learned");
    ffx::planSelectionDetail(page,ffx::planSuffix(page,pageStats,learned,L". "),false);
    check(ffx::commitTextPlan(page) && ffx::readSelectionDetail(),"page outputs/manual captured");nativeCapture=false;
    check(take()==L"Attributes" && take()==L"HP: 100" && take()==L"Potion" && take()==L"HP: 100. Potion","title, stats, abilities and manual detail retain producer order");
    armHold();nativeCapture=true;ffx::TextPlan sequence;ffx::prepareTextPlan(sequence,"overdrive-auron-sequence");
    const auto confirm=ffx::planNativeButtonMask(sequence,0x20),right=ffx::planNativeButtonMask(sequence,0x8000);
    sequence.gate=ffx::planPair(sequence,ffx::TextPlanOp::IfText,confirm,right);
    sequence.headline=ffx::planConcat(sequence,ffx::planLiteral(sequence,L"Sequence: "),ffx::planJoin(sequence,confirm,right));
    check(ffx::commitTextPlan(sequence) && ffx::commitTextPlan(sequence),"two copied logical button sequences admitted while held");sequence={};held();release();
    check(take()==L"Sequence: Confirm, Right" && take()==L"Sequence: Confirm, Right","logical masks decode on actual worker and equal casts remain distinct");
    nativeCapture=true;ffx::TextPlan dodge;ffx::prepareTextPlan(dodge,"mod-menu");
    dodge.headline=ffx::planSuffix(dodge,ffx::planPrefix(dodge,ffx::planLiteral(dodge,L"Dodge: "),ffx::planPrimaryDodgeName(dodge),L""),ffx::planLiteral(dodge,L". Press once when the beep begins."),L"");
    check(ffx::commitTextPlan(dodge),"primary raw dodge recipe");nativeCapture=false;
    check(take()==L"Dodge: Primary dodge. Press once when the beep begins.","manual dodge help formats the copied primary binding on Worker");
    armHold();nativeCapture=true;
    ffx::UtilityText guide(L"Via Purifico. ");guide.integer(7).literal(L" of 7 treasures collected.");
    ffx::emitUtilityText(guide);ffx::emitUtilityText(guide);guide=ffx::UtilityText{};
    held();release();
    const std::wstring guideLine=L"Via Purifico. 7 of 7 treasures collected.";
    check(take()==guideLine && take()==guideLine,"equal utility guide snapshots survive carrier reset in exact order");
    check(consoleCount==2 && consoleLengths[0]==guideLine.size() && consoleLengths[1]==guideLine.size() &&
        std::wcscmp(consoleHistory[0].data(),guideLine.c_str())==0 && std::wcscmp(consoleHistory[1].data(),guideLine.c_str())==0,
        "utility reply history keeps two equal receipts in publication order");
    armHold();nativeCapture=true;
    char reason[]="busy";ffx::UtilityText error=ffx::utilityError(L"Utility stopped: ",reason);
    ffx::emitUtilityText(error);std::memset(reason,'x',sizeof(reason)-1);error=ffx::UtilityText{};
    held();release();
    check(take()==L"Utility stopped: busy.","utilityError keeps copied narrow bytes after source and carrier reset");
    armHold();nativeCapture=true;
    ffx::UtilityText overflow(L"Rejected: ");
    for(unsigned i=0;i<ffx::UtilityText{}.parts.size()+1;++i) overflow.integer(i);
    ffx::emitUtilityText(overflow);
    ffx::UtilityText ordinary(L"Ordinary utility reply.");ffx::emitUtilityText(ordinary);
    overflow=ffx::UtilityText{};ordinary=ffx::UtilityText{};
    held();release();
    check(take()==L"Ordinary utility reply.","overflow rejects the entire utility carrier and the next ordinary reply succeeds");
    check(consoleCount==4 && std::wcscmp(consoleHistory[2].data(),L"Utility stopped: busy.")==0 &&
        std::wcscmp(consoleHistory[3].data(),L"Ordinary utility reply.")==0,"utility history records only complete accepted carriers in order");
    LARGE_INTEGER frequency{};QueryPerformanceFrequency(&frequency);double maxCapture=0,maxLatency=0;
    for(unsigned i=0;i<32;++i) {
        LARGE_INTEGER start{},captured{},finished{};QueryPerformanceCounter(&start);
        nativeCapture=true;ffx::TextPlan warm;ffx::prepareTextPlan(warm,"items");
        warm.headline=ffx::planCountedName(warm,ffx::planRecordName(warm,0x390a40,1),i);
        check(ffx::commitTextPlan(warm),"warm ready publication");nativeCapture=false;QueryPerformanceCounter(&captured);
        check(take()==L"Potion, "+std::to_wstring(i),"rapid request output remains ordered");QueryPerformanceCounter(&finished);
        maxCapture=std::max(maxCapture,1e6*(captured.QuadPart-start.QuadPart)/frequency.QuadPart);
        maxLatency=std::max(maxLatency,1e3*(finished.QuadPart-start.QuadPart)/frequency.QuadPart);
    }
    double hudCapture=0,hudLatency=0;
    for(unsigned pass=0;pass<32;++pass) {
        LARGE_INTEGER start{},captured{},finished{};QueryPerformanceCounter(&start);
        nativeCapture=true;ffx::TextPlan hud;ffx::prepareTextPlan(hud,"battle-turn-order");ffx::TextRef hudRows=0;
        for(unsigned i=0;i<32;++i)hudRows=ffx::planOrderedRow(hud,hudRows,ffx::planLiteral(hud,i%2?L"Enemy A":L"Tidus"),31-i);
        hud.headline=ffx::planConcat(hud,ffx::planLiteral(hud,L"Turn order: "),hudRows);
        check(ffx::commitTextPlan(hud),"maximum copied HUD plan publication");nativeCapture=false;QueryPerformanceCounter(&captured);
        std::wstring hudExpected=L"Turn order: ";for(unsigned i=0;i<32;++i){if(i)hudExpected+=L", ";hudExpected+=i%2?L"Tidus":L"Enemy A";}
        check(take()==hudExpected,"worker orders32 copied HUD rows and keeps every repeated turn");QueryPerformanceCounter(&finished);
        hudCapture=std::max(hudCapture,1e6*(captured.QuadPart-start.QuadPart)/frequency.QuadPart);
        hudLatency=std::max(hudLatency,1e3*(finished.QuadPart-start.QuadPart)/frequency.QuadPart);
    }
    std::printf("METRIC HUD32-row copied plan: max Native capture %.1fus, request-to-usable event %.2fms; actual queue/preparation, controlled sink, no game/GPU/PRISM/hardware timing.\n",hudCapture,hudLatency);
    nativeCapture=true;ffx::TextPlan oversized;ffx::prepareTextPlan(oversized,"bound");
    for(unsigned i=0;i<ffx::TextPlanNodeLimit+4;++i) ffx::planInteger(oversized,i);
    check(!ffx::commitTextPlan(oversized),"recipe overflow rejects whole event");nativeCapture=false;
    check(allocations.load()==0 && destructions.load()==0,"native builders/publication have zero owned heap activity");
    std::printf("PASS: owned text recipes, native getter affinity, source reuse, equal names/events, details and capacity; 32 warm requests max capture %.1f us, request-to-output %.2f ms (synthetic/event consumer).\n",maxCapture,maxLatency);
}
