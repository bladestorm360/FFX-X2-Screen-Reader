// @python
// @desc Navigation — entity scan + crow-flies + A* pathfinding (integrated)
//
// Hotkeys (via Python host):
//   s = Scan nearby entities
//   n/p = Next / Previous entity in list
//   c = Crow-flies update (recalculate distance/direction)
//   g = Go — A* pathfind with turn-by-turn directions
//   r = Repeat last announcement
//   d = Debug info (triangles, positions)
//   +/- = Adjust scan range
//   q = Quit

var logPath = 'D:/Games/Dev/Custom/FFX/ffx-x2-screen-reader/Docs/Scripts/Frida/output/FFX/nav_integrated.log';
var logHandle = new File(logPath, 'w');
function logFile(msg) { logHandle.write(msg + '\n'); logHandle.flush(); }
function logBoth(msg) { console.log(msg); logFile(msg); }

var base = Process.enumerateModules()[0].base;
function rva(offset) { return base.add(offset); }

logBoth('[*] Navigation — integrated entity scan + pathfinding');

// ============================================================
//  Constants
// ============================================================

var ACTOR_SIZE = 0x880;
var MAX_ACTORS = 128;
var UNITS_PER_STEP = 11.13;
var VPATRI_SIZE = 0x10;
var VPAVERTEX_SIZE = 0x08;
var MAX_DISTANCE_UNITS = 0; // 0 = no limit

var RVA_TRIS     = 0xF01A44;
var RVA_TRICOUNT = 0xF01A48;
var RVA_VERTICES = 0xF01A4C;
var RVA_SCALE    = 0xF01A50;

// Atel — current worker is at this RVA (from Fahrenheit gstate.cs: ptr_at, no deref)
var RVA_CURRENT_WORKER = 0xF270A4;
// AtelBasicWorker offsets
var OFF_WORKER_SCRIPT_CHUNK = 0x04;
// AtelScriptChunk offsets
var OFF_CHUNK_MAP_START     = 0x04;
var OFF_CHUNK_OFFSET_AUTHOR = 0x08;
// MapEntrance offsets (0x20 bytes each)
var OFF_ENTRANCE_ROT = 0x08;
var OFF_ENTRANCE_X   = 0x0C;
var OFF_ENTRANCE_Y   = 0x10;
var OFF_ENTRANCE_Z   = 0x14;

var OFF_CHR_ID   = 0x00;
var OFF_CHR_EN   = 0x02;
var OFF_CHR_NAME = 0x04;
var OFF_CHR_POS  = 0x0C;

// ============================================================
//  Utilities
// ============================================================

function classifyEntity(id) {
    if (id === 0x5001) return 'Save Sphere';
    if (id === 0x5002) return 'Treasure Chest';
    if (id === 0x5004) return 'Shrine';
    if (id >= 0x5000) return 'Object';
    if (id >= 0x2000 && id < 0x5000) return 'NPC';
    if (id >= 0x1000 && id < 0x2000) return 'Monster';
    if (id <= 0x0012) return 'Player';
    return 'Unknown';
}

function entityLabel(e) {
    var label = e.category;
    if (e.name !== '?' && e.name !== e.category) label += ' "' + e.name + '"';
    return label;
}

function readName(addr) {
    try {
        var p = addr.add(OFF_CHR_NAME).readPointer();
        if (p.isNull()) return '?';
        var s = p.readCString();
        return (s && s.length > 0 && s.length < 200) ? s : '?';
    } catch (e) { return '?'; }
}

function readPos(addr) {
    return {
        x: addr.add(OFF_CHR_POS).readFloat(),
        y: addr.add(OFF_CHR_POS + 4).readFloat(),
        z: addr.add(OFF_CHR_POS + 8).readFloat()
    };
}

function getActorsBase() {
    var p = rva(0x1FC44E4).readPointer();
    return p.isNull() ? null : p;
}

function getPlayerPos() {
    var ab = getActorsBase();
    return ab ? readPos(ab) : null;
}

