// @desc Stalker delta of baseline-vs-one-action — find the unified action-execution path.
//
// Pattern lifted from targeting_writer_probe.js (proven working in
// session 19). The theory (from user): FFX almost certainly uses ONE
// unified action-execution code path for allies, aeons, and enemies —
// the devs would not maintain three parallel dispatchers. Stalker's
// `onCallSummary` aggregates per-call-target counts during a tracked
// window. If we compare baseline (idle in battle) vs armed (one ability
// executes), the calls that appear in armed but NOT in baseline — or
// spike well above their baseline rate — are the action-execution path.
//
// Pre-flight:
//   1. Pre-battle save → load → enter battle.
//   2. Reach a moment where nothing is happening (CTB waiting, no
//      animation in progress). Just stand in battle, hands off.
//   3. run_frida.bat → 1 (FFX) → action_stalker_probe.
//
// Hotkeys:
//   F8   baseline toggle. First press: start Stalker on this thread,
//        stay IDLE for 5-10 seconds (no input). Second press: stop
//        baseline; per-frame rate per-target is computed and cached.
//   F9   arm. Stalker starts IMMEDIATELY into totalCounts. The probe
//        then waits for the action window to open and close:
//          BEGIN = actor's df0 enters 3 (action starts — session 32
//                  established this as the exec window).
//          END   = first damage event fires (MsDamageDisplay at
//                  0x39FA20). Damage means the ability has been
//                  dispatched and resolved — the banner was rendered,
//                  the ability script ran, the target/damage were
//                  computed. Stopping here gives the tightest window
//                  from action-start to effect-landed, no post-
//                  animation tail.
//          FALLBACK = if no damage fires (misses, stat-buff-only
//                     actions), df0 returning to 0 ends the capture.
//        At both BEGIN and END, totalCounts is snapshotted; the diff
//        is execCounts (exec-phase-only calls). If auto-detect never
//        fires, F10 dumps what's been captured since F9.
//   F10  manual dump. Works from either armed_waiting (auto-detect
//        missed) or armed_capturing (stop mid-action). Stops Stalker,
//        dumps counts + sanity + state-byte-peak summary.
//   F11  reset. Clears all state; baseline needs to be re-captured.
//
// Success criteria in the log:
//   - Heartbeat lines advance during both baseline and armed phases.
//   - At least one actor's +0xDFB byte cycled through 4 or 5 during
//     the armed window (proves an action fired).
//   - NEW section lists 5-20 FFX RVAs — these are the action-execution
//     path candidates. I cross-reference them against the decompile to
//     identify the entry-point function.
//
// Output: Docs/Scripts/Frida/output/FFX/action_stalker_probe.log

const LOG_PATH = 'D:/Games/Dev/Custom/FFX/ffx-x2-screen-reader/Docs/Scripts/Frida/output/FFX/action_stalker_probe.log';
const logHandle = new File(LOG_PATH, 'w');
function logFile(msg) { try { logHandle.write(msg + '\n'); logHandle.flush(); } catch (e) {} }
function logBoth(msg) { console.log(msg); logFile(msg); }

// ── Module + RVA helpers ──
let gameModule = null;
Process.enumerateModules().forEach(function (m) {
    if (m.name.toLowerCase() === 'ffx.exe') gameModule = m;
});
if (!gameModule) gameModule = Process.enumerateModules()[0];
const base = gameModule.base;
const moduleEnd = base.add(gameModule.size);
function rva(off) { return base.add(off); }

logBoth('[*] action_stalker_probe  module=' + gameModule.name +
        '  base=0x' + base.toString(16));

// ── Battle-state + actor-state sanity ──
const RVA_BTL_STATE = 0xD2A8E0;
const RVA_ALLY_PTR  = 0xD334CC;
const ALLY_STRIDE   = 0xF90;
const NUM_ACTORS    = 0x1f;
function inBattle() {
    try { return rva(RVA_BTL_STATE).readU8() !== 0; } catch (e) { return false; }
}
function getActor(slot) {
    try {
        if (slot >= NUM_ACTORS) return null;
        const p = rva(RVA_ALLY_PTR).readPointer();
        if (p.isNull() || p.compare(ptr(0x10000)) < 0) return null;
        return p.add(slot * ALLY_STRIDE);
    } catch (e) { return null; }
}
function snapshotActorStates() {
    const out = new Array(NUM_ACTORS);
    for (let i = 0; i < NUM_ACTORS; i++) {
        const a = getActor(i);
        if (!a) { out[i] = null; continue; }
        try {
            out[i] = {
                df0: a.add(0xDF0).readU8(),
                dfb: a.add(0xDFB).readU8()
            };
        } catch (e) { out[i] = null; }
    }
    return out;
}

