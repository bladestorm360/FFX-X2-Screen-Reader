#pragma once
#include "navigation.hpp"
namespace ffx {
struct ButterflyCue { float pan=0,proximity=0; bool active=false,hazard=false; unsigned epoch=0; };
void publishButterflyCue(const ButterflyCue&);
InteractionZone butterflyTouchZone(uintptr_t actor,uintptr_t player);
void installButterfly();
void resetButterfly();
void configureButterflyStarter(Entity&,const char* verifiedScript,unsigned local,uintptr_t worker);
void refreshButterflyStarter(Entity&);
bool butterflyActive();
bool butterflyNavigationLeaving(const AtelSpan&); // Native copied lifecycle fact only.
void butterflyStatus();
void butterflyDirections(bool hazards=false);
void refreshButterflyAudio();
std::vector<Entity> butterflyTargets();
void refreshButterflyTarget(Entity&);
}