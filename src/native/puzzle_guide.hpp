#pragma once
#include <string_view>
#include "utility_trials.hpp"
#include "navigation.hpp"
namespace ffx {
struct PuzzleStep {
    const wchar_t* instruction;
    unsigned worker;
    Point fallback;
    UtilityBinding done; // Checked low 0 uses an explicitly supported native position witness.
    bool check = false, platform = false, checkpoint = false;
    const wchar_t* label = L"Puzzle control";
    // A second binding that must also match, when the first one is shared
    // with other steps (the carried-sphere variable). low 0 means none.
    UtilityBinding also{0,1,0};
};
struct PuzzleRoute { const char* script; std::vector<PuzzleStep> steps; unsigned returnStep=0; };
std::wstring puzzleGuide(std::wstring_view action);
void puzzleGuideCompleted();
void resetPuzzleGuide();
void installPuzzleGuide();
}
