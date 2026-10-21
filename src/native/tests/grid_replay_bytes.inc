// Original code for isolated offline fixtures only. Never linked into the mod.
struct ReplayFixup {unsigned offset,rva;bool relative;};
const auto gridDotBodyStorage=::ffx_fixture::load<37>("native/gridDotBody.bin",0xd5bb3a34f79a46d5ull);
const auto& gridDotBody=gridDotBodyStorage.bytes;
const auto gridSelectorBodyStorage=::ffx_fixture::load<474>("native/gridSelectorBody.bin",0xa69b70a8eb031bf8ull);
const auto& gridSelectorBody=gridSelectorBodyStorage.bytes;
const ReplayFixup gridSelectorFixups[]={
    {7,0x8613d8,false},
    {18,0x1f05834,false},
    {177,0x22fe90,true},
    {192,0x792184,false},
    {216,0x88f830,false},
    {224,0x88f834,false},
    {233,0x88f830,false},
    {241,0x88f838,false},
    {247,0x88f834,false},
    {255,0x88f83c,false},
    {261,0x88f834,false},
    {270,0x88f838,false},
    {276,0x88f838,false},
    {282,0x88f830,false},
    {288,0x88f834,false},
    {294,0x88f834,false},
    {300,0x88f834,false},
    {306,0x88f838,false},
    {312,0x88f838,false},
    {318,0x88f838,false},
    {331,0x5497b8,true},
    {348,0x88f788,false},
    {354,0x80a004,false},
    {360,0x88f788,false},
    {366,0x88f830,false},
    {371,0x88f830,false},
    {466,0x549240,true},
};
const auto gridCursorBodyStorage=::ffx_fixture::load<65>("native/gridCursorBody.bin",0xfd5a8b289b575079ull);
const auto& gridCursorBody=gridCursorBodyStorage.bytes;
const ReplayFixup gridCursorFixups[]={
    {14,0x648d70,true},
    {20,0x1f05834,false},
    {49,0x1f05834,false},
    {59,0x659e80,false},
};
const auto gridCursorInnerBodyStorage=::ffx_fixture::load<198>("native/gridCursorInnerBody.bin",0xa1b54179e2a9268aull);
const auto& gridCursorInnerBody=gridCursorInnerBodyStorage.bytes;
const ReplayFixup gridCursorInnerFixups[]={
    {8,0x1f05834,false},
    {66,0x1f05834,false},
    {77,0x1f05834,false},
    {91,0x1f05834,false},
    {109,0x1f05834,false},
    {154,0x1f05834,false},
    {160,0x72c5d0,false},
    {171,0x1f05834,false},
    {183,0x1f05834,false},
};
const auto helpTransitionBodyStorage=::ffx_fixture::load<1422>("native/helpTransitionBody.bin",0x78219b0fafc5af61ull);
const auto& helpTransitionBody=helpTransitionBodyStorage.bytes;
const ReplayFixup helpTransitionFixups[]={
    {7,0x8613d8,false},
    {84,0xd2a02c,false},
    {96,0x75312c,false},
    {101,0x75313c,false},
    {108,0x50ae50,true},
    {173,0xd2a074,false},
    {301,0xd2a0a4,false},
    {317,0x8416c0,false},
    {359,0xd2a0a4,false},
    {377,0x8416c0,false},
    {410,0x376fe0,false},
    {1002,0x374970,true},
    {1055,0xd2a0a4,false},
    {1074,0x8416c4,false},
    {1175,0x376ff8,false},
    {1180,0x8416c0,false},
    {1201,0x8416cc,false},
    {1226,0xd2a0a4,false},
    {1237,0x8416cc,false},
    {1265,0x8416cc,false},
    {1348,0xd2a03c,false},
    {1355,0xd2a084,false},
    {1374,0x549240,true},
    {1409,0x549240,true},
    {1418,0x549afc,true},
};
