// ARCHIVED — BUG: reads from global DAT_ pointers instead of ECX. See track_cursor_native.js for fixed version.
// @python
// @desc Snapshot menu memory, diff on keypress to find selection index
//
// Uses Python host for keyboard input. Workflow:
//   1. Script attaches, reads menu globals, takes initial snapshot
//   2. Press ENTER in the terminal → takes snapshot A
//   3. Press up/down on title screen in game
//   4. Press ENTER in the terminal → takes snapshot B, diffs A vs B
//   5. Repeat to confirm which offset is the selection index
//
// Targets:
//   DAT_01a86108 (Ghidra VA) — menu instance array base pointer
//   DAT_00ccb9d8 (Ghidra VA) — main game object (~69KB struct)
//   DAT_012fbbf8 (Ghidra VA) — game state variable

'use strict';

var logPath = 'D:/Games/Dev/Custom/FFX/ffx-x2-screen-reader/Docs/Scripts/Frida/output/FFX/diff_menu_state.log';
var logHandle = new File(logPath, 'w');

function logFile(msg) {
    logHandle.write(msg + '\n');
    logHandle.flush();
}

function logBoth(msg) {
    console.log(msg);
    logFile(msg);
}

// ── Ghidra rebase (proven: base 0x400000, offset 0x680000) ──
var ffxMod = Process.enumerateModules()[0];
var runtimeBase = ffxMod.base;
var rebaseOffset = runtimeBase.sub(0x00400000);

function g(ghidraAddr) {
    return ptr(ghidraAddr).add(rebaseOffset);
}

logBoth('[*] diff_menu_state (rebase 0x' + rebaseOffset.toString(16) + ')');

// ── Read key global addresses ──
var pMenuArray = g(0x01a86108);    // menu instance array base pointer
var pGameObj = g(0x00ccb9d8);      // main game object pointer
var pGameState = g(0x012fbbf8);    // game state variable

logBoth('[*] Menu array ptr @ ' + pMenuArray);
logBoth('[*] Game obj ptr   @ ' + pGameObj);
logBoth('[*] Game state     @ ' + pGameState);

// Read current values
var menuArrayBase = pMenuArray.readPointer();
var gameObjBase = pGameObj.readPointer();
var gameState = pGameState.readU32();

logBoth('[*] Menu array base: 0x' + menuArrayBase.toString(16));
logBoth('[*] Game obj base:   0x' + gameObjBase.toString(16));
logBoth('[*] Game state:      0x' + gameState.toString(16));

// ── Define memory regions to snapshot ──
// Menu struct is 0x340 bytes per entry. Snapshot first 4 entries (0xD00 bytes)
// Also snapshot parts of the game object near known offsets

var regions = [];

if (!menuArrayBase.isNull()) {
    // First 4 menu instances (0x340 * 4 = 0xD00 bytes)
    regions.push({name: 'MenuArray[0-3]', addr: menuArrayBase, size: 0xD00});
    // Also check menu entries 4-7
    regions.push({name: 'MenuArray[4-7]', addr: menuArrayBase.add(0xD00), size: 0xD00});
} else {
    logBoth('[!] Menu array base is NULL — menu system may not be initialized');
}

if (!gameObjBase.isNull()) {
    // Game object: scan around known menu-related offsets
    // +0xf9bc area (menu flags), +0x10d50 area (title screen flag)
    regions.push({name: 'GameObj+0xF900', addr: gameObjBase.add(0xF900), size: 0x200});
    regions.push({name: 'GameObj+0x10D00', addr: gameObjBase.add(0x10D00), size: 0x100});
}

// Also snapshot the game state variable area (might have nearby menu index)
regions.push({name: 'GameState area', addr: pGameState.sub(0x40), size: 0x100});

// ── Snapshot function ──
var snapshots = [];

function takeSnapshot() {
    var snap = {};
    for (var i = 0; i < regions.length; i++) {
        var r = regions[i];
        try {
            snap[r.name] = {
                addr: r.addr,
                size: r.size,
                data: r.addr.readByteArray(r.size)
            };
        } catch (e) {
            logFile('[!] Cannot read ' + r.name + ' at ' + r.addr + ': ' + e.message);
            snap[r.name] = null;
        }
    }
    snap._gameState = pGameState.readU32();
    return snap;
}

// ── Diff function ──
function diffSnapshots(a, b) {
    var changes = [];
    for (var name in a) {
        if (name.startsWith('_')) continue;
        if (!a[name] || !b[name]) continue;

        var dataA = new Uint8Array(a[name].data);
        var dataB = new Uint8Array(b[name].data);
        var addr = a[name].addr;

        for (var i = 0; i < dataA.length; i++) {
            if (dataA[i] !== dataB[i]) {
                changes.push({
                    region: name,
                    offset: i,
                    addr: addr.add(i),
                    oldVal: dataA[i],
                    newVal: dataB[i]
                });
            }
        }
    }

    if (a._gameState !== b._gameState) {
        changes.push({
            region: 'GameState',
            offset: 0,
            addr: pGameState,
            oldVal: a._gameState,
            newVal: b._gameState
        });
    }

    return changes;
}

// ── RPC functions called by Python host ──
var snapshotA = null;
var snapshotCount = 0;

rpc.exports = {
    snapshot: function() {
        snapshotCount++;
        var snap = takeSnapshot();

        if (snapshotA === null) {
            snapshotA = snap;
            var msg = '[SNAP ' + snapshotCount + '] Snapshot A taken. Now press up/down in game, then take another snapshot.';
            logBoth(msg);
            return msg;
        } else {
            var snapshotB = snap;
            var changes = diffSnapshots(snapshotA, snapshotB);

            logFile('[SNAP ' + snapshotCount + '] Snapshot B taken. Diffing...');
            logFile('Changes found: ' + changes.length);

            var summary = '[DIFF] ' + changes.length + ' bytes changed:';
            logBoth(summary);

            for (var i = 0; i < changes.length; i++) {
                var c = changes[i];
                var line = '  ' + c.region + ' +0x' + c.offset.toString(16) +
                    ' (0x' + c.addr.toString(16) + '): ' +
                    c.oldVal + ' -> ' + c.newVal;
                logFile(line);
                // Console: show first 30 changes only
                if (i < 30) {
                    logBoth(line);
                }
            }

            if (changes.length > 30) {
                logBoth('  ... and ' + (changes.length - 30) + ' more (see log)');
            }

            // Reset for next diff
            snapshotA = snapshotB;
            logBoth('[*] Snapshot B is now A. Press up/down, then snapshot again.');
            return summary;
        }
    },

    status: function() {
        var state = pGameState.readU32();
        var msg = 'Game state: 0x' + state.toString(16) + ', snapshots: ' + snapshotCount;
        logBoth('[STATUS] ' + msg);
        return msg;
    }
};

logBoth('[*] Ready. Use Python host: press ENTER to take snapshots.');
