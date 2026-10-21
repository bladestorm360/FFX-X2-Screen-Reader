// Qualifies mtgz0700 input and hit-window events while native success scripts remain in control; manual cues mark the simultaneous native targeting window. See Docs/features/index.md and Docs/gagazet_cave_20261016.md.
#include "gagazet.hpp"
#include "events.hpp"
#include "utility_memory.hpp"
#include "native_worker_index.hpp"
#include "settings.hpp"
#include "mod_menu.hpp"
#include "utilities.hpp"
#include "nav_audio.hpp"
#include <cstring>
namespace ffx {
namespace {
enum class Stage { None, Throw, Flight, Rikku, Tidus };
struct State {unsigned epoch=0,allocation=0;uintptr_t worker=0,signal=0;Stage stage=Stage::None;int selection=-1;bool aligned=false;} state;
constexpr unsigned FlagLow=0x010000d8,Flags=3,ChoiceLow[]={0x56000008,0x5600000c};
constexpr unsigned ShotInput=0x24e2,RikkuInput=0x28c7,TidusInput=0x2a7a;
// Original PPP records18/17/16 and corresponding character destinations:
// left blue, right green, lower red (reported as orange by the tester).
constexpr const wchar_t* openings[]={L"Blue opening, left",L"Green opening, right",L"Red opening, below"};
bool script(UtilityScript& s) {
    if(!nativeGameThread() || inBattle() || !captureUtilityScript(s) || s.name!="mtgz0700" || s.length!=15853 || s.workers!=42) return false;
    AtelIdentity id;
    return readyAtelIdentity(s.chunk,id) && id.span.serial==s.allocationSerial && id.checksum==0xcb3127fb &&
        id.length==s.length && id.workers==42 && !std::strcmp(id.name,"mtgz0700");
}
bool owner(UtilityScript& s,uintptr_t w,unsigned entry,unsigned& pc) {
    if(!script(s) || read<uintptr_t>(w+4)!=s.chunk) return false;
    uintptr_t actual=0;if(!captureUtilityWorker(s,22,actual) || actual!=w) return false;
    const auto sig=read<uintptr_t>(w+0x90);const unsigned level=read<uint8_t>(w+0x32,255);
    if(!sig || level>8 || (static_cast<unsigned>(read<uint8_t>(sig+14))&15u)!=level || read<uint16_t>(sig+8,255)!=entry) return false;
    const auto cursor=read<uintptr_t>(w+0x12c+level*0x4c+0x18),code=s.chunk+s.codeOffset;
    if(cursor<code || cursor-code>=s.length) return false;
    pc=static_cast<unsigned>(cursor-code);s.worker=w;s.header=read<uintptr_t>(w);
    return true;
}
bool allowed() {return gameFocused() && !modMenuActive() && !utilityConsoleActive() && !scene;}
bool current(const UtilityScript& s) {
    return state.epoch==generation() && state.allocation==s.allocationSerial && state.worker==s.worker &&
        state.signal==read<uintptr_t>(s.worker+0x90);
}
int selection(const UtilityScript& s,Stage stage) {
    int v=-1;
    if(stage!=Stage::Rikku && stage!=Stage::Tidus) return -1;
    return captureUtilityValue(s,ChoiceLow[stage==Stage::Tidus],1,0,v) && v>=0 && v<3?v:-1;
}
UtilityText selected(Stage stage,int value) {
    if(stage==Stage::Throw || stage==Stage::Flight) return L"Wakka's target throw. The targeting tone marks when both gaps align. Listen through a couple of passes, then start the throw before the tone so the ball arrives at the tone. Do not press exactly on the tone.";
    if((stage!=Stage::Rikku && stage!=Stage::Tidus) || value<0 || value>2) return L"No opening selection is active. Enter the second trial first.";
    return UtilityText(stage==Stage::Rikku?L"Rikku: ":L"Tidus: ")+UtilityText(openings[value]);
}
void cue(Stage stage,int value,const UtilityScript& s) {
    const bool same=current(s) && state.stage==stage;
    const bool aligned=same && state.aligned;
    const bool changed=!same || state.selection!=value;
    state={generation(),s.allocationSerial,s.worker,read<uintptr_t>(s.worker+0x90),stage,value,aligned};
    if(changed && setting(Setting::GagazetCues) && !setting(Setting::GagazetAssist)) emitUtilityText(selected(stage,value),"gagazet-cue");
}
void input(Event& e) {
    if(e.epoch!=generation() || e.parentRva!=0x464180 || !allowed()) return;
    UtilityScript s;unsigned pc=0;
    const unsigned entry=read<uint16_t>(read<uintptr_t>(e.args[0]+0x90)+8,255);
    if((entry!=6 && entry!=7) || !owner(s,e.args[0],entry,pc)) return;
    const bool automatic=setting(Setting::GagazetAssist)!=0;
    if(entry==6 && pc==ShotInput && e.args[2]) {
        // The native Common004C executor receives its operand through its
        // original VM stack, not as a C argument. Exact PC fixes action5.
        cue(Stage::Throw,-1,s);
        if(!automatic) {
            int flags=0;
            const bool available=captureUtilityValue(s,FlagLow,Flags,2,flags);
            const bool aligned=available && (static_cast<unsigned>(flags)&0xc0u)==0xc0u;
            const bool crossed=available && aligned && !state.aligned;
            state.aligned=available && aligned;
            if(crossed && setting(Setting::GagazetCues)) navAudioTimingBeep(1100.f,generation(),Setting::GagazetCues);
        }
        if(automatic) {e.result=1;e.overrideResult=true;}
        if(e.result) {state.stage=Stage::Flight;navAudioCancelGagazetBeep();}
        return;
    }
    if(entry!=7) return;
    const auto stage=pc>=0x28be && pc<0x2a5a?Stage::Rikku:pc>=0x2a71 && pc<0x2c31?Stage::Tidus:Stage::None;
    if(stage==Stage::None) return;
    const int value=selection(s,stage);if(value<0)return;
    if(pc==(stage==Stage::Rikku?RikkuInput:TidusInput)) cue(stage,value,s);
    if(!automatic) return;
    int first=-1;if(!captureUtilityValue(s,ChoiceLow[0],1,0,first) || first<0 || first>2)return;
    // Native controls:5 Confirm,12 Up,13 Right,14 Down,15 Left. Rikku
    // chooses1, Tidus0; native2C31 derives remaining2 for Wakka. No local,
    // persistent completion, story, route or encounter flag is written here.
    if(stage==Stage::Tidus && first!=1) return; // Preserve a manual partial attempt.
    const unsigned confirm=stage==Stage::Rikku?RikkuInput:TidusInput;
    const unsigned directions[2][4]={{0x297f,0x2962,0x2928,0x2945},{0x2b56,0x2b39,0x2aff,0x2b1c}};
    const int desired=stage==Stage::Rikku?1:0;
    if(pc==confirm) {e.result=value==desired;e.overrideResult=true;return;}
    const auto& row=directions[stage==Stage::Tidus];
    for(unsigned i=0;i<4;++i) if(pc==row[i]) {e.result=value!=desired && i==(stage==Stage::Rikku?1u:3u);e.overrideResult=true;return;}
}
void hitRead(Event& e) {
    if(e.epoch!=generation() || !allowed() || e.parentRva!=0x464180 || e.args[1]!=2 || e.args[2]!=2) return;
    UtilityScript s;unsigned pc=0;
    if(!owner(s,e.args[0],6,pc) || pc<0x2550 || pc>=0x2569 || !current(s) || state.stage!=Stage::Flight) return;
    if(!setting(Setting::GagazetAssist)) return;
    // Only the two original hit-predicate operands receive auto-success.
    // Original throw/setup, native branch, reveal and cleanup remain intact;
    // the underlying global byte and manual collision outcome are unchanged.
    e.result|=0xc0;e.overrideResult=true;
}
void changed(Event& e) {
    if(e.epoch!=generation() || !allowed() || !setting(Setting::GagazetCues) || setting(Setting::GagazetAssist))return;
    UtilityScript s;if(!script(s) || state.epoch!=generation() || state.allocation!=s.allocationSerial)return;
    const auto w=e.args[0];const unsigned local=read<uint16_t>(w+0x2e,255);
    if(local==22 && (e.args[1]==13 || e.args[1]==14) && e.args[2]==0) {
        unsigned pc=0;const auto stage=e.args[1]==13?Stage::Rikku:Stage::Tidus;
        if(owner(s,w,7,pc) && current(s) && state.stage==stage)cue(stage,selection(s,stage),s);
    }
}
void finishing(Event& e) {
    if(state.epoch!=generation() || !state.worker || read<uint16_t>(state.worker+0x2e,255)!=22)return;
    using Getter=uintptr_t(__cdecl*)(int);
    if(reinterpret_cast<Getter>(address(0x46a7c0))(static_cast<int>(e.args[0]))==state.worker &&
        read<uintptr_t>(state.worker+0x90)==state.signal)e.before[0]=1;
}
void finished(Event& e) {if(e.before[0]) {state={};navAudioCancelGagazetBeep();}}
}
bool gagazetActive() {UtilityScript s;return script(s);}
UtilityText gagazetCommand(std::wstring_view action) {
    UtilityScript s;if(!script(s))return L"Gagazet's cave puzzle controls are available in the Submerged Passage.";
    if(action==L"status") {
        int flags=0;
        if(!captureUtilityValue(s,FlagLow,Flags,0,flags))return L"Gagazet puzzle state is not ready.";
        return UtilityText(L"Gagazet. First trial: ")+UtilityText(flags&1?L"complete. ":L"not complete. ")+
            UtilityText(L"Second trial: ")+UtilityText(flags&2?L"complete. ":L"not complete. ")+
            UtilityText(setting(Setting::GagazetAssist)?L"Automatic solving On.":L"Automatic solving Off.");
    }
    if(action==L"selected" || action==L"expected") {
        if(state.epoch!=generation() || state.allocation!=s.allocationSerial)return L"No puzzle input is active. Enter a trial first.";
        s.worker=state.worker;s.header=read<uintptr_t>(s.worker);
        if(!current(s))return L"No puzzle input is active.";
        return selected(state.stage,selection(s,state.stage));
    }
    return L"First trial: interact with Wakka's target to begin. Confirm throws and Back leaves. With automatic solving Off, the targeting tone marks when both gaps align. Listen through a couple of passes and start the throw before the tone so the ball arrives at the tone; do not press exactly on it. Second trial: Rikku chooses the green opening on the right, then Tidus chooses the blue opening on the left. Wakka takes the remaining lower red opening. Confirm each choice. After each trial, return to Mountain Cave through the exit in your current water section. The next path opens in Mountain Cave. Other water sections are reached from the cave, not directly through this pool. Turn on Gagazet puzzle auto-success before entering either puzzle for automatic solving. A partly chosen second trial may need Back and a fresh attempt.";
}
void resetGagazet() {state={};navAudioCancelGagazetBeep();}
void installGagazet() {
    hook(0x45e0c0,nullptr,input);
    hook(0x46c6c0,nullptr,hitRead);
    hook(0x4713a0,nullptr,changed);
    hook(0x469170,finishing,finished);
}
}
