#pragma once
#include <cstring>
namespace ffx {
// Authored facts about a qualified callback's unavailable branch, not game
// instructions. Baaj's Macalania guard reaches a control-restoration epilogue
// instead of a bare return. It performs no seal activation on that branch.
// Keep unknown calls fail-open: this fact applies only to the exact resource,
// worker, entry and call site verified in the external original-script model.
inline bool unavailableCallbackEpilogue(const char* name,unsigned length,unsigned checksum,
    unsigned local,unsigned entry,unsigned pc,unsigned call) {
    return name && !std::strcmp(name,"bjyt1000") && length==7530 && checksum==0xd13cfb8c &&
        local==34 && entry==2 && pc==7072 && call==93;
}
}
