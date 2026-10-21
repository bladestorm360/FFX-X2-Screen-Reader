// @desc Phase B dispatcher hunt — IWindowEvent vtable trap across menus
//
// Plan: C:\Users\pmoen\.claude\plans\dispatcher-mossy-sunrise.md
//
// Hypothesis: PhyreEngine routes window events through IWindowEvent
// vtables. The abstract base lives at 0x00b46c1c. FFEscMenu overrides
// it at 0x00b46c34 (rtti_classes.txt:3107-3112). The Iggy bridge
// registers its own at 0x00c85ec0 (FUN_00a44a30 in
// layer3_iggy_interface.txt:39343). Phyre's registration entry is
// FUN_00625e30 (0x625e30 / RVA 0x225e30) — every IWindowEvent
// implementer passes its subobject through it.
//
// This probe:
//   1. Hooks FUN_00625e30 to capture every registration at runtime
//      (catches any vtables we did not predict).
//   2. Pre-installs hooks on the three known vtables so we still see
//      events even if registration fired before script load.
//   3. Hooks slot 0 + slot 1 of every vtable (8-byte gap between the
//      IWindowEvent / FFEscMenu vtables in rtti_classes.txt suggests
//      2 method slots).
//   4. F8 starts observation; F9/F10/F11 label the current menu;
//      F12 dumps a per-(vtable, slot, label) fire matrix.
//
// Success criterion: a single (vtable, slot) pair fires for cursor
// moves across Items + Equip + Abilities, with consistent ECX vtable
// identification — that pair is the dispatcher seam.
//
// CONFIDENCE RATINGS (per CLAUDE.md):
//   FUN_00625e30 = registration entry: 0.7 (38-byte forwarder to
//     FUN_0061f070; param_1 is the IWindowEvent subobject)
//   0x00b46c1c IWindowEvent abstract vtable: 0.95 (rtti_classes.txt)
//   0x00b46c34 FFEscMenu override vtable: 0.95 (rtti_classes.txt +
//     0x68e170 init body)
//   0x00c85ec0 Iggy vtable: 0.9 (FUN_00a44a30 registration call site)
//   2 vtable slots: 0.85 (8-byte vtable gap in rtti_classes.txt)
//   Frame hook RVA 0x471D10: 0.95 (proven in action_stalker_probe.js)
//
// Output: Docs/Scripts/Frida/output/FFX/dispatcher_vtable_probe.log

const LOG_PATH = 'D:/Games/Dev/Custom/FFX/ffx-x2-screen-reader/Docs/Scripts/Frida/output/FFX/dispatcher_vtable_probe.log';
const logHandle = new File(LOG_PATH, 'w');
function logFile(msg) { try { logHandle.write(msg + '\n'); logHandle.flush(); } catch (e) {} }
function logBoth(msg) { console.log(msg); logFile(msg); }

// ── Module + RVA helpers (proven pattern, frida_reference.md) ──
let gameModule = null;
Process.enumerateModules().forEach(function (m) {
    if (m.name.toLowerCase() === 'ffx.exe') gameModule = m;
});
if (!gameModule) gameModule = Process.enumerateModules()[0];
const base = gameModule.base;
const moduleEnd = base.add(gameModule.size);
function rva(off) { return base.add(off); }
function toGhidra(addr) { return ptr(addr).sub(base).add(0x00400000); }
function isInModule(addr) {
    const p = ptr(addr);
    return p.compare(base) >= 0 && p.compare(moduleEnd) < 0;
}

logBoth('[*] dispatcher_vtable_probe  module=' + gameModule.name +
        '  base=0x' + base.toString(16));

// ── Known vtable RVAs (Ghidra VA - 0x400000) ──
const RVA_IWINDOWEVENT_VTABLE = 0x746c1c;  // 0x00b46c1c
const RVA_FFESCMENU_OVERRIDE  = 0x746c34;  // 0x00b46c34
const RVA_IGGY_VTABLE         = 0x885ec0;  // 0x00c85ec0
const RVA_FUN_00625e30        = 0x225e30;
const RVA_FRAME_HOOK          = 0x471D10;  // proven in action_stalker_probe.js

