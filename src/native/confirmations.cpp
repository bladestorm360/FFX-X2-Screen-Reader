// Keep the opening prompt distinct from changed highlighted options. Autodetail
// belongs to selection detail; the two choices are not appended to the prompt.
// See ../../Docs/features/menus/index.md#system-screens.
#include "events.hpp"
#include "settings.hpp"
#include "ready_game_text.hpp"

namespace ffx {
namespace {
void before(Event& e) {
    e.before[0] = read<int>(e.args[0] + 0x28, -1); e.before[1] = read<int16_t>(e.args[0] + 0x48, -1);
}
void after(Event& e) {
    const auto win = e.args[0];
    const int phase = read<int>(win + 0x28), row = read<int16_t>(win + 0x48);
    if (phase != 1 && phase != 2 && phase != 3) return;
    const bool opening = e.before[0] == 0;
    if (opening) {
        TextPlan p;prepareTextPlan(p,"confirmation-prompt",true);
        p.headline=planEncoded(p,read<uintptr_t>(win+0x24),256,TextPlanOp::Lines);
        commitTextPlan(p);
    }
    if (phase != 1 || (!opening && row == e.before[1]) || row < 0 || row > 1) return;
    TextPlan p;prepareTextPlan(p,"confirmation-option",!opening);
    p.headline=planEncoded(p,read<uintptr_t>(win+0x58+row*4),256,TextPlanOp::First);
    const auto description=planEncoded(p,read<uintptr_t>(win+0x24),256,TextPlanOp::Lines);
    planSelectionDetail(p,description,!opening && setting(Setting::AutoDetail)!=0);
    commitTextPlan(p);
}
}
void installConfirmations() { hook(0x4e2e20, before, after); }
}
