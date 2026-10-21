// @desc Find the real action-executor via damage-display backtrace.
//
// WHY a new probe: action_announce_probe hooked FUN_00793420 (0x393420,
// ally dispatcher case 6). AOB-validated as unique; hook genuinely
// attached. Zero fires during a test battle. Conclusion: case 6 is NOT
// the path normal attacks take — the static decompile trail misled us.
// The neighbouring switch cases (FUN_0078DF00, FUN_0078DF40) turned out
// to be state-bookkeeping (sets +0xDCD death flag, +0xDC8 in-battle),
// not action execution. Case 1 (FUN_007A07D0) looks like a script/event
// enqueuer, not a direct executor.
//
// Pivot: stop chasing the decompile. Hook the damage-display function
// — which we KNOW fires for every ally + enemy action — and capture
// Thread.backtrace on entry. The immediate callers ARE the action
// executor chain at runtime, by construction.
//
// Sanity: also hook the per-frame function (0x471D10) so we can confirm
// the probe is alive even if damage never fires. Past incidents
// (user-reported) include "silent hook didn't fire" debugging where
// the cause turned out to be a math or setup error, not a dead hook.
// The frame counter removes that ambiguity — if frame counter advances
// and damage-display fires zero times, the user didn't reach damage.
// If frame counter is ZERO too, the probe is broken.
//
// Also keeps fire counters on the decompile candidates (0x393420,
// 0x3A07D0, 0x3AEF30) — if one of them IS actually called per action,
// the counter proves it.
//
// Pre-flight:
//   1. Load a save from BEFORE a battle (FFX has no mid-battle save).
//   2. run_frida.bat → 1 (FFX) → action_backtrace_probe.
//   3. In battle, perform ally actions — Attack is enough. FFX uses CTB
//      (agility-based turn order), so all fast allies take their turns
//      first; that's expected. Keep going until at least one enemy gets
//      a turn and strikes back (generates a damage number for us).
//   4. Optional: cast a spell, use an item, to cover non-Attack paths.
//   5. Ctrl+C. Paste first ~80 lines of the log.
//
// Output: Docs/Scripts/Frida/output/FFX/action_backtrace_probe.log

const LOG_PATH = 'D:/Games/Dev/Custom/FFX/ffx-x2-screen-reader/Docs/Scripts/Frida/output/FFX/action_backtrace_probe.log';
const logHandle = new File(LOG_PATH, 'w');
function logFile(msg) { try { logHandle.write(msg + '\n'); logHandle.flush(); } catch (e) {} }

let consoleBudget = 25;
function logBoth(msg) {
    if (consoleBudget > 0) { console.log(msg); consoleBudget--; }
    logFile(msg);
}

// ── Module resolution (explicit by name) ──
let gameModule = null;
Process.enumerateModules().forEach(function (m) {
    if (m.name.toLowerCase() === 'ffx.exe') gameModule = m;
});
if (!gameModule) gameModule = Process.enumerateModules()[0];
const base = gameModule.base;
const moduleEnd = base.add(gameModule.size);
function rva(off) { return base.add(off); }

logBoth('[*] action_backtrace_probe  module=' + gameModule.name +
        '  base=0x' + base.toString(16) + '  size=0x' + gameModule.size.toString(16));

// ── RVAs ──
const RVA_FRAME         = 0x471D10;  // per-frame fn (proven-firing sanity)
const RVA_DAMAGE_DISP   = 0x39FA20;  // MsDamageDisplay (proven-firing in prod mod)
const RVA_CASE_6        = 0x393420;  // FUN_00793420 — prior candidate, zero fires
const RVA_CASE_1_TARGET = 0x3A07D0;  // FUN_007A07D0
const RVA_CASE_23_TGT   = 0x3AEF30;  // FUN_007AEF30 (cases 2 & 3)

// ── AOB validation (mandatory per project rule) ──
function patternAt(r, n) {
    try {
        const bytes = rva(r).readByteArray(n);
        if (!bytes) return null;
        const v = new Uint8Array(bytes);
        const parts = [];
        for (let i = 0; i < v.length; i++) {
            let h = v[i].toString(16);
            if (h.length === 1) h = '0' + h;
            parts.push(h);
        }
        return parts.join(' ');
    } catch (e) { return null; }
}
function verifyAOB(label, r, n) {
    const pat = patternAt(r, n);
    if (!pat) { logFile('[AOB ' + label + '] unreadable @ 0x' + r.toString(16)); return; }
    logFile('[AOB ' + label + '] rva=0x' + r.toString(16) + '  pattern=' + pat);
    try {
        const m = Memory.scanSync(base, gameModule.size, pat);
        if (m.length === 1) {
            const found = m[0].address.sub(base).toInt32();
            if (found === r) logFile('[AOB ' + label + '] OK unique at expected RVA');
            else logFile('[AOB ' + label + '] ! unique at 0x' + found.toString(16) + ' expected 0x' + r.toString(16));
        } else logFile('[AOB ' + label + '] ! ' + m.length + ' matches');
    } catch (e) { logFile('[AOB ' + label + '] scan err: ' + e.message); }
}
verifyAOB('frame',       RVA_FRAME,         20);
verifyAOB('damage_disp', RVA_DAMAGE_DISP,   20);
verifyAOB('case_6',      RVA_CASE_6,        20);
verifyAOB('case_1_tgt',  RVA_CASE_1_TARGET, 20);
verifyAOB('case_23_tgt', RVA_CASE_23_TGT,   20);

