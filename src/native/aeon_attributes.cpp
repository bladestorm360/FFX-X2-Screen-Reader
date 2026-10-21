// Aeon scene 7 phases map through the native selected-id getter to owned status pages.
// Feature flow: ../../Docs/features/menus/index.md#native-menudetail-flow
#include "events.hpp"
#include "status_review.hpp"
namespace ffx {
namespace {
void after(Event& e) {
    if(scene!=7) return;
    const int phase=read<int>(e.args[0]+0x28,-1);
    using Selected=uint8_t(__cdecl*)();
    const unsigned id=reinterpret_cast<Selected>(address(0x4a9830))();
    const int page=phase==4?0:(phase>=8 && phase<=14 && !(phase&1))?phase/2-3:-1;
    showAeonStatus(id,page);
}
}
void installAeonAttributes() {hook(0x4ccb30,nullptr,after);}
}
