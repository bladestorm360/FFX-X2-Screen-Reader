#pragma once
namespace ffx {
void installMinigames();
void resetMinigames();
bool minigameCapturesKey(unsigned vk);
bool dispatchMinigameKey(unsigned vk,bool shift);
void refreshMinigameAudio();
bool minigameActive();
}
