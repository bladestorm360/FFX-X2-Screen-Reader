// Native buy/sell lists and quantity selection. Selection identity includes
// current table entry and menu depth, so an unchanged row number is not enough
// after inventory changes. Native getters provide item/equipment text and price;
// owned TextPlans attach descriptions to the matching selected context.
// See Docs/features/menus/index.md#system-screens.
#include "events.hpp"
#include "settings.hpp"
#include "ready_game_text.hpp"

namespace ffx {
namespace {
using Key = std::array<intptr_t, 32>;
Key selected(Event& e) {
    Key k{}; const auto win = e.args[0];
    k[0] = read<int>(win + 0x28, -1); k[1] = read<int16_t>(win + 0x48, -1);
    k[2] = read<int8_t>(win + 0x45, -1);
    const auto count = read<uint16_t>(win + 0x30);
    if (k[1] < 0 || k[1] >= count || count > 512 || k[2] < 0) return {};
    if (e.rva == 0x4d77c0 || e.rva == 0x4d9d70) {
        using Table = uintptr_t(__cdecl*)(unsigned, uintptr_t);
        const unsigned idOffset = e.rva == 0x4d77c0 ? 0x146aadc : 0x146abe0;
        const unsigned bankOffset = e.rva == 0x4d77c0 ? 0x146ab68 : 0x146abec;
        auto bank = read<uintptr_t>(address(bankOffset));
        if (!bank) return {};
        auto table = reinterpret_cast<Table>(address(0x4c1ad0))(read<unsigned>(address(idOffset)), bank);
        if (!table || k[1] >= read<uint16_t>(table)) return {};
        k[3] = read<uint16_t>(table + 2 + k[1] * 2);
    } else k[3] = read<uint16_t>(address(0x1197730) + k[1] * 4);
    k[4] = 1;
    if(e.rva==0x4d77c0 || e.rva==0x4d7990) k[5]=read<uint8_t>(address(0x146ab60),255);
    if (e.rva == 0x4d0e80 && k[0] == 3) k[0] = 2;
    return k;
}
bool shop(Event& e) {
    if (e.rva == 0x4d77c0 || e.rva == 0x4d7990 || e.rva == 0x4d9d70) return true;
    return read<uintptr_t>(e.args[0] + 0x10) == address(0x4da440);
}
void before(Event& e) { if (shop(e)) e.before = selected(e); }
void after(Event& e) {
    if (!shop(e)) return;
    auto k = selected(e);
    if (!k[4] || k[0] != 2 || k == e.before || !k[3]) return;
    TextPlan p;prepareTextPlan(p,"shop-list");
    TextRef text=0,details=0; unsigned price = 0;
    using Getter = uintptr_t(__cdecl*)(unsigned);
    using Gear = uintptr_t(__cdecl*)(unsigned, uintptr_t*);
    if (e.rva == 0x4d77c0 || e.rva == 0x4d7990) {
        uintptr_t base = 0;
        auto record = e.rva == 0x4d77c0 ? reinterpret_cast<Getter>(address(0x4d9140))(static_cast<unsigned>(k[3])) :
            reinterpret_cast<Gear>(address(0x3abbf0))(static_cast<unsigned>(k[3]), &base);
        if (!record) return;
        text = planEquipmentName(p,record);
        details=planShopEquipmentDescription(p,record);
        price = reinterpret_cast<Getter>(address(e.rva == 0x4d77c0 ? 0x4d9340 : 0x4d93e0))(record);
    } else {
        text = planRecordName(p,0x390a40,static_cast<unsigned>(k[3]));
        price = reinterpret_cast<Getter>(address(e.rva == 0x4d9d70 ? 0x4db390 : 0x4db3e0))(static_cast<unsigned>(k[3]));
        text=planCountedName(p,text,quantity(static_cast<unsigned>(k[3])));
        details=planRecordDescription(p,0x390a40,static_cast<unsigned>(k[3]));
    }
    const auto line=planConcat(p,text,planConcat(p,planLiteral(p,L", "),planConcat(p,planInteger(p,price),planLiteral(p,L" Gil"))));
    p.headline=planPair(p,TextPlanOp::IfText,text,line);
    planSelectionDetail(p,details,setting(Setting::AutoDetail)!=0);
    commitTextPlan(p);
}
void quantityBefore(Event& e) {
    e.before[0] = read<int>(e.args[0] + 0x28, -1); e.before[1] = read<int16_t>(e.args[0] + 0x48, -1);
}
void quantityAfter(Event& e) {
    const int phase = read<int>(e.args[0] + 0x28), count = read<int16_t>(e.args[0] + 0x48);
    if (phase != 3 || count < 1 || (phase == e.before[0] && count == e.before[1])) return;
    const int price = read<int>(e.args[0] + 0x60);
    const auto id = read<uint16_t>(e.args[0] + 0x5c);
    TextPlan p;prepareTextPlan(p,"shop-quantity");
    auto line=planConcat(p,planRecordName(p,0x390a40,id),planConcat(p,planLiteral(p,L", "),
        planConcat(p,planInteger(p,count),planConcat(p,planLiteral(p,L", "),
            planConcat(p,planInteger(p,static_cast<int64_t>(price)*count),planLiteral(p,L" Gil"))))));
    p.headline=line;
    planSelectionDetail(p,planRecordDescription(p,0x390a40,id),setting(Setting::AutoDetail)!=0);
    commitTextPlan(p);
}
}
void installShops() {
    for (auto rva : {0x4d77c0,0x4d7990,0x4d9d70,0x4d0e80}) hook(rva, before, after);
    hook(0x4d9ba0, quantityBefore, quantityAfter); hook(0x4d9df0, quantityBefore, quantityAfter);
}
}
