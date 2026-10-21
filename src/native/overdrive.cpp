#include "events.hpp"
#include "settings.hpp"
#include "overdrive.hpp"
#include "nav_audio.hpp"
#include "mod_menu.hpp"
#include "utilities.hpp"
#include "wakka.hpp"
#include "text_plan.hpp"

namespace ffx {
namespace {
bool inside=false;
unsigned cueEpoch=0;
void auronSequence(Event&) {
    // Initialize has selected the native array and length, but the timed
    // input phase has not started. Each cast has its own initialization event.
    if(setting(Setting::OverdriveAssist) || !setting(Setting::OverdriveCues) ||
        !inBattle() || !gameFocused() || scene || modMenuActive() || utilityConsoleActive() ||
        read<uint16_t>(address(auron_rva::State))!=1) return;
    const auto count=read<unsigned>(address(auron_rva::Count));
    const auto source=read<uintptr_t>(address(auron_rva::Sequence));
    uint16_t buttons[16]{};
    if(!source || !count || count>15 || !copy(source,buttons,(count+1)*sizeof(uint16_t)) || buttons[count]!=0xffff) return;
    TextPlan p;prepareTextPlan(p,"overdrive-auron-sequence");p.controls.swap=false;TextRef names=0,gate=0;
    for(unsigned i=0;i<count;++i) {
        const auto name=planNativeButtonMask(p,buttons[i]);names=planJoin(p,names,name);
        gate=gate?planPair(p,TextPlanOp::IfText,gate,name):name;
    }
    p.gate=gate;p.headline=planConcat(p,planLiteral(p,L"Sequence: "),names);commitTextPlan(p);
}
void force(Event& e) {
    const unsigned state = e.rva == 0x492290 ? 0xf3d6f4 : e.rva == 0x490ee0 ? 0xf3d6b4 : 0xf3d6c8;
    const auto phase=read<uint16_t>(address(state));
    const bool tidus=e.rva==0x492290,lulu=e.rva==0x491af0;
    if(!inBattle() || !gameFocused() || scene || modMenuActive() || utilityConsoleActive()) { resetOverdrive(); return; }
    if(tidus) {
        if(cueEpoch!=generation()) { inside=false; cueEpoch=generation(); }
        const auto base=address(state);
        // The native Confirm test uses these inclusive signed-word bounds
        // before advancing the cursor. Its adjacent coordinate is preserved.
        const auto position=read<int16_t>(base+14),left=read<int16_t>(base+8),right=read<int16_t>(base+10);
        const bool success=phase==4 && left<=right && position>=left && position<=right;
        if(!setting(Setting::OverdriveAssist) && setting(Setting::OverdriveCues) && success && !inside)
            navAudioOverdriveBeep(880.f,generation());
        if(phase!=4 || setting(Setting::OverdriveAssist) || !setting(Setting::OverdriveCues)) navAudioCancelOverdriveBeep();
        inside=success;
    }
    e.before[0]=lulu && phase==4;
    e.before[1]=lulu?read<uint8_t>(address(0xf3d6df)):0;
    if (!setting(Setting::OverdriveAssist) || phase!=4) return;
    const unsigned success = e.rva == 0x492290 ? 0xf3d6b3 : e.rva == 0x490ee0 ? 0xf3d6b2 : 0xf3d6df;
    write<uint8_t>(address(success), lulu ? 0x10 : 1); write<uint16_t>(address(state), lulu ? 6 : 5);
    recordLog("overdrive forced success rva=%x", e.rva);
}
void updated(Event& e) {
    if(e.rva!=0x491af0 || !e.before[0] || setting(Setting::OverdriveAssist) || !setting(Setting::OverdriveCues) ||
        !inBattle() || !gameFocused() || scene || modMenuActive() || utilityConsoleActive()) return;
    const auto count=read<uint8_t>(address(0xf3d6df));
    if(count>e.before[1] && count<=16) {TextPlan p;prepareTextPlan(p,"overdrive-lulu",false);p.headline=planConcat(p,planInteger(p,count),planLiteral(p,L" casts"));commitTextPlan(p);}
}
}
void resetOverdrive() { inside=false; cueEpoch=0; navAudioCancelOverdriveBeep(); }
void installOverdrive() {
    installWakka();
    hook(auron_rva::Initialize,nullptr,auronSequence);
    for (auto rva : {0x492290, 0x490ee0, 0x491af0}) hook(rva, force, updated);
}
}
