// @desc Hook MsGetComData + MsMenuGetText; diff baseline vs armed to find the enemy-action banner caller.
//
// The hypothesis this probe tests: when an enemy uses a named ability in
// battle, somewhere between "df0 enters 3" (action-exec window opens) and
// "damage fires" (0x39FA20), the game resolves the ability's command text
// to render the on-screen banner. That resolve goes through one of two
// universal battle-text functions:
//
//   MsGetComData   (FUN_0079a4c0, RVA 0x39A4C0) — the real battle-command
//                  data resolver. 207 bytes, fully read; prefix-dispatches
//                  on (cmdId >> 12) to per-type getters (player ability,
//                  monster ability, item, etc.). Every TOBtlDraw*Message
//                  function I surveyed calls this to resolve text.
//
//   MsMenuGetText  (FUN_0078fd40, RVA 0x38FD40) — the menu-text dispatcher.
//                  Already hooked in production for battle rewards
//                  (arg0=4). Name says "Menu" but call sites pass arg0=1
//                  for ability text. Included as a control: if the banner
//                  uses this path instead, we'll see it fire during the
//                  armed window.
//
// Five prior sessions (20-36) hooked functions chosen by name-guess and
// rationalized mismatches. This probe does NOT guess. It hooks the data
// layer, collects (cmdId, caller_RA, decoded_text) for every resolve
// during baseline and armed windows, then diffs. Callers present in
// armed but not baseline, with decoded text matching the ability the
// user executed, are the banner site.
//
// Pre-flight:
//   1. Pre-battle save → load → enter battle.
//   2. Reach an idle moment (no animation, CTB waiting).
//   3. run_frida.bat → 1 (FFX) → action_text_diff_probe.
//
// Hotkeys:
//   F8   baseline toggle.
//        Press 1: start recording (cmdId, caller) counts into baseline.
//        Stay idle ~5-10s. CTB scrolling and cursor moves are fine.
//        Press 2: stop baseline. Per-frame rate per (caller, cmdId) is
//        cached for comparison.
//   F9   arm. Resets armed counters. State becomes armed_waiting. The
//        dispatcher hook at 0x387EC0 watches for any actor's df0 to
//        enter 3 (session 32 established this as the exec window start).
//        When it does, state flips to armed_capturing.
//   F10  manual dump. Works from either armed_waiting or armed_capturing.
//   F11  reset. Clears all state.
//
// Auto-end: on first damage fire (0x39FA20) during armed_capturing, OR
// on acting actor's df0 returning to 0 (no-damage actions like misses).
//
// Success criteria in the log:
//   [STATE] lines show df0 0→3 transition for some slot during armed.
//   [COM] or [MGT] lines appear during [CAPTURE START]..[CAPTURE END].
//   The [DIFF] section lists caller RVAs present in armed but not base.
//   For the ability the user executed, one caller + cmdId with decoded
//   text matching the ability name = the banner site.
//
// Output: Docs/Scripts/Frida/output/FFX/action_text_diff_probe.log

const LOG_PATH = 'D:/Games/Dev/Custom/FFX/ffx-x2-screen-reader/Docs/Scripts/Frida/output/FFX/action_text_diff_probe.log';
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

logBoth('[*] action_text_diff_probe  module=' + gameModule.name +
        '  base=0x' + base.toString(16));

// ── RVAs ──
const RVA_MS_GET_COM_DATA   = 0x39A4C0;   // MsGetComData — battle command data resolver
const RVA_MS_MENU_GET_TEXT  = 0x38FD40;   // MsMenuGetText — menu text dispatcher
const RVA_DISPATCHER        = 0x387EC0;   // FUN_00787EC0 — per-frame Atel script dispatcher
const RVA_DAMAGE            = 0x39FA20;   // damage display — end trigger
const RVA_FRAME_TICK        = 0x471D10;   // per-frame hook (for hotkeys + heartbeat)
const RVA_BTL_STATE         = 0xD2A8E0;   // battle-active byte
const RVA_ALLY_PTR          = 0xD334CC;   // unified actor array pointer
const ALLY_STRIDE           = 0xF90;
const NUM_ACTORS            = 0x1f;       // 31 slots

