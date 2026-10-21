#include "utility_trials.hpp"
#include "utilities.hpp"
#include "events.hpp"
#include "utility_trials_data.hpp"
#include "puzzle_completion.hpp"
#include <stdexcept>

namespace ffx {
const TrialUtility* captureCurrentTrial(const UtilityScript& script) noexcept {
    AtelIdentity identity;
    if(!readyAtelIdentity(script.chunk,identity) || identity.span.serial!=script.allocationSerial ||
        identity.length!=script.length || identity.workers!=script.workers || identity.codeOffset!=script.codeOffset || script.name!=identity.name) return nullptr;
    for(const auto& trial:trialUtilities) if(script.name==trial.script)
        return script.length==trial.length && identity.checksum==trial.checksum?&trial:nullptr;
    return nullptr;
}
const TrialUtility* currentTrial(const UtilityScript& script) {
    for (const auto& trial : trialUtilities) if (script.name == trial.script) {
        if (script.length != trial.length || scriptChecksum(script) != trial.checksum)
            throw std::runtime_error("This trial's script differs from the verified source");
        return &trial;
    }
    return nullptr;
}
UtilityText trialCommand(std::wstring_view action) {
    if(!nativeGameThread()) return L"This utility requires the native game owner.";
    if (action != L"status" && action != L"treasure" && action != L"collect" && action != L"finish")
        return L"Use trial status, trial treasure, trial collect, or trial finish.";
    if (inBattle()) return L"Trial commands are available in the field.";
    auto script = utilityScript();
    script.worker = utilityWorker(script, 0); script.header = read<uintptr_t>(script.worker);
    const auto trial = currentTrial(script);
    if (!trial) return L"You are not in one of the six supported trials. Use warps to find them.";
    const bool collected = (utilityValue(script, 0x01000a70, 1) & trial->mask) != 0;
    const bool prepared = utilityValue(script, trial->ready.low, trial->ready.high, trial->ready.element) == trial->ready.value;
    const UtilityText name = trial->name;
    if(script.name=="dome0700") {
        const auto phase=zanarkandPhase(utilityValue(script,0x20000a00,1));
        if(phase==ZanarkandPhase::GateOpen && (action==L"status" || action==L"finish")) return zanarkandGateDirections;
        if(action==L"status") return name+(phase==ZanarkandPhase::FirstVisit?
            L": first-visit trial. Destruction Sphere objective unlocks on a later return.":
            phase==ZanarkandPhase::Story?L": the native story and encounter sequence must finish before the return puzzle unlocks.":
            collected?L": return-visit Destruction Sphere treasure collected.":L": return-visit Destruction Sphere objective available.");
        if(action==L"finish") {
            if(phase!=ZanarkandPhase::FirstVisit) return L"The normal Zanarkand trial needs no completion shortcut. Use its native exit; the return treasure solver keeps you here.";
            if(utilityBusy()) return L"A utility script is still running. Finish its dialogue, then try again.";
            using Mode=int(__cdecl*)();
            if(reinterpret_cast<Mode>(address(0x46c840))()!=0) return L"Close the current game dialogue before using a trial command.";
            beginPuzzleCompletion(script);
            return L"Opening Zanarkand's normal trial gate. After it opens, leave the trials, re-enter them, then use the exit labelled Cloister of Trials to continue to the trial encounter.";
        }
        if(phase!=ZanarkandPhase::ReturnVisit) return L"Zanarkand's Destruction Sphere puzzle is locked until the game's later return visit. Complete the normal trial and its encounter sequence first.";
    }
    if (action == L"status") return name + (utilityBusy() ? L": a utility script is running." :
        collected ? L": Destruction Sphere treasure collected." : prepared ?
        L": Destruction Sphere objective complete; treasure available." : L": treasure objective not yet complete.");
    if (utilityBusy()) return L"A utility script is still running. Finish its dialogue, then try again.";
    using Mode = int(__cdecl*)();
    if (reinterpret_cast<Mode>(address(0x46c840))() != 0)
        return L"Close the current game dialogue before using a trial command.";
    if (action == L"treasure") {
        if (collected) return name + L": the treasure has already been collected.";
        if (prepared) return name + L": treasure ready. Open the chest or use trial collect.";
        checkUtilityScript(script, trial->treasure);
        for (const auto& value : trial->preparation)
            utilityValue(script, value.low, value.high, value.element);
        for (const auto& value : trial->preparation) utilityWrite(script, value);
        runUtilityScript(script, trial->treasure);
        if (script.name == "dome0700") utilityRequest(script, 32, 7);
        return name + L": preparing the Destruction Sphere treasure. Remain in the chamber; collect it before trial finish.";
    }
    if (action == L"collect") {
        if (collected) return name + L": the treasure has already been collected.";
        if (!prepared) return L"Use trial treasure first.";
        // Original chest scripts grant items and record the temple bit.
        runUtilityScript(script, trial->collect);
        return name + L": opening the treasure chest. Confirm any game dialogue.";
    }
    if (!collected) return L"Collect the Destruction Sphere treasure before finishing. Use trial treasure, then trial collect.";
    // Test warps never change story. Real first visits keep the native
    // completion write without regressing later saves.
    if (trial->endStory) {
        const int story = utilityValue(script, 0x20000a00, 1);
        if (story >= static_cast<int>(trial->firstStory) && story < static_cast<int>(trial->endStory))
            utilityWrite(script, {0x20000a00, 1, static_cast<int>(trial->endStory)});
    }
    if (script.name == "bvyt1100") utilityWrite(script, {0x0c00002b, 1, 1});
    utilityWarp(trial->exitRoom, trial->exitSpawn);
    return name + L": finishing the trial and leaving for the antechamber.";
}
}
