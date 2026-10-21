// @python
// @desc Entity scan + crow-flies — find nearby entities, get distance + cardinal direction
//
// Keyboard (via Python host):
//   s = Scan all entities (list with distance + direction)
//   n = Next entity in list
//   p = Previous entity in list
//   c = Crow-flies update (recalculate distance/direction to selected entity)
//   f = Flip axis mapping (cycles through axis interpretations)
//   d = Debug — print raw player position + selected entity position
//   q = Quit

var logPath = 'D:/Games/Dev/Custom/FFX/ffx-x2-screen-reader/Docs/Scripts/Frida/output/FFX/nav_entity_scan.log';
var logHandle = new File(logPath, 'w');
function logFile(msg) { logHandle.write(msg + '\n'); logHandle.flush(); }
function logBoth(msg) { console.log(msg); logFile(msg); }

var base = Process.enumerateModules()[0].base;
function rva(offset) { return base.add(offset); }

logBoth('[*] nav_entity_scan — Entity scanner + crow-flies navigation');

// Constants
var ACTOR_SIZE = 0x880;
var MAX_ACTORS = 128;
var UNITS_PER_STEP = 11.13; // rough estimate — needs recalibration on flat ground
var MAX_DISTANCE_UNITS = 500; // Hide entities beyond this many game units

// Actor offsets
var OFF_CHR_ID   = 0x00;
var OFF_CHR_EN   = 0x02;
var OFF_CHR_NAME = 0x04;
var OFF_CHR_POS  = 0x0C;  // X, Y, Z, W floats

// Axis mapping modes — we'll test empirically which is correct
// Each mode defines: which two axes are horizontal, which is vertical
// Mode 0: Y=vertical, XZ=horizontal (DirectX convention) — N=+Z, E=+X
// Mode 1: Z=vertical, XY=horizontal (OpenGL/GIS convention) — N=+Y, E=+X
// Mode 2: Y=vertical, XZ=horizontal, flipped — N=-Z, E=-X
// Mode 3: Z=vertical, XY=horizontal, flipped — N=-Y, E=-X
var axisMode = 0;
var AXIS_MODE_NAMES = [
    'Y=vert, N=+Z, E=+X',
    'Z=vert, N=+Y, E=+X',
    'Y=vert, N=-Z, E=-X',
    'Z=vert, N=-Y, E=-X'
];

// Entity classification
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

// Get horizontal deltas based on current axis mode
function getHorizontalDeltas(player, target) {
    var dh1, dh2; // horizontal axis 1 (east component), horizontal axis 2 (north component)
    switch (axisMode) {
        case 0: // Y=vert, N=+Z, E=+X
            dh1 = target.x - player.x; // east
            dh2 = target.z - player.z; // north
            break;
        case 1: // Z=vert, N=+Y, E=+X
            dh1 = target.x - player.x; // east
            dh2 = target.y - player.y; // north
            break;
        case 2: // Y=vert, N=-Z, E=-X
            dh1 = -(target.x - player.x); // east
            dh2 = -(target.z - player.z); // north
            break;
        case 3: // Z=vert, N=-Y, E=-X
            dh1 = -(target.x - player.x); // east
            dh2 = -(target.y - player.y); // north
            break;
    }
    return { east: dh1, north: dh2 };
}

function horizontalDistance(player, target) {
    var d = getHorizontalDeltas(player, target);
    return Math.sqrt(d.east * d.east + d.north * d.north);
}

