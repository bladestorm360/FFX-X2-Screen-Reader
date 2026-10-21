// @desc Trace keyboard input to cursor — find cursor memory address via memory scanning
//
// Hooks PInput::Update (backtrace) and keyboard device update (key detection).
// On each key press, scans/narrows module writable data for cursor-like DWORD changes.
//
// Console: backtrace, key presses, cursor candidates (up to 30)
// File: everything
//
// Instructions:
// 1. Start on title screen with cursor on first item (New Game)
// 2. Press DOWN arrow once — probe takes initial memory snapshot
// 3. Wait 2 seconds, press DOWN arrow again — probe narrows candidates
// 4. Wait 2 seconds, press UP arrow — probe narrows further
// 5. After 3-4 presses, candidates should converge to cursor address
//
// Key indices (from walk_device_list): 0x59, 0x5a, 0x73 = arrow/enter keys

var logPath = 'D:/Games/Dev/Custom/FFX/ffx-x2-screen-reader/Docs/Scripts/Frida/output/FFX/trace_input_to_cursor.log';
var logHandle = new File(logPath, 'w');
function logFile(msg) { logHandle.write(msg + '\n'); logHandle.flush(); }
function logBoth(msg) { console.log(msg); logFile(msg); }

// ── RVA addressing ──
var base = Process.enumerateModules()[0].base;
var modSize = Process.enumerateModules()[0].size;
var modEnd = base.add(modSize);
function rva(offset) { return base.add(offset); }
function toGhidra(addr) { return ptr(addr).sub(base).add(0x00400000); }

logBoth('[*] trace_input_to_cursor');
logBoth('[*] base=0x' + base.toString(16) + ' size=0x' + modSize.toString(16));

// ── State ──
var frameCount = 0;
var kbEcx = null;
var prevRawState = null;
var keyPressCount = 0;
var pendingScanFrame = -1;
var scanPhase = 0;       // 0=waiting, 1=initial done, 2+=narrowing
var candidates = [];     // [{addr: NativePointer, value: number}]
var moduleRanges = null;
var backtraceDone = false;

// ── Init writable module ranges (once) ──
function initModuleRanges() {
    if (moduleRanges) return;
    var all = Process.enumerateRanges('rw-');
    moduleRanges = [];
    for (var i = 0; i < all.length; i++) {
        if (all[i].base.compare(base) >= 0 && all[i].base.compare(modEnd) < 0) {
            moduleRanges.push(all[i]);
        }
    }
    var total = 0;
    for (var j = 0; j < moduleRanges.length; j++) total += moduleRanges[j].size;
    logBoth('[RANGES] ' + moduleRanges.length + ' writable ranges (' +
        (total / 1024).toFixed(0) + ' KB)');
}

// ── Initial scan: find all DWORD values 0-maxVal in module writable data ──
function doInitialScan(maxVal) {
    initModuleRanges();
    candidates = [];
    var totalBytes = 0;

    for (var r = 0; r < moduleRanges.length; r++) {
        var range = moduleRanges[r];
        try {
            var buf = range.base.readByteArray(range.size);
            var bytes = new Uint8Array(buf);
            totalBytes += bytes.length;

            for (var off = 0; off + 3 < bytes.length; off += 4) {
                // Quick filter: top 3 bytes must be 0 for small DWORD values
                if (bytes[off + 1] !== 0 || bytes[off + 2] !== 0 || bytes[off + 3] !== 0) continue;
                var val = bytes[off];
                if (val <= maxVal) {
                    candidates.push({addr: range.base.add(off), value: val});
                }
            }
        } catch (e) {
            logFile('[SCAN] Range error at ' + range.base + ': ' + e.message);
        }
    }

    scanPhase = 1;
    logBoth('[SCAN] Phase 1 (initial): ' + candidates.length + ' candidates (DWORD 0-' +
        maxVal + ') in ' + (totalBytes / 1024).toFixed(0) + ' KB');
    logBoth('[*] Press another arrow key to narrow candidates.');
}

