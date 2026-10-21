// ARCHIVED — BUG: reads from global DAT_ pointers instead of ECX. See track_cursor_native.js for fixed version.
// @python
// @desc Memory diff to find title screen cursor — snapshot on keypress, diff on next
//
// Scans the game object and nearby globals for ANY byte that changes when user
// presses up/down on the title screen. Uses Python host for keyboard triggers.
//
// Workflow:
//   1. Attaches, captures game object, reads initial state
//   2. ENTER → snapshot A
//   3. Press UP or DOWN in the game
//   4. ENTER → snapshot B, diff shows changed bytes
//   5. Repeat to confirm which offset is the cursor index

'use strict';

var logPath = 'D:/Games/Dev/Custom/FFX/ffx-x2-screen-reader/Docs/Scripts/Frida/output/FFX/diff_title_state.log';
var logHandle = new File(logPath, 'w');

function logFile(msg) {
    logHandle.write(msg + '\n');
    logHandle.flush();
}

function logBoth(msg) {
    console.log(msg);
    logFile(msg);
}

// ── RVA-based addressing (ASLR-safe, proven pattern) ──
var base = Process.enumerateModules()[0].base;
var modSize = Process.enumerateModules()[0].size;

function rva(offset) {
    return base.add(offset);
}

function toGhidra(addr) {
    return ptr(addr).sub(base).add(0x00400000);
}

logBoth('[*] diff_title_state');
logBoth('[*] base=0x' + base.toString(16));

// ── Capture game object pointer ──
// Hook FUN_00623d50 (RVA 0x223d50) once to get the game object
var gameObj = null;

try {
    var getObjAddr = rva(0x223d50);
    Interceptor.attach(getObjAddr, {
        onLeave: function(retval) {
            if (gameObj === null && !retval.isNull()) {
                gameObj = retval;
                logBoth('[*] Game object captured: 0x' + gameObj.toString(16));
                buildRegions();
            }
        }
    });
    logBoth('[OK] FUN_00623d50 hooked (waiting for game object)');
} catch(e) {
    logBoth('[!] FUN_00623d50: ' + e.message);
}

// ── Also read known global pointers directly ──
var gameObjStatic = null;
try {
    gameObjStatic = rva(0x8CB9D8).readPointer();  // DAT_00ccb9d8
    logBoth('[*] DAT_00ccb9d8 (static game obj): 0x' + gameObjStatic.toString(16));
} catch(e) {
    logBoth('[!] Cannot read DAT_00ccb9d8: ' + e.message);
}

// Read game state
try {
    var gameState = rva(0xEFBBF8).readU32();  // DAT_012fbbf8
    logBoth('[*] Game state: 0x' + gameState.toString(16) +
        (gameState === 0x17 ? ' (TITLE SCREEN)' : ''));
} catch(e) {}

// ── Define snapshot regions ──
var regions = [];

function buildRegions() {
    if (!gameObj) return;
    regions = [];

    // Game object from FUN_00623d50 — scan broadly
    // Scan in 0x200 byte chunks across the full object
    var scanRanges = [
        [0x0, 0x200],        // Early fields (state, flags, indices)
        [0x40, 0x80],        // Near vtable area
        [0x100, 0x200],      // Control structures
        [0x200, 0x400],      // More state
        [0x400, 0x600],
        [0x600, 0x800],
        [0x800, 0xa00],
        [0xa00, 0xc00],
        [0xc00, 0xe00],
        [0xe00, 0x1000],
        [0xf900, 0xfa00],    // Near menuFlag (+0xf9c6)
        [0x10d00, 0x10e00],  // Near titleFlag (+0x10d50), swfPtr (+0x10dc8)
        [0x10e00, 0x10f00],
        [0x11000, 0x11200],  // Higher offsets
        [0x11500, 0x11700],  // Near known in-game cursor offsets
    ];

    for (var i = 0; i < scanRanges.length; i++) {
        var start = scanRanges[i][0];
        var size = scanRanges[i][1] - scanRanges[i][0];
        regions.push({
            name: 'GameObj+0x' + start.toString(16),
            addr: gameObj.add(start),
            size: size,
            baseOffset: start
        });
    }

    // Static game object (DAT_00ccb9d8) — different object, scan key areas
    if (gameObjStatic && !gameObjStatic.isNull()) {
        regions.push({
            name: 'StaticObj+0x0',
            addr: gameObjStatic,
            size: 0x200,
            baseOffset: 0
        });
        regions.push({
            name: 'StaticObj+0x200',
            addr: gameObjStatic.add(0x200),
            size: 0x200,
            baseOffset: 0x200
        });
    }

    // Global cursor-related areas (from decompile analysis)
    // DAT_01841bd4 area (RVA 0x13E1bd4)
    try {
        regions.push({
            name: 'CursorGlobals',
            addr: rva(0x13E1b00),
            size: 0x200,
            baseOffset: 0
        });
    } catch(e) {}

    // DAT_01a86108 area (menu array, RVA 0x1606108)
    try {
        var menuBase = rva(0x1606108).readPointer();
        if (!menuBase.isNull()) {
            regions.push({
                name: 'MenuArray[0]',
                addr: menuBase,
                size: 0x340,
                baseOffset: 0
            });
        }
    } catch(e) {}

    // Singleton DAT_00cc96f8 (RVA 0x8C96F8) — returned by FUN_006145d0
    try {
        regions.push({
            name: 'Singleton_cc96f8',
            addr: rva(0x8C96F8),
            size: 0x200,
            baseOffset: 0
        });
    } catch(e) {}

    logBoth('[*] Regions configured: ' + regions.length + ' areas to scan');
}

