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

// Optional observations used to distinguish a returned sphere from a scripted
// removal. The original guide checkpoints stay usable if a read is unavailable.
inline constexpr unsigned besaidHeld=0x5c0000ac,besaidGlyphSource=0x5c0000b0,besaidDoor=0x5c0000b4,
    besaidDestructionSource=0x5c0000bc,besaidSphereSource=0x5c0000c0,besaidPedestal=0x5c0000cc,
    besaidHiddenOpened=0x5c0000dc;
inline constexpr unsigned besaidProgress[]={besaidGlyphSource,besaidDoor,besaidDestructionSource,besaidSphereSource};
inline constexpr unsigned kilikaSource=0x0c000058,kilikaDoor=0x0c000059,kilikaHeld=0x0c00005b,kilikaDoorStage=0x0c000063;
inline constexpr unsigned kilikaProgress[]={kilikaSource,kilikaDoor,kilikaHeld,kilikaDoorStage};
inline constexpr unsigned djoseHeld=0x0c000094,djoseSource1=0x0c000095,djoseSource2=0x0c000096,
    djoseDoor1=0x0c000097,djoseDoor2=0x0c000098,djoseDoorOpening=0x0c0000a7,djoseDoorOpen=0x0000028e;
inline constexpr unsigned djoseProgress[]={djoseHeld,djoseSource1,djoseSource2,djoseDoor1,djoseDoor2,djoseDoorOpening,djoseDoorOpen};
}
