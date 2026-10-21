// @python
// @desc Trigger scanner — scan memory for objects with positions near known exit locations
//
// We know the inn exit is southeast of center, near MapEntrance[0] at (-0.56, 0, -55.41).
// This probe scans loaded memory for float triplets matching exit-region coordinates
// and identifies what objects contain them.
//
// Keyboard:
//   s = Scan for objects near MapEntrance positions
//   q = Quit

var logPath = 'D:/Games/Dev/Custom/FFX/ffx-x2-screen-reader/Docs/Scripts/Frida/output/FFX/nav_trigger_scan.log';
var logHandle = new File(logPath, 'w');
function logFile(msg) { logHandle.write(msg + '\n'); logHandle.flush(); }
function logBoth(msg) { console.log(msg); logFile(msg); }

var base = Process.enumerateModules()[0].base;
function rva(offset) { return base.add(offset); }

logBoth('[*] nav_trigger_scan — Memory scan for exit trigger objects');

function scanForExits() {
    // Read MapEntrance positions from the current script chunk
    var entrances = [];
    try {
        var wp = rva(0xF270A4).readPointer();
        if (!wp.isNull()) {
            var chunk = wp.add(0x04).readPointer();
            if (!chunk.isNull() && chunk.toUInt32() > 0x10000) {
                var mapStart = chunk.add(0x04).readU32();
                var offsetAuthor = chunk.add(0x08).readU32();
                var count = Math.floor((offsetAuthor - mapStart) / 0x20);
                for (var i = 0; i < count && i < 10; i++) {
                    var addr = chunk.add(mapStart + i * 0x20);
                    entrances.push({
                        x: addr.add(0x0C).readFloat(),
                        y: addr.add(0x10).readFloat(),
                        z: addr.add(0x14).readFloat()
                    });
                    logBoth('[ENTRANCE ' + i + '] (' + entrances[i].x.toFixed(2) + ',' + entrances[i].y.toFixed(2) + ',' + entrances[i].z.toFixed(2) + ')');
                }
            }
        }
    } catch (e) { logBoth('[!] Could not read entrances: ' + e.message); }

    if (entrances.length === 0) {
        logBoth('[!] No entrances found');
        return;
    }

    // For each entrance, scan memory for float values matching the Z coordinate
    // The exit trigger should have a position near (but not identical to) the entrance
    // Scan the game's heap regions for matching floats
    logBoth('[*] Scanning memory for objects near entrance positions...');

    var modules = Process.enumerateModules();
    var gameModule = modules[0];
    var results = [];

    // For each entrance, search for its Z coordinate as a float in memory
    for (var ei = 0; ei < entrances.length; ei++) {
        var targetZ = entrances[ei].z;
        var targetX = entrances[ei].x;

        // Create byte pattern for the Z float (±2 units tolerance)
        // Use Memory.scanSync to find the float value
        var zBytes = new ArrayBuffer(4);
        new DataView(zBytes).setFloat32(0, targetZ, true);
        var pattern = '';
        var arr = new Uint8Array(zBytes);
        for (var b = 0; b < 4; b++) {
            pattern += (arr[b] < 16 ? '0' : '') + arr[b].toString(16) + ' ';
        }
        logBoth('[SEARCH] Entrance ' + ei + ' Z=' + targetZ.toFixed(2) + ' pattern: ' + pattern.trim());

        // Scan the game module's memory range
        var scanResults = Memory.scanSync(gameModule.base, gameModule.size, pattern.trim());

        logBoth('[SEARCH] Found ' + scanResults.length + ' matches for Z=' + targetZ.toFixed(2) + ' in game module');

        // Also scan broader memory (heap allocations)
        // Try scanning ranges near known data structures
        var heapRanges = Process.enumerateRanges('r--');
        var heapMatches = 0;

        for (var ri = 0; ri < heapRanges.length && heapMatches < 200; ri++) {
            var range = heapRanges[ri];
            if (range.size > 50000000) continue; // skip huge regions
            if (range.size < 1000) continue;

            try {
                var matches = Memory.scanSync(range.base, range.size, pattern.trim());
                for (var mi = 0; mi < matches.length && heapMatches < 200; mi++) {
                    var matchAddr = matches[mi].address;

                    // Check if there are valid X,Y floats nearby (at -8 and -4)
                    // that form a position triplet (X, Y, Z)
                    try {
                        var foundX = matchAddr.sub(8).readFloat();
                        var foundY = matchAddr.sub(4).readFloat();
                        var foundZ = matchAddr.readFloat();

                        // Check if X is also near the entrance X
                        if (Math.abs(foundX - targetX) < 50 && Math.abs(foundY) < 100) {
                            heapMatches++;
                            var info = 'addr=' + matchAddr.sub(8) +
                                ' pos=(' + foundX.toFixed(2) + ',' + foundY.toFixed(2) + ',' + foundZ.toFixed(2) + ')';

                            // Try to identify what object this belongs to
                            // Read a few bytes before the position to look for vtable pointers
                            var context = '';
                            try {
                                // Check 16-64 bytes before the position for vtable-like pointers
                                for (var voff = 16; voff <= 64; voff += 4) {
                                    var possibleVtable = matchAddr.sub(8 + voff).readU32();
                                    if (possibleVtable > gameModule.base.toUInt32() &&
                                        possibleVtable < gameModule.base.toUInt32() + gameModule.size) {
                                        context += ' vtable?@-' + (8+voff) + '=0x' + possibleVtable.toString(16);
                                    }
                                }
                            } catch (e) {}

                            logBoth('[MATCH] Entrance ' + ei + ': ' + info + context);
                            results.push({ entrance: ei, addr: matchAddr.sub(8), x: foundX, y: foundY, z: foundZ });
                        }
                    } catch (e) {}
                }
            } catch (e) {}
        }
    }

    logBoth('[DONE] Total matches: ' + results.length);
}

rpc.exports = {
    scan: function() { scanForExits(); return 'ok'; }
};
