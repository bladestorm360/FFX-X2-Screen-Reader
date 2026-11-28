#pragma once
#include "navigation.hpp"
#include "controller_model.hpp"
namespace ffx {
void startAutoWalk();
void stopAutoWalk();
// Stops for a lost route. Unlike stopAutoWalk it is not a player command, so a
// pending battle resume survives; a walk it ends is never resumed.
void releaseAutoWalk();
// Backslash toggle: stops an active walk and says so. False when no walk is active.
bool toggleAutoWalkOff();
// True only while a battle-confirmed resume is pending, including field recovery.
bool autoWalkResumePending();
// Stops like stopAutoWalk but keeps a pending battle resume. For field resets only.
void suspendAutoWalk();
// Called once on the native thread when a walk stopped by a battle may resume.
// The handler re-requests a route to the copied destination; it returns false
// when no request could be queued.
using AutoWalkResume=bool(*)(const NativeRouteTarget&);
void setAutoWalkResume(AutoWalkResume);
bool autoWalkActive();
void updateAutoWalk();
void autoWalkKeyboard(uint8_t* keys);
void autoWalkController(pad::State& state);
void autoWalkPlayerInput(unsigned vk);
// Each input path publishes Insert, Numpad Insert or Caps Lock state before
// reporting key presses. autoWalkKeyboard uses its own local physical scan.
void autoWalkReaderModifier(bool held);
unsigned autoWalkInputRevision(); // Copied manual-input cancellation token for delayed routes.
void installAutoWalk();
using NavigationKeyboardAssist=void(*)(uint8_t*);
using NavigationControllerAssist=void(*)(pad::State&);
void setNavigationInputAssist(NavigationKeyboardAssist,NavigationControllerAssist);
}
