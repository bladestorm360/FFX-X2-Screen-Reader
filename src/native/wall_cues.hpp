#pragma once
#include "navigation.hpp"
namespace ffx {
void installWallCues();
void captureWallCues(); // Native owner: copied geometry/camera facts only.
void cancelWallCues() noexcept;
unsigned copiedWallCues() noexcept; // SDL owner: atomic POD, no engine access.
#ifdef FFX_READER_TEST
void drainWallCuesForTest();
unsigned wallCuePressureForTest();
bool wallCuesReadyForTest();
void setWallCueWakeForTest(HANDLE);
#endif
}
