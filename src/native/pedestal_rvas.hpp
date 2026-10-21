#pragma once
#include <array>
namespace ffx::pedestal_rva {
inline constexpr unsigned Interpreter=0x464180,Remap=0x488bb0;
inline constexpr unsigned Held=0xf27080,Previous=0xf27084,Repeat=0xf27088,
    AnalogRepeat=0xf2708c,Pressed=0xf27090,Released=0xf27094;
inline constexpr std::array<unsigned,6> aggregates{Held,Previous,Repeat,AnalogRepeat,Pressed,Released};
inline constexpr std::array<unsigned,8> newDependencies{Interpreter,Remap,Held,Previous,Repeat,AnalogRepeat,Pressed,Released};
// Includes all seven hooks and the established shared getters/geometry inputs.
inline constexpr std::array<unsigned,28> dependencies{Interpreter,Remap,Held,Previous,Repeat,AnalogRepeat,Pressed,Released,
    0x471d10,0x471980,0x471a50,0x469040,0x469170,0x4b8980,0x46c840,0x46a7c0,
    0xf26ae8,0xf00740,0x1efb504,0x830e90,0x46c500,0x476360,0x4767e0,
    0x1fc44e4,0x1fc44e0,0x42b230,0x5af0,0x5b10};
}
