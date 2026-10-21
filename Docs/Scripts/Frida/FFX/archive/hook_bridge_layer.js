// @desc Hook Iggy bridge + game tick — capture ECX, read state, dump vtable
//
// Hooks 6 functions at the bridge layer between native code and Iggy/Flash.
// For EACH hook: captures ECX (this pointer), reads object state, logs everything.
// If a function doesn't fire, we use the game tick (which DOES fire) to dump
// the vtable and command queue to find functions that DO fire.
//
// Navigate title screen. Ctrl+C to stop.

var logPath = 'D:/Games/Dev/Custom/FFX/ffx-x2-screen-reader/Docs/Scripts/Frida/output/FFX/hook_bridge_layer.log';
var logHandle = new File(logPath, 'w');

function logFile(msg) {
    logHandle.write(msg + '\n');
    logHandle.flush();
}

function logBoth(msg) {
    console.log(msg);
    logFile(msg);
}

function safeRead(addr, readFn) {
    try { return readFn(addr); } catch(e) { return null; }
}

function safeReadPtr(addr) { return safeRead(addr, function(a) { return a.readPointer(); }); }
function safeReadU32(addr) { return safeRead(addr, function(a) { return a.readU32(); }); }
function safeReadU8(addr) { return safeRead(addr, function(a) { return a.readU8(); }); }

function hexDump(addr, len) {
    try {
        var buf = addr.readByteArray(len);
        var bytes = new Uint8Array(buf);
        var hex = '';
        for (var i = 0; i < bytes.length; i++) {
            var h = bytes[i].toString(16);
            if (h.length < 2) h = '0' + h;
            hex += h;
            if ((i + 1) % 4 === 0) hex += ' ';
        }
        return hex.trim();
    } catch(e) { return '(unreadable)'; }
}

// ── RVA addressing ──
var base = Process.enumerateModules()[0].base;
var modSize = Process.enumerateModules()[0].size;
function rva(offset) { return base.add(offset); }
function toGhidra(addr) { return ptr(addr).sub(base).add(0x00400000); }

logBoth('[*] hook_bridge_layer');
logBoth('[*] base=0x' + base.toString(16));

var counts = {};
var seen = {};
var gameObj = null;

// ── Hook 1: FUN_0066b470 (RVA 0x26b470) — Game tick, fires every frame ──
// This is our ANCHOR. It always fires. We use it to read game state.
var tickDumped = false;

try {
    Interceptor.attach(rva(0x26b470), {
        onEnter: function(args) {
            if (!counts['tick']) counts['tick'] = 0;
            counts['tick']++;

            // Only dump once (first frame)
            if (!tickDumped) {
                tickDumped = true;
                logBoth('[TICK] Game tick firing — dumping state');

                // Capture game object from FUN_00623d50 (it's called first in this function)
                // We can't call it from here, but we captured it in the other hook
                // Instead, read the game state directly
                var gameState = safeReadU32(rva(0xEFBBF8));
                logBoth('[TICK] Game state: 0x' + (gameState !== null ? gameState.toString(16) : 'null'));
            }
        }
    });
    logBoth('[OK] Game tick (RVA 0x26b470)');
} catch(e) {
    logBoth('[!] Game tick: ' + e.message);
}

// ── Hook 2: FUN_00623d50 (RVA 0x223d50) — Get game object, capture return + dump vtable ──
var vtableDumped = false;

try {
    Interceptor.attach(rva(0x223d50), {
        onLeave: function(retval) {
            if (gameObj === null && !retval.isNull()) {
                gameObj = retval;
                logBoth('[GAMEOBJ] Captured: 0x' + gameObj.toString(16));

                // Dump the full vtable at gameObj+0x124
                var vtablePtr = safeReadPtr(gameObj.add(0x124));
                if (vtablePtr) {
                    logBoth('[VTABLE] Base at gameObj+0x124 -> 0x' + vtablePtr.toString(16));
                    // Read vtable entries 0 through +0x30 (13 entries)
                    for (var i = 0; i <= 0x30; i += 4) {
                        var entry = safeReadPtr(vtablePtr.add(i));
                        if (entry) {
                            var gva = toGhidra(entry);
                            logBoth('[VTABLE] +0x' + i.toString(16) + ' -> 0x' +
                                entry.toString(16) + ' (Ghidra: 0x' + gva.toString(16) + ')');
                        }
                    }
                }

                // Dump key offsets from game object
                var offsets = [0x44, 0x4c, 0x50, 0x68, 0x6c, 0x70, 0x74, 0x78,
                               0x7c, 0x80, 0x84, 0x88, 0x8c, 0x90, 0x124, 0x154];
                for (var j = 0; j < offsets.length; j++) {
                    var off = offsets[j];
                    var val = safeReadU32(gameObj.add(off));
                    logFile('[GAMEOBJ] +0x' + off.toString(16) + ' = 0x' +
                        (val !== null ? val.toString(16) : 'null'));
                }

                // Read command queue at +0x154
                var queueIdx = safeReadU32(gameObj.add(0x154));
                logBoth('[QUEUE] Write index at +0x154: ' +
                    (queueIdx !== null ? queueIdx : 'null'));

                // Dump recent queue entries (stride 0x14, up to 16 entries)
                if (queueIdx !== null && queueIdx > 0) {
                    for (var q = 0; q < Math.min(queueIdx, 4); q++) {
                        var entryBase = gameObj.add(q * 0x14);
                        var dump = hexDump(entryBase.add(0x10), 0x14);
                        logFile('[QUEUE] entry[' + q + ']: ' + dump);
                    }
                }
            }
        }
    });
    logBoth('[OK] Get game object (RVA 0x223d50)');
} catch(e) {
    logBoth('[!] Get game object: ' + e.message);
}

