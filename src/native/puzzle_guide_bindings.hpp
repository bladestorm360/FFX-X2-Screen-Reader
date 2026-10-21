#pragma once
// Original verified guide checkpoints/tile guards; capture through Native C500.
namespace ffx::nav {
struct GuideBinding {unsigned low,high,element;};
inline constexpr GuideBinding guide_bsyt0100[]={{0x5c0000ec,0x1,0},{0x5c0000d4,0x1,0},{0x5c0000ac,0x1,0},{0x5c0000d8,0x1,0},{0x5c0000dc,0x1,0},{0x5c0000e4,0x1,0},{0x5c0000cc,0x1,0},{0x5c0000e8,0x1,0},{0x01000a70,0x1,0}};
inline constexpr GuideBinding guide_klyt1000[]={{0x01000a70,0x1,0}};
inline constexpr GuideBinding guide_djyt0600[]={{0x01000a70,0x1,0}};
inline constexpr GuideBinding guide_mcyt0500[]={{0x01000a70,0x1,0}};
inline constexpr GuideBinding guide_bvyt1100[]={{0x01000a70,0x1,0}};
inline constexpr GuideBinding guide_dome0700[]={{0x20000a00,0x1,0},{0x01000a70,0x1,0},{0x5c000004,0x1,0},{0x0c000022,0x1,0},{0x0c000029,0x1,0},{0x0c000024,0x1,0},{0x0c000023,0x1,0},{0x0c000027,0x1,0},{0x0c000025,0x1,0}};
}
