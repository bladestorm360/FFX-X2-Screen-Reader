// @desc Find the sphere-grid character-switch writer (eng+0x115BC).
//
// Problem: sphere grid cursor (eng+0x112FC) and open/close are already
// event-driven. Character switching (via shoulder buttons L1/R1) updates
// the 1-byte field at eng+0x115BC but no writer is visible in the grep
// for that offset (all matches were reads). Use Stalker baseline + arm +
// dump to find the actual writer.
//
// Pre-flight:
//   1. Open the sphere grid via main menu -> Sphere Grid.
//   2. Wait for the grid to render on your starting character.
//   3. Stay idle (no cursor moves).
//
// Probe workflow:
//   F8  — start baseline (idle 5-10s, no input)
//   F8  — stop baseline
//   F9  — arm
//   Press L1 or R1 ONCE to switch to the adjacent character.
//   F10 — dump
//
// Output: Docs/Scripts/Frida/output/FFX/sgrid_char_switch_probe.log
//   Section 1 NEW entries that correlate with character switch.
//
// The sanity block logs the chrId byte before/after the arm so we can
// confirm the switch actually happened.

const LOG_PATH = 'D:/Games/Dev/Custom/FFX/ffx-x2-screen-reader/Docs/Scripts/Frida/output/FFX/sgrid_char_switch_probe.log';
const logHandle = new File(LOG_PATH, 'w');
function logFile(msg) { try { logHandle.write(msg + '\n'); logHandle.flush(); } catch (e) {} }
function logBoth(msg) { console.log(msg); logFile(msg); }

let gameModule = null;
Process.enumerateModules().forEach(function(m) { if (m.name.toLowerCase() === 'ffx.exe') gameModule = m; });
if (!gameModule) gameModule = Process.enumerateModules()[0];
const base = gameModule.base;
const moduleEnd = base.add(gameModule.size);
function rva(offset) { return base.add(offset); }

logBoth('[*] Sphere-grid char-switch writer probe');
logBoth('[*] ' + gameModule.name + ' base=0x' + base.toString(16));

// Keyboard (same pattern).
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

// Sphere-grid sanity.
const SGRID_ENGINE_PTR_RVA = 0x1F05834;
function readChrId() {
    try {
        const eng = rva(SGRID_ENGINE_PTR_RVA).readPointer();
        if (eng.isNull() || eng.compare(ptr(0x10000)) <= 0) return null;
        return eng.add(0x115BC).readU8();
    } catch (e) { return null; }
}

// Stalker state machine.
let state = 'idle';
let gameThreadId = null;
let baselineCounts = {}, baselineRates = null, baselineFrames = 0, baselineStart = 0;
let armedCounts = {}, armedFrames = 0, armedStart = 0;
let chrBefore = null, chrAfter = null;
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
            for (const a in summary) target[a] = (target[a] || 0) + summary[a];
        }
    });
}
function stopStalker() { try { Stalker.unfollow(gameThreadId); } catch (e) {} }

function cmdBaseline() {
    if (gameThreadId === null) return;
    if (state === 'armed') return;
    if (state === 'idle' && baselineRates === null) {
        baselineCounts = {}; baselineStart = frameCount; state = 'baseline';
        try { startStalker(baselineCounts); logBoth('[BASELINE STARTED] idle, F8 to stop'); }
        catch (e) { state = 'idle'; }
    } else if (state === 'baseline') {
        stopStalker();
        baselineFrames = frameCount - baselineStart;
        if (baselineFrames < 10) { baselineCounts = {}; state = 'idle'; return; }
        baselineRates = {};
        for (const a in baselineCounts) baselineRates[a] = baselineCounts[a] / baselineFrames;
        state = 'idle';
        logBoth('[BASELINE STOPPED] ' + baselineFrames + ' frames. F9 to arm.');
    }
}
function cmdArm() {
    if (baselineRates === null) return;
    if (state !== 'idle') return;
    armedCounts = {}; armedStart = frameCount; state = 'armed';
    chrBefore = readChrId();
    try { startStalker(armedCounts); logBoth('[ARMED] chrBefore=' + chrBefore + ' — press L1/R1 ONCE, F10'); }
    catch (e) { state = 'idle'; }
}
function cmdDump() {
    if (state !== 'armed') return;
    stopStalker();
    armedFrames = frameCount - armedStart;
    chrAfter = readChrId();
    state = 'idle';
    logFile('[CHR] before=' + chrBefore + '  after=' + chrAfter);
    logBoth('[DUMP] armed=' + armedFrames + ' frames, chr ' + chrBefore + '->' + chrAfter);
    const newCalls = [], raw = [];
    for (const a in armedCounts) {
        const p = ptr(a);
        if (p.compare(base) < 0 || p.compare(moduleEnd) >= 0) continue;
        const rvaOff = p.sub(base).toInt32();
        const armedN = armedCounts[a];
        const baseN = baselineCounts[a] || 0;
        raw.push({ rva: rvaOff, armedN, baseN });
        if (baseN === 0) newCalls.push({ rva: rvaOff, armedN });
    }
    newCalls.sort(function(a, b) { return a.armedN - b.armedN; });
    raw.sort(function(a, b) { return a.armedN - b.armedN; });
    logFile('');
    logFile('=== SECTION 1: NEW — ' + newCalls.length + ' ===');
    for (let i = 0; i < newCalls.length; i++) logFile('  0x' + newCalls[i].rva.toString(16) + '  armed=' + newCalls[i].armedN);
    logFile('');
    logFile('=== SECTION 2: RAW — ' + raw.length + ' ===');
    for (let i = 0; i < raw.length; i++) logFile('  0x' + raw[i].rva.toString(16) + '  armed=' + raw[i].armedN + '  baseline=' + raw[i].baseN);
    logFile('=== END ===');
}
function cmdReset() {
    if (state !== 'idle') stopStalker();
    state = 'idle';
    baselineCounts = {}; baselineRates = null; baselineFrames = 0; armedCounts = {}; armedFrames = 0;
    logBoth('[RESET]');
}

const FRAME_RVA = 0x471D10;
Interceptor.attach(rva(FRAME_RVA), {
    onLeave: function() {
        if (gameThreadId === null) gameThreadId = Process.getCurrentThreadId();
        frameCount++;
        if (keyPressed(VK_F8))  cmdBaseline();
        if (keyPressed(VK_F9))  cmdArm();
        if (keyPressed(VK_F10)) cmdDump();
        if (keyPressed(VK_F11)) cmdReset();
    }
});

logBoth('[*] Ready. Open sphere grid, then F8/F8/F9/press L1-or-R1/F10.');
