#include "profile.hpp"
#include "profile_extensions_data.hpp"
#include "movie_profile.hpp"
#include <algorithm>
namespace ffx {
namespace {
bool wordGetterMatches(uintptr_t base,unsigned imageSize,const ExtensionSignature& spec,unsigned at,
    const std::map<unsigned,unsigned>& candidate) {
    if(!spec.wordOwner) return true;
    const auto call=nativeProfile==NativeProfile::Installed && spec.installedWordCall?spec.installedWordCall:spec.wordCall;
    const auto owner=candidate.find(spec.wordOwner);
    if(owner==candidate.end() || owner->second+spec.wordOffset+4>imageSize || at+call+5>imageSize) return false;
    const auto expected=read<uintptr_t>(base+owner->second+spec.wordOffset);
    if(expected<base || expected-base>=imageSize || read<uint8_t>(base+at+call)!=0xe8) return false;
    const int64_t getter=static_cast<int64_t>(at)+call+5+read<int>(base+at+call+1);
    if(getter<0x1000 || getter+7>imageSize) return false;
    const auto code=base+static_cast<uintptr_t>(getter);
    // The exact archived getter is MOV AX,[absolute pressed-word]; RET.
    // Several Common handlers share the masked query body. Qualify its call
    // by the independently derived aggregate operand, rather than its delta.
    return read<uint8_t>(code)==0x66 && read<uint8_t>(code+1)==0xa1 &&
        read<uint8_t>(code+6)==0xc3 && read<uintptr_t>(code+2)==expected;
}
}
bool resolveNativeExtensionsAt(uintptr_t base,unsigned imageSize) {
    std::map<unsigned,unsigned> candidate;
    // The executable hash was verified before this is called. Never infer a
    // blanket delta: each full signature must match exactly once in its window.
    for(const auto& spec:extensionSignatures) {
        const bool movieOnly=nativeProfile==NativeProfile::MovieCaptions;
        const unsigned movieRva=movieProfileRva(spec.rva);
        if(movieOnly && !movieRva && spec.rva!=0x548cf4 && spec.rva!=0x67c90a) continue;
        const auto tokens=nativeProfile==NativeProfile::Installed && spec.installedTokens ? spec.installedTokens : spec.tokens;
        const auto size=nativeProfile==NativeProfile::Installed && spec.installedSize?spec.installedSize:spec.size;
        unsigned start=std::max(0x1000u,spec.rva-4096),end=std::min(imageSize,spec.rva+4096+size);
        if(movieOnly && movieRva) { start=std::max(0x1000u,movieRva-4096); end=std::min(imageSize,movieRva+4096+size); }
        if(start>=end) return false;
        if(spec.owner) {
            const auto resolved=candidate.find(spec.owner);
            if(resolved==candidate.end()) { recordLog("native extension missing owner=%x rva=%x",spec.owner,spec.rva); return false; }
            const auto owner=resolved->second;
            const int relative=read<int>(base+owner+spec.offset);
            start=owner+spec.end+relative; end=start+size;
            if(start<0x1000 || end>imageSize) return false;
        }
        std::vector<uint8_t> bytes(end-start);
        if(!copy(base+start,bytes.data(),bytes.size())) return false;
        unsigned matches=0,found=0;
        for(unsigned pos=0;pos+size<=bytes.size();++pos) {
            bool same=true;
            for(unsigned i=0;i<size;++i) if(tokens[i]>=0 && bytes[pos+i]!=tokens[i]) { same=false; break; }
            if(same && wordGetterMatches(base,imageSize,spec,start+pos,candidate)) { ++matches; found=start+pos; }
        }
        if(matches!=1) { recordLog("native extension refused rva=%x matches=%u",spec.rva,matches); return false; }
        candidate[spec.rva]=found;
        recordLog("native extension matched rva=%x installed=%x bytes=%u",spec.rva,found,size);
    }
    for(const auto& operand:extensionOperands) {
        if(nativeProfile==NativeProfile::MovieCaptions && !candidate.count(operand.owner)) continue;
        const auto offset=nativeProfile==NativeProfile::Installed && operand.installedOffset?operand.installedOffset:operand.offset;
        const auto pointer=read<uintptr_t>(base+candidate.at(operand.owner)+offset);
        if(pointer<base || pointer-base>=imageSize) return false;
        const auto rva=static_cast<unsigned>(pointer-base);
        const auto found=candidate.find(operand.reference);
        if(found!=candidate.end() && found->second!=rva) return false;
        candidate[operand.reference]=rva;
    }
    if(nativeProfile!=NativeProfile::MovieCaptions) {
        const auto body=candidate.at(0x43e4e0),jumps=candidate.at(0x43e55c),selectors=candidate.at(0x43e574);
        if(jumps+24>imageSize || selectors+63>imageSize) return false;
        constexpr unsigned offsets[]={0x41,0x48,0x6a,0x59,0x2e,0x3d};
        for(unsigned i=0;i<6;++i) if(read<uintptr_t>(base+jumps+i*4)!=base+body+offsets[i]) return false;
        for(unsigned group=1;group<=63;++group) {
            const unsigned selected=group==1?0:group==2?1:group==13?2:group==14?3:group>=48?4:5;
            if(read<uint8_t>(base+selectors+group-1,255)!=selected) return false;
        }
    }
    extendedAddresses=std::move(candidate); return true;
}
bool resolveNativeExtensions() {
    if(nativeProfile==NativeProfile::Reference) return true;
    const auto base=reinterpret_cast<uintptr_t>(GetModuleHandleW(nullptr));
    IMAGE_DOS_HEADER dos{}; IMAGE_NT_HEADERS32 nt{};
    if(!copy(base,&dos,sizeof(dos)) || dos.e_magic!=IMAGE_DOS_SIGNATURE ||
        !copy(base+dos.e_lfanew,&nt,sizeof(nt)) || nt.Signature!=IMAGE_NT_SIGNATURE) return false;
    return resolveNativeExtensionsAt(base,nt.OptionalHeader.SizeOfImage);
}
}
