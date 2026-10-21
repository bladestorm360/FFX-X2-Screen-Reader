// @desc Find the "map load complete" event via auto-armed Stalker +
//       multi-run cross-correlation.
//
// WHY THIS IS HARD:
//  - Map transitions fire hundreds of teardown + init events across a ~1-2s
//    window. "Map finished loading" is buried in there.
//  - Naive baseline/arm/dump produces ~500+ NEW calls from a single run --
//    unrankable.
//  - Noise sources: OnPlayerMove animations/sounds (player is walking
//    during the action), ambient audio, particle updates, FMV wake-up,
//    map-specific NPC inits.
//
// STRATEGY (this probe):
//  1. One baseline captured while the player walks around the CURRENT map
//     (NOT stationary) -- captures OnPlayerMove noise + ambient so those
//     don't pollute the NEW list during armed runs.
//  2. Auto-arm Stalker the moment AtelSetEventJump2 (0x46FED0) fires --
//     known transition-start anchor.
//  3. Auto-dump when the actor array at 0x1FC44E4 flips from "invalid"
//     (NaN / zero position) to "valid." That's when the new map's actors
//     are populated -- the event we're hunting for.
//  4. Support multiple runs per session. Each run gets its own
//     [RUN N] section in the log. User transitions through several
//     different exits (different map types), Ctrl+C when done.
//  5. Offline cross-correlation (Claude does this): the RVAs that appear
//     in every run's NEW section are the canonical map-load code, not
//     map-specific content. Map-load-complete is in that intersection.
//
// USAGE:
//  1. Launch game, attach probe.
//  2. Walk into a field area (not the world map, not a battle).
//  3. F8 -- starts baseline. Walk around THE CURRENT MAP for 5-10s.
//     Do NOT cross an exit yet. Just wander.
//  4. F8 -- stops baseline. Probe is now armed-on-transition.
//  5. Walk through an exit. Probe auto-arms on AtelSetEventJump2, traces
//     until actors populate, auto-dumps as [RUN 1]. Speech may be silent
//     during the trace.
//  6. Walk through another exit (different map type ideally -- e.g.
//     outdoor -> indoor, or field -> battle-arena). [RUN 2] fires.
//  7. Repeat for 3-5 transitions. More runs = tighter intersection.
//  8. Ctrl+C the probe. Send the log to claude.
//
// HOTKEYS:
//   F8  = start/stop baseline (two presses)
//   F11 = reset everything (discard baseline + all run data, rearm)
//
// OUTPUT:
//   Docs/Scripts/Frida/output/FFX/map_load_complete_probe.log
//   Multiple [RUN N] sections, each with its own NEW list. Claude
//   intersects them offline to find the map-load-complete hook.

const LOG_PATH = 'D:/Games/Dev/Custom/FFX/ffx-x2-screen-reader/Docs/Scripts/Frida/output/FFX/map_load_complete_probe.log';
const logHandle = new File(LOG_PATH, 'w');
function logFile(msg) { try { logHandle.write(msg + '\n'); logHandle.flush(); } catch (e) {} }
function logBoth(msg) { console.log(msg); logFile(msg); }

let gameModule = null;
Process.enumerateModules().forEach(function(m) { if (m.name.toLowerCase() === 'ffx.exe') gameModule = m; });
if (!gameModule) gameModule = Process.enumerateModules()[0];
const base = gameModule.base;
const moduleEnd = base.add(gameModule.size);
function rva(offset) { return base.add(offset); }

logBoth('[*] Map-load-complete probe (auto-armed, multi-run)');
logBoth('[*] ' + gameModule.name + ' base=0x' + base.toString(16));

