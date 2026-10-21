// @desc Find ALL IggyMenu instances — hook multiple methods, identify by vtable
//
// v4: The opening screen's IggyMenu doesn't dispatch through FUN_00a28c10.
// Hook multiple IggyMenu/iggy methods to find ALL active instances.
// For each, check vtable and whether value path (+0x08) is initialized.
//
// Console: discovered instances, Flash values
// File: everything
//
// Attach on title screen. Press arrows. Ctrl+C to stop.

var logPath = 'D:/Games/Dev/Custom/FFX/ffx-x2-screen-reader/Docs/Scripts/Frida/output/FFX/read_iggy_menu.log';
var logHandle = new File(logPath, 'w');
function logFile(msg) { logHandle.write(msg + '\n'); logHandle.flush(); }
function logBoth(msg) { console.log(msg); logFile(msg); }

var base = Process.enumerateModules()[0].base;
function rva(offset) { return base.add(offset); }
function toGhidra(addr) { return ptr(addr).sub(base).add(0x00400000); }

logBoth('[*] read_iggy_menu v4 — find ALL instances');
logBoth('[*] base=0x' + base.toString(16));

// Known IggyMenu vtable addresses (Ghidra VAs)
var VTABLE_IGGY_MENU = 0x00b78c54;
var VTABLE_FFESCMENU_1 = 0x00b46c28;
var VTABLE_FFESCMENU_2 = 0x00b46c34;
var VTABLE_FFIGGY_PAUSE = 0x00b4bc0c;

var discoveredInstances = {}; // address string → {ptr, vtable, name, hasValuePath}

function identifyVtable(ghidraVtable) {
    var v = ghidraVtable.toInt32();
    if (v === VTABLE_IGGY_MENU) return 'IggyMenu';
    if (v === VTABLE_FFESCMENU_1 || v === VTABLE_FFESCMENU_2) return 'FFEscMenu';
    if (v === VTABLE_FFIGGY_PAUSE) return 'FFIggyPauseMenu';
    return 'Unknown(0x' + ghidraVtable.toString(16) + ')';
}

function registerInstance(ecxPtr, source) {
    var key = ecxPtr.toString(16);
    if (discoveredInstances[key]) return; // already known

    try {
        var vtable = ecxPtr.readPointer();
        var gv = toGhidra(vtable);
        var name = identifyVtable(gv);
        var valuePath = ecxPtr.add(0x08).readPointer();
        var hasPath = !valuePath.isNull();

        discoveredInstances[key] = {ptr: ecxPtr, vtable: gv, name: name, hasPath: hasPath};
        logBoth('[INSTANCE] 0x' + key + ' = ' + name +
            ' valuePath=' + (hasPath ? '0x' + valuePath.toString(16) : 'NULL') +
            ' (from ' + source + ')');

        // If this has a valid value path, try reading Flash values
        if (hasPath) {
            logBoth('[*] Found instance with initialized value path! Querying...');
            queryFlashValues(ecxPtr);
        }
    } catch(e) {
        logFile('[INSTANCE] 0x' + key + ' read failed: ' + e.message);
    }
}

// ── Hook 1: FUN_00a28c10 — IggyMenu::dispatchEvent ──
try {
    Interceptor.attach(rva(0x628C10), {
        onEnter: function(args) {
            registerInstance(ptr(this.context.ecx), 'dispatchEvent');
        }
    });
    logBoth('[OK] dispatchEvent hooked (0x628C10)');
} catch(e) { logBoth('[!] dispatchEvent: ' + e.message); }

// ── Hook 2: FUN_00a26ce0 — IggyMenu::processEvents ──
try {
    Interceptor.attach(rva(0x626CE0), {
        onEnter: function(args) {
            registerInstance(ptr(this.context.ecx), 'processEvents');
        }
    });
    logBoth('[OK] processEvents hooked (0x626CE0)');
} catch(e) { logBoth('[!] processEvents: ' + e.message); }

// ── Hook 3: FUN_00a265f0 — IggyMenu::initListeners ──
try {
    Interceptor.attach(rva(0x6265F0), {
        onEnter: function(args) {
            registerInstance(ptr(this.context.ecx), 'initListeners');
        }
    });
    logBoth('[OK] initListeners hooked (0x6265F0)');
} catch(e) { logBoth('[!] initListeners: ' + e.message); }