// ── Keyboard input (proven pattern from targeting_writer_probe.js) ──
let fnGetAsyncKeyState = null;
let fnGetForegroundWindow = null;
let fnGetWindowThreadProcessId = null;
const myPid = Process.id;
const pidBuf = Memory.alloc(4);
const keyWasDown = {};
Process.enumerateModules().forEach(function (m) {
    if (m.name.toLowerCase() === 'user32.dll') {
        m.enumerateExports().forEach(function (exp) {
            if (exp.name === 'GetAsyncKeyState') {
                fnGetAsyncKeyState = new NativeFunction(exp.address, 'int16', ['int']);
            }
            if (exp.name === 'GetForegroundWindow') {
                fnGetForegroundWindow = new NativeFunction(exp.address, 'pointer', []);
            }
            if (exp.name === 'GetWindowThreadProcessId') {
                fnGetWindowThreadProcessId = new NativeFunction(
                    exp.address, 'uint32', ['pointer', 'pointer']);
            }
        });
    }
});
function isGameFocused() {
    if (!fnGetForegroundWindow || !fnGetWindowThreadProcessId) return true;
    const hwnd = fnGetForegroundWindow();
    if (hwnd.isNull()) return false;
    fnGetWindowThreadProcessId(hwnd, pidBuf);
    return pidBuf.readU32() === myPid;
}
function keyPressed(vk) {
    if (!fnGetAsyncKeyState) return false;
    const down = (fnGetAsyncKeyState(vk) & 0x8000) !== 0;
    const was = keyWasDown[vk] || false;
    keyWasDown[vk] = down;
    if (!isGameFocused()) return false;
    return down && !was;
}
const VK_F8 = 0x77, VK_F9 = 0x78, VK_F10 = 0x79, VK_F11 = 0x7A;

// ── Stalker state machine ──
// idle → baseline → idle → armed_waiting → armed_capturing → idle
//   'armed_waiting'   = F9 pressed, probe watches for any actor dfb∈{4,5}
//   'armed_capturing' = Stalker actively following, waiting for acting
//                       actor's dfb to return to 0
let state = 'idle';
let gameThreadId = null;
let frameCount = 0;
let excludedOnce = false;
let baselineCounts = {}, baselineRates = null;
let baselineFrames = 0, baselineStart = 0;
// totalCounts = running accumulator since F9 (Stalker writes here).
// execCounts = diff (totalCounts at endCapture minus totalCounts at
// beginCapture) — this is the exec-phase-only call map we analyze.
// startSnapshot = deep copy of totalCounts at beginCapture time.
let totalCounts = {}, startSnapshot = {}, execCounts = {};
let armedFrames = 0, armedStart = 0;
let actingSlot = -1;  // slot of the actor whose action we're capturing

// Sanity captures
let armedBtlBefore = 0, armedBtlAfter = 0;
let armedActorsBefore = null, armedActorsAfter = null;
let armedActorPollMax = {};  // per-slot max dfb value during armed (both phases)

// Per-actor prev state for change streaming. -1 = uninitialized (so
// the first seen value isn't logged as a change).
const prevDf0 = new Array(NUM_ACTORS).fill(-1);
const prevDfb = new Array(NUM_ACTORS).fill(-1);

function excludeNonGameModules() {
    Process.enumerateModules().forEach(function (m) {
        if (m.name.toLowerCase() === gameModule.name.toLowerCase()) return;
        try { Stalker.exclude({ base: m.base, size: m.size }); } catch (e) {}
    });
}

// Shutdown flag. All hook callbacks check this and return early during
// Frida detach so we don't race the teardown. rpc.exports.dispose sets
// this synchronously before Frida begins unhooking.
let shuttingDown = false;
function startStalker(target) {
    if (!excludedOnce) { excludeNonGameModules(); excludedOnce = true; }
    Stalker.follow(gameThreadId, {
        events: { call: true },
        onCallSummary: function (summary) {
            for (const addr in summary) {
                target[addr] = (target[addr] || 0) + summary[addr];
            }
        }
    });
}
function stopStalker() { try { Stalker.unfollow(gameThreadId); } catch (e) {} }

