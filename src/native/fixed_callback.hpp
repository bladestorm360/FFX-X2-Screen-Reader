#pragma once
#include <array>
#include <cstddef>
#include <cstring>
#include <new>
#include <type_traits>
#include <utility>
namespace ffx {
// Capture only value snapshots and owner-qualified identities. This callback
// cannot allocate or own a destructor, engine object or borrowed text buffer.
template<class Signature,unsigned Capacity=256> class FixedCallback;
template<class R,class... A,unsigned Capacity> class FixedCallback<R(A...),Capacity> {
    alignas(std::max_align_t) std::array<unsigned char,Capacity> bytes_{};
    R(*invoke_)(void*,A...)=nullptr;
    void(*copy_)(void*,const void*) noexcept=nullptr;
public:
    FixedCallback()=default;
    FixedCallback(const FixedCallback& source) noexcept : invoke_(source.invoke_),copy_(source.copy_) {
        if(copy_) copy_(bytes_.data(),source.bytes_.data());
    }
    FixedCallback& operator=(const FixedCallback& source) noexcept {
        if(this!=&source) {invoke_=source.invoke_;copy_=source.copy_;if(copy_) copy_(bytes_.data(),source.bytes_.data());}
        return *this;
    }
    template<class F,std::enable_if_t<!std::is_same_v<std::decay_t<F>,FixedCallback>,int> =0>
    FixedCallback(F&& fn) {assign(std::forward<F>(fn));}
    template<class F,std::enable_if_t<!std::is_same_v<std::decay_t<F>,FixedCallback>,int> =0>
    FixedCallback& operator=(F&& fn) {assign(std::forward<F>(fn));return *this;}
    template<class F> void assign(F&& source) {
        using T=std::decay_t<F>;
        static_assert(std::is_nothrow_copy_constructible_v<T> && std::is_trivially_destructible_v<T> &&
            sizeof(T)<=Capacity && alignof(T)<=alignof(std::max_align_t),
            "fixed callbacks require bounded nonallocating value captures without owned destructors");
        static_assert(std::is_invocable_r_v<R,T&,A...>,"callback signature mismatch");
        new(bytes_.data()) T(std::forward<F>(source));
        copy_=[](void* destination,const void* original) noexcept {new(destination) T(*static_cast<const T*>(original));};
        invoke_=[](void* bytes,A... args)->R {
            auto& value=*static_cast<T*>(bytes);
            if constexpr(std::is_void_v<R>) value(std::forward<A>(args)...);
            else return value(std::forward<A>(args)...);
        };
    }
    explicit operator bool() const noexcept {return invoke_!=nullptr;}
    R operator()(A... args) {
        if constexpr(std::is_void_v<R>) invoke_(bytes_.data(),std::forward<A>(args)...);
        else return invoke_(bytes_.data(),std::forward<A>(args)...);
    }
};
}
