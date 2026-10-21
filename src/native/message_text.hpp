#pragma once
#include "runtime.hpp"
#include <array>
namespace ffx {
struct MessagePage {
    std::wstring prompt;
    std::array<std::wstring,32> choices{};
    std::vector<std::wstring> lines;
    bool selector=false;
};
struct TextPlan;
// Native owner resolves only renderer substitutions/Primer glyphs into copied
// instructions. Worker owns decoding, spacing, lines and selector strings.
unsigned planMessage(TextPlan&,unsigned slot,uintptr_t source,bool rows=false);
}