// ── Hook 4: FUN_00a28ba0 — IggyMenu::callFlashFunction ──
var callCount = 0;
try {
    Interceptor.attach(rva(0x628BA0), {
        onEnter: function(args) {
            registerInstance(ptr(this.context.ecx), 'callFlash');
            callCount++;
            try {
                var name = args[1].readCString();
                if (callCount <= 30) logBoth('[FLASH] "' + name + '"');
                logFile('[FLASH #' + callCount + '] "' + name + '"');
            } catch(e) {}
        }
    });
    logBoth('[OK] callFlash hooked (0x628BA0)');
} catch(e) { logBoth('[!] callFlash: ' + e.message); }

// ── Hook 5: FUN_00a28e90 — IggyMenu::registerCallback ──
try {
    Interceptor.attach(rva(0x628E90), {
        onEnter: function(args) {
            registerInstance(ptr(this.context.ecx), 'regCallback');
            try {
                var name = args[0].readCString();
                var cb = args[1];
                logBoth('[REG_CB] "' + name + '" -> Ghidra 0x' + toGhidra(cb).toString(16));
            } catch(e) {}
        }
    });
    logBoth('[OK] regCallback hooked (0x628E90)');
} catch(e) { logBoth('[!] regCallback: ' + e.message); }

// ── Hook 6: FUN_00a28ef0 — IggyMenu::setEventEnabled ──
try {
    Interceptor.attach(rva(0x628EF0), {
        onEnter: function(args) {
            registerInstance(ptr(this.context.ecx), 'setEventEnabled');
        }
    });
    logBoth('[OK] setEventEnabled hooked (0x628EF0)');
} catch(e) { logBoth('[!] setEventEnabled: ' + e.message); }

// ── Hook 7: FUN_00a28b20 — IggyMenu::readInt ──
try {
    Interceptor.attach(rva(0x628B20), {
        onEnter: function(args) {
            this.ecx = ptr(this.context.ecx);
            this.idx = args[0].toInt32();
        },
        onLeave: function(retval) {
            logBoth('[READ_INT] 0x' + this.ecx.toString(16) +
                ' idx=' + this.idx + ' -> ' + retval.toInt32());
        }
    });
    logBoth('[OK] readInt hooked');
} catch(e) {}

// ── Hook 8: FUN_00a28b60 — IggyMenu::readString ──
try {
    Interceptor.attach(rva(0x628B60), {
        onEnter: function(args) {
            this.ecx = ptr(this.context.ecx);
            this.idx = args[0].toInt32();
        },
        onLeave: function(retval) {
            try {
                logBoth('[READ_STR] 0x' + this.ecx.toString(16) +
                    ' idx=' + this.idx + ' -> "' + retval.readCString() + '"');
            } catch(e) {
                logBoth('[READ_STR] 0x' + this.ecx.toString(16) +
                    ' idx=' + this.idx + ' -> 0x' + retval.toString(16));
            }
        }
    });
    logBoth('[OK] readString hooked');
} catch(e) {}

// ── Also read DAT_01984c6c — checked in iggy tick, might be a menu pointer ──
try {
    var dat = rva(0x1584C6C); // DAT_01984c6c RVA
    var datVal = dat.readPointer();
    logBoth('[GLOBAL] DAT_01984c6c = 0x' + datVal.toString(16));
    if (!datVal.isNull()) {
        try {
            var datVtable = datVal.readPointer();
            logBoth('[GLOBAL] -> vtable Ghidra 0x' + toGhidra(datVtable).toString(16));
        } catch(e) {}
    }
} catch(e) {
    logBoth('[GLOBAL] DAT_01984c6c unreadable: ' + e.message);
}

// ── Query Flash values ──
function queryFlashValues(menuPtr) {
    logBoth('[QUERY] Reading Flash values from 0x' + menuPtr.toString(16));
    try {
        var readInt = new NativeFunction(rva(0x628B20), 'int', ['pointer', 'int'], 'thiscall');
        for (var idx = 0; idx < 10; idx++) {
            try {
                var result = readInt(menuPtr, idx);
                logBoth('[QUERY] readInt(' + idx + ') = ' + result);
            } catch(e) {
                logBoth('[QUERY] readInt(' + idx + ') FAILED: ' + e.message);
                break;
            }
        }
    } catch(e) {
        logBoth('[QUERY] Failed: ' + e.message);
    }
}

logBoth('[*] Attach on title screen. Press arrows.');
logBoth('[*] Watching 8 IggyMenu methods + 1 global for any instance...');
