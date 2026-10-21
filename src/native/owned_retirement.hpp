#pragma once
#include "runtime.hpp"
#include <array>
#include <atomic>
#include <type_traits>
#include <utility>

namespace ffx::retirement {
// Pools are prepared on bootstrap before native hooks are enabled. Only owned
// C++ storage belongs here; destructors must never call a native engine API.
struct PoolBase {
    PoolBase* next=nullptr;
    void (*drain)(PoolBase*) noexcept=nullptr;
};
bool registerPool(PoolBase*) noexcept;
bool start(HMODULE) noexcept;
void wake() noexcept;
void rejected() noexcept;
unsigned failures() noexcept;
#ifdef FFX_RETIREMENT_TEST
void drainForTest() noexcept;
#endif

template<class T, unsigned N=64> class Pool final : public PoolBase {
    static_assert(std::is_nothrow_swappable_v<T>,
        "retirement requires a bounded, nonallocating noexcept swap");
    struct Slot { std::atomic<unsigned> state{0}; T value{}; };
    std::array<Slot,N> slots_;
    std::atomic<unsigned> hint_{0};
    static void reclaim(PoolBase* base) noexcept {
        auto& pool=*static_cast<Pool*>(base);
        for(auto& slot:pool.slots_) if(slot.state.load(std::memory_order_acquire)==2) {
            try {
                // Construction, allocation and destruction occur on the worker.
                { T empty; using std::swap; swap(empty,slot.value); }
                slot.state.store(0,std::memory_order_release);
            } catch(...) {} // Keep ownership; a later worker pass may retry.
        }
    }
public:
    Pool() { drain=&reclaim; }
    bool capture(T& source) noexcept {
        const auto begin=hint_.fetch_add(1,std::memory_order_relaxed)%N;
        for(unsigned i=0;i<N;++i) {
            auto& slot=slots_[(begin+i)%N]; unsigned expected=0;
            if(!slot.state.compare_exchange_strong(expected,1,std::memory_order_acquire)) continue;
            using std::swap; swap(source,slot.value);
            slot.state.store(2,std::memory_order_release); wake(); return true;
        }
        rejected(); wake(); return false;
    }
};
template<class T> inline std::atomic<Pool<T>*> prepared{nullptr};
template<class T> bool prepare() {
    if(prepared<T>.load(std::memory_order_acquire)) return true;
    // Single bootstrap writer, before publication of hooks or producer access.
    auto* pool=new Pool<T>;
    if(!registerPool(pool)) {delete pool;return false;}
    prepared<T>.store(pool,std::memory_order_release);return true;
}
template<class T,class=void> struct HasEmpty : std::false_type {};
template<class T> struct HasEmpty<T,std::void_t<decltype(std::declval<T&>().empty())>> : std::true_type {};
template<class T> bool retire(T& source) noexcept {
    if constexpr(HasEmpty<T>::value) if(source.empty()) return true;
    auto* pool=prepared<T>.load(std::memory_order_acquire);
    if(!pool) {rejected();return false;}
    return pool->capture(source);
}
}
