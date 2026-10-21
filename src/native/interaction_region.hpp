#pragma once
#include "navigation.hpp"
#include <cmath>
#include <cstring>

namespace ffx {
// Type6 is an entry/leave region, not a talk rectangle. Original867BB0 uses
// geometry+28 to choose the moving reference; only -1 follows the player.
// Normalize the copied player-entry region into the existing rectangle route
// representation. Route strictly inside it, rather than stopping in its near
// envelope or offering a Confirm action. See the Baaj feature/native record.
inline InteractionZone playerEntryRegion(const std::array<uint8_t,0x4c>& bytes,unsigned channels) {
    InteractionZone zone;
    const auto scalar=[&](unsigned at) {float value;std::memcpy(&value,bytes.data()+at,sizeof(value));return value;};
    int16_t reference=0;std::memcpy(&reference,bytes.data()+0x28,sizeof(reference));
    if(!(channels&0x20) || reference!=-1) return zone;
    zone.center={scalar(0),scalar(4),scalar(8)};
    // Native867C09 negates this stored angle before its sine/cosine transform;
    // the generic type3 route representation consumes the transformed angle.
    zone.width=scalar(0x30);zone.angle=-scalar(0x34);zone.depth=scalar(0x38);zone.height=scalar(0x1c);
    zone.shape=3;zone.crossing=true;zone.range=0;zone.end=zone.center;
    // A quarter-unit inset is already applied to crossing rectangles by the
    // shared endpoint/anchor code. Reject collapsed regions rather than route
    // to an edge that the native strict comparisons can exclude.
    zone.valid=std::isfinite(zone.center.x) && std::isfinite(zone.center.y) && std::isfinite(zone.center.z) && std::isfinite(zone.width) && zone.width>.25 &&
        std::isfinite(zone.depth) && zone.depth>.25 && std::isfinite(zone.angle) && std::isfinite(zone.height);
    return zone;
}
}
