#pragma once
#include <string>
namespace ffx {
inline std::wstring sphereType(int id) {
    switch (id) {
    case 0x00: return L"Lock Lv.3";
    case 0x01: return L"Empty";
    case 0x02: return L"Strength +1";
    case 0x03: return L"Strength +2";
    case 0x04: return L"Strength +3";
    case 0x05: return L"Strength +4";
    case 0x06: return L"Defense +1";
    case 0x07: return L"Defense +2";
    case 0x08: return L"Defense +3";
    case 0x09: return L"Defense +4";
    case 0x0A: return L"Magic +1";
    case 0x0B: return L"Magic +2";
    case 0x0C: return L"Magic +3";
    case 0x0D: return L"Magic +4";
    case 0x0E: return L"Magic Def +1";
    case 0x0F: return L"Magic Def +2";
    case 0x10: return L"Magic Def +3";
    case 0x11: return L"Magic Def +4";
    case 0x12: return L"Agility +1";
    case 0x13: return L"Agility +2";
    case 0x14: return L"Agility +3";
    case 0x15: return L"Agility +4";
    case 0x16: return L"Luck +1";
    case 0x17: return L"Luck +2";
    case 0x18: return L"Luck +3";
    case 0x19: return L"Luck +4";
    case 0x1A: return L"Evasion +1";
    case 0x1B: return L"Evasion +2";
    case 0x1C: return L"Evasion +3";
    case 0x1D: return L"Evasion +4";
    case 0x1E: return L"Accuracy +1";
    case 0x1F: return L"Accuracy +2";
    case 0x20: return L"Accuracy +3";
    case 0x21: return L"Accuracy +4";
    case 0x22: return L"HP +200";
    case 0x23: return L"HP +300";
    case 0x24: return L"MP +40";
    case 0x25: return L"MP +20";
    case 0x26: return L"MP +10";
    case 0x27: return L"Lock Lv.1";
    case 0x28: return L"Lock Lv.2";
    case 0x29: return L"Lock Lv.4";
    case 0xFF: return L"NULL";
    default: return {};
    }
}
}
