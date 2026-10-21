// @desc Stalker-based event-discovery probe with baseline subtraction.
//
// Problem with raw Stalker call-counts: at any moment the game is firing
// hundreds of background calls per frame (render, audio, animation,
// per-frame update tick). A 20-frame armed window captures ~1000 distinct
// call targets, of which only ~10 are action-related. Un-sortable by eye.
//
// Fix: record an idle baseline in the SAME screen context first. For each
// call RVA, we know its baseline calls-per-frame rate. When armed, the
// "expected" count for that RVA during the armed window is rate × frames.
// Report only RVAs where armed_count substantially exceeds expected — the
// action-specific signal, with per-frame background noise cancelled out.
//
// Controls (game-unused function keys, require game window focus):
//   F8  = baseline toggle (first press starts, second press stops +
//          computes rates). Do this WHILE IDLE in the target screen.
//   F9  = arm (requires baseline computed). Starts fresh accumulator.
//   F10 = stop + dump filtered results:
//           Section 1: calls NEW to armed (baseline count == 0)
//           Section 2: calls with significant rate increase
//           Section 3: raw armed dump (for manual review)
//   F11 = reset everything (discard baseline + armed state)
//
// Intended workflow per event:
//   1. Launch game, run probe, wait for "Game thread captured" line.
//   2. Get to the target screen, stay perfectly idle (no camera, no
//      movement, no keypresses).
//   3. F8 — "BASELINE STARTED". Stay idle 5-10 seconds.
//   4. F8 — "BASELINE STOPPED. N frames, M distinct calls."
//   5. F9 — "ARMED".
//   6. Perform the ONE action (one cursor press, one window open).
//   7. F10 — dumps filtered results.
//   8. (F11 if you want to redo the baseline for a different screen.)
//
// Output: Docs/Scripts/Frida/output/FFX/stalker_event_probe.log

const LOG_PATH = 'D:/Games/Dev/Custom/FFX/ffx-x2-screen-reader/Docs/Scripts/Frida/output/FFX/stalker_event_probe.log';
const logHandle = new File(LOG_PATH, 'w');
function logFile(msg) { try { logHandle.write(msg + '\n'); logHandle.flush(); } catch (e) {} }
function logBoth(msg) { console.log(msg); logFile(msg); }

const gameModule = Process.enumerateModules()[0];
const base = gameModule.base;
const moduleEnd = base.add(gameModule.size);
function rva(offset) { return base.add(offset); }

logBoth('[*] Stalker event-discovery probe (baseline-subtracted)');
logBoth('[*] ' + gameModule.name + ' base=0x' + base.toString(16) + ' size=0x' + gameModule.size.toString(16));

// ── Keyboard (standard GetAsyncKeyState pattern) ──
let fnGetAsyncKeyState = null, fnGetForegroundWindow = null, fnGetWindowThreadProcessId = null;
const myPid = Process.id;
const pidBuf = Memory.alloc(4);
const keyWasDown = {};

