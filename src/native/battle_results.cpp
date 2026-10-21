#include "events.hpp"
#include "ready_game_text.hpp"
#include "status_review.hpp"
#include "esc_details.hpp"
namespace ffx {
namespace {
bool results=false,dirty=false;
int page=-1,previousState=-1;
std::array<unsigned,8> earnedAp{},initialLevels{};
unsigned earnedGil=0;
bool levelsPresented=false;
unsigned value(unsigned getter,unsigned id) {
    return reinterpret_cast<unsigned(__cdecl*)(unsigned)>(address(getter))(id);
}
int activePage() {
    if(!results) return -1;
    // Exact 8BA620 jump table: 9..14 hex are the AP presentation,
    // 15..1A hex the gil/reward presentation. 1C..2B is the separate
    // equipment-capacity management UI and retains native controls.
    const int state=read<int>(address(0x1469ec8),-1);
    return state>=9 && state<=0x14?0:state>=0x15 && state<=0x1a?1:-1;
}
TextRef labelNumber(TextPlan& p,const wchar_t* label,unsigned number) {
    return planConcat(p,planLiteral(p,label),planInteger(p,number));
}
void apRows(StatusBatch& batch,uintptr_t reward) {
    const unsigned count=read<uint8_t>(address(0x1469ed8));
    if(count>8) batch.valid=false;
    for(unsigned i=0;i<count && i<8;++i) {
        // Native 8BDDC0/8BA3C0 order, rather than numeric character order.
        const int id=read<int16_t>(address(0x1469ee6)+i*14,-1);
        if(id<0 || id>=8) {batch.valid=false;break;}
        const auto levels=reinterpret_cast<uint8_t(__cdecl*)(unsigned)>(address(0x4a9b30))(id);
        {
            TextPlan total;prepareTextPlan(total,"battle-results",false);total.context[6]=1;
            addStatusRow(batch,total,planConcat(total,planConcat(total,planPlayerName(total,id),planLiteral(total,L". ")),
                labelNumber(total,L"AP earned: ",earnedAp[id])));
        }
        TextPlan p;prepareTextPlan(p,"battle-results",false);
        auto line=planConcat(p,planPlayerName(p,id),planLiteral(p,L". "));
        line=planConcat(p,line,labelNumber(p,L"AP: ",read<unsigned>(reward+0x80+id*4)));
        line=planConcat(p,line,labelNumber(p,L". Sphere Level: ",levels));
        line=planConcat(p,line,planLiteral(p,L". Next AP: "));
        const auto next=levels>=99?planLiteral(p,L"-----"):
            planInteger(p,static_cast<int>(value(0x384f50,id)-value(0x385370,id)));
        addStatusRow(batch,p,planConcat(p,line,next));
        if(levels>initialLevels[id]) {
            TextPlan gained;prepareTextPlan(gained,"battle-results",false);
            addStatusRow(batch,gained,planConcat(gained,planConcat(gained,planPlayerName(gained,id),planLiteral(gained,L". ")),
                labelNumber(gained,L"Sphere levels gained: ",levels-initialLevels[id])));
        }
    }
}
void numberRow(StatusBatch& batch,const wchar_t* label,unsigned number) {
    TextPlan p;prepareTextPlan(p,"battle-results",false);
    addStatusRow(batch,p,labelNumber(p,label,number));
}
void rewardRows(StatusBatch& batch,uintptr_t reward) {
    {
        TextPlan total;prepareTextPlan(total,"battle-results",false);total.context[6]=1;
        addStatusRow(batch,total,labelNumber(total,L"Gil earned: ",earnedGil));
    }
    numberRow(batch,L"Gil: ",read<unsigned>(address(0x1469ed4)));
    const auto wallet=reinterpret_cast<unsigned(__cdecl*)()>(address(0x384f40))();
    numberRow(batch,L"Total Gil: ",wallet);
    const unsigned items=read<uint8_t>(reward+0xd0),keys=read<uint8_t>(reward+0xd1),gear=read<uint8_t>(reward+0xd2);
    if(items>8 || keys>1 || gear>8) {batch.valid=false;return;}
    // One bounded plan at a time, even in the unoptimized offline host. Keep
    // large copied-text scratch objects out of the same native stack frame.
    for(unsigned i=0;i<items+keys+gear;++i) {
        TextPlan p;prepareTextPlan(p,"battle-results",false);TextRef line=0;
        if(i<items) {
            line=planConcat(p,planRecordName(p,0x390a40,read<uint16_t>(reward+0xd4+i*2)),planLiteral(p,L" x"));
            line=planConcat(p,line,planInteger(p,read<uint8_t>(reward+0xe4+i)));
        } else if(i<items+keys) line=planRecordName(p,0x3908f0,read<uint16_t>(reward+0xec));
        else line=planEquipmentName(p,reward+0xfe+(i-items-keys)*22);
        addStatusRow(batch,p,line);
    }
}
void assemble(bool present,bool replace) {
    if(page<0 || activePage()!=page) return;
    auto batch=beginStatusRows(0,page,false,replace,replace);
    batch.battle=true;batch.present=present;
    {
        TextPlan heading;prepareTextPlan(heading,"battle-results",false);
        addStatusRow(batch,heading,planLiteral(heading,page?L"Gil and items":L"AP"));
    }
    const auto reward=address(0x1f10ea0);
    if(page) rewardRows(batch,reward);else apRows(batch,reward);
    if(finishStatusRows(batch)) dirty=false;
}
void opening(Event& e) {
    closeBattleReview();page=previousState=-1;dirty=false;
    // Variant 1 opens capacity management, not a new reward episode.
    results=e.args[1]!=1;
    levelsPresented=false;earnedAp={};initialLevels={};earnedGil=0;
    if(!results) return;
    // 8BDB30 has initialized both counters, before any 8BA620 award animation.
    earnedGil=read<unsigned>(address(0x1469ed4));
    for(unsigned id=0;id<8;++id) {
        earnedAp[id]=read<unsigned>(address(0x1f10ea0)+0x80+id*4);
        initialLevels[id]=reinterpret_cast<uint8_t(__cdecl*)(unsigned)>(address(0x4a9b30))(id);
    }
}
void gainedLevels() {
    if(levelsPresented) return;
    levelsPresented=true;
    const unsigned count=read<uint8_t>(address(0x1469ed8));
    if(count>8) return;
    for(unsigned i=0;i<count;++i) {
        const int id=read<int16_t>(address(0x1469ee6)+i*14,-1);
        if(id<0 || id>=8) return;
        const unsigned levels=reinterpret_cast<uint8_t(__cdecl*)(unsigned)>(address(0x4a9b30))(id);
        if(levels<=initialLevels[id]) continue;
        TextPlan p;prepareTextPlan(p,"battle-results-levels",false);
        p.headline=planConcat(p,planConcat(p,planPlayerName(p,id),planLiteral(p,L". ")),
            labelNumber(p,L"Sphere levels gained: ",levels-initialLevels[id]));
        commitTextPlan(p);
    }
}
void advanced(Event&) {
    if(escMenuActive()) return;
    const int state=read<int>(address(0x1469ec8),-1),next=activePage();
    // The outer callback includes Confirm's complete fast-forward loop. Announce
    // each participant's final gain once, never one utterance per rolling tick.
    if(results && ((next==0 && state>=0xd) || (page==0 && next==1))) gainedLevels();
    if(next<0) {if(page>=0) closeBattleReview();page=-1;previousState=state;return;}
    dirty=true;
    if(next!=page) {page=next;assemble(true,true);}
    // Both native Confirm fast-forward loops finish inside this outer call.
    // Quietly refresh once on completion, not on each inner AP/gil mutation.
    else if((state==0xd || state==0x19 || state==0x1a) && state!=previousState) assemble(false,false);
    previousState=state;
}
}
bool battleResultsReviewActive() {return page>=0 && activePage()==page;}
void refreshBattleResultReview() {
    // Review input runs on the same serialized native owner as 8BA620.
    // Refresh on demand during rolling animation; never enqueue a page per frame.
    if(dirty && battleResultsReviewActive()) assemble(false,false);
}
void resetBattleResults() {results=dirty=false;page=previousState=-1;closeBattleReview();}
void installBattleResults() {hook(0x4bdb30,nullptr,opening);hook(0x4ba620,nullptr,advanced);}
}
