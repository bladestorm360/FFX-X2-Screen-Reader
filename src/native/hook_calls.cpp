#include "hook_calls.hpp"
#include <atomic>
#include <new>
#include <type_traits>
#include <intrin.h>
#include <cstddef>
namespace ffx {
namespace {
constexpr unsigned Capacity=4096,Words=Capacity/32;
static_assert(std::is_nothrow_default_constructible_v<NativeCall>);
static_assert(std::is_trivially_destructible_v<NativeCall> && std::is_trivially_copyable_v<Event>,
    "Native return frames contain only bounded copied scalars; recycling must not retire owned containers");
struct Slot {
    alignas(NativeCall) unsigned char bytes[sizeof(NativeCall)];
};
Slot slots[Capacity];
std::atomic<unsigned> claimed[Words];
std::atomic<unsigned> hint{0};
}
NativeCall* claimNativeCall() noexcept {
    const auto begin=hint.fetch_add(1,std::memory_order_relaxed)%Words;
    for(unsigned n=0;n<Words;++n) {
        const auto word=(begin+n)%Words;
        auto value=claimed[word].load(std::memory_order_relaxed);
        const auto available=~value;if(!available) continue;
        unsigned bit=0;_BitScanForward(reinterpret_cast<unsigned long*>(&bit),available);
        if(!claimed[word].compare_exchange_strong(value,value|(1u<<bit),std::memory_order_acquire)) continue;
        auto& slot=slots[word*32+bit];
        auto* call=new(slot.bytes) NativeCall;
        return call;
    }
    return nullptr;
}
void retireNativeCall(NativeCall* call) noexcept {
    if(!call) return;
    const auto index=(reinterpret_cast<unsigned char*>(call)-reinterpret_cast<unsigned char*>(slots)-offsetof(Slot,bytes))/sizeof(Slot);
    // The returning/abandoning owner is the final user. Trivial destruction
    // and bitmap release make capacity immediately reusable, independent of
    // every worker, with no heap work or native wait.
    call->~NativeCall();claimed[index/32].fetch_and(~(1u<<(index%32)),std::memory_order_release);
}
}
