#pragma once
#include <cstddef>
#include <cstdint>

namespace ffx::pe {
struct LaaHeader {
    std::size_t characteristicsOffset{};
    std::uint16_t characteristics{};
    bool enabled{};
};

inline bool inspectLaa(const std::uint8_t* data, std::size_t length, LaaHeader& out) {
    if (!data || length < 64 || data[0] != 'M' || data[1] != 'Z') return false;
    const std::uint32_t peOffset = static_cast<std::uint32_t>(data[0x3c]) |
        (static_cast<std::uint32_t>(data[0x3d]) << 8) |
        (static_cast<std::uint32_t>(data[0x3e]) << 16) |
        (static_cast<std::uint32_t>(data[0x3f]) << 24);
    if (peOffset < 64 || static_cast<std::size_t>(peOffset) > length ||
        length - static_cast<std::size_t>(peOffset) < 24) return false;
    const std::size_t pe = static_cast<std::size_t>(peOffset);
    if (data[pe] != 'P' || data[pe + 1] != 'E' || data[pe + 2] != 0 || data[pe + 3] != 0) return false;
    const std::uint16_t machine = static_cast<std::uint16_t>(data[pe + 4] | (data[pe + 5] << 8));
    const std::uint16_t optionalSize = static_cast<std::uint16_t>(data[pe + 20] | (data[pe + 21] << 8));
    if (machine != 0x014c || optionalSize < 0x60) return false;
    const std::size_t opt = pe + 24;
    if (static_cast<std::size_t>(optionalSize) > length - opt) return false;
    if (data[opt] != 0x0b || data[opt + 1] != 0x01) return false;
    const std::size_t characteristicsOffset = pe + 22;
    const std::uint16_t characteristics = static_cast<std::uint16_t>(data[characteristicsOffset] |
        (data[characteristicsOffset + 1] << 8));
    out = {characteristicsOffset, characteristics, (characteristics & 0x0020) != 0};
    return true;
}

inline bool enableLaa(std::uint8_t* data, std::size_t length) {
    LaaHeader header{};
    if (!inspectLaa(data, length, header)) return false;
    if (header.enabled) return true;
    const std::uint16_t value = static_cast<std::uint16_t>(header.characteristics | 0x0020);
    data[header.characteristicsOffset] = static_cast<std::uint8_t>(value);
    data[header.characteristicsOffset + 1] = static_cast<std::uint8_t>(value >> 8);
    return true;
}
} // namespace ffx::pe
