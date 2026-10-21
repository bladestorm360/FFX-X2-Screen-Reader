#pragma once

#include "atel.hpp"
#include "navigation_targets.hpp"
#include <algorithm>
#include <cstring>
#include <stdexcept>

namespace ffx {
// Decode an already-owned, exact-length ATEL chunk. The input is never retained.
inline AtelScript decodeOwnedAtelScript(const uint8_t* ownedBytes, unsigned size) {
    constexpr unsigned Limit = 0x800000;
    constexpr unsigned CopyBudget = 0x200000;
    if (!ownedBytes || size < 0x38) throw std::runtime_error("Invalid owned ATEL span");
    auto require = [size](unsigned offset, unsigned length) {
        if (offset > size || length > size - offset) throw std::runtime_error("ATEL read outside owned span");
    };
    auto u32 = [&](unsigned offset) { require(offset, 4); uint32_t value; std::memcpy(&value, ownedBytes + offset, 4); return value; };
    auto u16 = [&](unsigned offset) { require(offset, 2); uint16_t value; std::memcpy(&value, ownedBytes + offset, 2); return value; };

    const unsigned length = u32(0), codeOffset = u32(0x30);
    const unsigned count = u16(0x34);
    if (!length || length > Limit || codeOffset > Limit || length > Limit - codeOffset ||
        !count || count > 1024) throw std::runtime_error("Invalid ATEL script bounds");
    require(codeOffset, length);

    AtelScript script;
    script.codeLength = length;
    script.checksum = 2166136261u;
    if (length <= 0x100000) script.code.assign(ownedBytes + codeOffset, ownedBytes + codeOffset + length);
    for (unsigned i = 0; i < length; ++i) script.checksum = (script.checksum ^ ownedBytes[codeOffset + i]) * 16777619u;

    const unsigned nameOffset = u32(0xc);
    if (nameOffset > Limit - 8) throw std::runtime_error("Invalid ATEL name");
    require(nameOffset, 8);
    char name[9]{};
    std::memcpy(name, ownedBytes + nameOffset, 8);
    script.name = name;
    if (script.code.empty() && !navigationCatalog(script)) throw std::runtime_error("Uncatalogued large ATEL script");
    script.instructionStarts = atelInstructionStarts(script.code);

    size_t copied = script.code.size() + script.instructionStarts.size();
    for (unsigned i = 0; i < count; ++i) {
        const unsigned indexOffset = 0x38 + i * 4;
        const unsigned offset = u32(indexOffset);
        if (!offset || offset > Limit - 0x34) throw std::runtime_error("Invalid ATEL worker offset");
        require(offset, 0x28);
        const unsigned entryCount = u16(offset + 8), jumpCount = u16(offset + 0xa), intCount = u16(offset + 4);
        const uint64_t bytesToCopy = 4ull * (entryCount + jumpCount + intCount);
        if (copied + bytesToCopy > CopyBudget) throw std::runtime_error("ATEL copied tables exceed bounded script budget");
        copied += static_cast<size_t>(bytesToCopy);
        auto table = [&](unsigned field, unsigned tableCount, bool signedValues) {
            const unsigned tableOffset = u32(offset + field);
            if (tableOffset > Limit || tableCount > 65535 || tableCount > (Limit - tableOffset) / 4)
                throw std::runtime_error("Invalid ATEL table bounds");
            require(tableOffset, tableCount * 4);
            std::vector<uint32_t> raw(tableCount);
            if (tableCount) std::memcpy(raw.data(), ownedBytes + tableOffset, tableCount * 4);
            if (signedValues) {
                std::vector<int> values; values.reserve(tableCount);
                for (auto value : raw) { int32_t item; std::memcpy(&item, &value, 4); values.push_back(item); }
                return std::pair<std::vector<unsigned>, std::vector<int>>{{}, std::move(values)};
            }
            return std::pair<std::vector<unsigned>, std::vector<int>>{std::vector<unsigned>(raw.begin(), raw.end()), {}};
        };
        AtelWorker worker; worker.header = offset;
        auto ints = table(0x18, intCount, true); worker.ints = std::move(ints.second);
        auto jumps = table(0x24, jumpCount, false); worker.jumps = std::move(jumps.first);
        auto entries = table(0x20, entryCount, false); worker.entries = std::move(entries.first);
        script.workers.push_back(std::move(worker));
    }
    return script;
}
}
