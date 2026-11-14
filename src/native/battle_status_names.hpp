#pragma once
#include <cstdint>
namespace ffx {
// Status names shared by battle status speech and battle target text plans.
// Bit i of actor+606 (permanent), byte actor+608+i (temporal), bit i of
// actor+616 (extra), byte actor+65E+i (buffs).
inline constexpr const wchar_t* battlePermanentStatus[]={L"KO",L"zombie",L"petrify",L"poison",L"power break",L"magic break",L"armor break",L"mental break",L"confuse",L"berserk",L"provoke",L"threaten"};
inline constexpr const wchar_t* battleTemporalStatus[]={L"sleep",L"silence",L"darkness",L"shell",L"protect",L"reflect",L"NulTide",L"NulBlaze",L"NulShock",L"NulFrost",L"regen",L"haste",L"slow"};
inline constexpr const wchar_t* battleExtraStatus[]={L"scan",L"distill power",L"distill mana",L"distill speed",L"distill move",L"distill ability",L"shield",L"boost",L"eject",L"auto-life",L"curse",L"defend",L"guard",L"sentinel",L"doom"};
inline constexpr const wchar_t* battleBuffStatus[]={L"Cheer",L"Aim",L"Focus",L"Reflex",L"Luck",L"Jinx"};
// Ailments named on battle targets: every permanent status; sleep, silence,
// darkness and slow; curse and doom. Positive effects are not included.
constexpr uint16_t battleAilmentPermanent=0x0fff,battleAilmentTemporal=0x1007,battleAilmentExtra=0x4400;
}
