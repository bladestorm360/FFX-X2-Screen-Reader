// @python
// @desc Player triangle probe — find current navmesh tri, calibrate cardinal directions
//
// Keyboard (via Python host):
//   S = Snapshot full actor[0] struct (0x880 bytes) — walk, then S again to diff
//   D = Diff current snapshot with previous (find candidate triangle index fields)
//   F = Find player's triangle via point-in-triangle test (barycentric XZ)
//   N = Record position for north calibration (press, walk screen-up, press again)
//   Q = Quit

var logPath = 'D:/Games/Dev/Custom/FFX/ffx-x2-screen-reader/Docs/Scripts/Frida/output/FFX/nav_player_tri_probe.log';
var logHandle = new File(logPath, 'w');
function logFile(msg) { logHandle.write(msg + '\n'); logHandle.flush(); }
function logBoth(msg) { console.log(msg); logFile(msg); }

var base = Process.enumerateModules()[0].base;
function rva(offset) { return base.add(offset); }

logBoth('[*] nav_player_tri_probe — Triangle finder + cardinal calibration');

var ACTOR_SIZE = 0x880;
var VPATRI_SIZE = 0x10;
var VPAVERTEX_SIZE = 0x08;

// Navmesh RVAs
var RVA_TRIS     = 0xF01A44;
var RVA_TRICOUNT = 0xF01A48;
var RVA_VERTICES = 0xF01A4C;
var RVA_SCALE    = 0xF01A50;

// State for snapshots
var prevSnapshot = null;
var curSnapshot = null;

// State for north calibration
var northCalibStart = null;

function getPlayerActor() {
    var actorsBase = rva(0x1FC44E4).readPointer();
    if (actorsBase.isNull()) return null;
    return actorsBase; // actor[0]
}

function getPlayerPos() {
    var actor = getPlayerActor();
    if (!actor) return null;
    return {
        x: actor.add(0x0C).readFloat(),
        y: actor.add(0x10).readFloat(),
        z: actor.add(0x14).readFloat()
    };
}

function snapshotActor() {
    var actor = getPlayerActor();
    if (!actor) {
        logBoth('[!] Actor array NULL');
        return;
    }

    prevSnapshot = curSnapshot;
    curSnapshot = actor.readByteArray(ACTOR_SIZE);
    var pos = getPlayerPos();

    logBoth('[SNAP] Actor[0] snapshot taken. pos=(' +
        pos.x.toFixed(2) + ', ' + pos.y.toFixed(2) + ', ' + pos.z.toFixed(2) + ')');

    if (prevSnapshot) {
        logBoth('[*] Previous snapshot exists. Press D to diff.');
    }
}

