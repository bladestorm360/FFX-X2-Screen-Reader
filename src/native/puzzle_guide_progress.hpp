#pragma once
#include "puzzle_guide_bindings.hpp"
#include <optional>

namespace ffx::nav {
// Indices are for the authored steps in puzzle_guide_data.hpp. A missing or
// ambiguous observation falls back to the guide's existing checkpoint logic.
template<class Read> std::optional<unsigned> besaidSphereStep(unsigned expected,Read read) {
    const auto held=read(besaidHeld);
    if(!held) return {};
    const auto pedestal=read(besaidPedestal);
    // A placed Besaid Sphere proves the preceding pickup even when both
    // interactions happened between observations.
    if(expected==7 && pedestal==1) return *held==3?10:9;
    if(expected>=8 && *held==1) {
        // The pedestal must hold the Besaid Sphere through the final push.
        // Revealing or collecting the treasure does not consume this sphere.
        if(pedestal==0) return 8;
    }
    if(*held!=0) return {};
    if(expected>=8) {
        if(pedestal==0 && read(besaidSphereSource)==1) return 7;
    }
    if(expected>=3 && expected<=5 && read(besaidHiddenOpened)==0) {
        // Worker 41 is the intended pickup after the door opens. Only ask for
        // that pickup again when its recess really holds the Glyph Sphere.
        if(expected>=4 && read(besaidDoor)==2) return 4;
        if(read(besaidGlyphSource)==2) return 2;
    }
    if(expected==10 && read(besaidDestructionSource)==3) return 9;
    return {};
}

// The entry sphere has identity 1. The native fire-clearing action after
// removing it from the door retires the opening sequence.
template<class Read> std::optional<unsigned> kilikaEntryStep(Read read) {
    if(read(kilikaDoorStage)!=0) return {};
    const auto held=read(kilikaHeld),source=read(kilikaSource),door=read(kilikaDoor);
    if(held==0 && source==1 && door==0) return 0;
    if(held==1 && source==0 && door==0) return 1;
    if(held==0 && source==0 && door==1) return 2;
    return {};
}

// Both Djose Spheres have value 1. The separate source recesses identify
// which pickup can advance the guide. The door script clears both slots.
template<class Read> std::optional<unsigned> djoseEntryStep(Read read) {
    if(read(djoseDoorOpen)!=0 || read(djoseDoorOpening)!=0 || read(djoseDoor2)!=0) return {};
    const auto held=read(djoseHeld),source1=read(djoseSource1),source2=read(djoseSource2),door1=read(djoseDoor1);
    if(door1==0 && source2==1) {
        if(held==0 && source1==1) return 0;
        if(held==1 && source1==0) return 1;
    }
    if(door1==1 && source1==0) {
        if(held==0 && source2==1) return 2;
        if(held==1 && source2==0) return 3;
    }
    return {};
}
}
