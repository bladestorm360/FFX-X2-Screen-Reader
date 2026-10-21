// @desc Trace input dispatch and menu selection — press up/down on title screen
//
// Hooks game functions that dispatch input to iggy and handle Flash callbacks.
// Goal: find what changes when user navigates menus (selection index, cursor state).
//
// Targets (Ghidra VAs, rebased at runtime):
//   FUN_00a28c10 — input→iggy dispatch (calls MakeEventNone, MouseMove, DispatchEventRS)
//   FUN_00a26ce0 — mouse/input event builder
//   FUN_00a3d7f0 — dispatches events to iggy player
//   FUN_006e9650 — AS3 callback result path + SetStringUTF8RS
//   FUN_006e99b0 — AS3 callback result path + SetStringUTF8RS
//   FUN_006fcd80 — massive iggy value manipulation function (14 xrefs)
//
// Press up, down, confirm on title screen. Ctrl+C to stop.

var logPath = 'D:/Games/Dev/Custom/FFX/ffx-x2-screen-reader/Docs/Scripts/Frida/output/FFX/trace_input_dispatch.log';
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
    } catch (e) {}
    return null;
}

// ── Ghidra rebase (proven pattern) ──
var ffxMod = Process.enumerateModules()[0];
var runtimeBase = ffxMod.base;
var ghidraImageBase = 0x00400000;
var rebaseOffset = runtimeBase.sub(ghidraImageBase);

function g(ghidraAddr) {
    return ptr(ghidraAddr).add(rebaseOffset);
}

logBoth('[*] trace_input_dispatch starting');
logBoth('[*] Rebase: 0x' + rebaseOffset.toString(16));

// Track call counts per function — file gets all, console gets first + periodic
var counts = {};
var seen = {};

function hookFunc(ghidraAddr, name, logArgs) {
    var addr = g(ghidraAddr);
    try {
        Interceptor.attach(addr, {
            onEnter: function(args) {
                if (!counts[name]) counts[name] = 0;
                counts[name]++;
                var n = counts[name];

                var line = '[' + name + ' #' + n + ']';
                if (logArgs) {
                    for (var i = 0; i < 6; i++) {
                        var val = args[i];
                        var s = tryReadCString(val);
                        line += ' a' + i + '=0x' + val.toString(16);
                        if (s) line += '("' + s + '")';
                    }
                }

                logFile(line);

                // Console: first call, then every 10th
                if (!seen[name]) {
                    seen[name] = true;
                    logBoth('[NEW] ' + name + ' firing');
                }
            }
        });
        logBoth('[OK] ' + name + ' at ' + addr);
    } catch (e) {
        logBoth('[!] ' + name + ' failed: ' + e.message);
    }
}

// ── Hook input dispatch functions ──

// Input→iggy dispatch: calls MakeEventNone, MakeEventMouseMove, DispatchEventRS
hookFunc(0x00a28c10, 'InputDispatch', true);

// Mouse/input event builder: calls MakeEvent* functions
hookFunc(0x00a26ce0, 'EventBuilder', true);

// Dispatches events to iggy player (calls DispatchEventRS)
hookFunc(0x00a3d7f0, 'PlayerDispatch', true);

// AS3 callback result paths (call CallbackResultPath + SetStringUTF8RS)
hookFunc(0x006e9650, 'AS3Result_A', true);
hookFunc(0x006e99b0, 'AS3Result_B', true);

// Massive iggy value manipulation (called from 14 places)
hookFunc(0x006fcd80, 'IggyValueManip', true);

// SetString callers (from previous probe — these didn't fire on static screen,
// but might fire when menu selection changes)
hookFunc(0x006ff110, 'SetStr_A', true);
hookFunc(0x006fcca0, 'SetStr_B', true);

// ── Also hook XInput functions to see raw gamepad input ──
// XInput9_1_0.dll has 2 imports — likely XInputGetState and XInputGetCapabilities
var xinputMod = null;
var modules = Process.enumerateModules();
for (var i = 0; i < modules.length; i++) {
    if (modules[i].name.toLowerCase() === 'xinput9_1_0.dll') {
        xinputMod = modules[i];
        break;
    }
}

if (xinputMod) {
    var xExports = xinputMod.enumerateExports();
    for (var i = 0; i < xExports.length; i++) {
        var xExp = xExports[i];
        if (xExp.type !== 'function') continue;
        (function(name, addr) {
            try {
                Interceptor.attach(addr, {
                    onEnter: function(args) {
                        if (!counts[name]) counts[name] = 0;
                        counts[name]++;

                        // Only log first few calls + periodic to avoid spam
                        if (counts[name] <= 5 || counts[name] % 100 === 0) {
                            logFile('[XINPUT] ' + name + ' #' + counts[name] +
                                ' a0=0x' + args[0].toString(16) +
                                ' a1=0x' + args[1].toString(16));
                        }

                        if (!seen[name]) {
                            seen[name] = true;
                            logBoth('[NEW] XInput: ' + name);
                        }
                    }
                });
                logFile('[OK] XInput ' + name + ' hooked');
            } catch (e) {
                logFile('[!] XInput ' + name + ' failed: ' + e.message);
            }
        })(xExp.name, xExp.address);
    }
}

logBoth('[*] All hooks set. Press up/down/confirm on title screen. Ctrl+C to stop.');
