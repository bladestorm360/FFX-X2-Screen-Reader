#pragma once
#include "events.hpp"
namespace ffx {
constexpr unsigned NativeListenerLimit=8,NativeCallDepth=128;
struct NativeCall {
    void* hook=nullptr;
    uintptr_t returnAddress=0,stack=0;
    std::array<Event,NativeListenerLimit> events;
    DWORD originalStarted=0;
    unsigned count=0;
    bool loading=false;
};
NativeCall* claimNativeCall() noexcept;
void retireNativeCall(NativeCall*) noexcept;
void abandonNativeCall(NativeCall*) noexcept;
struct NativeCallStack {
    std::array<NativeCall*,NativeCallDepth> values{};
    unsigned count=0;
    bool empty() const noexcept {return !count;}
    bool full() const noexcept {return count==values.size();}
    NativeCall& back() noexcept {return *values[count-1];}
    void push(NativeCall* call) noexcept {values[count++]=call;}
    NativeCall* take() noexcept {return values[--count];}
    void pop() noexcept {abandonNativeCall(take());}
    ~NativeCallStack() {while(count) pop();}
};
}
