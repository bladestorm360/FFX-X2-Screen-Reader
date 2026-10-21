// @desc Backtrace every sound event — find title screen cursor handler via call chain
//
// FUN_00886de0 fires for sound events. Events 0x80000005 and 0x8000004a were seen
// on the title screen. The backtrace of the cursor-move sound leads directly to
// the title screen's cursor handler.
//
// Console: event ID + top 3 callers (Ghidra VAs)
// File: full backtrace with all addresses
//
// Press up/down on title screen. Ctrl+C to stop.

var logPath = 'D:/Games/Dev/Custom/FFX/ffx-x2-screen-reader/Docs/Scripts/Frida/output/FFX/trace_sound_backtrace.log';
var logHandle = new File(logPath, 'w');

function logFile(msg) {
    logHandle.write(msg + '\n');
    logHandle.flush();
}

function logBoth(msg) {
    console.log(msg);
    logFile(msg);
}

// ── Ghidra rebase ──
var ffxMod = Process.enumerateModules()[0];
var rebaseOffset = ffxMod.base.sub(0x00400000);

function g(ghidraAddr) {
    return ptr(ghidraAddr).add(rebaseOffset);
}

// Convert runtime address back to Ghidra VA for lookup in decompile output
function toGhidra(runtimeAddr) {
    return runtimeAddr.sub(rebaseOffset);
}

logBoth('[*] trace_sound_backtrace (rebase 0x' + rebaseOffset.toString(16) + ')');
logBoth('[*] FFX.exe base: 0x' + ffxMod.base.toString(16) +
    ' end: 0x' + ffxMod.base.add(ffxMod.size).toString(16));

var soundCount = 0;
var seenTraces = {};

try {
    Interceptor.attach(g(0x00886de0), {
        onEnter: function(args) {
            soundCount++;
            var eventId = args[0].toUInt32();
            var eventHex = '0x' + eventId.toString(16);

            // Full backtrace
            var bt = Thread.backtrace(this.context, Backtracer.ACCURATE);

            // Convert to Ghidra VAs and identify which module each address is in
            var btLines = [];
            var btBrief = [];
            for (var i = 0; i < bt.length; i++) {
                var rtAddr = bt[i];
                var ghidraVA = toGhidra(rtAddr);

                // Check if within FFX.exe range
                var inFFX = rtAddr.compare(ffxMod.base) >= 0 &&
                    rtAddr.compare(ffxMod.base.add(ffxMod.size)) < 0;

                var label = inFFX ?
                    'FFX+0x' + ghidraVA.toString(16) :
                    '0x' + rtAddr.toString(16);

                btLines.push('  [' + i + '] ' + label +
                    ' (rt: 0x' + rtAddr.toString(16) + ')');

                if (i < 3) btBrief.push(label);
            }

            // Log everything to file
            logFile('[SOUND #' + soundCount + '] event=' + eventHex);
            logFile('  backtrace:');
            for (var j = 0; j < btLines.length; j++) {
                logFile(btLines[j]);
            }
            logFile('');

            // Console: brief — event + top 3 callers
            // Use a trace key to avoid repeating identical backtraces
            var traceKey = eventHex + ':' + btBrief.join(',');
            if (!seenTraces[traceKey]) {
                seenTraces[traceKey] = true;
                logBoth('[SOUND] ' + eventHex + ' <- ' + btBrief.join(' <- '));
            }
        }
    });
    logBoth('[OK] FUN_00886de0 hooked');
} catch (e) {
    logBoth('[!] FUN_00886de0: ' + e.message);
}

logBoth('[*] Press up/down on title screen. Ctrl+C to stop.');
