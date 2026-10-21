// Attribute stack-buff callbacks to a qualified loaded x86 effect image.
// Normalize relocations before comparing retained code, then verify caller cue
// and shared native API identity. A similar callback shape alone proves nothing.
// See Docs/features/battle/index.md#details-and-effects.
#include "status_animation.hpp"
#include "status_animation_data.hpp"
#include "native_image.hpp"
#include <cstring>
namespace ffx {
namespace {
#ifdef FFX_READER_TEST
uintptr_t fixtureBase=0;
#endif
bool matches(uintptr_t base,const StatusAnimationLayout& layout) {
    IMAGE_DOS_HEADER dos{}; IMAGE_NT_HEADERS32 nt{};
    if(!copy(base,&dos,sizeof(dos)) || dos.e_magic!=IMAGE_DOS_SIGNATURE || dos.e_lfanew<0 ||
        dos.e_lfanew>4096 || !copy(base+dos.e_lfanew,&nt,sizeof(nt)) || nt.Signature!=IMAGE_NT_SIGNATURE ||
        nt.FileHeader.Machine!=IMAGE_FILE_MACHINE_I386 || nt.OptionalHeader.Magic!=IMAGE_NT_OPTIONAL_HDR32_MAGIC ||
        nt.OptionalHeader.SizeOfImage!=layout.size || nt.FileHeader.TimeDateStamp!=layout.stamp) return false;
    std::array<uint8_t,96> code{};
    if(!copy(base+layout.window,code.data(),layout.length)) return false;
    // The retained window stores base-relative operands; ASLR must not turn a
    // matching original effect into either a false refusal or a guessed match.
    for(unsigned i=0;i<layout.relocationCount;++i) {
        const auto pos=layout.relocations[i]; uint32_t operand=0;
        memcpy(&operand,code.data()+pos,sizeof(operand)); operand-=static_cast<uint32_t>(base);
        memcpy(code.data()+pos,&operand,sizeof(operand));
    }
    if(code!=layout.code) return false;
    const auto api=read<uintptr_t>(base+layout.api);
    return api && read<uintptr_t>(api+0x3ec)==address(0x395a30);
}
}
bool statusAnimationTarget(const Event& e,unsigned& owner,unsigned& buff) {
    if(!nativeGameThread() || !e.caller || e.args[0]>=31) return false;
    auto base=loadedImageBase(e.caller);
#ifdef FFX_READER_TEST
    if(fixtureBase && e.caller>=fixtureBase) base=fixtureBase;
#endif
    if(!base) return false;
    for(const auto& layout:statusAnimationLayouts) {
        if(e.caller-base!=layout.cue || !matches(base,layout)) continue;
        owner=read<unsigned>(base+layout.owner)&255; buff=layout.buff;
        return owner<31;
    }
    return false;
}
#ifdef FFX_READER_TEST
void setStatusAnimationFixture(uintptr_t base) { fixtureBase=base; }
#endif
}