// ── Battle-state byte ──
function inBattle() {
    try { return rva(0xD2A8E0).readU8() !== 0; } catch (e) { return false; }
}

// ── Backtrace formatter ──
// Convert a NativePointer to "module_rel 0x<rva>" if inside FFX.exe, or
// "<module>+0x<off>" for addresses in other modules, or "<abs>" for
// addresses outside any known module.
const allModules = Process.enumerateModules();
function annotateAddr(p) {
    try {
        if (p.compare(base) >= 0 && p.compare(moduleEnd) < 0) {
            return 'ffx+0x' + p.sub(base).toString(16);
        }
        for (let i = 0; i < allModules.length; i++) {
            const m = allModules[i];
            if (m === gameModule) continue;
            if (p.compare(m.base) >= 0 && p.compare(m.base.add(m.size)) < 0) {
                return m.name + '+0x' + p.sub(m.base).toString(16);
            }
        }
    } catch (e) {}
    return p.toString();
}

// ── Fire counters ──
let frameFires = 0;
let damageFires = 0;
let case6Fires = 0, case1Fires = 0, case23Fires = 0;

// ── Sanity hook: per-frame fn ──
// We know this fires ~60 Hz. If frameFires stays at 0, the probe is
// dead at the root and every other assertion in the log is suspect.
try {
    Interceptor.attach(rva(RVA_FRAME), {
        onLeave: function () {
            frameFires++;
            // Log every 300 frames (~5 seconds at 60 Hz): sanity heartbeat.
            if (frameFires % 300 === 0) {
                logFile('[HEARTBEAT] frames=' + frameFires +
                    '  damage=' + damageFires +
                    '  case6=' + case6Fires +
                    '  case1=' + case1Fires +
                    '  case23=' + case23Fires +
                    '  inBattle=' + inBattle());
            }
        }
    });
    logBoth('[*] hooked frame @ 0x' + RVA_FRAME.toString(16) + ' (sanity)');
} catch (e) { logBoth('[!] frame hook FAILED: ' + e.message); }

// ── Damage display hook with backtrace ──
// The star of the probe. When a damage number appears on screen, we
// capture the call stack. The immediate caller is the action-executor
// at runtime. ACCURATE backtracer (frame-pointer walk) works because
// MSVC code uses standard `push ebp; mov ebp,esp` prologue.
try {
    Interceptor.attach(rva(RVA_DAMAGE_DISP), {
        onEnter: function (args) {
            damageFires++;
            if (!inBattle()) return;  // skip any out-of-battle fire

            const chrId = args[0].toInt32() & 0xFF;
            const dmgType = args[1].toInt32();
            const value = args[2].toInt32();

            let bt = null;
            try { bt = Thread.backtrace(this.context, Backtracer.ACCURATE); }
            catch (e1) {
                try { bt = Thread.backtrace(this.context, Backtracer.FUZZY); }
                catch (e2) { bt = []; }
            }

            const frames = [];
            for (let i = 0; i < Math.min(bt.length, 10); i++) {
                frames.push(annotateAddr(bt[i]));
            }

            logBoth('[DMG #' + damageFires + '] chr=' + chrId +
                ' type=' + dmgType + ' value=' + value);
            logFile('    stack: ' + frames.join(' <- '));
        }
    });
    logBoth('[*] hooked damage @ 0x' + RVA_DAMAGE_DISP.toString(16) + ' (backtrace)');
} catch (e) { logBoth('[!] damage hook FAILED: ' + e.message); }

// ── Candidate counters ──
// Prove by counting whether each decompile-based candidate actually
// fires during the test. Counters advance regardless of battle-state
// so we catch any call.
try {
    Interceptor.attach(rva(RVA_CASE_6), {
        onEnter: function (args) {
            case6Fires++;
            if (case6Fires <= 10) {
                logFile('[CASE6 FIRE #' + case6Fires + '] actor=' + (args[0].toInt32() & 0xFF) +
                    ' flag=0x' + (args[1].toInt32() & 0xFF).toString(16) +
                    ' inBattle=' + inBattle());
            }
        }
    });
    logBoth('[*] hooked case6 @ 0x' + RVA_CASE_6.toString(16));
} catch (e) { logBoth('[!] case6 hook FAILED: ' + e.message); }

try {
    Interceptor.attach(rva(RVA_CASE_1_TARGET), {
        onEnter: function (args) {
            case1Fires++;
            if (case1Fires <= 10) {
                logFile('[CASE1 FIRE #' + case1Fires + '] arg0=' + args[0].toInt32() +
                    ' arg1=' + args[1].toInt32() + '  inBattle=' + inBattle());
            }
        }
    });
    logBoth('[*] hooked case1_tgt @ 0x' + RVA_CASE_1_TARGET.toString(16));
} catch (e) { logBoth('[!] case1_tgt hook FAILED: ' + e.message); }

try {
    Interceptor.attach(rva(RVA_CASE_23_TGT), {
        onEnter: function (args) {
            case23Fires++;
            if (case23Fires <= 10) {
                logFile('[CASE23 FIRE #' + case23Fires + '] arg0=' + args[0].toInt32() +
                    '  inBattle=' + inBattle());
            }
        }
    });
    logBoth('[*] hooked case23_tgt @ 0x' + RVA_CASE_23_TGT.toString(16));
} catch (e) { logBoth('[!] case23_tgt hook FAILED: ' + e.message); }

logBoth('[*] Ready. In battle, do a few ally actions + wait for an enemy turn. Ctrl+C.');
