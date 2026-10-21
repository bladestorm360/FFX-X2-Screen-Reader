#pragma once
#include "runtime.hpp"
#include <array>
#include <algorithm>
namespace ffx {
template<size_t N> struct ReadyBytes {std::array<uint8_t,N> bytes{};unsigned length=0;};
template<size_t N> void copyReadyAscii(uintptr_t source,ReadyBytes<N>& text,unsigned limit) {
    text.length=0;limit=std::min<unsigned>(limit,static_cast<unsigned>(N));
    for(unsigned i=0;i<limit;++i) {uint8_t byte=0;if(!copy(source+i,&byte,1) || !byte) break;text.bytes[text.length++]=byte;}
}
// Worker only: source is owned bytes, never a native/game pointer.
std::wstring decodeReadyAscii(const uint8_t*,unsigned);
template<size_t N> std::wstring decodeReadyAscii(const ReadyBytes<N>& text) {
    return text.length<=N?decodeReadyAscii(text.bytes.data(),text.length):std::wstring{};
}
}
