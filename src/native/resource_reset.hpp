#pragma once
#include "events.hpp"
namespace ffx {
// Bootstrap installs native-owner callbacks. Loader callbacks only invalidate
// the generation and publish bounded copied request values.
void configureOwnerReset(void(*reset)(),void(*arrival)(unsigned,unsigned));
bool ownerResetReady() noexcept;
bool serviceOwnerReset() noexcept;
void publishFieldArrival(unsigned room,unsigned epoch) noexcept;
bool nativeResourceLoading() noexcept;
}
