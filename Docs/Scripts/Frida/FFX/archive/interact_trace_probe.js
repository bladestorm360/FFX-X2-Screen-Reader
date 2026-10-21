// @desc Interact-trace probe — captures the Atel handler RVAs called during a
//        known interactable activation (save sphere, chest, NPC, etc.) so we
//        can populate DIALOG/SCENE/SAVE/CHEST_FN_RVAS in production code.
//
// Same baseline-subtract pattern as stalker_event_probe.js: capture the per-
// frame idle background, then arm and perform ONE interaction. The "NEW"
// calls (not seen in baseline) are the action-specific ones. This probe
// adds an extra step on top of that: cross-reference the NEW RVAs against
// the 11 Atel namespace tables (Common, Map, ChEvent, etc.) to identify
// which captured RVAs are Atel-script-callable handlers. The matched
// (namespace, index) pairs are the engine-level function IDs we feed back
// into the production worker classifier.
//
// Controls (game-unused function keys, require game window focus):
//   F8  = baseline toggle. Press while perfectly idle in the target screen.
//          First press starts, second press stops + computes per-frame rates.
//   F9  = arm (requires baseline). Starts fresh accumulator.
//   F10 = stop + dump filtered results (Atel matches first, then raw NEW).
//   F11 = reset everything.
//
// Workflow (salvage ship, save sphere):
//   1. Game running on salvage ship deck. Run probe via run_frida.bat.
//      Wait for "Game thread captured" line (printed when the first frame
//      after attach reaches the FRAME_RVA hook).
//   2. Stand idle near the save sphere (no movement, no camera, no input).
//   3. F8 (baseline started). Stay perfectly still 5-10 seconds.
//   4. F8 (baseline stopped — should be 200+ frames, several thousand calls).
//   5. F9 (armed).
//   6. Press X to activate the save sphere. Save UI should appear.
//   7. F10 (immediately, before dismissing the UI). Dump written to file.
//   8. Send the log.
//
// Output: D:/Games/Dev/Custom/FFX/ffx-x2-screen-reader/Docs/Scripts/Frida/output/FFX/interact_trace.log

const LOG_PATH = 'D:/Games/Dev/Custom/FFX/ffx-x2-screen-reader/Docs/Scripts/Frida/output/FFX/interact_trace.log';
const logHandle = new File(LOG_PATH, 'w');
function logFile(msg) { try { logHandle.write(msg + '\n'); logHandle.flush(); } catch (e) {} }
function logBoth(msg) { console.log(msg); logFile(msg); }

const gameModule = Process.enumerateModules()[0];
const base = gameModule.base;
const moduleEnd = base.add(gameModule.size);
function rva(offset) { return base.add(offset); }

logBoth('[*] Interact-trace probe (baseline-subtracted, Atel cross-referenced)');
logBoth('[*] ' + gameModule.name + ' base=0x' + base.toString(16) + ' size=0x' + gameModule.size.toString(16));

// ── Atel namespace tables (mirrors src/core/addresses.js) ──
// Each entry is 0x10 bytes containing 4 function pointers:
//   field 0 = init, field 1 = exec, field 2 = ret_float, field 3 = ret_int
// Function IDs in script bytecode are encoded as (namespace << 12) | index.
const ATEL_NAMESPACES = [
    { id: 0x0, rva: 0x850050, count: 616, name: 'Common'  },
    { id: 0x1, rva: 0x852BE0, count: 30,  name: 'Math'    },
    { id: 0x4, rva: 0x888D88, count: 71,  name: 'SgEvent' },
    { id: 0x5, rva: 0x8891F8, count: 145, name: 'ChEvent' },
    { id: 0x6, rva: 0x843988, count: 138, name: 'Camera'  },
    { id: 0x7, rva: 0x842618, count: 296, name: 'Battle'  },
    { id: 0x8, rva: 0x85DC90, count: 108, name: 'Map'     },
    { id: 0x9, rva: 0x85D8C0, count: 1,   name: 'Mount'   },
    { id: 0xB, rva: 0x840E20, count: 16,  name: 'Movie'   },
    { id: 0xC, rva: 0x852DD8, count: 94,  name: 'Debug'   },
    { id: 0xD, rva: 0x885EB0, count: 1,   name: 'AbiMap'  }
];
const FIELD_NAMES = ['init', 'exec', 'ret_float', 'ret_int'];

// Map from "relRva" (decimal int as string) -> array of matches:
//   { nsId, nsName, idx, fieldName }
// Built once at probe init by walking every namespace entry.
const atelHandlerMap = {};
let atelHandlerCount = 0;

