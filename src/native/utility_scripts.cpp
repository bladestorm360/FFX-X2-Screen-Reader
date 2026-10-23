#include "utility_scripts.hpp"
#include "utilities.hpp"
#include "events.hpp"
#include "puzzle_solver.hpp"
#include "puzzle_guide.hpp"
#include "puzzle_tiles.hpp"
#include "via_purifico.hpp"
#include <stdexcept>

namespace ffx {
namespace {
struct Continuation {
    uintptr_t worker = 0, signal = 0, chunk = 0;
    unsigned epoch = 0, entry = 0, pc = 0, reservation=0, allocationSerial=0;
    int regY = 0;
    int choice = -1;
    bool waiting = false;
    unsigned returnPc = 0;
} active;
unsigned nextReservation=0;
struct Initialization {uintptr_t worker=0,signal=0;unsigned epoch=0;} initialization;
void failContinuation(const wchar_t* message) {
    const auto worker=active.worker,signal=active.signal; active={};
    puzzleCompleted(worker,signal,false,true); viaScriptCompleted(false,true); utilityReply(message);
}

void finishing(Event& e) {
    using Worker = uintptr_t(__cdecl*)(int);
    const auto worker = reinterpret_cast<Worker>(address(0x46a7c0))(static_cast<int>(e.args[0]));
    e.before[0] = worker; e.before[1] = read<uintptr_t>(worker + 0x90);
    e.before[2] = read<uint16_t>(static_cast<uintptr_t>(e.before[1]) + 8,0xffff);
    e.before[3] = 1;
    if(active.returnPc && active.epoch==generation() && active.worker==worker && active.signal==static_cast<uintptr_t>(e.before[1])) {
        const unsigned level=read<uint8_t>(active.signal+14)&15;
        const auto type=read<uint8_t>(read<uintptr_t>(worker));
        // The interpreter calls this function with its PC still at RET.
        // Capture before native release/dispatch can reuse the same signal.
        e.before[3]=level<=((type==5 || type==6)?1u:8u) &&
            read<uintptr_t>(worker+0x12c+level*0x4c+0x18)==
                active.chunk+read<unsigned>(active.chunk+0x30)+active.returnPc;
    }
}
void finished(Event& e) {
    const auto worker = static_cast<uintptr_t>(e.before[0]), signal = static_cast<uintptr_t>(e.before[1]);
    if (!worker || !signal) return;
    const bool owned = active.epoch == generation() && active.worker == worker && active.signal == signal;
    if (owned) active = {};
    const bool completed=e.before[3]!=0;
    puzzleCompleted(worker, signal, completed, owned); viaScriptCompleted(completed,owned);
    puzzleTileCompleted(worker,static_cast<unsigned>(e.before[2]));
    puzzleGuideCompleted();
}

void initialized(Event& e) {
    initialization={};
    if (!active.waiting || active.epoch != generation() ||
        e.args[0] != active.worker || e.args[1] != active.signal) return;
    active.waiting = false;
    if (read<uintptr_t>(active.worker + 4) != active.chunk ||
        read<uint16_t>(active.signal + 8) != active.entry) {
        failContinuation(L"The requested script changed before it could start."); return;
    }
    const unsigned level = read<uint8_t>(active.signal + 0xe) & 15;
    const auto type = read<uint8_t>(read<uintptr_t>(active.worker));
    if (level > ((type == 5 || type == 6) ? 1u : 8u)) { failContinuation(L"The requested native thread is unavailable."); return; }
    const auto thread = active.worker + 0x12c + level * 0x4c;
    const auto code = active.chunk + read<unsigned>(active.chunk + 0x30);
    // FUN_00869040 has initialized this exact requested signal's thread.
    // Resume a verified empty-stack boundary in the original bytecode.
    // No entry table or script code is overwritten.
    if (!write<uintptr_t>(thread + 0x18, code + active.pc) ||
        !write<int>(thread + 0x24, active.regY)) {
        failContinuation(L"Could not start the requested native script."); return;
    }
    recordLog("utility continuation worker=%u entry=%u pc=%x signal=%p",
        read<uint16_t>(active.worker + 0x2e), active.entry, active.pc,
        reinterpret_cast<void*>(active.signal));
    initialization={e.args[0],e.args[1],generation()};
}
void automaticChoice(Event& e) {
    if(active.choice<0 || active.waiting || active.epoch!=generation() ||
        e.args[0]!=active.worker || !active.signal || read<uintptr_t>(active.worker+4)!=active.chunk ||
        read<uintptr_t>(active.worker+0x90)!=active.signal || !e.args[1] || read<unsigned>(e.args[1])!=1) return;
    AtelSpan span;if(!nativeAtelSpan(active.chunk,span) || span.serial!=active.allocationSerial) return;
    // Common013B's original exec859A80 stores a signed16 selection at frame+4
    // and returns1 when done. Its original ret-int85A590 then closes the menu,
    // restores the operand stack and returns this selection to ATEL. Preserve
    // that return path, all preceding setup and all subsequent native effects.
    if(write<int16_t>(e.args[1]+4,static_cast<int16_t>(active.choice))) {active.choice=-1;e.result=1;e.overrideResult=true;}
}
}
bool utilityInitializationOwned(uintptr_t worker,uintptr_t signal) {
    return nativeGameThread() && initialization.epoch==generation() &&
        initialization.worker==worker && initialization.signal==signal;
}
bool utilityBusy() {
    if (!active.signal || active.epoch != generation()) { active = {}; return false; }
    // A free signal is reserved before its deferred request enters native code.
    if(active.waiting) return true;
    // Read only in response to an explicit command, never a passive watcher.
    auto signal = read<uintptr_t>(active.worker + 0x80);
    for (unsigned count = 0; signal && count < 64; ++count) {
        if (signal == active.signal) return true;
        signal = read<uintptr_t>(signal);
    }
    active = {}; return false;
}
void checkUtilityScript(const UtilityScript& script, const ScriptAction& action) {
    if (utilityBusy()) throw std::runtime_error("A utility script is still running; finish its dialogue first");
    const auto worker = utilityWorker(script, action.worker);
    const auto header = read<uintptr_t>(worker);
    AtelSpan span;
    if(!nativeAtelSpan(script.chunk,span) || span.serial!=script.allocationSerial || header<script.chunk ||
        header-script.chunk>span.size || span.size-(header-script.chunk)<0x28)
        throw std::runtime_error("The native entry table does not match the verified action");
    const auto entryOffset = read<unsigned>(header + 0x20);
    if (action.entry >= read<uint16_t>(header + 8) || entryOffset>span.size || (action.entry+1ull)*4>span.size-entryOffset ||
        read<unsigned>(script.chunk + entryOffset + action.entry * 4) != action.entryPc || action.pc >= script.length ||
        (action.returnPc && (action.returnPc>=script.length || read<uint8_t>(script.chunk+script.codeOffset+action.returnPc)!=0x3c)))
        throw std::runtime_error("The native entry table does not match the verified action");
    const auto signal = read<uintptr_t>(worker + 0x88);
    if (!signal) throw std::runtime_error("The native worker has no free request slots");
}
void runUtilityScript(const UtilityScript& script, const ScriptAction& action) {
    checkUtilityScript(script, action);
    const auto worker = utilityWorker(script, action.worker);
    const auto signal = read<uintptr_t>(worker + 0x88);
    const unsigned reservation=++nextReservation;
    active = {worker, signal, script.chunk, generation(), action.entry, action.pc, reservation, script.allocationSerial, action.regY, action.choice, true, action.returnPc};
    try { utilityRequest(script, action.worker, action.entry,[reservation](bool success) {
        if(success || active.reservation!=reservation || active.epoch!=generation()) return;
        const auto worker=active.worker,signal=active.signal; active={};
        puzzleCompleted(worker,signal,false,true); viaScriptCompleted(false,true);
    },[reservation] {
        return active.waiting && active.reservation==reservation && active.epoch==generation() &&
            read<uintptr_t>(active.worker+4)==active.chunk && read<uintptr_t>(active.worker+0x88)==active.signal;
    },true,action.activateWorker); }
    catch (...) { active = {}; throw; }
}
void installUtilityScripts() {
    hook(0x469040, nullptr, initialized);
    // The interpreter's 3C/3D/3E/3F cases use 69170, directly or via
    // its controller return callback. It releases +90, restores +88,
    // acknowledges the caller and starts the next native signal.
    hook(0x469170, finishing, finished);
    hook(0x459a80,nullptr,automaticChoice);
}
}