const KNOWN_VTABLE_LABELS = {};
KNOWN_VTABLE_LABELS[rva(RVA_IWINDOWEVENT_VTABLE).toString()] = 'IWindowEvent_abstract';
KNOWN_VTABLE_LABELS[rva(RVA_FFESCMENU_OVERRIDE).toString()]  = 'FFEscMenu_override';
KNOWN_VTABLE_LABELS[rva(RVA_IGGY_VTABLE).toString()]         = 'Iggy_vtable';

const SLOT_COUNT = 2;

// ── State ──
let state = 'idle';            // 'idle' | 'observing'
let currentLabel = '(none)';
let gameThreadId = null;
let frameCount = 0;
let knownVtablesInstalled = false;
const hookedVtables = {};      // vtableAddr.toString() -> bool
const fireCounts = {};         // "vtableAddr:slot:label" -> count
const distinctEcx = {};        // "vtableAddr:slot:label" -> { ecxStr: true, ... }
const pendingHooks = [];       // queue of vtable NativePointers awaiting hook install

// ── Vtable hook installation ──
// Reads slots 0..SLOT_COUNT-1 from the vtable and attaches Interceptor
// to each method. Deferred from the frame hook so we never call
// Interceptor.attach inside another hook's callback.
function installVtableHooks(vtableAddr) {
    const key = vtableAddr.toString();
    if (hookedVtables[key]) return;
    hookedVtables[key] = true;
    const label = KNOWN_VTABLE_LABELS[key] ||
                  ('disc_0x' + toGhidra(vtableAddr).toString(16));

    let installedSlots = 0;
    for (let slot = 0; slot < SLOT_COUNT; slot++) {
        let methodAddr;
        try {
            methodAddr = vtableAddr.add(slot * 4).readPointer();
        } catch (e) {
            logFile('[VTABLE] ' + label + ' slot=' + slot +
                    ' read fail: ' + e.message);
            continue;
        }
        if (!isInModule(methodAddr)) {
            logFile('[VTABLE] ' + label + ' slot=' + slot +
                    ' methodAddr=' + methodAddr +
                    ' OUT_OF_MODULE — skipping');
            continue;
        }
        const methodGhidraVA = toGhidra(methodAddr);
        try {
            // IIFE captures slot/label/methodAddr per-iteration.
            (function (slotIdx, vtableLabel, methodAddrCap,
                       methodGhidraVACap, vtableKey) {
                Interceptor.attach(methodAddrCap, {
                    onEnter: function () {
                        if (state !== 'observing') return;
                        const ecx = ptr(this.context.ecx);
                        const tid = Process.getCurrentThreadId();
                        const countKey = vtableKey + ':' + slotIdx +
                                         ':' + currentLabel;
                        fireCounts[countKey] =
                            (fireCounts[countKey] || 0) + 1;
                        if (!distinctEcx[countKey])
                            distinctEcx[countKey] = {};
                        distinctEcx[countKey][ecx.toString()] = true;

                        // First fire of (vtable, slot, label): detail log.
                        if (fireCounts[countKey] === 1) {
                            let ecxVtableStr = 'read_fail';
                            try {
                                const ecxVtable = ecx.readPointer();
                                if (isInModule(ecxVtable)) {
                                    ecxVtableStr = '0x' +
                                        toGhidra(ecxVtable).toString(16);
                                } else {
                                    ecxVtableStr = 'oob_' +
                                        ecxVtable.toString();
                                }
                            } catch (e) {}
                            logFile('[FIRE] vtable=' + vtableLabel +
                                    ' slot=' + slotIdx +
                                    ' method=0x' +
                                    methodGhidraVACap.toString(16) +
                                    ' ECX=0x' + ecx.toString(16) +
                                    ' ECX_vtable=' + ecxVtableStr +
                                    ' tid=' + tid +
                                    ' label=' + currentLabel);
                            if (gameThreadId !== null &&
                                tid !== gameThreadId) {
                                logBoth('[!! THREAD] ' + vtableLabel +
                                        ' slot ' + slotIdx +
                                        ' fired on tid=' + tid +
                                        ' (gameThreadId=' +
                                        gameThreadId + ')');
                            }
                        }
                    }
                });
            })(slot, label, methodAddr, methodGhidraVA, key);
            logFile('[HOOK] ' + label + ' slot ' + slot +
                    ' -> 0x' + methodGhidraVA.toString(16));
            installedSlots++;
        } catch (e) {
            logFile('[!] hook fail: ' + label + ' slot ' + slot +
                    ': ' + e.message);
        }
    }
    logBoth('[VTABLE HOOKED] ' + label + ' slots=' + installedSlots);
}