function buildAtelHandlerMap() {
    for (let n = 0; n < ATEL_NAMESPACES.length; n++) {
        const ns = ATEL_NAMESPACES[n];
        const tableBase = rva(ns.rva);
        for (let i = 0; i < ns.count; i++) {
            const entry = tableBase.add(i * 0x10);
            for (let fo = 0; fo < 4; fo++) {
                try {
                    const fnAddr = entry.add(fo * 4).readPointer();
                    if (fnAddr.isNull()) continue;
                    const relRva = fnAddr.sub(base).toInt32();
                    if (relRva <= 0 || relRva > 0x3000000) continue;
                    const key = '' + relRva;
                    if (!atelHandlerMap[key]) atelHandlerMap[key] = [];
                    atelHandlerMap[key].push({
                        nsId: ns.id, nsName: ns.name, idx: i, fieldName: FIELD_NAMES[fo]
                    });
                    atelHandlerCount++;
                } catch (e) {}
            }
        }
    }
    logBoth('[*] Atel handler map built: ' + atelHandlerCount + ' entries across ' +
        Object.keys(atelHandlerMap).length + ' unique RVAs');
}

// Also dump the full namespace tables to file so the post-trace analysis
// has both halves in one log: what got called + what every Atel slot points
// to. Verbose section, file-only.
function dumpAtelTables() {
    logFile('');
    logFile('=== ATEL NAMESPACE DUMP ===');
    for (let n = 0; n < ATEL_NAMESPACES.length; n++) {
        const ns = ATEL_NAMESPACES[n];
        const tableBase = rva(ns.rva);
        logFile('--- ns=' + ns.name + ' (id=0x' + ns.id.toString(16) +
            ' rva=0x' + ns.rva.toString(16) + ' count=' + ns.count + ') ---');
        for (let i = 0; i < ns.count; i++) {
            const entry = tableBase.add(i * 0x10);
            const ptrs = [];
            for (let fo = 0; fo < 4; fo++) {
                try {
                    const fnAddr = entry.add(fo * 4).readPointer();
                    if (fnAddr.isNull()) { ptrs.push('NULL'); continue; }
                    const relRva = fnAddr.sub(base).toInt32();
                    if (relRva <= 0 || relRva > 0x3000000) {
                        ptrs.push('OOR(' + fnAddr.toString() + ')');
                    } else {
                        ptrs.push('0x' + relRva.toString(16));
                    }
                } catch (e) { ptrs.push('ERR'); }
            }
            const fnId = (ns.id << 12) | i;
            logFile('  idx=' + i + ' fnId=0x' + fnId.toString(16) +
                ' init=' + ptrs[0] + ' exec=' + ptrs[1] +
                ' retF=' + ptrs[2] + ' retI=' + ptrs[3]);
        }
    }
    logFile('=== END NAMESPACE DUMP ===');
}

// ── Keyboard (standard GetAsyncKeyState pattern, copied from stalker_event_probe.js) ──
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

// ── Stalker state machine (mirrors stalker_event_probe.js) ──
let state = 'idle';
let gameThreadId = null;

let baselineCounts = {};
let baselineFrames = 0;
let baselineRates = null;
let baselineStartFrame = 0;

let armedCounts = {};
let armedFrames = 0;
let armedStartFrame = 0;

