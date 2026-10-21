#pragma once
#include "navigation_scene.hpp"
namespace ffx::nav {
enum class Intent : unsigned {Face,Start,AssistStart,ArmAndStart,ArmAndFace,UpdatePedestal,AssistFailed};
void startNavigationIntents(); // Bootstrap; preallocated facts only after publication.
unsigned navigationIntentRevision();
void invalidateNavigationIntents(); // Atomic invalidation, any reset/input owner.
bool submitNavigationIntent(const Scene&,const Entity&,Intent,unsigned manual,unsigned revision);
void dispatchNavigationIntents(); // Native input owner; established deferred-action boundary.
}