// ── Hook 3: FUN_00a220b0 (RVA 0x6220b0) — Iggy bridge handler ──
try {
    Interceptor.attach(rva(0x6220b0), {
        onEnter: function(args) {
            if (!counts['bridge']) counts['bridge'] = 0;
            counts['bridge']++;

            var ecx = this.context.ecx;
            logBoth('[BRIDGE #' + counts['bridge'] + '] ECX=0x' + ecx.toString(16));

            // Read the bridge object state
            var activeFlag = safeReadU32(ptr(ecx).add(0x1c));
            var stateA = safeReadU32(ptr(ecx).add(0x44));
            var stateB = safeReadU32(ptr(ecx).add(0x20));
            var ptr68 = safeReadPtr(ptr(ecx).add(0x68));
            var ptr6c = safeReadPtr(ptr(ecx).add(0x6c));

            logFile('[BRIDGE] active(+0x1c)=' + activeFlag +
                ' stateA(+0x44)=0x' + (stateA !== null ? stateA.toString(16) : 'null') +
                ' stateB(+0x20)=0x' + (stateB !== null ? stateB.toString(16) : 'null'));

            // Double-dereference +0x68 and +0x6c to get cursor values
            if (ptr68) {
                var inner68 = safeReadPtr(ptr68);
                if (inner68) {
                    var cursor68 = safeReadU32(inner68);
                    logBoth('[BRIDGE] **(+0x68) cursor = ' + cursor68);
                }
                // Also check byte at (ptr68)[1] — the flag checked in the decompile
                var flag68 = safeReadU8(ptr68.add(4));
                logFile('[BRIDGE] *(+0x68)[1] flag = ' + flag68);
            }
            if (ptr6c) {
                var inner6c = safeReadPtr(ptr6c);
                if (inner6c) {
                    var cursor6c = safeReadU32(inner6c);
                    logBoth('[BRIDGE] **(+0x6c) cursor = ' + cursor6c);
                }
            }

            // Read input channel pointers
            for (var ch = 0x78; ch <= 0x90; ch += 4) {
                var chVal = safeReadU32(ptr(ecx).add(ch));
                if (chVal !== null && chVal !== 0) {
                    logFile('[BRIDGE] input channel +0x' + ch.toString(16) + ' = 0x' + chVal.toString(16));
                }
            }

            // Dump first 0x60 bytes of the bridge object for analysis
            logFile('[BRIDGE] hex dump +0x0: ' + hexDump(ptr(ecx), 0x60));
            logFile('[BRIDGE] hex dump +0x60: ' + hexDump(ptr(ecx).add(0x60), 0x40));
        }
    });
    logBoth('[OK] Bridge handler (RVA 0x6220b0)');
} catch(e) {
    logBoth('[!] Bridge handler: ' + e.message);
}

// ── Hook 4: FUN_00a21cf0 (RVA 0x621cf0) — Bridge routing function ──
try {
    Interceptor.attach(rva(0x621cf0), {
        onEnter: function(args) {
            if (!counts['route']) counts['route'] = 0;
            counts['route']++;
            var ecx = this.context.ecx;
            logBoth('[ROUTE #' + counts['route'] + '] ECX=0x' + ecx.toString(16) +
                ' p3=0x' + args[2].toString(16) + ' p5=0x' + args[4].toString(16));
        }
    });
    logBoth('[OK] Bridge router (RVA 0x621cf0)');
} catch(e) {
    logBoth('[!] Bridge router: ' + e.message);
}

// ── Hook 5: FUN_005b5fd0 (RVA 0x1b5fd0) — Input lock (called from game tick) ──
try {
    Interceptor.attach(rva(0x1b5fd0), {
        onEnter: function(args) {
            if (!counts['lock']) counts['lock'] = 0;
            counts['lock']++;
            if (counts['lock'] <= 3) {
                logBoth('[LOCK #' + counts['lock'] + '] arg0=' + args[0].toInt32());
            }
        }
    });
    logBoth('[OK] Input lock (RVA 0x1b5fd0)');
} catch(e) {
    logBoth('[!] Input lock: ' + e.message);
}

// ── Hook 6: FUN_005b3760 (RVA 0x1b3760) — State dispatch ──
try {
    Interceptor.attach(rva(0x1b3760), {
        onEnter: function(args) {
            if (!counts['dispatch']) counts['dispatch'] = 0;
            counts['dispatch']++;
            logBoth('[DISPATCH #' + counts['dispatch'] + '] arg0=0x' +
                args[0].toString(16) + ' arg1=' + args[1].toInt32());
        }
    });
    logBoth('[OK] State dispatch (RVA 0x1b3760)');
} catch(e) {
    logBoth('[!] State dispatch: ' + e.message);
}

logBoth('[*] All hooks set. Navigate title screen. Ctrl+C to stop.');
logBoth('[*] If bridge/route/dispatch fire: cursor data will be logged.');
logBoth('[*] If only tick/lock fire: vtable dump shows what functions ARE active.');
