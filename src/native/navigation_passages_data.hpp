#pragma once
#include "navigation_targets.hpp"
namespace ffx {
struct NavigationPassageRecord {
    const char* script;
    unsigned length, checksum, worker;
    NavigationTarget target;
};
inline const NavigationPassageRecord navigationPassageTargets[] = {
    // City platform boarding uses touch entry4 and requests native player/map
    // motion. The ordinary text/treasure catalog misses these non-actor stops.
    // Preserve actual shape/callback enablement while a platform is moving.
    {"sins0400",45865,0x01aaeb94,42,{42,6,-1,-1,L"First moving platform, stop 1",EntryMask(0x10),2}},
    {"sins0400",45865,0x01aaeb94,43,{43,6,-1,-1,L"First moving platform, stop 2",EntryMask(0x10),2}},
    {"sins0400",45865,0x01aaeb94,59,{59,6,-1,-1,L"Second moving platform, stop 1",EntryMask(0x10),2}},
    {"sins0400",45865,0x01aaeb94,60,{60,6,-1,-1,L"Second moving platform, stop 2",EntryMask(0x10),2}},
    // These trigger the native trials and return control to this water section.
    // A late-story reward warp inside W22 does not make W32 a physical exit.
    {"mtgz0700",15853,0xcb3127fb,32,{32,2,-1,-1,L"First Trial of Gagazet: Wakka's ball target",EntryMask(0x4),2}},
    {"mtgz0700",15853,0xcb3127fb,33,{33,2,-1,-1,L"Second Trial of Gagazet: coloured openings",EntryMask(0x4),2}},
    {"mtgz0600",24363,0x131b7d53,33,{33,2,-1,-1,L"Prepare swimming party",EntryMask(0x4),2}},
    {"hiku0000",42244,0xffb34f6f,26,{26,1,-2,-1,L"Exit 3",EntryMask(0x10),2}},
    {"hiku0000",42244,0xffb34f6f,27,{27,1,-2,-1,L"Exit 4",EntryMask(0x10),2}},
    {"hiku0000",42244,0xffb34f6f,29,{29,1,-2,-1,L"Elevator up. Confirm to ride",EntryMask(0x14),2}},
    {"hiku0000",42244,0xffb34f6f,30,{30,1,-2,-1,L"Elevator down. Confirm to ride",EntryMask(0x14),2}},
    {"hiku0200",21789,0x858fecc9,20,{20,1,-2,-1,L"Exit 3",EntryMask(0x10),2}},
    {"hiku0200",21789,0x858fecc9,22,{22,1,-2,-1,L"Exit 4",EntryMask(0x10),2}},
    {"hiku0200",21789,0x858fecc9,23,{23,1,-2,-1,L"Exit 5",EntryMask(0x10),2}},
    {"hiku0200",21789,0x858fecc9,24,{24,1,-2,-1,L"Exit 6",EntryMask(0x10),2}},
    {"hiku0500",80410,0x733360dc,23,{23,1,-2,-1,L"Exit 3",EntryMask(0x10),2}},
    {"hiku0500",80410,0x733360dc,25,{25,1,-2,-1,L"Exit 4",EntryMask(0x10),2}},
    {"hiku0500",80410,0x733360dc,26,{26,1,-2,-1,L"Exit 5",EntryMask(0x10),2}},
    {"hiku0500",80410,0x733360dc,27,{27,1,-2,-1,L"Exit 6",EntryMask(0x10),2}},
    {"hiku0500",80410,0x733360dc,28,{28,1,-2,-1,L"Exit 7",EntryMask(0x10),2}},
    {"hiku0500",80410,0x733360dc,29,{29,1,-2,-1,L"Exit 8",EntryMask(0x10),2}},
    {"hiku2000",13902,0x2372f284,14,{14,1,-2,-1,L"Exit 3",EntryMask(0x10),2}},
    {"hiku2000",13902,0x2372f284,15,{15,1,-2,-1,L"Exit 4",EntryMask(0x10),2}},
    {"hiku2000",13902,0x2372f284,17,{17,1,-2,-1,L"Elevator up. Confirm to ride",EntryMask(0x14),2}},
    {"hiku2000",13902,0x2372f284,18,{18,1,-2,-1,L"Elevator down. Confirm to ride",EntryMask(0x14),2}},
    {"bvyt0900",37649,0x97fd6083,30,{30,1,209,-1,L"Exit 1",EntryMask(0x40),2}},
    {"stbv0000",19861,0xada03b86,26,{26,1,208,-1,L"Exit 1",EntryMask(0x10),2}},
    // Yuna's dropped sphere is an interactable only after its native reveal.
    {"mtgz0800",25957,0x2bb420e0,9,{9,6,-1,-1,L"Yuna's sphere",EntryMask(0x4),2}},
    // The two staircases share a native reveal gate, but remain distinct crossings.
    {"dome0600",38708,0x617936c9,25,{25,2,-1,-1,L"Staircase 1",EntryMask(0x4),2}},
    {"dome0600",38708,0x617936c9,26,{26,2,-1,-1,L"Staircase 2",EntryMask(0x4),2}},
    // Baaj's six temple seals are player-entry regions, not text/NPC scripts.
    // Exact script identity prevents applying these worker numbers elsewhere.
    {"bjyt1000",7530,0xd13cfb8c,30,{30,6,-1,-1,L"Kilika seal",EntryMask(0x4),3}},
    {"bjyt1000",7530,0xd13cfb8c,31,{31,6,-1,-1,L"Djose seal",EntryMask(0x4),3}},
    {"bjyt1000",7530,0xd13cfb8c,32,{32,6,-1,-1,L"Bevelle seal",EntryMask(0x4),3}},
    {"bjyt1000",7530,0xd13cfb8c,33,{33,6,-1,-1,L"Besaid seal",EntryMask(0x4),3}},
    {"bjyt1000",7530,0xd13cfb8c,34,{34,6,-1,-1,L"Macalania seal",EntryMask(0x4),3}},
    {"bjyt1000",7530,0xd13cfb8c,35,{35,6,-1,-1,L"Zanarkand seal",EntryMask(0x4),3}},
};
}
