#pragma once
#include <atomic>

namespace ffx {
// Process lifetime only; deliberately absent from saved settings.
inline std::atomic<bool> modEnabled{true};
inline std::atomic<unsigned> modToggleRequests{0};
inline std::atomic<unsigned> modResumeEpoch{0};
inline std::atomic<unsigned> modToggleSerial{0},modLifetimeEpoch{0};
// Published at bootstrap; only the verified native input owner
// calls this. The hotkey worker never touches game pointers or reader state.
inline std::atomic<void(*)(unsigned)> modToggleOnOwner{nullptr};
inline void serviceModToggle() {
    if(const auto handler=modToggleOnOwner.load()) {
        const auto requests=modToggleRequests.exchange(0);
        handler(requests);
    }
}
void toggleMod();
void startModToggle();
bool modToggleKey(unsigned vk,bool down,bool control,bool shift,bool focused);
void resetModInput();
void resetDirectInput();
void resetControllerInput();
}
