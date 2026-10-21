// @desc Read title screen vtable handler + backtrace sound events + dump cursor state
//
// Uses RVAs (offsets from module base) for ALL addresses. ASLR-safe.
// RVA = Ghidra VA - 0x00400000 (Ghidra image base)
//
// Hooks:
//   FUN_0066b470 (RVA 0x26b470) — game tick, reads vtable handler on first call
//   FUN_00886de0 (RVA 0x486de0) — sound event, backtrace every call
//   FUN_00623d50 (RVA 0x223d50) — get game object, captures return value
//
// Navigate title screen. Ctrl+C to stop.

var logPath = 'D:/Games/Dev/Custom/FFX/ffx-x2-screen-reader/Docs/Scripts/Frida/output/FFX/read_vtable_handler.log';
var logHandle = new File(logPath, 'w');

function logFile(msg) {
    logHandle.write(msg + '\n');
    logHandle.flush();
}

function logBoth(msg) {
    console.log(msg);
    logFile(msg);
}

// ── RVA-based address resolution (ASLR-safe) ──
var base = Process.enumerateModules()[0].base;
var modSize = Process.enumerateModules()[0].size;

function rva(offset) {
    return base.add(offset);
}

// Convert runtime address to Ghidra VA for decompile lookup
function toGhidra(addr) {
    return ptr(addr).sub(base).add(0x00400000);
}

logBoth('[*] read_vtable_handler');
logBoth('[*] base=0x' + base.toString(16) + ' size=0x' + modSize.toString(16));

// ── Read initial game state ──
var gameStateRVA = 0xEFBBF8;      // DAT_012fbbf8
var gameObjPtrRVA = 0x8CB9D8;     // DAT_00ccb9d8
var menuArrayRVA = 0x1606108;     // DAT_01a86108
var cursorIdxRVA = 0x13E1be8;     // DAT_01841be8
var itemTableRVA = 0x13E1bd4;     // DAT_01841bd4
var menuObjPtrRVA = 0x1F05834;    // DAT_02305834

// Verify RVAs are within module bounds
function checkRVA(name, offset) {
    if (offset >= modSize) {
        logBoth('[!] ' + name + ' RVA 0x' + offset.toString(16) + ' exceeds module size 0x' + modSize.toString(16));
        return false;
    }
    return true;
}

checkRVA('gameState', gameStateRVA);
checkRVA('gameObjPtr', gameObjPtrRVA);
checkRVA('menuArray', menuArrayRVA);
checkRVA('cursorIdx', cursorIdxRVA);
checkRVA('menuObjPtr', menuObjPtrRVA);

// Read game state
try {
    var gameState = rva(gameStateRVA).readU32();
    logBoth('[*] Game state: 0x' + gameState.toString(16) + (gameState === 0x17 ? ' (TITLE SCREEN)' : ''));
} catch(e) {
    logBoth('[!] Cannot read game state: ' + e.message);
}

// Read game object
var gameObj = ptr(0);
try {
    gameObj = rva(gameObjPtrRVA).readPointer();
    logBoth('[*] Game object: 0x' + gameObj.toString(16));
} catch(e) {
    logBoth('[!] Cannot read game object: ' + e.message);
}

// Read menu object pointer
try {
    var menuObjVal = rva(menuObjPtrRVA).readPointer();
    logBoth('[*] Menu obj (DAT_02305834): 0x' + menuObjVal.toString(16) + (menuObjVal.isNull() ? ' (NULL — not initialized)' : ''));
} catch(e) {
    logBoth('[!] Cannot read menu obj: ' + e.message);
}

// Read cursor index
try {
    var cursorIdx = rva(cursorIdxRVA).readU32();
    logBoth('[*] Cursor index (DAT_01841be8): ' + cursorIdx);
} catch(e) {
    logBoth('[!] Cannot read cursor index: ' + e.message);
}