function distXZ(a, b) {
    var dx = a.x - b.x, dz = a.z - b.z;
    return Math.sqrt(dx * dx + dz * dz);
}

function cardinalDir(dx, dz) {
    if (Math.abs(dx) < 0.01 && Math.abs(dz) < 0.01) return 'here';
    var a = Math.atan2(dz, dx) * 180 / Math.PI;
    if (a >= -22.5 && a < 22.5) return 'east';
    if (a >= 22.5  && a < 67.5) return 'northeast';
    if (a >= 67.5  && a < 112.5) return 'north';
    if (a >= 112.5 && a < 157.5) return 'northwest';
    if (a >= 157.5 || a < -157.5) return 'west';
    if (a >= -157.5 && a < -112.5) return 'southwest';
    if (a >= -112.5 && a < -67.5) return 'south';
    return 'southeast';
}

// ============================================================
//  Navmesh
// ============================================================

var cachedNav = null;

function readNavmesh() {
    var triCount = rva(RVA_TRICOUNT).readS32();
    var trisRaw  = rva(RVA_TRIS).readU32();
    var vertsRaw = rva(RVA_VERTICES).readU32();
    var scale    = rva(RVA_SCALE).readFloat();

    if (triCount <= 0 || trisRaw < 0x10000 || vertsRaw < 0x10000 || scale <= 0) return null;

    var tB = ptr(trisRaw), vB = ptr(vertsRaw);
    var tris = [];

    for (var i = 0; i < triCount; i++) {
        var t = tB.add(i * VPATRI_SIZE);
        var vi0 = t.readS16(), vi1 = t.add(2).readS16(), vi2 = t.add(4).readS16();
        var n0  = t.add(6).readS16(), n1 = t.add(8).readS16(), n2 = t.add(10).readS16();
        var data = t.add(12).readS32();
        var coll = (data >>> 0) & 0x7F;
        var battleZone = (data >>> 7) & 0x3;
        var unk1 = (data >>> 9) & 0x3;
        var location = (data >>> 11) & 0x3;
        var unk2 = (data >>> 13) & 0x3;
        var soundType = (data >>> 15) & 0x3;
        var upper = (data >>> 17) & 0x7FFF;

        var vxs = [], vzs = [], cx = 0, cz = 0;
        var vis = [vi0, vi1, vi2];
        for (var j = 0; j < 3; j++) {
            var va = vB.add(vis[j] * VPAVERTEX_SIZE);
            var wx = va.readS16() / scale;
            var wz = va.add(4).readS16() / scale;
            vxs.push(wx); vzs.push(wz);
            cx += wx; cz += wz;
        }

        tris.push({
            verts: vis, neighbours: [n0, n1, n2], coll: coll,
            battleZone: battleZone, unk1: unk1, location: location,
            unk2: unk2, soundType: soundType, upper: upper,
            cx: cx / 3, cz: cz / 3, vxs: vxs, vzs: vzs
        });
    }

    return { tris: tris, triCount: triCount, scale: scale };
}

function getNav() {
    if (!cachedNav) cachedNav = readNavmesh();
    return cachedNav;
}

// Player can walk on Pass(0) and BlockNPC(2) — block everything else
function isWalkable(coll) {
    return coll === 0 || coll === 2;
}

// ============================================================
//  Point-in-triangle (XZ plane)
// ============================================================

