#include <atomic>
#include "external_fixture.hpp"
#include "native_heap_counter.hpp"
#include <chrono>
#include "../sphere_owned.hpp"
#include "ready_speech_adapter.hpp"
#include "test_jecht_caption.hpp"
#include "../owned_callback_proof.hpp"
#include "../input_device.hpp"
#include "../auto_walk.hpp"
#include "../airship_search.hpp"
#include "../events.hpp"
#include "../copied_text.hpp"
#include "../profile.hpp"
#include "../profile_extensions_data.hpp"
#include "../navigation.hpp"
#include "../navigation_intents.hpp"
#include "../navigation_observations.hpp"
#include "../nav_audio.hpp"
#include "../owned_nav_target.hpp"
#include "../pedestal.hpp"
#include "../utility_memory.hpp"
#include "../utility_context.hpp"
#include "../wall_cues.hpp"
#include "../nav_format.hpp"
#include "../navigation_targets.hpp"
#include "../sphere_grid.hpp"
#include "../settings.hpp"
#include "../naming.hpp"
#include "../message_text.hpp"
#include "../text_plan.hpp"
#include "../ready_text.hpp"
#include "../ready_game_text.hpp"
#include "../owned_message_decoder.hpp"
#include "message_reference_parser.hpp"
#include "../speech_domains.hpp"
#include "../blitz_review.hpp"
#include "../field_controls.hpp"
#include "../battle_sensor.hpp"
#include "../game_visual_text.hpp"
#include "../overdrive.hpp"
#include "../status_animation.hpp"
#include "../status_animation_data.hpp"
#include "../swimming_navigation.hpp"
#include "../exit_labels.hpp"
#include "../butterfly_navigation.hpp"
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <map>
#include <functional>
#include <utility>
#include <fstream>
#include <limits>
namespace ffx::speechq { void notifyPreparation() noexcept {} }
namespace ffx {
uintptr_t testBase = 0;
std::atomic<int> scene{0};
bool testForeground=true;
bool testPad=false;
// Reader fixtures select independent synthetic devices; they do not simulate
// chronological hardware edges. Recency/tie behavior has its separate SDK test.
void fixtureInputDevice(bool controller) {inputDeviceMark.store(0);observeInputDevice(controller);}
bool testStatusOwner=true;
bool nativeGameThread() { return testStatusOwner; }
bool readerResourceLoading=false;
bool nativeResourceLoading() noexcept {return readerResourceLoading;}
bool outputAt(const std::wstring& text,bool interrupt,const char* producer,unsigned epoch) { return epoch==generation() && output(text,interrupt,producer); }
bool controllerOwned() { return testPad; }
NavigationKeyboardAssist searchKeyboard=nullptr;NavigationControllerAssist searchController=nullptr;
void setNavigationInputAssist(NavigationKeyboardAssist k,NavigationControllerAssist c) {searchKeyboard=k;searchController=c;}
void openHiddenAirshipMenu() {}
void pauseNavAudio() {}
void pauseNavRouteAudio() {}
bool fixtureFieldAudioAllowed=true;
void setFieldAudioAllowed(bool allowed) {fixtureFieldAudioAllowed=allowed;} // Actual voices have the SDL dummy-stream host.
void captureWallCues() {}
unsigned overdriveBeeps=0;
void navAudioOverdriveBeep(float,unsigned) { ++overdriveBeeps; }
void navAudioCancelOverdriveBeep() {}
void resetWakka() {}
void installWakka() {}
bool modMenuActive() { return false; }
bool utilityConsoleActive() { return false; }
bool utilityBusy() { return false; } // This host does not run utility scripts.
UtilityContext utilityContext() {return {};}
bool utilityContextValid(const UtilityContext&,bool) {return false;}
bool utilityActionAllowed(bool) {return false;}
void consoleReplyReady(const wchar_t*,size_t) {}
void utilityReply(const wchar_t* text) {output(text,true,"fixture-utility");}
bool deferNativeAction(NativeAction action) {
    if(!testStatusOwner) return false;const bool ready=!action.prepare || action.prepare(action);
    if(ready && action.invoke) action.invoke(action);if(action.complete) action.complete(action,ready);if(action.retire) action.retire(action);return true;
}
NavLeg fixtureLeg;bool fixtureRoute=false;
bool navLeg(NavLeg& leg) {if(!fixtureRoute) return false;leg=fixtureLeg;return true;} // Copied steering endpoint; actual audio/replans have their separate host.
bool butterflyActive() { return false; }
bool butterflyNavigationLeaving(const AtelSpan&) {return false;}
std::vector<Entity> butterflyTargets() { return {}; }
void refreshButterflyTarget(Entity&) {}
void refreshButterflyStarter(Entity&) {}
void configureButterflyStarter(Entity&,const char*,unsigned,uintptr_t) {}
void prepareNavRetirement() {}
void resetNavAudio() {fixtureRoute=false;}
void clearNavRoute() {fixtureRoute=false;}
void stopAutoWalk() {}
unsigned autoStarts=0;
void startAutoWalk() { ++autoStarts; }
void installAutoWalk() {}
void armPedestal(const Entity&) {}
void cancelPedestal() {}
void installPedestal() {}
void preparePedestalZone(Entity&,uintptr_t) {}
std::wstring pedestalInstruction(const Entity&) { return L"Fixture push instruction"; }
bool nativeReaderResourcesReady() noexcept {return true;}
unsigned autoWalkInputRevision() {return 0;}
unsigned pedestalConfirmMask() {return 0x20;}
PedestalNavigationState capturePedestalNavigationState() {return {};}
void armPedestalOwned(const NativeRouteTarget&,const UtilityScript&,const PedestalPlan&) {}
void updatePedestalOwnedTarget(const NativeRouteTarget&) {}
bool testBlitzMirror=false;
BlitzSnapshot captureBlitzSnapshot(bool,bool) {BlitzSnapshot s;s.epoch=generation();s.valid=s.lobby=testBlitzMirror;return s;}
bool observeBlitzMessageOwned(unsigned slot,const MessagePage&,const BlitzSnapshot&) { return testBlitzMirror && slot==2; }
AtelPreparation blitzSnapshotPreparation(const BlitzSnapshot& s) { return s.valid?AtelPreparation::Ready:AtelPreparation::Retired; }
void releaseBlitzSnapshot(const BlitzSnapshot&) {}
bool blitzReviewAvailable() { return false; }
void installBlitzReview() {}
void resetBlitzReview() { testBlitzMirror=false; }
void refreshNavCollision() {}
bool navRouteKnown() { return false; }
unsigned navUpdates=0,targetUpdates=0,workerReads=0;
void updateOwnedNavAudio(const nav::Scene&,const Mesh&,const std::vector<Entity>&,const speechq::Header&,const std::wstring&) {++navUpdates;}
void updateNavAudio(Point,bool) { ++navUpdates; }
void updateNavTarget(const Entity&) { ++targetUpdates; }
void routeNavAudio(const std::vector<Point>& points,const Entity& target) {OwnedNavTarget captured;if(points.empty() || !captureOwnedNavTarget(target,captured)) {fixtureRoute=false;return;}fixtureLeg.start=points.front();fixtureLeg.goal=points.size()>1?points[1]:points.front();fixtureLeg.final=points.back();fixtureLeg.target=captured.native;fixtureLeg.epoch=generation();fixtureLeg.arrived=points.size()<2;fixtureRoute=true;}
void drainController() {}
void captureSpeech(bool) {}
void protectTutorialSpeech(bool) {}
struct Listener { Handler enter, leave; };
std::map<unsigned, std::vector<Listener>> listeners;
std::map<unsigned, void(*)(bool)> keys;
std::vector<std::pair<std::wstring, bool>> said;
bool hook(unsigned id, Handler enter, Handler leave) { listeners[id].push_back({enter,leave}); return true; }
void hotkey(unsigned vk, void(*cb)(bool)) { keys[vk] = cb; }
std::map<unsigned,bool(*)()> contexts;
void hotkeyWhen(unsigned vk,void(*cb)(bool),bool(*available)()) { keys[vk]=cb; contexts[vk]=available; }
std::map<unsigned,std::pair<void(*)(bool),bool(*)()>> chords;
void hotkeyChordWhen(unsigned vk,bool shift,void(*cb)(bool),bool(*available)(),bool) { chords[vk|(shift?0x10000u:0)]={cb,available}; }
bool protectedSpeech=false;
void protectSpeech(bool value) { if(!value) speechq::resetDomainProtection();protectedSpeech=value; }
bool output(const std::wstring& s,bool interrupt,const char*) {
    speechq::Header h{};h.generation=generation();
    if(!s.empty()) said.emplace_back(s,interrupt && !protectedSpeech && !speechq::domainsProtectSpeech(h));return true;
}
bool outputDialogue(const std::wstring& s,bool interrupt,const char* producer,bool) { return output(s,interrupt,producer); }
unsigned generation() { return 1; }
void log(const char*, ...) {}
}
#define drainSpeechDomainsForTest fixtureDrainDomains
#include "test_speech_domains.hpp"
#undef drainSpeechDomainsForTest
namespace ffx {void drainSpeechDomainsForTest() {const auto owner=testStatusOwner;testStatusOwner=false;fixtureDrainDomains();testStatusOwner=owner;}}
using namespace ffx;
namespace {
constexpr unsigned win=0x2200000, pool=0x2230000, sphere=pool+7*0x340;
void check(bool ok, const char* why) { if (!ok) { std::fprintf(stderr,"FAIL: %s\n",why); std::exit(1); } }
uintptr_t fixtureAddress(unsigned off) {
    if(extendedAddresses.count(off)) return testBase+extendedAddresses.at(off);
    for(const auto range: {std::pair<unsigned,unsigned>{0x146a240,128}, {0x85a790,64}})
        if(off>=range.first && off-range.first<range.second && extendedAddresses.count(range.first))
            return testBase+extendedAddresses.at(range.first)+off-range.first;
    // Existing vectors express subfields as old base+offset. Translate their
    // owning data base; production resolves bases before adding field offsets.
    if (nativeProfile == NativeProfile::Installed && off >= 0x700000 && off < 0x2100000) {
        const RvaPair* owner = nullptr;
        for (const auto& pair : nativeAddresses) {
            if (pair.reference > off) break;
            owner = &pair;
        }
        if (owner && off - owner->reference < 0x10000)
            return testBase + owner->installed + off - owner->reference;
    }
    return address(off);
}
template<class T> void set(unsigned off, T value) { check(write<T>(fixtureAddress(off),value),"fixture write"); }
void fixtureCollisionVertexCount(unsigned count) {
    check(count<=32767,"fixture collision vertex count fits native signed int16");
    constexpr unsigned root=0x25f0000;
    set<uintptr_t>(0xf01a54,address(root));
    set<int16_t>(root+0xA,static_cast<int16_t>(count));
}
uintptr_t encoded(unsigned off, const char* text) {
    static const char charset[] = "0123456789ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz";
    auto dst = reinterpret_cast<unsigned char*>(fixtureAddress(off));
    for (; *text; ++text, ++dst) {
        if (*text == ' ') *dst = 0x3a;
        else if(*text=='\n') *dst=3;
        else if(*text=='-') *dst=0x47;
        else if(*text=='.') *dst=0x48;
        else if(*text=='!') *dst=0x3b;
        else {
            const char* found = std::strchr(charset,*text);
            check(found != nullptr,"fixture text charset");
            const auto index = found - charset;
            *dst = static_cast<unsigned char>(index < 10 ? 0x30+index : index < 36 ? 0x50+index-10 : 0x70+index-36);
        }
    }
    *dst = 0; return fixtureAddress(off);
}
void stub(unsigned off, void* function) {
    auto code = reinterpret_cast<unsigned char*>(fixtureAddress(off));
    code[0]=0xb8; const auto target=reinterpret_cast<uintptr_t>(function);
    std::memcpy(code+1,&target,4); code[5]=0xff; code[6]=0xe0;
    FlushInstructionCache(GetCurrentProcess(),code,7);
}
uintptr_t __cdecl navigationWorker(int) { ++workerReads; return address(0x2251000); }
unsigned __cdecl language() { return 0; }
uintptr_t __cdecl names(int id) { return address(id==1 ? 0x2100020 : id==8 ? 0x2100040 : 0x2100000); }
uintptr_t __cdecl record(unsigned id, uintptr_t* base) {
    id&=0xfff;
    if(base) *base=address(id==7 ? 0x21000a0 : id==2 ? 0x2100080 : 0x2100060);
    set<uint16_t>(0x2101000,0); set<uint8_t>(0x2101022,2); set<uint16_t>(0x2101008,0x200);
    set<int8_t>(0x2101000+0x5d,id==0x49?0x49:id==0x51?0x51:-1);
    return address(0x2101000);
}
// Recovered original 790850 predicate: twelve-bit key ID,128 entries and
// sixteen-bit ownership words. Synthetic records supply text separately.
int __cdecl keyOwned(unsigned id) {
    id&=0xfff;
    return id<0x80 && (read<uint16_t>(address(0xd30f1c)+(id>>4)*2)&(1u<<(id&15)))!=0;
}
uintptr_t __cdecl keyRecord(unsigned id,uintptr_t* base) {
    const auto result=record(id,base);
    if(!(id&0xfff) && base) *base=address(0x2404a00);
    return result;
}
uintptr_t __cdecl gear(unsigned) { return address(0x21000c0); }
uintptr_t __cdecl gearRecord(unsigned id, uintptr_t*) { return address(id==254?0x2310800:0x2101040); }
uintptr_t __cdecl menu(int space, unsigned id, unsigned) {
    return address(space==11 || space==12 ? 0x21000e0 : space==7 ? (id==1 ? 0x2100100 : 0x2100120) : 0x2100140);
}
uintptr_t __cdecl menuHelp(int space,unsigned id,unsigned) {
    return space==1?address(id==0x1033?0x2400400:0x2400440):0;
}
unsigned __cdecl qty(unsigned id) { return id==2 ? 4 : 3; }
uint8_t __cdecl party(int slot) { return static_cast<uint8_t>(slot==1 ? 1 : 0); }
uint8_t __cdecl aeon() { return 8; }
uintptr_t __cdecl shopTable(unsigned, uintptr_t) { return address(0x2110000); }
unsigned __cdecl price(unsigned) { return 50; }
bool exactEscLabels=false;
void __cdecl loc(unsigned id, char* out, unsigned size) {
    if(!exactEscLabels) {strcpy_s(out,size,"Return to game");return;}
    check(testStatusOwner,"Escape localization stays on its native owner");
    if(id==0x2f0 || id==0x2f1 || (id>=0x2f6 && id<=0x2ff) || id==0x37f || id==0x380) {
        sprintf_s(out,size,"Native feature result %u",id);return;
    }
    const char* text=id==0x307?"Quit game?":id==0x308?"Yes":id==0x309?"No":id==0x217?"Resume":id==0x218?"Resume game":id==0x221?"Audio":
        id==0x222?"Change audio settings":id==0x2b6?"Encounters":id==0x2b7?"Change encounter rate":
        id==0x290?"Press $key$ to activate":id==0x291?"Speed":id==0x293?"Change game speed":
        id==0x297?"Enemy Encounter Override":id==0x299?"Set enemy encounter frequency. It is not affected by character equipment.":
        id==0x230?"Set master volume":"Other";
    strcpy_s(out,size,text);
}
uintptr_t __cdecl character(unsigned id) { return address(0x2120000)+id*0xf90; }
uint8_t __cdecl overdriveCurrent(unsigned id) { return read<uint8_t>(character(id)+0x5bc); }
uint8_t __cdecl overdriveMaximum(unsigned id) { return read<uint8_t>(character(id)+0x5bd); }
uintptr_t __cdecl savedCharacter(unsigned id) { return fixtureAddress(0xd3205c)+id*0x94; }
uintptr_t __cdecl equipmentLabel(unsigned,unsigned,int,uintptr_t) { return address(0x21000c0); }
uintptr_t __cdecl nodeDefinition(unsigned id,uintptr_t table,uintptr_t* strings) {
    if(strings) *strings=address(0x2310200);
    if(table==address(0x2310b00)) return address(id==0x49?0x2310c00:0x2310c40);
    return address(id==1?0x2310100:id==2?0x2310140:0x2310000);
}
uintptr_t __cdecl bindingLabel(unsigned value) {
    return address(value==0x3b?0x2310540:value==0x3d?0x2310560:value==3?0x2310500:0x2310520);
}
bool gridStatFixture=false;
unsigned __cdecl fixtureStat(uintptr_t saved,uintptr_t entry,int) {
    return gridStatFixture && entry==address(0x85a608)+2*0x1c?read<uint8_t>(saved+0x2f):123;
}
uintptr_t __cdecl requirementsTable() { return address(0x2110100); }
int __cdecl increase(int, unsigned, unsigned* id, unsigned* needed) { *id=1; *needed=5; return 1; }
void event(unsigned id, uintptr_t arg, const std::function<void()>& change={}, uintptr_t result=0) {
    auto& list=listeners[id]; check(!list.empty(),"registered event");
    std::vector<Event> calls(list.size());
    for(size_t i=0;i<list.size();++i) {
        auto& e=calls[i]; e.rva=id; e.args[0]=arg; e.result=result; e.epoch=generation();
        if(list[i].enter) list[i].enter(e);
    }
    if(change) change();
    for(size_t i=0;i<list.size();++i) if(list[i].leave) list[i].leave(calls[i]);
    prepareCopiedControlNames();drainSpeechDomainsForTest();
}
void fixturePhysicalPart(unsigned chunk,unsigned bytes) {
    // Exact, explicit fixture allocation: one EV01 part containing the already
    // initialized raw bytes. This tests physical bounds, independently of the
    // semantic code in synthetic map-header cases.
    static bool installed=false;
    if(!installed) {check(startAtelResources(),"fixture resource capture storage");installAtelResources();installed=true;}
    check(chunk>=0x30 && bytes && bytes<0x800000-0x30,"explicit fixture part size");
    const auto parent=address(chunk-0x30);
    Event freed;freed.args[0]=parent;for(const auto& listener:listeners[0x48d490]) if(listener.enter) listener.enter(freed);
    ZeroMemory(reinterpret_cast<void*>(parent),0x30);write<unsigned>(parent,0x31305645);write<unsigned>(parent+4,0x30);
    write<unsigned>(parent+8,bytes+0x30);write<unsigned>(parent+12,0xffffffff);
    Event load;load.args[0]=12;load.nativeStack=0x900000;load.epoch=generation();
    for(const auto& listener:listeners[0x48d230]) if(listener.enter) listener.enter(load);
    Event allocation;allocation.caller=address(0x48d230)+0x74;allocation.edi=bytes+0x30;allocation.args[0]=(allocation.edi+0x7ffu)&~0x7ffu;
    allocation.result=parent;allocation.epoch=generation();
    for(const auto& listener:listeners[0x2871c0]) if(listener.enter) listener.enter(allocation);
    for(const auto& listener:listeners[0x2871c0]) if(listener.leave) listener.leave(allocation);
    load.result=parent;for(const auto& listener:listeners[0x48d230]) if(listener.leave) listener.leave(load);
    const auto owner=testStatusOwner;testStatusOwner=false;drainAtelResourcesForTest();testStatusOwner=owner;
    AtelSpan span;const bool found=nativeAtelSpan(address(chunk),span);
    if(!found || span.size!=bytes) std::fprintf(stderr,"Physical fixture chunk=%x bytes=%u found=%u observed=%u rejected=%u\n",chunk,bytes,found,span.size,rejectedAtelResourcesForTest());
    check(found && span.size==bytes,"actual exact resource service retains the explicit physical part extent");
}
void navigationFixturePump() {
    for(unsigned i=0;i<3;++i) {if(testStatusOwner && (nav::navigationDataPending() || i>0)) {
        Event e;e.epoch=generation();e.rva=0x225930;for(const auto& listener:listeners[e.rva]) if(listener.leave) listener.leave(e);
    }drainSpeechDomainsForTest();}
}
void expect(const wchar_t* text, const char* why) {
    navigationFixturePump();
    prepareCopiedControlNames();drainSpeechDomainsForTest();
    check(!said.empty(),why);
    if(said.back().first!=text) {
        std::fwprintf(stderr,L"Expected [%s], received [%s]\n",text,said.back().first.c_str());
        check(false,why);
    }
}
}
// Synthetic direct renderer helper for known expected strings. Production has
// no synchronous allocating message-page parser; actual capture/getters and the
// owned decoder remain the subject of these tests.
ffx::MessagePage messagePage(unsigned slot,uintptr_t source) {
    ffx::TextPlan p;ffx::prepareTextPlan(p,"fixture-message");const auto root=ffx::planMessage(p,slot,source);
    check(p.valid && root,"fixture complete message capture");const auto& n=p.nodes[root-1];
    prepareCopiedControlNames();
    return ffx::decodeOwnedMessage(p.bytes.data()+n.offset,n.length,ffx::copiedControlContext(p.controls));
}
#include "battle_followups.inc"
#include "battle_hud_followups.inc"
#include "battle_switch_followups.inc"
#include "battle_outcomes_followups.inc"
#include "battle_debuff_followups.inc"
#include "status_animation_followups.inc"
#include "navigation_controls_followups.inc"
#include "navigation_identity_followups.inc"
#include "reader_followups.inc"
#include "airship_passages_followups.inc"
#include "polish_text_followups.inc"
#include "maintenance_readers_followups.inc"
#include "arena_capture_followups.inc"
#include "help_capture_followups.inc"
#include "airship_search_followups.inc"
#include "airship_runtime_followups.inc"
#include "reported_event_runtime_followups.inc"
#include "field_menu_fixture.cpp"
#include "sphere_reader_followups.inc"
#include "sphere_movement_followups.inc"
#include "sphere_owned_followups.inc"
#include "message_followups.inc"
#include "navigation_refresh_followups.inc"
#include "navigation_guard_followups.inc"
#include "navigation_interaction_followups.inc"
#include "navigation_format_followups.inc"
#include "swimming_followups.inc"
#include "button_glyph_followups.inc"
#include "real_swim_floor_followups.inc"
#include "swimming_progression_followups.inc"
#include "exit_labels_followups.inc"
#include "auron_sequence_followups.inc"
#include "sensor_overdrive_followups.inc"
#include "intro_cue_followups.inc"
#include "results_esc_followups.inc"
#include "battle_amount_followups.inc"
#include "formation_active_followups.inc"
#include "seymour_followups.inc"
#include "overdrive_learn_followups.inc"
#include "battle_hp_followups.inc"
#include "grid_native_replay.inc"
#include "airship_destinations_followups.inc"
#include "customize_native_replay.inc"
#include "help_native_replay.inc"
#include "baaj_navigation_followups.inc"
#include "airship_cid_followups.inc"
#include "sin_native_followups.inc"
#include "sin_platform_followups.inc"
#include "via_swimming_followups.inc"
int main(int argc, char** argv) {
    std::setvbuf(stdout,nullptr,_IONBF,0);
    SetErrorMode(SEM_FAILCRITICALERRORS|SEM_NOGPFAULTERRORBOX);
    SetUnhandledExceptionFilter([](EXCEPTION_POINTERS* p)->LONG {
        std::fprintf(stderr,"FAIL: isolated reader exception %lx at %p (mapped offset %lx), operand %p\n",
            p->ExceptionRecord->ExceptionCode,reinterpret_cast<void*>(p->ContextRecord->Eip),
            static_cast<unsigned long>(p->ContextRecord->Eip-testBase),
            reinterpret_cast<void*>(p->ExceptionRecord->ExceptionInformation[1]));
        const auto stack=reinterpret_cast<uintptr_t*>(p->ContextRecord->Esp);
        std::fprintf(stderr,"native replay return %p (mapped %lx)\n",reinterpret_cast<void*>(stack[0]),static_cast<unsigned long>(stack[0]-testBase));
        std::fprintf(stderr,"native replay last call mapped %lx\n",static_cast<unsigned long>(replayLastCall-testBase));
        return EXCEPTION_EXECUTE_HANDLER;
    });
    try {
    if (argc > 1 && std::strcmp(argv[1], "--installed-profile") == 0)
        nativeProfile = NativeProfile::Installed;
    for(const auto& spec:extensionSignatures) {
        bool fixed=false; for(const auto& pair:nativeAddresses) if(pair.reference==spec.rva) fixed=true;
        if(!fixed) extendedAddresses[spec.rva]=spec.rva;
    }
    for(const auto& operand:extensionOperands) {
        extendedAddresses[operand.reference]=operand.reference;
        if(nativeProfile==NativeProfile::Installed) for(const auto& pair:nativeAddresses)
            if(pair.reference==operand.reference) extendedAddresses[operand.reference]=pair.installed;
    }
    // Unknown extension functions normally use their reference RVA in this
    // synthetic host. In Installed that slot is the *mapped* saved-character
    // getter3853F0; keep the newly stubbed roster function in separate memory.
    // Actual installed signature uniqueness is checked by profile/preflight.
    if(nativeProfile==NativeProfile::Installed)extendedAddresses[0x385330]=0x20f1000;
    testBase=reinterpret_cast<uintptr_t>(VirtualAlloc(nullptr,0x2800000,MEM_COMMIT|MEM_RESERVE,PAGE_EXECUTE_READWRITE));
    check(testBase!=0,"isolated memory");
    if (nativeProfile == NativeProfile::Installed) {
        for (const auto& pair : nativeAddresses) {
            check(profileRva(pair.reference) == pair.installed, "native profile translation");
            check(logicalRva(address(pair.reference)) == pair.reference, "callback identity normalization");
        }
        bool refused = false;
        try { profileRva(0xdeadbe); } catch (const std::out_of_range&) { refused = true; }
        check(refused, "unverified native address must be refused");
    }
    encoded(0x2100000,"Tidus"); encoded(0x2100020,"Yuna"); encoded(0x2100040,"Valefor");
    encoded(0x2100060,"Potion"); encoded(0x2100080,"Ether"); encoded(0x21000a0,"Key");
    encoded(0x21000c0,"Sonic Steel"); encoded(0x21000e0,"Strength");
    encoded(0x2100100,"Sound"); encoded(0x2100120,"Stereo"); encoded(0x2100140,"Items");
    encoded(0x2100260,"Restores HP"); encoded(0x2100280,"Restores MP"); encoded(0x21002a0,"Native key help");
    stub(0x384f40,currentGil); stub(0x385070,names); stub(0x3852b0,language); stub(0x38fd40,menu); stub(0x38fc00,menuHelp);
    for(auto id:{0x3908f0,0x39a4c0,0x390a40}) stub(id,record);
    stub(0x3908f0,keyRecord);stub(0x390850,keyOwned);encoded(0x2404a00,"Withered Bouquet");
    stub(0x3abe10,gear); stub(0x390500,qty); stub(0x4a9ab0,party); stub(0x4a9830,aeon);
    stub(0x385330,fixtureFormationParty);
    for(unsigned i=0;i<3;++i)set<uint8_t>(0x24b0000+i,static_cast<uint8_t>(i));
    stub(0x3abbf0,gearRecord); stub(0x3853f0,savedCharacter); stub(0x4cec70,fixtureStat);
    stub(0x3a0c70,equipmentLabel); stub(0x3ab890,nodeDefinition);
    stub(0x5af0,bindingLabel); stub(0x5b10,bindingLabel);
    stub(0x4c1c00,requirementsTable); stub(0x4c1b50,requirementsTable); stub(0x4cd2f0,increase);
    stub(0x4c1ad0,shopTable); stub(0x4db390,price); stub(0x4db3e0,price);
    stub(0x2414a0,loc); stub(0x394030,character);
    followupStubs(); navigationIdentityFixtures(); airshipPassageFixtures();
    polishTextFixtures();
    setSetting(Setting::MenuIndex,1);
    installFieldMenus(); installMessages(); installSystemMenus(); installSphereGrid(); installBattle(); installNavigation();
    maintenanceReaderFixtures(); airshipRuntimeFixtures(); baajNavigationFixtures(); airshipCidFixtures(); sinNativeFixtures(); sinPlatformFixtures(); airshipDestinationFixtures(); reportedEventRuntimeFixtures(); arenaCaptureFixtures(); helpCaptureFixtures();
    set<int>(win+0x28,1); set<uint16_t>(win+0x30,2); set<int8_t>(win+0x45,0);
    set<uint8_t>(win+0x42,1); set<uint8_t>(win+0x43,0); set<int>(win+0x94,4);
    set<uint16_t>(0x1197730,1); set<uint8_t>(0x1197732,3);
    set<uint16_t>(0x1197734,2); set<uint8_t>(0x1197736,4);
    scene=3;
    event(0x4c6da0,address(win),[]{set<int>(win+0x28,2);});
    expect(L"Potion, 3","items opening");
    auto count=said.size(); event(0x4c6da0,address(win)); check(said.size()==count,"items idle silence");
    event(0x4c6da0,address(win),[]{set<int>(win+0x58,1);}); expect(L"Ether, 4","second column");
    keys['I'](false); expect(L"Restores MP","manual item description with Autodetail off");
    setSetting(Setting::AutoDetail,1);
    count=said.size(); event(0x4c6da0,address(win),[]{set<int>(win+0x58,0);});
    check(said.size()==count+2 && said[count].first==L"Potion, 3","one highlight and one automatic item description");
    expect(L"Restores HP","automatic item description");
    setSetting(Setting::AutoDetail,0); set<int>(win+0x58,1);
    set<uint16_t>(0x1196f32,0xa007);set<uint16_t>(0xd30f1c,0x81);
    event(0x4c6da0,address(win),[]{set<uint8_t>(win+0x43,1);}); expect(L"Key","separate key ordering");
    keys['I'](false);expect(L"Native key help","owned key items retain the actual native help field");
    event(0x4c6da0,address(win),[]{set<int>(win+0x58,0);});expect(L"Empty","zero key slot cannot become the first key-item record");
    keys['I'](false);expect(L"No native description is available for this selection.","blank key slot clears the prior item's help");
    count=said.size();event(0x4c6da0,address(win),[]{set<int16_t>(win+0x48,1);});
    check(said.size()==count+1 && said.back().first==L"Empty","two separate blank key slots both speak");
    setSetting(Setting::AutoDetail,1);
    event(0x4c6da0,address(win),[]{set<uint16_t>(0x1196f34,0xa000);});expect(L"Withered Bouquet","nonzero owned raw key with definition index0 remains a real item");
    count=said.size();event(0x4c6da0,address(win),[]{set<uint16_t>(0xd30f1c,0x80);});
    check(said.size()==count+1 && said.back().first==L"Empty","unowned nonzero key matches native blank rendering, without automatic hidden help");
    event(0x4c6da0,address(win),[]{set<uint16_t>(0x1196f34,0xa080);});expect(L"Empty","native key ownership limit rejects an out-of-range definition");
    count=said.size();event(0x4c6da0,address(win),[]{set<int16_t>(win+0x48,2);});check(said.size()==count,"key cursor outside native row count stays silent");
    setSetting(Setting::AutoDetail,0);set<int16_t>(win+0x48,0);set<int>(win+0x58,1);set<uint16_t>(0xd30f1c,0x81);
    event(0x4c6da0,address(win),[]{set<int>(win+0x28,1);});
    event(0x4c6da0,address(win),[]{set<int>(win+0x28,2);}); expect(L"Key","return from item use");
    encoded(0x2100160,"Use"); set<int>(win+0x28,4);
    set<uintptr_t>(win+0x58,address(0x2100160)); set<uintptr_t>(win+0x64,address(0x2100160));
    event(0x4e3880,address(win),[]{set<int>(win+0x28,5);}); expect(L"Use","bar opening");
    count=said.size(); event(0x4e3880,address(win),[]{set<int16_t>(win+0x48,1);});
    check(said.size()==count+1,"same label in different rows");
    scene=4; set<int16_t>(win+0x48,0); set<int>(win+0x28,1); set<uintptr_t>(win+0x10,address(0x4cf9c0));
    event(0x4b4460,address(win),[]{set<int>(win+0x28,2);}); expect(L"Sonic Steel","equipment native name");
    set<uint8_t>(0x210104b,2); set<uint16_t>(0x210104e,1); set<uint16_t>(0x2101050,2);
    event(0x4b4460,address(win),[]{set<int>(win+0x28,1);});
    event(0x4b4460,address(win),[]{set<int>(win+0x28,2);}); expect(L"Sonic Steel","equipment detail off keeps highlight name");
    keys['I'](false); check(said.back().first.find(L"Potion, Ether")==0,"manual ordered equipment abilities");
    setSetting(Setting::AutoDetail,1); event(0x4b4460,address(win),[]{set<int>(win+0x28,1);});
    count=said.size(); event(0x4b4460,address(win),[]{set<int>(win+0x28,2);});
    check(said.size()==count+2 && said.back().first.find(L"Potion, Ether")==0,"Autodetail owns equipment narration exactly once");
    setSetting(Setting::AutoDetail,0);
    set<uint16_t>(0x1197734,0); set<uint8_t>(0x1197736,0); set<int16_t>(win+0x48,0);
    event(0x4b4460,address(win),[]{set<int16_t>(win+0x48,1);});
    expect(L"Unequip armor","native final armor row cannot be decoded as equipment record0 and speak a weapon");
    keys['I'](false); expect(L"Remove the selected character's armor.","unequip action has its own instruction, never weapon abilities");
    set<int16_t>(win+0x48,0); // terminal armor row must not leak into later Learn/Increase windows
    set<int>(win+0x1c,5); set<int>(0x147151c,1); set<int>(0x1471520,0);
    for(unsigned i=0;i<3;++i)set<uint8_t>(0x24b0000+i,static_cast<uint8_t>(i));
    event(0x4e0340,address(win),[]{set<int>(win+0x1c,0x0e);}); expect(L"Yuna, active party","formation first stage");
    event(0x4e0340,address(win),[]{set<int>(win+0x1c,0x14);}); expect(L"Tidus, active party","formation swap stage");
    formationActiveFixtures();
    scene=7; set<int>(win+0x28,2);
    event(0x4ccea0,address(win),[]{set<int>(win+0x28,3);}); expect(L"Valefor","aeon selector");
    const auto aeonSave=0xd3205c+8*0x94;
    set<unsigned>(aeonSave+0x1c,1877);set<unsigned>(aeonSave+0x24,1917);
    set<unsigned>(aeonSave+0x20,50);set<unsigned>(aeonSave+0x28,60);
    set<uint16_t>(0x146a9e4,1);set<uint16_t>(0x146a9e8,2);
    for(unsigned i=0;i<3;++i)set<uint16_t>(0x146a9c4+i*2,static_cast<uint16_t>(0x2001+i));
    count=said.size(); event(0x4ccb30,address(win),[]{set<int>(win+0x28,4);});
    check(said.size()==count+1 && said[count].first==L"Valefor","Aeon Status AutoDetail Off opens with just its name");
    check(contexts.at(VK_DOWN)(),"Aeon Status owns the shared arrow buffer");
    keys[VK_DOWN](false);expect(L"HP: 1877/1917","Aeon HP uses displayed current/max instead of maximum alone");
    keys[VK_DOWN](false);expect(L"MP: 50/60","Aeon MP uses displayed current/max");
    keys[VK_DOWN](false);expect(L"Special Attack: Potion","native special count owns special-attack labels");
    keys[VK_DOWN](false);expect(L"Overdrive: Ether","appended native Overdrive count is not omitted");
    keys['I'](false);expect(L"No native description is available for this selection.","Aeon buffer does not invent ability help");
    count=said.size(); event(0x4ccb30,address(win)); check(said.size()==count,"Aeon attributes idle");
    for(unsigned page=1;page<=4;++page) {
        event(0x4ccb30,address(win),[=]{set<int>(win+0x28,static_cast<int>((page+3)*2));});
        constexpr unsigned pageWindow=0x2470000;
        set<int>(pageWindow+0x64,1);set<int>(pageWindow+0x60,page);
        Event draw;draw.rva=0x4d2760;draw.epoch=generation();draw.args[0]=address(pageWindow);
        const auto n=said.size();
        for(const auto& l:listeners[draw.rva])if(l.enter)l.enter(draw);
        for(const auto& l:listeners[draw.rva])if(l.leave)l.leave(draw);
        drainSpeechDomainsForTest();
        const wchar_t* titles[]={L"",L"Skills",L"Special",L"White Magic",L"Black Magic"};
        check(said.size()==n+1 && said.back().first==std::wstring(L"Valefor. ")+titles[page]+L". No learned abilities","each empty Aeon category announces the actual active page");
        keys[VK_UP](false);expect(L"No learned abilities","empty category is an actual review-buffer row");
        count=said.size();
        for(const auto& l:listeners[draw.rva])if(l.enter)l.enter(draw);
        for(const auto& l:listeners[draw.rva])if(l.leave)l.leave(draw);
        drainSpeechDomainsForTest();check(said.size()==count,"idle rendering never repeats an empty-page announcement");
    }
    event(0x4ccb30,address(win),[]{set<int>(win+0x28,15);});
    check(!contexts.at(VK_DOWN)(),"Aeon buffer closes before returning to Learn/Increase");
    set<uintptr_t>(win+0x10,address(0x4cdb70)); set<uint8_t>(0x1197733,0);
    setSetting(Setting::AutoDetail,1); // requirement detail case explicitly owns its preference
    set<uint16_t>(0x2110104,1); set<uint8_t>(0x2110106,5); set<int>(win+0x28,1);
    event(0x4b4460,address(win),[]{set<int>(win+0x28,2);});
    expect(L"Potion, 5 required, 3 available","Learn native requirements");
    // Increase ignores the draw callback identity; use an opaque fixture callback.
    set<int>(win+0x28,1); set<uintptr_t>(win+0x10,address(0x2102000));
    event(0x4cd6a0,address(win),[]{set<int>(win+0x28,2);});
    expect(L"Potion, 5 required, 3 available","Increase prompt queued after stat label");
    check(!said.back().second && said[said.size()-2].first==L"Strength","Increase prompt ordering");
    setSetting(Setting::AutoDetail,0); // subsequent focus/manual Grid cases explicitly use headline-only mode
    set<int>(0x8416b0,0xb); set<uintptr_t>(0xd2a01c,address(0x2210000));
    set<uintptr_t>(0xd2a03c,address(0x2220000)); set<int>(0xd2a080,1);
    set<int16_t>(0x2210004,1); set<int16_t>(0x2220018,-1);
    set<uintptr_t>(0xd2a024,address(0x2210100)); set<uintptr_t>(0xd2a040,address(0x2210200));
    encoded(0x2210300,"Help page"); event(0x375df0,0);
    count=said.size();
    auto helpRender=[&] {
        event(0x36f960,0,[&] {
            for(auto listener:listeners[0x376a50]) if(listener.leave) {
                Event e;e.epoch=generation();e.parentRva=0x36f960;
                e.args[0]=address(0x2210300);e.args[1]=address(0x2210200);e.args[4]=address(0x2210400);listener.leave(e);
            }
            Event glyph;glyph.epoch=generation();glyph.parentRva=0x36f960;glyph.args[2]=address(0x2210300);glyph.args[3]=50;glyph.args[4]=60;
            for(auto listener:listeners[0x4e52e0])if(listener.leave)listener.leave(glyph);
        });drainSpeechDomainsForTest();drainCopiedTextForTest();
    };
    helpRender();check(said.size()==count+1,"natural Help decode");expect(L"Help page","Help content");
    helpRender();check(said.size()==count+1,"Help idle silence");set<int>(0x8416b0,0);
    set<uintptr_t>(0x1686108,address(pool)); set<int16_t>(pool+0x2700,7);
    set<uintptr_t>(pool+0x2704,address(0x656330)); set<uint8_t>(sphere+0x23,1);
    set<int16_t>(sphere+0x1e,2); set<uintptr_t>(sphere+0x40,address(0x2100140));
    set<uintptr_t>(sphere+0x4c,address(0x2100160));
    event(0x6583e0,0,[]{set<int16_t>(sphere+0x18,1); set<int16_t>(sphere+0x1a,1);});
    expect(L"Use","sphere full row excludes scroll offset");
    count=said.size(); event(0x6583e0,0); check(said.size()==count,"sphere idle silence");
    constexpr unsigned grid=0x2320000;
    set<int16_t>(grid+2,2); set<int16_t>(pool+0x2700,-1);
    set<uintptr_t>(0x1f05834,address(grid)); set<uintptr_t>(0x16860e0,address(0x2310400));
    encoded(0x2310200,"Strength Node"); encoded(0x2310240,"Increases Strength by 4");
    set<uint16_t>(0x2310000,0); set<uint16_t>(0x2310008,0x40);
    set<uint16_t>(grid+0x112fc,1); set<uint16_t>(grid+0x112fe,0xabcd);
    set<uint8_t>(grid+0x115bc,0); set<uint8_t>(grid+0x829+0x28,2);
    event(0x653570,0,[]{set<uint8_t>(0x1685f70,1);});
    expect(L"Strength Node, node 2","native node getter and signed index ignore adjacent field");
    keys['I'](false); expect(L"Increases Strength by 4","native grid description");
    encoded(0x2310240,"Increases Strength\nby 4");
    keys['I'](false); expect(L"Increases Strength by 4","grid description reads all native lines");
    set<uint16_t>(0x2310100+0x12,1); set<uint16_t>(grid+0x80e+0x28,1);
    keys['I'](false); expect(L"Restores HP","ability node resolves actual command description without waiting for activation");
    set<uint16_t>(0x2310100+0x10,0x400);
    set<uint16_t>(0x2310140+0x10,0x8000); set<uint8_t>(0x2310140+0x16,0x12);
    set<uint16_t>(0x2310c00+0xa,0x400); set<uint8_t>(0x2310c40+0xd,1);
    encoded(0x2310280,"Activates ability nodes"); set<uint16_t>(0x2310c00,0x80);
    set<uintptr_t>(0x16860e4,address(0x2310b00));
    set<int16_t>(grid+4,1); set<int16_t>(grid+0xa808,0); set<int16_t>(grid+0xa80a,1);
    GridSnapshot snapshot;
    check(sphereSnapshot(snapshot,true) && snapshot.nodes[1].sphere==0x2049,"native sphere catalog resolves ability material even when inventory is empty");
    check(sphereItemDescription(0x2049)==L"Activates ability nodes","sphere inventory description uses sphere-specific native text");
    set<uint16_t>(grid+0x80e,2);
    check(sphereSnapshot(snapshot,true) && snapshot.nodes[0].sphere==0x2051 && snapshot.nodes[0].locked,"native lock availability type resolves corresponding key");
    set<uint16_t>(grid+0x80e,0);
    set<uint16_t>(grid+0x80e+0x28,0);
    Event activation; activation.rva=0x648910; activation.args[0]=0; activation.args[1]=1;
    listeners[0x648910][0].enter(activation); set<uint8_t>(grid+0x829+0x28,3); listeners[0x648910][0].leave(activation);
    expect(L"Tidus: Strength Node activated","activation belongs to actual character bit");
    gridStatFixture=true; set<uint8_t>(0xd3205c+0x2f,12); set<uint8_t>(grid+0x829+0x28,2);
    activation={}; activation.rva=0x648910; activation.args[0]=0; activation.args[1]=1;
    listeners[0x648910][0].enter(activation); set<uint8_t>(grid+0x829+0x28,3); set<uint8_t>(0xd3205c+0x2f,16);
    listeners[0x648910][0].leave(activation);
    expect(L"Tidus: Strength Node activated. Strength 12 to 16","activation reads native before/after stat values");
    gridStatFixture=false;
    event(0x4a9810,0,[=]{set<uint8_t>(grid+0x115bc,1);});
    expect(L"Strength Node, activated for Yuna, node 2","character switch refreshes node activation information");
    set<uint8_t>(0x1685f70,0); resetSelectionDetail();
    encoded(0x2100180,"Proceed"); encoded(0x21001a0,"Yes"); encoded(0x21001c0,"No");
    set<int>(win+0x28,0); set<int16_t>(win+0x48,1);
    set<uintptr_t>(win+0x24,address(0x2100180)); set<uintptr_t>(win+0x58,address(0x21001a0));
    set<uintptr_t>(win+0x5c,address(0x21001c0));
    event(0x4e2e20,address(win),[]{set<int>(win+0x28,1);});
    expect(L"No","confirmation native default"); check(!said.back().second,"prompt precedes queued choice");
    set<int>(win+0x28,1); set<int16_t>(win+0x48,1);
    set<uintptr_t>(0x146abec,address(0x2110000)); set<uint16_t>(0x2110000,2);
    set<uint16_t>(0x2110002,1); set<uint16_t>(0x2110004,2);
    event(0x4d9d70,address(win),[]{set<int>(win+0x28,2);}); expect(L"Ether, 4, 50 Gil","native shop order price and quantity");
    set<uint16_t>(win+0x5c,1); set<int>(win+0x60,50);
    event(0x4d9ba0,address(win),[]{set<int>(win+0x28,3); set<int16_t>(win+0x48,10);});
    expect(L"Potion, 10, 500 Gil","shop quantity input");
    set<uint16_t>(0x2210100,0x218); event(0x28e6e0,address(0x2210100)); expect(L"Return to game","ESC native localization");
    // Native config names/values are read through the game's text getter.
    set<uintptr_t>(0x146a40c,address(0x2211000)); set<int>(0x146a408,1); set<int>(0x146a3f8,-1);
    set<uintptr_t>(0x2211000,address(0x2211100)); set<int>(0x2211104,2);
    set<uint16_t>(0x2211118,1); set<unsigned>(0x221111c,2); set<unsigned>(0x2211120,1);
    set<int>(win+0x1c,3); event(0x4cb0b0,address(win)); expect(L"Sound: Stereo","config initial setting and value");
    event(0x4cb0b0,address(win),[]{set<int>(0x2211108,1);}); expect(L"Sound","config changed value");
    encoded(0xf3f4f8,"Help page"); keys['N'](false); expect(L"No native description is available for this selection.","field I rejects stale battle help buffer");
    // Save popup focus queues behind the body, including reopening on the
    // same initial choice. Slot data is copied before its shared buffer changes.
    strcpy_s(reinterpret_cast<char*>(address(0x8eb690)),256,"Save");
    strcpy_s(reinterpret_cast<char*>(address(0x8eb890)),64,"Yes");
    strcpy_s(reinterpret_cast<char*>(address(0x8eba90)),64,"No");
    event(0x302fe0,0); event(0x2efd90,0); expect(L"No","save initial choice");
    check(!said.back().second,"save body survives initial focus");
    set<int>(0x8e72f0,1); event(0x2efd90,0); expect(L"Yes","save moved choice");
    check(said.back().second,"moved save choice interrupts");
    set<int>(0x8e72f0,0); event(0x302fe0,0); event(0x2efd90,0); expect(L"No","save popup reopens");
    strcpy_s(reinterpret_cast<char*>(address(0x2211200)),64,"Besaid");
    strcpy_s(reinterpret_cast<char*>(address(0x2211240)),64,"00:10");
    Event slot; slot.args[0]=8; slot.args[2]=address(0x2211200); slot.args[5]=address(0x2211240);
    listeners[0x302e70][0].enter(slot);
    strcpy_s(reinterpret_cast<char*>(address(0x2211200)),64,"Changed");
    event(0x302db0,8); expect(L"Slot 8: Besaid, 00:10","copied save slot");
    readerResourceLoading=true;slot.args[0]=198;listeners[0x302e70][0].enter(slot);
    readerResourceLoading=false;event(0x302db0,198);
    expect(L"Slot 198 empty","snapshot slot producer retains loading guards");
    scene=6; set<int>(win+0x1c,4); set<unsigned>(0xd3205c+0x1c,100); set<unsigned>(0xd3205c+0x24,200);
    event(0x4d1dd0,address(win)); keys[VK_OEM_PERIOD](false); expect(L"HP: 100/200","status review");
    // ESC Audio's native setter retains the game's own label identity.
    set<uintptr_t>(0x1efb504,address(0x2211300)); set<unsigned>(0x83962c,4);
    set<unsigned>(0x839624,0x222);
    Event audio; audio.ecx=address(0x2211300); audio.args[0]=4; audio.args[1]=7;
    listeners[0x2a00][0].enter(audio); set<unsigned>(0x2211310,7); listeners[0x2a00][0].leave(audio);
    expect(L"Return to game: 7","native Audio setter label and value");
    set<int16_t>(0x2210104,110); set<int16_t>(0x2210106,0);
    event(0x28e6e0,address(0x2210100)); expect(L"Return to game: 7","ESC audio focus uses native row value");
    strcpy_s(reinterpret_cast<char*>(address(0x2310500)),32,"Key three"); strcpy_s(reinterpret_cast<char*>(address(0x2310520)),32,"Key zero");
    Event focus; listeners[0x28e730][0].enter(focus);
    Event focusValue; focusValue.args[0]=0; focusValue.result=0x460; listeners[0x628b20][0].leave(focusValue);
    focusValue.args[0]=1; focusValue.result=0; listeners[0x628b20][0].leave(focusValue);
    listeners[0x28e730][0].leave(focus); expect(L"Return to game: Key zero","keyboard row reads native key catalog");
    event(0x301760,0,[]{set<unsigned>(0x2211300+0x60,3);});
    expect(L"Return to game: Key three, Key zero","native rebinding reports changed row and both key columns");
    set<uint8_t>(0xd2a8e0,1); set<uintptr_t>(0xd334cc,address(0x2120000));
    encoded(0x2120540,"Tidus"); set<uint8_t>(0x2120dc8,1);
    for(auto listener:listeners[0x39fa20]) if(listener.leave) {
        Event e; e.args[1]=1; e.args[2]=static_cast<uintptr_t>(-20); listener.leave(e);
    }
    expect(L"Tidus: restored 20 MP","MP restoration");
    set<uint8_t>(0xf3c911,4); event(0x4a0d70,0); expect(L"Tidus","battle turn opening");
    keys['T'](false); expect(L"Tidus","turn hotkey");
    set<unsigned>(0x21205d0,100);set<unsigned>(0x21206e4,100); set<unsigned>(0x2120594,200);
    set<unsigned>(0x21205d4,17);set<unsigned>(0x21206e8,17); set<unsigned>(0x2120598,30);
    setSetting(Setting::ShowMaximumHp,0);
    setSetting(Setting::ShowMaximumMp,0);
    keys['H'](false); expect(L"Tidus HP 100 (50%) MP 17. Status: None","H defaults to current MP displayed by the battle panel");
    setSetting(Setting::ShowMaximumMp,1);
    keys['H'](false); expect(L"Tidus HP 100 (50%) MP 17/30. Status: None","maximum MP is independently opt-in");
    setSetting(Setting::ShowMaximumMp,0); setSetting(Setting::ShowMaximumHp,1);
    keys['H'](false); expect(L"Tidus HP 100/200 MP 17. Status: None","maximum HP does not enable maximum MP");
    setSetting(Setting::ShowMaximumHp,0);
    activeBattleFixtures(); battleHudFixtures(); battleSwitchFixtures(); battleOutcomesFixtures(); statusAnimationFixtures();battleAmountFixtures();seymourFixtures();overdriveLearnFixtures();battleHpFixtures();
    set<uintptr_t>(0xf3c930,address(0x2211400)); set<unsigned>(0xf3c934,2); set<unsigned>(0xf3c954,1);
    set<uint16_t>(0x2211400,1); set<uint16_t>(0x2211402,2); set<uint16_t>(0xf3c92e,1);
    event(0x49c870,address(0xf3c910)); expect(L"Potion","battle command");
    count=said.size(); event(0x49c870,address(0xf3c910)); check(said.size()==count,"unchanged battle cursor");
    event(0x49c870,address(0xf3c910),[]{set<uint8_t>(0xf3c952,1);}); expect(L"Ether","battle command move");
    protectSpeech(false);
    event(0x49d300,address(0xf3c910),[]{set<uint16_t>(0xf3c9e8,1);}); expect(L"Tidus: MP 17/30","party MP target preview");
    check(said.back().second,"target movement interrupts after deliberate input, including held menu navigation");
    event(0x38f0b0,0,[]{set<uint16_t>(0x2120606,8);}); expect(L"Tidus: poison","first application status");
    event(0x38f0b0,0,[]{set<uint16_t>(0x2120606,0);}); expect(L"Tidus: poison cured","status cured");
    event(0x38f0b0,0,[]{set<uint8_t>(0x212060d,5);set<uint8_t>(0x2120613,5);}); expect(L"Tidus: reflect, haste","temporal statuses");
    event(0x38f0b0,0,[]{set<uint8_t>(0x212060d,4);set<uint8_t>(0x2120613,4);});
    count=said.size(); event(0x38f0b0,0); check(said.size()==count,"status turn decrement is not a new application");
    event(0x38f0b0,0,[]{set<uint8_t>(0x212060d,0);set<uint8_t>(0x2120613,0);}); expect(L"Tidus: reflect ended, haste ended","temporal cures");
    event(0x38f0b0,0,[]{set<uint8_t>(0x212065e,1);}); expect(L"Tidus: Cheer 1","stacking battle buff");
    set<uint8_t>(0x2120df0,3); event(0x498bd0,1,{},1); expect(L"Tidus: Potion","action announcement");
    event(0x498bd0,1,{},0); expect(L"Tidus: Potion","silent native action title still gets spoken");
    Event special; special.result=1; special.args[0]=4; special.args[1]=2; special.args[2]=3;
    listeners[0x39cff0][0].leave(special); expect(L"Ether x3","special item ability speaks exact quantity");
    special.args[0]=6; special.args[1]=1; special.args[2]=2; listeners[0x39cff0][0].leave(special);
    expect(L"Yuna: Ether","special learned ability speaks native character and command");
    encoded(0x2310600,"Miss"); Event battleMessage; battleMessage.args[0]=0; battleMessage.args[1]=address(0x2310600);
    listeners[0x49db10][0].enter(battleMessage); expect(L"Miss","native battle failure message");
    battleDebuffFixtures();
    phasedResultsFixtures();
    sensorFixtures(); overdriveFixtures();
    setSetting(Setting::OverdriveAssist,0); set<int>(0xf3d6f4,4); event(0x492290,0);
    check(read<int>(address(0xf3d6f4))==4,"disabled Overdrive menu setting preserves state");
    setSetting(Setting::OverdriveAssist,1); event(0x492290,0);
    check(read<int>(address(0xf3d6f4))==5 && read<uint8_t>(address(0xf3d6b3))==1,"existing Tidus overdrive assist");
    check(keys.count('N') && keys.count('H') && keys.count('S') && keys.count(VK_OEM_COMMA) && keys.count(VK_OEM_PERIOD),"all review hotkeys");
    set<uint8_t>(0xd2a8e0,0); resetNavigation();
    keys[VK_OEM_5](false); expect(L"No entity selected","navigation empty selection feedback");
    constexpr unsigned actors=0x2240000, npc=actors+0x880;
    set<uintptr_t>(0x1fc44e4,address(actors)); set<unsigned>(0x1fc44e0,2);
    set<uint16_t>(actors+2,1); set<uint16_t>(npc,0x2002); set<uint16_t>(npc+2,1);
    set<float>(npc+0xc,120); set<uintptr_t>(npc+4,address(0x2211600));
    strcpy_s(reinterpret_cast<char*>(address(0x2211600)),64,"chr_yuna");
    // One real talk entry bound to the actor, plus an unrelated ambient model.
    // Asset names alone must not identify an NPC or include decorations.
    constexpr unsigned ctrl=0x2250000, worker=0x2251000, chunk=0x2252000;
    stub(0x46a7c0,reinterpret_cast<void*>(navigationWorker));
    set<uintptr_t>(0xf26ae8,address(ctrl)); set<uint16_t>(ctrl+0xc,1);
    set<uint8_t>(ctrl,2);set<uintptr_t>(ctrl+0x48,address(chunk));set<uintptr_t>(ctrl+0x2c,address(0xd2ca90)+0x1ec);set<uintptr_t>(0xf00740,address(actors));
    set<uintptr_t>(worker,address(chunk+0x200)); set<uintptr_t>(worker+4,address(chunk));
    set<uintptr_t>(worker+0x9c,address(npc));
    set<unsigned>(chunk,10); set<unsigned>(chunk+0xc,0x80); set<unsigned>(chunk+0x30,0x100);
    set<uint16_t>(chunk+0x34,1); set<unsigned>(chunk+0x38,0x200);
    strcpy_s(reinterpret_cast<char*>(address(chunk+0x80)),9,"fixture0");
    set<uint8_t>(chunk+0x200,1); set<uint16_t>(chunk+0x208,3); set<unsigned>(chunk+0x220,0x240);
    const unsigned char talk[]={0xae,0,0,0xae,0,0,0xb5,0x64,0,0x34,0x3c};
    set<unsigned>(chunk,static_cast<unsigned>(sizeof(talk)));set<unsigned>(chunk+0x240+7*4,10); // Explicit no-op RET for collected-chest remapping.
    memcpy(reinterpret_cast<void*>(address(chunk+0x100)),talk,sizeof(talk));
    prepareInteractionFixture(worker,ctrl);fixturePhysicalPart(chunk,0x1000);
    set<unsigned>(0x1fc44e0,3); set<uint16_t>(npc+0x880,0xf117); set<uint16_t>(npc+0x882,1);
    set<float>(npc+0x88c,60);
    count=said.size(); keys[VK_OEM_3](false);navigationFixturePump();
    check(said.size()==count+2 && said[count].first==L"1 All","navigation scan count queues before selection");
    expect(L"Villager, 10.8 steps unknown, 1 of 1","navigation native name and distance");
    setSetting(Setting::HideUnreachable,0); count=said.size(); keys['U'](false);navigationFixturePump();
    check(setting(Setting::HideUnreachable)==1 && said.size()==count+2 && said[count].first==L"Hide unreachable destinations: On. Reachability unavailable; destinations remain listed","U enables the saved filter and retains destinations with unknown collision");
    expect(L"Villager, 10.8 steps unknown, 1 of 1","filter preserves the selected destination");
    keys['U'](false);navigationFixturePump(); check(setting(Setting::HideUnreachable)==0,"U restores all destinations");
    mapControlsFixtures(); navigationIdentityRuntimeFixtures();
    setSetting(Setting::PartialSteps,0);
    keys[VK_OEM_5](false); expect(L"Villager: collision data unavailable. Direct distance 11 unknown","navigation no-mesh path fallback uses concise whole distances");
    setSetting(Setting::PartialSteps,1);
    keys[VK_OEM_5](false); expect(L"Villager: collision data unavailable. Direct distance 10.8 unknown","partial distances also apply to direct path fallback");
    setSetting(Setting::PartialSteps,0); navigationFormatFixtures();
    keys['P'](false); expect(L"Villager: 10.8 steps unknown","navigation distance key");
    set<uintptr_t>(0x1f11430,address(0x2311000)); set<float>(0x2311040,1);
    double pan=0,front=0; check(relativeBearing({0,0,-1},pan,front) && pan==1 && front==0,"camera-right matches spoken east and audio pan");
    check(direction({0,0,-1})==L"east","spoken and audio direction share camera basis");
    set<uintptr_t>(0xf00740,address(npc)); check(playerActor()==address(npc),"actual controlled actor can occupy a nonzero slot");
    set<uintptr_t>(0xf00740,address(actors)); set<uintptr_t>(0x1f11430,0);
    set<unsigned>(actors+0x194,0x200); set<float>(actors+0x4c,1);
    scene=0;
    const auto fieldUpdates=navUpdates,fieldTargets=targetUpdates,fieldWorkers=workerReads;
    set<uint8_t>(0xd2a8e0,1); event(0x43d3b0,0); event(0x42b500,address(npc));
    check(navUpdates==fieldUpdates && targetUpdates==fieldTargets && workerReads==fieldWorkers,"battle animation callbacks do no field-navigation work");
    set<uint8_t>(0xd2a8e0,0); scene=4; event(0x43d3b0,0);
    check(navUpdates==fieldUpdates && targetUpdates==fieldTargets && workerReads==fieldWorkers,"native menus do no field-navigation work");
    scene=0; event(0x43d3b0,0); expect(L"Movement blocked","collision refusal gives concrete movement feedback");
    count=said.size(); event(0x43d3b0,0); check(said.size()==count,"held obstruction is one event edge");
    event(0x43d3b0,0,[=]{set<float>(actors+0xc,2);});
    event(0x43d3b0,0); expect(L"Movement blocked","new collision refusal after movement is spoken again");
    set<float>(actors+0xc,0);
    keys[VK_OEM_4](true); expect(L"category: Object, 0","previous category wrap");
    keys[VK_OEM_4](true); expect(L"Villager, 10.8 steps unknown, 1 of 1","NPC category selection");
    event(0x425010,0,{},address(npc));
    event(0x426770,address(npc),[=]{set<uint16_t>(npc+2,0);});
    keys[VK_OEM_5](false); expect(L"No entity selected","actor disposal invalidates navigation");
    navigationInteractionFixtures();
    resetMessages(); resetSystemMenus(); resetBattle(); resetSphereGrid();
    set<int16_t>(0x2210104,0); set<int16_t>(0x2210106,0);
    event(0x28e6e0,address(0x2210100)); expect(L"Return to game","teardown clears stale ESC binding-help ownership");
    set<uint8_t>(0x2101040+4,0); set<uint8_t>(0x2101040+5,0); set<uint8_t>(0x2101040+0xb,2);
    set<uint16_t>(0x2101040+0xe,2); set<uint16_t>(0x2101040+0x10,0xff);
    // Index 254 returns a separate existing piece for the comparison fixture.
    set<uint8_t>(0x146ab60,0);
    set<unsigned>(0xd3205c+0x20,17); set<unsigned>(0xd3205c+0x28,30);
    set<uint8_t>(0xd3205c+0x2d,254);
    set<uint8_t>(0x2310800+0xb,1); set<uint16_t>(0x2310800+0xe,1);
    check(equipmentDetails(address(0x2101040))==L"Ether, 1 empty slot","equipment ability and empty slot details");
    check(shopEquipmentDescription(address(0x2101040))==L"Tidus. HP 100/200, MP 17/30. Current equipment: Sonic Steel, Potion. Selected equipment: Ether, 1 empty slot","shop reads both rendered panels without catalog effects or computed comparisons");
    check(equipmentDescription(address(0x2101040))==L"Ether, 1 empty slot","equipment descriptions contain visible abilities once");
    visibleEquipmentFixtures();
    // Description owners: actual native strings and distinct selection events,
    // both manual and automatic; equal text on another row remains speakable.
    encoded(0x2100300,"Native command bar help");
    set<int>(win+0x28,4); set<int16_t>(win+0x48,0); set<uint16_t>(win+0x30,2); set<int8_t>(win+0x45,0);
    set<uintptr_t>(win+0x5c,address(0x2100300)); set<uintptr_t>(win+0x68,address(0x2100300));
    event(0x4e3880,address(win),[]{set<int>(win+0x28,5);}); keys['I'](false); expect(L"Native command bar help","command bar native help pointer");
    setSetting(Setting::AutoDetail,1); count=said.size();
    event(0x4e3880,address(win),[]{set<int16_t>(win+0x48,1);});
    check(said.size()==count+2,"equal command bar descriptions belong to distinct rows");
    encoded(0x2100140,"Items\n-\nManage your items");
    set<int>(win+0x1c,0); set<int>(0x1471508,0); set<int>(0x1471528,0);
    event(0x4e0340,address(win),[]{set<int>(win+0x1c,5);}); expect(L"Manage your items","main menu native description");
    set<uint16_t>(0x210104e,0x8001); set<uint16_t>(0x2101050,0x8002); set<uint8_t>(0x210104b,2);
    const auto gearDescription=equipmentDescription(address(0x2101040));
    check(gearDescription==L"Potion, Ether","equipment names match visible rows without expanded catalog effects");
    scene=4; set<uintptr_t>(win+0x10,address(0x4cf9c0)); set<int>(win+0x28,1); set<int16_t>(win+0x48,0);
    count=said.size(); event(0x4b4460,address(win),[]{set<int>(win+0x28,2);});
    check(said.size()==count+2 && said.back().first==gearDescription,"single gear description producer");
    setSetting(Setting::AutoDetail,0);
    std::memset(reinterpret_cast<void*>(address(win)),0,0x100);
    set<int>(win+0x28,0); set<int16_t>(win+0x48,0);
    set<uintptr_t>(win+0x24,address(0x2100180)); set<uintptr_t>(win+0x58,address(0x21001a0)); set<uintptr_t>(win+0x5c,address(0x21001c0));
    count=said.size();event(0x4e2e20,address(win),[]{set<int>(win+0x28,4);});
    check(said.size()==count,"confirmation phase guard rejects an undisplayed opening prompt");
    set<int>(win+0x28,0);
    event(0x4e2e20,address(win),[]{set<int>(win+0x28,1);});
    keys['I'](false); expect(L"Proceed","confirmation I retains the native prompt");
    setSetting(Setting::AutoDetail,1); count=said.size();
    event(0x4e2e20,address(win),[]{set<int16_t>(win+0x48,1);});
    check(said.size()==count+2 && said.back().first==L"Proceed","confirmation automatic detail follows moved choice");
    setSetting(Setting::AutoDetail,0);
    sphereReaderFixtures();
    sphereMovementFixtures();
    sphereOwnedFixtures();
    messageFixtures();
    followupFixtures();
    fieldMenuFixtures();
    swimmingFixtures();
    buttonGlyphFixtures();
    swimmingProgressionFixtures();
    exitLabelFixtures();airshipSearchFixtures();introCueFixtures();viaSwimmingFixtures();
    escFocusFixtures();specialFeatureNotificationFixtures();battleResultsBufferFixtures();realHelpPageFixtures();nativeHelpRendererFixture();gridNativeReplayFixture();customizeNativeReplayFixture();
    std::printf("PASS: production native menu, Help, sphere, battle and hotkey fixtures (%zu hook sites, %s profile).\n",listeners.size(),nativeProfile == NativeProfile::Installed ? "installed" : "reference");
    VirtualFree(reinterpret_cast<void*>(testBase),0,MEM_RELEASE);
    return 0;
    } catch (const std::exception& error) {
        std::fprintf(stderr, "FAIL: %s\n", error.what());
        return 1;
    }
}
