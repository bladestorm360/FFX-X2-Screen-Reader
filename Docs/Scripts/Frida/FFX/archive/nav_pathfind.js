// @python
// @desc Pathfinding — A* on navmesh with turn-by-turn cardinal directions
//
// Keyboard (via Python host):
//   s = Scan entities (same as entity scan)
//   n/p = Next/previous entity
//   g = Go — pathfind to selected entity (A* turn-by-turn)
//   c = Crow-flies to selected entity (straight line)
//   d = Debug — show player triangle + target triangle
//   q = Quit

var logPath = 'D:/Games/Dev/Custom/FFX/ffx-x2-screen-reader/Docs/Scripts/Frida/output/FFX/nav_pathfind.log';
var logHandle = new File(logPath, 'w');
function logFile(msg) { logHandle.write(msg + '\n'); logHandle.flush(); }
function logBoth(msg) { console.log(msg); logFile(msg); }

var base = Process.enumerateModules()[0].base;
function rva(offset) { return base.add(offset); }

logBoth('[*] nav_pathfind — A* navmesh pathfinding');

// Constants
var ACTOR_SIZE = 0x880;
var MAX_ACTORS = 128;
var UNITS_PER_STEP = 11.13;
var VPATRI_SIZE = 0x10;
var VPAVERTEX_SIZE = 0x08;
var MAX_DISTANCE_UNITS = 500;

// RVAs
var RVA_TRIS     = 0xF01A44;
var RVA_TRICOUNT = 0xF01A48;
var RVA_VERTICES = 0xF01A4C;
var RVA_SCALE    = 0xF01A50;

// Actor offsets
var OFF_CHR_ID   = 0x00;
var OFF_CHR_EN   = 0x02;
var OFF_CHR_NAME = 0x04;
var OFF_CHR_POS  = 0x0C;

// ---- Shared utilities ----

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

function readName(actorBase) {
    try {
        var namePtr = actorBase.add(OFF_CHR_NAME).readPointer();
        if (namePtr.isNull()) return '?';
        var name = namePtr.readCString();
        if (name && name.length > 0 && name.length < 200) return name;
        return '?';
    } catch (e) { return '?'; }
}

function readPos(actorBase) {
    return {
        x: actorBase.add(OFF_CHR_POS).readFloat(),
        y: actorBase.add(OFF_CHR_POS + 4).readFloat(),
        z: actorBase.add(OFF_CHR_POS + 8).readFloat()
    };
}

function getPlayerPos() {
    var actorsBase = rva(0x1FC44E4).readPointer();
    if (actorsBase.isNull()) return null;
    return readPos(actorsBase);
}

function distXZ(a, b) {
    var dx = a.x - b.x;
    var dz = a.z - b.z;
    return Math.sqrt(dx * dx + dz * dz);
}

function cardinalDir(dx, dz) {
    if (Math.abs(dx) < 0.01 && Math.abs(dz) < 0.01) return 'here';
    var angle = Math.atan2(dz, dx) * 180 / Math.PI;
    if (angle >= -22.5 && angle < 22.5) return 'east';
    if (angle >= 22.5 && angle < 67.5) return 'northeast';
    if (angle >= 67.5 && angle < 112.5) return 'north';
    if (angle >= 112.5 && angle < 157.5) return 'northwest';
    if (angle >= 157.5 || angle < -157.5) return 'west';
    if (angle >= -157.5 && angle < -112.5) return 'southwest';
    if (angle >= -112.5 && angle < -67.5) return 'south';
    if (angle >= -67.5 && angle < -22.5) return 'southeast';
    return '?';
}

// ---- Navmesh reading ----

function readNavmesh() {
    var triCount = rva(RVA_TRICOUNT).readS32();
    var trisRaw = rva(RVA_TRIS).readU32();
    var vertsRaw = rva(RVA_VERTICES).readU32();
    var scale = rva(RVA_SCALE).readFloat();

    if (triCount <= 0 || trisRaw < 0x10000 || vertsRaw < 0x10000 || scale <= 0) return null;

    var trisBase = ptr(trisRaw);
    var vertsBase = ptr(vertsRaw);

    // Read all triangles into JS arrays for fast A*
    var tris = [];
    for (var i = 0; i < triCount; i++) {
        var t = trisBase.add(i * VPATRI_SIZE);
        var v0 = t.readS16(), v1 = t.add(2).readS16(), v2 = t.add(4).readS16();
        var n0 = t.add(6).readS16(), n1 = t.add(8).readS16(), n2 = t.add(10).readS16();
        var data = t.add(12).readS32();
        var collGroup = (data >>> 0) & 0x7F;

        // Compute centroid in world coords (divide by scale)
        var cx = 0, cz = 0;
        var indices = [v0, v1, v2];
        var vxs = [], vzs = [];
        for (var vi = 0; vi < 3; vi++) {
            var vAddr = vertsBase.add(indices[vi] * VPAVERTEX_SIZE);
            var wx = vAddr.readS16() / scale;
            var wz = vAddr.add(4).readS16() / scale;
            cx += wx; cz += wz;
            vxs.push(wx); vzs.push(wz);
        }
        cx /= 3; cz /= 3;

        tris.push({
            idx: i,
            verts: indices,
            neighbours: [n0, n1, n2],
            collGroup: collGroup,
            cx: cx, cz: cz,
            vxs: vxs, vzs: vzs
        });
    }

    return { tris: tris, triCount: triCount, scale: scale };
}