// ── AOB sanity: log prologue bytes so we can verify against the decompile ──
function logPrologue(label, rvaOff, n) {
    try {
        const bytes = [];
        for (let i = 0; i < n; i++) {
            bytes.push(('0' + rva(rvaOff).add(i).readU8().toString(16)).slice(-2));
        }
        logFile('[AOB] ' + label + ' @0x' + rvaOff.toString(16) + ' bytes: ' + bytes.join(' '));
    } catch (e) {
        logBoth('[!] AOB read failed for ' + label + ': ' + e.message);
    }
}
logPrologue('MsGetComData  ', RVA_MS_GET_COM_DATA, 24);
logPrologue('MsMenuGetText ', RVA_MS_MENU_GET_TEXT, 24);
logPrologue('Dispatcher    ', RVA_DISPATCHER, 24);
logPrologue('DamageDisplay ', RVA_DAMAGE, 24);

// ── Battle-state + actor-state ──
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

// ── FFX charset decoder (inlined for standalone probe) ──
// Mirrors src/core/charset.js:decodeFFXString but simpler — just strip to
// printable output with a small max-length, for caller identification.
const CHARSET = '0123456789 !"#$%&\'()*+,-./:;<=>?ABCDEFGHIJKLMNOPQRSTUVWXYZ[\\]^_‘abcdefghijklmnopqrstuvwxyz{|}~';
function decodeFFXTextBytes(addr, maxLen) {
    if (!addr || addr.isNull()) return '';
    if (maxLen <= 0 || maxLen > 256) maxLen = 64;
    let raw;
    try { raw = addr.readByteArray(maxLen); }
    catch (e) { return '<unreadable>'; }
    if (!raw) return '<null>';
    const bytes = new Uint8Array(raw);
    let result = '';
    let i = 0;
    while (i < maxLen) {
        let b = bytes[i];
        if (b === 0x00) break;
        if (b === 0x03 || b === 0x0A) { result += ' '; i++; continue; }
        if (b === 0x09 || b === 0x10 || b === 0x13) { i += 2; continue; }
        if (b < 0x2B) { i++; continue; }
        let idx = 0;
        if (b < 0x30) {
            idx = (b - 0x2B) * 0xD0;
            i++;
            if (i >= maxLen) break;
            b = bytes[i];
        }
        idx += (b - 0x30);
        if (idx >= 0 && idx < CHARSET.length) result += CHARSET[idx];
        i++;
    }
    return result.trim();
}

