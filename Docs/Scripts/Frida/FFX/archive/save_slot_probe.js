// @desc Save slot discovery probe — hooks candidate functions to find save/load cursor handler
//
// Usage: open save/load menu in game, cycle through slots, then read log.
// Goal: identify which function fires on cursor change and carries slot data.

var logPath = 'D:/Games/Dev/Custom/FFX/ffx-x2-screen-reader/Docs/Scripts/Frida/output/FFX/save_slot_probe.log';
var logHandle = new File(logPath, 'w');
function logFile(msg) { logHandle.write(msg + '\n'); logHandle.flush(); }
function logBoth(msg) { console.log(msg); logFile(msg); }

var base = Process.enumerateModules()[0].base;
function rva(offset) { return base.add(offset); }

logBoth('[*] Save slot discovery probe');
logBoth('[*] Open save/load menu and cycle slots. Ctrl+C to stop.');

// ── Cursor globals (from session 3) ──
// DAT_00ce72dc (RVA 0x8E72DC): column/submenu cursor
// DAT_00ce72e0 (RVA 0x8E72E0): row/item cursor — this is the slot index
var cursorCol = rva(0x8E72DC);
var cursorRow = rva(0x8E72E0);
var prevCol = -999, prevRow = -999;

// ── Candidate functions ──
// All are best-guesses from Fahrenheit source for save/load display paths.
var CANDIDATES = [
    {rva: 0x2F26A0, name: 'setSaveDataInfo'},
    {rva: 0x2F2670, name: 'setSaveDataIconString'},
    {rva: 0x248870, name: 'SaveDataSetSaveDataInfo'},
    {rva: 0x2480E0, name: 'SaveDataGetLoc'},
    {rva: 0x302DB0, name: 'FUN_00702DB0 (setSaveLoadFocus fire)'},
    {rva: 0x2EFD90, name: 'FUN_006EFD90 (save/load state handler)'},
    {rva: 0x2EF460, name: 'FUN_006EF460 (input action reader)'},
    {rva: 0x2EF770, name: 'FUN_006EF770 (navigation handler)'}
];

var hookCounts = {};

CANDIDATES.forEach(function(c) {
    hookCounts[c.name] = 0;
    try {
        Interceptor.attach(rva(c.rva), {
            onEnter: function(args) {
                hookCounts[c.name]++;
                // Log the first 3 calls fully, then just count
                if (hookCounts[c.name] <= 3) {
                    var ecx = ptr(this.context.ecx);
                    var info = '[' + c.name + ' #' + hookCounts[c.name] + '] ecx=0x' + ecx.toString(16);
                    for (var i = 0; i < 4; i++) {
                        try { info += ' a' + i + '=0x' + args[i].toString(16); } catch(e) { break; }
                    }
                    info += ' col=' + cursorCol.readS32() + ' row=' + cursorRow.readS32();
                    logFile(info);
                    // If arg1 looks like a pointer, dump bytes
                    try {
                        var a1 = args[1];
                        if (!a1.isNull() && a1.compare(ptr(0x10000)) > 0 && a1.compare(ptr(0x7FFFFFFF)) < 0) {
                            var bytes = new Uint8Array(a1.readByteArray(64));
                            var hex = '';
                            for (var b = 0; b < 64; b++) hex += ('0'+bytes[b].toString(16)).slice(-2) + ' ';
                            logFile('  a1 bytes: ' + hex);
                        }
                    } catch(e) {}
                }
            }
        });
        logBoth('[OK] ' + c.name + ' @ RVA 0x' + c.rva.toString(16));
    } catch(e) {
        logBoth('[FAIL] ' + c.name + ': ' + e.message);
    }
});

// ── Cursor change watcher via per-frame game hook ──
// Frida injected scripts don't have setInterval/setTimeout — piggyback on a
// function that fires each frame instead.
var frameCount = 0;
try {
    Interceptor.attach(rva(0x471D10), {
        onLeave: function() {
            try {
                frameCount++;
                var c = cursorCol.readS32(), r = cursorRow.readS32();
                if (c !== prevCol || r !== prevRow) {
                    logBoth('[CURSOR] col=' + c + ' row=' + r);
                    prevCol = c; prevRow = r;
                }
                // Periodic summary of which hooks fired
                if (frameCount % 300 === 0) {
                    var active = [];
                    Object.keys(hookCounts).forEach(function(k) {
                        if (hookCounts[k] > 0) active.push(k + '=' + hookCounts[k]);
                    });
                    if (active.length > 0) logFile('[SUMMARY] fired: ' + active.join(', '));
                }
            } catch(e) {}
        }
    });
} catch(e) { logBoth('[FAIL] per-frame hook: ' + e.message); }
