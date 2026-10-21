// @desc Hook menu selection and button press functions — press up/down/confirm on title
//
// Key targets from Ghidra string analysis:
//   FUN_00702af0 — "changeSelectedBtn" — changes the selected menu option
//   FUN_0062bd10 — "Buttonpress" — button press handler
//   FUN_0062c8e0 — "Select" + "Unrecognised argument '%s' passed to Buttonpress()"
//   FUN_00623090 — "Select" + "Buttonpress()" error
//   FUN_00623220 — "Select" + "Buttonpress()" error
//   FUN_00622f00 — "Select" + "Buttonpress()" error
//   FUN_00702db0 — "setSaveLoadFocus"
//   FUN_0084f550 — "Select" + "BATTLE" + "SYSTEM" — game state router
//   FUN_0084c1e0 — "Select"
//   FUN_0068fec0 — "EscMenuControllerSettingRefresh" etc.
//   FUN_007c6d90 — "Focus Off"
//
// Press up/down/confirm on title screen. Ctrl+C to stop.

var logPath = 'D:/Games/Dev/Custom/FFX/ffx-x2-screen-reader/Docs/Scripts/Frida/output/FFX/trace_menu_select.log';
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

// ── Ghidra rebase (proven: base 0x400000, offset 0x680000) ──
var ffxMod = Process.enumerateModules()[0];
var runtimeBase = ffxMod.base;
var ghidraImageBase = 0x00400000;
var rebaseOffset = runtimeBase.sub(ghidraImageBase);

function g(ghidraAddr) {
    return ptr(ghidraAddr).add(rebaseOffset);
}

logBoth('[*] trace_menu_select starting (rebase 0x' + rebaseOffset.toString(16) + ')');

var counts = {};
var seen = {};

function hookFunc(ghidraAddr, name) {
    var addr = g(ghidraAddr);
    try {
        Interceptor.attach(addr, {
            onEnter: function(args) {
                if (!counts[name]) counts[name] = 0;
                counts[name]++;
                var n = counts[name];

                // Read first 6 args, try as strings and ints
                var parts = [];
                for (var i = 0; i < 6; i++) {
                    var val = args[i];
                    var s = tryReadCString(val);
                    var hex = '0x' + val.toString(16);
                    // Small values likely integers (menu index, button id, etc.)
                    var intVal = val.toInt32();
                    if (s && s.length > 1 && s.length < 200) {
                        parts.push('a' + i + '=' + hex + '("' + s + '")');
                    } else if (intVal >= -100 && intVal <= 10000) {
                        parts.push('a' + i + '=' + intVal);
                    } else {
                        parts.push('a' + i + '=' + hex);
                    }
                }

                var line = '[' + name + ' #' + n + '] ' + parts.join(' ');
                logFile(line);

                // Backtrace for first 5 calls of each function
                if (n <= 5) {
                    var bt = Thread.backtrace(this.context, Backtracer.ACCURATE)
                        .map(function(a) { return a.toString(); })
                        .join(', ');
                    logFile('  bt: ' + bt);
                }

                // Console: first occurrence + when args contain readable strings
                if (!seen[name]) {
                    seen[name] = true;
                    logBoth('[NEW] ' + name + ' | ' + parts.join(' '));
                }
            }
        });
        logBoth('[OK] ' + name);
    } catch (e) {
        logBoth('[!] ' + name + ': ' + e.message);
    }
}

// ── Primary targets: menu selection and button press ──
hookFunc(0x00702af0, 'changeSelectedBtn');
hookFunc(0x0062bd10, 'Buttonpress');
hookFunc(0x0062c8e0, 'SelectHandler_A');
hookFunc(0x00623090, 'SelectHandler_B');
hookFunc(0x00623220, 'SelectHandler_C');
hookFunc(0x00622f00, 'SelectHandler_D');

// ── Secondary targets: focus/state management ──
hookFunc(0x00702db0, 'setSaveLoadFocus');
hookFunc(0x0084f550, 'GameStateRouter');
hookFunc(0x0084c1e0, 'SelectHandler_E');
hookFunc(0x0068fec0, 'EscMenuController');
hookFunc(0x007c6d90, 'FocusOff');

logBoth('[*] All hooks set. Press up/down/confirm. Ctrl+C to stop.');
