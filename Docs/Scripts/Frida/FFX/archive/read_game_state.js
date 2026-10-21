// @desc Read title menu state — targeted offsets from Ghidra decompile
//
// FUN_006667a0 writes selected item to scene+0xfaa4
// FUN_00666850 writes menu item to scene+0xfa80, flag at scene+0xfa9c
// These are title menu callbacks found via string xref to /menu/titl/
//
// Reads scene object via FUN_00623d50, monitors specific offsets.
//
// Attach on title menu (NEW GAME / LOAD). Press arrows.

var logPath = 'D:/Games/Dev/Custom/FFX/ffx-x2-screen-reader/Docs/Scripts/Frida/output/FFX/read_game_state.log';
var logHandle = new File(logPath, 'w');
function logFile(msg) { logHandle.write(msg + '\n'); logHandle.flush(); }
function logBoth(msg) { console.log(msg); logFile(msg); }

var base = Process.enumerateModules()[0].base;
function rva(offset) { return base.add(offset); }

logBoth('[*] title_menu_state');

// Get scene object
var getScene = new NativeFunction(rva(0x223D50), 'pointer', [], 'thiscall');
var scene = null;
try {
    scene = getScene();
    logBoth('[SCENE] 0x' + scene.toString(16));
} catch(e) {
    logBoth('[!] getScene failed: ' + e.message);
}

if (scene && !scene.isNull()) {
    // Read title menu offsets (from Ghidra: FUN_006667a0, FUN_00666850)
    var offsets = [];
    for (var off = 0xfa70; off <= 0xfac0; off += 4) {
        offsets.push(off);
    }

    // Initial read
    logBoth('[*] Title menu state (scene + 0xfa70 to 0xfac0):');
    var prevVals = {};
    for (var i = 0; i < offsets.length; i++) {
        try {
            var val = scene.add(offsets[i]).readS32();
            prevVals[offsets[i]] = val;
            if (val !== 0) {
                logBoth('  +0x' + offsets[i].toString(16) + ' = ' + val +
                    ' (0x' + (val >>> 0).toString(16) + ')');
            }
        } catch(e) {}
    }

    // Also check the 0x10dxx range (where FUN_00657b60 tried to store IggyMenu)
    logBoth('[*] Checking 0x10dc0-0x10de0:');
    for (var x = 0x10dc0; x <= 0x10de0; x += 4) {
        try {
            var val = scene.add(x).readS32();
            if (val !== 0 && val !== -1) {
                logBoth('  +0x' + x.toString(16) + ' = 0x' + (val >>> 0).toString(16));
            }
        } catch(e) {}
    }

    // Hook PInput::Update to monitor changes per frame
    var frameCount = 0;
    try {
        Interceptor.attach(rva(0x225930), {
            onLeave: function(retval) {
                frameCount++;
                // Check for changes every frame
                var changes = [];
                for (var j = 0; j < offsets.length; j++) {
                    try {
                        var val = scene.add(offsets[j]).readS32();
                        if (val !== prevVals[offsets[j]]) {
                            changes.push({off: offsets[j], from: prevVals[offsets[j]], to: val});
                            prevVals[offsets[j]] = val;
                        }
                    } catch(e) {}
                }
                if (changes.length > 0) {
                    for (var c = 0; c < changes.length; c++) {
                        logBoth('[CHANGE] +0x' + changes[c].off.toString(16) +
                            ': ' + changes[c].from + ' -> ' + changes[c].to +
                            ' (frame ' + frameCount + ')');
                    }
                }
            }
        });
        logBoth('[OK] PInput::Update hooked — monitoring for changes');
    } catch(e) {
        logBoth('[!] Hook failed: ' + e.message);
    }
}

// Also hook FUN_006667a0 and FUN_00666850 directly
try {
    Interceptor.attach(rva(0x2667A0), {
        onEnter: function(args) {
            logBoth('[FUN_6667a0] Sets scene+0xfaa4 = ' + args[0]);
        }
    });
    logBoth('[OK] FUN_006667a0 hooked');
} catch(e) {}

try {
    Interceptor.attach(rva(0x266850), {
        onEnter: function(args) {
            logBoth('[FUN_666850] Sets scene+0xfa80 = ' + args[0]);
        }
    });
    logBoth('[OK] FUN_00666850 hooked');
} catch(e) {}

// Hook FUN_00665900 — the main title screen handler
try {
    Interceptor.attach(rva(0x265900), {
        onEnter: function(args) {
            logBoth('[TITLE_HANDLER] FUN_00665900 fired! param_1=' + args[0] +
                ' param_2=' + args[1]);
        }
    });
    logBoth('[OK] FUN_00665900 hooked');
} catch(e) {}

logBoth('[*] Press arrows on title menu. Watch for [CHANGE] messages.');
