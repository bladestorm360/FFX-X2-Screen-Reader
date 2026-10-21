#pragma once
#include "runtime.hpp"
namespace ffx {
// Address ownership without acquiring the Windows loader lock. The caller's
// native lifecycle/generation still owns the module's lifetime.
inline uintptr_t loadedImageBase(uintptr_t candidate) {
    MEMORY_BASIC_INFORMATION info{};
    return candidate && VirtualQuery(reinterpret_cast<void*>(candidate),&info,sizeof(info)) &&
        info.State==MEM_COMMIT && info.Type==MEM_IMAGE?
        reinterpret_cast<uintptr_t>(info.AllocationBase):0;
}
}
