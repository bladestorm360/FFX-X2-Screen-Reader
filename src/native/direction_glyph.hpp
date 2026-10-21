#pragma once
#include <cstdint>
namespace ffx {
// Adjacent pairs encode a diagonal chord. Opposing directions, triples and
// the complete direction pad remain alternatives in native control prompts.
constexpr const wchar_t* directionGlyphSeparator(std::uint8_t glyph) noexcept {
    return glyph==0x43 || glyph==0x46 || glyph==0x49 || glyph==0x4c ? L" plus " : L" or ";
}
}
