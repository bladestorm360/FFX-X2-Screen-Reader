#pragma once
#include "utility_trials.hpp"
namespace ffx {
bool puzzleCompletionHasExit(const UtilityScript&);
struct PuzzleCompletion {
    const char* script;
    ScriptAction first, later, exit;
    unsigned storyThreshold;
    std::vector<UtilityBinding> preparation;
    unsigned firstMinimum=0; // first action may belong to a bounded native story phase
    ScriptAction bonus{};
    UtilityBinding bonusCollected{}; // Original caller's post-chest marker.
};
void checkPuzzleCompletion(const UtilityScript&);
// Zanarkand stops at its gate; the player takes the native encounter route.
bool beginPuzzleCompletion(const UtilityScript&);
void leavePuzzleCompletion(const UtilityScript&);
const ScriptAction* remainingPuzzleTreasure(const UtilityScript&);
void recordPuzzleTreasure(const UtilityScript&);
}
