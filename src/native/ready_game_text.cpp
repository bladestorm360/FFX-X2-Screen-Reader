#include "ready_game_text.hpp"
#include "events.hpp"
#include "settings.hpp"
#include <algorithm>
namespace ffx {
namespace {
using Language=unsigned(__cdecl*)();
unsigned language() {return reinterpret_cast<Language>(address(0x3852b0))();}
TextRef labelNumber(TextPlan& p,const wchar_t* label,TextRef number) {
    return planConcat(p,planLiteral(p,label),number);
}
}
TextRef planPlayerName(TextPlan& p,int id) {
    if(id<0 || id>=20) return 0;
    using Getter=uintptr_t(__cdecl*)(unsigned);
    return planEncoded(p,reinterpret_cast<Getter>(address(0x385070))(id),40);
}
TextRef planBattleName(TextPlan& p,unsigned id) {
    if(id>=31) return 0;
    const auto array=read<uintptr_t>(address(0xd334cc));
    const auto name=array?planEncoded(p,array+id*0xf90+0x540,40):0;
    return planPair(p,TextPlanOp::Or,name,planPlayerName(p,id));
}
TextRef planBattleSelectionVitals(TextPlan& p,unsigned command) {
    using Getter=uintptr_t(__cdecl*)(unsigned);
    const auto vitals=[&](unsigned id) -> TextRef {
        if(id>=18) return 0;
        const auto actor=reinterpret_cast<Getter>(address(0x394030))(id);
        if(!actor || !read<uint8_t>(actor+0x10)) return 0;
        // Original492C20 uses39ADE0/39AE00/39AE60: current HP, effective
        // battle maximum and current MP. +594 already includes native capacity
        // changes; +59C is the separate base maximum and must not replace it.
        const auto hp=planVitals(p,read<int>(actor+0x5d0),read<int>(actor+0x594),false,setting(Setting::ShowMaximumHp)!=0);
        return planConcat(p,hp,planConcat(p,planLiteral(p,L", MP "),planInteger(p,read<int>(actor+0x5d4))));
    };
    if(command!=0x30ff) return vitals(command&255);
    TextRef result=0;
    for(unsigned id=15;id<18;++id) {
        const auto detail=vitals(id);if(!detail) continue;
        const auto line=planConcat(p,planBattleName(p,id),planConcat(p,planLiteral(p,L", "),detail));
        result=planPair(p,TextPlanOp::Periods,result,line);
    }
    return result;
}
void copyReadyBattleName(ReadyBattleName& name,unsigned id) {
    if(id>=31) return;
    const auto array=read<uintptr_t>(address(0xd334cc));
    if(array) name.actorLength=copyReadyGameText(array+id*0xf90+0x540,name.actor.data(),static_cast<unsigned>(name.actor.size()));
    if(id<20) {
        using Getter=uintptr_t(__cdecl*)(unsigned);
        name.playerLength=copyReadyGameText(reinterpret_cast<Getter>(address(0x385070))(id),name.player.data(),static_cast<unsigned>(name.player.size()));
    }
}
std::wstring renderReadyBattleName(const ReadyBattleName& name,const OwnedTextContext& context) {
    if(name.actorLength>name.actor.size() || name.playerLength>name.player.size()) return {};
    const auto actor=decodeOwnedBytes(name.actor.data(),name.actorLength,OwnedTextMode::Terminated,context);
    if(!actor.empty() && !actor.front().empty()) return actor.front();
    const auto player=decodeOwnedBytes(name.player.data(),name.playerLength,OwnedTextMode::Terminated,context);
    return player.empty()?std::wstring{}:player.front();
}
TextRef planNativeText(TextPlan& p,unsigned getter,unsigned id) {
    using Getter=uintptr_t(__cdecl*)(unsigned);
    return planEncoded(p,reinterpret_cast<Getter>(address(getter))(id));
}
TextRef planRecordName(TextPlan& p,unsigned getter,unsigned id) {
    using Getter=uintptr_t(__cdecl*)(unsigned,uintptr_t*);
    uintptr_t base=0;const auto record=reinterpret_cast<Getter>(address(getter))(id,&base);
    if(!record || !base) return 0;
    return planEncoded(p,base+read<uint16_t>(record+(language()&1)*4));
}
TextRef planRecordDescription(TextPlan& p,unsigned getter,unsigned id) {
    if(getter==0x39a4c0) {
        const auto type=id>>12;if(type!=2 && type!=3 && type!=4 && type!=6 && type!=8) return 0;
    }
    using Getter=uintptr_t(__cdecl*)(unsigned,uintptr_t*);
    uintptr_t base=0;const auto record=reinterpret_cast<Getter>(address(getter))(id,&base);
    if(!record || !base) return 0;
    return planEncoded(p,base+read<uint16_t>(record+8+(language()&1)*4),1024,TextPlanOp::Lines);
}
TextRef planMenuText(TextPlan& p,int space,unsigned id) {
    using Getter=uintptr_t(__cdecl*)(int,unsigned,unsigned);
    const auto lang=language();return planEncoded(p,reinterpret_cast<Getter>(address(0x38fd40))(space,id,lang));
}
TextRef planMenuDescription(TextPlan& p,int space,unsigned id) {
    using Getter=uintptr_t(__cdecl*)(int,unsigned,unsigned);
    const auto lang=language();
    if(space==1 && id>=0x1033 && id<=0x104f)
        return planEncoded(p,reinterpret_cast<Getter>(address(0x38fc00))(space,id,lang),1024,TextPlanOp::Lines);
    return planEncoded(p,reinterpret_cast<Getter>(address(0x38fd40))(space,id,lang),1024,TextPlanOp::Description);
}
TextRef planEquipmentName(TextPlan& p,uintptr_t record) {
    if(!record) return 0;
    using Name=uintptr_t(__cdecl*)(unsigned,unsigned,int,uintptr_t);
    return planEncoded(p,reinterpret_cast<Name>(address(0x3a0c70))(read<uint16_t>(record),
        read<uint8_t>(record+4),static_cast<int>(language()),0));
}
TextRef planEquipmentDetails(TextPlan& p,uintptr_t record) {
    if(!record) return 0;
    const auto slots=read<uint8_t>(record+0xb);if(slots>4) return 0;
    TextRef result=0;unsigned filled=0;std::array<uint16_t,4> ids{};
    for(unsigned i=0;i<4;++i) ids[i]=read<uint16_t>(record+0xe+i*2,0xff);
    // Keep all four visible named ability records, including equal names.
    for(const auto id:ids) {
        if(id && id!=0xff && id!=0xffff) {++filled;result=planJoin(p,result,planRecordName(p,0x39a4c0,id));}
    }
    const auto empty=std::max<unsigned>(slots,filled)-filled;
    if(empty) result=planJoin(p,result,planConcat(p,planInteger(p,empty),
        planLiteral(p,empty==1?L" empty slot":L" empty slots")));
    return result;
}
TextRef planEquippedItemName(TextPlan& p,unsigned character,bool armor) {
    const auto id=equippedItem(character,armor);
    if(id==255) return planPair(p,TextPlanOp::Or,planMenuText(p,11,0x17),planLiteral(p,L"Nothing equipped"));
    const auto name=id<200?planNativeText(p,0x3abe10,id):0;
    return planEquipmentFallback(p,name,character,armor,id);
}
TextRef planCharacterDescription(TextPlan& p,unsigned id,bool includeLevels) {
    if(id>=20) return 0;
    using Saved=uintptr_t(__cdecl*)(unsigned);
    const auto record=reinterpret_cast<Saved>(address(0x3853f0))(id);if(!record) return 0;
    const auto hp=read<unsigned>(record+0x1c),maxHp=read<unsigned>(record+0x24);
    const auto mp=read<unsigned>(record+0x20),maxMp=read<unsigned>(record+0x28);
    const auto levels=read<uint8_t>(record+0x3b);
    auto result=labelNumber(p,L"HP ",planInteger(p,hp));
    result=planConcat(p,result,labelNumber(p,L"/",planInteger(p,maxHp)));
    result=planConcat(p,result,labelNumber(p,L", MP ",planInteger(p,mp)));
    result=planConcat(p,result,labelNumber(p,L"/",planInteger(p,maxMp)));
    return includeLevels?planConcat(p,result,labelNumber(p,L", Sphere Levels ",planInteger(p,levels))):result;
}
TextRef planShopEquipmentDescription(TextPlan& p,uintptr_t record) {
    const auto owner=read<uint8_t>(address(0x146ab60),255),type=read<uint8_t>(record+5,255);
    if(!record || owner>=7 || read<uint8_t>(record+4,255)>=7 || type>1) return 0;
    using Character=uintptr_t(__cdecl*)(unsigned);using Gear=uintptr_t(__cdecl*)(unsigned,uintptr_t*);
    const auto saved=reinterpret_cast<Character>(address(0x3853f0))(owner);if(!saved) return 0;
    const auto index=read<uint8_t>(saved+0x2d+type,255);
    const auto current=index==255?0:reinterpret_cast<Gear>(address(0x3abbf0))(index,nullptr);
    const auto status=planCharacterDescription(p,owner,false),name=planPlayerName(p,owner);
    auto result=planConcat(p,name,planLiteral(p,L". "));result=planConcat(p,result,status);
    result=planConcat(p,result,planLiteral(p,L". Current equipment: "));
    auto equipment=planLiteral(p,L"None");
    if(current) {const auto label=planEquipmentName(p,current);equipment=planConcat(p,planConcat(p,label,planLiteral(p,L", ")),planEquipmentDetails(p,current));}
    result=planConcat(p,result,equipment);result=planConcat(p,result,planLiteral(p,L". Selected equipment: "));
    return planConcat(p,result,planEquipmentDetails(p,record));
}
TextRef planCountedName(TextPlan& p,TextRef name,int64_t count) {
    const auto line=planConcat(p,name,labelNumber(p,L", ",planInteger(p,count)));
    return planPair(p,TextPlanOp::IfText,name,line);
}
void planSelectionDetail(TextPlan& p,TextRef description,bool automatic) {
    p.description=description;p.automatic=automatic;p.detail=beginSelectionDetail();
}
}