function diffSnapshots() {
    if (!prevSnapshot || !curSnapshot) {
        logBoth('[!] Need 2 snapshots. Press S, walk, press S again, then D.');
        return;
    }

    var prev = new Uint8Array(prevSnapshot);
    var cur = new Uint8Array(curSnapshot);

    logBoth('[DIFF] Comparing actor[0] snapshots (0x880 bytes)...');
    logFile('');
    logFile('--- FULL DIFF ---');

    var changedOffsets = [];

    // Check every 2-byte and 4-byte aligned value
    for (var off = 0; off < ACTOR_SIZE - 1; off += 2) {
        var prevU16 = prev[off] | (prev[off + 1] << 8);
        var curU16 = cur[off] | (cur[off + 1] << 8);

        if (prevU16 !== curU16) {
            // Also check as signed 16
            var prevS16 = prevU16 > 0x7FFF ? prevU16 - 0x10000 : prevU16;
            var curS16 = curU16 > 0x7FFF ? curU16 - 0x10000 : curU16;

            var entry = {
                offset: off,
                prevU16: prevU16,
                curU16: curU16,
                prevS16: prevS16,
                curS16: curS16
            };

            // Check 4-byte values at aligned offsets
            if (off % 4 === 0 && off < ACTOR_SIZE - 3) {
                var prevU32 = prev[off] | (prev[off+1] << 8) | (prev[off+2] << 16) | (prev[off+3] << 24);
                var curU32 = cur[off] | (cur[off+1] << 8) | (cur[off+2] << 16) | (cur[off+3] << 24);

                // Float interpretation
                var buf = new ArrayBuffer(4);
                var view = new DataView(buf);
                view.setUint32(0, prevU32, true);
                var prevF = view.getFloat32(0, true);
                view.setUint32(0, curU32, true);
                var curF = view.getFloat32(0, true);

                entry.prevU32 = prevU32;
                entry.curU32 = curU32;
                entry.prevFloat = prevF;
                entry.curFloat = curF;
            }

            changedOffsets.push(entry);
        }
    }

    // Console: show only small integer changes (candidate triangle indices)
    var candidates = [];
    var knownFloatOffsets = [0x0C, 0x10, 0x14, 0x18, 0x1C, 0x20, 0x24, 0x28, 0x154, 0x158, 0x168];

    for (var i = 0; i < changedOffsets.length; i++) {
        var e = changedOffsets[i];
        var isKnownFloat = false;
        for (var k = 0; k < knownFloatOffsets.length; k++) {
            if (e.offset >= knownFloatOffsets[k] && e.offset < knownFloatOffsets[k] + 4) {
                isKnownFloat = true;
                break;
            }
        }

        // Log ALL changes to file
        var line = '  +0x' + e.offset.toString(16).padStart(3, '0') + ': u16=' + e.prevU16 + '->' + e.curU16;
        if (e.prevFloat !== undefined) {
            line += ' float=' + e.prevFloat.toFixed(4) + '->' + e.curFloat.toFixed(4);
        }
        logFile(line);

        // Candidate triangle index: small integer (0-5000), not a known float offset
        if (!isKnownFloat && e.curS16 >= 0 && e.curS16 < 5000 && e.prevS16 >= 0 && e.prevS16 < 5000) {
            candidates.push(e);
        }
    }

    logBoth('[DIFF] ' + changedOffsets.length + ' changed offsets total');
    logBoth('[*] Candidate triangle index fields (small ints 0-5000, not known floats):');

    if (candidates.length === 0) {
        logBoth('  (none found — triangle index may not be stored in Actor struct)');
    } else {
        for (var i = 0; i < candidates.length && i < 20; i++) {
            var c = candidates[i];
            logBoth('  +0x' + c.offset.toString(16).padStart(3, '0') +
                ': ' + c.prevS16 + ' -> ' + c.curS16);
        }
    }
    logBoth('[*] Full diff in log file.');
}

function findTriByPointInTri() {
    var pos = getPlayerPos();
    if (!pos) {
        logBoth('[!] Cannot read player position');
        return;
    }

    var triCount = rva(RVA_TRICOUNT).readS32();
    var trisRaw = rva(RVA_TRIS).readU32();
    var verticesRaw = rva(RVA_VERTICES).readU32();
    var scaleVal = rva(RVA_SCALE).readFloat();

    if (triCount <= 0 || trisRaw < 0x10000 || verticesRaw < 0x10000) {
        logBoth('[!] Navmesh not loaded');
        return;
    }

    var trisBase = ptr(trisRaw);
    var vertsBase = ptr(verticesRaw);

    logBoth('[*] Searching ' + triCount + ' triangles for player pos (' +
        pos.x.toFixed(2) + ', ' + pos.z.toFixed(2) + ') on XZ plane...');

    var px = pos.x;
    var pz = pos.z;
    var found = -1;
    var closestDist = 999999;
    var closestTri = -1;

    for (var i = 0; i < triCount; i++) {
        var t = trisBase.add(i * VPATRI_SIZE);
        var data = t.add(12).readS32();
        var cg = (data >>> 0) & 0x7F;

        // Get vertex positions
        var v0i = t.readS16();
        var v1i = t.add(2).readS16();
        var v2i = t.add(4).readS16();

        var ax = vertsBase.add(v0i * VPAVERTEX_SIZE).readS16() / scaleVal;
        var az = vertsBase.add(v0i * VPAVERTEX_SIZE + 4).readS16() / scaleVal;
        var bx = vertsBase.add(v1i * VPAVERTEX_SIZE).readS16() / scaleVal;
        var bz = vertsBase.add(v1i * VPAVERTEX_SIZE + 4).readS16() / scaleVal;
        var cx = vertsBase.add(v2i * VPAVERTEX_SIZE).readS16() / scaleVal;
        var cz = vertsBase.add(v2i * VPAVERTEX_SIZE + 4).readS16() / scaleVal;

        // Barycentric point-in-triangle test (XZ plane)
        var v0x = cx - ax, v0z = cz - az;
        var v1x = bx - ax, v1z = bz - az;
        var v2x = px - ax, v2z = pz - az;

        var dot00 = v0x * v0x + v0z * v0z;
        var dot01 = v0x * v1x + v0z * v1z;
        var dot02 = v0x * v2x + v0z * v2z;
        var dot11 = v1x * v1x + v1z * v1z;
        var dot12 = v1x * v2x + v1z * v2z;

        var invDenom = dot00 * dot11 - dot01 * dot01;
        if (Math.abs(invDenom) < 0.0001) continue;
        invDenom = 1.0 / invDenom;

        var u = (dot11 * dot02 - dot01 * dot12) * invDenom;
        var v = (dot00 * dot12 - dot01 * dot02) * invDenom;

        if (u >= -0.01 && v >= -0.01 && (u + v) <= 1.01) {
            found = i;
            logBoth('[FOUND] Player is on triangle ' + i +
                ' (collision=' + collisionGroupName(cg) + ')');

            // Show triangle details
            var n0 = t.add(6).readS16();
            var n1 = t.add(8).readS16();
            var n2 = t.add(10).readS16();
            logBoth('  verts=[' + v0i + ',' + v1i + ',' + v2i +
                '] neighbours=[' + n0 + ',' + n1 + ',' + n2 + ']');

            var centX = (ax + bx + cx) / 3;
            var centZ = (az + bz + cz) / 3;
            logBoth('  centroid=(' + centX.toFixed(2) + ',' + centZ.toFixed(2) + ')');
            break;
        }

        // Track closest triangle (for near-miss debugging)
        var centX = (ax + bx + cx) / 3;
        var centZ = (az + bz + cz) / 3;
        var d = Math.sqrt((px - centX) * (px - centX) + (pz - centZ) * (pz - centZ));
        if (d < closestDist) {
            closestDist = d;
            closestTri = i;
        }
    }

    if (found < 0) {
        logBoth('[!] Player not inside any triangle!');
        logBoth('  Closest triangle: ' + closestTri + ' (dist=' + closestDist.toFixed(2) + ')');
    }
}