function findTriangle(nav, px, pz) {
    for (var i = 0; i < nav.tris.length; i++) {
        var t = nav.tris[i];
        var ax = t.vxs[0], az = t.vzs[0];
        var bx = t.vxs[1], bz = t.vzs[1];
        var cx = t.vxs[2], cz = t.vzs[2];

        var v0x = cx - ax, v0z = cz - az;
        var v1x = bx - ax, v1z = bz - az;
        var v2x = px - ax, v2z = pz - az;

        var d00 = v0x*v0x + v0z*v0z;
        var d01 = v0x*v1x + v0z*v1z;
        var d02 = v0x*v2x + v0z*v2z;
        var d11 = v1x*v1x + v1z*v1z;
        var d12 = v1x*v2x + v1z*v2z;

        var inv = d00*d11 - d01*d01;
        if (Math.abs(inv) < 0.0001) continue;
        inv = 1.0 / inv;

        var u = (d11*d02 - d01*d12) * inv;
        var v = (d00*d12 - d01*d02) * inv;
        if (u >= -0.05 && v >= -0.05 && (u + v) <= 1.05) return i;
    }
    // Fallback: nearest centroid
    var best = 999999, bi = -1;
    for (var i = 0; i < nav.tris.length; i++) {
        var t = nav.tris[i];
        var d = (px-t.cx)*(px-t.cx) + (pz-t.cz)*(pz-t.cz);
        if (d < best) { best = d; bi = i; }
    }
    return bi;
}

// ============================================================
//  A* pathfinding
// ============================================================

function astar(nav, startTri, endTri) {
    if (startTri === endTri) return [startTri];
    if (startTri < 0 || endTri < 0) return null;

    var tris = nav.tris;
    var open = {}, closed = {}, gS = {}, fS = {}, from = {};

    var ex = tris[endTri].cx, ez = tris[endTri].cz;
    gS[startTri] = 0;
    fS[startTri] = Math.sqrt((tris[startTri].cx-ex)*(tris[startTri].cx-ex) + (tris[startTri].cz-ez)*(tris[startTri].cz-ez));
    open[startTri] = true;

    for (var iter = 0; iter < 10000; iter++) {
        var cur = -1, bestF = 1e18;
        for (var k in open) { var ki = parseInt(k); if (fS[ki] < bestF) { bestF = fS[ki]; cur = ki; } }
        if (cur === -1) return null;
        if (cur === endTri) {
            var path = [cur];
            while (from[cur] !== undefined) { cur = from[cur]; path.unshift(cur); }
            return path;
        }

        delete open[cur]; closed[cur] = true;
        var tri = tris[cur];
        for (var ni = 0; ni < 3; ni++) {
            var nb = tri.neighbours[ni];
            if (nb < 0 || nb >= nav.triCount || closed[nb]) continue;
            if (!isWalkable(tris[nb].coll)) continue;

            var tg = gS[cur] + Math.sqrt((tri.cx-tris[nb].cx)*(tri.cx-tris[nb].cx) + (tri.cz-tris[nb].cz)*(tri.cz-tris[nb].cz));
            if (gS[nb] === undefined || tg < gS[nb]) {
                from[nb] = cur;
                gS[nb] = tg;
                fS[nb] = tg + Math.sqrt((tris[nb].cx-ex)*(tris[nb].cx-ex) + (tris[nb].cz-ez)*(tris[nb].cz-ez));
                open[nb] = true;
            }
        }
    }
    return null;
}

// ============================================================
//  Portal-based path smoothing (Simple Stupid Funnel)
// ============================================================

// Find the shared edge between two adjacent triangles → return two vertex positions
function sharedEdge(nav, triA, triB) {
    var a = nav.tris[triA], b = nav.tris[triB];
    var shared = [];
    for (var i = 0; i < 3; i++) {
        for (var j = 0; j < 3; j++) {
            if (a.verts[i] === b.verts[j]) shared.push(i);
        }
    }
    if (shared.length !== 2) return null;
    return {
        lx: a.vxs[shared[0]], lz: a.vzs[shared[0]],
        rx: a.vxs[shared[1]], rz: a.vzs[shared[1]]
    };
}

