#pragma once
#include "runtime.hpp"
#include <algorithm>
#include <array>
#include <atomic>
#include <cstdio>
#include <cstring>
#include <type_traits>
#include <utility>
namespace ffx {
constexpr unsigned LogArgumentLimit=16,LogDataLimit=4096;
enum class LogArgumentKind : unsigned {Scalar,Pointer,Narrow,Wide};
struct LogArgument {std::uint64_t bits=0;unsigned offset=0;LogArgumentKind kind=LogArgumentKind::Scalar;bool owned=false;};
struct LogCapture {
    FILETIME timestamp{};
    std::array<LogArgument,LogArgumentLimit> arguments{};
    alignas(wchar_t) std::array<unsigned char,LogDataLimit> data{};
    std::array<char,512> format{};
    unsigned count=0,used=0,byteCost=0;
    void(*formatter)(const LogCapture&,char*,size_t)=nullptr;
};
using LogCaptureHandler=void(*)(const LogCapture&) noexcept;
inline std::atomic<LogCaptureHandler> logCaptureHandler{nullptr};
// Bounded ready-data copying only; no formatter runs in these helpers.
inline unsigned copyLogString(void* destination,const void* source,unsigned capacity,unsigned width,
    unsigned sourceLimit=0xffffffffu) noexcept {
    if(!source || !capacity || (width!=1 && width!=2)) return 0;
    __try {
        auto* out=static_cast<unsigned char*>(destination);auto* data=static_cast<const unsigned char*>(source);
        unsigned length=0;
        while(length+1<capacity && length<sourceLimit) {
            const unsigned offset=length*width;out[offset]=data[offset];if(width==2) out[offset+1]=data[offset+1];
            if(!out[offset] && (width==1 || !out[offset+1])) return length+1;
            ++length;
        }
        out[length*width]=0;if(width==2) out[length*width+1]=0;return length+1;
    } __except(EXCEPTION_EXECUTE_HANDLER) {return 0;}
}
void formatLogCapture(LogCapture&,char*,size_t);
template<class T,bool=std::is_enum_v<T>> struct LogEnum {using Type=T;};
template<class T> struct LogEnum<T,true> {using Type=std::underlying_type_t<T>;};
template<class T> using LogBase=typename LogEnum<T>::Type;
template<class T> using LogPromoted=std::conditional_t<std::is_floating_point_v<T>,double,
    std::conditional_t<std::is_integral_v<LogBase<T>> && sizeof(LogBase<T>)<sizeof(int),int,LogBase<T>>>;
template<class T> auto readLogArgument(const LogCapture& packet,unsigned index) {
    const auto& argument=packet.arguments[index];
    if constexpr(std::is_same_v<T,std::nullptr_t>) return static_cast<void*>(nullptr);
    else if constexpr(std::is_pointer_v<T>) {
        return argument.owned?reinterpret_cast<T>(const_cast<unsigned char*>(packet.data.data()+argument.offset)):
            reinterpret_cast<T>(static_cast<uintptr_t>(argument.bits));
    } else {
        LogPromoted<T> value{};std::memcpy(&value,&argument.bits,sizeof(value));return value;
    }
}
template<class... T,size_t... I> void formatLogTyped(const LogCapture& packet,char* output,size_t capacity,std::index_sequence<I...>) {
    _snprintf_s(output,capacity,_TRUNCATE,packet.format.data(),readLogArgument<T>(packet,static_cast<unsigned>(I))...);
}
template<class... T> void formatLogTyped(const LogCapture& packet,char* output,size_t capacity) {
    formatLogTyped<T...>(packet,output,capacity,std::index_sequence_for<T...>{});
}
template<class A> void copyLogArgument(LogCapture& packet,unsigned index,A&& source) noexcept {
    using T=std::decay_t<A>;auto& argument=packet.arguments[index];
    if constexpr(std::is_same_v<T,std::nullptr_t>) {argument.kind=LogArgumentKind::Pointer;}
    else if constexpr(std::is_pointer_v<T>) {
        argument.bits=reinterpret_cast<uintptr_t>(source);argument.kind=LogArgumentKind::Pointer;
        using Element=std::remove_cv_t<std::remove_pointer_t<T>>;
        if constexpr(std::is_same_v<Element,char> || std::is_same_v<Element,wchar_t> ||
            std::is_same_v<Element,signed char> || std::is_same_v<Element,unsigned char>) {
            constexpr unsigned width=sizeof(Element);argument.kind=width==1?LogArgumentKind::Narrow:LogArgumentKind::Wide;
            if(!source) return;
            const auto aligned=(packet.used+width-1)&~(width-1);
            if(aligned>LogDataLimit-width) return;
            const auto available=(LogDataLimit-aligned)/width;
            constexpr auto extent=std::extent_v<std::remove_reference_t<A>>;
            argument.offset=aligned;
            const auto copied=copyLogString(packet.data.data()+aligned,source,available,width,
                extent?static_cast<unsigned>(extent):0xffffffffu);
            if(copied) {packet.used=aligned+copied*width;argument.owned=true;}
        }
    } else {
        static_assert(std::is_arithmetic_v<T> || std::is_enum_v<T>,"log arguments must be copied scalar or ready string data");
        const LogPromoted<T> value=static_cast<LogPromoted<T>>(source);
        static_assert(sizeof(value)<=sizeof(argument.bits));std::memcpy(&argument.bits,&value,sizeof(value));
    }
}
template<class... A> void recordLog(const char* format,A&&... source) {
    static_assert(sizeof...(A)<=LogArgumentLimit);
    const auto handler=logCaptureHandler.load(std::memory_order_acquire);
    if(!handler) return; // Unavailable logger never formats on a native caller.
    LogCapture packet;packet.count=sizeof...(A);packet.formatter=&formatLogTyped<std::decay_t<A>...>;
    GetSystemTimeAsFileTime(&packet.timestamp);
    const auto copied=copyLogString(packet.format.data(),format,static_cast<unsigned>(packet.format.size()),1);
    if(!copied) return;
    unsigned index=0;(copyLogArgument(packet,index++,std::forward<A>(source)),...);
    // Guard the amount of owned input work, independent of later formatting.
    packet.byteCost=std::min(LogDataLimit-1,copied-1+packet.used+packet.count*8)+16;
    handler(packet);
}
}
