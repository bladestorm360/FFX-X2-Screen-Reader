#include <atomic>
#include "../blitz_review.hpp"
#include "../utility_memory.hpp"
#include "../minigame_memory.hpp"
#include "../events.hpp"
#include "../text_plan.hpp"
#include "../settings.hpp"
#include "test_copied_controls.hpp"
#include "ready_speech_adapter.hpp"
#include <map>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <stdexcept>
namespace ffx {
std::atomic<int> scene{0};int selected=0,unassigned=0;
unsigned epoch=1;
bool valid=true,failStat=false,failLevel=false;
bool resourcePending=false;
std::array<uintptr_t,8> windows{},cursors{};
std::array<uint8_t,128> workerHeader{},window{},cursor{};
std::vector<std::wstring> speech;
bool copy(uintptr_t p,void* out,size_t n) {
    __try { memcpy(out,reinterpret_cast<void*>(p),n); return true; } __except(1) { return false; }
}
uintptr_t address(unsigned rva) { return reinterpret_cast<uintptr_t>(rva==0x14676b0?windows.data():cursors.data()); }
unsigned generation() { return epoch; }
UtilityScript utilityScript() { UtilityScript s; if(valid) { s.name="bltz0200"; s.length=113266; s.workers=7; s.chunk=1; s.allocationSerial=1; } return s; }
bool captureUtilityScript(UtilityScript& s) noexcept {s=utilityScript();return valid;}
unsigned scriptChecksum(const UtilityScript&) { return 0x58a5c0a5; }
uintptr_t utilityWorker(const UtilityScript&,unsigned) { if(resourcePending) throw std::runtime_error("pending utility worker must not be read"); return reinterpret_cast<uintptr_t>(workerHeader.data()); }
bool captureUtilityWorker(const UtilityScript& s,unsigned local,uintptr_t& w) noexcept {if(resourcePending) return false;w=utilityWorker(s,local);return true;}
AtelPreparation atelPreparation(uintptr_t) { return resourcePending?AtelPreparation::Pending:AtelPreparation::Ready; }
bool nativeAtelSpan(uintptr_t chunk,AtelSpan& span) { span={chunk,epoch,1,200000};return true; }
bool readyAtelIdentity(uintptr_t chunk,AtelIdentity& identity) {
    if(resourcePending) return false;
    identity.span={chunk,epoch,1,200000};identity.length=113266;identity.workers=7;identity.checksum=0x58a5c0a5;
    std::memcpy(identity.name,"bltz0200",9);return true;
}
bool onAtelPrepared(AtelPreparedHandler) { return true; }
int utilityValue(const UtilityScript&,unsigned low,unsigned high,unsigned element) {
    if(low==0x56000000 && high==1) return selected;
    if(low==0x5d0003f4 && high==16) return element+7;
    if(low==0x010013ce && high==60) {if(failLevel) throw std::runtime_error("fixture unavailable level");return 3;}
    if(low==0x5d000340 && high==6) return static_cast<int>(element)<unassigned?7:100;
    if(failStat && low==0x5d000100) throw std::runtime_error("fixture unavailable stat");
    const std::map<unsigned,int> values={{0x5d000040,123},{0x5d000300,60},{0x5d000180,12},{0x5d000100,14},
        {0x5d0001c0,16},{0x5d000240,18},{0x5d000200,20},{0x5d000280,22}};
    return values.at(low)+static_cast<int>(element);
}
MinigameScript captureMinigameScript(uintptr_t) { return {}; }
bool captureUtilityValue(const UtilityScript& s,unsigned low,unsigned high,unsigned element,int& value) noexcept {try {value=utilityValue(s,low,high,element);return true;}catch(...) {return false;}}
std::wstring join(const std::vector<std::wstring>& parts,const wchar_t* separator) {
    std::wstring result; for(auto& p:parts) { if(!result.empty()) result+=separator; result+=p; } return result;
}
bool output(const std::wstring& text,bool,const char*) { speech.push_back(text); return true; }
void log(const char*,...) {}
bool outputDialogue(const std::wstring& text,bool interrupt,const char* producer,bool) {return output(text,interrupt,producer);}
int setting(Setting s) {return s==Setting::Speech?1:0;}
bool hook(unsigned,Handler,Handler) {return false;}
void emitSelectionDetailOwned(std::wstring,SelectionDetailToken,bool,const speechq::Header&) {}
void appendSelectionDetailOwned(std::wstring,SelectionDetailToken) {}
}
#include "test_speech_domains.hpp"
namespace ffx::speechq { void notifyPreparation() noexcept {} }
using namespace ffx;
bool observe(unsigned slot,const MessagePage& page) {return observeBlitzMessageOwned(slot,page,captureBlitzSnapshot());}
void reviewKey(unsigned key) {dispatchBlitzKey(key,true);drainSpeechDomainsForTest();}
void check(bool value,const char* why) { if(!value) { std::fprintf(stderr,"FAIL: %s\n",why); std::exit(1); } }
MessagePage page(std::initializer_list<std::wstring> lines,bool selector=false) { MessagePage p; p.lines=lines; p.selector=selector; return p; }
int main() {
    installTextPlans();installBlitzReview();resetBlitzReview(); check(!blitzCapturesKey(VK_DOWN,true),"field keeps Ctrl arrows outside Blitzball");
    resourcePending=true;
    auto pending=captureBlitzSnapshot();
    check(pending.valid&&pending.lobby&&blitzSnapshotPreparation(pending)==AtelPreparation::Pending,"pending exact resource retains a valid native lobby snapshot");
    check(!observeBlitzMessageOwned(2,page({L"Pending name"}),pending)&&!blitzReviewAvailable(),"pending copied message is not accepted into review before resource readiness");
    resourcePending=false;
    check(blitzSnapshotPreparation(pending)==AtelPreparation::Ready&&observeBlitzMessageOwned(2,page({L"Ready name"}),pending),"same copied snapshot is accepted after exact resource readiness");
    reviewKey(VK_RIGHT);check(speech.back().find(L"Ready name")==0,"ready resource publishes retained copied name to review");
    releaseBlitzSnapshot(pending);
    resetBlitzReview();
    resourcePending=true;
    std::array<BlitzSnapshot,64> tickets{};
    for(auto& ticket:tickets) { ticket=captureBlitzSnapshot();check(ticket.valid&&blitzSnapshotPreparation(ticket)==AtelPreparation::Pending,"each bounded pending receipt retains its exact ticket"); }
    const auto warningCount=speech.size();
    const auto rejected=captureBlitzSnapshot();
    check(!rejected.valid&&speech.size()==warningCount+1,"the sixty-fifth pending ticket is rejected with one explicit overload notice");
    const auto rejectedAgain=captureBlitzSnapshot();
    check(!rejectedAgain.valid&&speech.size()==warningCount+1,"repeated overload is reported once until capacity recovers");
    for(const auto& ticket:tickets) releaseBlitzSnapshot(ticket);
    const auto recovered=captureBlitzSnapshot();check(recovered.valid,"released pending tickets allow fresh capture");releaseBlitzSnapshot(recovered);
    resetBlitzReview();resourcePending=false;
    observe(1,page({},true));
    check(observe(2,page({L"Tidus"})),"verified name-only stats window has no speech producer");
    observe(3,page({L"Venom Tackle",L"30 HP",L"Adds poison with the displayed probability."}));
    reviewKey(VK_LEFT); check(speech.back()==L"Tidus","first buffer row is selected player's name");
    reviewKey(VK_DOWN); check(speech.back()==L"Level: 3","Ctrl Down advances to level");
    reviewKey(VK_RIGHT);
    for(auto text: {L"HP: 123",L"Speed: 60",L"Endurance: 12",L"Attack: 14",L"Pass: 16",L"Block: 18",L"Shot: 20",L"Catch: 22",L"30 HP",L"Adds poison"})
        check(speech.back().find(text)!=std::wstring::npos,"buffer includes native stat or move detail with its correct label");
    selected=1; observe(1,page({},true)); reviewKey(VK_RIGHT);
    check(speech.back()==L"No Blitzball details are displayed.","fresh selector cannot retain previous player's stats/move");
    observe(2,page({L"Wakka"})); reviewKey(VK_RIGHT);
    check(speech.back().find(L"HP: 124")!=std::wstring::npos && speech.back().find(L"Venom")==std::wstring::npos,"next player's values replace old description");
    failStat=true;reviewKey(VK_RIGHT);
    check(speech.back().find(L"Wakka. Level: 3. HP: 124. Speed: 61. Endurance: 13") == 0 && speech.back().find(L"Attack:")==std::wstring::npos,"partial getter failure retains already copied native stats in original order");
    failStat=false;
    failLevel=true;reviewKey(VK_RIGHT);
    check(speech.back().find(L"Wakka")==0 && speech.back().find(L"Level:")==std::wstring::npos,"missing first numeric getter still preserves the captured native player name");failLevel=false;
    windows[1]=reinterpret_cast<uintptr_t>(window.data()); cursors[1]=reinterpret_cast<uintptr_t>(cursor.data());
    *reinterpret_cast<uint16_t*>(window.data()+0x2c)=2; cursor[1]=1;
    observe(6,page({L"Assign Positions",L"Choose your team."}));
    const wchar_t* positions[]={L"left forward",L"right forward",L"midfielder",L"left defense",L"right defense",L"goalkeeper"};
    for(int i=0;i<6;++i) { unassigned=i; check(dispatchBlitzKey(VK_OEM_5,false),"Backslash owned by active position assignment"); drainSpeechDomainsForTest();check(speech.back()==L"Assigning "+std::wstring(positions[i]),"assignment uses first empty native slot"); }
    cursor[1]=0; check(!blitzCapturesKey(VK_OEM_5,false),"closed selector releases Backslash");
    ++epoch; check(!blitzReviewAvailable(),"map change invalidates the review buffer");
    valid=false; check(!observe(2,page({L"Ordinary dialogue"})),"ordinary windows retain normal speech");
    std::puts("PASS: Blitzball native player stats, move lines, fresh selection buffers, mirror ownership and all six position announcements.");
}