// ── Hook 1: FUN_0066b470 (game tick) — read vtable handler ──
var tickCount = 0;
var capturedGameObj = null;

try {
    Interceptor.attach(rva(0x26b470), {
        onEnter: function(args) {
            tickCount++;
            if (tickCount > 3) return; // only log first few

            // FUN_0066b470 starts with iVar1 = FUN_00623d50()
            // The game object is returned in EAX from FUN_00623d50
            // We can't easily get it here — capture from FUN_00623d50 hook instead
        },
        onLeave: function(retval) {
            if (tickCount > 3) return;
            logFile('[TICK #' + tickCount + '] returned ' + retval.toInt32());
        }
    });
    logBoth('[OK] Game tick (RVA 0x26b470)');
} catch(e) {
    logBoth('[!] Game tick: ' + e.message);
}

// ── Hook 2: FUN_00623d50 (get game object) — capture return value ──
var getObjCount = 0;

try {
    Interceptor.attach(rva(0x223d50), {
        onLeave: function(retval) {
            getObjCount++;
            if (capturedGameObj === null && !retval.isNull()) {
                capturedGameObj = retval;
                logBoth('[GameObj] Captured: 0x' + retval.toString(16));

                // Read the vtable handler: *(*(gameObj + 0x124) + 8)
                try {
                    var vtableBase = retval.add(0x124).readPointer();
                    var handler = vtableBase.add(8).readPointer();
                    var ghidraVA = toGhidra(handler);
                    logBoth('[HANDLER] vtable+8 -> 0x' + handler.toString(16) +
                        ' (Ghidra: 0x' + ghidraVA.toString(16) + ')');
                    logBoth('[HANDLER] Look up this Ghidra VA in the decompile output!');
                } catch(e) {
                    logFile('[!] Cannot read vtable handler: ' + e.message);
                }

                // Also dump some state from the game object
                try {
                    var titleFlag = retval.add(0x10d50).readU8();
                    var menuFlag = retval.add(0xf9c6).readU8();
                    var swfPtr = retval.add(0x10dc8).readPointer();
                    logBoth('[GameObj] titleFlag(+0x10d50)=' + titleFlag +
                        ' menuFlag(+0xf9c6)=' + menuFlag +
                        ' swfPtr(+0x10dc8)=0x' + swfPtr.toString(16));
                } catch(e) {
                    logFile('[!] Cannot read game obj fields: ' + e.message);
                }
            }
        }
    });
    logBoth('[OK] Get game object (RVA 0x223d50)');
} catch(e) {
    logBoth('[!] Get game object: ' + e.message);
}

// ── Hook 3: FUN_00886de0 (sound event) — backtrace every call ──
var soundCount = 0;
var seenTraces = {};

try {
    Interceptor.attach(rva(0x486de0), {
        onEnter: function(args) {
            soundCount++;
            var eventId = args[0].toUInt32();
            var eventHex = '0x' + eventId.toString(16);

            var bt = Thread.backtrace(this.context, Backtracer.ACCURATE);
            var btGhidra = [];
            for (var i = 0; i < bt.length; i++) {
                var gva = toGhidra(bt[i]);
                btGhidra.push('0x' + gva.toString(16));
            }

            logFile('[SOUND #' + soundCount + '] event=' + eventHex);
            logFile('  backtrace: ' + btGhidra.join(' <- '));

            // Console: unique traces only
            var traceKey = eventHex + ':' + btGhidra.slice(0, 3).join(',');
            if (!seenTraces[traceKey]) {
                seenTraces[traceKey] = true;
                logBoth('[SOUND] ' + eventHex + ' <- ' + btGhidra.slice(0, 4).join(' <- '));
            }
        }
    });
    logBoth('[OK] Sound event (RVA 0x486de0)');
} catch(e) {
    logBoth('[!] Sound event: ' + e.message);
}

logBoth('[*] All hooks set. Navigate title screen (up/down/confirm/back). Ctrl+C to stop.');