function cmdBaseline() {
    if (gameThreadId === null) { logBoth('[!] thread not captured yet — wait for first frame'); return; }
    if (state === 'armed_waiting' || state === 'armed_capturing') {
        logBoth('[!] F11 to reset armed first'); return;
    }
    if (state === 'idle' && baselineRates === null) {
        baselineCounts = {}; baselineStart = frameCount; state = 'baseline';
        try {
            startStalker(baselineCounts);
            logBoth('[BASELINE STARTED] stay IDLE 5-10s — F8 to stop');
        } catch (e) { logBoth('[!] follow failed: ' + e.message); state = 'idle'; }
    } else if (state === 'baseline') {
        stopStalker();
        baselineFrames = frameCount - baselineStart;
        if (baselineFrames < 10) {
            logBoth('[!] baseline too short (' + baselineFrames + ' frames); reset and retry');
            baselineCounts = {}; state = 'idle'; return;
        }
        baselineRates = {};
        for (const a in baselineCounts) baselineRates[a] = baselineCounts[a] / baselineFrames;
        state = 'idle';
        logBoth('[BASELINE STOPPED] ' + baselineFrames + ' frames, ' +
                Object.keys(baselineCounts).length + ' call targets. F9 to arm.');
    }
}

function cmdArm() {
    if (baselineRates === null) { logBoth('[!] need baseline first (F8 twice)'); return; }
    if (state !== 'idle') return;
    totalCounts = {}; startSnapshot = {}; execCounts = {};
    armedStart = frameCount;
    armedBtlBefore = inBattle() ? 1 : 0;
    armedActorsBefore = snapshotActorStates();
    armedActorPollMax = {};
    actingSlot = -1;
    for (let i = 0; i < NUM_ACTORS; i++) { prevDf0[i] = -1; prevDfb[i] = -1; }
    // Start Stalker ONCE. It runs continuously from now until endCapture.
    // We never stop/restart mid-flight — that drops buffered events
    // (session 29's 0-calls bug). Instead we snapshot+diff totalCounts.
    state = 'armed_waiting';
    try {
        startStalker(totalCounts);
        logBoth('[ARMED WAITING] inBattle=' + armedBtlBefore +
                ' — Stalker running continuously; snapshot-diff on action');
    } catch (e) {
        logBoth('[!] follow failed: ' + e.message); state = 'idle';
    }
}

// Transitions armed_waiting → armed_capturing on df0=5. Stalker keeps
// running continuously — we just snapshot the accumulated counter at
// this point. At endCapture we'll snapshot again and compute the diff;
// that's the exec-phase-only call map. No stop/restart, so no lost
// events.
function beginCapture(slot) {
    // Force any buffered Stalker events into totalCounts BEFORE we
    // snapshot — otherwise pre-action events still in-flight would
    // land in totalCounts after our snapshot and pollute execCounts.
    try { Stalker.flush(); } catch (e) {}
    startSnapshot = {};
    for (const k in totalCounts) startSnapshot[k] = totalCounts[k];
    armedStart = frameCount; actingSlot = slot;
    state = 'armed_capturing';
    logBoth('[CAPTURE START] slot=' + slot + ' (df0 entered exec=3) — snapshot taken');
}

// End of exec phase. Flush Stalker's buffer into totalCounts, take a
// second snapshot, diff against startSnapshot to isolate exec-phase
// calls, then stop Stalker and dump.
function endCaptureAuto(reason) {
    if (state !== 'armed_capturing') return;
    try { Stalker.flush(); } catch (e) {}
    execCounts = {};
    for (const k in totalCounts) {
        const diff = totalCounts[k] - (startSnapshot[k] || 0);
        if (diff > 0) execCounts[k] = diff;
    }
    stopStalker();
    armedFrames = frameCount - armedStart;
    armedBtlAfter = inBattle() ? 1 : 0;
    armedActorsAfter = snapshotActorStates();
    state = 'idle';
    logBoth('[CAPTURE END: ' + reason + '] slot=' + actingSlot +
            '  armed=' + armedFrames + ' frames  inBattle before/after=' +
            armedBtlBefore + '/' + armedBtlAfter +
            '  execCounts=' + Object.keys(execCounts).length);
    dumpSanity();
    dumpFiltered();
}

