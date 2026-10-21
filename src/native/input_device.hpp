#pragma once
#define WIN32_LEAN_AND_MEAN
#define NOMINMAX
#include <windows.h>
#include <atomic>
#include <cstdint>
namespace ffx {
// A single bounded CAS orders observed device events without a producer lock.
// Keyboard wins an equal-time tie. Neutral connection/held snapshots do not
// constitute new device events; queued hardware edges retain their own time.
inline std::atomic<uint64_t> inputDeviceMark{0};
static_assert(std::atomic<uint64_t>::is_always_lock_free,"native input mark must be lock-free");
inline bool controllerEventCurrent(uint64_t stamp) {
    const auto mark=inputDeviceMark.load();return !mark || (mark&1) || stamp>(mark>>1);
}
inline bool observeInputDeviceAt(bool controller,uint64_t stamp) {
    auto previous=inputDeviceMark.load();
    if(previous && (stamp<(previous>>1) || (controller && !(previous&1) && stamp==(previous>>1)))) return false;
    return inputDeviceMark.compare_exchange_strong(previous,(stamp<<1)|(controller?1u:0u));
}
inline void observeInputDevice(bool controller) {observeInputDeviceAt(controller,GetTickCount64());}
inline bool nativeInputUsesController() {return (inputDeviceMark.load()&1)!=0;}
}
