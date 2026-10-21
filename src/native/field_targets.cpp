#include "events.hpp"
#include "settings.hpp"
#include "ready_game_text.hpp"
#include <algorithm>
namespace ffx {
namespace {
using Key=std::array<intptr_t,32>;
using Value=unsigned(__cdecl*)(unsigned);
unsigned value(unsigned getter,unsigned id) { return reinterpret_cast<Value>(address(getter))(id); }
Key targetKey(uintptr_t window) {
    Key k{}; const int phase=read<int>(window+0x28,-1);
    if((scene!=2 && scene!=3) || (phase!=4 && phase!=5) || read<uint8_t>(window+0x41)) return k;
    k[0]=window; k[1]=read<int16_t>(window+0x48,-1);
    k[2]=read<int>(window+0x58); k[3]=read<int>(window+0x5c);
    k[4]=read<uint8_t>(address(0x146a351),255);
    return k;
}
struct TargetValues {std::array<unsigned,54> values{};unsigned count=0;};
TargetValues targetValues(const Key& k) {
    if(!k[0] || k[2]<0 || k[2]>1 || k[4]>1) return {};
    using Count=int(__cdecl*)();
    const int count=reinterpret_cast<Count>(address(k[2]?0x4a9ba0:0x4a9ae0))();
    if(count<1 || count>18 || (k[4] && (k[1]<0 || k[1]>=count))) return {};
    TargetValues values;
    for(int row=0;row<count;++row) {
        if(k[4] && row!=k[1]) continue;
        using Party=uint8_t(__cdecl*)(unsigned);
        const auto id=reinterpret_cast<Party>(address(k[2]?0x4a9b70:0x4a9a80))(row);
        if(id>=18) return {};
        // The native target renderer selects MP only for display mode 1.
        // Every other display mode shows HP; never add hidden statistics.
        const bool mp=k[3]==1;
        values.values[values.count++]=id;
        values.values[values.count++]=value(mp?0x4a9920:0x4a9870,id);
        values.values[values.count++]=value(mp?0x4a9960:0x4a9940,id);
    }
    return values;
}
void targetBefore(Event& e) {
    e.before=targetKey(e.args[0]);const auto values=targetValues(e.before);
    e.copiedCount=values.count;std::copy_n(values.values.begin(),values.count,e.copied.begin());
}
void targetAfter(Event& e) {
    const auto k=targetKey(e.args[0]); const auto values=targetValues(k);
    if(!values.count || (k==e.before && values.count==e.copiedCount &&
        std::equal(values.values.begin(),values.values.begin()+values.count,e.copied.begin()))) return;
    TextPlan p;prepareTextPlan(p,"field-item-target");TextRef parts=0;
    for(unsigned i=0;i<values.count;i+=3) {
        const auto name=planPlayerName(p,static_cast<int>(values.values[i]));
        const bool mp=k[3]==1;
        const auto resource=planVitals(p,mp?static_cast<int64_t>(values.values[i+1]):static_cast<int>(values.values[i+1]),
            mp?static_cast<int64_t>(values.values[i+2]):static_cast<int>(values.values[i+2]),mp,setting(Setting::ShowMaximumHp)!=0);
        auto line=planConcat(p,planConcat(p,name,planLiteral(p,L": ")),resource);
        parts=planPair(p,TextPlanOp::Periods,parts,line);
    }
    p.headline=k[4]?parts:planConcat(p,planLiteral(p,L"All party members. "),parts);
    commitTextPlan(p);
    resetSelectionDetail();
}
Key slotKey(uintptr_t window) {
    Key k{};
    if(scene!=4 || read<int>(window+0x1c,-1)!=10) return k;
    using Getter=bool(__cdecl*)(); using Character=uint8_t(__cdecl*)();
    k[0]=window; k[1]=reinterpret_cast<Getter>(address(0x4cfc00))();
    k[2]=reinterpret_cast<Character>(address(0x4a9810))();
    return k;
}
void slotBefore(Event& e) { e.before=slotKey(e.args[0]); }
void slotAfter(Event& e) {
    const auto k=slotKey(e.args[0]);
    if(!k[0] || k==e.before || k[1]>1 || k[2]<0 || k[2]>=18) return;
    const auto id=equippedItem(static_cast<unsigned>(k[2]),k[1]!=0);
    TextPlan p;prepareTextPlan(p,"equipment-slot");
    const auto label=planMenuText(p,11,k[1]?6:5);
    const auto name=planEquippedItemName(p,static_cast<unsigned>(k[2]),k[1]!=0);
    recordLog("equipment slot character=%u armor=%d index=%u",static_cast<unsigned>(k[2]),k[1]!=0,id);
    p.headline=planSuffix(p,label,name,L": ");
    using Gear=uintptr_t(__cdecl*)(unsigned,uintptr_t*);
    planSelectionDetail(p,id>=200?0:planEquipmentDetails(p,reinterpret_cast<Gear>(address(0x3abbf0))(id,nullptr)),setting(Setting::AutoDetail)!=0);
    commitTextPlan(p);
}
}
void installFieldTargets() {
    hook(0x4c7400,targetBefore,targetAfter);
    hook(0x4ceff0,slotBefore,slotAfter);
}
}
