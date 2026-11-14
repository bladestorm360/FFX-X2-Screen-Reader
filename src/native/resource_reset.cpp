// Reset on the native owner after loading. Retirement failure or epoch drift
// withholds readiness; the bounded 32-arrival queue discards stale generations.
// See ../../Docs/features/runtime/index.md#initialization-and-ownership.
#include "resource_reset.hpp"
#include "owned_retirement.hpp"
#include "nonblocking_snapshots.hpp"
#include <atomic>
#include <windows.h>
namespace ffx {
namespace {
void(*resetOwner)()=nullptr;
void(*arrivedOwner)(unsigned,unsigned)=nullptr;
std::atomic<unsigned> completed{0};
struct Arrival {unsigned room,epoch;};
async::TryCaptureQueue<Arrival,32> arrivals;
std::atomic<unsigned> rejectedArrivals{0};
std::atomic<bool> configured{false};
// Native owner only. After a failed reset, wait 16, 32, ... up to 1024 ms
// before the next attempt, so a reset that keeps failing (for example under
// retirement pressure) does not rerun on every hooked call. The wait is in
// time because many hooked calls run each frame. Readiness stays withheld
// meanwhile, exactly as before. A new generation starts over.
unsigned failedEpoch=0,failedAttempts=0;
uint64_t retryAt=0;
bool resetFailed(unsigned epoch) noexcept {
    if(!failedAttempts++) recordLog("native owner reset failed generation=%u; retrying with backoff",epoch);
    failedEpoch=epoch;retryAt=GetTickCount64()+(8ull<<(failedAttempts<7?failedAttempts:7));
    return false;
}
}
void configureOwnerReset(void(*reset)(),void(*arrival)(unsigned,unsigned)) {
    resetOwner=reset;arrivedOwner=arrival;completed=generation();configured.store(true,std::memory_order_release);
}
bool ownerResetReady() noexcept {
    return !configured.load(std::memory_order_acquire) || completed.load(std::memory_order_acquire)==generation();
}
bool serviceOwnerReset() noexcept {
    if(!configured.load(std::memory_order_acquire)) return true;
    if(!nativeGameThread() || nativeResourceLoading()) return false;
    const auto epoch=generation();
    if(completed.load(std::memory_order_acquire)!=epoch) {
        if(failedAttempts && failedEpoch!=epoch) failedAttempts=0;
        if(failedAttempts && GetTickCount64()<retryAt) return false;
        const auto before=retirement::failures();
        try {resetOwner();} catch(...) {return resetFailed(epoch);}
        if(before!=retirement::failures()) return resetFailed(epoch);
        if(epoch!=generation() || nativeResourceLoading()) return false;
        completed.store(epoch,std::memory_order_release);
        if(failedAttempts) {
            recordLog("native owner reset recovered generation=%u after %u failures",epoch,failedAttempts);
            failedAttempts=0;
        }
    }
    Arrival next;
    for(unsigned count=0;count<32 && arrivals.tryPop(next);++count)
        if(next.epoch==generation() && !nativeResourceLoading() && arrivedOwner)
            arrivedOwner(next.room,next.epoch);
    return ownerResetReady() && !nativeResourceLoading();
}
void publishFieldArrival(unsigned room,unsigned epoch) noexcept {
    // A dropped arrival means that room's arrival handling never runs. Log the
    // first one so the session log explains a silent room change.
    if(!arrivals.tryPush({room,epoch}) && !rejectedArrivals.fetch_add(1,std::memory_order_relaxed))
        recordLog("field arrival dropped room=%u generation=%u; arrival queue full",room,epoch);
}
}
