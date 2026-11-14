#pragma once
// Synthetic battle confirm handler call sites. Production (field_menus.cpp)
// resolves canUse and the battle buzzer from relative calls inside the
// profile-matched handler 49C870, so fixtures write the same byte patterns.
#include "../runtime.hpp"
#include <cstring>
#include <initializer_list>
#include <windows.h>

namespace ffx {
inline void battleCallSites(uintptr_t scratch,int(__cdecl* canUse)(int,int)) {
    const auto site=[](uintptr_t at,uintptr_t target,std::initializer_list<uint8_t> prefix) {
        auto code=reinterpret_cast<uint8_t*>(at);std::memcpy(code,prefix.begin(),prefix.size());code+=prefix.size();
        code[0]=0xe8;const auto rel=static_cast<int32_t>(target-reinterpret_cast<uintptr_t>(code+5));std::memcpy(code+1,&rel,4);
    };
    // canUse: PUSH EBP; MOV EBP,ESP; POP EBP; MOV EAX,fn; JMP EAX. Buzzer: prologue only (hooked, never run).
    const auto canUseCode=scratch,buzzerCode=scratch+0x100;
    const uint8_t prologue[]={0x55,0x8b,0xec,0x5d,0xb8};
    std::memcpy(reinterpret_cast<void*>(canUseCode),prologue,sizeof(prologue));
    const auto target=reinterpret_cast<uintptr_t>(canUse);std::memcpy(reinterpret_cast<void*>(canUseCode+5),&target,4);
    reinterpret_cast<uint8_t*>(canUseCode)[9]=0xff;reinterpret_cast<uint8_t*>(canUseCode)[10]=0xe0;
    std::memcpy(reinterpret_cast<void*>(buzzerCode),prologue,3);
    const auto handler=address(0x49c870);
    site(handler+0x379,canUseCode,{0x0f,0xbf,0x46,0x08,0x50});
    site(handler+0xeb,buzzerCode,{0x6a,0x03});
    FlushInstructionCache(GetCurrentProcess(),reinterpret_cast<void*>(canUseCode),11);
}
}
