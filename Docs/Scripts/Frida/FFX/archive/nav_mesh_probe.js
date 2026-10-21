// @python
// @desc Navmesh probe — validate VPA triangle/vertex reading, pointer patterns
//
// Keyboard (via Python host):
//   M = Print navmesh summary (tri count, vertex count, scale, blocked count)
//   T = Dump all triangles to log file
//   V = Dump first 20 vertices to console
//   Q = Quit

var logPath = 'D:/Games/Dev/Custom/FFX/ffx-x2-screen-reader/Docs/Scripts/Frida/output/FFX/nav_mesh_probe.log';
var logHandle = new File(logPath, 'w');
function logFile(msg) { logHandle.write(msg + '\n'); logHandle.flush(); }
function logBoth(msg) { console.log(msg); logFile(msg); }

var base = Process.enumerateModules()[0].base;
function rva(offset) { return base.add(offset); }

logBoth('[*] nav_mesh_probe — VPA navmesh discovery probe');
logBoth('[*] Base: ' + base);

// VPA struct sizes (from Fahrenheit vpa.cs)
var VPATRI_SIZE = 0x10;
var VPAVERTEX_SIZE = 0x08;

// VpaTri offsets within 0x10 bytes
// +0x00: vertex_indices[3] (short[3] = 6 bytes)
// +0x06: neighbour_indices[3] (short[3] = 6 bytes)
// +0x0C: data (int, bitfield)

// VpaVertex: short x, y, z, unused (8 bytes)

// RVAs for navmesh globals (from Fahrenheit gstate.cs)
// These are addresses within the game's data segment.
// Need to determine: do they contain the data directly, or pointers to data elsewhere?
var RVA_TRIS     = 0xF01A44;
var RVA_TRICOUNT = 0xF01A48;
var RVA_VERTICES = 0xF01A4C;
var RVA_SCALE    = 0xF01A50;
var RVA_NAVMESH  = 0xF01A54;

function extractBits(val, start, count) {
    return (val >>> start) & ((1 << count) - 1);
}

function collisionGroupName(group) {
    switch (group) {
        case 0: return 'Pass';
        case 1: return 'BlockAll';
        case 2: return 'BlockNPC';
        case 14: return 'BlockPlayer';
        default: return 'Unknown(' + group + ')';
    }
}

