// Reset on the native owner after loading. Retirement failure or epoch drift
// withholds readiness; the bounded 32-arrival queue discards stale generations.
// See ../../Docs/features/runtime/index.md#initialization-and-ownership.
#include "resource_reset.hpp"
#include "owned_retirement.hpp"
#include "nonblocking_snapshots.hpp"
#include <atomic>
namespace ffx {
namespace {
void(*resetOwner)()=nullptr;
void(*arrivedOwner)(unsigned,unsigned)=nullptr;
std::atomic<unsigned> completed{0};
struct Arrival {unsigned room,epoch;};
async::TryCaptureQueue<Arrival,32> arrivals;
std::atomic<unsigned> rejectedArrivals{0};
std::atomic<bool> configured{false};
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
        const auto before=retirement::failures();
        try {resetOwner();} catch(...) {return false;}
        if(before!=retirement::failures() || epoch!=generation() || nativeResourceLoading()) return false;
        completed.store(epoch,std::memory_order_release);
    }
    Arrival next;
    for(unsigned count=0;count<32 && arrivals.tryPop(next);++count)
        if(next.epoch==generation() && !nativeResourceLoading() && arrivedOwner)
            arrivedOwner(next.room,next.epoch);
    return ownerResetReady() && !nativeResourceLoading();
}
void publishFieldArrival(unsigned room,unsigned epoch) noexcept {
    if(!arrivals.tryPush({room,epoch})) rejectedArrivals.fetch_add(1,std::memory_order_relaxed);
}
}
