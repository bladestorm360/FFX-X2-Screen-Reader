// Original functions for isolated offline tests only; never shipped.
const auto customizeLookupBodyStorage=::ffx_fixture::load<135>("native/customizeLookupBody.bin",0xba8dcb6c4877b44eull);
const auto& customizeLookupBody=customizeLookupBodyStorage.bytes;
const ReplayFixup customizeLookupFixups[]={
    {62,0xd30f2c,false},
    {84,0x1f10dde,false},
    {106,0x1f10f9e,false},
    {120,0x3abe10,true},
};
const auto customizeListBodyStorage=::ffx_fixture::load<176>("native/customizeListBody.bin",0x914d01cbb3b2d3b7ull);
const auto& customizeListBody=customizeListBodyStorage.bytes;
const ReplayFixup customizeListFixups[]={
    {18,0xd2c526,false},
    {23,0xd30f2f,false},
    {145,0xd3205f,false},
    {167,0xd2c526,false},
};
const auto customizeVisibleBodyStorage=::ffx_fixture::load<76>("native/customizeVisibleBody.bin",0x98e4fc3cfafb75f2ull);
const auto& customizeVisibleBody=customizeVisibleBodyStorage.bytes;
const ReplayFixup customizeVisibleFixups[]={
    {12,0x3abbf0,true},
    {65,0x4bf6f0,true},
};
