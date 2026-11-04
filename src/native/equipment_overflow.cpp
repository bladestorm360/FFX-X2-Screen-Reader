// Battle reward equipment capacity management: native keep/discard focus,
// replacement selection, visible instructions and selected modifiers. Reuse
// the existing results controller return, ready TextPlans and I/Autodetail.
// Native pointers stay on that owner; preparation receives copied bytes only.
// Native layouts/phase evidence: Docs/features/menus/equipment_overflow_20261103.md.
#include "equipment_overflow.hpp"
#include "events.hpp"
#include "ready_game_text.hpp"
#include "message_text.hpp"
#include "settings.hpp"

namespace ffx {
namespace {
using Key=std::array<uintptr_t,32>;
Key previous{};
SelectionDetailToken detail;
unsigned epoch=0;
bool armed=false,visible=false;

// The two headings are native image labels, not text records. Match all eight
// supported UI locales (0 JP,1 EN,2 FR,3 ES,4 DE,5 IT,9 ZH,10 KO). Native
// prompts/equipment still use their own localized getters and text decoder.
const wchar_t* label(unsigned which) {
    using Locale=unsigned(__cdecl*)();
    const auto language=reinterpret_cast<Locale>(address(0x241290))();
    static constexpr const wchar_t* labels[][4]={
        {L"\u6240\u6301\u30ea\u30b9\u30c8",L"\u7834\u68c4\u30ea\u30b9\u30c8",L"\u7a7a\u304d",L"\u88c5\u5099\u4e2d"},
        {L"Keep list",L"Discard list",L"Empty",L"Equipped"},
        {L"Liste \u00e0 garder",L"Liste \u00e0 jeter",L"Vide",L"\u00c9quip\u00e9"},
        {L"Lista para conservar",L"Lista para descartar",L"Vac\u00edo",L"Equipado"},
        {L"Behalten",L"Wegwerfen",L"Leer",L"Ausger\u00fcstet"},
        {L"Lista da conservare",L"Lista da scartare",L"Vuoto",L"Equipaggiato"},
        {L"\u4fdd\u7559\u5217\u8868",L"\u4e22\u5f03\u5217\u8868",L"\u7a7a",L"\u5df2\u88c5\u5907"},
        {L"\ubcf4\uad00 \ubaa9\ub85d",L"\ubc84\ub9b4 \ubaa9\ub85d",L"\ube44\uc5b4 \uc788\uc74c",L"\uc7a5\ucc29 \uc911"}
    };
    const auto row=language<=5?language:language==9?6u:language==10?7u:1u;
    return labels[row][which];
}
void retire() {
    const auto current=currentSelectionDetailToken();
    if(detail.serial && current.epoch==detail.epoch && current.serial==detail.serial) resetSelectionDetail();
    detail={};previous={};visible=false;
}
}
void beginEquipmentOverflow() {retire();armed=true;epoch=generation();}
void resetEquipmentOverflow() {retire();armed=false;epoch=0;}
void observeEquipmentOverflow() {
    if(!nativeGameThread() || !armed) return;
    if(epoch!=generation()) {resetEquipmentOverflow();return;}
    // Native 8BA620: 20/21 browse/scroll,25/26 choose the opposite-side swap
    // target,23/29 show the native discard warning. Other phases animate entry
    // or leave the UI. No passive memory watcher or new physical hook is used.
    const int state=read<int>(address(0x1469ec8),-1);
    if(state<0x1c || state>=0x2a) {if(visible) retire();return;}
    if(state!=0x20 && state!=0x21 && state!=0x25 && state!=0x26 && state!=0x23 && state!=0x29) return;
    const auto fields=address(0x146a148),table=read<uintptr_t>(address(0x21cc580));
    const int count=read<int>(fields,-1),top=read<int>(fields+8,-1),side=read<int>(fields+0x14,-1);
    const int left=read<int>(fields+0x18,-1),right=read<int>(fields+0x1c,-1),picking=read<int>(fields+0x20,-1);
    if(!table || count<1 || count>200 || top<0 || top>=count || side<0 || side>1 ||
        left<0 || left>=6 || right<0 || right>=6 || picking<0 || picking>1) {retire();return;}
    const unsigned selectedSide=static_cast<unsigned>(side)^static_cast<unsigned>(picking);
    const unsigned row=selectedSide?static_cast<unsigned>(right):static_cast<unsigned>(top+left);
    if(!selectedSide && row>=static_cast<unsigned>(count)) {retire();return;}
    const auto handle=read<uint16_t>(selectedSide?address(0x1f10f8e)+row*2:table+row*2,0xffff);
    const auto kind=handle>>12,handleIndex=handle&0xfff;
    // Native 7ABBF0 clamps malformed IDs to record0. Refuse those aliases
    // instead of announcing unrelated gear; FF is an actual empty sentinel.
    if(handle!=255 && !((kind==5 && handleIndex<200) || ((kind==7 || kind==11) && handleIndex<8))) {retire();return;}
    using Gear=uintptr_t(__cdecl*)(unsigned,uintptr_t*);
    const auto record=handle==255?0:reinterpret_cast<Gear>(address(0x3abbf0))(handle,nullptr);
    if(handle!=255 && !record) {retire();return;}
    const auto prompts=address(0x21cc590);
    Key key{};key[0]=1;key[1]=selectedSide;key[2]=row;key[3]=handle;key[4]=picking;
    key[5]=read<uintptr_t>(prompts-8);key[6]=read<uintptr_t>(prompts);
    key[7]=state==0x23 || state==0x29;key[8]=table;key[9]=count;
    // Include ready record identity/ownership/modifiers: a native swap can keep
    // both the row number and handle while changing the selected equipment.
    uint8_t bytes[22]{};if(record && !copy(record,bytes,sizeof(bytes))) {retire();return;}
    for(unsigned i=0;i<sizeof(bytes);++i) key[10+i]=bytes[i];
    const auto current=currentSelectionDetailToken();
    if(visible && key==previous && current.epoch==detail.epoch && current.serial==detail.serial) return;
    TextPlan p;prepareTextPlan(p,"equipment-overflow");
    const bool promptChanged=!visible || key[5]!=previous[5] || key[6]!=previous[6] || key[7]!=previous[7];
    TextRef headline=0;
    // Native 8BC0C0 measures/draws these messages with window7, whose current
    // substitution slots name equipment in discard warnings. Copy the same
    // resolved message operands; decoding raw bytes would omit that name.
    if(promptChanged) headline=planJoin(p,planMessage(p,7,key[5]),planMessage(p,7,key[6]));
    headline=planJoin(p,headline,planLiteral(p,label(selectedSide)));
    const auto name=bytes[2]?planEquipmentName(p,record):planLiteral(p,label(2));
    headline=planJoin(p,headline,name);
    if(bytes[2] && bytes[6]!=255) headline=planJoin(p,headline,planLiteral(p,label(3)));
    if(setting(Setting::MenuIndex)) {
        auto index=planConcat(p,planInteger(p,row+1),planLiteral(p,L"/"));
        headline=planJoin(p,headline,planConcat(p,index,planInteger(p,selectedSide?6:count)));
    }
    p.headline=headline;
    planSelectionDetail(p,bytes[2]?planEquipmentDetails(p,record):0,setting(Setting::AutoDetail)!=0);
    detail=p.detail;
    if(commitTextPlan(p)) {previous=key;visible=true;}
    else retire(); // Refused capture cannot leave I pointing at the old gear.
}
}
