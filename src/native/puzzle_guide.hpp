#pragma once
#include <string_view>
#include "utility_trials.hpp"
#include "navigation.hpp"
namespace ffx {
struct PuzzleStep {
    const wchar_t* instruction;
    unsigned worker;
    Point fallback;
    UtilityBinding done;
    bool check = false, platform = false, checkpoint = false;
    const wchar_t* label = L"Puzzle control";
};
struct PuzzleRoute { const char* script; std::vector<PuzzleStep> steps; unsigned returnStep=0; };
std::wstring puzzleGuide(std::wstring_view action);
void puzzleGuideCompleted();
void resetPuzzleGuide();
void installPuzzleGuide();
}