function cardinalDirection(player, target) {
    var d = getHorizontalDeltas(player, target);
    if (Math.abs(d.east) < 0.01 && Math.abs(d.north) < 0.01) return 'here';

    // atan2(north, east) gives angle where 0=east, 90=north
    var angle = Math.atan2(d.north, d.east) * 180 / Math.PI;

    // Quantize to 8 directions
    // East=0, NE=45, N=90, NW=135, W=180/-180, SW=-135, S=-90, SE=-45
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

// Entity list state
var entityList = [];
var selectedIdx = 0;

function scanEntities() {
    var actorsBase = rva(0x1FC44E4).readPointer();
    if (actorsBase.isNull()) {
        logBoth('[!] Actor array NULL');
        return;
    }

    var playerPos = readPos(actorsBase); // actor[0] = player
    entityList = [];
    var consecutiveInvalid = 0;

    for (var i = 1; i < MAX_ACTORS; i++) { // skip actor[0] (player)
        var addr = actorsBase.add(i * ACTOR_SIZE);
        try {
            var chrId = addr.add(OFF_CHR_ID).readU16();
            var posX = addr.add(OFF_CHR_POS).readFloat();

            // Filter garbage: 0xCDCD or impossible position
            if (chrId === 0xCDCD || posX < -400000000 || isNaN(posX)) {
                consecutiveInvalid++;
                if (consecutiveInvalid >= 10) break;
                continue;
            }
            consecutiveInvalid = 0;

            var chrEnabled = addr.add(OFF_CHR_EN).readU16();
            if (chrEnabled === 0) continue;

            var category = classifyEntity(chrId);
            // Skip other player characters in party
            if (category === 'Player' && i > 0) continue;

            var name = readName(addr);
            var pos = readPos(addr);
            var dist = horizontalDistance(playerPos, pos);
            var steps = Math.round(dist / UNITS_PER_STEP);
            var dir = cardinalDirection(playerPos, pos);

            // Filter by distance — entities beyond MAX_DISTANCE_UNITS are on other screens
            if (dist > MAX_DISTANCE_UNITS) {
                logFile('[skip] ' + category + ' "' + name + '" at ' + dist.toFixed(0) + ' units — too far');
                continue;
            }

            entityList.push({
                index: i,
                chrId: chrId,
                category: category,
                name: name,
                pos: pos,
                dist: dist,
                steps: steps,
                dir: dir
            });
        } catch (e) {
            consecutiveInvalid++;
            if (consecutiveInvalid >= 10) break;
        }
    }

    // Sort by distance
    entityList.sort(function(a, b) { return a.dist - b.dist; });
    selectedIdx = 0;

    if (entityList.length === 0) {
        logBoth('[SCAN] No entities found on this map');
        return;
    }

    logBoth('[SCAN] ' + entityList.length + ' entities found (mode: ' + AXIS_MODE_NAMES[axisMode] + ')');
    for (var i = 0; i < entityList.length; i++) {
        var e = entityList[i];
        var label = e.category;
        if (e.name !== '?' && e.name !== e.category) label += ' "' + e.name + '"';
        logBoth('[' + (i + 1) + '] ' + label + ' — ' + e.steps + ' steps ' + e.dir);
    }

    // Announce first
    announceSelected();
}

function announceSelected() {
    if (entityList.length === 0) {
        logBoth('[!] No entities. Press s to scan.');
        return;
    }
    var e = entityList[selectedIdx];
    var label = e.category;
    if (e.name !== '?' && e.name !== e.category) label += ' "' + e.name + '"';
    logBoth('[' + (selectedIdx + 1) + '/' + entityList.length + '] ' + label + ' — ' + e.steps + ' steps ' + e.dir);
}

function nextEntity() {
    if (entityList.length === 0) return;
    selectedIdx = (selectedIdx + 1) % entityList.length;
    announceSelected();
}

function prevEntity() {
    if (entityList.length === 0) return;
    selectedIdx = (selectedIdx - 1 + entityList.length) % entityList.length;
    announceSelected();
}

function crowFliesUpdate() {
    if (entityList.length === 0) {
        logBoth('[!] No entities. Press s to scan.');
        return;
    }

    var playerPos = getPlayerPos();
    if (!playerPos) {
        logBoth('[!] Cannot read player position');
        return;
    }

    var e = entityList[selectedIdx];
    e.dist = horizontalDistance(playerPos, e.pos);
    e.steps = Math.round(e.dist / UNITS_PER_STEP);
    e.dir = cardinalDirection(playerPos, e.pos);

    var label = e.category;
    if (e.name !== '?' && e.name !== e.category) label += ' "' + e.name + '"';
    logBoth(label + ' — ' + e.steps + ' steps ' + e.dir);
}

function flipAxes() {
    axisMode = (axisMode + 1) % 4;
    logBoth('[AXES] Switched to mode ' + axisMode + ': ' + AXIS_MODE_NAMES[axisMode]);

    // Recalculate all entities with new axis mapping
    if (entityList.length > 0) {
        var playerPos = getPlayerPos();
        if (playerPos) {
            for (var i = 0; i < entityList.length; i++) {
                var e = entityList[i];
                e.dist = horizontalDistance(playerPos, e.pos);
                e.steps = Math.round(e.dist / UNITS_PER_STEP);
                e.dir = cardinalDirection(playerPos, e.pos);
            }
            entityList.sort(function(a, b) { return a.dist - b.dist; });
            announceSelected();
        }
    }
}

function adjustRange(delta) {
    MAX_DISTANCE_UNITS = Math.max(50, MAX_DISTANCE_UNITS + delta);
    logBoth('[RANGE] Max distance: ' + MAX_DISTANCE_UNITS + ' units (~' + Math.round(MAX_DISTANCE_UNITS / UNITS_PER_STEP) + ' steps). Press s to rescan.');
}

function debugPositions() {
    var playerPos = getPlayerPos();
    if (!playerPos) {
        logBoth('[!] Cannot read player');
        return;
    }
    logBoth('[DEBUG] Player: X=' + playerPos.x.toFixed(2) + ' Y=' + playerPos.y.toFixed(2) + ' Z=' + playerPos.z.toFixed(2));

    if (entityList.length > 0) {
        var e = entityList[selectedIdx];
        logBoth('[DEBUG] Target: X=' + e.pos.x.toFixed(2) + ' Y=' + e.pos.y.toFixed(2) + ' Z=' + e.pos.z.toFixed(2));
        logBoth('[DEBUG] Delta: dX=' + (e.pos.x - playerPos.x).toFixed(2) +
            ' dY=' + (e.pos.y - playerPos.y).toFixed(2) +
            ' dZ=' + (e.pos.z - playerPos.z).toFixed(2));
        logBoth('[DEBUG] Mode: ' + AXIS_MODE_NAMES[axisMode] +
            ' | dist=' + e.dist.toFixed(2) + ' units = ' + e.steps + ' steps ' + e.dir);
    }
}

rpc.exports = {
    scan: function() { scanEntities(); return 'ok'; },
    next: function() { nextEntity(); return 'ok'; },
    prev: function() { prevEntity(); return 'ok'; },
    crow: function() { crowFliesUpdate(); return 'ok'; },
    flip: function() { flipAxes(); return 'ok'; },
    rangeUp: function() { adjustRange(200); return 'ok'; },
    rangeDown: function() { adjustRange(-200); return 'ok'; },
    debug: function() { debugPositions(); return 'ok'; }
};
