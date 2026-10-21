// Original83E4E0, including the actual two-stage switch tables.
#pragma once
struct CollisionReplayFixup {unsigned offset,va;bool relative;};
inline const auto collisionReplayBodyStorage=::ffx_fixture::load<211>("native/collisionReplayBody.bin",0x513793fdee6b7a5dull);
inline const auto& collisionReplayBody=collisionReplayBodyStorage.bytes;
inline constexpr CollisionReplayFixup collisionReplayFixups[]={{7,0x1301a44,false},{35,0x83e574,false},{42,0x83e55c,false},{49,0x1301998,false},{73,0x82d8e0,true},{90,0x82d8e0,true},{124,0x83e521,false},{128,0x83e528,false},{132,0x83e54a,false},{136,0x83e539,false},{140,0x83e50e,false},{144,0x83e51d,false}};
inline const auto choiceExecBodyStorage=::ffx_fixture::load<167>("native/choiceExecBody.bin",0xe38cec67e0013130ull);
inline const auto& choiceExecBody=choiceExecBodyStorage.bytes;
inline constexpr CollisionReplayFixup choiceExecFixups[]={{13,0x86be20,true},{51,0x8b7c50,true},{65,0x8b7f60,true},{79,0x8b7f80,true},{88,0x8b7fa0,true},{111,0x864090,true},{127,0x8b7f80,true},{136,0x8b7fa0,true}};
inline const auto choiceReturnBodyStorage=::ffx_fixture::load<94>("native/choiceReturnBody.bin",0x8e5256b37e434455ull);
inline const auto& choiceReturnBody=choiceReturnBodyStorage.bytes;
inline constexpr CollisionReplayFixup choiceReturnFixups[]={{20,0x86e380,true},{27,0x86de90,true},{33,0x864090,true},{71,0x86e380,true}};
