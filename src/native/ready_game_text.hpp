#pragma once
#include "text_plan.hpp"
namespace ffx {
TextRef planPlayerName(TextPlan&,int);
TextRef planBattleSelectionVitals(TextPlan&,unsigned);
TextRef planBattleName(TextPlan&,unsigned);
struct ReadyBattleName {
    std::array<uint8_t,40> actor{},player{};
    unsigned actorLength=0,playerLength=0;
};
void copyReadyBattleName(ReadyBattleName&,unsigned);
std::wstring renderReadyBattleName(const ReadyBattleName&,const OwnedTextContext&);
TextRef planNativeText(TextPlan&,unsigned getter,unsigned id);
TextRef planRecordName(TextPlan&,unsigned getter,unsigned id);
TextRef planRecordDescription(TextPlan&,unsigned getter,unsigned id);
TextRef planMenuText(TextPlan&,int space,unsigned id);
TextRef planMenuDescription(TextPlan&,int space,unsigned id);
TextRef planEquipmentName(TextPlan&,uintptr_t);
TextRef planEquipmentDetails(TextPlan&,uintptr_t);
TextRef planEquippedItemName(TextPlan&,unsigned,bool armor);
TextRef planCharacterDescription(TextPlan&,unsigned,bool includeLevels=true);
TextRef planShopEquipmentDescription(TextPlan&,uintptr_t);
TextRef planCountedName(TextPlan&,TextRef,int64_t); // Empty name suppresses quantity.
void planSelectionDetail(TextPlan&,TextRef,bool automatic);
}
