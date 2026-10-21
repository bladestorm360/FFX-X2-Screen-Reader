// @desc Hook ALL iggy_w32.dll exports to discover which functions the game actually calls
//
// Attaches lightweight Interceptor hooks to every iggy export. When a function fires,
// it logs the function name (first occurrence to console, all to file). After navigating
// menus, the log shows exactly which iggy APIs are hot.
//
// Console: each unique function name on first fire only
// File: every call with args[0]-args[3] hex values

var logPath = 'D:/Games/Dev/Custom/FFX/ffx-x2-screen-reader/Docs/Scripts/Frida/output/FFX/trace_iggy_all.log';
var logHandle = new File(logPath, 'w');

function logFile(msg) {
    logHandle.write(msg + '\n');
    logHandle.flush();
}

function logBoth(msg) {
    console.log(msg);
    logFile(msg);
}

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
    var exports = iggyMod.enumerateExports();
    var hooked = 0;
    var failed = 0;
    var seen = {};
    var callCounts = {};

    logBoth('[*] trace_iggy_all: hooking ' + exports.length + ' iggy exports...');

    for (var i = 0; i < exports.length; i++) {
        var exp = exports[i];
        if (exp.type !== 'function') continue;

        try {
            (function(name, addr) {
                Interceptor.attach(addr, {
                    onEnter: function(args) {
                        if (!callCounts[name]) callCounts[name] = 0;
                        callCounts[name]++;

                        // File: log every call with first 4 args
                        logFile('[CALL] ' + name + ' #' + callCounts[name] +
                            ' a0=0x' + args[0].toString(16) +
                            ' a1=0x' + args[1].toString(16) +
                            ' a2=0x' + args[2].toString(16) +
                            ' a3=0x' + args[3].toString(16));

                        // Console: first occurrence only
                        if (!seen[name]) {
                            seen[name] = true;
                            logBoth('[NEW] ' + name);
                        }
                    }
                });
                hooked++;
            })(exp.name, exp.address);
        } catch (e) {
            failed++;
            logFile('[SKIP] ' + exp.name + ' — ' + e.message);
        }
    }

    logBoth('[*] Hooked: ' + hooked + ', failed: ' + failed);
    logBoth('[*] Navigate menus, enter battle, etc. Ctrl+C to stop.');
    logBoth('[*] On exit, check log for call counts per function.');
}
