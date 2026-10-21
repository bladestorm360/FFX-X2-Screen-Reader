#pragma once
#include "events.hpp"
namespace ffx {
bool statusAnimationTarget(const Event&,unsigned& owner,unsigned& buff);
#ifdef FFX_READER_TEST
void setStatusAnimationFixture(uintptr_t base);
#endif
}
