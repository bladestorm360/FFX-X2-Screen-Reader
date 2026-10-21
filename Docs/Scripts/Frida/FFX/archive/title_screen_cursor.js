// @desc Title screen — hook iggy event dispatch + keyboard enqueue, enumerate iggy exports
//
// The title screen runs in a BLOCKING while loop before the main game loop.
// Cursor navigation is likely handled inside iggy/Flash. This probe:
// 1. Enumerates iggy_w32.dll exports (find variable-reading functions)
// 2. Hooks FUN_005bb2d0 (keyboard enqueue, type 0xb) — see input entering queue
// 3. Hooks _IggyPlayerDispatchEventRS_12 — see events dispatched to iggy
// 4. Hooks FUN_0066b470 (iggy tick) — confirm it fires on title screen
// 5. Hooks "Finish" callback LAB_006525b0 — detect user confirmation
//
// Console: iggy exports summary, key events, cursor changes
// File: everything including full export list
//
// Start on title screen. Press arrows / confirm. Ctrl+C to stop.

var logPath = 'D:/Games/Dev/Custom/FFX/ffx-x2-screen-reader/Docs/Scripts/Frida/output/FFX/title_screen_cursor.log';
var logHandle = new File(logPath, 'w');
function logFile(msg) { logHandle.write(msg + '\n'); logHandle.flush(); }
function logBoth(msg) { console.log(msg); logFile(msg); }

// ── RVA addressing ──
var base = Process.enumerateModules()[0].base;
function rva(offset) { return base.add(offset); }
function toGhidra(addr) { return ptr(addr).sub(base).add(0x00400000); }

logBoth('[*] title_screen_cursor v2');
logBoth('[*] base=0x' + base.toString(16));

// ── Find iggy module ──
var iggyMod = null;
var modules = Process.enumerateModules();
for (var i = 0; i < modules.length; i++) {
    if (modules[i].name.toLowerCase() === 'iggy_w32.dll') {
        iggyMod = modules[i];
        break;
    }
}

if (!iggyMod) {
    logBoth('[!] iggy_w32.dll not found');
} else {
    logBoth('[*] iggy_w32.dll at 0x' + iggyMod.base.toString(16));

    // ── Enumerate iggy exports — find input/variable-reading functions ──
    var exports = iggyMod.enumerateExports();
    var inputExports = [];
    var varExports = [];
    var allExportNames = [];

    for (var e = 0; e < exports.length; e++) {
        var name = exports[e].name;
        allExportNames.push(name);
        var nameLower = name.toLowerCase();

        if (nameLower.indexOf('key') !== -1 || nameLower.indexOf('input') !== -1 ||
            nameLower.indexOf('focus') !== -1 || nameLower.indexOf('event') !== -1 ||
            nameLower.indexOf('dispatch') !== -1 || nameLower.indexOf('makeevent') !== -1) {
            inputExports.push({name: name, addr: exports[e].address});
        }
        if (nameLower.indexOf('variable') !== -1 || nameLower.indexOf('getvar') !== -1 ||
            nameLower.indexOf('property') !== -1 || nameLower.indexOf('member') !== -1 ||
            nameLower.indexOf('getvalue') !== -1 || nameLower.indexOf('callfunction') !== -1) {
            varExports.push({name: name, addr: exports[e].address});
        }
    }

    logBoth('[IGGY] ' + exports.length + ' total exports');
    logBoth('[IGGY] Input/event related:');
    for (var j = 0; j < inputExports.length; j++) {
        logBoth('  ' + inputExports[j].name + ' @ 0x' + inputExports[j].addr.toString(16));
    }
    logBoth('[IGGY] Variable/property related:');
    for (var k = 0; k < varExports.length; k++) {
        logBoth('  ' + varExports[k].name + ' @ 0x' + varExports[k].addr.toString(16));
    }

    // Full list to file
    logFile('[IGGY] All exports:');
    for (var a = 0; a < allExportNames.length; a++) {
        logFile('  ' + allExportNames[a]);
    }

    // ── Hook _IggyPlayerDispatchEventRS_12 if found ──
    var dispatchAddr = null;
    for (var d = 0; d < exports.length; d++) {
        if (exports[d].name.indexOf('DispatchEvent') !== -1) {
            dispatchAddr = exports[d].address;
            break;
        }
    }

    if (dispatchAddr) {
        var dispatchCount = 0;
        try {
            Interceptor.attach(dispatchAddr, {
                onEnter: function(args) {
                    dispatchCount++;
                    // Log first 20 events to console, then file only
                    // args: (iggyPlayer, event_ptr, ???)
                    var msg = '[DISPATCH #' + dispatchCount + '] args: ' +
                        args[0] + ', ' + args[1] + ', ' + args[2];
                    if (dispatchCount <= 20) {
                        logBoth(msg);
                    } else if (dispatchCount % 100 === 0) {
                        logBoth('[DISPATCH] count=' + dispatchCount);
                    }
                    logFile(msg);

                    // Try to read event data at args[1]
                    try {
                        var eventPtr = args[1];
                        var eventBytes = eventPtr.readByteArray(32);
                        var bytes = new Uint8Array(eventBytes);
                        var hex = '';
                        for (var b = 0; b < bytes.length; b++) {
                            var h = bytes[b].toString(16);
                            if (h.length < 2) h = '0' + h;
                            hex += h;
                            if ((b + 1) % 4 === 0) hex += ' ';
                        }
                        logFile('[DISPATCH] event data: ' + hex);
                    } catch(e) {}
                }
            });
            logBoth('[OK] IggyPlayerDispatchEvent hooked');
        } catch(e) {
            logBoth('[!] DispatchEvent hook failed: ' + e.message);
        }
    } else {
        logBoth('[!] _IggyPlayerDispatchEventRS_12 not found in exports');
    }
}

