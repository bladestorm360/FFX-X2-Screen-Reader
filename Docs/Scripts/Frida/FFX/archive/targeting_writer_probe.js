// @desc Find the battle targeting-bitmask writer (winBase+0xD8/+0xDA).
//
// Problem: battle-window cursor is event-driven via FUN_0089C870 /
// FUN_0089CD50 / FUN_0089D580 (all write +0x42 selIdx). Targeting bits
// live at +0xD8 (allies) / +0xDA (monsters). Offset +0xD8 collides with
// ~100 unrelated structs across the decompile so we can't grep for it.
// Use Stalker baseline + arm + dump to find the function that actually
// writes the bitmask during a live target-change action.
//
// Pre-flight:
//   1. Start a battle (random encounter works).
//   2. When it's a character's turn, press Attack (or any command that
//      asks you to select a target). Cursor is now on enemy 1.
//   3. Do NOT move yet. Stay idle.
//
// Probe workflow:
//   F8  — start baseline (stay idle, don't touch controller for 5-10s)
//   F8  — stop baseline (computes per-frame rate per function)
//   F9  — arm (you have ~1 second to press the next button)
//   Press Left or Right ONCE (cursor moves to a different enemy).
//   F10 — dump filtered results.
//
// Output: Docs/Scripts/Frida/output/FFX/targeting_writer_probe.log
//   Section 1 (NEW) = calls that fired during armed but NEVER during
//   baseline. Strongest candidates for the targeting writer.
//   Section 2 (SURGE) = calls that fired much more during armed.
//   Section 3 (RAW) = full dump.
//
// Expected: one or two functions in Section 1 that are specifically
// about targeting (not about cursor — cursor writers already known).
// Report those RVAs back to Claude for analysis.
//
// Also includes a sanity check that logs the current target bitmasks
// before/after the arm window so we can confirm the action actually
// changed them.

const LOG_PATH = 'D:/Games/Dev/Custom/FFX/ffx-x2-screen-reader/Docs/Scripts/Frida/output/FFX/targeting_writer_probe.log';
const logHandle = new File(LOG_PATH, 'w');
function logFile(msg) { try { logHandle.write(msg + '\n'); logHandle.flush(); } catch (e) {} }
function logBoth(msg) { console.log(msg); logFile(msg); }

// Resolve FFX.exe module explicitly.
let gameModule = null;
Process.enumerateModules().forEach(function(m) {
    if (m.name.toLowerCase() === 'ffx.exe') gameModule = m;
});
if (!gameModule) gameModule = Process.enumerateModules()[0];
const base = gameModule.base;
const moduleEnd = base.add(gameModule.size);
function rva(offset) { return base.add(offset); }

logBoth('[*] Targeting-writer probe (Stalker baseline/arm/dump)');
logBoth('[*] ' + gameModule.name + ' base=0x' + base.toString(16));

// ── Keyboard input (standard GetAsyncKeyState pattern) ──
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
const VK_F8 = 0x77, VK_F9 = 0x78, VK_F10 = 0x79, VK_F11 = 0x7A;

// ── Targeting-scenario sanity: read curChrs/curMons from current battle window
const BTL_WIN_ARRAY_RVA = 0xF3C910;
const BTL_CUR_WIN_IDX_RVA = 0x1FCC092;
function readTargeting() {
    try {
        const idx = rva(BTL_CUR_WIN_IDX_RVA).readU8();
        if (idx > 15) return null;
        const winBase = rva(BTL_WIN_ARRAY_RVA).add(idx * 0xF0);
        return {
            curChrs: winBase.add(0xD8).readU16(),
            curMons: winBase.add(0xDA).readU16()
        };
    } catch (e) { return null; }
}

// ── Stalker state machine (same as stalker_event_probe) ──
let state = 'idle';
let gameThreadId = null;
let baselineCounts = {}, baselineRates = null, baselineFrames = 0, baselineStart = 0;
let armedCounts = {}, armedFrames = 0, armedStart = 0;
let armedTargetingBefore = null, armedTargetingAfter = null;
let frameCount = 0;
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
            for (const addr in summary) target[addr] = (target[addr] || 0) + summary[addr];
        }
    });
}
function stopStalker() { try { Stalker.unfollow(gameThreadId); } catch (e) {} }

