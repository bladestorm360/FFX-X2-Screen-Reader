#pragma once

#include <array>
#include <cstdint>

namespace ffx::nav {

struct CollisionSnapshot {
    std::array<std::uint32_t, 16> remaps{};
    std::uint32_t actorFlags = 0;
    bool actorIsControlled = false;
};

// Mirrors the original83E4E0 machine code AND its byte dispatch/jump tables at
// 83E574/83E55C. The old decompiler labels every case one too high: group1 is
// blocked, and the mutable gates are48..63. Alias values remain unmasked.
inline bool blockedForGroup(std::uint32_t raw, const CollisionSnapshot& snapshot) {
    std::uint32_t group = raw & 0x7fu;
    std::uint32_t visitedAliases = 0;
    unsigned aliasHops = 0;

    for (;;) {
        if (group < 1 || group > 63) {
            return false;
        }
        switch (group) {
        case 1: return true;
        case 2: return !snapshot.actorIsControlled;
        case 13: return ((snapshot.actorFlags >> 2) & 1u) != 0;
        case 14: return snapshot.actorIsControlled;
        default:
            if (group < 48 || group > 63) return false;
            break;
        }
        const unsigned index = static_cast<unsigned>(group - 48);
        const std::uint32_t bit = std::uint32_t{1} << index;
        if ((visitedAliases & bit) != 0 || aliasHops == 16) {
            // Native follows malformed alias cycles forever; bound this snapshot walk.
            return true;
        }
        visitedAliases |= bit;
        ++aliasHops;
        group = snapshot.remaps[index];
    }
}

} // namespace ffx::nav
