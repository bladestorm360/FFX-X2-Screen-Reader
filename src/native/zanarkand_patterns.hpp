#pragma once
#include <array>
#include "zanarkand_glyph_data.hpp"
namespace ffx {
struct ZanarkandPatternSnapshot {unsigned count=0;bool main=false;std::array<unsigned,9> glyphs{};};
void installZanarkandPatterns();
bool captureZanarkandPattern(ZanarkandPatternSnapshot&);
inline const wchar_t* zanarkandGlyphLabel(unsigned worker) {
    for(const auto& row:zanarkandGlyphs) if(row.worker==worker) return row.label;
    return L"Glyph unavailable";
}
}