let frameCount = 0;
const FRAME_RVA = 0x471D10;  // proven per-frame anchor (stalker_event_probe.js)

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
        baselineRates = {};
        for (const addr in baselineCounts) {
            baselineRates[addr] = baselineCounts[addr] / baselineFrames;
        }
        state = 'idle';
        logBoth('[BASELINE STOPPED] ' + baselineFrames + ' frames, ' +
                Object.keys(baselineCounts).length + ' distinct call targets. Ready to arm with F9.');
    } else if (state === 'idle' && baselineRates !== null) {
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
        logBoth('[ARMED] @ frame ' + frameCount + ' — perform the interaction, F10 to stop');
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
// For each RVA captured during arming, classify:
//   NEW   = baseline had zero hits for this RVA (strongest signal)
//   SURGE = armed rate >= 3x baseline rate AND delta >= 3 (interesting signal)
// Then for each category, separately list any Atel handler matches.
function dumpFiltered() {
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

        if (baseN === 0) {
            newCalls.push({ rva: rvaOff, armedN: armedN });
        } else if (armedN >= 3 && expected > 0 && (armedN / expected) >= 3) {
            surgeCalls.push({
                rva: rvaOff, armedN: armedN, expected: expected.toFixed(2),
                ratio: (armedN / expected).toFixed(2),
                delta: (armedN - expected).toFixed(2)
            });
        }
    }

    newCalls.sort(function(a, b) { return a.armedN - b.armedN; });
    surgeCalls.sort(function(a, b) { return parseFloat(b.delta) - parseFloat(a.delta); });

    // Cross-reference each NEW and SURGE entry against the Atel handler map.
    function annotateAtel(rvaOff) {
        const matches = atelHandlerMap['' + rvaOff];
        if (!matches || matches.length === 0) return '';
        return ' [ATEL: ' + matches.map(function(m) {
            const fnId = (m.nsId << 12) | m.idx;
            return m.nsName + '.' + m.fieldName + '#' + m.idx + ' (fnId=0x' + fnId.toString(16) + ')';
        }).join(' | ') + ']';
    }

    function isAtel(rvaOff) {
        const matches = atelHandlerMap['' + rvaOff];
        return matches && matches.length > 0;
    }

    logFile('');
    logFile('=== SECTION 1: ATEL HANDLERS in NEW (zero baseline hits) ===');
    logFile('=== Strongest signal — these script handlers fired during the action.');
    logFile('=== Cross-reference against the namespace dump below for full table context.');
    let atelNewCount = 0;
    for (let i = 0; i < newCalls.length; i++) {
        if (!isAtel(newCalls[i].rva)) continue;
        atelNewCount++;
        logFile('  0x' + newCalls[i].rva.toString(16) + '  armed=' + newCalls[i].armedN +
            annotateAtel(newCalls[i].rva));
    }
    logFile('  (' + atelNewCount + ' Atel handlers in NEW)');

    logFile('');
    logFile('=== SECTION 2: ATEL HANDLERS in SURGE (rate >> baseline) ===');
    let atelSurgeCount = 0;
    for (let i = 0; i < surgeCalls.length; i++) {
        if (!isAtel(surgeCalls[i].rva)) continue;
        atelSurgeCount++;
        const e = surgeCalls[i];
        logFile('  0x' + e.rva.toString(16) + '  armed=' + e.armedN + '  expected=' + e.expected +
            '  ratio=' + e.ratio + 'x  delta=' + e.delta + annotateAtel(e.rva));
    }
    logFile('  (' + atelSurgeCount + ' Atel handlers in SURGE)');

    logFile('');
    logFile('=== SECTION 3: ALL NEW (file-only, including non-Atel) — ' + newCalls.length + ' ===');
    for (let i = 0; i < newCalls.length; i++) {
        logFile('  0x' + newCalls[i].rva.toString(16) + '  armed=' + newCalls[i].armedN +
            annotateAtel(newCalls[i].rva));
    }

    logFile('');
    logFile('=== SECTION 4: ALL SURGE (file-only, including non-Atel) — ' + surgeCalls.length + ' ===');
    for (let i = 0; i < surgeCalls.length; i++) {
        const e = surgeCalls[i];
        logFile('  0x' + e.rva.toString(16) + '  armed=' + e.armedN + '  expected=' + e.expected +
            '  ratio=' + e.ratio + 'x  delta=' + e.delta + annotateAtel(e.rva));
    }

    logFile('');
    logFile('=== SECTION 5: RAW armed dump — ' + armedRvaCount + ' distinct targets ===');
    const raw = [];
    for (const addr in armedCounts) {
        const absPtr = ptr(addr);
        if (absPtr.compare(base) < 0 || absPtr.compare(moduleEnd) >= 0) continue;
        raw.push({ rva: absPtr.sub(base).toInt32(), armedN: armedCounts[addr],
                   baseN: baselineCounts[addr] || 0 });
    }
    raw.sort(function(a, b) { return a.armedN - b.armedN; });
    for (let i = 0; i < raw.length; i++) {
        logFile('  0x' + raw[i].rva.toString(16) + '  armed=' + raw[i].armedN +
            '  baseline=' + raw[i].baseN + annotateAtel(raw[i].rva));
    }

    // Namespace dump appended once per dump cycle so analysis is self-contained.
    dumpAtelTables();

    logFile('=== END ===');

    logBoth('[*] Dumped. Atel-NEW: ' + atelNewCount + '. Atel-SURGE: ' + atelSurgeCount +
            '. Total NEW: ' + newCalls.length + '. SURGE: ' + surgeCalls.length +
            '. RAW: ' + armedRvaCount + '. See ' + LOG_PATH);
}

// ── Per-frame hotkey poll + thread capture ──
Interceptor.attach(rva(FRAME_RVA), {
    onLeave: function() {
        if (gameThreadId === null) {
            gameThreadId = Process.getCurrentThreadId();
            logBoth('[*] Game thread captured: tid=' + gameThreadId);
            // Build the Atel handler lookup AFTER thread capture so we know
            // the module is fully loaded and the namespace tables are populated.
            try { buildAtelHandlerMap(); }
            catch (e) { logBoth('[!] Atel map build failed: ' + e.message); }
        }
        frameCount++;
        if (keyPressed(VK_F8))  cmdBaseline();
        if (keyPressed(VK_F9))  cmdArm();
        if (keyPressed(VK_F10)) cmdDump();
        if (keyPressed(VK_F11)) cmdReset();
    }
});

logBoth('[*] Ready. Workflow: F8 (baseline start, idle) -> F8 (baseline stop) -> F9 (arm) -> press X on interactable -> F10 (dump). F11 = reset.');
