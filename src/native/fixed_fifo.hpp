#pragma once
#include <array>
#include <type_traits>
namespace ffx {
// Owner-local primitive FIFO; no allocation, destructor work or synchronization.
template<class T,unsigned Capacity> struct FixedFifo {
    static_assert(std::is_trivially_copyable_v<T> && Capacity>0);
    std::array<T,Capacity> values{};
    unsigned first=0,count=0;
    bool empty() const noexcept {return !count;}
    unsigned size() const noexcept {return count;}
    void clear() noexcept {first=count=0;}
    T& front() noexcept {return values[first];}
    bool push_back(T value) noexcept {
        if(count==Capacity) return false;
        values[(first+count++)%Capacity]=value;return true;
    }
    void pop_front() noexcept {first=(first+1)%Capacity;--count;}
};
}