// ── Keyboard ──
let fnGetAsyncKeyState = null, fnGetForegroundWindow = null, fnGetWindowThreadProcessId = null;
const myPid = Process.id;
const pidBuf = Memory.alloc(4);
const keyWasDown = {};
Process.enumerateModules().forEach(function(m) {
    if (m.name.toLowerCase() === 'user32.dll') {
        m.enumerateExports().forEach(function(exp) {
            if (exp.name === 'GetAsyncKeyState') fnGetAsyncKeyState = new NativeFunction(exp.address, 'int16', ['int']);
            if (exp.name === 'GetForegroundWindow') fnGetForegroundWindow = new NativeFunction(exp.address, 'pointer', []);
            if (exp.name === 'GetWindowThreadProcessId') fnGetWindowThreadProcessId = new NativeFunction(exp.address, 'uint32', ['pointer', 'pointer']);
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
const VK_F8 = 0x77, VK_F11 = 0x7A;

// ── Actor array sampler (how we know "map load complete") ──
const ACTOR_ARRAY_RVA = 0x1FC44E4;
function actorsValid() {
    try {
        const ab = rva(ACTOR_ARRAY_RVA).readPointer();
        if (ab.isNull() || ab.compare(ptr(0x10000)) <= 0) return false;
        const x = ab.add(0x0C).readFloat();
        return !isNaN(x) && x !== 0;
    } catch (e) { return false; }
}

// ── Stalker management ──
let gameThreadId = null;
let excludedOnce = false;
function excludeNonGameModules() {
    Process.enumerateModules().forEach(function(m) {
        if (m.name.toLowerCase() === gameModule.name.toLowerCase()) return;
        try { Stalker.exclude({ base: m.base, size: m.size }); } catch (e) {}
    });
}
function startStalker(target) {
    if (!excludedOnce) { excludeNonGameModules(); excludedOnce = true; }
    Stalker.follow(gameThreadId, {
        events: { call: true },
        onCallSummary: function(summary) {
            for (const a in summary) target[a] = (target[a] || 0) + summary[a];
        }
    });
}
function stopStalker() { try { Stalker.unfollow(gameThreadId); } catch (e) {} }

// ── State machine ──
// 'idle'        -> waiting for user to start baseline
// 'baseline'    -> capturing idle walking call rate
// 'armed-idle'  -> baseline done, waiting for transition to auto-fire
// 'armed-trace' -> transition fired, tracing until actors valid
let state = 'idle';
let baselineCounts = {}, baselineRates = null, baselineFrames = 0, baselineStart = 0;
let armedCounts = {}, armedStart = 0, armedFrameCount = 0;
let runNumber = 0;
let frameCount = 0;
// Minimum trace duration. AtelSetEventJump2 fires at transition START,
// but the previous map's actor array is still in memory for a few frames
// after that (game hasn't torn it down yet). Without this, actorsValid()
// returns true immediately and the probe dumps after 1 frame with empty
// data. 30 frames matches the nav-module's own pre-check window in
// navigation/index.js::onTick -- proven reliable in production.
const MIN_TRACE_FRAMES = 30;
const TRACE_TIMEOUT_FRAMES = 600; // ~10s at 60fps -- abort if actors never show up

function cmdBaselineToggle() {
    if (gameThreadId === null) return;
    if (state === 'idle' && baselineRates === null) {
        baselineCounts = {}; baselineStart = frameCount; state = 'baseline';
        try { startStalker(baselineCounts); logBoth('[BASELINE STARTED] walk around the current map 5-10s, F8 to stop'); }
        catch (e) { logBoth('[!] follow failed: ' + e.message); state = 'idle'; }
    } else if (state === 'baseline') {
        stopStalker();
        baselineFrames = frameCount - baselineStart;
        if (baselineFrames < 30) {
            logBoth('[!] baseline too short (' + baselineFrames + ' frames) -- restart');
            baselineCounts = {}; state = 'idle';
            return;
        }
        baselineRates = {};
        for (const a in baselineCounts) baselineRates[a] = baselineCounts[a] / baselineFrames;
        state = 'armed-idle';
        logBoth('[BASELINE STOPPED] ' + baselineFrames + ' frames, ' +
                Object.keys(baselineCounts).length + ' call targets');
        logBoth('[*] Armed for auto-trace. Walk through an exit. Each transition produces a [RUN N] section.');
    } else {
        logBoth('[!] F8 ignored in state=' + state + ' (F11 to reset)');
    }
}

function cmdReset() {
    if (state === 'baseline' || state === 'armed-trace') stopStalker();
    state = 'idle';
    baselineCounts = {}; baselineRates = null; baselineFrames = 0;
    armedCounts = {}; armedFrameCount = 0;
    runNumber = 0;
    logBoth('[RESET]');
}

// Auto-arm on AtelSetEventJump2 -- the transition-start anchor.
try {
    Interceptor.attach(rva(0x46FED0), {
        onEnter: function(args) {
            if (state !== 'armed-idle') return;
            armedCounts = {}; armedStart = frameCount; armedFrameCount = 0;
            state = 'armed-trace';
            try { startStalker(armedCounts); }
            catch (e) { logBoth('[!] auto-arm follow failed: ' + e.message); state = 'armed-idle'; }
            const room = args[0].toInt32();
            logFile('');
            logFile('[RUN ' + (runNumber + 1) + ' START] frame=' + frameCount + ' room=0x' + room.toString(16));
        }
    });
    logBoth('[*] AtelSetEventJump2 transition hook armed');
} catch (e) { logBoth('[!] transition hook failed: ' + e.message); }

// Auto-dump when actors become valid (or timeout).
function autoDumpRun(reason) {
    stopStalker();
    armedFrameCount = frameCount - armedStart;
    state = 'armed-idle';
    runNumber++;

    // Compute NEW (baseline=0) and SURGE (rate >=3x baseline).
    const newCalls = [], surgeCalls = [];
    for (const a in armedCounts) {
        const p = ptr(a);
        if (p.compare(base) < 0 || p.compare(moduleEnd) >= 0) continue;
        const rvaOff = p.sub(base).toInt32();
        const armedN = armedCounts[a];
        const baseN = baselineCounts[a] || 0;
        const baseRate = baselineRates[a] || 0;
        const expected = baseRate * armedFrameCount;
        if (baseN === 0) newCalls.push({ rva: rvaOff, armedN });
        else if (armedN >= 3 && expected > 0 && (armedN / expected) >= 3) {
            surgeCalls.push({ rva: rvaOff, armedN, expected: expected.toFixed(2), ratio: (armedN / expected).toFixed(2) });
        }
    }
    newCalls.sort(function(a, b) { return a.armedN - b.armedN; });
    surgeCalls.sort(function(a, b) { return b.armedN - a.armedN; });

    logFile('[RUN ' + runNumber + ' END] reason=' + reason + ' frames=' + armedFrameCount);
    logFile('[RUN ' + runNumber + ' NEW] count=' + newCalls.length);
    for (let i = 0; i < newCalls.length; i++) logFile('  0x' + newCalls[i].rva.toString(16) + '  armed=' + newCalls[i].armedN);
    logFile('[RUN ' + runNumber + ' SURGE] count=' + surgeCalls.length);
    for (let i = 0; i < surgeCalls.length; i++) {
        const e = surgeCalls[i];
        logFile('  0x' + e.rva.toString(16) + '  armed=' + e.armedN + '  expected=' + e.expected + '  ratio=' + e.ratio + 'x');
    }
    logBoth('[RUN ' + runNumber + '] complete (' + reason + '): ' + newCalls.length + ' NEW, ' + surgeCalls.length + ' SURGE');
}

// Per-frame driver: keyboard polling + actor-valid check.
const FRAME_RVA = 0x471D10;
Interceptor.attach(rva(FRAME_RVA), {
    onLeave: function() {
        if (gameThreadId === null) {
            gameThreadId = Process.getCurrentThreadId();
            logBoth('[*] game thread=' + gameThreadId);
        }
        frameCount++;
        if (keyPressed(VK_F8))  cmdBaselineToggle();
        if (keyPressed(VK_F11)) cmdReset();

        if (state === 'armed-trace') {
            armedFrameCount = frameCount - armedStart;
            // Don't check actor validity until we've traced a minimum
            // window -- the old map's actors are still valid at the
            // moment AtelSetEventJump2 fires. We want to capture the
            // teardown + reload, which happens across many frames.
            if (armedFrameCount >= MIN_TRACE_FRAMES && actorsValid()) {
                autoDumpRun('actors-valid');
            } else if (armedFrameCount > TRACE_TIMEOUT_FRAMES) {
                autoDumpRun('timeout');
            }
        }
    }
});

logBoth('[*] Ready. Get to a field map. F8 to start baseline, F8 to stop, then walk through exits.');
