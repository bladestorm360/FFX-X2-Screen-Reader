// Original Gagazet W22 declarations and worker entries. Fixture only.
#pragma once
inline constexpr unsigned gagazetDeclarations[][2]={{0x20000a00,0x1},{0x1000aa3,0x1},{0x10000d8,0x3},{0x5c000014,0x1},{0x5c00001c,0x1},{0xc000020,0x1},{0xc000021,0x1},{0x27000000,0x8},{0x27000000,0x8},{0x27000000,0x8},{0x27000000,0x8},{0x66000000,0x1},{0x66000004,0x1},{0x56000008,0x1},{0x5600000c,0x1},{0x56000010,0x1},{0x66000000,0x1}};
inline constexpr unsigned gagazetEntries[]={8554,8617,9160,9160,9160,9160,9161,10261};
struct GagazetFixup {unsigned offset,va;bool relative;};
inline const auto gagazetIntegerBodyStorage=::ffx_fixture::load<120>("native/gagazetIntegerBody.bin",0x93fce79ca863f9b2ull);
inline const auto& gagazetIntegerBody=gagazetIntegerBodyStorage.bytes;
inline constexpr GagazetFixup gagazetIntegerFixups[]={{20,0x86c500,true},{51,0x86c738,false},{107,0x9497e0,true}};
inline constexpr unsigned gagazetIntegerJumps[]={55,65,75,85,95,95,104};
inline const auto gagazetOperandBodyStorage=::ffx_fixture::load<123>("native/gagazetOperandBody.bin",0x3d16a0a638f5296cull);
inline const auto& gagazetOperandBody=gagazetOperandBodyStorage.bytes;
inline constexpr GagazetFixup gagazetOperandFixups[]={{38,0x86c6c0,true},{72,0x86c5a0,true},{94,0x1327098,false}};
