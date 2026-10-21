#pragma once
#include <array>
namespace ffx {
// Caption-only Will profile. Every body independently matched the retained
// native reference against SHA256 18e1b899...52d123. Import entries are derived
// from their matched caller's relative operand. No FFX gameplay RVA is reused.
inline constexpr std::array<RvaPair,8> movieAddresses{{
    {0x6273b0,0x61d1f0}, {0x628330,0x61e170},
    {0x627e90,0x61dcd0}, {0x628350,0x61e190},
    {0x2d90f0,0x2d4240}, {0x2d77b0,0x2d2910},
    {0x2dad80,0x2d5b00}, {0x505d30,0x4fbb00}
}};
inline unsigned movieProfileRva(unsigned reference) {
    for(const auto& entry:movieAddresses) if(entry.reference==reference) return entry.installed;
    return 0;
}
}
