#pragma once
#include "utility_text.hpp"
#include "utility_scripts.hpp"
#include <vector>
namespace ffx {
// dome0700 worker37 initialization: <2775 is the normal puzzle, exactly
// 2775 opens the encounter route, 2780..2870 retains its story setup, and
// >=2875 selects the separate stage5 return puzzle. Never synthesize a visit.
enum class ZanarkandPhase { FirstVisit, GateOpen, Story, ReturnVisit };
inline constexpr wchar_t zanarkandGateDirections[]=L"Zanarkand's normal trial gate is open. Leave the trials, re-enter them, then use the exit labelled Cloister of Trials to continue to the trial encounter.";
inline ZanarkandPhase zanarkandPhase(int story) {
    return story<2775?ZanarkandPhase::FirstVisit:story==2775?ZanarkandPhase::GateOpen:
        story>=2875?ZanarkandPhase::ReturnVisit:ZanarkandPhase::Story;
}
struct TrialUtility {
    const wchar_t* name;
    const char* script;
    unsigned length, checksum, room, exitRoom, exitSpawn, mask;
    ScriptAction treasure, collect;
    UtilityBinding ready;
    std::vector<UtilityBinding> preparation;
    unsigned firstStory, endStory;
};
const TrialUtility* currentTrial(const UtilityScript&);
const TrialUtility* captureCurrentTrial(const UtilityScript&) noexcept;
UtilityText trialCommand(std::wstring_view);
}