Process.enumerateModules().forEach(function(m) {
    if (m.name.toLowerCase() === 'user32.dll') {
        m.enumerateExports().forEach(function(exp) {
            if (exp.name === 'GetAsyncKeyState')
                fnGetAsyncKeyState = new NativeFunction(exp.address, 'int16', ['int']);
            if (exp.name === 'GetForegroundWindow')
                fnGetForegroundWindow = new NativeFunction(exp.address, 'pointer', []);
            if (exp.name === 'GetWindowThreadProcessId')
                fnGetWindowThreadProcessId = new NativeFunction(exp.address, 'uint32', ['pointer', 'pointer']);
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

const VK_F8  = 0x77;
const VK_F9  = 0x78;
const VK_F10 = 0x79;
const VK_F11 = 0x7A;

// ── Stalker state machine ──
//   STATE: 'idle' -> 'baseline' -> 'idle' -> 'armed' -> 'idle'
let state = 'idle';
let gameThreadId = null;

let baselineCounts = {};     // rva string -> count
let baselineFrames = 0;
let baselineRates = null;    // rva string -> per-frame rate (computed on stop)
let baselineStartFrame = 0;

let armedCounts = {};        // rva string -> count
let armedFrames = 0;
let armedStartFrame = 0;

let frameCount = 0;
const FRAME_RVA = 0x471D10;

function excludeNonGameModules() {
    let count = 0;
    Process.enumerateModules().forEach(function(m) {
        if (m.name.toLowerCase() === gameModule.name.toLowerCase()) return;
        try { Stalker.exclude({ base: m.base, size: m.size }); count++; }
        catch (e) {}
    });
    logFile('[*] excluded ' + count + ' non-game modules');
}
let excludedOnce = false;

function startStalker(targetMap) {
    if (!excludedOnce) { excludeNonGameModules(); excludedOnce = true; }
    Stalker.follow(gameThreadId, {
        events: { call: true },
        onCallSummary: function(summary) {
            for (const addr in summary) {
                targetMap[addr] = (targetMap[addr] || 0) + summary[addr];
            }
        }
    });
}

function stopStalker() {
    try { Stalker.unfollow(gameThreadId); } catch (e) {}
}

// ── Control ──
function cmdBaseline() {
    if (gameThreadId === null) { logBoth('[!] Game thread not captured yet'); return; }
    if (state === 'armed') { logBoth('[!] Currently armed — press F10 first'); return; }
    if (state === 'idle' && baselineRates === null) {
        // Start baseline
        baselineCounts = {};
        baselineStartFrame = frameCount;
        state = 'baseline';
        try {
            startStalker(baselineCounts);
            logBoth('[BASELINE STARTED] @ frame ' + frameCount + ' — stay perfectly idle, F8 again to stop');
        } catch (e) { logBoth('[!] Stalker.follow failed: ' + e.message); state = 'idle'; }
    } else if (state === 'baseline') {
        stopStalker();
        baselineFrames = frameCount - baselineStartFrame;
        if (baselineFrames < 10) {
            logBoth('[!] Baseline too short (' + baselineFrames + ' frames) — try again');
            baselineCounts = {};
            state = 'idle';
            return;
        }
        // Compute per-frame rates
        baselineRates = {};
        for (const addr in baselineCounts) {
            baselineRates[addr] = baselineCounts[addr] / baselineFrames;
        }
        state = 'idle';
        logBoth('[BASELINE STOPPED] ' + baselineFrames + ' frames, ' +
                Object.keys(baselineCounts).length + ' distinct call targets. Ready to arm with F9.');
    } else if (state === 'idle' && baselineRates !== null) {
        // Restart baseline (discard previous)
        baselineRates = null;
        baselineCounts = {};
        cmdBaseline();
    }
}

function cmdArm() {
    if (gameThreadId === null) { logBoth('[!] Game thread not captured yet'); return; }
    if (baselineRates === null) { logBoth('[!] No baseline — press F8 first'); return; }
    if (state !== 'idle') { logBoth('[!] Not idle (state=' + state + ')'); return; }
    armedCounts = {};
    armedStartFrame = frameCount;
    state = 'armed';
    try {
        startStalker(armedCounts);
        logBoth('[ARMED] @ frame ' + frameCount + ' — perform the action, F10 to stop');
    } catch (e) { logBoth('[!] Stalker.follow failed: ' + e.message); state = 'idle'; }
}

function cmdDump() {
    if (state !== 'armed') { logBoth('[!] Not armed'); return; }
    stopStalker();
    armedFrames = frameCount - armedStartFrame;
    state = 'idle';
    logBoth('[DUMP] armed=' + armedFrames + ' frames, baseline=' + baselineFrames + ' frames');
    dumpFiltered();
}

function cmdReset() {
    if (state !== 'idle') { stopStalker(); state = 'idle'; }
    baselineCounts = {}; baselineRates = null; baselineFrames = 0;
    armedCounts = {}; armedFrames = 0;
    logBoth('[RESET]');
}

// ── Filtering & dump ──
function dumpFiltered() {
    // Classify each armed call target:
    //   NEW: baseline had zero calls for this RVA. Strongest signal.
    //   SURGE: armed rate > 3x baseline rate AND delta >= 3.
    //   Everything else is noise.
    const newCalls = [];
    const surgeCalls = [];
    const armedRvaCount = Object.keys(armedCounts).length;

    for (const addr in armedCounts) {
        const absPtr = ptr(addr);
        if (absPtr.compare(base) < 0 || absPtr.compare(moduleEnd) >= 0) continue;
        const rvaOff = absPtr.sub(base).toInt32();
        const armedN = armedCounts[addr];
        const baseN = baselineCounts[addr] || 0;
        const baseRate = baselineRates[addr] || 0;
        const expected = baseRate * armedFrames;
        const delta = armedN - expected;

        if (baseN === 0) {
            newCalls.push({ rva: rvaOff, armedN: armedN });
        } else if (armedN >= 3 && expected > 0 && (armedN / expected) >= 3) {
            surgeCalls.push({
                rva: rvaOff, armedN: armedN, expected: expected.toFixed(2),
                ratio: (armedN / expected).toFixed(2), delta: delta.toFixed(2)
            });
        }
    }

    newCalls.sort(function(a, b) { return a.armedN - b.armedN; });
    surgeCalls.sort(function(a, b) { return b.delta - a.delta; });

    logFile('');
    logFile('=== SECTION 1: NEW to armed (zero baseline hits) — ' + newCalls.length + ' ===');
    logFile('=== Strongest candidates. Low-count entries first.');
    for (let i = 0; i < newCalls.length; i++) {
        logFile('  0x' + newCalls[i].rva.toString(16) + '  armed=' + newCalls[i].armedN);
    }

    logFile('');
    logFile('=== SECTION 2: SURGE (armed rate >> baseline rate) — ' + surgeCalls.length + ' ===');
    logFile('=== Sorted by delta descending.');
    for (let i = 0; i < surgeCalls.length; i++) {
        const e = surgeCalls[i];
        logFile('  0x' + e.rva.toString(16) + '  armed=' + e.armedN + '  expected=' + e.expected +
                '  ratio=' + e.ratio + 'x  delta=' + e.delta);
    }

    logFile('');
    logFile('=== SECTION 3: RAW armed dump — ' + armedRvaCount + ' distinct targets ===');
    logFile('=== Sorted by armed count ascending.');
    const raw = [];
    for (const addr in armedCounts) {
        const absPtr = ptr(addr);
        if (absPtr.compare(base) < 0 || absPtr.compare(moduleEnd) >= 0) continue;
        raw.push({ rva: absPtr.sub(base).toInt32(), armedN: armedCounts[addr],
                   baseN: baselineCounts[addr] || 0 });
    }
    raw.sort(function(a, b) { return a.armedN - b.armedN; });
    for (let i = 0; i < raw.length; i++) {
        logFile('  0x' + raw[i].rva.toString(16) + '  armed=' + raw[i].armedN + '  baseline=' + raw[i].baseN);
    }
    logFile('=== END ===');

    logBoth('[*] Dumped. Section 1 (NEW): ' + newCalls.length +
            '. Section 2 (SURGE): ' + surgeCalls.length +
            '. Section 3 (RAW): ' + armedRvaCount + '. See ' + LOG_PATH);
}

// ── Per-frame hotkey poll + thread capture ──
Interceptor.attach(rva(FRAME_RVA), {
    onLeave: function() {
        if (gameThreadId === null) {
            gameThreadId = Process.getCurrentThreadId();
            logBoth('[*] Game thread captured: tid=' + gameThreadId);
        }
        frameCount++;
        if (keyPressed(VK_F8))  cmdBaseline();
        if (keyPressed(VK_F9))  cmdArm();
        if (keyPressed(VK_F10)) cmdDump();
        if (keyPressed(VK_F11)) cmdReset();
    }
});

logBoth('[*] Ready. Workflow: F8 (baseline start) -> idle 5-10s -> F8 (baseline stop) -> F9 (arm) -> action -> F10 (dump). F11 = reset.');
