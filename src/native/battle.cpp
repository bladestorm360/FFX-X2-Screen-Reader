#include "events.hpp"
#include "settings.hpp"
#include "message_text.hpp"
#include "battle_sensor.hpp"
#include "overdrive.hpp"
#include "wakka.hpp"
#include "ready_game_text.hpp"
#include "game_visual_text.hpp"
#include "battle_hud.hpp"
#include "menu_list_text.hpp"
#include "profile.hpp"
#include <cmath>
#include <algorithm>
#include <cstring>

namespace ffx {
void queueBattleActiveStats(unsigned,uintptr_t);
void installBattleResults();
void resetBattleResults();
void installBattleAnnouncements();
void resetBattleAnnouncements();
void announceBattleFormation(bool complete=false);
void battleOpeningMessage(uintptr_t);
namespace {
enum class RowAvailability { Usable, Unavailable, Hidden };
struct CommandFocus {
    uintptr_t window=0;
    unsigned epoch=0,type=0;
    int row=0,id=0,actor=0;
    bool operator==(const CommandFocus& other) const {
        return window==other.window && epoch==other.epoch && type==other.type &&
            row==other.row && id==other.id && actor==other.actor;
    }
};
CommandFocus spokenCommand{},renderedCommand{};
RowAvailability spokenAvailability=RowAvailability::Usable;
uintptr_t commandDrawReturn=0,listDrawReturn=0,expectedDrawReturn=0;
void resetCommandFocus() {
    spokenCommand={};renderedCommand={};expectedDrawReturn=0;
}
unsigned previousPlayers = 0, previousMonsters = 0;
unsigned previousSeymour=0;
bool previewMp = false;
uintptr_t previousTargetWindow=0;
unsigned previousTargetOwner=255,previousTargetCommand=0xffff;
bool previousSensor=false;
void resetTargetPreview() {
    previousPlayers=previousMonsters=previousSeymour=0;
    previousTargetWindow=0;previousTargetOwner=255;previousTargetCommand=0xffff;
    previousSensor=previewMp=false;
}
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
CommandFocus commandFocus(uintptr_t win) {
    if(!win || read<uint8_t>(win+1)!=4 || !read<unsigned>(win+0x44)) return {};
    const auto row=read<int16_t>(win+0x42,-1);
    const auto list=read<uintptr_t>(win+0x20);
    const auto count=read<unsigned>(win+0x24);
    if(!list || count>0x1000 || static_cast<unsigned>(row)>=count) return {};
    return {win,generation(),read<uint16_t>(win+6),row,read<uint16_t>(list+row*2,0xffff),read<int16_t>(win+8,-1)};
}
void speakCommand(const CommandFocus& focus,RowAvailability availability) {
    if(focus==spokenCommand && availability==spokenAvailability) return;
    if(availability==RowAvailability::Hidden) {
        resetSelectionDetail();spokenCommand=focus;spokenAvailability=availability;return;
    }
    const auto win=focus.window;
    const auto id=focus.id;
    const auto type=focus.type;
    TextPlan p;prepareTextPlan(p,"battle-command");TextRef detail=0;
    if(type==7 || type==15) {
        using Gear=uintptr_t(__cdecl*)(unsigned,uintptr_t*);
        detail=planEquipmentDetails(p,reinterpret_cast<Gear>(address(0x3abbf0))(id,nullptr));
    } else if(type==10) detail=planBattleSelectionVitals(p,id);
    else if(type==5) detail=summonDetail(p,id);
    else {
        const auto description=planRecordDescription(p,0x39a4c0,id);
        detail=planPair(p,TextPlanOp::Or,description,planEncoded(p,address(0xf3f4f8),128,TextPlanOp::Lines));
    }
    const auto name=commandName(p,win,id,type);
    p.headline=type==10?planSuffix(p,name,detail,L", "):name;
    if(availability==RowAvailability::Unavailable)
        p.headline=planSuffix(p,p.headline,planLiteral(p,menuAvailabilityText(nativeMenuLanguage()).unavailable),L", ");
    planSelectionDetail(p,detail,type!=10 && (setting(Setting::CombatDetail)!=0 || setting(Setting::AutoDetail)!=0));
    if(commitTextPlan(p)) {spokenCommand=focus;spokenAvailability=availability;}
}
uintptr_t availabilityReturn(const CommandFocus& focus) {
    // 8A0D70 installs these native wait renderers. Equipment, Change, pay
    // and character rows use their established speech paths.
    unsigned reference=0,installed=0;
    switch(focus.type) {
    case 0: reference=0x4a4950;installed=0x4a49b0;break;
    case 1: case 2: case 3: case 4: case 6: case 0xe: case 0x11:
        reference=0x4a74d0;installed=0x4a7520;break;
    case 5:
        if(focus.id!=0x30ff && focus.id!=0xff) return 0;
        reference=0x4a75a0;installed=0x4a75f0;break;
    case 0xc: reference=0x4a3620;installed=0x4a3680;break;
    case 0xd: reference=0x4a6a10;installed=0x4a6a60;break;
    case 0x14: reference=0x4a6810;installed=0x4a6870;break;
    default: return 0;
    }
    const auto caller=focus.type==0?commandDrawReturn:listDrawReturn;
    if(!caller || focus.actor<0 || focus.actor>=18) return 0;
    const auto renderer=address(0)+(nativeProfile==NativeProfile::Installed?installed:reference);
    return read<uintptr_t>(focus.window+0x90)==renderer?caller:0;
}
void highlighted(uintptr_t win) {
    if(!inBattle()) return;
    const auto focus=commandFocus(win);
    if(!focus.window) return;
    const auto caller=availabilityReturn(focus);
    if(caller && win==currentWindow()) {
        if(!(focus==renderedCommand)) {
            renderedCommand=focus;
            resetSelectionDetail();
        }
        expectedDrawReturn=caller;
        if(focus.id==0xff) speakCommand(focus,RowAvailability::Hidden);
    } else if(!caller) {
        if(win==currentWindow()) {renderedCommand={};expectedDrawReturn=0;}
        speakCommand(focus,RowAvailability::Usable);
    }
}
void commandAvailability(Event& e) {
    // At the two checked draw calls EBX is the window, ECX is the native
    // row index, and the original C call takes actor and record as arguments.
    // Compare copied values before touching game memory. Other rows, game
    // logic calls, and identical results require no native reads here.
    const auto& focus=renderedCommand;
    if(!expectedDrawReturn || e.caller!=expectedDrawReturn || e.ebx!=focus.window ||
        e.epoch!=focus.epoch || e.ecx!=static_cast<unsigned>(focus.row) ||
        e.args[0]!=static_cast<unsigned>(focus.actor) || static_cast<int32_t>(e.args[1])!=static_cast<int16_t>(focus.id)) return;
    const auto result=static_cast<int32_t>(e.result);
    const auto availability=result==-4?RowAvailability::Hidden:result<0?RowAvailability::Unavailable:RowAvailability::Usable;
    if(focus==spokenCommand && availability==spokenAvailability) return;
    if(scene || !inBattle() || e.epoch!=generation() || focus.window!=currentWindow() || !(focus==commandFocus(focus.window))) return;
    speakCommand(focus,availability);
}
uintptr_t relativeBattleCall(uintptr_t site) {
    std::array<uint8_t,5> bytes{};
    if(!copy(site,bytes.data(),bytes.size()) || bytes[0]!=0xe8) return 0;
    int32_t displacement=0;
    std::memcpy(&displacement,bytes.data()+1,sizeof(displacement));
    return site+bytes.size()+static_cast<uintptr_t>(displacement);
}
uintptr_t drawAvailabilityReturn(uintptr_t call,uintptr_t predicate,bool main) {
    // Match the indexed record load, actor from the same EBX window, and the
    // renderer's hidden-row decision. Relative branch offsets can relocate.
    if(read<uint32_t>(call-23)!=0x0f20438b || read<uint32_t>(call-19)!=0x664804b7 ||
        read<uint16_t>(call-13)!=0x840f || read<uint16_t>(call-7)!=0x5098 ||
        read<uint32_t>(call-5)!=0x0843bf0f || read<uint8_t>(call-1)!=0x50 ||
        relativeBattleCall(call)!=predicate) return 0;
    const auto ret=call+5;
    if(read<uint32_t>(ret+(main?5:6))!=(main?0x0ffcfe83u:0x0ffcf883u)) return 0;
    return ret;
}
void installCommandAvailability() {
    const auto base=address(0);
    const bool installed=nativeProfile==NativeProfile::Installed;
    const auto predicate=base+(installed?0x49aca0:0x49ac10);
    if(relativeBattleCall(address(0x49c870)+0x37e)!=predicate || read<uint8_t>(predicate)!=0x55 ||
        read<uint16_t>(predicate+1)!=0xec8b) {
        recordLog("battle row availability predicate address not verified");
        return;
    }
    const auto main=base+(installed?0x4a49b0:0x4a4950);
    const auto mix=base+(installed?0x4a6870:0x4a6810);
    const auto list=relativeBattleCall(mix+0x2e);
    commandDrawReturn=drawAvailabilityReturn(main+0x1d58,predicate,true);
    if(list==base+(installed?0x49f370:0x49f300))
        listDrawReturn=drawAvailabilityReturn(list+0xc08,predicate,false);
    if(commandDrawReturn || listDrawReturn) hookAddress(predicate,0x49ac10,nullptr,commandAvailability);
    if(!commandDrawReturn || !listDrawReturn) recordLog("battle row availability draw edge missing main=%u list=%u",
        commandDrawReturn?1u:0u,listDrawReturn?1u:0u);
}
void cursor(Event& e) {
    if(previousTargetWindow) {resetTargetPreview();resetBattleSensor();}
    highlighted(e.args[0]);
}
void battleMenuRejected(Event& e) {
    // These controller bodies use sound 3 for refused inputs. The same sound
    // from 892290 belongs to Tidus's Overdrive timing and has its own cue.
    switch(e.parentRva) {
    case 0x49c870: case 0x49cd50: case 0x49d580: case 0x4a2290: case 0x4a2630:
        if(e.args[0]==3 && !scene && inBattle() && read<int>(address(0xf3d6a0))!=1)
            queueMenuRejectionCue();
        break;
    }
}
void installBattleMenuRejection() {
    // Verify the direct PUSH 3 / CALL edge within the profile-matched command
    // handler before hooking the sound wrapper. A mismatch skips this cue.
    const auto push=address(0x49c870)+0xeb;
    std::array<uint8_t,7> bytes{};
    if(copy(push,bytes.data(),bytes.size()) && bytes[0]==0x6a && bytes[1]==3 && bytes[2]==0xe8) {
        int32_t displacement=0;
        std::memcpy(&displacement,bytes.data()+3,sizeof(displacement));
        const auto target=push+bytes.size()+static_cast<uintptr_t>(displacement);
        const auto expected=address(0)+(nativeProfile==NativeProfile::Installed?0x486ba0:0x486b00);
        if(target==expected && read<uint8_t>(target)==0x55 && read<uint16_t>(target+1)==0xec8b) {
            hookAddress(target,0x486b00,battleMenuRejected,nullptr);
            return;
        }
    }
    recordLog("battle menu rejection cue address not verified");
}
void pop(Event&) {amountFocus={};resetTargetPreview();resetBattleSensor();resetCommandFocus();highlighted(currentWindow());}
void opened(Event&) {
    amountFocus={};
    resetTargetPreview();
    resetBattleSensor();
    resetCommandFocus();
    if (!inBattle()) return;
    announceBattleFormation();
    const auto win = currentWindow();
    if (!win) return;
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
// Copies the target's status words (actor+606/+608..+614/+616, KO+DCC and
// Doom's counter+5C8) as one node; names are chosen on the worker.
TextRef targetAilments(TextPlan& p,uintptr_t chr) {
    uint16_t permanent=read<uint16_t>(chr+0x606),temporal=0;
    if(read<uint8_t>(chr+0xdcc)) permanent|=1;
    for(unsigned i=0;i<13;++i) if(read<uint8_t>(chr+0x608+i)) temporal|=static_cast<uint16_t>(1u<<i);
    return planBattleAilments(p,permanent,temporal,read<uint16_t>(chr+0x616),read<uint8_t>(chr+0x5c8));
}
TextRef withAilments(TextPlan& p,TextRef target,uintptr_t chr) {
    const auto ailments=targetAilments(p,chr);
    return ailments?planJoin(p,target,ailments):target;
}
void targets(Event& e) {
    if (!nativeGameThread() || e.epoch!=generation() || !inBattle()) return;
    const auto win = e.args[0];
    // The native handler can pop the window before its return. The retired
    // window's command and target masks no longer own a preview.
    if(!win || win!=currentWindow()) return;
    if(renderedCommand.window) resetCommandFocus();
    const unsigned players = read<uint16_t>(win + 0xd8), monsters = read<uint16_t>(win + 0xda);
    if (!players && !monsters) {resetTargetPreview();resetBattleSensor();return;}
    const auto owner=read<uint8_t>(win+8);
    // Native 89A2A0 stores the accepted command at +0C when it creates the
    // target window. Cursor rows in its parent window have unrelated IDs.
    const auto command=read<uint16_t>(win+0xc,0xffff);
    const bool newPreview=previousTargetWindow!=win || previousTargetOwner!=owner || previousTargetCommand!=command;
    const auto selected=read<uint16_t>(address(0xf3d1b4),255);
    const auto wheels=seymourWheels(monsters);
    const bool sensor=read<uint8_t>(win+2)==1 && read<int>(address(0xf3d6a0))!=1 && battleSensorVisible(selected);
    highlightBattleSensor(win);
    if(!newPreview && players==previousPlayers && monsters==previousMonsters && previousSensor==sensor && previousSeymour==wheels.key) return;
    if(newPreview) {
        previewMp=false;
        if(hasBattleCommandRecord(command)) {
            using Getter = uintptr_t(__cdecl*)(unsigned, uintptr_t*);
            uintptr_t base = 0;
            const auto record=reinterpret_cast<Getter>(address(0x39a4c0))(command,&base);
            if(record && base) previewMp=(read<uint8_t>(record+0x22)&2)!=0;
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
        auto target=withAilments(p,planBattleTarget(p,name,hp,maximum,kind),chr);
        if(selected==bit+16) {
            const auto mechanic=wheelMechanic(p,wheels,selected);
            if(mechanic)target=planSuffix(p,target,mechanic,L", ");
        }
        names=planJoin(p,names,target);
    }
    const auto array=read<uintptr_t>(address(0xd334cc));
    for(unsigned bit=0;bit<18;++bit) if((players&(1u<<bit)) && array) {
        const auto chr=array+bit*0xf90;if(!read<uint8_t>(chr+0xdc8)) continue;
        names=planJoin(p,names,withAilments(p,planBattleTarget(p,planBattleName(p,bit),
            read<int>(chr+(previewMp?0x5d4:0x5d0)),read<int>(chr+(previewMp?0x598:0x594)),previewMp?2:1),chr));
    }
    p.headline=names;commitTextPlan(p); previousPlayers = players; previousMonsters = monsters;
    previousTargetWindow=win;previousTargetOwner=owner;previousTargetCommand=command;previousSensor=sensor;previousSeymour=wheels.key;
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
    resetCommandFocus();
    resetTargetPreview();resetWakka();resetOverdrive();resetBattleSensor();resetBattleStatus();resetBattleResults();resetBattleAnnouncements();
}
void installBattle() {
    installBattleHud();
    installBattleGraphics();
    hook(0x39fa20, nullptr, damage); hook(0x498bd0, actionBefore, action);
    hook(0x49db10,message,nullptr); hook(0x39cff0,nullptr,special);
    installBattleResults(); installBattleAnnouncements();
    hook(0x49a5a0, nullptr, pop); hook(0x4a0d70, nullptr, opened);
    for (auto rva : {0x49c870,0x49cd50,0x49d580,0x4a2100,0x4a2290,0x4a2630}) hook(rva, nullptr, cursor);
    installBattleMenuRejection();
    installCommandAvailability();
    hook(0x49d300, nullptr, targets);
    hook(0x4a3250,nullptr,amountDrawn);
    hotkey('T', whoseTurn); hotkey('H', activeStats);
    hotkeyChordWhen('O',false,overdriveMeter,[] { return inBattle(); });
    installBattleStatus(); installBattleSensor(); installOverdrive();
}
}