function cmdDump() {
    // Manual override. Works from both armed_waiting (auto-detect missed)
    // and armed_capturing (user wants to stop mid-action).
    if (state !== 'armed_waiting' && state !== 'armed_capturing') return;
    try { Stalker.flush(); } catch (e) {}
    const fromWaiting = (state === 'armed_waiting');
    if (fromWaiting) {
        // No begin-snapshot was taken — execCounts = totalCounts as a
        // raw dump of everything since F9 (noisy but useful).
        execCounts = {};
        for (const k in totalCounts) execCounts[k] = totalCounts[k];
    } else {
        // Same diff logic as endCaptureAuto.
        execCounts = {};
        for (const k in totalCounts) {
            const diff = totalCounts[k] - (startSnapshot[k] || 0);
            if (diff > 0) execCounts[k] = diff;
        }
    }
    stopStalker();
    armedFrames = frameCount - armedStart;
    armedBtlAfter = inBattle() ? 1 : 0;
    armedActorsAfter = snapshotActorStates();
    state = 'idle';

    logBoth('[DUMP MANUAL' + (fromWaiting ? ' (auto-detect missed; raw total)' : '') +
            '] slot=' + actingSlot + '  armed=' + armedFrames +
            ' frames  inBattle before/after=' + armedBtlBefore + '/' + armedBtlAfter +
            '  execCounts=' + Object.keys(execCounts).length);

    dumpSanity();
    dumpFiltered();
}

function dumpSanity() {
    logFile('[SANITY] actor-state transitions during armed:');
    let anyActed = false;
    for (let i = 0; i < NUM_ACTORS; i++) {
        const peak = armedActorPollMax[i];
        if (peak === undefined) continue;
        const b = armedActorsBefore && armedActorsBefore[i];
        const a = armedActorsAfter && armedActorsAfter[i];
        // "Acting" = df0 reached 5 (execute phase) in the current model.
        // "Acting" = df0 reached at least 3 (exec phase) at some point.
        // Session 32/33 established df0=3 as the action window; df0=5
        // is a brief finalize marker. Either satisfies "action fired".
        const peakIsActing = (peak.df0 >= 3);
        if (peakIsActing) anyActed = true;
        logFile('  slot=' + i +
            ' before=' + (b ? ('df0=' + b.df0 + ' dfb=' + b.dfb) : 'n/a') +
            ' peak_df0=' + peak.df0 + ' peak_dfb=' + peak.dfb +
            ' after=' + (a ? ('df0=' + a.df0 + ' dfb=' + a.dfb) : 'n/a'));
    }
    logBoth('[SANITY] action-fired = ' + anyActed +
            (anyActed ? '' : ' (WARNING — no actor reached df0≥3; rerun)'));
}

function cmdReset() {
    if (state === 'baseline' || state === 'armed_waiting' || state === 'armed_capturing') stopStalker();
    state = 'idle';
    baselineCounts = {}; baselineRates = null; baselineFrames = 0;
    totalCounts = {}; startSnapshot = {}; execCounts = {};
    armedFrames = 0;
    armedBtlBefore = 0; armedBtlAfter = 0;
    armedActorsBefore = null; armedActorsAfter = null;
    armedActorPollMax = {};
    actingSlot = -1;
    logBoth('[RESET]');
}

