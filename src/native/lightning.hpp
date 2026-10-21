#pragma once
#include <string>
namespace ffx {
void installLightning();
void resetLightning();
// A cancelled native strike stays cancelled until the next strike/generation.
void cancelLightningWarning();
#ifdef FFX_READER_TEST
std::wstring lightningDodgeControls(); // Semantic reference; production uses copied control recipes.
#endif
}
