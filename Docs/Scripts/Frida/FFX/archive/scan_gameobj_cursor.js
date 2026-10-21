// ARCHIVED — BUG: reads from global DAT_ pointers instead of ECX. See track_cursor_native.js for fixed version.
// @desc Scan full 69KB game object with noise elimination — find cursor on input
//
// Hooks FUN_0062f000 (main game loop, confirmed via backtrace).
// Phase 1: Auto-calibrate noise (3 frames, no input needed)
// Phase 2: Report ONLY non-noise changes (= cursor movement from input)
//
// Console: "Calibrating..." then "Ready" then ONLY cursor changes.
// File: full noise set + all diffs.
//
// Navigate title screen after "Ready" appears. Ctrl+C to stop.

var logPath = 'D:/Games/Dev/Custom/FFX/ffx-x2-screen-reader/Docs/Scripts/Frida/output/FFX/scan_gameobj_cursor.log';
var logHandle = new File(logPath, 'w');

function logFile(msg) {
    logHandle.write(msg + '\n');
    logHandle.flush();
}

function logBoth(msg) {
    console.log(msg);
    logFile(msg);
}

function readBytes(addr, len) {
    try { return new Uint8Array(addr.readByteArray(len)); } catch(e) { return null; }
}

// ── RVA addressing ──
var base = Process.enumerateModules()[0].base;
function rva(offset) { return base.add(offset); }

logBoth('[*] scan_gameobj_cursor');
logBoth('[*] base=0x' + base.toString(16));

// ── Game object capture ──
var gameObj = null;
var OBJ_SIZE = 0x11000; // ~69KB

// Read FFX game object from DAT_00ccb9d8 (RVA 0x8CB9D8)
// NOT the PhyreEngine object from FUN_00623d50 (which is static)
try {
    gameObj = rva(0x8CB9D8).readPointer();
    if (gameObj.isNull()) {
        logBoth('[!] DAT_00ccb9d8 is NULL');
        gameObj = null;
    } else {
        logBoth('[*] FFX game object (DAT_00ccb9d8): 0x' + gameObj.toString(16));
    }
} catch(e) {
    logBoth('[!] Cannot read DAT_00ccb9d8: ' + e.message);
}

// ── Noise calibration + cursor detection ──
var CALIBRATION_FRAMES = 5; // frames to collect noise
var frameCount = 0;
var snapshots = [];
var noiseSet = {};         // key: offset, value: true
var calibrated = false;
var lastSnap = null;

function takeFullSnapshot() {
    if (!gameObj) return null;
    return readBytes(gameObj, OBJ_SIZE);
}

function buildNoiseSet() {
    // Compare consecutive snapshot pairs — any offset that changed in ANY pair is noise
    for (var i = 1; i < snapshots.length; i++) {
        var a = snapshots[i - 1];
        var b = snapshots[i];
        if (!a || !b) continue;
        for (var j = 0; j < a.length && j < b.length; j++) {
            if (a[j] !== b[j]) {
                noiseSet[j] = true;
            }
        }
    }
    var noiseCount = Object.keys(noiseSet).length;
    logBoth('[*] Noise calibration complete: ' + noiseCount + ' noisy bytes out of ' + OBJ_SIZE);
    logFile('[NOISE] Offsets: ' + Object.keys(noiseSet).sort(function(a,b){return a-b;}).slice(0, 50).join(', ') + '...');
}

function diffWithNoiseFilter(oldSnap, newSnap) {
    var changes = [];
    for (var i = 0; i < oldSnap.length && i < newSnap.length; i++) {
        if (oldSnap[i] !== newSnap[i] && !noiseSet[i]) {
            changes.push({
                offset: i,
                oldVal: oldSnap[i],
                newVal: newSnap[i]
            });
        }
    }
    return changes;
}

// Hook FUN_005b3760 (RVA 0x1b3760) — DISPATCH, confirmed firing 27K+/frame
// Use the first call per "frame cycle" as our frame tick
var firstObjKey = null;

try {
    Interceptor.attach(rva(0x1b3760), {
        onEnter: function(args) {
            if (!gameObj) return;

            // Detect frame boundary: when first-seen object repeats
            var objKey = args[0].toString(16);
            if (firstObjKey === null) {
                firstObjKey = objKey;
                frameCount++;
            } else if (objKey === firstObjKey) {
                frameCount++;
            } else {
                return; // Not a frame boundary, skip
            }

            if (!calibrated) {
                // Phase 1: Calibration — collect snapshots silently
                var snap = takeFullSnapshot();
                if (snap) {
                    snapshots.push(snap);
                    if (frameCount === 1) {
                        logBoth('[*] Calibrating noise (' + CALIBRATION_FRAMES + ' frames, do NOT press anything)...');
                    }
                    if (snapshots.length >= CALIBRATION_FRAMES) {
                        buildNoiseSet();
                        calibrated = true;
                        lastSnap = snap;
                        logBoth('[*] Ready. Press UP/DOWN on title screen. Only cursor changes will appear.');
                    }
                }
            } else {
                // Phase 2: Detect input-only changes
                var newSnap = takeFullSnapshot();
                if (newSnap && lastSnap) {
                    var changes = diffWithNoiseFilter(lastSnap, newSnap);
                    if (changes.length > 0) {
                        var parts = [];
                        for (var c = 0; c < Math.min(changes.length, 10); c++) {
                            var ch = changes[c];
                            parts.push('+0x' + ch.offset.toString(16) + ':' +
                                ch.oldVal + '->' + ch.newVal);
                        }
                        var msg = '[CURSOR?] ' + changes.length + ' non-noise changes: ' +
                            parts.join(', ');
                        if (changes.length > 10) msg += ' +' + (changes.length - 10) + ' more';
                        logBoth(msg);

                        // Full detail to file
                        for (var d = 0; d < changes.length; d++) {
                            logFile('  +0x' + changes[d].offset.toString(16) + ': ' +
                                changes[d].oldVal + ' -> ' + changes[d].newVal);
                        }
                    }
                    lastSnap = newSnap;
                }

                // Periodic frame count to file
                if (frameCount % 300 === 0) {
                    logFile('[FRAME] ' + frameCount);
                }
            }
        }
    });
    logBoth('[OK] DISPATCH hooked as frame anchor (RVA 0x1b3760)');
} catch(e) {
    logBoth('[!] DISPATCH: ' + e.message);
}

logBoth('[*] Waiting for game object capture + noise calibration...');