function funnelSmooth(nav, path, startX, startZ, endX, endZ) {
    if (path.length <= 1) return [{ x: endX, z: endZ }];

    // Collect portal midpoints as waypoints, using actual start/end positions
    var waypoints = [{ x: startX, z: startZ }];

    for (var i = 0; i < path.length - 1; i++) {
        var edge = sharedEdge(nav, path[i], path[i + 1]);
        if (edge) {
            waypoints.push({ x: (edge.lx + edge.rx) / 2, z: (edge.lz + edge.rz) / 2 });
        }
    }
    waypoints.push({ x: endX, z: endZ });

    // Greedy line-of-sight simplification:
    // From waypoint[i], find the farthest waypoint[j] where the overall cardinal
    // direction from i to j matches i to i+1 (i.e., still going roughly the same way)
    var simplified = [waypoints[0]];
    var anchor = 0;

    while (anchor < waypoints.length - 1) {
        var anchorDir = cardinalDir(
            waypoints[anchor + 1].x - waypoints[anchor].x,
            waypoints[anchor + 1].z - waypoints[anchor].z
        );

        // Extend as far as direction holds
        var farthest = anchor + 1;
        for (var j = anchor + 2; j < waypoints.length; j++) {
            var dir = cardinalDir(
                waypoints[j].x - waypoints[anchor].x,
                waypoints[j].z - waypoints[anchor].z
            );
            if (dir === anchorDir) {
                farthest = j;
            } else {
                break;
            }
        }

        simplified.push(waypoints[farthest]);
        anchor = farthest;
    }

    return simplified;
}

function waypointsToDirections(waypoints) {
    var segments = [];
    for (var i = 1; i < waypoints.length; i++) {
        var dx = waypoints[i].x - waypoints[i-1].x;
        var dz = waypoints[i].z - waypoints[i-1].z;
        var dist = Math.sqrt(dx*dx + dz*dz);
        var dir = cardinalDir(dx, dz);

        if (segments.length > 0 && segments[segments.length-1].dir === dir) {
            segments[segments.length-1].dist += dist;
        } else if (dist > 0.5) {
            segments.push({ dir: dir, dist: dist });
        }
    }
    return segments;
}

// ============================================================
//  Map exits (from worker zone slots at offset 0xB18)
// ============================================================

// Trigger system (from decompiled FUN_0087bd10 debug display):
//   AtelGetActorType (RVA 0x46A740) returns actor type:
//     type 2/5 = LINE trigger (two endpoints)
//     type 3/6 = BOX trigger (position + rotation) — these are exits/doors
//   FUN_0086c240 (RVA 0x46C240) returns pointer to trigger geometry:
//     [0],[1],[2] = X,Y,Z position
//     [0xD] = rotation (for box triggers)
//     [0xC],[0xD],[0xE] = second endpoint (for line triggers)

var RVA_ATEL_GET_ACTOR_TYPE = 0x46A740;  // AtelGetActorType - Fahrenheit confirmed
var RVA_GET_TRIGGER_GEOM    = 0x46C240;  // Returns float* to trigger geometry