function dumpFiltered() {
    const newCalls = [], surgeCalls = [], raw = [];
    for (const a in execCounts) {
        const p = ptr(a);
        if (p.compare(base) < 0 || p.compare(moduleEnd) >= 0) continue;
        const rvaOff = p.sub(base).toInt32();
        const armedN = execCounts[a];
        const baseN = baselineCounts[a] || 0;
        const baseRate = baselineRates[a] || 0;
        const expected = baseRate * armedFrames;
        raw.push({ rva: rvaOff, armedN, baseN });
        if (baseN === 0) newCalls.push({ rva: rvaOff, armedN });
        else if (armedN >= 3 && expected > 0 && (armedN / expected) >= 3) {
            surgeCalls.push({ rva: rvaOff, armedN,
                expected: expected.toFixed(2), ratio: (armedN / expected).toFixed(2) });
        }
    }
    newCalls.sort(function (a, b) { return b.armedN - a.armedN; });
    surgeCalls.sort(function (a, b) { return b.armedN - a.armedN; });
    raw.sort(function (a, b) { return b.armedN - a.armedN; });
    logFile('');
    logFile('=== NEW (baseline=0) — ' + newCalls.length + ' ===');
    for (let i = 0; i < newCalls.length; i++) {
        logFile('  0x' + newCalls[i].rva.toString(16) + '  armed=' + newCalls[i].armedN);
    }
    logFile('');
    logFile('=== SURGE (>=3x rate) — ' + surgeCalls.length + ' ===');
    for (let i = 0; i < surgeCalls.length; i++) {
        const e = surgeCalls[i];
        logFile('  0x' + e.rva.toString(16) + '  armed=' + e.armedN +
                '  expected=' + e.expected + '  ratio=' + e.ratio + 'x');
    }
    logFile('');
    logFile('=== RAW — ' + raw.length + ' ===');
    for (let i = 0; i < raw.length; i++) {
        logFile('  0x' + raw[i].rva.toString(16) +
                '  armed=' + raw[i].armedN + '  baseline=' + raw[i].baseN);
    }
    logFile('=== END ===');
    logBoth('[*] ' + newCalls.length + ' NEW, ' + surgeCalls.length +
            ' SURGE, ' + raw.length + ' raw — see log');
}

// ── Per-frame tick: capture game thread, poll hotkeys, heartbeat. ──
// Actor-state detection is NOT done here — it's done inside the
// FUN_00787EC0 hook below (where the game itself sees dfb=4/5 and
// we're guaranteed to sample at the same moment).
Interceptor.attach(rva(0x471D10), {
    onLeave: function () {
        if (shuttingDown) return;
        if (gameThreadId === null) {
            gameThreadId = Process.getCurrentThreadId();
            logBoth('[*] game thread=' + gameThreadId);
        }
        frameCount++;
        if (keyPressed(VK_F8))  cmdBaseline();
        if (keyPressed(VK_F9))  cmdArm();
        if (keyPressed(VK_F10)) cmdDump();
        if (keyPressed(VK_F11)) cmdReset();

        if (frameCount % 300 === 0) {
            logFile('[HEARTBEAT] frames=' + frameCount + ' state=' + state +
                    ' inBattle=' + inBattle());
        }
    }
});

// ── Dispatcher hook: the REAL actor-state detector. ──
// FUN_00787EC0 is the action dispatcher. It fires every frame and its
// body iterates all 31 actors checking +0xDFB ∈ {4,5} to invoke the
// ATEL script. Sampling actor state INSIDE this hook (onEnter) is
// guaranteed to see the same transient dfb value the dispatcher acts
// on — unlike polling from 0x471D10, which samples at a different
// point in the game loop and can miss transient transitions.
//
// This hook drives the armed state machine: stream-logs every state-
// byte change since the last dispatcher fire, triggers beginCapture
// on any dfb∈{4,5}, triggers endCaptureAuto when the acting slot's
// dfb returns to 0.
Interceptor.attach(rva(0x387EC0), {
    onEnter: function () {
        if (shuttingDown) return;
        if (state !== 'armed_waiting' && state !== 'armed_capturing') return;
        let firstActingSlot = -1;
        for (let i = 0; i < NUM_ACTORS; i++) {
            const a = getActor(i);
            if (!a) continue;
            let df0 = 0, dfb = 0;
            try {
                df0 = a.add(0xDF0).readU8();
                dfb = a.add(0xDFB).readU8();
            } catch (e) { continue; }
            // Track peak df0 (now the canonical action-state byte per
            // session 28). Also log peak dfb in the sanity dump so we
            // still see it if future battles use the dfb path instead.
            if (armedActorPollMax[i] === undefined) {
                armedActorPollMax[i] = { df0: df0, dfb: dfb };
            } else {
                if (df0 > armedActorPollMax[i].df0) armedActorPollMax[i].df0 = df0;
                if (dfb > armedActorPollMax[i].dfb) armedActorPollMax[i].dfb = dfb;
            }
            const pDf0 = prevDf0[i], pDfb = prevDfb[i];
            if (pDf0 === -1 && pDfb === -1) {
                if (df0 !== 0 || dfb !== 0) {
                    logFile('[STATE init] frame=' + frameCount +
                        ' slot=' + i + ' df0=' + df0 + ' dfb=' + dfb);
                }
            } else if (df0 !== pDf0 || dfb !== pDfb) {
                logFile('[STATE] frame=' + frameCount +
                    ' slot=' + i +
                    ' df0=' + pDf0 + '→' + df0 +
                    ' dfb=' + pDfb + '→' + dfb);
            }
            prevDf0[i] = df0;
            prevDfb[i] = dfb;
            // Empirical (session 31): df0=3 is the real action-execute
            // window (animation, banner, script — lasts hundreds of ms).
            // df0=5 is a brief 1-frame finalize marker after the action
            // finishes. For Rikku's normal attack: df0=3 for 57 frames
            // (~950ms) followed by df0=5 for 1 frame then 0. Previous
            // df0=5 predicate only caught the finalize blink.
            if (df0 === 3 && firstActingSlot === -1) {
                firstActingSlot = i;
            }
        }
        if (state === 'armed_waiting' && firstActingSlot !== -1) {
            beginCapture(firstActingSlot);
        } else if (state === 'armed_capturing') {
            // End when the acting actor's df0 returns to 0 (covers the
            // full 3→5→0 lifecycle so we don't stop at the 3→5 blip).
            let stillActing = false;
            try {
                const a = getActor(actingSlot);
                if (a) {
                    const df0 = a.add(0xDF0).readU8();
                    stillActing = (df0 !== 0);
                }
            } catch (e) {}
            const ranTooLong = (frameCount - armedStart) > 900;
            if (!stillActing) {
                endCaptureAuto('acting actor df0 returned to 0');
            } else if (ranTooLong) {
                endCaptureAuto('15s safety timeout — action stuck?');
            }
        }
    }
});