// Try both pointer patterns and report which works
function probeNavmesh() {
    logBoth('[*] Probing navmesh pointer patterns...');
    logBoth('');

    // Read tri_count — this should be a plain integer at the address
    var triCountAddr = rva(RVA_TRICOUNT);
    var triCount = triCountAddr.readS32();
    logBoth('[*] tri_count at rva(0xF01A48) = ' + triCount);

    // Read scale
    var scaleAddr = rva(RVA_SCALE);
    var scaleVal = scaleAddr.readFloat();
    logBoth('[*] scale at rva(0xF01A50) = ' + scaleVal);

    // Test tris pointer — Pattern A: direct (unlikely, would overlap)
    // Test tris pointer — Pattern B: indirect (read pointer, then data)
    var trisAddr = rva(RVA_TRIS);
    var trisRaw = trisAddr.readU32();
    logBoth('[*] Raw value at rva(0xF01A44) = 0x' + trisRaw.toString(16));

    var verticesAddr = rva(RVA_VERTICES);
    var verticesRaw = verticesAddr.readU32();
    logBoth('[*] Raw value at rva(0xF01A4C) = 0x' + verticesRaw.toString(16));

    var navmeshAddr = rva(RVA_NAVMESH);
    var navmeshRaw = navmeshAddr.readU32();
    logBoth('[*] Raw value at rva(0xF01A54) = 0x' + navmeshRaw.toString(16));

    // These raw values are likely pointers (large addresses).
    // If they look like valid addresses (> 0x10000), treat them as pointers.
    var trisBase = null;
    var verticesBase = null;

    if (trisRaw > 0x10000) {
        trisBase = ptr(trisRaw);
        logBoth('[*] Interpreting tris as POINTER -> ' + trisBase);
    }
    if (verticesRaw > 0x10000) {
        verticesBase = ptr(verticesRaw);
        logBoth('[*] Interpreting vertices as POINTER -> ' + verticesBase);
    }

    // Validate: read first triangle and check if vertex indices are reasonable
    if (trisBase && triCount > 0) {
        logBoth('');
        logBoth('[*] Validating first triangle (Pattern B: indirect pointer)...');
        var tri0 = trisBase;
        var v0 = tri0.readS16();
        var v1 = tri0.add(2).readS16();
        var v2 = tri0.add(4).readS16();
        var n0 = tri0.add(6).readS16();
        var n1 = tri0.add(8).readS16();
        var n2 = tri0.add(10).readS16();
        var data = tri0.add(12).readS32();

        logBoth('  tri[0]: verts=[' + v0 + ',' + v1 + ',' + v2 +
            '] neighbours=[' + n0 + ',' + n1 + ',' + n2 +
            '] data=0x' + (data >>> 0).toString(16));

        var collGroup = extractBits(data, 0, 7);
        var battleZone = extractBits(data, 7, 2);
        var location = extractBits(data, 11, 2);
        var soundType = extractBits(data, 15, 2);

        logBoth('  collision=' + collisionGroupName(collGroup) +
            ' battleZone=' + battleZone +
            ' location=' + location +
            ' soundType=' + soundType);

        // Resolve vertex world coordinates
        if (verticesBase && v0 >= 0 && v1 >= 0 && v2 >= 0) {
            for (var vi = 0; vi < 3; vi++) {
                var vIdx = (vi === 0) ? v0 : (vi === 1) ? v1 : v2;
                var vAddr = verticesBase.add(vIdx * VPAVERTEX_SIZE);
                var vx = vAddr.readS16();
                var vy = vAddr.add(2).readS16();
                var vz = vAddr.add(4).readS16();
                var worldX = vx / scaleVal;
                var worldY = vy / scaleVal;
                var worldZ = vz / scaleVal;
                logBoth('  vert[' + vIdx + ']: raw=(' + vx + ',' + vy + ',' + vz +
                    ') world=(' + worldX.toFixed(1) + ',' + worldY.toFixed(1) + ',' + worldZ.toFixed(1) + ')');
            }
        }

        // Check if pattern seems valid
        if (v0 >= 0 && v0 < 50000 && v1 >= 0 && v1 < 50000 && v2 >= 0 && v2 < 50000) {
            logBoth('[OK] Pattern B (indirect pointer) produces valid vertex indices');
        } else {
            logBoth('[!] Pattern B produced suspicious vertex indices — may need different approach');
        }
    }

    // Check neighbour sentinel values
    if (trisBase && triCount > 0) {
        logBoth('');
        logBoth('[*] Checking neighbour sentinel values across all triangles...');
        var negativeNeighbours = 0;
        var maxNeighbour = 0;
        var sentinelValues = {};

        for (var i = 0; i < triCount && i < 10000; i++) {
            var triAddr = trisBase.add(i * VPATRI_SIZE);
            for (var ni = 0; ni < 3; ni++) {
                var nVal = triAddr.add(6 + ni * 2).readS16();
                if (nVal < 0) {
                    negativeNeighbours++;
                    var key = '' + nVal;
                    sentinelValues[key] = (sentinelValues[key] || 0) + 1;
                }
                if (nVal > maxNeighbour) maxNeighbour = nVal;
            }
        }

        logBoth('  Negative neighbour count: ' + negativeNeighbours);
        logBoth('  Max valid neighbour index: ' + maxNeighbour + ' (tri_count=' + triCount + ')');
        var sentKeys = Object.keys(sentinelValues);
        for (var si = 0; si < sentKeys.length; si++) {
            logBoth('  Sentinel ' + sentKeys[si] + ': ' + sentinelValues[sentKeys[si]] + ' occurrences');
        }
    }

    // Summary
    logBoth('');
    if (triCount > 0 && trisBase) {
        // Count blocked triangles
        var blockedCount = 0;
        var passCount = 0;
        for (var i = 0; i < triCount && i < 10000; i++) {
            var d = trisBase.add(i * VPATRI_SIZE + 12).readS32();
            var cg = extractBits(d, 0, 7);
            if (cg === 0) passCount++;
            else blockedCount++;
        }
        logBoth('[SUMMARY] Navmesh: ' + triCount + ' triangles, scale=' + scaleVal.toFixed(4) +
            '. ' + passCount + ' walkable, ' + blockedCount + ' blocked.');
    } else {
        logBoth('[!] Could not read navmesh — may not be loaded (check if in field mode)');
    }
}

