#pragma once
#include "events.hpp"
namespace ffx {
bool battleSensorVisible(unsigned id);
void highlightBattleSensor(uintptr_t window);
void resetBattleSensor();
void installBattleSensor();
bool battleSensorAvailable();
bool battleSensorCapturesKey(unsigned vk,bool control);
bool dispatchBattleSensorKey(unsigned vk,bool control);
}
