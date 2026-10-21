// Append current native Learn/Increase item requirements to the selection's
// detail token. Row/depth/phase changes, not equal text, delimit announcements;
// the native getters supply the required item, amount and current inventory.
// See Docs/features/menus/index.md#native-menudetail-flow.
#include "events.hpp"
#include "profile.hpp"
#include "ready_game_text.hpp"

namespace ffx {
namespace {
void requirement(unsigned item, unsigned needed) {
    if (item == 0xffffffff || !item) return;
    TextPlan p;prepareTextPlan(p,"field-requirement",false);
    const auto name=planRecordName(p,0x390a40,item);
    auto line=planConcat(p,planCountedName(p,name,needed),planLiteral(p,L" required, "));
    line=planConcat(p,line,planInteger(p,quantity(item)));
    line=planConcat(p,line,planLiteral(p,L" available"));
    p.headline=p.description=planPair(p,TextPlanOp::IfText,name,line);
    p.append=true;p.detail=currentSelectionDetailToken();commitTextPlan(p);
}
void before(Event& e) {
    const auto p = e.args[0];
    e.before[0] = read<int>(p + 0x28, -1); e.before[1] = read<int16_t>(p + 0x48, -1);
    e.before[2] = read<int8_t>(p + 0x45, -1);
}
void after(Event& e) {
    const auto win = e.args[0];
    const auto renderer = logicalRva(read<uintptr_t>(win + 0x10));
    const int phase = read<int>(win + 0x28), row = read<int16_t>(win + 0x48), depth = read<int8_t>(win + 0x45);
    if (scene != 4 && scene != 7 && scene != 9) return;
    if (phase != 2 || row < 0 || row >= 512 || row >= read<uint16_t>(win + 0x30) || depth < 0 ||
        (e.before[0] == phase && e.before[1] == row && e.before[2] == depth)) return;
    using Table = uintptr_t(__cdecl*)();
    if (renderer == 0x4cdb70 || renderer == 0x4d5f30) {
        const auto entry = address(0x1197730) + row * 4;
        const unsigned index = read<uint8_t>(entry + 3, 255);
        if (index == 255) return;
        const auto table = reinterpret_cast<Table>(address(renderer == 0x4cdb70 ? 0x4c1c00 : 0x4c1b50))();
        if (table) requirement(read<uint16_t>(table + index * 8 + 4), read<uint8_t>(table + index * 8 + 6));
    }
}
}
void announceIncreaseRequirements(int row) {
    if (row < 0 || row >= 10) return;
    using Aeon = uint8_t(__cdecl*)();
    using Requirements = int(__cdecl*)(int, unsigned, unsigned*, unsigned*);
    unsigned item = 0xffffffff, needed = 0;
    reinterpret_cast<Requirements>(address(0x4cd2f0))(row, reinterpret_cast<Aeon>(address(0x4a9830))(), &item, &needed);
    requirement(item, needed);
}
void installFieldDetails() {
    hook(0x4b4460, before, after);
}
}
