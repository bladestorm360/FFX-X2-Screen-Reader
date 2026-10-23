#pragma once
#include "utility_memory.hpp"
namespace ffx {
// returnPc, when supplied, requires the owned request to reach this original
// final RET. A native cancellation/other return cannot acknowledge its reward.
struct ScriptAction { unsigned worker, entry, entryPc, pc; int regY = 0, choice = -1; bool activateWorker = false; unsigned returnPc = 0; };
void installUtilityScripts();
void runUtilityScript(const UtilityScript&, const ScriptAction&);
void checkUtilityScript(const UtilityScript&, const ScriptAction&);
bool utilityBusy();
// Only inside the serialized469040 listener chain: its initialization receipt
// distinguishes an owned request from next-signal pointer reuse during return.
bool utilityInitializationOwned(uintptr_t worker,uintptr_t signal);
}
