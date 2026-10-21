#include "status_review.hpp"
#include "sphere_grid.hpp"
#include "ready_game_text.hpp"
namespace ffx {
namespace {
int activeSlot=0,activeCharacter=-1;
unsigned activeEpoch=0;
int slot(uintptr_t window) {
    const auto pool=read<uintptr_t>(address(0x1686108));
    if(!pool || window<pool || (window-pool)%0x340) return 0;
    const auto index=(window-pool)/0x340;
    return index>=1 && index<=5?static_cast<int>(index):0;
}
void page(unsigned index) {
    const auto engine=read<uintptr_t>(address(0x1f05834)),pool=read<uintptr_t>(address(0x1686108));
    if(!sphereGridOpen() || !engine || !pool || index<1 || index>5) return;
    const auto id=read<uint8_t>(engine+0x115bc,255);
    using Saved=uintptr_t(__cdecl*)(unsigned);
    const auto saved=id<7?reinterpret_cast<Saved>(address(0x3853f0))(id):0;
    const auto window=pool+index*0x340;
    const int count=read<int16_t>(window+0x1e,-1);
    if(!saved || count<0 || count>64) return;
    // Slot6 is the Display Status button hint, not a page heading. These
    // descriptions identify the displayed lists built by654B40/6459E0:
    // stats, command category41,40,45,44 for the currently selected character.
    constexpr const wchar_t* headings[]={L"",L"Attributes",L"Special",L"Skills",L"White magic",L"Black magic"};
    // Validate ready stat kinds before publishing any new context. The window
    // count remains the native64-row bound; disabled rows remain hidden.
    if(index==1) for(int i=0;i<count;++i) {
        const auto entry=window+0x40+i*12;
        if(!read<uint8_t>(entry+4) && read<unsigned>(entry+8,255)>=10) return;
    }
    auto batch=beginStatusRows(id,index,true,true);
    TextPlan title;prepareTextPlan(title,"sphere-status-page",false);
    const auto headline=planConcat(title,planConcat(title,planPlayerName(title,id),planLiteral(title,L". ")),planLiteral(title,headings[index]));
    addStatusRow(batch,title,headline);
    // Constructor 654B40 supplies the actual localized labels and stat-kind
    // IDs. 64FA30 renders maximum HP/MP and these eight saved stats only.
    constexpr unsigned offsets[]={0x24,0x28,0x2f,0x33,0x30,0x34,0x31,0x35,0x32,0x36};
    for(int i=0;i<count;++i) {
        const auto entry=window+0x40+i*12;
        if(read<uint8_t>(entry+4)) continue; // native renderer hides disabled rows
        if(index==1) {
            const auto kind=read<unsigned>(entry+8,255);
            if(kind>=10) {batch.valid=false;break;}
            TextPlan p;prepareTextPlan(p,"sphere-status-row",false);
            const auto label=planEncoded(p,read<uintptr_t>(entry));
            const auto value=kind<2?read<unsigned>(saved+offsets[kind]):read<uint8_t>(saved+offsets[kind]);
            const auto text=planConcat(p,planConcat(p,label,planLiteral(p,L": ")),planInteger(p,value));
            addStatusRow(batch,p,planPair(p,TextPlanOp::IfText,label,text),0,true);
        } else {
            const auto ability=read<unsigned>(entry);
            // Both native row renderers (64F250/64F9A0) call 4F0590,
            // which resolves command records through 39A4C0.
            const auto getter=0x39a4c0;
            TextPlan p;prepareTextPlan(p,"sphere-status-row",false);
            addStatusRow(batch,p,planRecordName(p,getter,ability),0,true);
        }
    }
    // Native65A2E0 hides unlearned commands using this character's learned bits.
    // Empty lists are valid and do not mean that another party member is locked.
    if(!finishStatusRows(batch)) return;
    activeSlot=index; activeCharacter=id; activeEpoch=generation();
    rememberSphereCharacter(id);
}
void openBefore(Event& e) { e.before[0]=read<uint8_t>(e.args[0]+0x23); }
void opened(Event& e) {
    const auto index=slot(e.args[0]);
    if(index && !e.before[0] && read<uint8_t>(e.args[0]+0x23)==1) page(index);
}
void closed(Event& e) {
    const auto index=slot(e.args[0]);
    if(index && !read<uint8_t>(e.args[0]+0x23)) {
        closeGridStatus(index);
        if(activeSlot==index) activeSlot=0;
    }
}
void characterChanged(Event&) {
    if(!activeSlot || activeEpoch!=generation() || !sphereGridOpen() || !statusReviewAvailable()) return;
    const auto engine=read<uintptr_t>(address(0x1f05834)),pool=read<uintptr_t>(address(0x1686108));
    if(engine && pool && read<uint8_t>(pool+activeSlot*0x340+0x23) &&
        read<uint8_t>(engine+0x115bc,255)!=activeCharacter) page(activeSlot);
}
}
void installGridStatus() {
    // Actual window visibility mutations, not a draw loop or UI polling.
    hook(0x65a270,openBefore,opened); hook(0x659800,nullptr,closed);
    hook(0x658ec0,nullptr,characterChanged);
}
}
