#pragma once
#include "runtime.hpp"
#include "owned_text_decoder.hpp"
#include <array>
namespace ffx {
struct CopiedControls {
    bool controller=false,swap=false,polish=false;
    std::array<unsigned,81> bindings{};
};
void prepareCopiedControlNames(); // Bootstrap, before hook publication.
void prepareCopiedControlSnapshot(CopiedControls&); // Native owner: POD only.
OwnedTextContext copiedControlContext(const CopiedControls&); // Workers only.
// Copy a bounded ready encoded prefix. Payload bytes may be zero, so structural
// widths delimit the copy; no glyph decoding, allocation or text formatting.
inline unsigned copyReadyGameText(uintptr_t source,uint8_t* target,unsigned capacity) noexcept {
    unsigned length=0;
    while(length<capacity) {
        uint8_t byte=0;if(!copy(source+length,&byte,1)) break;
        target[length++]=byte;if(!byte) break;
        unsigned extra=0;
        if(byte==4) {
            uint8_t next=0;if(length==capacity || !copy(source+length,&next,1)) break;
            target[length++]=next;extra=next<0x30?1u:0u;
        } else if(byte==10 || byte==11 || (byte>=6 && byte<=14) || byte==16 ||
            (byte>=18 && byte<=42) || (byte>=0x2b && byte<0x30)) extra=1;
        while(extra-- && length<capacity) {
            uint8_t payload=0;if(!copy(source+length,&payload,1)) return length;
            target[length++]=payload;
        }
    }
    return length;
}
}
