#include "events.hpp"
#include "atel.hpp"
#include "navigation_targets.hpp"
#include <stdexcept>
#include <algorithm>
#pragma optimize("gt",on)

namespace ffx {
namespace {
constexpr unsigned Limit = 0x800000;
template<class T> std::vector<T> table(uintptr_t chunk, unsigned offset, unsigned count) {
    if (offset > Limit || count > 65535 || count * sizeof(T) > Limit - offset) throw std::runtime_error("Invalid ATEL table bounds");
    std::vector<T> values(count);
    if (count && !copy(chunk + offset, values.data(), count * sizeof(T))) throw std::runtime_error("Unreadable ATEL table");
    return values;
}
}
AtelScript readScript(uintptr_t chunk) {
    AtelScript script;
    const auto length = read<unsigned>(chunk), codeOffset = read<unsigned>(chunk + 0x30);
    const auto count = read<uint16_t>(chunk + 0x34);
    if (!length || length > Limit || codeOffset > Limit || length > Limit - codeOffset || !count || count > 1024) throw std::runtime_error("Invalid ATEL script bounds");
    script.codeLength=length; script.checksum=2166136261u;
    // Large known minigames need catalog identity, not an owned multi-megabyte
    // bytecode copy. Smaller/unknown field scripts retain fallback analysis.
    if(length<=0x100000) script.code.resize(length);
    uint8_t bytes[4096];
    for (unsigned i = 0; i < length; i += 0x10000) {
        const unsigned end=std::min(i+0x10000u,length);
        for(unsigned at=i;at<end;at+=sizeof(bytes)) {
            const unsigned size=std::min(static_cast<unsigned>(sizeof(bytes)),end-at);
            if(!copy(chunk+codeOffset+at,bytes,size)) throw std::runtime_error("Unreadable ATEL bytecode");
            for(unsigned j=0;j<size;++j) script.checksum=(script.checksum^bytes[j])*16777619u;
            if(!script.code.empty()) std::copy(bytes,bytes+size,script.code.begin()+at);
        }
    }
    char name[9]{};
    const auto nameOffset = read<unsigned>(chunk + 0xc);
    if (nameOffset > Limit - 8 || !copy(chunk + nameOffset, name, 8)) throw std::runtime_error("Invalid ATEL name");
    script.name = name;
    if(script.code.empty() && !navigationCatalog(script)) throw std::runtime_error("Uncatalogued large ATEL script");
    script.instructionStarts=atelInstructionStarts(script.code);
    size_t copied=script.code.size()+script.instructionStarts.size();
    for (unsigned i = 0; i < count; ++i) {
        const auto offset = read<unsigned>(chunk + 0x38 + i * 4);
        if (!offset || offset > Limit - 0x34) throw std::runtime_error("Invalid ATEL worker offset");
        const auto header = chunk + offset;
        copied+=4u*(read<uint16_t>(header+8)+read<uint16_t>(header+0xa)+read<uint16_t>(header+4));
        if(copied>0x200000u) throw std::runtime_error("ATEL copied tables exceed bounded script budget");
        AtelWorker worker; worker.header = offset;
        worker.entries = table<unsigned>(chunk, read<unsigned>(header + 0x20), read<uint16_t>(header + 8));
        worker.jumps = table<unsigned>(chunk, read<unsigned>(header + 0x24), read<uint16_t>(header + 0xa));
        worker.ints = table<int>(chunk, read<unsigned>(header + 0x18), read<uint16_t>(header + 4));
        script.workers.push_back(std::move(worker));
    }
    return script;
}
}
