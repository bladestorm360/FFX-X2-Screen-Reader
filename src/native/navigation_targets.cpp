// Match the exact prepared script identity before applying target metadata.
// Small correction overlays precede the generated worker-sorted catalog; an
// unrelated map with the same local worker number must never inherit a label.
// See Docs/features/navigation/index.md#identification-and-observations.
#include "navigation_targets.hpp"
#include "navigation_targets_data.hpp"
#include "navigation_passages_data.hpp"
namespace ffx {
const NavigationScript* navigationCatalog(const AtelScript& script) {
    for (const auto& record : navigationScripts)
        if (script.name == record.name && (script.codeLength?script.codeLength:script.code.size()) == record.length && script.checksum == record.checksum)
            return &record;
    return nullptr;
}
const NavigationTarget* navigationTarget(const NavigationScript* script, unsigned worker) {
    if (!script) return nullptr;
    for (const auto& record : navigationPassageTargets)
        if (std::string(script->name) == record.script && script->length == record.length && script->checksum == record.checksum && record.worker == worker)
            return &record.target;
    unsigned low = script->start, high = low + script->count;
    while (low < high) {
        const auto middle = low + (high - low) / 2;
        if (navigationTargets[middle].worker < worker) low = middle + 1;
        else high = middle;
    }
    if (low < script->start + script->count && navigationTargets[low].worker == worker)
        return &navigationTargets[low];
    return nullptr;
}
}