function dumpAllTriangles() {
    var triCount = rva(RVA_TRICOUNT).readS32();
    var trisRaw = rva(RVA_TRIS).readU32();
    var verticesRaw = rva(RVA_VERTICES).readU32();
    var scaleVal = rva(RVA_SCALE).readFloat();

    if (triCount <= 0 || trisRaw < 0x10000) {
        logBoth('[!] No navmesh loaded');
        return;
    }

    var trisBase = ptr(trisRaw);
    var verticesBase = ptr(verticesRaw);

    logFile('--- FULL TRIANGLE DUMP ---');
    logFile('tri_count=' + triCount + ' scale=' + scaleVal);

    for (var i = 0; i < triCount; i++) {
        var t = trisBase.add(i * VPATRI_SIZE);
        var v0 = t.readS16(), v1 = t.add(2).readS16(), v2 = t.add(4).readS16();
        var n0 = t.add(6).readS16(), n1 = t.add(8).readS16(), n2 = t.add(10).readS16();
        var data = t.add(12).readS32();
        var cg = extractBits(data, 0, 7);

        // Compute centroid
        var cx = 0, cy = 0, cz = 0;
        var indices = [v0, v1, v2];
        for (var vi = 0; vi < 3; vi++) {
            var vAddr = verticesBase.add(indices[vi] * VPAVERTEX_SIZE);
            cx += vAddr.readS16() / scaleVal;
            cy += vAddr.add(2).readS16() / scaleVal;
            cz += vAddr.add(4).readS16() / scaleVal;
        }
        cx /= 3; cy /= 3; cz /= 3;

        logFile('tri[' + i + ']: v=[' + v0 + ',' + v1 + ',' + v2 +
            '] n=[' + n0 + ',' + n1 + ',' + n2 +
            '] coll=' + collisionGroupName(cg) +
            ' centroid=(' + cx.toFixed(1) + ',' + cy.toFixed(1) + ',' + cz.toFixed(1) + ')');
    }

    logBoth('[*] Dumped ' + triCount + ' triangles to log file.');
}

function dumpVertices() {
    var verticesRaw = rva(RVA_VERTICES).readU32();
    var scaleVal = rva(RVA_SCALE).readFloat();

    if (verticesRaw < 0x10000) {
        logBoth('[!] No vertices loaded');
        return;
    }

    var verticesBase = ptr(verticesRaw);
    logBoth('[*] First 20 vertices (scale=' + scaleVal + '):');

    for (var i = 0; i < 20; i++) {
        var v = verticesBase.add(i * VPAVERTEX_SIZE);
        var x = v.readS16(), y = v.add(2).readS16(), z = v.add(4).readS16();
        logBoth('  v[' + i + ']: raw=(' + x + ',' + y + ',' + z +
            ') world=(' + (x / scaleVal).toFixed(2) + ',' + (y / scaleVal).toFixed(2) + ',' + (z / scaleVal).toFixed(2) + ')');
    }
}

rpc.exports = {
    mesh: function() { probeNavmesh(); return 'ok'; },
    dump: function() { dumpAllTriangles(); return 'ok'; },
    verts: function() { dumpVertices(); return 'ok'; }
};
