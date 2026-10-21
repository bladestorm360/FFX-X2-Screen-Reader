#include "puzzle_completion.hpp"
#include "puzzle_completion_data.hpp"
#include <stdexcept>
namespace ffx {
namespace {
const PuzzleCompletion& recipe(const UtilityScript& script) {
    if (!currentTrial(script)) throw std::runtime_error("No supported trial is active");
    for (const auto& r : puzzleCompletions) if (script.name == r.script) return r;
    throw std::runtime_error("No verified trial completion contract");
}
bool firstVisit(const UtilityScript& script, const PuzzleCompletion& r) {
    if(!r.storyThreshold) return false;
    const int progress=utilityValue(script,0x20000a00,1);
    return progress>=static_cast<int>(r.firstMinimum) && progress<static_cast<int>(r.storyThreshold);
}
}
void checkPuzzleCompletion(const UtilityScript& script) {
    const auto& r = recipe(script);
    if(script.name=="dome0700" && zanarkandPhase(utilityValue(script,0x20000a00,1))!=ZanarkandPhase::FirstVisit)
        throw std::runtime_error("The normal Zanarkand trial is already complete or its story sequence is active");
    const bool first = firstVisit(script,r);
    checkUtilityScript(script,first ? r.first : r.later);
    if (first && r.exit.worker) checkUtilityScript(script,r.exit);
    for (const auto& value : r.preparation) utilityValue(script,value.low,value.high,value.element);
    if(const auto bonus=remainingPuzzleTreasure(script)) checkUtilityScript(script,*bonus);
}
const ScriptAction* remainingPuzzleTreasure(const UtilityScript& script) {
    const auto& r=recipe(script);
    return r.bonus.worker && utilityValue(script,r.bonusCollected.low,r.bonusCollected.high,r.bonusCollected.element)!=r.bonusCollected.value?&r.bonus:nullptr;
}
void recordPuzzleTreasure(const UtilityScript& script) {
    const auto& r=recipe(script);
    // Bevelle W15 writes this marker only after W17:7's original REQEW
    // returns (PC79283). Native chest code performs the actual item grant.
    if(r.bonus.worker) utilityWrite(script,r.bonusCollected);
}
bool puzzleCompletionHasExit(const UtilityScript& script) {
    const auto& r=recipe(script); return firstVisit(script,r) && r.exit.worker!=0;
}
bool beginPuzzleCompletion(const UtilityScript& script) {
    const auto& r = recipe(script);
    const bool first = firstVisit(script,r);
    checkPuzzleCompletion(script);
    // Zanarkand's socket helper opens the gate at six inserts and records
    // 2775. Stop there: the player's native exit starts the encounter route.
    if (first || !r.storyThreshold)
        for (const auto& value : r.preparation) utilityWrite(script,value);
    runUtilityScript(script,first ? r.first : r.later);
    return first && r.exit.worker != 0;
}
void leavePuzzleCompletion(const UtilityScript& script) {
    const auto& r = recipe(script);
    if (!r.exit.worker) throw std::runtime_error("This trial has no second completion action");
    runUtilityScript(script,r.exit);
}
}
