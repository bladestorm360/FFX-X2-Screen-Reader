#pragma once
#include <array>
#include <atomic>
#include <new>
#include <type_traits>
namespace ffx {
// Mod-owned process-lifetime C++ storage. Construct on the designated bootstrap
// or worker owner before publishing users. Native DLL attach/detach neither
// constructs allocating containers nor runs their destructors. Never put an
// engine-owned object or an engine-calling destructor in this storage.
template<class T> class ProcessStorage {
    alignas(T) std::array<unsigned char,sizeof(T)> bytes_{};
    std::atomic<T*> value_{nullptr};
public:
    constexpr ProcessStorage() noexcept=default;
    ProcessStorage(const ProcessStorage&)=delete;
    ProcessStorage& operator=(const ProcessStorage&)=delete;
    // Exactly one initializer, outside native game callbacks. Publication is
    // permanent; an allocation failure leaves the storage unpublished.
    void prepare() {
        if(value_.load(std::memory_order_acquire)) return;
        auto* value=new(bytes_.data()) T;
        value_.store(value,std::memory_order_release);
    }
    bool ready() const noexcept {return value_.load(std::memory_order_acquire)!=nullptr;}
    T& get() noexcept {return *value_.load(std::memory_order_acquire);}
};
static_assert(std::is_trivially_destructible_v<ProcessStorage<std::array<unsigned,4>>>);
}