function tryInstallKnownVtables() {
    if (knownVtablesInstalled) return;
    knownVtablesInstalled = true;
    [RVA_IWINDOWEVENT_VTABLE, RVA_FFESCMENU_OVERRIDE,
     RVA_IGGY_VTABLE].forEach(function (rvaOff) {
        const vt = rva(rvaOff);
        if (!isInModule(vt)) {
            logBoth('[!] known vtable VA=0x' +
                    (rvaOff + 0x400000).toString(16) +
                    ' out of module — skip');
            return;
        }
        installVtableHooks(vt);
    });
}

// ── FUN_00625e30 — Phyre IWindowEvent registration entry (conf 0.7) ──
// 38-byte forwarder; param_1 = IWindowEvent subobject pointer at
// &stack0x00000004 (first stack arg). subobj[0] = vtable.
try {
    Interceptor.attach(rva(RVA_FUN_00625e30), {
        onEnter: function (args) {
            try {
                const subobjPtr = ptr(this.context.esp).add(4)
                                                      .readPointer();
                if (!isInModule(subobjPtr)) {
                    logFile('[REG] subobj=' + subobjPtr +
                            ' OUT_OF_MODULE');
                    return;
                }
                const vtable = subobjPtr.readPointer();
                if (!isInModule(vtable)) {
                    logFile('[REG] subobj=0x' +
                            subobjPtr.toString(16) +
                            ' vtable=' + vtable + ' OOB');
                    return;
                }
                const key = vtable.toString();
                if (!hookedVtables[key]) {
                    pendingHooks.push(vtable);
                    logFile('[REG] subobj=0x' +
                            subobjPtr.toString(16) +
                            ' vtable=0x' +
                            toGhidra(vtable).toString(16) +
                            ' (queued)');
                }
            } catch (e) {
                logFile('[REG] err: ' + e.message);
            }
        }
    });
    logBoth('[OK] FUN_00625e30 hooked (RVA 0x' +
            RVA_FUN_00625e30.toString(16) + ')');
} catch (e) {
    logBoth('[!] FUN_00625e30 hook fail: ' + e.message);
}

