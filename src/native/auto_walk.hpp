#pragma once
#include "navigation.hpp"
#include "controller_model.hpp"
namespace ffx {
void startAutoWalk();
void stopAutoWalk();
bool autoWalkActive();
void updateAutoWalk();
void autoWalkKeyboard(uint8_t* keys);
void autoWalkController(pad::State& state);
void autoWalkPlayerInput(unsigned vk);
unsigned autoWalkInputRevision(); // Copied manual-input cancellation token for delayed routes.
void installAutoWalk();
using NavigationKeyboardAssist=void(*)(uint8_t*);
using NavigationControllerAssist=void(*)(pad::State&);
void setNavigationInputAssist(NavigationKeyboardAssist,NavigationControllerAssist);
}
