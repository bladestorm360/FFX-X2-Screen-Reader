#pragma once
#include "settings.hpp"
namespace ffx {
// Native-owner preparation contexts; shared F8/controller menu uses these
// saved switches before the activity starts. No new field hotkey is captured.
bool minigameAssistAvailable(Setting);
void installMinigameCompletion();
void resetMinigameCompletion();
// Opening a mod/native menu cancels queued work, even if it closes again.
void cancelMinigameCompletion();
}