// ── Keyboard hotkeys (proven pattern, action_stalker_probe.js) ──
let fnGetAsyncKeyState = null;
let fnGetForegroundWindow = null;
let fnGetWindowThreadProcessId = null;
const myPid = Process.id;
const pidBuf = Memory.alloc(4);
const keyWasDown = {};
Process.enumerateModules().forEach(function (m) {
    if (m.name.toLowerCase() === 'user32.dll') {
        m.enumerateExports().forEach(function (exp) {
            if (exp.name === 'GetAsyncKeyState')
                fnGetAsyncKeyState = new NativeFunction(
                    exp.address, 'int16', ['int']);
            if (exp.name === 'GetForegroundWindow')
                fnGetForegroundWindow = new NativeFunction(
                    exp.address, 'pointer', []);
            if (exp.name === 'GetWindowThreadProcessId')
                fnGetWindowThreadProcessId = new NativeFunction(
                    exp.address, 'uint32', ['pointer', 'pointer']);
        });
    }
});
function isGameFocused() {
    if (!fnGetForegroundWindow || !fnGetWindowThreadProcessId)
        return true;
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
const VK_F8 = 0x77, VK_F9 = 0x78, VK_F10 = 0x79;
const VK_F11 = 0x7A, VK_F12 = 0x7B;

// ── Frame hook: capture thread, drain pending hook queue, poll keys ──
Interceptor.attach(rva(RVA_FRAME_HOOK), {
    onLeave: function () {
        if (gameThreadId === null) {
            gameThreadId = Process.getCurrentThreadId();
            logBoth('[*] game thread=' + gameThreadId);
            tryInstallKnownVtables();
        }
        frameCount++;
        while (pendingHooks.length > 0) {
            installVtableHooks(pendingHooks.shift());
        }
        if (keyPressed(VK_F8))  cmdStart();
        if (keyPressed(VK_F9))  cmdLabel('Items');
        if (keyPressed(VK_F10)) cmdLabel('Equip');
        if (keyPressed(VK_F11)) cmdLabel('Abilities');
        if (keyPressed(VK_F12)) cmdDump();
        if (frameCount % 600 === 0) {
            logFile('[HEARTBEAT] frames=' + frameCount +
                    ' state=' + state +
                    ' label=' + currentLabel +
                    ' hookedVtables=' +
                    Object.keys(hookedVtables).length);
        }
    }
});

function cmdStart() {
    state = 'observing';
    currentLabel = '(none)';
    logBoth('[OBSERVE START] hookedVtables=' +
            Object.keys(hookedVtables).length +
            ' — open Items, arrow, F9 → Equip, arrow, F10 → ' +
            'Abilities, arrow, F11 → F12 dump');
}

function cmdLabel(label) {
    if (state !== 'observing') {
        logBoth('[!] not observing — press F8 first');
        return;
    }
    currentLabel = label;
    logBoth('[LABEL] ' + label);
}

function cmdDump() {
    if (state !== 'observing') {
        logBoth('[!] not observing — press F8 first');
        return;
    }
    state = 'idle';
    logBoth('[DUMP] hookedVtables=' +
            Object.keys(hookedVtables).length +
            '  fireEntries=' + Object.keys(fireCounts).length);

    const byVtableSlot = {};
    for (const k in fireCounts) {
        const parts = k.split(':');
        const vKey = parts[0], slot = parts[1], label = parts[2];
        const vsKey = vKey + '|' + slot;
        if (!byVtableSlot[vsKey]) byVtableSlot[vsKey] = {};
        byVtableSlot[vsKey][label] = {
            count: fireCounts[k],
            distinct: Object.keys(distinctEcx[k] || {}).length
        };
    }

    logFile('');
    logFile('=== DISPATCHER VTABLE PROBE — DUMP ===');
    for (const vsKey in byVtableSlot) {
        const parts = vsKey.split('|');
        const vAddr = ptr(parts[0]);
        const slot = parts[1];
        const vLabel = KNOWN_VTABLE_LABELS[parts[0]] ||
                       ('disc_0x' + toGhidra(vAddr).toString(16));
        logFile('[SLOT] ' + vLabel + ' slot=' + slot +
                ' (vtable VA 0x' + toGhidra(vAddr).toString(16) + ')');
        for (const label in byVtableSlot[vsKey]) {
            const e = byVtableSlot[vsKey][label];
            logFile('    label=' + label +
                    '  fires=' + e.count +
                    '  distinct_this=' + e.distinct);
        }
    }
    logFile('=== END ===');

    // Console: which slots fired across all 3 menu labels
    const universal = [];
    for (const vsKey in byVtableSlot) {
        const labels = byVtableSlot[vsKey];
        if (labels.Items && labels.Equip && labels.Abilities) {
            const parts = vsKey.split('|');
            const vAddr = ptr(parts[0]);
            const vLabel = KNOWN_VTABLE_LABELS[parts[0]] ||
                           ('disc_0x' +
                            toGhidra(vAddr).toString(16));
            universal.push(vLabel + '/slot' + parts[1]);
        }
    }
    logBoth('[UNIVERSAL] (vtable, slot) fired for Items+Equip+' +
            'Abilities: ' +
            (universal.length > 0 ? universal.join(', ') : '(none)'));
    logBoth('[*] full breakdown in ' + LOG_PATH);
}

logBoth('[*] Ready. F8 to start. F9=Items F10=Equip F11=Abilities ' +
        'F12=dump.');

// ── Shutdown ──
rpc.exports = {
    dispose: function () {
        try { logFile('[DISPOSE] frames=' + frameCount); } catch (e) {}
        try { logHandle.close(); } catch (e) {}
    }
};
