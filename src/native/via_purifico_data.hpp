#pragma once
#include "utility_trials.hpp"
#include "navigation.hpp"
namespace ffx {
// Exact bvyt0900 (37649 / 97fd6083) original chest grant/collection entries.
struct ViaChest { ScriptAction action; unsigned mask; Point position; };
inline constexpr ViaChest viaChests[] = {
    {{11,2,19006,19006,0},1,{-376.320068359375,0,362.0260314941406}},
    {{12,2,19212,19212,0},2,{-164.9473419189453,0,667.2623291015625}},
    {{13,2,19409,19409,0},4,{-602.3702392578125,0,1144.1639404296875}},
    {{14,2,19606,19606,0},8,{-520.08740234375,0,1118.295654296875}},
    {{15,2,19803,19803,0},16,{-601.91357421875,0,1082.6793212890625}},
    {{16,2,20009,20009,0},32,{237.76600646972656,0,725.0362548828125}},
    {{17,2,20215,20215,0},64,{311.35186767578125,0,690.4712524414062}},
};
// Native node order after the relocation glyph: N=1,W=2,S=3,E=4.
// Only the consumer reads native activation/collection flags; no invented state.
struct ViaPanel { unsigned worker; Point position; unsigned toChamberDirection; };
inline constexpr ViaPanel viaPanels[] = {
    {36,{-360,0,1125},2},{38,{0,0,975},2},{40,{-360,0,675},1},
    {37,{0,0,225},1},{39,{360,0,675},2},{41,{-645,-1.9684710502624512,1125},0},
};
}