// ── Snapshot / Diff ──
var snapshotA = null;
var snapCount = 0;

function takeSnapshot() {
    var snap = {};
    for (var i = 0; i < regions.length; i++) {
        var r = regions[i];
        try {
            snap[r.name] = {
                addr: r.addr,
                size: r.size,
                baseOffset: r.baseOffset,
                data: new Uint8Array(r.addr.readByteArray(r.size))
            };
        } catch(e) {
            snap[r.name] = null;
        }
    }
    return snap;
}

function diffSnapshots(a, b) {
    var changes = [];
    for (var name in a) {
        if (!a[name] || !b[name]) continue;
        var dA = a[name].data;
        var dB = b[name].data;
        var addr = a[name].addr;
        var baseOff = a[name].baseOffset;

        for (var i = 0; i < dA.length; i++) {
            if (dA[i] !== dB[i]) {
                changes.push({
                    region: name,
                    offset: i,
                    structOffset: baseOff + i,
                    addr: addr.add(i),
                    ghidra: toGhidra(addr.add(i)),
                    oldVal: dA[i],
                    newVal: dB[i],
                    // Also read as 16 and 32 bit values for context
                    old16: (i + 1 < dA.length) ? dA[i] | (dA[i+1] << 8) : dA[i],
                    new16: (i + 1 < dB.length) ? dB[i] | (dB[i+1] << 8) : dB[i],
                });
            }
        }
    }
    return changes;
}

// ── RPC exports for Python host ──
rpc.exports = {
    snapshot: function() {
        if (regions.length === 0) {
            return 'Game object not captured yet — wait for a frame tick, then try again.';
        }

        snapCount++;
        var snap = takeSnapshot();

        if (snapshotA === null) {
            snapshotA = snap;
            return '[SNAP ' + snapCount + '] Snapshot A taken (' + regions.length +
                ' regions). Now press UP or DOWN in the game, then take another snapshot.';
        }

        var snapshotB = snap;
        var changes = diffSnapshots(snapshotA, snapshotB);

        logFile('[DIFF] ' + changes.length + ' bytes changed');

        // Group changes by region for readability
        var byRegion = {};
        for (var i = 0; i < changes.length; i++) {
            var c = changes[i];
            if (!byRegion[c.region]) byRegion[c.region] = [];
            byRegion[c.region].push(c);
        }

        var summary = '[DIFF] ' + changes.length + ' bytes changed across ' +
            Object.keys(byRegion).length + ' regions:';
        logBoth(summary);

        var consoleLines = 0;
        for (var region in byRegion) {
            var rChanges = byRegion[region];
            logBoth('  ' + region + ': ' + rChanges.length + ' changes');
            for (var j = 0; j < rChanges.length; j++) {
                var c = rChanges[j];
                var line = '    +0x' + c.structOffset.toString(16) +
                    ' (ghidra 0x' + c.ghidra.toString(16) + '): ' +
                    c.oldVal + ' -> ' + c.newVal +
                    ' (u16: ' + c.old16 + '->' + c.new16 + ')';
                logFile(line);
                // Console: limit to 40 most interesting changes
                if (consoleLines < 40) {
                    logBoth(line);
                    consoleLines++;
                }
            }
        }

        if (consoleLines >= 40) {
            logBoth('  ... more in log file');
        }

        // B becomes the new A for next diff
        snapshotA = snapshotB;
        return summary + '\n[*] Snapshot B is now A. Press up/down, then snapshot again.';
    },

    status: function() {
        var msg = 'gameObj=' + (gameObj ? '0x' + gameObj.toString(16) : 'null') +
            ' regions=' + regions.length + ' snapshots=' + snapCount;
        logBoth('[STATUS] ' + msg);
        return msg;
    }
};

logBoth('[*] Ready. Python host: ENTER=snapshot, s=status, q=quit');
