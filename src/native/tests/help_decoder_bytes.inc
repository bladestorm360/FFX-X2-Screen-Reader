// Original1422-byte Help decoder; isolated offline fixture only.
const auto helpDecoderBodyStorage=::ffx_fixture::load<1422>("native/helpDecoderBody.bin",0x78219b0fafc5af61ull);
const auto& helpDecoderBody=helpDecoderBodyStorage.bytes;
const HelpFixup helpDecoderFixups[]={
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
const unsigned helpDecoderJumps[]={0x376bee,0x376c06,0x376c1e,0x376c36,0x376c4e,0x376c66,0x376eeb,0x376f15,0x376f3e,0x376f39,0x376f3e};