// ── Hook FUN_005bb2d0 — keyboard enqueue (type 0xb) ──
var kbEnqueueCount = 0;
try {
    Interceptor.attach(rva(0x1BB2D0), {
        onEnter: function(args) {
            kbEnqueueCount++;
            var ecx = ptr(this.context.ecx);
            // Log first 10, then every 100
            if (kbEnqueueCount <= 10 || kbEnqueueCount % 100 === 0) {
                logBoth('[KB_ENQUEUE #' + kbEnqueueCount + '] ECX=0x' +
                    ecx.toString(16) + ' args: ' + args[0] + ', ' + args[1]);
            }
            logFile('[KB_ENQUEUE #' + kbEnqueueCount + '] ECX=0x' +
                ecx.toString(16) + ' args: ' + args[0] + ', ' + args[1]);
        }
    });
    logBoth('[OK] Keyboard enqueue hooked (FUN_005bb2d0, RVA 0x1BB2D0)');
} catch(e) {
    logBoth('[!] Keyboard enqueue hook failed: ' + e.message);
}

// ── Hook FUN_0066b470 — iggy tick (confirm it fires) ──
var iggyTickCount = 0;
try {
    Interceptor.attach(rva(0x26B470), {
        onEnter: function(args) {
            iggyTickCount++;
            if (iggyTickCount === 1) {
                logBoth('[IGGY_TICK] First call confirmed — iggy tick IS firing');
            }
            if (iggyTickCount % 500 === 0) {
                logFile('[IGGY_TICK] count=' + iggyTickCount);
            }
        }
    });
    logBoth('[OK] Iggy tick hooked (FUN_0066b470, RVA 0x26B470)');
} catch(e) {
    logBoth('[!] Iggy tick hook failed: ' + e.message);
}

// ── Hook LAB_006525b0 — "Finish" callback ──
try {
    Interceptor.attach(rva(0x2525B0), {
        onEnter: function(args) {
            logBoth('[FINISH] Callback fired! User confirmed selection.');
            logBoth('[FINISH] args: ' + args[0] + ', ' + args[1] + ', ' + args[2]);
        }
    });
    logBoth('[OK] Finish callback hooked (LAB_006525b0, RVA 0x2525B0)');
} catch(e) {
    logBoth('[!] Finish callback hook failed: ' + e.message);
}

logBoth('[*] Ready. Press arrows/confirm on title screen.');
logBoth('[*] Look up Ghidra addresses in decompile before writing next probe.');
