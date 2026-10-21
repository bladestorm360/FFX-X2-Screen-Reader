#include "copied_text.hpp"
#include "events.hpp"
#include "settings.hpp"
#include <atomic>
namespace ffx {
namespace {
// Original OpeningScreen.swf closes its logos/autosave notice with Finish.
// Copy the boot loop's active results on their owner; bind controls only on
// the established PInput owner, never on a loading/Flash thread.
std::atomic<unsigned> serial{0},opening{0};
constexpr unsigned Waiting=1,Active=2,Ready=3;
thread_local bool autosaveRequest=false;
void warningRequest(Event& e) {
    e.before[0]=autosaveRequest;autosaveRequest=false;
    constexpr char key[]="Loc_Texture_Saveload_Attention_Before_Game";
    const auto source=read<uintptr_t>(e.args[1]+8);
    if(!source)return;
    for(unsigned i=0;i<sizeof(key);++i) {
        char byte=0;if(!copy(source+i,&byte,1) || byte!=key[i])return;
    }
    autosaveRequest=true;
}
void warningReturned(Event& e) {autosaveRequest=e.before[0]!=0;}
void warningReady(Event& e) {
    // getLocByStr -> native localization -> newline conversion -> Flash UTF8.
    // Copy only this displayed warning, on its Flash owner, before stack reuse.
    if(!autosaveRequest || e.parentRva!=0x2e96d0 ||
       e.caller!=address(0x2e99b0)+0x3e || e.args[2]!=0x3ff || !e.args[1])return;
    CopiedTextPacket packet;packet.kind=CopiedTextKind::IntroWarning;packet.epoch=e.epoch;
    for(unsigned i=0;i<1024;++i) {
        uint8_t byte=0;if(!copy(e.args[1]+i,&byte,1))return;
        if(!byte) {
            if(packet.length[0] && commitCopiedText(packet))
                recordLog("opening autosave warning copied bytes=%u gen=%u",packet.length[0],e.epoch);
            return;
        }
        packet.text[0][packet.length[0]++]=byte;
    }
}
void opened(Event&) {opening.store((serial.fetch_add(1)+1)*4+Waiting,std::memory_order_release);}
void activeResult(Event& e) {
    // Bootstrap is asynchronous: the loader may have run before hooks exist.
    // Either original boot-loop read can establish that same active episode.
    const auto boot=address(0x241870);
    if(e.caller!=boot+0x139 && e.caller!=boot+0x19f)return;
    auto ticket=opening.load(std::memory_order_acquire);const auto phase=ticket&3;
    const bool active=static_cast<uint8_t>(e.result)!=0; // Original returns AL, not all of EAX.
    if(!ticket && active) {
        const auto current=(serial.fetch_add(1)+1)*4+Active;
        if(opening.compare_exchange_strong(ticket,current,std::memory_order_acq_rel))
            recordLog("opening cue observed active after bootstrap episode=%u",current/4);
        return;
    }
    if((phase==Waiting && active) || (phase==Active && !active))
        if(opening.compare_exchange_strong(ticket,(ticket&~3u)+(active?Active:Ready),std::memory_order_acq_rel) && !active)
            recordLog("opening cue native Finish observed episode=%u",ticket/4);
}
void inputReady(Event& e) {
    const auto ticket=opening.load(std::memory_order_acquire);
    if((ticket&3)!=Ready || !nativeGameThread() || e.epoch!=generation() ||
       !gameFocused() || scene || inBattle())return;
    // Saved current-room field, as used by native navigation. EFBBF8 is an
    // engine mode, not a room: mode23 occurs here immediately after Finish.
    // Room23 is startmap0; 15C/15D are its attract/intro route.
    // Do not carry a postponed startup instruction into ordinary gameplay.
    const auto room=read<uint16_t>(address(0xd2ca90),0xffff);
    if(room!=0 && room!=23 && room!=0x15c && room!=0x15d) {
        auto expected=ticket;
        if(opening.compare_exchange_strong(expected,ticket&~3u,std::memory_order_acq_rel))
            recordLog("opening cue retired outside startup room=%d episode=%u",room,ticket/4);
        return;
    }
    CopiedTextPacket packet;packet.kind=CopiedTextKind::IntroControls;
    prepareCopiedControls(packet);
    if(opening.load(std::memory_order_acquire)!=ticket || !commitCopiedText(packet))return;
    auto expected=ticket;
    if(opening.compare_exchange_strong(expected,ticket&~3u,std::memory_order_acq_rel))
        recordLog("opening cue copied controls room=%d gen=%u episode=%u",room,e.epoch,ticket/4);
}
}
void installIntroCue() {
    hookSnapshot(0x2e96d0,warningRequest,warningReturned);
    hookSnapshot(0x2e9840,nullptr,warningReady);
    hookSnapshot(0x257b60,opened,nullptr);
    hookSnapshot(0x260580,nullptr,activeResult);
    hook(0x225930,nullptr,inputReady);
}
}
