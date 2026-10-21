#pragma once
#include <array>
namespace ffx {
// Extracted scripts have up to 289 callbacks per worker. Never truncate a
// remapped interaction to a single 32-bit mask. Generator rejects overflow.
struct EntryMask {
    std::array<unsigned,10> words{};
    constexpr EntryMask(unsigned low=0xffffffffu) {
        if(low==0xffffffffu) for(auto& word:words) word=0xffffffffu;
        else words[0]=low;
    }
    constexpr EntryMask(std::array<unsigned,10> value):words(value) {}
    bool contains(unsigned entry) const { return entry/32<words.size() && (words[entry/32]&(1u<<(entry%32))); }
};
}
