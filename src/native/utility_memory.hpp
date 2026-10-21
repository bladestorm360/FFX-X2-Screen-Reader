#pragma once
#include "runtime.hpp"
#include <functional>
#include "fixed_callback.hpp"
#include "atel_resources.hpp"

namespace ffx {
struct UtilityScript {
    uintptr_t controller = 0, worker = 0, header = 0, chunk = 0;
    std::string name;
    unsigned length = 0, codeOffset = 0, workers = 0;
    unsigned allocationSerial = 0;
};
struct UtilityBinding { unsigned low, high; int value; unsigned element = 0; };
UtilityScript utilityScript();
bool captureUtilityScript(UtilityScript&) noexcept; // Native bounded candidate, no allocated refusal text.
AtelPreparation utilityPreparation(uintptr_t* chunk=nullptr);
unsigned scriptChecksum(const UtilityScript& script);
uintptr_t utilityWorker(const UtilityScript& script, unsigned local);
bool captureUtilityWorker(const UtilityScript&,unsigned local,uintptr_t&) noexcept;
int utilityValue(const UtilityScript& script, unsigned low, unsigned high, unsigned element = 0);
// Owner-only bounded scalar capture for owned planners. Missing preparation,
// changed rows and unreadable values return false without exception allocation.
bool captureUtilityValue(const UtilityScript&,unsigned low,unsigned high,unsigned element,int& value) noexcept;
void utilityWrite(const UtilityScript& script, const UtilityBinding& value);
void utilityRequest(const UtilityScript& script, unsigned worker, unsigned entry,
    FixedCallback<void(bool),64> completed={},FixedCallback<bool(),64> qualified={},bool dispatchIdle=false,bool activateWorker=false);
void utilityWarp(unsigned room, unsigned spawn,bool force=false,bool macalaniaTest=false,bool gagazetTest=false);
}
