#include "../events.hpp"
#include "../movie_dialogue.hpp"
#include "../settings.hpp"
#include "test_copied_controls.hpp"
#include <map>
#include <cstdio>
#include <cstdlib>
#include <cstring>
namespace ffx {
struct Listener { Handler enter,leave; };
std::map<unsigned,Listener> listeners;
std::vector<std::wstring> said;
std::vector<int> modes;
bool nativeVoice=false,testForeground=true;
unsigned generation() { return 1; }
bool voicedDialogue() { return nativeVoice; }
bool voicedDialogueAt(unsigned) { return nativeVoice; }
bool hook(unsigned id,Handler enter,Handler leave) { listeners[id]={enter,leave}; return true; }
bool copy(uintptr_t p,void* out,size_t n) {
    __try { memcpy(out,reinterpret_cast<void*>(p),n); return true; }
    __except(1) { return false; }
}
std::vector<std::wstring> decode(uintptr_t p,size_t,TextMode) { return {reinterpret_cast<const wchar_t*>(p)}; }
std::wstring clean(std::wstring s) { return s; }
std::wstring join(const std::vector<std::wstring>& p,const wchar_t*) { return p.empty()?L"":p[0]; }
bool outputDialogue(const std::wstring& text,bool,const char*,bool voiced) {
    said.push_back(text); modes.push_back(voiced?setting(Setting::VoicedDialogue):2); return true;
}
void log(const char*,...) {}
}
#include "test_speech_domains.hpp"
using namespace ffx;
void check(bool ok,const char* message) { if(!ok) { fprintf(stderr,"FAIL: %s\n",message); exit(1); } }
void enter(Event& e) { if(listeners[e.rva].enter) listeners[e.rva].enter(e); drainSpeechDomainsForTest(); }
void leave(Event& e) { if(listeners[e.rva].leave) listeners[e.rva].leave(e); drainSpeechDomainsForTest(); }
int main() {
    installMovieDialogue();
    alignas(4) uint8_t object[0x800]{},row1[0x18]{},row2[0x18]{};
    const auto owner=reinterpret_cast<uintptr_t>(object),video=owner+0x38;
    const auto r1=reinterpret_cast<uintptr_t>(row1),r2=reinterpret_cast<uintptr_t>(row2);
    uintptr_t channel=0x123456;
    write<uint8_t>(owner+0x6d0,1); write<uint8_t>(owner+0x6d2,1);
    write<int>(video+0xe4,1); write<uintptr_t>(video+0x294,0x654321);
    write<uintptr_t>(video+0x2a0,channel); write<uintptr_t>(owner+0x72c,r1);
    write<int>(owner+0x748,2); write<int>(r1+8,0); write<int>(r2+8,1);
    auto audioStart=[&](bool success=true,unsigned parent=0x6273b0) {
        Event create; create.rva=0x6273b0; create.ecx=video; enter(create);
        Event played; played.rva=0x548cf4; played.parentRva=parent;
        played.args[4]=reinterpret_cast<uintptr_t>(&channel); played.result=success?0:1; leave(played);
        create.result=success?1:0; leave(create);
        Event update; update.rva=0x2d77b0; update.ecx=owner; leave(update);
    };
    auto text=[&](uintptr_t row,unsigned epoch=1,unsigned parent=0x2dad80) {
        write<uintptr_t>(owner+0x72c,row);
        Event timeline; timeline.rva=0x2dad80; timeline.ecx=owner; timeline.epoch=epoch; enter(timeline);
        Event draw; draw.rva=0x505d30; draw.parentRva=parent; draw.epoch=epoch;
        const auto captionBytes=encodeMovieCaption(L"Repeated caption");
        draw.args[0]=reinterpret_cast<uintptr_t>(captionBytes.data()); enter(draw);
        leave(timeline);
    };
    auto pause=[&](bool value,bool success=true,uintptr_t target=0) {
        Event e; e.rva=0x67c90a; e.result=success?0:1; e.args[0]=target?target:channel;
        e.args[1]=value; leave(e);
    };
    auto volume=[&](float value,bool success=true) {
        Event e; e.rva=0x628330; e.ecx=video; e.result=success?1:0;
        uint32_t bits; memcpy(&bits,&value,4); e.args[0]=bits; leave(e);
    };
    check(!movieDialogueVoiced(),"video flags alone cannot suppress text");
    audioStart(false); check(!movieDialogueVoiced(),"failed FMOD start remains readable");
    audioStart(true,0x1234); check(!movieDialogueVoiced(),"music/SFX start cannot arm movie dialogue");
    audioStart(); check(movieDialogueVoiced(),"successful native movie audio arms dialogue policy");
    Event staticStart; staticStart.rva=0x2d90f0; staticStart.ecx=0x1234; leave(staticStart);
    check(movieDialogueVoiced(),"static movie start cannot snapshot arbitrary ECX and erase the validated manager");
    write<int>(video+0xe4,2); write<uint8_t>(owner+0x6fc,1);
    Event requestedPause; requestedPause.rva=0x2d77b0; requestedPause.ecx=owner; leave(requestedPause);
    pause(true,false); check(movieDialogueVoiced(),"failed FMOD pause preserves audio despite changed native pause flags");
    pause(true,true,0x999999); check(movieDialogueVoiced(),"unrelated audio pause is ignored");
    pause(true); check(!movieDialogueVoiced(),"paused soundtrack is readable");
    write<int>(video+0xe4,1); write<uint8_t>(owner+0x6fc,0); leave(requestedPause);
    pause(false); check(movieDialogueVoiced(),"resumed soundtrack uses selected policy");
    volume(0,false); check(movieDialogueVoiced(),"failed volume change preserves state");
    volume(0); check(!movieDialogueVoiced(),"muted movie soundtrack remains readable");
    volume(1); check(movieDialogueVoiced(),"audible volume restored");
    for(int mode=0;mode<3;++mode) {
        resetMovieDialogue(); audioStart(); setSetting(Setting::VoicedDialogue,mode);
        text(r1); check(modes.back()==mode,"timed caption uses all three voiced output modes");
        const auto n=said.size(); text(r1); check(said.size()==n,"same native caption event is announced once");
        text(r2); check(said.size()==n+1,"equal text at different caption rows remains a distinct event");
        text(r2,2); check(said.size()==n+2,"new generation invalidates caption identity");
        Event gap; gap.rva=0x2dad80; gap.ecx=owner; enter(gap); leave(gap);
        text(r2,2); check(said.size()==n+3,"gap permits re-entry of same caption row");
    }
    setSetting(Setting::VoicedDialogue,0); resetMovieDialogue();
    text(r1); check(modes.back()==2,"caption with no audible soundtrack is spoken");
    nativeVoice=true; text(r2); check(modes.back()==0,"separate native voice also suppresses caption"); nativeVoice=false;
    const auto n=said.size(); text(r1,1,0x999999); check(said.size()==n,"unrelated renderer use is ignored");
    write<int>(r1+8,10); text(r1); check(said.size()==n,"invalid timeline row is ignored"); write<int>(r1+8,0);
    audioStart(); write<uintptr_t>(video+0x294,0);
    Event update; update.rva=0x2d77b0; update.ecx=owner; leave(update);
    check(!movieDialogueVoiced(),"silent video track is not treated as voiced dialogue");
    write<uintptr_t>(video+0x294,0x654321); audioStart();
    Event stop; stop.rva=0x628350; stop.ecx=video; enter(stop);
    check(!movieDialogueVoiced(),"native video shutdown clears audio");
    audioStart(); write<uint8_t>(owner+0x6d0,0); leave(update);
    check(!movieDialogueVoiced(),"EOF clears movie dialogue state");
    write<uint8_t>(owner+0x6d0,1); audioStart(); resetMovieDialogue();
    check(!movieDialogueVoiced(),"teardown clears copied movie/audio state");
    puts("PASS: movie caption identities, three output modes, audio success/failure, pause, mute, silence, isolation and teardown.");
}
