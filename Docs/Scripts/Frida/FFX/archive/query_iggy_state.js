// @desc Query iggy Flash state — enumerate focusable objects, read Flash variables
//
// The title screen cursor is managed inside iggy/Flash (confirmed: no C++ cursor logic).
// This probe captures the iggy player pointer from DispatchEvent, then calls iggy
// exported APIs to query Flash movie state.
//
// Hooks:
//   _IggyPlayerDispatchEventRS@12 — capture player pointer + read event types
//   _IggyMakeEventKey@16 — detect if game EVER creates key events
//
// API calls (via NativeFunction):
//   _IggyPlayerGetFocusableObjects@20 — list focusable Flash elements
//
// Console: player pointer, focusable objects, event types
// File: everything
//
// Start on title screen. Press arrows. Ctrl+C to stop.

var logPath = 'D:/Games/Dev/Custom/FFX/ffx-x2-screen-reader/Docs/Scripts/Frida/output/FFX/query_iggy_state.log';
var logHandle = new File(logPath, 'w');
function logFile(msg) { logHandle.write(msg + '\n'); logHandle.flush(); }
function logBoth(msg) { console.log(msg); logFile(msg); }

var base = Process.enumerateModules()[0].base;
function rva(offset) { return base.add(offset); }

logBoth('[*] query_iggy_state');
logBoth('[*] base=0x' + base.toString(16));

// ── Find iggy module and exports ──
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
}

// Resolve iggy exports by name
function findExport(name) {
    var exports = iggyMod.enumerateExports();
    for (var i = 0; i < exports.length; i++) {
        if (exports[i].name.indexOf(name) !== -1) {
            return exports[i];
        }
    }
    return null;
}

var dispatchExport = findExport('DispatchEventRS');
var makeEventKeyExport = findExport('MakeEventKey');
var getFocusableExport = findExport('GetFocusableObjects');
var makeEventNoneExport = findExport('MakeEventNone');
var rootPathExport = findExport('PlayerRootPath');
var tickExport = findExport('PlayerTickRS');

logBoth('[IGGY] DispatchEventRS: ' + (dispatchExport ? dispatchExport.addr : 'NOT FOUND'));
logBoth('[IGGY] MakeEventKey: ' + (makeEventKeyExport ? makeEventKeyExport.addr : 'NOT FOUND'));
logBoth('[IGGY] GetFocusableObjects: ' + (getFocusableExport ? getFocusableExport.addr : 'NOT FOUND'));
logBoth('[IGGY] MakeEventNone: ' + (makeEventNoneExport ? makeEventNoneExport.addr : 'NOT FOUND'));
logBoth('[IGGY] RootPath: ' + (rootPathExport ? rootPathExport.addr : 'NOT FOUND'));
logBoth('[IGGY] TickRS: ' + (tickExport ? tickExport.addr : 'NOT FOUND'));

// ── State ──
var iggyPlayer = null;
var dispatchCount = 0;
var queriedFocusable = false;
var eventTypes = {};  // track unique event types

// ── Hook DispatchEventRS — capture player pointer and event types ──
if (dispatchExport) {
    try {
        Interceptor.attach(ptr(dispatchExport.addr), {
            onEnter: function(args) {
                dispatchCount++;
                var player = args[0];
                var eventPtr = args[1];

                // Capture player pointer on first call
                if (!iggyPlayer) {
                    iggyPlayer = player;
                    logBoth('[PLAYER] Iggy player pointer: 0x' + player.toString(16));

                    // Try to query focusable objects
                    queryFocusableObjects(player);
                }

                // Read event type (first 4 bytes)
                try {
                    var eventType = eventPtr.readU32();
                    if (!eventTypes[eventType]) {
                        eventTypes[eventType] = 0;
                        logBoth('[EVENT] New type: ' + eventType);
                    }
                    eventTypes[eventType]++;

                    // Log non-heartbeat events with full data
                    if (eventType !== 7) {
                        var bytes = new Uint8Array(eventPtr.readByteArray(32));
                        var hex = '';
                        for (var b = 0; b < bytes.length; b++) {
                            var h = bytes[b].toString(16);
                            if (h.length < 2) h = '0' + h;
                            hex += h;
                            if ((b + 1) % 4 === 0) hex += ' ';
                        }
                        logBoth('[EVENT] type=' + eventType + ' data: ' + hex);
                    }
                } catch(e) {}

                // Periodic summary
                if (dispatchCount % 60 === 0) {
                    logFile('[DISPATCH] count=' + dispatchCount + ' types=' + JSON.stringify(eventTypes));
                }
            }
        });
        logBoth('[OK] DispatchEventRS hooked');
    } catch(e) {
        logBoth('[!] DispatchEventRS hook failed: ' + e.message);
    }
}

