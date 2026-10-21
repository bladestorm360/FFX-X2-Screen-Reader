// @desc Hook PhyreEngine's central event dispatcher — the ProcessEvent equivalent
//
// FUN_00627630 (RVA 0x227630, 310 bytes) is the central dispatcher.
// It parses commands via FUN_00627890, matches event names against the callback
// array (stride 0x48), and calls the matching handler.
//
// Also hooks FUN_0062c8a0 (RVA 0x22c8a0, its caller) to verify the chain fires.
// Also hooks individual handlers to see if they receive events.
//
// Console: first occurrence of each event type, then ONLY input events.
// File: everything.
//
// Press d-pad or keyboard on title screen. Ctrl+C to stop.

var logPath = 'D:/Games/Dev/Custom/FFX/ffx-x2-screen-reader/Docs/Scripts/Frida/output/FFX/hook_dispatcher.log';
var logHandle = new File(logPath, 'w');

function logFile(msg) {
    logHandle.write(msg + '\n');
    logHandle.flush();
}

function logBoth(msg) {
    console.log(msg);
    logFile(msg);
}

function tryReadCString(p) {
    try {
        if (p.isNull()) return null;
        var s = p.readCString();
        if (s && s.length > 0 && s.length < 4096) return s;
    } catch(e) {}
    return null;
}

// ── RVA addressing ──
var base = Process.enumerateModules()[0].base;
function rva(offset) { return base.add(offset); }
function toGhidra(addr) { return ptr(addr).sub(base).add(0x00400000); }

logBoth('[*] hook_dispatcher (ProcessEvent equivalent)');
logBoth('[*] base=0x' + base.toString(16));

var seenEvents = {};
var dispatchCount = 0;
var inputEvents = ['buttonpress', 'buttondown', 'buttonup', 'keypress', 'keydown', 'keyup', 'mousepress', 'inputsource'];

// ── Hook 1: FUN_00627630 (RVA 0x227630) — THE CENTRAL DISPATCHER ──
try {
    Interceptor.attach(rva(0x227630), {
        onEnter: function(args) {
            dispatchCount++;
            this.ecx = ptr(this.context.ecx);

            // Read the callback array to see what's registered
            if (dispatchCount === 1) {
                var arrayPtr = this.ecx.add(0x14).readPointer();
                var count = this.ecx.add(0x18).readU32();
                logBoth('[DISPATCHER] ECX=0x' + this.ecx.toString(16) +
                    ' callbacks=' + count);

                // Dump all registered event names
                for (var i = 0; i < count && i < 20; i++) {
                    var entryBase = arrayPtr.add(i * 0x48);
                    var funcPtr = entryBase.readPointer();
                    var name = tryReadCString(entryBase.add(4));
                    var ghidra = toGhidra(funcPtr);
                    logBoth('[REG] ' + (name || '?') + ' -> 0x' + ghidra.toString(16));
                }
            }

            // After FUN_00627890 parses the command, local_8 and local_c
            // are on the stack. We can't easily read stack locals from onEnter.
            // Instead, we'll read from the callback array match on onLeave.
        },
        onLeave: function(retval) {
            // Log periodic frame count
            if (dispatchCount % 500 === 0) {
                logFile('[FRAME] dispatch count: ' + dispatchCount);
            }
        }
    });
    logBoth('[OK] Dispatcher (RVA 0x227630)');
} catch(e) {
    logBoth('[!] Dispatcher: ' + e.message);
}

// ── Hook 2: FUN_0062c8a0 (RVA 0x22c8a0) — caller of dispatcher ──
var callerCount = 0;
try {
    Interceptor.attach(rva(0x22c8a0), {
        onEnter: function(args) {
            callerCount++;
            if (callerCount === 1) {
                logBoth('[CALLER] FUN_0062c8a0 fires. ECX=0x' +
                    ptr(this.context.ecx).toString(16));
            }
        }
    });
    logBoth('[OK] Dispatcher caller (RVA 0x22c8a0)');
} catch(e) {
    logBoth('[!] Caller: ' + e.message);
}

// ── Hook 3-8: Individual event handlers ──
// These are called BY the dispatcher with (context, paramString)
// Hook them to see the actual event name and parameter when they fire

var handlers = [
    {rva: 0x223220, name: 'Buttonpress'},
    {rva: 0x222f00, name: 'ButtonDown'},
    {rva: 0x223090, name: 'ButtonUp'},
    {rva: 0x224a50, name: 'Keypress'},
    {rva: 0x2249d0, name: 'KeyDown'},
    {rva: 0x224a10, name: 'KeyUp'},
    {rva: 0x224ad0, name: 'Mousepress'},
    {rva: 0x2245d0, name: 'InputSource'},
];

for (var h = 0; h < handlers.length; h++) {
    (function(handler) {
        try {
            Interceptor.attach(rva(handler.rva), {
                onEnter: function(args) {
                    // args[0] = context (in_ECX + 0x20 from dispatcher)
                    // args[1] = parameter string (local_c from dispatcher)
                    var param = tryReadCString(args[1]);
                    var ctx = args[0];

                    var msg = '[EVENT] ' + handler.name +
                        (param ? '(' + param + ')' : '') +
                        ' ctx=0x' + ctx.toString(16);

                    // Console: ALWAYS for input events
                    logBoth(msg);

                    // Backtrace on first call
                    if (!seenEvents[handler.name]) {
                        seenEvents[handler.name] = true;
                        var bt = Thread.backtrace(this.context, Backtracer.ACCURATE);
                        var btGhidra = [];
                        for (var i = 0; i < Math.min(bt.length, 5); i++) {
                            btGhidra.push('0x' + toGhidra(bt[i]).toString(16));
                        }
                        logBoth('[TRACE] ' + handler.name + ' <- ' +
                            btGhidra.join(' <- '));
                    }
                }
            });
            logBoth('[OK] ' + handler.name + ' (RVA 0x' + handler.rva.toString(16) + ')');
        } catch(e) {
            logBoth('[!] ' + handler.name + ': ' + e.message);
        }
    })(handlers[h]);
}

// ── Hook 9: FUN_00627890 (command parser) — read event name directly ──
try {
    Interceptor.attach(rva(0x227890), {
        onLeave: function(retval) {
            // After parsing, the event name and param are in the caller's stack
            // We can't read them from here directly, but retval tells us if parsing succeeded
            // retval != 0 means a command was parsed
            if (retval.toInt32() !== 0) {
                logFile('[PARSE] Command parsed (retval=' + retval.toInt32() + ')');
            }
        }
    });
    logBoth('[OK] Command parser (RVA 0x227890)');
} catch(e) {
    logBoth('[!] Parser: ' + e.message);
}

logBoth('[*] All hooks set. Press d-pad or keyboard arrows.');
logBoth('[*] Input events will appear as: [EVENT] Buttonpress(Down)');
