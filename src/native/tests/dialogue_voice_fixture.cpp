#include <atomic>
#include "../events.hpp"
#include "../dialogue_voice.hpp"
#include "../settings.hpp"
#include "../message_text.hpp"
#include "../text_plan.hpp"
#include "../blitz_review.hpp"
#include "../field_controls.hpp"
#include "test_copied_controls.hpp"
#include "ready_speech_adapter.hpp"
#include "test_jecht_caption.hpp"
#include <map>
#include <cstdio>
#include <cstdlib>
#include <cstring>
namespace ffx {
struct Listener { Handler enter,leave; };
std::map<unsigned,Listener> listeners;
uint8_t globals[0x1500000]{};
std::atomic<int> scene{0};
unsigned generation() { return 1; }
void installButtonGlyphs() {}
std::vector<std::wstring> said;
std::vector<int> modes;
uintptr_t address(unsigned rva) { return reinterpret_cast<uintptr_t>(globals)+rva; }
std::vector<std::wstring> decode(uintptr_t p,size_t,TextMode) { return {reinterpret_cast<const wchar_t*>(p)}; }
std::wstring first(uintptr_t p,size_t) { return reinterpret_cast<const wchar_t*>(p); }

BlitzSnapshot captureBlitzSnapshot(bool,bool) {return {};}
bool observeBlitzMessageOwned(unsigned,const MessagePage&,const BlitzSnapshot&) { return false; }
AtelPreparation blitzSnapshotPreparation(const BlitzSnapshot& s) { return s.valid?AtelPreparation::Ready:AtelPreparation::Retired; }
void releaseBlitzSnapshot(const BlitzSnapshot&) {}
bool blitzReviewAvailable() { return false; }
void installBlitzReview() {}
void resetBlitzReview() {}
FieldControlsSnapshot captureFieldControls(unsigned,uintptr_t) {return {};}
bool observeFieldControlsOwned(const MessagePage&,const FieldControlsSnapshot&) {return false;}
void installFieldControls() {}
void resetFieldControls() {}
void invalidateFieldControlsSlot(unsigned) {}
void rejectFieldControlsCapture(const FieldControlsSnapshot&) {}
std::wstring clean(std::wstring s) { return s; }
std::wstring join(const std::vector<std::wstring>& parts,const wchar_t* separator) {
    std::wstring result;
    for(const auto& p:parts) { if(!result.empty()) result+=separator; result+=p; } return result;
}
bool output(const std::wstring& s,bool,const char*) { said.push_back(s); modes.push_back(2); return true; }
bool outputDialogue(const std::wstring& s,bool,const char*,bool voiced) {
    said.push_back(s); modes.push_back(voiced?setting(Setting::VoicedDialogue):2); return true;
}
void protectTutorialSpeech(bool) {}
bool sphereGridOpen() { return false; }
void hotkeyChordWhen(unsigned,bool,void(*)(bool),bool(*)(),bool) {}
void installHelp() {}
void resetHelp() {}
void selectionDetail(const std::wstring&,bool) {}
SelectionDetailToken beginSelectionDetail() {return {generation(),1};}
void emitSelectionDetailOwned(std::wstring,SelectionDetailToken,bool,const speechq::Header&) {}
void appendSelectionDetailOwned(std::wstring,SelectionDetailToken) {}
std::wstring buttonGlyph(uint8_t) {return {};}
bool testForeground=true;
bool hook(unsigned id,Handler enter,Handler leave) { listeners[id]={enter,leave}; return true; }
bool copy(uintptr_t p,void* out,size_t n) {
    __try { memcpy(out,reinterpret_cast<void*>(p),n); return true; }
    __except(1) { return false; }
}
void log(const char*,...) {}
}
#include "test_speech_domains.hpp"
using namespace ffx;
void check(bool value,const char* message) { if(!value) { fprintf(stderr,"FAIL: %s\n",message); exit(1); } }
void enter(Event& e) { if(listeners[e.rva].enter) listeners[e.rva].enter(e); drainSpeechDomainsForTest(); }
void leave(Event& e) { if(listeners[e.rva].leave) listeners[e.rva].leave(e); drainSpeechDomainsForTest(); }
uintptr_t caption(const wchar_t* text) {
    static std::array<uint8_t,256> bytes{};bytes.fill(0);const auto encoded=encodeMovieCaption(text);
    check(!encoded.empty() && encoded.size()<=bytes.size(),"native encoded fixture caption fits");
    std::memcpy(bytes.data(),encoded.data(),encoded.size());return reinterpret_cast<uintptr_t>(bytes.data());
}
int main() {
    installTextPlans();installMessages();
    alignas(4) uint8_t object[0x90]{};
    auto owner=reinterpret_cast<uintptr_t>(object);
    write<uintptr_t>(owner+0x14,0x123456); write<float>(owner+0x50,127); write<float>(owner+0x88,1);
    auto start=[&](unsigned parent,bool success=true) {
        Event e; e.rva=parent; e.ecx=owner; enter(e);
        Event child; child.rva=0x548c9a; child.parentRva=parent; child.args[0]=read<uintptr_t>(owner+0x14); child.result=success?0:1;
        leave(child); leave(e);
    };
    auto change=[&](unsigned id) { Event e; e.rva=id; e.ecx=owner; leave(e); };
    check(!voicedDialogue(),"no voice defaults to normal speech");
    start(0x30b6b0,false); check(!voicedDialogue(),"failed FMOD start cannot suppress speech");
    start(0x30b6b0); check(voicedDialogue(),"successful immediate voice start");
    write<unsigned>(owner+0x80,1); change(0x30b420); check(!voicedDialogue(),"muted voice fallback");
    write<unsigned>(owner+0x80,0); change(0x30b420); check(voicedDialogue(),"unmute restores policy");
    write<float>(owner+0x88,0); change(0x30b3a0); check(!voicedDialogue(),"zero master/voice mix fallback");
    write<float>(owner+0x88,1); change(0x30b3a0);
    write<float>(owner+0x50,0); change(0x30b8e0); check(!voicedDialogue(),"zero individual voice volume fallback");
    write<float>(owner+0x50,127); change(0x30b8e0);
    write<unsigned>(owner+0x54,1); change(0x30b460); check(!voicedDialogue(),"paused voice fallback");
    write<unsigned>(owner+0x54,0); change(0x30b460); check(voicedDialogue(),"resumed voice policy");
    unsigned state=8; Event query; query.rva=0x548ca6; query.args[0]=0x999999; query.args[1]=reinterpret_cast<uintptr_t>(&state);
    state=1; leave(query); check(voicedDialogue(),"unrelated SFX state cannot end dialogue voice");
    query.args[0]=0x123456; leave(query); check(!voicedDialogue(),"native stopped/ready state ends voice");
    start(0x30b250); check(voicedDialogue(),"successful deferred/loading branch");
    Event callback; callback.rva=0x307b40; callback.args[0]=0x999999; callback.args[1]=2; callback.args[4]=owner+0x40;
    enter(callback); check(voicedDialogue(),"stale event completion cannot end new voice");
    callback.args[0]=0x123456; enter(callback); check(!voicedDialogue(),"native voice completion ends suppression");
    start(0x30b6b0); Event clear; clear.rva=0x30aec0; clear.ecx=owner; enter(clear);
    check(!voicedDialogue(),"prepare next voice clears previous line");
    start(0x30b6b0); resetDialogueVoice(); check(!voicedDialogue(),"map teardown clears voice identity");
    // A successful SFX/battle start outside FmodVoice cannot suppress dialogue.
    start(0x1234); check(!voicedDialogue(),"unrelated audio never arms policy");
    // Exercise both actual dialogue producers and a TK selector during voice.
    alignas(4) uint8_t window[0x100]{},cursor[8]{};
    auto w=reinterpret_cast<uintptr_t>(window),c=reinterpret_cast<uintptr_t>(cursor);
    write<uintptr_t>(address(0x14676b0),w); write<uintptr_t>(address(0x1468a50),c);
    Event npc; npc.rva=0x4b8980; npc.args[0]=0;
    Event subtitle; subtitle.rva=0x376a50;
    // Actual copied-message producer/central policy, with a controlled copied
    // SFX result. Native source/ticket/audio integration is in jecht_fixture.
    for(int mode=0;mode<3;++mode) {
        resetMessages();setSetting(Setting::VoicedDialogue,mode);
        testJechtCaption={generation(),85040,1,1};testJechtCaptionState={false,false};
        write<uintptr_t>(w+0x94,caption(L"JECHT TEST SUBTITLE"));
        const auto before=said.size();enter(npc);leave(npc);
        check(said.size()==before,"pending Jecht SFX subtitle does not queue ordinary speech ahead of cues");
        testJechtCaptionState={true,true};drainSpeechDomainsForTest();
        check(said.size()==before+1 && modes.back()==mode,"matching Jecht subtitle uses all three voiced output modes");
        resetMessages();testJechtCaptionState={true,true};
        subtitle.args[0]=caption(L"JECHT ENCODED SUBTITLE");leave(subtitle);
        check(modes.back()==mode,"encoded Jecht subtitle shares the exact SFX classification");
        resetMessages();write<uint8_t>(c+1,1);enter(npc);leave(npc);
        check(modes.back()==mode,"plain Jecht taunt cannot be misclassified by an inherited TK cursor");write<uint8_t>(c+1,0);
        resetMessages();testJechtCaptionState={true,false};
        enter(npc);leave(npc);check(modes.back()==2,"failed/missing/muted Jecht SFX uses ordinary speech");
        resetMessages();testJechtCaptionState={false,false};enter(npc);leave(npc);
        const auto cancelled=said.size();resetMessages();drainSpeechDomainsForTest();
        testJechtCaptionState={true,true};drainSpeechDomainsForTest();
        check(said.size()==cancelled,"reset retires pending Jecht subtitle without stale output");
    }
    testJechtCaption={};testJechtCaptionState={};
    for(int mode=0;mode<3;++mode) {
        resetMessages(); start(0x30b6b0); setSetting(Setting::VoicedDialogue,mode);
        write<uintptr_t>(w+0x94,caption(L"Voiced NPC line"));
        enter(npc); leave(npc); check(modes.back()==mode,"NPC dialogue receives selected voiced output");
        subtitle.args[0]=caption(L"Voiced subtitle"); leave(subtitle);
        check(modes.back()==mode,"event dialogue receives selected voiced output");
        resetDialogueVoice(); subtitle.args[0]=caption(L"Unvoiced subtitle"); leave(subtitle);
        check(modes.back()==2,"unvoiced event falls back in every output mode");
    }
    for(int mode=0;mode<3;++mode) {
        resetMessages(); setSetting(Setting::VoicedDialogue,mode);
        Event prepare; prepare.rva=0x30aec0; prepare.ecx=owner; enter(prepare);
        write<uintptr_t>(owner+0x14,0x123456); write<unsigned>(owner+0x60,1); leave(prepare);
        const auto n=said.size();
        subtitle.args[0]=caption(L"Caption before voice playback"); leave(subtitle);
        check(said.size()==n,"prepared voice defers early subtitle until actual playback result");
        start(0x30b6b0); check(said.size()==n+1 && modes.back()==mode,"later native voice start applies selected policy to copied caption");
        resetMessages(); enter(prepare); leave(prepare);
        subtitle.args[0]=caption(L"Failed voice caption"); leave(subtitle);
        start(0x30b6b0,false); check(modes.back()==2,"failed native voice start releases pending caption to normal speech");
        resetMessages(); enter(prepare); leave(prepare);
        subtitle.args[0]=caption(L"Cancelled map caption"); leave(subtitle);
        const auto before=said.size(); resetMessages(); start(0x30b6b0);
        check(said.size()==before,"teardown drops stale pending copied captions");
    }
    resetMessages();Event preparedCaption;preparedCaption.rva=0x30aec0;preparedCaption.ecx=owner;
    enter(preparedCaption);write<uintptr_t>(owner+0x14,0x123456);write<unsigned>(owner+0x60,1);leave(preparedCaption);
    subtitle.args[0]=caption(L"Caption canceled before later voice starts");leave(subtitle);
    const auto pendingBeforeCancel=said.size();++speechq::fixture::cancellation;
    start(0x30b6b0);
    check(said.size()==pendingBeforeCancel,"later voice start cannot revive a canceled pending caption with its fresh header");
    resetMessages();
    // Movie dialogue uses the same two real message producers, although its
    // soundtrack bypasses FmodVoice. Successful native audio events arm it.
    alignas(4) uint8_t movieObject[0x800]{};
    const auto movie=reinterpret_cast<uintptr_t>(movieObject),video=movie+0x38;
    uintptr_t channel=0x13579;
    write<uint8_t>(movie+0x6d0,1); write<uint8_t>(movie+0x6d2,1);
    write<int>(video+0xe4,1); write<uintptr_t>(video+0x294,0x24680);
    write<uintptr_t>(video+0x2a0,channel);
    for(int mode=0;mode<3;++mode) {
        resetMessages(); setSetting(Setting::VoicedDialogue,mode);
        Event create; create.rva=0x6273b0; create.ecx=video; enter(create);
        Event played; played.rva=0x548cf4; played.parentRva=0x6273b0;
        played.args[4]=reinterpret_cast<uintptr_t>(&channel); leave(played);
        create.result=1; leave(create);
        Event update; update.rva=0x2d77b0; update.ecx=movie; leave(update);
        write<uintptr_t>(w+0x94,caption(L"Movie NPC subtitle"));
        enter(npc); leave(npc); check(modes.back()==mode,"movie NPC caption receives voiced output policy");
        subtitle.args[0]=caption(L"Movie event subtitle"); leave(subtitle);
        check(modes.back()==mode,"movie event caption receives voiced output policy");
    }
    resetMessages(); start(0x30b6b0); setSetting(Setting::VoicedDialogue,0);
    enter(npc); write<uint8_t>(c+1,1); write<uint8_t>(c+5,0);
    write<uintptr_t>(w+0x94,caption(L"Selector prompt")); leave(npc);
    check(modes.back()==2 && said[said.size()-2]==L"Selector prompt","TK choice/menu remains spoken during voice");
    resetMessages(); scene=0xb; const auto before=said.size(); leave(subtitle);
    check(said.size()==before,"Help decoder remains outside dialogue suppression");
    puts("PASS: native dialogue voice start, failure, loading, pause, volume, mute, completion, audio isolation and teardown.");
}
