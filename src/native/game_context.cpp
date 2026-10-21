#include "events.hpp"
#include "nav_audio.hpp"
#include "minigames.hpp"
#include "navigation.hpp"
namespace ffx {
void resetStatusReview();
void helpSceneChanged();
namespace {
void opening(Event&) { pauseNavAudio(); }
void changed(Event&) {
    using Active=unsigned(__cdecl*)();
    const auto mask=reinterpret_cast<Active>(address(0x4aa410))()&0x1ffffff;
    int next=0;
    // Native overlays have higher window IDs than their parent. Observe the
    // completed mutation, including nested opens and main-menu ID remapping.
    for(int id=24;id>0;--id) if(mask&(1u<<id)) { next=id; break; }
    if(scene==next) return;
    resetSelectionDetail();
    if(next!=6) resetStatusReview();
    scene=next;
    helpSceneChanged();
    recordLog("scene=%d active-windows=%x owner=%lu gen=%u",scene.load(std::memory_order_acquire),mask,GetCurrentThreadId(),generation());
    if(scene) pauseNavAudio();
    else { updateNavAudio(playerPosition(),false); refreshMinigameAudio(); }
}
}
void installGameContext() {
    hook(0x4aa0b0,opening,changed);
    hook(0x4aad20,nullptr,changed);
    hook(0x4aaad0,nullptr,changed);
}
}