// ── Damage hook: end trigger + ground-truth correlation. ──
// Damage firing means the ability has been dispatched and resolved —
// the banner was rendered, the ability script ran, the target/damage
// were computed. Stopping capture here gives a tight window from
// df0=3-entry to first-damage-fire, excluding post-damage animation
// tail. For basic attacks this is the right end trigger. Multi-hit
// or overdrive abilities are out of scope per user direction.
//
// Secondary: log the acting-actor state panel for ground-truth —
// confirms whether df0=3 really is the mid-action value.
Interceptor.attach(rva(0x39FA20), {
    onEnter: function (args) {
        if (shuttingDown) return;
        if (state !== 'armed_waiting' && state !== 'armed_capturing') return;
        try {
            const target = args[0].toInt32() & 0xFF;
            const dmgType = args[1].toInt32();
            const value = args[2].toInt32();
            const acting = [];
            for (let i = 0; i < NUM_ACTORS; i++) {
                const a = getActor(i);
                if (!a) continue;
                let df0 = 0, dfb = 0;
                try {
                    df0 = a.add(0xDF0).readU8();
                    dfb = a.add(0xDFB).readU8();
                } catch (e) { continue; }
                if (df0 !== 0 || dfb !== 0) {
                    acting.push(i + ':df0=' + df0 + ',dfb=' + dfb);
                }
            }
            logFile('[DMG] frame=' + frameCount + ' target=' + target +
                ' type=' + dmgType + ' value=' + value +
                ' acting=[' + acting.join(' ') + ']');
            // End the capture on first damage during armed_capturing.
            // This is the primary end trigger. df0→0 remains as a
            // fallback for no-damage actions (misses, self-heals with
            // no numeric output, etc.).
            if (state === 'armed_capturing') {
                endCaptureAuto('damage fired — ability complete');
            }
        } catch (e) {}
    }
});

logBoth('[*] Ready. Idle in battle → F8 baseline → F8 stop → F9 arm → trigger any action → auto-dumps.');

// ── Shutdown handler ──
// Frida CLI calls rpc.exports.dispose() on quit (either 'q'+Enter or
// Ctrl+C). We flip the shuttingDown flag (so any in-flight hook
// callbacks bail immediately), stop Stalker if still following, and
// close the log file handle. Frida detaches the Interceptor hooks
// itself after this returns.
rpc.exports = {
    dispose: function () {
        shuttingDown = true;
        // Best-effort Stalker cleanup regardless of state.
        if (gameThreadId !== null) {
            try { Stalker.flush(); } catch (e) {}
            try { Stalker.unfollow(gameThreadId); } catch (e) {}
        }
        // Explicit file close so any buffered writes are committed
        // before Frida tears down the JS runtime.
        try { logFile('[DISPOSE] shutdown'); } catch (e) {}
        try { logHandle.close(); } catch (e) {}
    }
};