// ── Narrow scan: find candidates whose value changed ──
function doNarrowScan() {
    var changed = [];
    var unchanged = 0;
    var errors = 0;

    for (var i = 0; i < candidates.length; i++) {
        try {
            var newVal = candidates[i].addr.readU32();
            if (newVal !== candidates[i].value) {
                if (newVal <= 100) {
                    changed.push({
                        addr: candidates[i].addr,
                        from: candidates[i].value,
                        to: newVal
                    });
                }
            } else {
                unchanged++;
            }
        } catch (e) { errors++; }
    }

    // Update candidates for next round
    candidates = [];
    for (var j = 0; j < changed.length; j++) {
        candidates.push({addr: changed[j].addr, value: changed[j].to});
    }

    scanPhase++;
    logBoth('[SCAN] Phase ' + scanPhase + ': ' + changed.length + ' changed, ' +
        unchanged + ' unchanged' + (errors > 0 ? ', ' + errors + ' errors' : ''));

    // Console: top 30
    var limit = Math.min(changed.length, 30);
    for (var k = 0; k < limit; k++) {
        var g = toGhidra(changed[k].addr);
        logBoth('  [' + (k + 1) + '] 0x' + g.toString(16) + ': ' +
            changed[k].from + ' -> ' + changed[k].to);
    }
    if (changed.length > 30) {
        logBoth('  ... ' + (changed.length - 30) + ' more in log file');
    }

    // File: all
    for (var l = 0; l < changed.length; l++) {
        logFile('[CAND] Ghidra 0x' + toGhidra(changed[l].addr).toString(16) +
            ' (rt 0x' + changed[l].addr.toString(16) + '): ' +
            changed[l].from + ' -> ' + changed[l].to);
    }

    if (changed.length === 0) {
        logBoth('[!] No candidates changed. Cursor may not be in module static data.');
        logBoth('[!] Next step: expand scan to heap memory.');
    } else if (changed.length <= 10) {
        logBoth('[*] Small set! Likely includes the cursor address.');
        logBoth('[*] Press another key to narrow further, or check log for details.');
    }
}

// ── Hook 1: PInput::Update — backtrace + scan trigger ──
try {
    Interceptor.attach(rva(0x225930), {
        onEnter: function (args) {
            frameCount++;

            if (!backtraceDone) {
                backtraceDone = true;
                try {
                    var bt = Thread.backtrace(this.context, Backtracer.ACCURATE);
                    logBoth('[BACKTRACE] PInput::Update callers:');
                    for (var i = 0; i < Math.min(bt.length, 8); i++) {
                        logBoth('  [' + i + '] Ghidra 0x' + toGhidra(bt[i]).toString(16));
                    }
                } catch (e) {
                    logBoth('[BACKTRACE] Error: ' + e.message);
                }
            }
        },
        onLeave: function (retval) {
            // Execute scan 2 frames after key press (game logic has processed input)
            if (pendingScanFrame >= 0 && frameCount >= pendingScanFrame + 2) {
                pendingScanFrame = -1;
                if (scanPhase === 0) {
                    doInitialScan(5);
                } else {
                    doNarrowScan();
                }
            }
        }
    });
    logBoth('[OK] PInput::Update hooked (RVA 0x225930)');
} catch (e) {
    logBoth('[!] PInput::Update: ' + e.message);
}

// ── Hook 2: Keyboard device update — detect key presses ──
try {
    Interceptor.attach(rva(0x22d770), {
        onEnter: function (args) {
            this.ecx = ptr(this.context.ecx);
        },
        onLeave: function (retval) {
            var ecx = this.ecx;
            if (!kbEcx) {
                kbEcx = ecx;
                logBoth('[KB] ECX=0x' + ecx.toString(16));
            }

            // Read "raw state" buffer at ECX+0x200 (256 bytes, leading-edge indicator)
            var rawBuf;
            try {
                rawBuf = new Uint8Array(ecx.add(0x200).readByteArray(0x100));
            } catch (e) { return; }

            if (prevRawState) {
                for (var k = 0; k < 0x100; k++) {
                    if (rawBuf[k] === 1 && prevRawState[k] === 0) {
                        keyPressCount++;
                        logBoth('[KEY #' + keyPressCount + '] index=0x' +
                            k.toString(16) + ' (frame ' + frameCount + ')');

                        // Trigger scan 2 frames later (only if no scan already pending)
                        if (pendingScanFrame < 0) {
                            pendingScanFrame = frameCount;
                        }
                    }
                }
            }
            prevRawState = rawBuf;
        }
    });
    logBoth('[OK] Keyboard update hooked (RVA 0x22d770)');
} catch (e) {
    logBoth('[!] Keyboard update: ' + e.message);
}

logBoth('[*] Ready. Press arrow keys on title screen, one at a time.');
logBoth('[*] Wait ~2 seconds between presses. Candidates narrow each press.');