function readMapExits() {
    var exits = [];

    try {
        // FUN_0086a7c0 gets worker by index from the current Atel controller
        var fnGetWorker = new NativeFunction(rva(0x46A7C0), 'pointer', ['int']);

        // Read actor count from current controller (DAT_01326ae8 + 0xC)
        var controllerPtr = rva(0xF26AE8).readPointer();
        if (controllerPtr.isNull() || controllerPtr.toUInt32() < 0x10000) {
            logFile('[exits] No Atel controller');
            return exits;
        }
        var actorCount = controllerPtr.add(0x0C).readU16();
        logFile('[exits] Controller at ' + controllerPtr + ', ' + actorCount + ' actors');

        for (var i = 0; i < actorCount; i++) {
            try {
                // Get worker pointer for actor index i
                var worker = fnGetWorker(i);
                if (worker.isNull() || worker.toUInt32() < 0x10000) continue;

                // Read type: *(byte*)(*worker)
                // *worker = first dword = pointer to script header or type data
                var firstDword = worker.readPointer();
                if (firstDword.isNull() || firstDword.toUInt32() < 0x10000) continue;
                var actorType = firstDword.readU8();

                // Types: 2/5 = line trigger, 3/6 = box trigger
                if (actorType !== 2 && actorType !== 3 && actorType !== 5 && actorType !== 6) continue;

                var typeName = (actorType === 3 || actorType === 6) ? 'Box' : 'Line';

                // Read geometry based on type
                var geomOffset = (actorType === 5 || actorType === 6) ? 0x284 : 0x558;
                var geomPtr = worker.add(geomOffset);

                var x = geomPtr.readFloat();
                var y = geomPtr.add(4).readFloat();
                var z = geomPtr.add(8).readFloat();

                if (isNaN(x) || isNaN(z) || !isFinite(x) || !isFinite(z)) continue;
                if (Math.abs(x) > 50000 || Math.abs(z) > 50000) continue;

                logFile('[exits] Worker ' + i + ' type=' + actorType + ' (' + typeName + ')' +
                    ' geom@+0x' + geomOffset.toString(16) +
                    ' pos=(' + x.toFixed(2) + ',' + y.toFixed(2) + ',' + z.toFixed(2) + ')');

                exits.push({
                    index: -(i + 1),
                    chrId: 0,
                    category: 'Exit',
                    name: typeName + ' ' + (exits.length + 1),
                    pos: { x: x, y: y, z: z },
                    dist: 0, steps: 0, dir: ''
                });
            } catch (e) {
                logFile('[exits] Worker ' + i + ' error: ' + e.message);
            }
        }

        logFile('[exits] Found ' + exits.length + ' trigger workers');
    } catch (e) {
        logFile('[exits] Error: ' + e.message);
    }

    return exits;
}

// ============================================================
//  Entity scanning
// ============================================================

var entityList = [];
var selectedIdx = 0;
var lastMsg = '';

function say(msg) { lastMsg = msg; logBoth(msg); }

function scanEntities() {
    var ab = getActorsBase();
    if (!ab) { say('[!] Actor array NULL'); return; }

    var pp = readPos(ab);
    entityList = [];
    var bad = 0;

    for (var i = 1; i < MAX_ACTORS; i++) {
        var addr = ab.add(i * ACTOR_SIZE);
        try {
            var id = addr.add(OFF_CHR_ID).readU16();
            var px = addr.add(OFF_CHR_POS).readFloat();
            if (id === 0xCDCD || px < -4e8 || isNaN(px)) { bad++; if (bad >= 10) break; continue; }
            bad = 0;

            var en = addr.add(OFF_CHR_EN).readU16();
            if (en === 0) continue;

            var cat = classifyEntity(id);
            if (cat === 'Player') continue;

            var name = readName(addr);
            var pos = readPos(addr);
            var dist = distXZ(pp, pos);
            if (MAX_DISTANCE_UNITS > 0 && dist > MAX_DISTANCE_UNITS) continue;

            var steps = Math.round(dist / UNITS_PER_STEP);
            var dir = cardinalDir(pos.x - pp.x, pos.z - pp.z);

            entityList.push({ index: i, chrId: id, category: cat, name: name, pos: pos, dist: dist, steps: steps, dir: dir });
        } catch (e) { bad++; if (bad >= 10) break; }
    }

    // Add exit zones from worker zone slots
    var exits = readMapExits();
    for (var ei = 0; ei < exits.length; ei++) {
        var ex = exits[ei];
        ex.dist = distXZ(pp, ex.pos);
        ex.steps = Math.round(ex.dist / UNITS_PER_STEP);
        ex.dir = cardinalDir(ex.pos.x - pp.x, ex.pos.z - pp.z);
        entityList.push(ex);
    }

    entityList.sort(function(a, b) { return a.dist - b.dist; });
    selectedIdx = 0;
    cachedNav = null; // refresh navmesh on new scan

    if (entityList.length === 0) { say('[SCAN] No entities nearby'); return; }

    say('[SCAN] ' + entityList.length + ' entities:');
    for (var i = 0; i < entityList.length; i++) {
        var e = entityList[i];
        say('[' + (i+1) + '] ' + entityLabel(e) + ' — ' + e.steps + ' steps ' + e.dir);
    }
}

