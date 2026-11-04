// Native buy/sell lists and quantity selection. Selection identity includes
// current table entry and menu depth, so an unchanged row number is not enough
// after inventory changes. Native getters provide item/equipment text and price;
// owned TextPlans attach descriptions to the matching selected context.
// See Docs/features/menus/index.md#system-screens.
#include "events.hpp"
#include "settings.hpp"
#include "ready_game_text.hpp"
#include "process_storage.hpp"
#include "nonblocking_snapshots.hpp"

namespace ffx {
namespace {
constexpr unsigned Receiver=9,Capture=0,ReadExtra=1;
struct ShopView {SelectionDetailToken token;int scene=0;uintptr_t window=0;unsigned pane=0;};
async::AtomicSnapshot<ShopView> published;
struct ExtraState {ShopView view;std::wstring name,body;};
ProcessStorage<ExtraState> extraStorage;
bool same(SelectionDetailToken a,SelectionDetailToken b) {return a.epoch==b.epoch && a.serial==b.serial;}
bool current(const ShopView& view) {
    ShopView now;
    return view.token.serial && view.token.epoch==generation() && view.scene==scene && published.tryRead(now) &&
        same(view.token,now.token) && view.window==now.window && same(view.token,currentSelectionDetailToken());
}
void retireExtra() {
    auto& state=extraStorage.get();
    if(!current(state.view)) {state.view={};std::wstring{}.swap(state.name);std::wstring{}.swap(state.body);}
}
void clearExtra(uintptr_t window) {
    ShopView view;if(!published.tryRead(view) || !view.token.serial || (window && view.window!=window)) return;
    published.publish({});
    if(same(view.token,currentSelectionDetailToken())) resetSelectionDetail();
}
bool extraAvailable() {
    ShopView view;return published.tryRead(view) && current(view);
}
void readExtra(bool) {
    ShopView view;if(!nativeGameThread() || !published.tryRead(view) || !current(view)) return;
    TextPlan p;prepareTextPlan(p,"shop-extra");p.receiver=Receiver;p.command=ReadExtra;p.detail=view.token;
    p.context[2]=view.scene;p.context[3]=static_cast<intptr_t>(view.window);p.context[4]=view.pane;
    if(!commitTextPlan(p)) output(L"Extra information is busy. Try U again.",true,"shop-extra-pressure");
}
void receive(const speechq::Header& header,const TextPlan& p,std::wstring line,std::wstring modifiers) {
    const ShopView view{p.detail,static_cast<int>(p.context[2]),static_cast<uintptr_t>(p.context[3]),static_cast<unsigned>(p.context[4])};
    auto& state=extraStorage.get();retireExtra();
    if(p.command==ReadExtra) {
        if(!current(view)) return;
        if(!view.pane) {speechq::emitOwned(L"No extra information is displayed for this selection.",header,"shop-extra",false,true);return;}
        if(same(state.view.token,view.token)) speechq::emitOwned(join({state.name,state.body},L". "),header,"shop-extra",false,true);
        else speechq::emitOwned(L"Shop information is not ready. Select the item again.",header,"shop-extra",false,true);
        return;
    }
    if(p.command!=Capture) return;
    const auto controls=copiedControlContext(p.controls);
    const auto name=renderTextPlan(p,static_cast<TextRef>(p.context[0]),controls);
    const auto body=renderTextPlan(p,static_cast<TextRef>(p.context[1]),controls);
    if(current(view)) {state.view=view;state.name=name;state.body=body;}
    speechq::emitOwned(std::move(line),header,p.producer,false,p.interrupt);
    // One selected description: native modifiers first, then the displayed
    // compared character. It stays cached for I regardless of automatic mode.
    emitSelectionDetailOwned(join({modifiers,name},L". "),p.detail,p.automatic,header);
    // Automatic extra review is independent. With the primary already spoken,
    // its character heading introduces this body; otherwise include the heading
    // here. This is structured ordering, never equal-text/event suppression.
    if(p.flags && view.pane && !body.empty())
        speechq::emitOwned(p.automatic?body:join({name,body},L". "),header,"shop-extra-auto",false,false);
}
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
    if (!k[4] || k[0] != 2 || !k[3]) {clearExtra(e.args[0]);return;}
    if(k==e.before) return;
    // Retire the prior pane before any new record/name getter can fail. A
    // missing replacement must not leave U or I reading the previous item.
    clearExtra(0);
    TextPlan p;prepareTextPlan(p,"shop-list");
    TextRef text=0,details=0; unsigned price = 0;uintptr_t selectedRecord=0;
    using Getter = uintptr_t(__cdecl*)(unsigned);
    using Gear = uintptr_t(__cdecl*)(unsigned, uintptr_t*);
    if (e.rva == 0x4d77c0 || e.rva == 0x4d7990) {
        uintptr_t base = 0;
        auto record = e.rva == 0x4d77c0 ? reinterpret_cast<Getter>(address(0x4d9140))(static_cast<unsigned>(k[3])) :
            reinterpret_cast<Gear>(address(0x3abbf0))(static_cast<unsigned>(k[3]), &base);
        if (!record) return;
        selectedRecord=record;
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
    if(e.rva==0x4d77c0 || e.rva==0x4d7990) {
        // These existing native selection events own the displayed shop panes.
        // Renderer8D85F0's owner/type guards qualify the extra data. No window
        // is retained for dereferencing: it is only an opaque context identity.
        p.receiver=Receiver;p.command=Capture;p.flags=setting(Setting::AutoExtraInfo)!=0;
        p.context[0]=planShopComparedCharacter(p,selectedRecord);
        p.context[1]=planShopExtraInfo(p,selectedRecord);p.context[2]=scene;p.context[3]=static_cast<intptr_t>(e.args[0]);p.context[4]=p.context[1]!=0;
        published.publish({p.detail,static_cast<int>(p.context[2]),e.args[0],static_cast<unsigned>(p.context[4])});
    }
    if(!commitTextPlan(p)) clearExtra(e.args[0]);
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
    extraStorage.prepare();registerTextPlanReceiver(Receiver,receive);registerTextPlanMaintenance(Receiver,retireExtra);
    // Qualified chords precede the ordinary U binding, retaining field filtering
    // and native input ownership outside an active equipment-shop selection.
    hotkeyChordWhen('U',false,readExtra,extraAvailable);
    for (auto rva : {0x4d77c0,0x4d7990,0x4d9d70,0x4d0e80}) hook(rva, before, after);
    hook(0x4d9ba0, quantityBefore, quantityAfter); hook(0x4d9df0, quantityBefore, quantityAfter);
}
}
