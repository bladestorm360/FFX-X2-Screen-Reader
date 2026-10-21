#include "events.hpp"
#include "settings.hpp"
#include "message_text.hpp"
#include "battle_sensor.hpp"
#include "overdrive.hpp"
#include "wakka.hpp"
#include "ready_game_text.hpp"
#include "game_visual_text.hpp"
#include "battle_hud.hpp"
#include <cmath>
#include <algorithm>

namespace ffx {
void queueBattleActiveStats(unsigned,uintptr_t);
void installBattleResults();
void resetBattleResults();
void installBattleAnnouncements();
void resetBattleAnnouncements();
void announceBattleFormation(bool complete=false);
void battleOpeningMessage(uintptr_t);
namespace {
int previousRow = -1, previousCommand = -1;
unsigned previousPlayers = 0, previousMonsters = 0, lastCommand = 0;
unsigned previousSeymour=0;
bool targeting = false, previewMp = false;
uintptr_t previousTargetWindow=0;
unsigned previousTargetOwner=255;
bool previousSensor=false;
// Numeric pay windows have no command-list cursor. Track the native rendered
// amount and selected decimal place, retiring on every window transition.
struct AmountFocus {uintptr_t window=0;unsigned epoch=0,owner=255;int amount=-1,digit=-1;};
AmountFocus amountFocus;
uintptr_t currentWindow() {
    const auto index = read<uint8_t>(address(0x1fcc092), 255);
    return index <= 15 ? address(0xf3c910) + index * 0xf0 : 0;
}
TextRef summonDetail(TextPlan& p,unsigned command) {
    return planPair(p,TextPlanOp::SpaceJoin,planMessage(p,0,address(0xf3f4f8)),planBattleSelectionVitals(p,command));
}
TextRef commandName(TextPlan& p,uintptr_t win,unsigned id,unsigned type) {
    (void)win;
    if(type==7 || type==15) return planNativeText(p,0x3abe10,id);
    if(type==10 || (type==5 && id!=0x30ff)) return planBattleName(p,id&255);
    const auto text=planRecordName(p,0x39a4c0,id);
    if((id&0xf000)!=0x2000) return text;
    const auto line=planConcat(p,text,planConcat(p,planLiteral(p,L" ("),
        planConcat(p,planInteger(p,quantity(id)),planLiteral(p,L")"))));
    return planPair(p,TextPlanOp::IfText,text,line);
}
void highlighted(uintptr_t win) {
    if (!inBattle() || !win || read<uint8_t>(win + 1) != 4 || !read<unsigned>(win + 0x44)) return;
    const int row = read<uint8_t>(win + 0x42);
    const auto list=read<uintptr_t>(win+0x20);
    const auto count=read<unsigned>(win+0x24);
    if(!list || count>0x1000 || static_cast<unsigned>(row)>=count) return;
    const int id=read<uint16_t>(list+row*2);
    if (row == previousRow && id == previousCommand) return;
    previousRow = row; previousCommand = id;
    lastCommand=id;
    const auto type=read<uint16_t>(win+6);
    TextPlan p;prepareTextPlan(p,"battle-command");TextRef detail=0;
    if(type==7 || type==15) {
        using Gear=uintptr_t(__cdecl*)(unsigned,uintptr_t*);
        detail=planEquipmentDetails(p,reinterpret_cast<Gear>(address(0x3abbf0))(lastCommand,nullptr));
    } else if(type==10) detail=planBattleSelectionVitals(p,lastCommand);
    else if(type==5) detail=summonDetail(p,lastCommand);
    else {
        const auto description=planRecordDescription(p,0x39a4c0,lastCommand);
        detail=planPair(p,TextPlanOp::Or,description,planEncoded(p,address(0xf3f4f8),128,TextPlanOp::Lines));
    }
    const auto name=commandName(p,win,id,type);
    p.headline=type==10?planSuffix(p,name,detail,L", "):name;
    planSelectionDetail(p,detail,type!=10 && (setting(Setting::CombatDetail)!=0 || setting(Setting::AutoDetail)!=0));
    commitTextPlan(p);

}
void cursor(Event& e) {
    if (targeting) { targeting = false; previousPlayers = previousMonsters = 0; resetBattleSensor(); }
    highlighted(e.args[0]);
}
void pop(Event&) { amountFocus={};resetBattleSensor(); previousRow = previousCommand = -1; highlighted(currentWindow()); }
void opened(Event&) {
    amountFocus={};
    resetBattleSensor();
    if (!inBattle()) return;
    announceBattleFormation();
    const auto win = currentWindow();
    if (!win) return;
    previousRow = previousCommand = -1;
    if(read<uint16_t>(win+6)==0) {
        resetSelectionDetail();TextPlan p;prepareTextPlan(p,"battle-turn",false);
        p.headline=planBattleName(p,read<uint8_t>(win+8));commitTextPlan(p);
    }
    else highlighted(win);
}
void amountDrawn(Event& e) {
    // Original4A3250 draws the native label, selected digit and nine-digit
    // amount only outside automatic battle mode. This is a render event,
    // never a timer or passive watch of the amount's backing global.
    if(!nativeGameThread() || e.epoch!=generation())return;
    const auto win=e.args[0];
    if(!inBattle() || scene || !win || win!=currentWindow() || read<uint8_t>(win+1)!=4 ||
        read<int>(address(0xf3d6a0))==1) {amountFocus={};return;}
    const auto type=read<uint16_t>(win+6);
    if(type!=0x15 && type!=0x16) {amountFocus={};return;}
    const int amount=read<int>(address(0xf40424),-1),digit=read<int16_t>(address(0xf40418),-1);
    const auto owner=read<uint8_t>(win+8,255);
    if(owner>=18 || amount<0 || amount>999999999 || digit<0 || digit>8) return;
    if(amountFocus.epoch==generation() && amountFocus.window==win && amountFocus.owner==owner &&
        amountFocus.amount==amount && amountFocus.digit==digit) return;
    unsigned step=1;for(int i=0;i<digit;++i)step*=10;
    TextPlan p;prepareTextPlan(p,"battle-gil-amount");
    // +B4 is the localized label just obtained by the original renderer from
    // native namespace0/string3014. Copy it before any other text getter runs.
    const auto label=planEncoded(p,read<uintptr_t>(win+0xb4),256);
    p.headline=planConcat(p,planSuffix(p,label,planInteger(p,amount),L": "),
        planConcat(p,planLiteral(p,L" Gil, step "),planInteger(p,step)));
    if(commitTextPlan(p)) amountFocus={win,generation(),owner,amount,digit};
}
// Unique visible wheel mechanics, qualified by both native model and ATEL ID.
// Original m106 dispatches actor21..24 to shared words4/8/C/10; m131 counts
// their elements before choosing its spells/affinities. Shared battle storage
// D34568 is supplied to controllers2/3/4 by original7983A0/875650, independent
// of whichever interpreter happens to be current when the target UI returns.
bool monsterScript(uintptr_t chr,unsigned model,const char* id) {
    if(!chr || !read<uint8_t>(chr+0xdc8) || read<unsigned>(chr+4)!=model) return false;
    const auto script=read<uintptr_t>(chr+0xf78);
    const auto offset=read<unsigned>(script+0xc);
    char name[4]{};
    return script && offset>=0x38 && offset<4096 && copy(script+offset,name,sizeof(name)) &&
        name[0]==id[0] && name[1]==id[1] && name[2]==id[2] && !name[3];
}
struct SeymourWheels {uintptr_t actors=0;unsigned key=0;std::array<unsigned,4> element{};};
SeymourWheels seymourWheels(unsigned monsters) {
    SeymourWheels s;
    if(!(monsters&0x1f0))return s;
    const auto actors=read<uintptr_t>(address(0xd34460));
    if(!monsterScript(actors,0x1083,"131"))return s;
    // Element order matches the verified native Sensor convention.
    constexpr unsigned colors[4][4]={{1,0,3,2},{3,2,1,0},{0,3,2,1},{2,1,0,3}};
    std::array<int,4> positions{};
    if(!copy(address(0xd34568)+4,positions.data(),sizeof(positions)))return s;
    unsigned key=0x100;
    for(unsigned i=0;i<4;++i) {
        if(positions[i]<0 || positions[i]>3 || !monsterScript(actors+(i+1)*0xf90,0x106a,"106"))return {};
        s.element[i]=colors[i][positions[i]];key|=static_cast<unsigned>(positions[i])<<(i*2);
    }
    s.actors=actors;s.key=key;return s;
}
TextRef wheelMechanic(TextPlan& p,const SeymourWheels& s,unsigned id) {
    constexpr const wchar_t* elements[]={L"Fire",L"Lightning",L"Water",L"Ice"};
    if(!s.key || id<20 || id>24)return 0;
    if(id!=20) return planConcat(p,planLiteral(p,L"Energizing "),
        planConcat(p,planEncoded(p,s.actors+0x540,40),planConcat(p,planLiteral(p,L" with "),
        planConcat(p,planLiteral(p,elements[s.element[id-21]]),
            planLiteral(p,L". Physical attacks turn left; magic turns right.")))));
    std::array<unsigned,4> counts{};for(const auto element:s.element)++counts[element];
    TextRef mix=0;
    for(unsigned i=0;i<4;++i)if(counts[i])mix=planJoin(p,mix,
        planSuffix(p,planLiteral(p,elements[i]),planInteger(p,counts[i]),L" "));
    return planConcat(p,planLiteral(p,L"Energizing wheels: "),mix);
}
void targets(Event& e) {
    if (!nativeGameThread() || e.epoch!=generation() || !inBattle()) return;
    const auto win = e.args[0];
    const unsigned players = read<uint16_t>(win + 0xd8), monsters = read<uint16_t>(win + 0xda);
    if (!players && !monsters) { targeting = false; previousPlayers = previousMonsters = previousSeymour = 0; resetBattleSensor(); return; }
    const auto owner=read<uint8_t>(win+8);
    const auto selected=read<uint16_t>(address(0xf3d1b4),255);
    const auto wheels=seymourWheels(monsters);
    const bool sensor=read<uint8_t>(win+2)==1 && read<int>(address(0xf3d6a0))!=1 && battleSensorVisible(selected);
    highlightBattleSensor(win);
    if (players == previousPlayers && monsters == previousMonsters && previousTargetWindow==win && previousTargetOwner==owner && previousSensor==sensor && previousSeymour==wheels.key) return;
    if(previousTargetWindow!=win || previousTargetOwner!=owner) targeting=false;
    if (!targeting) {
        targeting = true; previewMp = false;
        // Same data field used by the game's command HP/MP preview.
        if (lastCommand) {
            using Getter = uintptr_t(__cdecl*)(unsigned, uintptr_t*);
            uintptr_t base = 0;
            auto record = reinterpret_cast<Getter>(address(0x39a4c0))(lastCommand, &base);
            if (record) previewMp = (read<uint8_t>(record + 0x22) & 2) != 0;
        }
    }
    TextPlan p;prepareTextPlan(p,"battle-target");TextRef names=0;
    const auto monstersArray=read<uintptr_t>(address(0xd34460));
    for(unsigned bit=4;bit<12;++bit) if((monsters&(1u<<bit)) && monstersArray) {
        const auto chr=monstersArray+(bit-4)*0xf90;
        if(!read<uint8_t>(chr+0xdc8)) continue;
        const auto name=planEncoded(p,chr+0x540,40);
        // Native current/effective maximum, shared with Sensor and Change.
        // +6E4 is an uncapped action forecast, not the displayed HP total.
        int hp=read<int>(chr+0x5d0),maximum=read<int>(chr+0x594);unsigned kind=0;
        if(sensor && selected==bit+16 && battleSensorVisible(bit+16)) {
            using Getter=uintptr_t(__cdecl*)(unsigned);const auto actor=reinterpret_cast<Getter>(address(0x394030))(bit+16);
            hp=read<int>(actor+0x5d0);maximum=read<int>(actor+0x594);kind=1;
        }
        auto target=planBattleTarget(p,name,hp,maximum,kind);
        if(selected==bit+16) {
            const auto mechanic=wheelMechanic(p,wheels,selected);
            if(mechanic)target=planSuffix(p,target,mechanic,L", ");
        }
        names=planJoin(p,names,target);
    }
    const auto array=read<uintptr_t>(address(0xd334cc));
    for(unsigned bit=0;bit<18;++bit) if((players&(1u<<bit)) && array) {
        const auto chr=array+bit*0xf90;if(!read<uint8_t>(chr+0xdc8)) continue;
        names=planJoin(p,names,planBattleTarget(p,planBattleName(p,bit),
            read<int>(chr+(previewMp?0x5d4:0x5d0)),read<int>(chr+(previewMp?0x598:0x594)),previewMp?2:1));
    }
    p.headline=names;commitTextPlan(p); previousPlayers = players; previousMonsters = monsters;
    previousTargetWindow=win; previousTargetOwner=owner; previousSensor=sensor;previousSeymour=wheels.key;
}
void whoseTurn(bool) {
    if (!inBattle()) return;
    const auto win = currentWindow();
    if (win && read<uint8_t>(win + 1)) {
        TextPlan p;prepareTextPlan(p,"battle-turn-hotkey");
        p.headline=planBattleName(p,read<uint8_t>(win+8));commitTextPlan(p);
    }
}
uintptr_t activeCharacter(unsigned& id) {
    if(!inBattle()) return 0;
    const auto win=currentWindow();
    if(!win || !read<uint8_t>(win+1)) return 0;
    id=read<uint8_t>(win+8,255);
    if(id>=18) return 0;
    using Getter=uintptr_t(__cdecl*)(unsigned);
    const auto actor=reinterpret_cast<Getter>(address(0x394030))(id);
    return actor && read<uint8_t>(actor+0xdc8)?actor:0;
}
void overdriveMeter(bool) {
    unsigned id=255;
    if(!activeCharacter(id)) return;
    using Getter=uint8_t(__cdecl*)(unsigned);
    const unsigned current=reinterpret_cast<Getter>(address(0x395560))(id);
    const unsigned maximum=reinterpret_cast<Getter>(address(0x3955a0))(id);
    if(!maximum) return;
    auto percent=std::min(100u,(current*100+maximum/2)/maximum);
    visibleOverdrive(id,percent);
    TextPlan p;prepareTextPlan(p,"battle-overdrive-meter");
    p.headline=planConcat(p,planConcat(p,planBattleName(p,id),planLiteral(p,L" Overdrive ")),
        planConcat(p,planInteger(p,percent),planLiteral(p,L"%")));
    commitTextPlan(p);
}
void activeStats(bool) {
    unsigned id=255;
    const auto chr=activeCharacter(id);
    if(!chr) return;
    queueBattleActiveStats(id,chr);
}
void damage(Event& e) {
    // MsNumberRegist returns0 only after registering the visible glyphs.
    // A full native number pool returns-1; do not speak a rejected popup.
    if(e.result!=0) return;
    // MsNumberRegist kinds 0/1 are HP/MP. Kinds 2 and 5 and values above
    // 5 are handled by their own producers or ignored; 3/4 are visible labels.
    if(e.args[1]==3 || e.args[1]==4) {
        const auto label=battleGraphicLabel(e.args[1]==3?BattleGraphic::Miss:BattleGraphic::Guard);
        if(!label || !*label) return;
        TextPlan p;prepareTextPlan(p,e.args[1]==3?"battle-miss":"battle-guard",false);
        p.headline=planConcat(p,planConcat(p,planBattleName(p,e.args[0]&255),planLiteral(p,L": ")),
            planLiteral(p,label));
        commitTextPlan(p);return;
    }
    if(e.args[1]>1) return;
    const int value = static_cast<int>(e.args[2]);
    // The renderer retains the healing color for a zero result when the
    // original amount (arg3) was negative, e.g. restoration at full vitals.
    const bool healing=value<0 || (value==0 && static_cast<int>(e.args[3])<0);
    const auto amount=static_cast<int64_t>(value<0?-static_cast<int64_t>(value):value);
    TextPlan p;prepareTextPlan(p,"battle-damage",false);
    const auto unit=e.args[1]==1;
    const auto tail=unit
        ? planConcat(p,planLiteral(p,healing?L"restored ":L""),
            planConcat(p,planInteger(p,amount),planLiteral(p,healing?L" MP":L" MP damage")))
        : planConcat(p,planLiteral(p,healing?L"healed for ":L""),
            planConcat(p,planInteger(p,amount),planLiteral(p,healing?L"":L" damage")));
    p.headline=planConcat(p,planConcat(p,planBattleName(p,e.args[0]&255),planLiteral(p,L": ")),tail);
    commitTextPlan(p);
}
void actionBefore(Event& e) {
    resetBattleSensor();
    e.before[30]=GetTickCount();
    recordLog("battle action banner enter command=%x owner=%lu",static_cast<unsigned>(e.args[0]),GetCurrentThreadId());
}
void action(Event& e) {
    recordLog("battle action banner return command=%x call-ms=%lu owner=%lu",static_cast<unsigned>(e.args[0]),
        GetTickCount()-static_cast<DWORD>(e.before[30]),GetCurrentThreadId());
    if (!inBattle()) return;
    const auto array = read<uintptr_t>(address(0xd334cc));
    unsigned owner=31;
    if(array) for(unsigned i=0;i<31;++i) if(read<uint8_t>(array+i*0xf90+0xdf0)==3) {owner=i;break;}
    TextPlan p;prepareTextPlan(p,"battle-action",false);
    const auto text=planRecordName(p,0x39a4c0,e.args[0]&0xffff);
    // The native command owns its name even when no actor currently has the
    // optional action-state flag. Do not lose debuff banners on that condition.
    const auto line=owner<31?planConcat(p,planConcat(p,planBattleName(p,owner),planLiteral(p,L": ")),text):text;
    p.headline=planPair(p,TextPlanOp::IfText,text,line);commitTextPlan(p);
}
void message(Event& e) {
    if(inBattle() && e.args[0]==0) battleOpeningMessage(e.args[1]);
    else if(inBattle() && e.args[0]==1 && e.parentRva!=0x498bd0) {
        // Slot1 also receives direct native ability/result messages. The nested
        // banner call is owned by action() above, so it has one producer.
        TextPlan p;prepareTextPlan(p,"battle-action-message",false);
        p.headline=planMessage(p,7,e.args[1],true);commitTextPlan(p);
    }
}
void special(Event& e) {
    if(!inBattle() || !e.result) return;
    // MsBtl message queue preserves the exact item/quantity/ability arguments.
    if(e.args[0]!=4 && e.args[0]!=6 && e.args[0]!=8) return;
    TextPlan p;prepareTextPlan(p,"battle-special",false);
    if(e.args[0]==4) p.headline=planConcat(p,planRecordName(p,0x39a4c0,e.args[1]&0xffff),
        planConcat(p,planLiteral(p,L" x"),planInteger(p,e.args[2])));
    else if(e.args[0]==6) p.headline=planConcat(p,
        planConcat(p,planBattleName(p,e.args[1]&255),planLiteral(p,L": ")),
        planRecordName(p,0x39a4c0,e.args[2]&0xffff));
    else p.headline=planConcat(p,planInteger(p,e.args[1]),planLiteral(p,L" Gil"));
    commitTextPlan(p);
}

}
bool readBattleSummonDetail() {
    const auto win=currentWindow();
    if(!inBattle() || !win || read<uint8_t>(win+1)!=4 || read<uint16_t>(win+6)!=5 || !read<unsigned>(win+0x44)) return false;
    const auto list=read<uintptr_t>(win+0x20),count=read<unsigned>(win+0x24);
    const auto row=read<uint8_t>(win+0x42);
    if(!list || count>0x1000 || row>=count) return false;
    TextPlan p;prepareTextPlan(p,"battle-summon-detail");
    planSelectionDetail(p,summonDetail(p,read<uint16_t>(list+row*2)),false);commitTextPlan(p);
    return readSelectionDetail();
}
void installBattleGraphics();
void resetBattleGraphics();
void installBattleStatus();
void resetBattleStatus();
void installOverdrive();
void resetBattle() {
    amountFocus={};
    resetBattleHud();
    resetBattleGraphics();
    previousRow = previousCommand = -1; previousPlayers = previousMonsters = lastCommand = 0;
    previousTargetWindow=0; previousTargetOwner=255; previousSensor=false;previousSeymour=0;
    targeting = previewMp = false; resetWakka(); resetOverdrive(); resetBattleSensor(); resetBattleStatus(); resetBattleResults(); resetBattleAnnouncements();
}
void installBattle() {
    installBattleHud();
    installBattleGraphics();
    hook(0x39fa20, nullptr, damage); hook(0x498bd0, actionBefore, action);
    hook(0x49db10,message,nullptr); hook(0x39cff0,nullptr,special);
    installBattleResults(); installBattleAnnouncements();
    hook(0x49a5a0, nullptr, pop); hook(0x4a0d70, nullptr, opened);
    for (auto rva : {0x49c870,0x49cd50,0x49d580,0x4a2100,0x4a2290,0x4a2630}) hook(rva, nullptr, cursor);
    hook(0x49d300, nullptr, targets);
    hook(0x4a3250,nullptr,amountDrawn);
    hotkey('T', whoseTurn); hotkey('H', activeStats);
    hotkeyChordWhen('O',false,overdriveMeter,[] { return inBattle(); });
    installBattleStatus(); installBattleSensor(); installOverdrive();
}
}
