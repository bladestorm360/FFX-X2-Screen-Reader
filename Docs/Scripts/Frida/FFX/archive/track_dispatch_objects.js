// @desc Hook DISPATCH directly — track 11 scene objects, detect state changes on input
//
// FUN_005b3760 (RVA 0x1b3760) fires 27K+ times per frame with 11 unique object pointers.
// CONFIRMED FIRING from previous probe. This hook:
//   1. First call of each unique object: backtrace + vtable + 512-byte dump
//   2. Per-frame: diff 512 bytes of each known object against previous snapshot
//   3. Console: ONLY state changes (cursor movement detection)
//   4. File: everything
//
// Navigate title screen (up/down). Ctrl+C to stop.

var logPath = 'D:/Games/Dev/Custom/FFX/ffx-x2-screen-reader/Docs/Scripts/Frida/output/FFX/track_dispatch_objects.log';
var logHandle = new File(logPath, 'w');

function logFile(msg) {
    logHandle.write(msg + '\n');
    logHandle.flush();
}

function logBoth(msg) {
    console.log(msg);
    logFile(msg);
}

function safeReadPtr(addr) {
    try { return addr.readPointer(); } catch(e) { return null; }
}

function readBytes(addr, len) {
    try { return new Uint8Array(addr.readByteArray(len)); } catch(e) { return null; }
}

// ── RVA addressing ──
var base = Process.enumerateModules()[0].base;
function rva(offset) { return base.add(offset); }
function toGhidra(addr) { return ptr(addr).sub(base).add(0x00400000); }

logBoth('[*] track_dispatch_objects v2');
logBoth('[*] base=0x' + base.toString(16));

// ── Object registry ──
var SNAP_SIZE = 512;
var objects = {};        // key: addr hex string, value: {ptr, snap, vtable, count}
var objectList = [];     // ordered list of unique addresses
var totalCalls = 0;
var backtraceDone = false;
var initReportDone = false;
var frameMarker = 0;     // increments when we see the first object repeat

// Track "frames" — a complete cycle is when we see the first object appear again
var firstObjAddr = null;
var frameCount = 0;

try {
    Interceptor.attach(rva(0x1b3760), {
        onEnter: function(args) {
            totalCalls++;
            var objPtr = args[0];
            var objKey = objPtr.toString(16);

            // Detect frame boundaries: when the first-seen object repeats
            if (firstObjAddr === null) {
                firstObjAddr = objKey;
            }
            if (objKey === firstObjAddr && totalCalls > 1) {
                frameCount++;

                // After initial discovery, do per-frame diffs
                if (initReportDone && frameCount > 1) {
                    doFrameDiffs();
                }
            }

            // Register new objects
            if (!objects[objKey]) {
                objects[objKey] = {
                    ptr: objPtr,
                    snap: null,
                    vtable: null,
                    count: 0,
                    idx: objectList.length
                };
                objectList.push(objKey);

                // First encounter: backtrace + vtable + dump
                var vtablePtr = safeReadPtr(objPtr);
                var ghidraVtable = vtablePtr ? toGhidra(vtablePtr) : null;
                objects[objKey].vtable = ghidraVtable;

                logFile('[NEW OBJ #' + objectList.length + '] 0x' + objKey +
                    ' vtable=' + (ghidraVtable ? '0x' + ghidraVtable.toString(16) : 'null'));

                // Backtrace on very first call only
                if (!backtraceDone) {
                    backtraceDone = true;
                    var bt = Thread.backtrace(this.context, Backtracer.ACCURATE);
                    var btLines = [];
                    for (var i = 0; i < bt.length; i++) {
                        btLines.push('0x' + toGhidra(bt[i]).toString(16));
                    }
                    logFile('[BACKTRACE] ' + btLines.join(' <- '));
                    logBoth('[BACKTRACE] Caller chain: ' + btLines.slice(0, 4).join(' <- '));
                }

                // Dump 512 bytes
                var bytes = readBytes(objPtr, SNAP_SIZE);
                if (bytes) {
                    objects[objKey].snap = bytes;
                    var hex = '';
                    for (var j = 0; j < bytes.length; j++) {
                        var h = bytes[j].toString(16);
                        if (h.length < 2) h = '0' + h;
                        hex += h;
                        if ((j + 1) % 32 === 0) hex += '\n    ';
                        else if ((j + 1) % 4 === 0) hex += ' ';
                    }
                    logFile('[DUMP] obj #' + objectList.length + ':\n    ' + hex);
                }
            }

            objects[objKey].count++;

            // After seeing all unique objects for ~2 frames, report and start tracking
            if (!initReportDone && frameCount >= 2) {
                initReportDone = true;
                logBoth('[*] Found ' + objectList.length + ' unique objects across ' +
                    frameCount + ' frames. Tracking state changes...');
                for (var k = 0; k < objectList.length; k++) {
                    var o = objects[objectList[k]];
                    logBoth('[OBJ ' + k + '] 0x' + objectList[k] +
                        ' vtable=' + (o.vtable ? '0x' + o.vtable.toString(16) : 'null') +
                        ' calls=' + o.count);
                }
                logBoth('[*] Press UP/DOWN. Changes will appear on console.');
            }
        }
    });
    logBoth('[OK] DISPATCH hooked (RVA 0x1b3760)');
} catch(e) {
    logBoth('[!] DISPATCH: ' + e.message);
}

function doFrameDiffs() {
    for (var i = 0; i < objectList.length; i++) {
        var key = objectList[i];
        var obj = objects[key];
        if (!obj.ptr) continue;

        var newSnap = readBytes(obj.ptr, SNAP_SIZE);
        if (!newSnap || !obj.snap) {
            obj.snap = newSnap;
            continue;
        }

        // Diff
        var changes = [];
        for (var b = 0; b < SNAP_SIZE && b < newSnap.length && b < obj.snap.length; b++) {
            if (obj.snap[b] !== newSnap[b]) {
                changes.push({
                    offset: b,
                    oldVal: obj.snap[b],
                    newVal: newSnap[b]
                });
            }
        }

        if (changes.length > 0) {
            // STATE CHANGED — console output
            var parts = [];
            for (var c = 0; c < Math.min(changes.length, 6); c++) {
                var ch = changes[c];
                parts.push('+0x' + ch.offset.toString(16) + ':' + ch.oldVal + '->' + ch.newVal);
            }
            var msg = '[CHANGE] obj' + i + ' (0x' + key + '): ' +
                changes.length + ' bytes: ' + parts.join(', ');
            if (changes.length > 6) msg += ' +' + (changes.length - 6) + ' more';
            logBoth(msg);

            // Full detail to file
            for (var d = 0; d < changes.length; d++) {
                logFile('  +0x' + changes[d].offset.toString(16) + ': ' +
                    changes[d].oldVal + ' -> ' + changes[d].newVal);
            }
        }

        obj.snap = newSnap;
    }

    // Periodic frame count to file
    if (frameCount % 100 === 0) {
        logFile('[FRAME] ' + frameCount + ' total_calls=' + totalCalls);
    }
}

logBoth('[*] Waiting for dispatch events. Navigate title screen.');