function announceSelected() {
    if (entityList.length === 0) { say('[!] No entities. Press s to scan.'); return; }
    var e = entityList[selectedIdx];
    say('[' + (selectedIdx+1) + '/' + entityList.length + '] ' + entityLabel(e) + ' — ' + e.steps + ' steps ' + e.dir);
}

function crowFlies() {
    if (entityList.length === 0) { say('[!] No entities.'); return; }
    var pp = getPlayerPos();
    if (!pp) { say('[!] Cannot read player'); return; }
    var e = entityList[selectedIdx];
    e.dist = distXZ(pp, e.pos);
    e.steps = Math.round(e.dist / UNITS_PER_STEP);
    e.dir = cardinalDir(e.pos.x - pp.x, e.pos.z - pp.z);
    say(entityLabel(e) + ' — ' + e.steps + ' steps ' + e.dir);
}

// ============================================================
//  Pathfinding command
// ============================================================

function pathfindToSelected() {
    if (entityList.length === 0) { say('[!] No entities. Press s to scan.'); return; }

    var pp = getPlayerPos();
    if (!pp) { say('[!] Cannot read player'); return; }

    var nav = getNav();
    if (!nav) { say('[!] Navmesh not loaded'); return; }

    var target = entityList[selectedIdx];
    var sTri = findTriangle(nav, pp.x, pp.z);
    var eTri = findTriangle(nav, target.pos.x, target.pos.z);

    logFile('[PATH] start tri=' + sTri + ' end tri=' + eTri);

    if (sTri < 0 || eTri < 0) { say('[!] Cannot locate triangles'); return; }

    if (sTri === eTri) {
        var dx = target.pos.x - pp.x, dz = target.pos.z - pp.z;
        var steps = Math.round(Math.sqrt(dx*dx+dz*dz) / UNITS_PER_STEP);
        say('[PATH] ' + entityLabel(target) + ': ' + steps + ' steps ' + cardinalDir(dx, dz));
        return;
    }

    var path = astar(nav, sTri, eTri);
    if (!path) { say('[!] No path found — target may be unreachable from here'); return; }

    logFile('[PATH] A* route: ' + path.length + ' tris');

    // Smooth via portals
    var waypoints = funnelSmooth(nav, path, pp.x, pp.z, target.pos.x, target.pos.z);
    var segments = waypointsToDirections(waypoints);

    if (segments.length === 0) { say('[PATH] Already at target'); return; }

    var totalDist = 0;
    for (var i = 0; i < segments.length; i++) totalDist += segments[i].dist;
    var totalSteps = Math.round(totalDist / UNITS_PER_STEP);

    var parts = [];
    for (var i = 0; i < segments.length; i++) {
        var segSteps = Math.round(segments[i].dist / UNITS_PER_STEP);
        if (segSteps > 0) parts.push(segSteps + ' ' + segments[i].dir);
    }

    say('[PATH] ' + entityLabel(target) + ' (' + totalSteps + ' steps): ' + parts.join(', '));
}

// ============================================================
//  Debug + range
// ============================================================