function cmdBaseline() {
    if (gameThreadId === null) { logBoth('[!] thread not captured yet'); return; }
    if (state === 'armed') { logBoth('[!] F10 to stop armed first'); return; }
    if (state === 'idle' && baselineRates === null) {
        baselineCounts = {}; baselineStart = frameCount; state = 'baseline';
        try { startStalker(baselineCounts); logBoth('[BASELINE STARTED] idle for 5-10s, F8 again to stop'); }
        catch (e) { logBoth('[!] follow failed: ' + e.message); state = 'idle'; }
    } else if (state === 'baseline') {
        stopStalker();
        baselineFrames = frameCount - baselineStart;
        if (baselineFrames < 10) { logBoth('[!] baseline too short'); baselineCounts = {}; state = 'idle'; return; }
        baselineRates = {};
        for (const a in baselineCounts) baselineRates[a] = baselineCounts[a] / baselineFrames;
        state = 'idle';
        logBoth('[BASELINE STOPPED] ' + baselineFrames + ' frames, ' +
                Object.keys(baselineCounts).length + ' call targets. F9 to arm.');
    }
}
function cmdArm() {
    if (baselineRates === null) { logBoth('[!] need baseline first'); return; }
    if (state !== 'idle') return;
    armedCounts = {}; armedStart = frameCount; state = 'armed';
    armedTargetingBefore = readTargeting();
    try {
        startStalker(armedCounts);
        logBoth('[ARMED] targetingBefore=' +
                (armedTargetingBefore ? ('chrs=0x' + armedTargetingBefore.curChrs.toString(16) +
                 ' mons=0x' + armedTargetingBefore.curMons.toString(16)) : 'n/a') +
                ' — press Left or Right ONCE, then F10');
    } catch (e) { logBoth('[!] follow failed: ' + e.message); state = 'idle'; }
}
function cmdDump() {
    if (state !== 'armed') return;
    stopStalker();
    armedFrames = frameCount - armedStart;
    armedTargetingAfter = readTargeting();
    state = 'idle';
    logBoth('[DUMP] armed=' + armedFrames + ' frames');
    logFile('[TARGETING] before=' + JSON.stringify(armedTargetingBefore) +
            '  after=' + JSON.stringify(armedTargetingAfter));
    dumpFiltered();
}
function cmdReset() {
    if (state !== 'idle') stopStalker();
    state = 'idle';
    baselineCounts = {}; baselineRates = null; baselineFrames = 0;
    armedCounts = {}; armedFrames = 0;
    logBoth('[RESET]');
}

function dumpFiltered() {
    const newCalls = [], surgeCalls = [], raw = [];
    for (const a in armedCounts) {
        const p = ptr(a);
        if (p.compare(base) < 0 || p.compare(moduleEnd) >= 0) continue;
        const rvaOff = p.sub(base).toInt32();
        const armedN = armedCounts[a];
        const baseN = baselineCounts[a] || 0;
        const baseRate = baselineRates[a] || 0;
        const expected = baseRate * armedFrames;
        raw.push({ rva: rvaOff, armedN, baseN });
        if (baseN === 0) newCalls.push({ rva: rvaOff, armedN });
        else if (armedN >= 3 && expected > 0 && (armedN / expected) >= 3) {
            surgeCalls.push({ rva: rvaOff, armedN, expected: expected.toFixed(2), ratio: (armedN / expected).toFixed(2) });
        }
    }
    newCalls.sort(function(a, b) { return a.armedN - b.armedN; });
    surgeCalls.sort(function(a, b) { return b.armedN - a.armedN; });
    raw.sort(function(a, b) { return a.armedN - b.armedN; });
    logFile('');
    logFile('=== SECTION 1: NEW (baseline=0) — ' + newCalls.length + ' ===');
    for (let i = 0; i < newCalls.length; i++) logFile('  0x' + newCalls[i].rva.toString(16) + '  armed=' + newCalls[i].armedN);
    logFile('');
    logFile('=== SECTION 2: SURGE (>=3x rate) — ' + surgeCalls.length + ' ===');
    for (let i = 0; i < surgeCalls.length; i++) {
        const e = surgeCalls[i];
        logFile('  0x' + e.rva.toString(16) + '  armed=' + e.armedN + '  expected=' + e.expected + '  ratio=' + e.ratio + 'x');
    }
    logFile('');
    logFile('=== SECTION 3: RAW — ' + raw.length + ' ===');
    for (let i = 0; i < raw.length; i++) logFile('  0x' + raw[i].rva.toString(16) + '  armed=' + raw[i].armedN + '  baseline=' + raw[i].baseN);
    logFile('=== END ===');
    logBoth('[*] ' + newCalls.length + ' NEW, ' + surgeCalls.length + ' SURGE, ' + raw.length + ' raw. See log.');
}

const FRAME_RVA = 0x471D10;
Interceptor.attach(rva(FRAME_RVA), {
    onLeave: function() {
        if (gameThreadId === null) {
            gameThreadId = Process.getCurrentThreadId();
            logBoth('[*] game thread=' + gameThreadId);
        }
        frameCount++;
        if (keyPressed(VK_F8))  cmdBaseline();
        if (keyPressed(VK_F9))  cmdArm();
        if (keyPressed(VK_F10)) cmdDump();
        if (keyPressed(VK_F11)) cmdReset();
    }
});

logBoth('[*] Ready. Start a battle, reach target-select. F8 baseline, F8 stop, F9 arm, press L/R, F10 dump.');