// ---- Point-in-triangle (XZ plane) ----

function findTriangle(nav, px, pz) {
    for (var i = 0; i < nav.tris.length; i++) {
        var t = nav.tris[i];
        var ax = t.vxs[0], az = t.vzs[0];
        var bx = t.vxs[1], bz = t.vzs[1];
        var cx = t.vxs[2], cz = t.vzs[2];

        // Barycentric test
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

        if (u >= -0.05 && v >= -0.05 && (u + v) <= 1.05) {
            return i;
        }
    }
    // Fallback: closest centroid
    var bestDist = 999999;
    var bestIdx = -1;
    for (var i = 0; i < nav.tris.length; i++) {
        var t = nav.tris[i];
        var d = Math.sqrt((px - t.cx) * (px - t.cx) + (pz - t.cz) * (pz - t.cz));
        if (d < bestDist) { bestDist = d; bestIdx = i; }
    }
    return bestIdx;
}

// ---- A* pathfinding ----

function astar(nav, startTri, endTri) {
    if (startTri === endTri) return [startTri];
    if (startTri < 0 || endTri < 0) return null;

    var tris = nav.tris;
    var openSet = {};
    var closedSet = {};
    var gScore = {};
    var fScore = {};
    var cameFrom = {};

    gScore[startTri] = 0;
    var endCx = tris[endTri].cx, endCz = tris[endTri].cz;
    fScore[startTri] = Math.sqrt(
        (tris[startTri].cx - endCx) * (tris[startTri].cx - endCx) +
        (tris[startTri].cz - endCz) * (tris[startTri].cz - endCz)
    );
    openSet[startTri] = true;

    for (var iter = 0; iter < 5000; iter++) {
        // Find lowest fScore in openSet
        var current = -1;
        var bestF = 999999999;
        for (var key in openSet) {
            var k = parseInt(key);
            if (fScore[k] < bestF) { bestF = fScore[k]; current = k; }
        }

        if (current === -1) return null; // No path
        if (current === endTri) {
            // Reconstruct path
            var path = [current];
            while (cameFrom[current] !== undefined) {
                current = cameFrom[current];
                path.unshift(current);
            }
            return path;
        }

        delete openSet[current];
        closedSet[current] = true;

        // Check neighbours
        var tri = tris[current];
        for (var ni = 0; ni < 3; ni++) {
            var neighbour = tri.neighbours[ni];
            if (neighbour < 0 || neighbour >= nav.triCount) continue; // edge of mesh
            if (closedSet[neighbour]) continue;

            var nTri = tris[neighbour];
            // Skip blocked triangles
            if (nTri.collGroup === 1 || nTri.collGroup === 14) continue; // BlockAll or BlockPlayer

            var tentativeG = gScore[current] + Math.sqrt(
                (tri.cx - nTri.cx) * (tri.cx - nTri.cx) +
                (tri.cz - nTri.cz) * (tri.cz - nTri.cz)
            );

            if (gScore[neighbour] === undefined || tentativeG < gScore[neighbour]) {
                cameFrom[neighbour] = current;
                gScore[neighbour] = tentativeG;
                fScore[neighbour] = tentativeG + Math.sqrt(
                    (nTri.cx - endCx) * (nTri.cx - endCx) +
                    (nTri.cz - endCz) * (nTri.cz - endCz)
                );
                openSet[neighbour] = true;
            }
        }
    }

    return null; // Exceeded iteration limit
}

// ---- Convert path to smoothed cardinal directions ----

