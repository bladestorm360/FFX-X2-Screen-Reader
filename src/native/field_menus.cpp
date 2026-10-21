#include "events.hpp"
#include "profile.hpp"
#include "settings.hpp"
#include "sphere_grid.hpp"
#include "status_review.hpp"
#include "esc_details.hpp"
#include "ready_game_text.hpp"
#include "naming.hpp"
#include <algorithm>

namespace ffx {
void announceIncreaseRequirements(int row);
namespace {
using Key = std::array<intptr_t, 32>;
Key windowKey(uintptr_t p) {
    Key k{};
    k[0] = read<int>(p + 0x28, -1); k[1] = read<int16_t>(p + 0x48, -1);
    k[2] = read<uint16_t>(p + 0x30); k[3] = read<int8_t>(p + 0x45, -1);
    k[4] = logicalRva(read<uintptr_t>(p + 0x10));
    return k;
}
Key itemKey(uintptr_t p) {
    auto k = windowKey(p);
    if (k[0] != 2 || k[1] < 0 || k[1] >= k[2] || k[3] < 0) return {};
    k[3] = std::min(k[3], static_cast<intptr_t>(read<uint8_t>(p + 0x42)) - 1);
    if (k[3] < 0 || k[3] > 1) return {};
    auto col = read<int>(p + 0x58 + k[3] * 4, -1);
    if (col < 0 || col > 1) return {};
    k[5] = k[1] * 2 + col; k[6] = read<uint8_t>(p + 0x43);
    if (k[5] < 0 || k[5] >= 512 || k[5] >= read<int>(p + 0x94)) return {};
    if (k[6] == 1) {
        k[7] = read<uint16_t>(address(0x1196f30) + k[5] * 2); k[8] = -1;
        // Native 8C7C00 draws only a nonzero, currently owned key item.
        // Passing a blank slot's zero through the record getter names item0.
        using Owned=int(__cdecl*)(unsigned);
        k[10]=!k[7] || !reinterpret_cast<Owned>(address(0x390850))(static_cast<unsigned>(k[7]));
    }
    else if (k[6] == 0 || k[6] == 2) {
        auto entry = address(0x1197730) + k[5] * 4;
        auto count = read<uint8_t>(entry + 2);
        if (!count) return {};
        k[7] = read<uint16_t>(entry); k[8] = k[6] == 0 ? count : -1;
    } else return {};
    k[9] = 1; return k;
}
void itemBefore(Event& e) { e.before = itemKey(e.args[0]); }
void itemAfter(Event& e) {
    if (scene != 2 && scene != 3) return;
    const auto k = itemKey(e.args[0]);
    if (!k[9] || k == e.before) return;
    const auto getter = k[6] == 1 ? 0x3908f0 : k[6] == 2 ? 0x39a4c0 : 0x390a40;
    TextPlan p;prepareTextPlan(p,"items");
    if(k[10]) {
        p.headline=planLiteral(p,L"Empty");planSelectionDetail(p,0,false);commitTextPlan(p);return;
    }
    auto name=planRecordName(p,getter,static_cast<unsigned>(k[7]));
    p.headline=k[8]>=0?planCountedName(p,name,k[8]):name;
    planSelectionDetail(p,planRecordDescription(p,getter,static_cast<unsigned>(k[7])),setting(Setting::AutoDetail)!=0);
    commitTextPlan(p);
}
Key listKey(uintptr_t p) {
    auto k = windowKey(p);
    if (k[3] < 0 || k[1] < 0 || k[1] >= k[2] || k[1] >= 512) return {};
    if (k[4] == 0x4d6830) {
        const auto table = read<uintptr_t>(address(0x146a9f8));
        if (!table) return {};
        k[5] = read<uint16_t>(table + k[1] * 2); k[6] = -1;
        using Gear=uintptr_t(__cdecl*)(unsigned,uintptr_t*);
        const auto record=reinterpret_cast<Gear>(address(0x3abbf0))(static_cast<unsigned>(k[5]),nullptr);
        // Original 8D6730/8D5720 skip unowned and hidden records. Sold slots
        // retain their old name/abilities and may remain inside this row table.
        k[7]=!record || !read<uint8_t>(record+2) || (read<uint8_t>(record+3)&2);
    } else {
        if (k[4] != 0x4cf9c0 && k[4] != 0x4d16c0 && k[4] != 0x4cdb70 && k[4] != 0x4d5f30) return {};
        auto entry = address(0x1197730) + k[1] * 4;
        // The original Overdrive renderer draws a blank cell for flag0.
        // Keep its cursor event, while withholding that undisplayed record.
        if (k[4] == 0x4d16c0) k[7]=!read<uint8_t>(entry + 3);
        k[5] = read<uint16_t>(entry); k[6] = read<uint8_t>(entry + 2);
    }
    if (k[4] == 0x4d16c0 && k[0] == 3) k[0] = 2;
    k[9] = 1; return k;
}
void listBefore(Event& e) { e.before = listKey(e.args[0]); }
void listAfter(Event& e) {
    if (scene != 4 && scene != 7 && scene != 9 && scene != 0x15) return;
    auto k = listKey(e.args[0]);
    if (!k[9] || k[0] != 2 || k == e.before) return;
    const bool unequip=k[4]==0x4cf9c0 && k[5]==0;
    TextPlan p;prepareTextPlan(p,"field-list");
    if(k[7]) {
        p.headline=planLiteral(p,L"Empty");planSelectionDetail(p,0,false);commitTextPlan(p);return;
    }
    if(unequip) p.headline=planLiteral(p,L"Unequip armor");
    else if (k[6] == -1 || k[4] == 0x4cf9c0) p.headline=planNativeText(p,0x3abe10,static_cast<unsigned>(k[5]));
    else if (k[4] == 0x4d16c0 && k[6] == 2) p.headline=planNativeText(p,0x473af0,static_cast<unsigned>(k[5]));
    else if (k[4] == 0x4d16c0 && k[6] == 3) p.headline=planNativeText(p,0x4ddfa0,static_cast<unsigned>(k[5]) + 0x1033);
    else p.headline=planRecordName(p,0x39a4c0,static_cast<unsigned>(k[5]));
    if(unequip) {
        planSelectionDetail(p,planLiteral(p,L"Remove the selected character's armor."),setting(Setting::AutoDetail)!=0);
    } else if(k[6]==-1 || k[4]==0x4cf9c0) {
        using Gear=uintptr_t(__cdecl*)(unsigned,uintptr_t*);
        const auto record=reinterpret_cast<Gear>(address(0x3abbf0))(static_cast<unsigned>(k[5]),nullptr);
        planSelectionDetail(p,planEquipmentDetails(p,record),setting(Setting::AutoDetail)!=0);
    } else if(k[4]==0x4d16c0 && k[6]==3)
        planSelectionDetail(p,planMenuDescription(p,1,static_cast<unsigned>(k[5])+0x1033),setting(Setting::AutoDetail)!=0);
    else if(k[4]==0x4d16c0 && k[6]==2)
        planSelectionDetail(p,planCharacterDescription(p,static_cast<unsigned>(k[5])),setting(Setting::AutoDetail)!=0);
    else planSelectionDetail(p,planRecordDescription(p,0x39a4c0,static_cast<unsigned>(k[5])),setting(Setting::AutoDetail)!=0);
    commitTextPlan(p);
}
void barBefore(Event& e) { e.before = windowKey(e.args[0]); }
void barAfter(Event& e) {
    auto k = windowKey(e.args[0]);
    if (k[0] != 5 || k[3] < 0 || k[1] < 0 || k[1] >= k[2] || k[1] >= 64 || k == e.before) return;
    TextPlan p;prepareTextPlan(p,"command-bar");
    p.headline=planEncoded(p,read<uintptr_t>(e.args[0]+0x58+k[1]*12));
    planSelectionDetail(p,planEncoded(p,read<uintptr_t>(e.args[0]+0x5c+k[1]*12),1024,TextPlanOp::Lines),setting(Setting::AutoDetail)!=0);
    commitTextPlan(p);
}
Key mainKey(uintptr_t p) {
    Key k{};
    k[0] = read<int>(p + 0x1c, -1);
    k[1] = read<int>(address(0x1471508), -1); k[2] = read<int>(address(0x147150c), -1);
    if (k[0] == 0x0e || k[0] == 0x14) {
        k[3] = read<int>(address(k[0] == 0x0e ? 0x147151c : 0x1471520), -1);
        if (k[3] >= 0 && k[3] < 8) {
            using Getter = uint8_t(__cdecl*)(int);
            k[4] = reinterpret_cast<Getter>(address(0x4a9ab0))(static_cast<int>(k[3]));
        } else k[4] = -1;
        // The native formation code reads the three-byte front-party roster
        // from 385330. The menu row can extend beyond those three members, so
        // cursor position alone cannot identify an active character.
        if(k[4]>=0 && k[4]<18) {
            using Party=uintptr_t(__cdecl*)(int*);
            const auto roster=reinterpret_cast<Party>(address(0x385330))(nullptr);
            uint8_t members[3]{};
            if(roster && copy(roster,members,sizeof(members))) {
                bool valid=true,active=false;
                for(const auto id:members) {
                    if(id!=255 && id>=18) {valid=false;break;}
                    if(id==k[4]) active=true;
                }
                if(valid) {k[5]=active;k[6]=1;}
            }
        }
    }
    return k;
}
void mainBefore(Event& e) { e.before = mainKey(e.args[0]); }
void mainAfter(Event& e) {
    const auto k = mainKey(e.args[0]);
    if(k!=e.before && (k[0]==5 || k[0]==7)) resetSelectionDetail();
    if (k[0] == 5 && (e.before[0] != 5 || k[1] != e.before[1]) && k[1] >= 0 && k[1] < 16) {
        const int logical = read<int>(address(0x1471528) + k[1] * 8, -1);
        if (logical >= 0) {
            TextPlan p;prepareTextPlan(p,"main-menu");
            p.headline=planMenuText(p,9,logical);
            planSelectionDetail(p,planMenuDescription(p,9,logical),setting(Setting::AutoDetail)!=0);
            commitTextPlan(p);
        }
    }
    if (k[0] == 7 && (e.before[0] != 7 || k[2] != e.before[2])) {
        TextPlan p;prepareTextPlan(p,"main-character");
        p.headline=planPlayerName(p,static_cast<int>(k[2]));
        planSelectionDetail(p,planCharacterDescription(p,static_cast<unsigned>(k[2])),setting(Setting::AutoDetail)!=0);
        commitTextPlan(p);
    }
    if ((k[0] == 0x0e || k[0] == 0x14) && k[4] >= 0 && k[4] < 18 &&
        (e.before[0] != k[0] || e.before[3] != k[3] || e.before[4] != k[4] ||
         e.before[5] != k[5] || e.before[6] != k[6]))
        {
            TextPlan p;prepareTextPlan(p,"formation");
            p.headline=planPlayerName(p,static_cast<int>(k[4]));
            if(k[6]) p.headline=planConcat(p,p.headline,planLiteral(p,k[5]?L", active party":L", reserve"));
            planSelectionDetail(p,planCharacterDescription(p,static_cast<unsigned>(k[4])),setting(Setting::AutoDetail)!=0);
            commitTextPlan(p);
        }
}
unsigned aeon() { using Getter = uint8_t(__cdecl*)(); return reinterpret_cast<Getter>(address(0x4a9830))(); }
void aeonBefore(Event& e) { e.before[0] = read<int>(e.args[0] + 0x28, -1); e.before[1] = aeon(); }
void aeonAfter(Event& e) {
    if (scene != 7 || read<int>(e.args[0] + 0x28) != 3) return;
    auto id = aeon();
    if (e.before[0] != 3 || e.before[1] != static_cast<intptr_t>(id)) {
        TextPlan p;prepareTextPlan(p,"aeon-selector");
        p.headline=planPlayerName(p,id);
        planSelectionDetail(p,planCharacterDescription(p,id),setting(Setting::AutoDetail)!=0);
        commitTextPlan(p);
    }
}
void increaseAfter(Event& e) {
    auto k = windowKey(e.args[0]);
    if (scene != 7 || k[0] != 2 || k[3] < 0 || k[1] < 0 || k[1] >= 10 || k[1] >= k[2] || k == e.before) return;
    auto id = read<int>(address(0x85a60c) + k[1] * 0x1c);
    TextPlan p;prepareTextPlan(p,"aeon-increase");
    p.headline=planMenuText(p,12,id);
    planSelectionDetail(p,planMenuDescription(p,12,id),setting(Setting::AutoDetail)!=0);
    commitTextPlan(p);
    announceIncreaseRequirements(static_cast<int>(k[1]));
}
void helpHotkey(bool) {
    refreshBattleResultReview();
    if((escMenuActive() || battleResultsReviewActive()) && readSelectionDetail()) return;
    if(readBattleSummonDetail()) return;
    if(statusReviewAvailable() && readSelectionDetail()) return;
    if(readSphereDescription()) return;
    if(readSelectionDetail()) return;
    TextPlan p;prepareTextPlan(p,"help-hotkey");
    const auto native=inBattle()?planEncoded(p,address(0xf3f4f8),128,TextPlanOp::Lines):0;
    p.headline=planPair(p,TextPlanOp::Or,native,planLiteral(p,L"No native description is available for this selection."));
    commitTextPlan(p);
}
void namingHelpHotkey(bool shift) {
    if(!shift && readEnteredName()) return;
    helpHotkey(shift);
}
void detailHotkey(bool shift) {
    if(!shift) {helpHotkey(false);return;}
    if(readEscControls()) return;
    if(readHelpControls()) return;
    TextPlan p;prepareTextPlan(p,"control-help-hotkey");p.headline=planLiteral(p,L"No controls displayed.");commitTextPlan(p);
}
void customizeBefore(Event& e) {
    if(scene!=9) return;
    const auto state=read<int>(address(0x146aa28),-1);
    if(state!=12 && state!=13) return;
    const auto table=read<uintptr_t>(address(0x146a9f8));
    const int row=read<int16_t>(e.args[0]+0x48,-1);
    if(!table || row<0 || row>=read<uint16_t>(e.args[0]+0x30) || row>=512) return;
    e.before[0]=1; e.before[1]=read<uint16_t>(table+row*2,0xffff);
}
void customizeAfter(Event& e) {
    // C/D -> A is the native completed customization result. C cancel -> 6.
    if(scene!=9 || !e.before[0] || e.before[1]==0xffff || read<int>(address(0x146aa28),-1)!=10) return;
    const auto id=static_cast<unsigned>(e.before[1]);
    TextPlan p;prepareTextPlan(p,"customized-equipment",false);
    p.headline=planNativeText(p,0x3abe10,id);
    using Gear=uintptr_t(__cdecl*)(unsigned,uintptr_t*);
    planSelectionDetail(p,planEquipmentDetails(p,reinterpret_cast<Gear>(address(0x3abbf0))(id,nullptr)),false);
    commitTextPlan(p);
}
}
void installSaveLoadMenu();
void installFieldDetails();
void installAeonAttributes();
void installFieldTargets();
void installFieldMenus() {
    installTextPlans();
    hook(0x4e0340, mainBefore, mainAfter);
    hook(0x4c6da0, itemBefore, itemAfter);
    hook(0x4b4460, listBefore, listAfter); hook(0x4d0e80, listBefore, listAfter);
    hook(0x4e3880, barBefore, barAfter);
    hook(0x4ccea0, aeonBefore, aeonAfter);
    hook(0x4cd6a0, barBefore, increaseAfter);
    hook(0x4d5800, customizeBefore, customizeAfter);
    hotkey('I', detailHotkey);
    hotkey('N', namingHelpHotkey);
    installSaveLoadMenu();
    installFieldDetails();
    installAeonAttributes();
    installFieldTargets();
}
}
