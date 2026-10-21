// Original FFX textured/plain clipping. Fixture only.
#pragma once
inline constexpr float hudClipOutside=-100000.f;
struct HudFixup {unsigned offset,rva;};
inline const auto hudTextureClipBodyStorage=::ffx_fixture::load<683>("native/hudTextureClipBody.bin",0x7d81947784fb4869ull);
inline const auto& hudTextureClipBody=hudTextureClipBodyStorage.bytes;
inline constexpr HudFixup hudTextureClipFixups[]={{6,0x1471690},{21,0x1471694},{44,0x147166c},{50,0x1471668},{66,0x14716b8},{83,0x14716c0},{168,0x14716b8},{176,0x14716c0},{244,0x14716c0},{260,0x1471668},{266,0x147166c},{286,0x14716b8},{312,0x14716c0},{387,0x14716b8},{395,0x14716c0},{457,0x14716c0},{474,0x14716c4},{500,0x14716bc},{583,0x14716bc},{591,0x14716c4},{659,0x14716c4}};
inline const auto hudPlainClipBodyStorage=::ffx_fixture::load<310>("native/hudPlainClipBody.bin",0xf3927ef129df65e8ull);
inline const auto& hudPlainClipBody=hudPlainClipBodyStorage.bytes;
inline constexpr HudFixup hudPlainClipFixups[]={{5,0x1471690},{34,0x14716b8},{53,0x14716c0},{72,0x14716c4},{89,0x14716bc},{112,0x7368c0},{126,0x14716c0},{132,0x14716bc},{138,0x14716b8},{176,0x14716c0},{182,0x14716bc},{216,0x14716bc},{262,0x14716c4}};