function pathToDirections(nav, path) {
    if (!path || path.length < 2) return [];

    // Build waypoints from triangle centroids
    var waypoints = [];
    for (var i = 0; i < path.length; i++) {
        var t = nav.tris[path[i]];
        waypoints.push({ x: t.cx, z: t.cz });
    }

    // Smoothed approach: track cumulative direction from last anchor point.
    // Only emit a new segment when the overall direction from anchor changes.
    var segments = [];
    var anchorIdx = 0;

    for (var i = 1; i < waypoints.length; i++) {
        // Direction from anchor to current point
        var dx = waypoints[i].x - waypoints[anchorIdx].x;
        var dz = waypoints[i].z - waypoints[anchorIdx].z;
        var curDir = cardinalDir(dx, dz);

        // Look ahead: direction from anchor to NEXT point (if exists)
        var nextDir = curDir;
        if (i + 1 < waypoints.length) {
            var ndx = waypoints[i + 1].x - waypoints[anchorIdx].x;
            var ndz = waypoints[i + 1].z - waypoints[anchorIdx].z;
            nextDir = cardinalDir(ndx, ndz);
        }

        // If the overall direction is about to change, emit segment and set new anchor
        if (nextDir !== curDir || i === waypoints.length - 1) {
            var segDist = Math.sqrt(dx * dx + dz * dz);
            if (segDist > 0.5) {
                segments.push({ dir: curDir, dist: segDist });
            }
            anchorIdx = i;
        }
    }

    // Merge consecutive same-direction segments (can happen after smoothing)
    var merged = [];
    for (var i = 0; i < segments.length; i++) {
        if (merged.length > 0 && merged[merged.length - 1].dir === segments[i].dir) {
            merged[merged.length - 1].dist += segments[i].dist;
        } else {
            merged.push({ dir: segments[i].dir, dist: segments[i].dist });
        }
    }

    return merged;
}

// ---- Entity list (reused from entity scan) ----

var entityList = [];
var selectedIdx = 0;

function scanEntities() {
    var actorsBase = rva(0x1FC44E4).readPointer();
    if (actorsBase.isNull()) {
        logBoth('[!] Actor array NULL');
        return;
    }

    var playerPos = readPos(actorsBase);
    entityList = [];
    var consecutiveInvalid = 0;

    for (var i = 1; i < MAX_ACTORS; i++) {
        var addr = actorsBase.add(i * ACTOR_SIZE);
        try {
            var chrId = addr.add(OFF_CHR_ID).readU16();
            var posX = addr.add(OFF_CHR_POS).readFloat();

            if (chrId === 0xCDCD || posX < -400000000 || isNaN(posX)) {
                consecutiveInvalid++;
                if (consecutiveInvalid >= 10) break;
                continue;
            }
            consecutiveInvalid = 0;

            var chrEnabled = addr.add(OFF_CHR_EN).readU16();
            if (chrEnabled === 0) continue;

            var category = classifyEntity(chrId);
            if (category === 'Player' && i > 0) continue;

            var name = readName(addr);
            var pos = readPos(addr);
            var dist = distXZ(playerPos, pos);

            if (dist > MAX_DISTANCE_UNITS) continue;

            var steps = Math.round(dist / UNITS_PER_STEP);
            var dx = pos.x - playerPos.x;
            var dz = pos.z - playerPos.z;
            var dir = cardinalDir(dx, dz);

            entityList.push({
                index: i, chrId: chrId, category: category,
                name: name, pos: pos, dist: dist, steps: steps, dir: dir
            });
        } catch (e) {
            consecutiveInvalid++;
            if (consecutiveInvalid >= 10) break;
        }
    }

    entityList.sort(function(a, b) { return a.dist - b.dist; });
    selectedIdx = 0;

    if (entityList.length === 0) {
        logBoth('[SCAN] No entities found nearby');
        return;
    }

    logBoth('[SCAN] ' + entityList.length + ' entities:');
    for (var i = 0; i < entityList.length; i++) {
        var e = entityList[i];
        var label = e.category;
        if (e.name !== '?' && e.name !== e.category) label += ' "' + e.name + '"';
        logBoth('[' + (i + 1) + '] ' + label + ' — ' + e.steps + ' steps ' + e.dir);
    }
}

function announceSelected() {
    if (entityList.length === 0) { logBoth('[!] No entities. Press s to scan.'); return; }
    var e = entityList[selectedIdx];
    var label = e.category;
    if (e.name !== '?' && e.name !== e.category) label += ' "' + e.name + '"';
    logBoth('[' + (selectedIdx + 1) + '/' + entityList.length + '] ' + label + ' — ' + e.steps + ' steps ' + e.dir);
}

function crowFlies() {
    if (entityList.length === 0) { logBoth('[!] No entities.'); return; }
    var playerPos = getPlayerPos();
    if (!playerPos) { logBoth('[!] Cannot read player'); return; }
    var e = entityList[selectedIdx];
    e.dist = distXZ(playerPos, e.pos);
    e.steps = Math.round(e.dist / UNITS_PER_STEP);
    var dx = e.pos.x - playerPos.x;
    var dz = e.pos.z - playerPos.z;
    e.dir = cardinalDir(dx, dz);
    var label = e.category;
    if (e.name !== '?' && e.name !== e.category) label += ' "' + e.name + '"';
    logBoth(label + ' — ' + e.steps + ' steps ' + e.dir);
}

