#include "customize_replay_bytes.inc"
namespace {
void customizeNativeReplayFixture() {
    const auto lookup=replayCode(0x3abbf0,customizeLookupBody,customizeLookupFixups);
    const auto list=replayCode(0x25da000,customizeListBody,customizeListFixups);
    const auto visible=replayCode(0x25da200,customizeVisibleBody,customizeVisibleFixups);
    const auto inventory=nativeReplayAddress(0xd30f2c);
    std::memset(reinterpret_cast<void*>(inventory),0,200*22);
    write<uint8_t>(inventory+2*22+2,1);write<uint8_t>(inventory+2*22+4,0);
    write<uint8_t>(inventory+3*22+2,1);write<uint8_t>(inventory+3*22+4,0);
    write<uint8_t>(inventory+3*22+5,1);
    using List=uintptr_t(__cdecl*)(unsigned,int,int,int*);
    using Lookup=uintptr_t(__cdecl*)(unsigned,uintptr_t*);
    using Visible=int(__cdecl*)(unsigned,int);
    int count=0;const auto table=reinterpret_cast<List>(list)(~0u,-1,0,&count);
    check(count==200 && read<uint16_t>(table)==0x5000 && read<uint16_t>(table+4)==0x5002,
        "original equipment list retains empty inventory slots alongside owned gear");
    check(reinterpret_cast<Lookup>(lookup)(0x5000,nullptr)==inventory && reinterpret_cast<Lookup>(lookup)(0x5002,nullptr)==inventory+44,
        "original getter maps current table handles to exact live records");
    check(!reinterpret_cast<Visible>(visible)(0x5000,1) && reinterpret_cast<Visible>(visible)(0x5002,1),
        "original Customize predicate skips sold slot and accepts current armor/weapon");
    scene=9;stub(0x3abe10,liveCustomizeName);set<uintptr_t>(0x146a9f8,table);
    set<int>(win+0x28,1);set<int8_t>(win+0x45,0);set<uint16_t>(win+0x30,4);
    set<uintptr_t>(win+0x10,address(0x4d6830));set<int16_t>(win+0x48,0);
    event(0x4b4460,address(win),[]{set<int>(win+0x28,2);});expect(L"Empty","production reader with original getter withholds sold slot name");
    event(0x4b4460,address(win),[]{set<int16_t>(win+0x48,2);});expect(L"Current armor","production reader follows original table to owned armor");
    event(0x4b4460,address(win),[&]{write<uint8_t>(inventory+44+2,0);});expect(L"Empty","ownership change at unchanged row/handle retires sold armor");
    event(0x4b4460,address(win),[&]{write<uint16_t>(table+4,0x5003);});expect(L"Current weapon","rebuilt table row resolves new weapon after armor sale");
    event(0x4b4460,address(win),[&]{write<uint8_t>(inventory+66+3,2);});expect(L"Empty","production visibility agrees with original hidden-weapon predicate");
    check(!reinterpret_cast<Visible>(visible)(0x5003,1),"original native hidden predicate rejects flagged weapon");
    stub(0x3abbf0,gearRecord);stub(0x3abe10,slotName);scene=0;
    puts("PASS: original135-byte equipment getter,176-byte native list and76-byte Customize visibility predicate; sold weapon/armor tombstones, unchanged row/handle, rebuilt table and hidden flags. Native controller/text backend simulated.");
}
}
