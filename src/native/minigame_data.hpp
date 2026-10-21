// Generated from extracted ATEL by minigame_bindings.py. FFXDataParser layout attribution.
#pragma once
namespace ffx {
struct MinigameCatalog { const char* name; unsigned checksum,length,workers; };
inline constexpr MinigameCatalog minigameCatalog[]={
    {"bltz0000",0x2ba296ba,4647290,41},
    {"bltz0000",0x4c6c4230,4647290,41},
    {"bltz0000",0x601e7c1c,4646208,41},
    {"bltz0000",0xef631536,4647290,41},
    {"nagi0000",0x6722a493,225132,150},
    {"nagi0000",0x42b3fc26,225368,150},
};
struct MinigameBinding { unsigned index,low,high; };
inline constexpr MinigameBinding blitzBindings[]={
    {11,0x1452,1},
    {12,0x1453,1},
    {166,0xc000f76,1},
    {175,0xc000fb0,1},
    {192,0xc0010a5,1},
    {215,0xc0010d3,1},
};
inline constexpr MinigameBinding raceBindings[]={
    {67,0x6c00001c,1},
    {116,0xc0000bc,1},
    {117,0xc0000bd,1},
    {128,0xd0000c9,1},
    {130,0xd0000cb,4},
    {135,0xc0000d3,1},
    {137,0xc0000d5,1},
    {149,0xc0000e1,1},
};
struct RacePickupBinding { unsigned worker,element,mask; };
inline constexpr RacePickupBinding racePickups[]={
    {39,0,1},
    {40,0,2},
    {41,0,4},
    {42,0,8},
    {43,0,16},
    {44,1,4},
    {45,1,8},
    {46,1,16},
    {47,1,32},
    {48,1,64},
    {49,1,128},
    {50,2,1},
    {51,2,2},
    {52,2,4},
    {53,2,8},
    {54,2,16},
    {55,2,32},
    {56,2,64},
    {57,2,128},
    {58,3,1},
};
}
