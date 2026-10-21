#pragma once
#include "utility_text.hpp"
#include "utility_trials.hpp"
#include "events.hpp"
namespace ffx {
struct PuzzleInteraction { unsigned worker, entry, entryPc, returnPc; bool destruction; UtilityBinding sphere{}, otherSphere{}; };
struct PuzzleContract {
    const char* script;
    UtilityBinding carried;
    int destruction;
    std::vector<PuzzleInteraction> interactions;
};
void installPuzzleSolver();
void pumpPuzzleSolver();
void puzzleInitialized(Event&);
void puzzleCompleted(uintptr_t worker, uintptr_t signal, bool completed, bool utility = false);
void resetPuzzleSolver();
UtilityText puzzleCommand(std::wstring_view action);
bool puzzleArmed();
}