function debugInfo() {
    var pp = getPlayerPos();
    if (!pp) { say('[!] Cannot read player'); return; }
    var nav = getNav();
    if (!nav) { say('[DEBUG] Player pos=(' + pp.x.toFixed(2) + ',' + pp.y.toFixed(2) + ',' + pp.z.toFixed(2) + ') — no navmesh'); return; }

    var tri = findTriangle(nav, pp.x, pp.z);
    say('[DEBUG] Player pos=(' + pp.x.toFixed(2) + ',' + pp.y.toFixed(2) + ',' + pp.z.toFixed(2) + ') tri=' + tri + '/' + nav.triCount);

    if (tri >= 0) {
        var t = nav.tris[tri];
        say('[DEBUG] Tri ' + tri + ': coll=' + t.coll + ' neighbours=[' + t.neighbours.join(',') + ']');
    }

    if (entityList.length > 0) {
        var e = entityList[selectedIdx];
        var et = findTriangle(nav, e.pos.x, e.pos.z);
        say('[DEBUG] Target ' + entityLabel(e) + ' pos=(' + e.pos.x.toFixed(2) + ',' + e.pos.z.toFixed(2) + ') tri=' + et);
    }

    // Count walkable vs blocked, and scan ALL zone tag fields
    var walk = 0, block = 0;
    var zoneTags = {}; // field_name -> { value -> [tri indices] }
    var fields = ['battleZone', 'unk1', 'location', 'unk2', 'soundType'];

    for (var i = 0; i < nav.tris.length; i++) {
        var t = nav.tris[i];
        if (isWalkable(t.coll)) walk++; else block++;

        for (var fi = 0; fi < fields.length; fi++) {
            var fname = fields[fi];
            var val = t[fname];
            if (val !== 0) {
                if (!zoneTags[fname]) zoneTags[fname] = {};
                if (!zoneTags[fname][val]) zoneTags[fname][val] = [];
                zoneTags[fname][val].push(i);
            }
        }
    }
    say('[DEBUG] Navmesh: ' + walk + ' walkable, ' + block + ' blocked');

    // Report non-zero zone tags — these could be exit zones
    var tagKeys = Object.keys(zoneTags);
    if (tagKeys.length === 0) {
        say('[DEBUG] All triangle zone fields are 0 — no zone tags on this map');
    } else {
        for (var ki = 0; ki < tagKeys.length; ki++) {
            var fname = tagKeys[ki];
            var vals = zoneTags[fname];
            var valKeys = Object.keys(vals);
            for (var vi = 0; vi < valKeys.length; vi++) {
                var val = valKeys[vi];
                var triList = vals[val];
                // Compute centroid of all tagged triangles
                var avgX = 0, avgZ = 0;
                for (var ti = 0; ti < triList.length; ti++) {
                    avgX += nav.tris[triList[ti]].cx;
                    avgZ += nav.tris[triList[ti]].cz;
                }
                avgX /= triList.length;
                avgZ /= triList.length;
                var dist = pp ? Math.sqrt((avgX-pp.x)*(avgX-pp.x) + (avgZ-pp.z)*(avgZ-pp.z)) : 0;
                var steps = Math.round(dist / UNITS_PER_STEP);
                var dir = pp ? cardinalDir(avgX - pp.x, avgZ - pp.z) : '?';
                say('[ZONE] ' + fname + '=' + val + ': ' + triList.length + ' triangles, center=(' +
                    avgX.toFixed(1) + ',' + avgZ.toFixed(1) + ') — ' + steps + ' steps ' + dir);
            }
        }
    }
}

function adjustRange(delta) {
    MAX_DISTANCE_UNITS = Math.max(50, MAX_DISTANCE_UNITS + delta);
    say('[RANGE] ' + MAX_DISTANCE_UNITS + ' units (~' + Math.round(MAX_DISTANCE_UNITS / UNITS_PER_STEP) + ' steps). Press s to rescan.');
}

// ============================================================
//  RPC exports
// ============================================================

rpc.exports = {
    scan: function() { scanEntities(); return 'ok'; },
    next: function() {
        if (entityList.length === 0) return 'ok';
        selectedIdx = (selectedIdx + 1) % entityList.length;
        announceSelected(); return 'ok';
    },
    prev: function() {
        if (entityList.length === 0) return 'ok';
        selectedIdx = (selectedIdx - 1 + entityList.length) % entityList.length;
        announceSelected(); return 'ok';
    },
    crow: function() { crowFlies(); return 'ok'; },
    go: function() { pathfindToSelected(); return 'ok'; },
    repeat: function() { if (lastMsg) console.log(lastMsg); return 'ok'; },
    debug: function() { debugInfo(); return 'ok'; },
    rangeUp: function() { adjustRange(200); return 'ok'; },
    rangeDown: function() { adjustRange(-200); return 'ok'; }
};
