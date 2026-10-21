#pragma once
#include "fixed_callback.hpp"
namespace ffx {
struct NativeAction;
// Accept the established zero-argument callbacks and explicit action-local
// state callbacks. All captures still obey FixedCallback's POD contract.
template<class R,class... A> struct ActionCallback : FixedCallback<R(NativeAction&,A...)> {
    using Base=FixedCallback<R(NativeAction&,A...)>;
    ActionCallback()=default;
    template<class F> ActionCallback(F source) {assign(source);}
    template<class F> ActionCallback& operator=(F source) {assign(source);return *this;}
    template<class F> void assign(F source) {
        if constexpr(std::is_invocable_r_v<R,F&,NativeAction&,A...>) Base::assign(source);
        else Base::assign([source](NativeAction&,A... args) mutable -> R {return source(std::forward<A>(args)...);});
    }
};
struct NativeAction {
    unsigned epoch=0;
    ActionCallback<bool> prepare;
    ActionCallback<void> invoke;
    ActionCallback<void,bool> complete;
    // Ownership cleanup only: runs on every exit, including stale-generation
    // discard. It must never call the engine or publish a gameplay action.
    ActionCallback<void> retire;
    std::array<unsigned char,64> state{};
    template<class T> T data() const noexcept {
        static_assert(std::is_trivially_copyable_v<T> && sizeof(T)<=64);
        T value{};std::memcpy(&value,state.data(),sizeof(value));return value;
    }
    template<class T> void data(const T& value) noexcept {
        static_assert(std::is_trivially_copyable_v<T> && sizeof(T)<=64);
        std::memcpy(state.data(),&value,sizeof(value));
    }
};
static_assert(std::is_nothrow_copy_constructible_v<NativeAction> && std::is_trivially_destructible_v<NativeAction>);
struct NativeActionQueue {
    std::array<NativeAction,8> values{};
    unsigned first=0,count=0;
    bool empty() const noexcept {return !count;}
    unsigned size() const noexcept {return count;}
    bool push(const NativeAction& action) noexcept {
        if(count==values.size()) return false;
        values[(first+count++)%values.size()]=action;return true;
    }
    NativeAction take() noexcept {auto result=values[first];first=(first+1)%values.size();--count;return result;}
};
}
