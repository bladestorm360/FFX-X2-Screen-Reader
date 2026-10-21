#pragma once
#include <array>
namespace ffx::lightning_rva {
inline constexpr unsigned Query=0x45ef20;
inline constexpr std::array<unsigned,7> dependencies{Query,0x464180,0x46c500,
    0x46c840,0xf26ae8,0xf00740,0x471d10};
}