function collisionGroupName(group) {
    switch (group) {
        case 0: return 'Pass';
        case 1: return 'BlockAll';
        case 2: return 'BlockNPC';
        case 14: return 'BlockPlayer';
        default: return 'Unk(' + group + ')';
    }
}

function northCalibrate() {
    var pos = getPlayerPos();
    if (!pos) {
        logBoth('[!] Cannot read player position');
        return;
    }

    if (!northCalibStart) {
        northCalibStart = { x: pos.x, y: pos.y, z: pos.z };
        logBoth('[NORTH] Start position recorded: (' +
            pos.x.toFixed(2) + ', ' + pos.y.toFixed(2) + ', ' + pos.z.toFixed(2) + ')');
        logBoth('[*] Now walk SCREEN-UP (north on screen) and press N again.');
    } else {
        var dx = pos.x - northCalibStart.x;
        var dy = pos.y - northCalibStart.y;
        var dz = pos.z - northCalibStart.z;

        logBoth('[NORTH] End position: (' +
            pos.x.toFixed(2) + ', ' + pos.y.toFixed(2) + ', ' + pos.z.toFixed(2) + ')');
        logBoth('[NORTH] Delta: dX=' + dx.toFixed(2) + ' dY=' + dy.toFixed(2) + ' dZ=' + dz.toFixed(2));

        // Determine which axis changed most
        var absDx = Math.abs(dx);
        var absDy = Math.abs(dy);
        var absDz = Math.abs(dz);

        if (absDx > absDz && absDx > absDy) {
            logBoth('[NORTH] Primary movement axis: X (' + (dx > 0 ? 'positive' : 'negative') + ')');
            logBoth('[NORTH] Screen-north = ' + (dx > 0 ? '+X' : '-X'));
        } else if (absDz > absDx && absDz > absDy) {
            logBoth('[NORTH] Primary movement axis: Z (' + (dz > 0 ? 'positive' : 'negative') + ')');
            logBoth('[NORTH] Screen-north = ' + (dz > 0 ? '+Z' : '-Z'));
        } else {
            logBoth('[NORTH] Primary movement axis: Y (' + (dy > 0 ? 'positive' : 'negative') + ')');
            logBoth('[NORTH] This is unexpected for horizontal movement — Y is usually vertical');
        }

        logBoth('[*] Also walk screen-RIGHT and press N twice to determine east axis.');
        northCalibStart = null;
    }
}

rpc.exports = {
    snapshot: function() { snapshotActor(); return 'ok'; },
    diff: function() { diffSnapshots(); return 'ok'; },
    findtri: function() { findTriByPointInTri(); return 'ok'; },
    north: function() { northCalibrate(); return 'ok'; }
};
