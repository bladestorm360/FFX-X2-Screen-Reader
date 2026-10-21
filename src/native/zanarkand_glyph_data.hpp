// Generated from verified Zanarkand ATEL and FP glyph geometry.
#pragma once
#include <array>
namespace ffx {
struct ZanarkandGlyph {unsigned worker;const wchar_t* label;};
inline constexpr ZanarkandGlyph zanarkandGlyphs[]={
    {57,L"Floor panel 2: orange T, four squares"},
    {58,L"Floor panel 3: yellow straight line, four squares"},
    {59,L"Floor panel 4: orange T, four squares"},
    {60,L"Floor panel 5: white square, four squares"},
    {61,L"Floor panel 6: cyan zigzag, four squares"},
    {62,L"Floor panel 7: white square, four squares"},
    {63,L"Floor panel 8: yellow straight line, four squares"},
    {64,L"Floor panel 9: blue L, four squares"},
    {65,L"Floor panel 10: white square, four squares"},
    {66,L"Floor panel 11: blue L, four squares"},
    {67,L"Floor panel 12: white square, four squares"},
    {68,L"Floor panel 13: red corner, three squares"},
    {69,L"Floor panel 14: white square, four squares"},
    {70,L"Floor panel 15: red corner, three squares"},
    {71,L"Floor panel 16: cyan zigzag, four squares"},
    {72,L"Floor panel 17: red corner, three squares"},
    {73,L"Floor panel 18: red corner, three squares"},
    {74,L"Floor panel 19: red corner, three squares"},
    {75,L"Floor panel 20: white square, four squares"},
    {76,L"Floor panel 21: cyan zigzag, four squares"},
    {77,L"Floor panel 22: cyan mirrored zigzag, four squares"},
    {78,L"Floor panel 23: blue L, four squares"},
    {79,L"Floor panel 24: red corner, three squares"},
    {80,L"Floor panel 25: blue L, four squares"},
    {81,L"Floor panel 26: white square, four squares"},
    {82,L"Floor panel 27: blue L, four squares"},
    {83,L"Floor panel 28: yellow straight line, four squares"},
};
struct ZanarkandSequence {unsigned worker,stage,entryPc,endPc,count;std::array<unsigned,9> glyphs;};
inline constexpr ZanarkandSequence zanarkandSequences[]={
    {53,0,41959,42056,5,{57,58,59,61,64}},
    {54,1,42109,42686,6,{74,73,71,77,80,82}},
    {54,2,42109,42686,7,{74,73,79,66,76,78,83}},
    {54,3,42109,42686,9,{74,73,70,68,72,79,66,80,77}},
    {54,4,42109,42686,7,{74,73,70,68,66,76,82}},
};
}
