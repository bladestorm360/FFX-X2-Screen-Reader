#include "puzzle_solver.hpp"
#include "zanarkand_patterns.hpp"
#include "puzzle_guide.hpp"
#include "puzzle_completion.hpp"
#include "puzzle_solver_data.hpp"
#include "utilities.hpp"
#include "mod_menu.hpp"
#include "via_purifico.hpp"
#include <stdexcept>
#include <cstring>

namespace ffx {
namespace {
enum class Phase { Idle, Activation, Prepare, Reveal, Collect, Chest, Finish, Completion, Leave, Departure, BonusChest };
Phase phase = Phase::Idle;
unsigned epoch = 0;
bool armed = false, treasureOnly = false;
bool completionHasExit = false;
uintptr_t activationWorker = 0, activationSignal = 0;
uintptr_t bonusWorker = 0, bonusSignal = 0;
uintptr_t bonusChunk = 0;
unsigned bonusSerial = 0;
const PuzzleContract* contract = nullptr;
const TrialUtility* trial = nullptr;
const PuzzleContract* findContract(const std::string& name) {
    for (const auto& c : puzzleContracts) if (name == c.script) return &c;
    return nullptr;
}
void stop(const UtilityText& reason) {
    armed = false; phase = Phase::Idle; activationSignal = activationWorker = 0;
    emitUtilityText(reason);
}
UtilityScript context() {
    auto script = utilityScript();
    script.worker = utilityWorker(script, 0); script.header = read<uintptr_t>(script.worker);
    if (epoch != generation() || !trial || currentTrial(script) != trial)
        throw std::runtime_error("The active trial changed; enable the solver again");
    return script;
}
bool collected(const UtilityScript& script) { return (utilityValue(script, 0x01000a70, 1) & trial->mask) != 0; }
}
bool puzzleArmed() { return armed && epoch == generation(); }
void resetPuzzleSolver() {
    armed = false; epoch = 0; phase = Phase::Idle;
    contract = nullptr; trial = nullptr; activationWorker = activationSignal = 0;
    bonusWorker=bonusSignal=0;
    bonusChunk=bonusSerial=0;
    resetPuzzleGuide(); resetViaPurifico();
}
UtilityText puzzleCommand(std::wstring_view action) {
    if(!nativeGameThread()) return L"This utility requires the native game owner.";
    if (action == L"off") {
        armed = false;
        // Do not interrupt an original animation/chest dialogue already running.
        if (phase != Phase::Idle) phase = Phase::Idle;
        return L"Trial solver off. The current game animation or dialogue can finish normally.";
    }
    if (action == L"step" || action == L"expected" || action == L"next" || action == L"previous" || action == L"path") {
        puzzleGuide(action);return {};
    }
    if (action != L"on" && action != L"status") return L"Use the mod menu to enable the trial solver or read the puzzle guide.";
    if (inBattle()) return L"The trial solver is available in the field.";
    auto script = utilityScript();
    script.worker = utilityWorker(script, 0); script.header = read<uintptr_t>(script.worker);
    const auto current = currentTrial(script);
    if (!current) return L"No supported Cloister of Trials is active.";
    if (action == L"status") return UtilityText(current->name) + L" trial. " +
        (puzzleArmed() ? L"Automatic solving is on. " : L"Automatic solving is off. ") + trialCommand(L"status");
    if (phase != Phase::Idle || utilityBusy()) return L"Wait for the current trial script to finish before enabling the solver.";
    const bool zanarkand=script.name=="dome0700";
    const auto visit=zanarkand?zanarkandPhase(utilityValue(script,0x20000a00,1)):ZanarkandPhase::FirstVisit;
    if(zanarkand && visit==ZanarkandPhase::GateOpen) return zanarkandGateDirections;
    if(zanarkand && visit==ZanarkandPhase::Story) return L"Finish Zanarkand's native story and encounter sequence before enabling the return-visit treasure solver.";
    // All state inspection and requests stay on the game's native owner.
    trial = current; contract = findContract(script.name);
    if (!contract) throw std::runtime_error("No interaction contract for this trial");
    utilityValue(script, contract->carried.low, contract->carried.high, contract->carried.element);
    if(!zanarkand || visit==ZanarkandPhase::ReturnVisit) {
        checkUtilityScript(script, trial->treasure); checkUtilityScript(script, trial->collect);
    }
    if(!zanarkand || visit==ZanarkandPhase::FirstVisit) checkPuzzleCompletion(script);
    epoch = generation(); armed = true;
    bonusWorker=bonusSignal=0;
    if(zanarkand) return visit==ZanarkandPhase::FirstVisit?
        L"Zanarkand first-visit automatic solver on. Interact with a puzzle control to solve the normal trial and open its gate. After solving, leave the trials, re-enter them, then use the exit labelled Cloister of Trials to continue to the trial encounter. The Destruction Sphere puzzle remains locked.":
        L"Zanarkand return-visit Destruction Sphere solver on. Interact with a puzzle control to collect its treasure and remain in the trial.";
    if(script.name=="bvyt1100") return L"Bevelle automatic solver on. At the entrance, push the rail pedestal onto the track to start solving. Both trial chests will be collected before leaving. Spheres and recesses are reachable after using the rail pedestal and can also start the solver.";
    return UtilityText(trial->name) + L" automatic solver on. Interact with a sphere, glyph, pedestal, or puzzle control. "
        L"Destruction Sphere interactions collect its treasure and keep you here. Other puzzle interactions collect its treasure, complete the trial, and leave the chamber.";
}
static void interceptPuzzle(Event& event) {
    const bool continuing=phase!=Phase::Idle;
    const bool pending=continuing && epoch==generation();
    if ((!puzzleArmed() && !pending) || !contract || !trial || inBattle()) return;
    // Confirmation input can queue another player interaction during any
    // trial's reveal, chest or completion. Retire only verified player entries;
    // the exact utility initialization receipt preserves our original scripts.
    const auto worker = event.args[0], signal = event.args[1];
    if (!worker || !signal) return;
    if(utilityInitializationOwned(worker,signal)) return;
    auto script = context();
    if(script.name=="dome0700") {
        const auto visit=zanarkandPhase(utilityValue(script,0x20000a00,1));
        const bool openingGate=phase==Phase::Completion && !treasureOnly && visit==ZanarkandPhase::GateOpen;
        if(visit!=ZanarkandPhase::FirstVisit && visit!=ZanarkandPhase::ReturnVisit && !openingGate) { stop(L"Zanarkand's native encounter route is active. Automatic solving is off.");return; }
    }
    const unsigned source = read<uint16_t>(signal + 10, 0xffff);
    if (read<uintptr_t>(worker + 4) != script.chunk ||
        (source != 0xffff && source != read<uint16_t>(script.controller + 0xa))) return;
    const unsigned local = read<uint16_t>(worker + 0x2e, 0xffff);
    const unsigned entry = read<uint16_t>(signal + 8, 0xffff);
    for (const auto& action : contract->interactions) {
        if (local != action.worker || entry != action.entry) continue;
        if (!continuing && utilityBusy()) return;
        const auto header = read<uintptr_t>(worker);
        AtelSpan span;
        if(!nativeAtelSpan(script.chunk,span) || span.serial!=script.allocationSerial || header<script.chunk ||
            header-script.chunk>span.size || span.size-(header-script.chunk)<0x28)
            throw std::runtime_error("Puzzle interaction differs from its verified entry");
        const auto table = read<unsigned>(header + 0x20);
        if (entry >= read<uint16_t>(header + 8) || table>span.size || (entry+1ull)*4>span.size-table ||
            read<unsigned>(script.chunk + table + entry * 4) != action.entryPc || action.returnPc >= script.length ||
            read<uint8_t>(script.chunk + script.codeOffset + action.returnPc) != 0x3c)
            throw std::runtime_error("Puzzle interaction differs from its verified entry");
        unsigned level = read<uint8_t>(signal + 14) & 15;
        const unsigned type = read<uint8_t>(header);
        if (type == 5 || type == 6) level = level > 1 ? 1 : level;
        else if (level > 8) level = 8;
        // Return through this worker's original RET. The native scheduler
        // releases the request and acknowledges REQEW/REQSW normally.
        if (!write<uintptr_t>(worker + 0x12c + level * 0x4c + 0x18,
            script.chunk + script.codeOffset + action.returnPc)) throw std::runtime_error("Could not replace the puzzle interaction");
        if(continuing) {
            recordLog("trial queued player interaction retired worker=%u entry=%u phase=%u",local,entry,static_cast<unsigned>(phase));
            return;
        }
        treasureOnly = action.destruction ||
            utilityValue(script, contract->carried.low, contract->carried.high, contract->carried.element) == contract->destruction ||
            (action.sphere.low && utilityValue(script, action.sphere.low, action.sphere.high, action.sphere.element) == action.sphere.value) ||
            (action.otherSphere.low && utilityValue(script, action.otherSphere.low, action.otherSphere.high, action.otherSphere.element) == action.otherSphere.value);
        if(script.name=="dome0700") treasureOnly=zanarkandPhase(utilityValue(script,0x20000a00,1))==ZanarkandPhase::ReturnVisit;
        activationWorker = worker; activationSignal = signal; phase = Phase::Activation;
        utilityReply(treasureOnly ? L"Solving the Destruction Sphere objective." :
            script.name=="dome0700"?L"Solving Zanarkand's normal first-visit trial.":L"Solving the trial and collecting its treasure.");
        return;
    }
}
void puzzleInitialized(Event& event) {
    try { interceptPuzzle(event); }
    catch (const std::exception& error) {
        stop(utilityError(L"Trial solver stopped: ",error.what()));
    }
}
void puzzleCompleted(uintptr_t worker, uintptr_t signal, bool completed, bool utility) {
    if (epoch != generation()) return;
    if(utility && bonusWorker && worker==bonusWorker && signal==bonusSignal) {
        bonusWorker=bonusSignal=0;
        if(!completed) {stop(L"The remaining trial chest was interrupted. Enable the solver again when control returns.");return;}
        try {
            const auto script=context();
            if(script.chunk!=bonusChunk || script.allocationSerial!=bonusSerial) throw std::runtime_error("The trial resource changed before the chest finished");
            recordPuzzleTreasure(script);
        }
        catch(const std::exception& error) {stop(utilityError(L"Trial treasure collection stopped: ",error.what()));return;}
        // Off lets the already running native chest/dialogue finish and keeps
        // its original caller's marker, without resuming trial completion.
        if(phase==Phase::BonusChest) phase=Phase::Finish;
        return;
    }
    if(phase==Phase::Idle) return;
    if (phase == Phase::Activation) {
        if (worker != activationWorker || signal != activationSignal) return;
    } else if (!utility) return;
    if (!completed) { stop(L"The trial script was interrupted. Enable the solver again when control returns."); return; }
    if (phase == Phase::Activation) {
        activationWorker = activationSignal = 0;
        phase=Phase::Prepare;
    } else if (phase == Phase::Reveal) phase = Phase::Collect;
    else if (phase == Phase::Chest) phase = Phase::Finish;
    else if (phase == Phase::Completion) {
        phase = completionHasExit ? Phase::Leave : Phase::Idle;
        if(trial && !std::strcmp(trial->script,"dome0700")) utilityReply(zanarkandGateDirections);
    }
    else if (phase == Phase::Departure) phase = Phase::Idle;
}
void pumpPuzzleSolver() {
    pumpViaPurifico();
    if (phase == Phase::Idle || phase == Phase::Activation || phase == Phase::Reveal || phase == Phase::Chest ||
        phase == Phase::Completion || phase == Phase::Departure || phase==Phase::BonusChest) return;
    try {
        const auto script = context();
        if (phase == Phase::Prepare) {
            if(script.name=="dome0700" && !treasureOnly) {
                if(zanarkandPhase(utilityValue(script,0x20000a00,1))!=ZanarkandPhase::FirstVisit)
                    throw std::runtime_error("Zanarkand's trial phase changed; use the native encounter route");
                completionHasExit=false;armed=false;phase=Phase::Completion;
                beginPuzzleCompletion(script);
                utilityReply(L"Opening the normal trial gate through its original script. The Destruction Sphere and encounter sequence remain separate.");
                return;
            }
            if(script.name=="dome0700" && zanarkandPhase(utilityValue(script,0x20000a00,1))!=ZanarkandPhase::ReturnVisit)
                throw std::runtime_error("Zanarkand's return puzzle is no longer unlocked");
            if (collected(script)) { phase = Phase::Finish; return; }
            const bool ready = utilityValue(script, trial->ready.low, trial->ready.high, trial->ready.element) == trial->ready.value;
            if (ready) { phase = Phase::Collect; return; }
            phase=Phase::Reveal;
            (void)trialCommand(L"treasure");
            if (!utilityBusy()) throw std::runtime_error("Treasure preparation did not start");
            utilityReply(L"Placing the Destruction Sphere and preparing its treasure. Confirm any game dialogue.");
        } else if (phase == Phase::Collect) {
            if (collected(script)) { phase = Phase::Finish; return; }
            phase=Phase::Chest;
            (void)trialCommand(L"collect");
            if (!utilityBusy()) throw std::runtime_error("Chest collection did not start");
            utilityReply(L"Collecting the treasure. Confirm any game dialogue.");
        } else if (phase == Phase::Leave) {
            phase=Phase::Departure; leavePuzzleCompletion(script);
            utilityReply(L"Trial solved. Taking the game's exit to the next chamber.");
        } else {
            if (!collected(script)) throw std::runtime_error("Original chest script did not record collection");
            if(const auto bonus=remainingPuzzleTreasure(script)) {
                bonusWorker=utilityWorker(script,bonus->worker);bonusSignal=read<uintptr_t>(bonusWorker+0x88);
                bonusChunk=script.chunk;bonusSerial=script.allocationSerial;
                phase=Phase::BonusChest;
                try {runUtilityScript(script,*bonus);}
                catch(...) {bonusWorker=bonusSignal=0;throw;}
                if(!utilityBusy()) throw std::runtime_error("Remaining trial chest did not start");
                utilityReply(L"Collecting the remaining trial chest. Confirm the game dialogue.");return;
            }
            phase = Phase::Idle;
            if (treasureOnly) utilityReply(L"Destruction Sphere treasure collected. You remain in the trial. Automatic solving is still on.");
            else {
                completionHasExit=puzzleCompletionHasExit(script);
                armed=false; phase=Phase::Completion;
                (void)beginPuzzleCompletion(script);
                utilityReply(L"Treasure collected. Completing the trial through its original game script.");
            }
        }
    } catch (const std::exception& error) {
        stop(utilityError(L"Trial solver stopped: ",error.what()));
    }
}
void installPuzzleSolver() { hook(0x469040, nullptr, puzzleInitialized); installPuzzleGuide(); installZanarkandPatterns(); }
}