// Resolve text for an MsGetComData call. Semantics per the 207-byte body:
//   - retval is a data-entry pointer (or &DAT_01136000 sentinel).
//   - out_base holds the base address used with the entry's name offset.
//   - text pointer = out_base + *(ushort*)(entry + K) for some K.
// Different callers use different K values:
//   FUN_008998e0, FUN_00899730: K=8   (text at entry+8)
//   FUN_00895290 (Learning):    K=0   (text at entry+0)
// The probe doesn't know which K applies per call — it tries both offsets
// and returns whichever produces a printable result. Belt-and-braces.
function decodeFromEntry(entryPtr, outBasePtr) {
    const results = { k0: '', k8: '' };
    try {
        if (!entryPtr || entryPtr.isNull()) return results;
        if (!outBasePtr || outBasePtr.isNull()) return results;
        const outBase = outBasePtr.readPointer();
        if (!outBase || outBase.isNull()) return results;
        try {
            const off0 = entryPtr.readU16();
            results.k0 = decodeFFXTextBytes(outBase.add(off0), 64);
        } catch (e) {}
        try {
            const off8 = entryPtr.add(8).readU16();
            results.k8 = decodeFFXTextBytes(outBase.add(off8), 64);
        } catch (e) {}
    } catch (e) {}
    return results;
}
// Pick the better-looking decoded candidate (usually the one longer
// than 1 char and containing letters).
function pickText(candidates) {
    const k0 = candidates.k0 || '';
    const k8 = candidates.k8 || '';
    const score = function (s) {
        if (!s) return 0;
        if (s === '<null>' || s === '<unreadable>') return 0;
        let letters = 0;
        for (let i = 0; i < s.length; i++) {
            const c = s.charCodeAt(i);
            if ((c >= 0x41 && c <= 0x5a) || (c >= 0x61 && c <= 0x7a)) letters++;
        }
        return letters;
    };
    const s0 = score(k0), s8 = score(k8);
    if (s0 === 0 && s8 === 0) return '';
    if (s8 >= s0) return 'k8="' + k8 + '"' + (s0 > 0 ? ' k0="' + k0 + '"' : '');
    return 'k0="' + k0 + '"' + (s8 > 0 ? ' k8="' + k8 + '"' : '');
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

// ── State machine ──
// idle → baseline → idle → armed_waiting → armed_capturing → idle
let state = 'idle';
let frameCount = 0;
let shuttingDown = false;

// Baseline: per (caller_rva, cmdId, source) → count
// Source is 'COM' or 'MGT' to distinguish the two hooks.
// Key format: source|caller_rva|cmdId
const baselineCounts = {};
let baselineFrames = 0, baselineStart = 0;
let baselineActive = false;

// Armed: same keyspace, per-call verbose logging to file.
const armedCounts = {};
let armedFrames = 0, armedStart = 0;
let actingSlot = -1;

// Per-slot df0/dfb transition tracking
const prevDf0 = new Array(NUM_ACTORS).fill(-1);
const prevDfb = new Array(NUM_ACTORS).fill(-1);
const armedActorPollMax = {};

function mkKey(source, callerRva, cmdId) {
    return source + '|' + callerRva.toString(16) + '|' + cmdId.toString(16);
}

function isInRecordingWindow() {
    return baselineActive || state === 'armed_capturing';
}

// ── Commands ──
function cmdBaseline() {
    if (state === 'armed_waiting' || state === 'armed_capturing') {
        logBoth('[!] F11 to reset armed first'); return;
    }
    if (!baselineActive) {
        baselineActive = true;
        baselineStart = frameCount;
        // Clear counts on each new baseline
        for (const k in baselineCounts) delete baselineCounts[k];
        logBoth('[BASELINE STARTED] stay idle ~5-10s — F8 again to stop');
    } else {
        baselineActive = false;
        baselineFrames = frameCount - baselineStart;
        const keyCount = Object.keys(baselineCounts).length;
        logBoth('[BASELINE STOPPED] ' + baselineFrames + ' frames, ' +
                keyCount + ' unique (source,caller,cmdId) triples. F9 to arm.');
    }
}

function cmdArm() {
    if (state !== 'idle') return;
    // Clear armed counters
    for (const k in armedCounts) delete armedCounts[k];
    armedStart = frameCount;
    actingSlot = -1;
    for (let i = 0; i < NUM_ACTORS; i++) { prevDf0[i] = -1; prevDfb[i] = -1; }
    for (const k in armedActorPollMax) delete armedActorPollMax[k];
    state = 'armed_waiting';
    logBoth('[ARMED WAITING] inBattle=' + (inBattle() ? 1 : 0) +
            ' — trigger one enemy named ability');
}

function beginCapture(slot) {
    actingSlot = slot;
    armedStart = frameCount;
    state = 'armed_capturing';
    logBoth('[CAPTURE START] slot=' + slot + ' (df0 entered 3)');
    logFile('[CAPTURE START] frame=' + frameCount + ' slot=' + slot);
}

function endCaptureAuto(reason) {
    if (state !== 'armed_capturing') return;
    armedFrames = frameCount - armedStart;
    state = 'idle';
    logBoth('[CAPTURE END: ' + reason + '] slot=' + actingSlot +
            '  armed=' + armedFrames + ' frames  armedEntries=' +
            Object.keys(armedCounts).length);
    logFile('[CAPTURE END: ' + reason + '] frame=' + frameCount +
            ' slot=' + actingSlot + ' armedFrames=' + armedFrames);
    dumpSanity();
    dumpDiff();
}

function cmdDump() {
    if (state !== 'armed_waiting' && state !== 'armed_capturing') return;
    const fromWaiting = (state === 'armed_waiting');
    armedFrames = frameCount - armedStart;
    state = 'idle';
    logBoth('[DUMP MANUAL' + (fromWaiting ? ' (auto-detect missed)' : '') +
            '] slot=' + actingSlot + '  armed=' + armedFrames +
            ' frames  armedEntries=' + Object.keys(armedCounts).length);
    logFile('[DUMP MANUAL] frame=' + frameCount + ' slot=' + actingSlot);
    dumpSanity();
    dumpDiff();
}

function cmdReset() {
    state = 'idle';
    baselineActive = false;
    for (const k in baselineCounts) delete baselineCounts[k];
    for (const k in armedCounts) delete armedCounts[k];
    for (const k in armedActorPollMax) delete armedActorPollMax[k];
    actingSlot = -1;
    logBoth('[RESET]');
}

function dumpSanity() {
    logFile('[SANITY] actor-state peaks during armed:');
    let anyActed = false;
    for (let i = 0; i < NUM_ACTORS; i++) {
        const peak = armedActorPollMax[i];
        if (peak === undefined) continue;
        if (peak.df0 >= 3) anyActed = true;
        logFile('  slot=' + i + ' peak_df0=' + peak.df0 + ' peak_dfb=' + peak.dfb);
    }
    logBoth('[SANITY] action-fired = ' + anyActed +
            (anyActed ? '' : ' (WARNING — no actor reached df0>=3; rerun)'));
}

function dumpDiff() {
    // Build a list of (source, callerRva, cmdId, armedCount, baseCount, sampleText)
    // where sampleText is captured during armed (logged per-call below).
    const armedKeys = Object.keys(armedCounts);
    armedKeys.sort();
    const newOnly = [];
    const surge = [];
    const all = [];
    for (let i = 0; i < armedKeys.length; i++) {
        const k = armedKeys[i];
        const armedEntry = armedCounts[k];
        const baseEntry = baselineCounts[k];
        const armedN = armedEntry.count;
        const baseN = baseEntry ? baseEntry.count : 0;
        const baseRate = (baselineFrames > 0 && baseN > 0) ? (baseN / baselineFrames) : 0;
        const expected = baseRate * armedFrames;
        const entry = {
            key: k,
            source: armedEntry.source,
            caller: armedEntry.caller,
            cmdId: armedEntry.cmdId,
            armedN: armedN,
            baseN: baseN,
            expected: expected,
            sampleText: armedEntry.sampleText
        };
        all.push(entry);
        if (baseN === 0) newOnly.push(entry);
        else if (armedN >= 3 && expected > 0 && (armedN / expected) >= 3) {
            surge.push(entry);
        }
    }
    const sortByArmed = function (a, b) { return b.armedN - a.armedN; };
    newOnly.sort(sortByArmed);
    surge.sort(sortByArmed);
    all.sort(sortByArmed);

    logFile('');
    logFile('=== DIFF: NEW (baseline=0) — ' + newOnly.length + ' entries ===');
    for (let i = 0; i < newOnly.length; i++) {
        const e = newOnly[i];
        logFile('  [' + e.source + '] caller=0x' + e.caller.toString(16) +
                ' cmdId=0x' + e.cmdId.toString(16) +
                ' armedN=' + e.armedN + ' text="' + (e.sampleText || '') + '"');
    }
    logFile('');
    logFile('=== DIFF: SURGE (>=3x rate) — ' + surge.length + ' entries ===');
    for (let i = 0; i < surge.length; i++) {
        const e = surge[i];
        logFile('  [' + e.source + '] caller=0x' + e.caller.toString(16) +
                ' cmdId=0x' + e.cmdId.toString(16) +
                ' armedN=' + e.armedN + ' expected=' + e.expected.toFixed(2) +
                ' ratio=' + (e.armedN / e.expected).toFixed(2) + 'x' +
                ' text="' + (e.sampleText || '') + '"');
    }
    logFile('');
    logFile('=== DIFF: ALL (armed entries, sorted by armedN) — ' + all.length + ' entries ===');
    for (let i = 0; i < all.length; i++) {
        const e = all[i];
        logFile('  [' + e.source + '] caller=0x' + e.caller.toString(16) +
                ' cmdId=0x' + e.cmdId.toString(16) +
                ' armedN=' + e.armedN + ' baseN=' + e.baseN +
                ' text="' + (e.sampleText || '') + '"');
    }
    logFile('=== END ===');
    logBoth('[*] ' + newOnly.length + ' NEW, ' + surge.length +
            ' SURGE, ' + all.length + ' total — see log for [DIFF] sections');
}

// ── MsGetComData hook ──
Interceptor.attach(rva(RVA_MS_GET_COM_DATA), {
    onEnter: function (args) {
        if (shuttingDown) return;
        if (!isInRecordingWindow()) return;
        if (!inBattle()) return;
        this.cmdId = args[0].toInt32() & 0xFFFF;
        this.outBasePtr = args[1];  // may be NULL
        this.callerRva = this.returnAddress.sub(base).toInt32();
    },
    onLeave: function (retval) {
        if (shuttingDown) return;
        if (!isInRecordingWindow()) return;
        if (this.cmdId === undefined) return;
        const cmdId = this.cmdId;
        const caller = this.callerRva;
        let text = '';
        try {
            const cand = decodeFromEntry(retval, this.outBasePtr);
            text = pickText(cand);
        } catch (e) {}
        const key = mkKey('COM', caller, cmdId);
        // Bucket
        const bucket = baselineActive ? baselineCounts : armedCounts;
        if (bucket[key]) {
            bucket[key].count++;
            if (!bucket[key].sampleText && text) bucket[key].sampleText = text;
        } else {
            bucket[key] = {
                source: 'COM', caller: caller, cmdId: cmdId,
                count: 1, sampleText: text || ''
            };
        }
        // Verbose per-call log ONLY during armed (file only)
        if (state === 'armed_capturing') {
            logFile('[COM] f=' + frameCount +
                ' cmd=0x' + cmdId.toString(16) +
                ' prefix=' + (cmdId >> 12) +
                ' row=0x' + (cmdId & 0xfff).toString(16) +
                ' caller=0x' + caller.toString(16) +
                ' text="' + text + '"');
        }
    }
});

// ── MsMenuGetText hook (control: see if banner uses this path) ──
Interceptor.attach(rva(RVA_MS_MENU_GET_TEXT), {
    onEnter: function (args) {
        if (shuttingDown) return;
        if (!isInRecordingWindow()) return;
        if (!inBattle()) return;
        const arg0 = args[0].toInt32();
        // Filter out-of-range table indices (MsMenuGetText supports 0..0x10)
        if (arg0 < 0 || arg0 > 0x10) return;
        this.arg0 = arg0;
        this.arg1 = args[1].toInt32() & 0xFFFF;
        this.callerRva = this.returnAddress.sub(base).toInt32();
    },
    onLeave: function (retval) {
        if (shuttingDown) return;
        if (!isInRecordingWindow()) return;
        if (this.arg0 === undefined) return;
        const arg0 = this.arg0;
        const arg1 = this.arg1;
        const caller = this.callerRva;
        let text = '';
        // MsMenuGetText returns a pointer directly to the text bytes.
        try {
            if (retval && !retval.isNull()) {
                text = decodeFFXTextBytes(retval, 64);
            }
        } catch (e) {}
        // Key includes arg0 (table) since same caller may use multiple tables.
        const key = mkKey('MGT|t' + arg0, caller, arg1);
        const bucket = baselineActive ? baselineCounts : armedCounts;
        if (bucket[key]) {
            bucket[key].count++;
            if (!bucket[key].sampleText && text) bucket[key].sampleText = text;
        } else {
            bucket[key] = {
                source: 'MGT|t' + arg0, caller: caller, cmdId: arg1,
                count: 1, sampleText: text || ''
            };
        }
        if (state === 'armed_capturing') {
            logFile('[MGT] f=' + frameCount +
                ' table=' + arg0 + ' id=0x' + arg1.toString(16) +
                ' caller=0x' + caller.toString(16) +
                ' text="' + text + '"');
        }
    }
});

// ── Per-frame tick: hotkeys + heartbeat ──
Interceptor.attach(rva(RVA_FRAME_TICK), {
    onLeave: function () {
        if (shuttingDown) return;
        frameCount++;
        if (keyPressed(VK_F8))  cmdBaseline();
        if (keyPressed(VK_F9))  cmdArm();
        if (keyPressed(VK_F10)) cmdDump();
        if (keyPressed(VK_F11)) cmdReset();

        if (frameCount % 600 === 0) {
            logFile('[HEARTBEAT] frames=' + frameCount + ' state=' + state +
                    ' baselineActive=' + baselineActive +
                    ' inBattle=' + inBattle());
        }
    }
});

// ── Dispatcher hook: actor-state detector + armed transitions ──
// Session 28 learned: sampling actor state in the frame hook misses
// transient df0 flips; the dispatcher at 0x387EC0 is the only reliable
// sampling point (the game itself reads df0 there each frame).
Interceptor.attach(rva(RVA_DISPATCHER), {
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
                    ' df0=' + pDf0 + '->' + df0 +
                    ' dfb=' + pDfb + '->' + dfb);
            }
            prevDf0[i] = df0;
            prevDfb[i] = dfb;
            if (df0 === 3 && firstActingSlot === -1) firstActingSlot = i;
        }
        if (state === 'armed_waiting' && firstActingSlot !== -1) {
            beginCapture(firstActingSlot);
        } else if (state === 'armed_capturing') {
            let stillActing = false;
            try {
                const a = getActor(actingSlot);
                if (a) stillActing = (a.add(0xDF0).readU8() !== 0);
            } catch (e) {}
            const ranTooLong = (frameCount - armedStart) > 900;
            if (!stillActing) {
                endCaptureAuto('acting actor df0 returned to 0');
            } else if (ranTooLong) {
                endCaptureAuto('15s safety timeout');
            }
        }
    }
});

// ── Damage hook: primary end-trigger (tight window) ──
Interceptor.attach(rva(RVA_DAMAGE), {
    onEnter: function (args) {
        if (shuttingDown) return;
        if (state !== 'armed_waiting' && state !== 'armed_capturing') return;
        try {
            const target = args[0].toInt32() & 0xFF;
            const dmgType = args[1].toInt32();
            const value = args[2].toInt32();
            logFile('[DMG] frame=' + frameCount + ' target=' + target +
                ' type=' + dmgType + ' value=' + value);
            if (state === 'armed_capturing') {
                endCaptureAuto('damage fired');
            }
        } catch (e) {}
    }
});

logBoth('[*] Ready. In battle: F8 baseline -> F8 stop -> F9 arm -> trigger enemy ability.');
logBoth('    Auto-ends on damage fire or df0 return-to-0. F10 manual. F11 reset.');

// ── Shutdown handler ──
rpc.exports = {
    dispose: function () {
        shuttingDown = true;
        try { logFile('[DISPOSE] shutdown'); } catch (e) {}
        try { logHandle.close(); } catch (e) {}
    }
};