// ---- Pathfind command ----

function pathfindToSelected() {
    if (entityList.length === 0) { logBoth('[!] No entities. Press s to scan.'); return; }

    var playerPos = getPlayerPos();
    if (!playerPos) { logBoth('[!] Cannot read player'); return; }

    var nav = readNavmesh();
    if (!nav) { logBoth('[!] Navmesh not loaded'); return; }

    var target = entityList[selectedIdx];

    // Find triangles
    var startTri = findTriangle(nav, playerPos.x, playerPos.z);
    var endTri = findTriangle(nav, target.pos.x, target.pos.z);

    logFile('[PATH] Player tri=' + startTri + ' Target tri=' + endTri);
    logFile('[PATH] Player pos=(' + playerPos.x.toFixed(2) + ',' + playerPos.z.toFixed(2) +
        ') Target pos=(' + target.pos.x.toFixed(2) + ',' + target.pos.z.toFixed(2) + ')');

    if (startTri < 0 || endTri < 0) {
        logBoth('[!] Could not find triangles for player or target');
        return;
    }

    if (startTri === endTri) {
        // Same triangle — just give crow-flies
        var dx = target.pos.x - playerPos.x;
        var dz = target.pos.z - playerPos.z;
        var dist = Math.sqrt(dx * dx + dz * dz);
        var steps = Math.round(dist / UNITS_PER_STEP);
        logBoth('[PATH] ' + steps + ' steps ' + cardinalDir(dx, dz));
        return;
    }

    // A*
    var path = astar(nav, startTri, endTri);
    if (!path) {
        logBoth('[!] No path found — target may be unreachable');
        return;
    }

    logFile('[PATH] Route: ' + path.length + ' triangles: ' + path.join(','));

    // Convert to cardinal directions
    var segments = pathToDirections(nav, path);
    if (segments.length === 0) {
        logBoth('[PATH] Already at target');
        return;
    }

    // Calculate total distance
    var totalDist = 0;
    for (var i = 0; i < segments.length; i++) totalDist += segments[i].dist;
    var totalSteps = Math.round(totalDist / UNITS_PER_STEP);

    // Build output
    var label = target.category;
    if (target.name !== '?' && target.name !== target.category) label += ' "' + target.name + '"';

    var parts = [];
    for (var i = 0; i < segments.length; i++) {
        var s = segments[i];
        var segSteps = Math.round(s.dist / UNITS_PER_STEP);
        if (segSteps > 0) {
            parts.push(segSteps + ' ' + s.dir);
        }
    }

    logBoth('[PATH] ' + label + ' (' + totalSteps + ' steps): ' + parts.join(', '));
}

function debugTriangles() {
    var playerPos = getPlayerPos();
    if (!playerPos) { logBoth('[!] Cannot read player'); return; }

    var nav = readNavmesh();
    if (!nav) { logBoth('[!] Navmesh not loaded'); return; }

    var playerTri = findTriangle(nav, playerPos.x, playerPos.z);
    logBoth('[DEBUG] Player pos=(' + playerPos.x.toFixed(2) + ',' + playerPos.z.toFixed(2) + ') tri=' + playerTri);

    if (playerTri >= 0) {
        var t = nav.tris[playerTri];
        logBoth('[DEBUG] Triangle ' + playerTri + ': centroid=(' + t.cx.toFixed(2) + ',' + t.cz.toFixed(2) +
            ') neighbours=[' + t.neighbours.join(',') + ']');
    }

    if (entityList.length > 0) {
        var target = entityList[selectedIdx];
        var targetTri = findTriangle(nav, target.pos.x, target.pos.z);
        logBoth('[DEBUG] Target pos=(' + target.pos.x.toFixed(2) + ',' + target.pos.z.toFixed(2) + ') tri=' + targetTri);
    }

    logBoth('[DEBUG] Navmesh: ' + nav.triCount + ' triangles, scale=' + nav.tris[0] ? nav.scale : '?');
}

rpc.exports = {
    scan: function() { scanEntities(); return 'ok'; },
    next: function() { selectedIdx = (selectedIdx + 1) % Math.max(1, entityList.length); announceSelected(); return 'ok'; },
    prev: function() { selectedIdx = (selectedIdx - 1 + Math.max(1, entityList.length)) % Math.max(1, entityList.length); announceSelected(); return 'ok'; },
    crow: function() { crowFlies(); return 'ok'; },
    go: function() { pathfindToSelected(); return 'ok'; },
    debug: function() { debugTriangles(); return 'ok'; }
};
