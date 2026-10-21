// @desc IWindowEvent vtable introspection probe.
//
// Premise: PhyreEngine menus inherit from Phyre::PFramework::IWindowEvent.
// The decompile confirms FFEscMenu sets IWindowEvent::vftable at object
// offset +0x9C, meaning game menus are polymorphic IWindowEvent instances.
// The framework dispatches events via virtual method calls through those
// vtables; grep can't see those calls because they're indirect through
// `this` pointers.
//
// This probe reads the vtable entries at KNOWN RVAs (not scanning memory,
// not pattern-matching — just reading 4-byte function pointers at fixed
// locations in the module image), Interceptor.attaches a call-counter to
// each, and logs which virtual methods fire during gameplay. Frequent
// callers are per-frame update / render methods (skip). Rare callers
// that correlate with specific user actions (cursor move, window open /
// close, focus change) are the event hooks we want.
//
// Usage: run via run_frida.bat -> FFX -> iwindowevent_vtable_probe.
// During the probe:
//   1. Let the title screen load (~10 seconds).
//   2. Open the main menu (triangle or ESC).
//   3. Move cursor up / down a few times.
//   4. Open a submenu.
//   5. Close back to field.
//   6. (Optional) Start a battle, use a battle menu.
// Ctrl+C to stop the probe.
//
// Output: Docs/Scripts/Frida/output/FFX/iwindowevent_vtable_probe.log
//   Per-entry hit counts + sample args for rare-firing entries.

const LOG_PATH = 'D:/Games/Dev/Custom/FFX/ffx-x2-screen-reader/Docs/Scripts/Frida/output/FFX/iwindowevent_vtable_probe.log';
const logHandle = new File(LOG_PATH, 'w');
function logFile(msg) { try { logHandle.write(msg + '\n'); logHandle.flush(); } catch (e) {} }
function logBoth(msg) { console.log(msg); logFile(msg); }

const base = Process.enumerateModules()[0].base;
function rva(offset) { return base.add(offset); }

logBoth('[*] IWindowEvent vtable probe');
logBoth('[*] base=0x' + base.toString(16));

// Vtable RVAs from rtti_classes.txt (Ghidra VA - 0x400000):
//   Phyre::PFramework::IWindowEvent::vftable         @ Ghidra 0x00b46c1c -> RVA 0x746c1c
//   FFEscMenu::vftable                               @ Ghidra 0x00b46c28 -> RVA 0x746c28
//   FFEscMenu::vftable (secondary, for IWindowEvent) @ Ghidra 0x00b46c34 -> RVA 0x746c34
//
// Multiple-inheritance-style layout: the secondary vtable (0x746c34) is
// the IWindowEvent-interface slice. That's where the event dispatch
// methods live. Probe both primary and secondary.

const VTABLES = [
    { name: 'IWindowEvent',          rva: 0x746c1c, slotCount: 16 },
    { name: 'FFEscMenu.main',        rva: 0x746c28, slotCount: 16 },
    { name: 'FFEscMenu.IWindowEvt',  rva: 0x746c34, slotCount: 16 }
];

const hits = {};  // key "<vtable>[i]" -> count
const firstArgs = {}; // key -> sample first arg (Int32 hex)
const SAMPLE_COUNT = 3;

function attachVtableSlot(vtableName, slotIdx, fnPtr) {
    const key = vtableName + '[' + slotIdx + ']';
    try {
        Interceptor.attach(fnPtr, {
            onEnter: function(args) {
                hits[key] = (hits[key] || 0) + 1;
                if (!firstArgs[key]) firstArgs[key] = [];
                if (firstArgs[key].length < SAMPLE_COUNT) {
                    const argSample = [];
                    for (let i = 0; i < 4; i++) {
                        try {
                            const a = args[i];
                            argSample.push('0x' + a.toInt32().toString(16));
                        } catch (e) { argSample.push('?'); }
                    }
                    firstArgs[key].push(argSample.join(','));
                }
            }
        });
    } catch (e) {
        logFile('[!] attach ' + key + ' fn=0x' + fnPtr.sub(base).toInt32().toString(16) + ' failed: ' + e.message);
    }
}

VTABLES.forEach(function(vt) {
    const vtBase = rva(vt.rva);
    logFile('[*] ' + vt.name + ' vtable @ RVA 0x' + vt.rva.toString(16) + ' (abs 0x' + vtBase.toString(16) + ')');
    for (let i = 0; i < vt.slotCount; i++) {
        try {
            const fnPtr = vtBase.add(i * 4).readPointer();
            if (fnPtr.isNull()) continue;
            const fnRva = fnPtr.sub(base).toInt32();
            // Sanity: reject obviously invalid fn pointers (module is ~11 MB).
            if (fnRva < 0 || fnRva > 0x2000000) continue;
            logFile('  [' + i + '] fn RVA 0x' + fnRva.toString(16));
            attachVtableSlot(vt.name, i, fnPtr);
        } catch (e) {
            logFile('  [' + i + '] read failed: ' + e.message);
        }
    }
});

// Periodic summary every ~5 seconds (counted via the per-frame hook).
const FRAME_RVA = 0x471D10;
let frameCount = 0;
const SUMMARY_EVERY = 300; // ~5s at 60fps

function dumpSummary() {
    logFile('');
    logFile('=== HIT SUMMARY @ frame ' + frameCount + ' ===');
    const keys = Object.keys(hits).sort(function(a, b) { return hits[a] - hits[b]; });
    // Low-frequency first — those are event hooks. High-frequency last — render/update.
    for (let i = 0; i < keys.length; i++) {
        const k = keys[i];
        const samples = firstArgs[k] ? (' samples=[' + firstArgs[k].join(' | ') + ']') : '';
        logFile('  ' + k + ' = ' + hits[k] + samples);
    }
}

Interceptor.attach(rva(FRAME_RVA), {
    onLeave: function() {
        frameCount++;
        if (frameCount % SUMMARY_EVERY === 0) dumpSummary();
    }
});

logBoth('[*] Probe installed. Play the game to exercise menus, battles, cursor moves.');
logBoth('[*] Summary written to log every ~5s. Ctrl+C to stop.');