// ── Hook MakeEventKey — see if game EVER creates keyboard events ──
if (makeEventKeyExport) {
    try {
        Interceptor.attach(ptr(makeEventKeyExport.addr), {
            onEnter: function(args) {
                logBoth('[MAKE_KEY!] MakeEventKey called! args: ' +
                    args[0] + ', ' + args[1] + ', ' + args[2] + ', ' + args[3]);
            }
        });
        logBoth('[OK] MakeEventKey hooked');
    } catch(e) {
        logBoth('[!] MakeEventKey hook failed: ' + e.message);
    }
}

// ── Hook MakeEventNone — confirm it fires (creates the type-7 events) ──
if (makeEventNoneExport) {
    var noneCount = 0;
    try {
        Interceptor.attach(ptr(makeEventNoneExport.addr), {
            onEnter: function(args) {
                noneCount++;
                if (noneCount === 1) {
                    logBoth('[MAKE_NONE] First call — event buffer: 0x' + args[0].toString(16));
                }
            }
        });
        logBoth('[OK] MakeEventNone hooked');
    } catch(e) {
        logBoth('[!] MakeEventNone hook failed: ' + e.message);
    }
}

// ── Hook TickRS — confirm iggy ticks ──
if (tickExport) {
    var tickCount = 0;
    try {
        Interceptor.attach(ptr(tickExport.addr), {
            onEnter: function(args) {
                tickCount++;
                if (tickCount === 1) {
                    logBoth('[TICK] First IggyPlayerTick — player: 0x' + args[0].toString(16));
                }
                if (tickCount % 60 === 0) {
                    logFile('[TICK] count=' + tickCount);
                }
            }
        });
        logBoth('[OK] TickRS hooked');
    } catch(e) {
        logBoth('[!] TickRS hook failed: ' + e.message);
    }
}

// ── Query focusable objects using NativeFunction ──
function queryFocusableObjects(player) {
    if (!getFocusableExport || queriedFocusable) return;
    queriedFocusable = true;

    try {
        // _IggyPlayerGetFocusableObjects@20: stdcall, 5 dword params
        // Guessed signature: (player, outArray, maxCount, outCount, flags)
        var getFocusable = new NativeFunction(
            ptr(getFocusableExport.addr),
            'int',                                    // return: error code
            ['pointer', 'pointer', 'int', 'pointer', 'int'],  // params
            'stdcall'
        );

        // Allocate buffers
        var maxObjects = 64;
        var outArray = Memory.alloc(maxObjects * 4);  // array of pointers
        var outCount = Memory.alloc(4);                // int*
        outCount.writeU32(0);

        logBoth('[FOCUS] Calling GetFocusableObjects...');
        var result = getFocusable(player, outArray, maxObjects, outCount, 0);
        var count = outCount.readU32();

        logBoth('[FOCUS] Result=' + result + ' count=' + count);

        if (count > 0 && count < maxObjects) {
            for (var f = 0; f < count; f++) {
                var objPtr = outArray.add(f * 4).readPointer();
                logBoth('[FOCUS] Object ' + f + ': 0x' + objPtr.toString(16));
            }
        }
    } catch(e) {
        logBoth('[FOCUS] GetFocusableObjects failed: ' + e.message);
    }
}

logBoth('[*] Ready. Press arrows on title screen.');
logBoth('[*] Watching for: key events, focus changes, new event types.');
