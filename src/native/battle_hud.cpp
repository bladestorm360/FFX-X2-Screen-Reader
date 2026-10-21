// Original HUD draw -> accepted clipped identifying quad -> owner POD frame.
// No internal forecast enumeration, text processing, sorting or worker pointers.
#include "battle_hud.hpp"
#include "events.hpp"
#include "ready_game_text.hpp"
#include <algorithm>
#include <cmath>
#include <cstring>
namespace ffx {
namespace {
struct Turn {unsigned sprite=0;float y=0;};
struct Meter {unsigned actor=255,percent=0;};
struct Panel {unsigned actor=255,mask=0;std::array<int,4> values{};}; // HP, max HP, MP, max MP.
struct Frame {
    unsigned epoch=0,count=0,meters=0;
    std::array<Turn,32> turns{};std::array<Meter,3> gauges{};
    std::array<Panel,3> panels{};
} pending,displayed;
bool drawing=false,status=false,row=false,icon=false,meter=false,accepted=false,quadEligible=false;
unsigned sprite=0,quad=0,slot=0,meterActor=255,meterPercent=0;
float rowY=0;
std::array<unsigned,3> statusActors{};unsigned statusCount=0;
unsigned panelCount=0,panelIndex=3,numberOrdinal=0,statusMode=255;
struct Number {bool active=false;unsigned panel=3,field=0;int value=0;} number;
uintptr_t numberClipReturn=0;
float scalar(uintptr_t bits) {const auto word=static_cast<uint32_t>(bits);float f;std::memcpy(&f,&word,4);return f;}
bool ready() {return drawing && pending.epoch==generation() && inBattle();}
void parentCall(Event&) {} // Establish the verified nearest hooked parent.
bool rectangle(const Event& e) {
    const float x0=read<float>(e.args[0],-100000),y0=read<float>(e.args[1],-100000),
        x1=read<float>(e.args[2],-100000),y1=read<float>(e.args[3],-100000);
    return std::isfinite(x0) && std::isfinite(y0) && std::isfinite(x1) && std::isfinite(y1) &&
        x1>x0 && y1>y0 && x1>0 && y1>0 && x0<512 && y0<416;
}
void frameBefore(Event&) {
    pending={};pending.epoch=generation();drawing=inBattle();status=row=icon=meter=false;number={};
}
void frameAfter(Event&) {
    if(ready())displayed=pending;else displayed={};
    drawing=status=row=icon=meter=false;
}
void rowBefore(Event& e) {
    row=ready() && e.parentRva==0x4a0520;accepted=false;
    rowY=scalar(e.args[3]);if(!std::isfinite(rowY) || std::abs(rowY)>1000000)row=false;
}
void rowAfter(Event&) {row=icon=false;}
void iconBefore(Event& e) {
    icon=ready() && row && e.parentRva==0x4a1660;quad=0;sprite=static_cast<unsigned>(e.args[0]);
    if(sprite<0x1b8 || sprite>0x1d4)icon=false;
}
void iconAfter(Event&) {icon=false;}
void texturedBefore(Event& e) {
    quadEligible=ready() && icon && e.parentRva==0x49e5d0 && e.args[0]==400;
    // Lettered enemies draw an anonymous backdrop first. Only the identifying
    // letter quad qualifies them; clipping the letter never reveals its label.
    if(quadEligible) {++quad;quadEligible=(sprite>=0x1c3 && sprite<=0x1d2)?quad==2:quad==1;}
}
void texturedAfter(Event&) {quadEligible=false;}
void clippedTexture(Event& e) {
    if(!ready() || !e.result)return;
    if(number.active && e.parentRva==0x505890 && e.caller==numberClipReturn && rectangle(e)) {
        auto& panel=pending.panels[number.panel];panel.values[number.field]=number.value;panel.mask|=1u<<number.field;
    }
    if(!icon || !quadEligible || e.parentRva!=0x503bb0 || !rectangle(e))return;
    // The caller's qualified textured quad is live on this native call stack.
    if(!accepted && ((sprite>=0x1c3 && sprite<=0x1d2)?quad==2:quad==1) && pending.count<pending.turns.size()) {
        pending.turns[pending.count++]={sprite,rowY};accepted=true;
    }
}
void statusBefore(Event& e) {
    status=ready() && e.parentRva==0x49b2d0;statusCount=slot=panelCount=0;panelIndex=3;number={};
    if(!status)return;
    // This is the native three-row table used by this exact draw, including
    // empty rows. No roster/party scan is used to manufacture visible meters.
    for(unsigned i=0;i<3;++i) {const auto id=read<uint16_t>(address(0xf3f76c)+i*2,255);if(id!=255)statusActors[statusCount++]=id;}
    statusMode=read<uint8_t>(address(0xf3d742),255);
}
void statusAfter(Event&) {status=meter=false;number={};}
void panelName(Event& e) {
    if(!ready() || !status || e.parentRva!=0x4963c0)return;
    if(panelCount>=3) {panelIndex=3;return;}
    panelIndex=panelCount++;numberOrdinal=0;
    if(panelIndex>=statusCount || e.args[0]!=statusActors[panelIndex] || e.args[0]>=18) {panelIndex=3;return;}
    pending.panels[panelIndex].actor=static_cast<unsigned>(e.args[0]);
}
void numberBefore(Event& e) {
    number={};
    if(!ready() || !status || e.parentRva!=0x4963c0 || panelIndex>=3 || numberOrdinal>=2)return;
    const auto ordinal=numberOrdinal++;
    if(statusMode!=0 && statusMode!=1 && statusMode!=3)return;
    const auto value=static_cast<int>(e.args[0]);if(value<0)return;
    const auto field=statusMode==3?ordinal*2:statusMode==1?ordinal+2:ordinal;
    number={true,panelIndex,field,value};
}
void numberAfter(Event&) {number={};}
void meterBefore(Event& e) {
    meter=ready() && status && e.parentRva==0x4963c0 && slot<statusCount;
    accepted=false;if(!meter)return;
    meterActor=statusActors[slot++];const int maximum=read<int16_t>(address(0x21d0aa4));
    const auto fill=static_cast<int>(e.args[0]);
    if(meterActor>=18 || maximum<=0) {meter=false;return;}
    meterPercent=static_cast<unsigned>(std::clamp<int64_t>((int64_t{fill}*100+maximum/2)/maximum,0,100));
}
void clippedPlain(Event& e) {
    if(!ready() || !meter || e.parentRva!=0x4f4b20 || !e.result || !rectangle(e) || accepted || pending.meters>=pending.gauges.size())return;
    pending.gauges[pending.meters++]={meterActor,meterPercent};accepted=true;
}
void meterAfter(Event&) {meter=false;}
TextRef iconName(TextPlan& p,unsigned value) {
    if(value>=0x1b8 && value<=0x1bf)return planBattleName(p,value-0x1b8);
    if(value==0x1c0)return planLiteral(p,L"Aeon");
    if(value>=0x1c3 && value<=0x1d2) {
        static constexpr const wchar_t* enemies[]={L"Enemy A",L"Enemy B",L"Enemy C",L"Enemy D",L"Enemy E",L"Enemy F",L"Enemy G",L"Enemy H"};
        return planLiteral(p,enemies[(value-(value>=0x1cb?0x1cb:0x1c3))&7]);
    }
    return planLiteral(p,value==0x1d4?L"Unknown":L"Enemy");
}
void turnOrder(bool) {
    if(!inBattle())return;
    TextPlan p;prepareTextPlan(p,"battle-turn-order");TextRef rows=0;
    if(displayed.epoch==generation()) {
        std::array<TextRef,29> names{};
        for(unsigned i=0;i<displayed.count;++i) {
            const auto& t=displayed.turns[i];const auto index=t.sprite-0x1b8;
            if(index>=names.size())continue;
            if(!names[index])names[index]=iconName(p,t.sprite);
            rows=planOrderedRow(p,rows,names[index],static_cast<int64_t>(t.y*1024));
        }
    }
    p.headline=rows?planConcat(p,planLiteral(p,L"Turn order: "),rows):planLiteral(p,L"No turn order displayed");commitTextPlan(p);
}
void allMeters(bool) {
    if(!inBattle())return;
    TextPlan p;prepareTextPlan(p,"battle-visible-overdrive");TextRef rows=0;
    if(displayed.epoch==generation())for(unsigned i=0;i<displayed.meters;++i) {
        const auto& g=displayed.gauges[i];
        const auto line=planConcat(p,planConcat(p,planBattleName(p,g.actor),planLiteral(p,L" Overdrive ")),
            planConcat(p,planInteger(p,g.percent),planLiteral(p,L"%")));
        rows=planJoin(p,rows,line);
    }
    p.headline=rows?rows:planLiteral(p,L"No Overdrive meters displayed");commitTextPlan(p);
}
TextRef panelVital(TextPlan& p,const Panel& panel,unsigned field) {
    const bool current=(panel.mask&(1u<<field))!=0,maximum=(panel.mask&(2u<<field))!=0;
    if(!current && !maximum)return 0;
    const auto label=field==0?(current?L"HP ":L"Maximum HP "):(current?L"MP ":L"Maximum MP ");
    auto text=planConcat(p,planLiteral(p,label),planInteger(p,panel.values[field+!current]));
    if(current && maximum)text=planConcat(p,text,planConcat(p,planLiteral(p,L"/"),planInteger(p,panel.values[field+1])));
    return text;
}
void allVitals(bool) {
    if(!inBattle())return;
    TextPlan p;prepareTextPlan(p,"battle-visible-vitals");TextRef rows=0;
    if(displayed.epoch==generation())for(const auto& panel:displayed.panels) {
        if(panel.actor>=18 || !panel.mask)continue;
        const auto values=planPair(p,TextPlanOp::SpaceJoin,panelVital(p,panel,0),panelVital(p,panel,2));
        rows=planJoin(p,rows,planConcat(p,planConcat(p,planBattleName(p,panel.actor),planLiteral(p,L" ")),values));
    }
    p.headline=rows?rows:planLiteral(p,L"No HP or MP panels displayed");commitTextPlan(p);
}
}
bool visibleOverdrive(unsigned id,unsigned& percent) {
    if(displayed.epoch!=generation() || !inBattle())return false;
    for(unsigned i=0;i<displayed.meters;++i)if(displayed.gauges[i].actor==id){percent=displayed.gauges[i].percent;return true;}
    return false;
}
void resetBattleHud() {pending={};displayed={};drawing=status=row=icon=meter=false;number={};}
void installBattleHud() {
    // Original 8FB940's accepted foreground glyph clip returns at +154.
    // Shadows and unrendered number preparation do not qualify a HUD value.
    numberClipReturn=address(0x4fb940)+0x154;
    hook(0x4a0520,parentCall,nullptr);hook(0x4f4b20,parentCall,nullptr);
    hook(0x49b2d0,frameBefore,frameAfter);hook(0x4a1660,rowBefore,rowAfter);hook(0x49e5d0,iconBefore,iconAfter);
    hook(0x503bb0,texturedBefore,texturedAfter);hook(0x4e5a20,nullptr,clippedTexture);
    hook(0x4963c0,statusBefore,statusAfter);hook(0x496050,meterBefore,meterAfter);hook(0x4e58e0,nullptr,clippedPlain);
    hook(0x39af20,panelName,nullptr);hook(0x505890,numberBefore,numberAfter);
    hotkeyChordWhen('T',true,turnOrder,[]{return inBattle();});
    hotkeyChordWhen('O',true,allMeters,[]{return inBattle();});
    hotkeyChordWhen('H',true,allVitals,[]{return inBattle();});
}
}
