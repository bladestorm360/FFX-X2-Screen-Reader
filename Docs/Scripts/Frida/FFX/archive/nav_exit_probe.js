// @python
// @desc Exit probe — hook transitions + dump zone data to find map exit triggers
//
// Hooks TkSetEventJump, graphicFieldMapLoad, and monitors room_id changes.
// When a transition fires, logs the player position = the exit location.
// Also dumps the Atel script chunk zone/area data for analysis.
//
// Keyboard (via Python host):
//   s = Scan current state (room_id, player pos, navmesh boundary edges)
//   z = Dump zone/area data from script chunk
//   b = Find navmesh boundary edges (potential exits — edges with neighbour=-1)
//   w = Start watching for transitions (hooks fire automatically)
//   q = Quit

var logPath = 'D:/Games/Dev/Custom/FFX/ffx-x2-screen-reader/Docs/Scripts/Frida/output/FFX/nav_exit_probe.log';
var logHandle = new File(logPath, 'w');
function logFile(msg) { logHandle.write(msg + '\n'); logHandle.flush(); }
function logBoth(msg) { console.log(msg); logFile(msg); }

var base = Process.enumerateModules()[0].base;
function rva(offset) { return base.add(offset); }

logBoth('[*] nav_exit_probe — Zone transition discovery');

var ACTOR_SIZE = 0x880;
var VPATRI_SIZE = 0x10;
var VPAVERTEX_SIZE = 0x08;

function getPlayerPos() {
    var ab = rva(0x1FC44E4).readPointer();
    if (ab.isNull()) return null;
    return {
        x: ab.add(0x0C).readFloat(),
        y: ab.add(0x10).readFloat(),
        z: ab.add(0x14).readFloat()
    };
}

function getRoomId() {
    return rva(0xD2CA90).readU16();
}

function getSpawnpoint() {
    return rva(0xD2CA90).add(0x0C).readU8();
}

// ---- Scan current state ----

function scanState() {
    var pos = getPlayerPos();
    var room = getRoomId();
    var spawn = getSpawnpoint();

    if (pos) {
        logBoth('[STATE] Room=' + room + ' Spawn=' + spawn +
            ' Pos=(' + pos.x.toFixed(2) + ',' + pos.y.toFixed(2) + ',' + pos.z.toFixed(2) + ')');
    } else {
        logBoth('[STATE] Room=' + room + ' Spawn=' + spawn + ' Pos=unavailable');
    }
}

// ---- Dump zone/area data from script chunk ----

function dumpZoneData() {
    try {
        // Try dereferenced worker pointer
        var workerPtr = rva(0xF270A4).readPointer();
        if (workerPtr.isNull() || workerPtr.toUInt32() < 0x10000) {
            logBoth('[!] No current worker');
            return;
        }

        var chunkPtr = workerPtr.add(0x04).readPointer();
        if (chunkPtr.isNull() || chunkPtr.toUInt32() < 0x10000) {
            logBoth('[!] No script chunk');
            return;
        }

        var mapStart = chunkPtr.add(0x04).readU32();
        var offsetAuthor = chunkPtr.add(0x08).readU32();
        var zoneBytesField = chunkPtr.add(0x1E).readU16();
        var offsetArea = chunkPtr.add(0x28).readU32();
        var offsetCode = chunkPtr.add(0x30).readU32();
        var scriptNum = chunkPtr.add(0x34).readU16();

        logBoth('[CHUNK] script_chunk at ' + chunkPtr);
        logBoth('[CHUNK] map_start=0x' + mapStart.toString(16) +
            ' offset_author=0x' + offsetAuthor.toString(16) +
            ' zone_bytes=' + zoneBytesField +
            ' offset_area=0x' + offsetArea.toString(16) +
            ' offset_code=0x' + offsetCode.toString(16) +
            ' script_num=' + scriptNum);

        // Dump MapEntrance data
        var entranceCount = Math.floor((offsetAuthor - mapStart) / 0x20);
        logBoth('[CHUNK] ' + entranceCount + ' map entrances:');
        for (var i = 0; i < entranceCount && i < 20; i++) {
            var addr = chunkPtr.add(mapStart + i * 0x20);
            // Dump all 0x20 bytes as hex + interpreted fields
            var raw = '';
            for (var b = 0; b < 0x20; b++) {
                var byte = addr.add(b).readU8();
                raw += (byte < 16 ? '0' : '') + byte.toString(16) + ' ';
            }
            var x = addr.add(0x0C).readFloat();
            var y = addr.add(0x10).readFloat();
            var z = addr.add(0x14).readFloat();
            var rot = addr.add(0x08).readFloat();
            var unk00 = addr.readS16();
            var unk02 = addr.add(0x02).readS16();
            var unk04 = addr.add(0x04).readS16();
            var unk06 = addr.add(0x06).readS16();

            logBoth('  [' + i + '] unk=(' + unk00 + ',' + unk02 + ',' + unk04 + ',' + unk06 +
                ') rot=' + rot.toFixed(2) +
                ' pos=(' + x.toFixed(2) + ',' + y.toFixed(2) + ',' + z.toFixed(2) + ')');
            logFile('  [' + i + '] raw: ' + raw);
        }

        // Dump area/zone data
        if (offsetArea > 0) {
            // Area data may come after code — just read up to 512 bytes
            var areaSize = 512;
            logBoth('[ZONE] Area data at chunk+0x' + offsetArea.toString(16) + ' (' + areaSize + ' bytes):');

            // Dump first 256 bytes of zone data as hex
            var zoneAddr = chunkPtr.add(offsetArea);
            var dumpSize = Math.min(areaSize, 256);
            var hexDump = '';
            for (var b = 0; b < dumpSize; b++) {
                if (b > 0 && b % 16 === 0) {
                    logFile('  +' + (b - 16).toString(16).padStart(4, '0') + ': ' + hexDump);
                    hexDump = '';
                }
                var byte = zoneAddr.add(b).readU8();
                hexDump += (byte < 16 ? '0' : '') + byte.toString(16) + ' ';
            }
            if (hexDump.length > 0) logFile('  +' + (dumpSize - (dumpSize % 16 || 16)).toString(16).padStart(4, '0') + ': ' + hexDump);

            // Try interpreting as array of coordinate pairs (x,z shorts or floats)
            logFile('[ZONE] Attempting float interpretation:');
            for (var off = 0; off + 12 <= dumpSize; off += 4) {
                var f = zoneAddr.add(off).readFloat();
                if (!isNaN(f) && Math.abs(f) > 0.01 && Math.abs(f) < 10000) {
                    logFile('  +0x' + off.toString(16) + ': float=' + f.toFixed(2));
                }
            }

            logBoth('[ZONE] Full dump in log file (' + dumpSize + ' bytes)');
        } else {
            logBoth('[ZONE] No area data (offset_area=0x' + offsetArea.toString(16) + ')');
        }
    } catch (e) {
        logBoth('[!] Error: ' + e.message);
    }
}

// ---- Find navmesh boundary edges ----

function findBoundaryEdges() {
    var triCount = rva(0xF01A48).readS32();
    var trisRaw = rva(0xF01A44).readU32();
    var vertsRaw = rva(0xF01A4C).readU32();
    var scale = rva(0xF01A50).readFloat();

    if (triCount <= 0 || trisRaw < 0x10000 || vertsRaw < 0x10000) {
        logBoth('[!] No navmesh loaded');
        return;
    }

    var tB = ptr(trisRaw), vB = ptr(vertsRaw);
    var boundaries = [];

    for (var i = 0; i < triCount; i++) {
        var t = tB.add(i * VPATRI_SIZE);
        var vis = [t.readS16(), t.add(2).readS16(), t.add(4).readS16()];
        var ns = [t.add(6).readS16(), t.add(8).readS16(), t.add(10).readS16()];

        for (var e = 0; e < 3; e++) {
            if (ns[e] === -1) {
                // This edge has no neighbour — it's a boundary
                var v1idx = vis[e];
                var v2idx = vis[(e + 1) % 3];

                var v1x = vB.add(v1idx * VPAVERTEX_SIZE).readS16() / scale;
                var v1z = vB.add(v1idx * VPAVERTEX_SIZE + 4).readS16() / scale;
                var v2x = vB.add(v2idx * VPAVERTEX_SIZE).readS16() / scale;
                var v2z = vB.add(v2idx * VPAVERTEX_SIZE + 4).readS16() / scale;

                var mx = (v1x + v2x) / 2;
                var mz = (v1z + v2z) / 2;
                var edgeLen = Math.sqrt((v2x-v1x)*(v2x-v1x) + (v2z-v1z)*(v2z-v1z));

                boundaries.push({ tri: i, edge: e, mx: mx, mz: mz, len: edgeLen });
            }
        }
    }

    // Cluster nearby boundary edges into exit candidates
    var playerPos = getPlayerPos();
    logBoth('[BOUNDARY] ' + boundaries.length + ' boundary edges found');

    // Sort by distance from player
    if (playerPos) {
        boundaries.sort(function(a, b) {
            var da = (a.mx-playerPos.x)*(a.mx-playerPos.x) + (a.mz-playerPos.z)*(a.mz-playerPos.z);
            var db = (b.mx-playerPos.x)*(b.mx-playerPos.x) + (b.mz-playerPos.z)*(b.mz-playerPos.z);
            return da - db;
        });
    }

    // Show closest 10
    var count = Math.min(boundaries.length, 10);
    for (var i = 0; i < count; i++) {
        var b = boundaries[i];
        var dist = playerPos ? Math.sqrt((b.mx-playerPos.x)*(b.mx-playerPos.x) + (b.mz-playerPos.z)*(b.mz-playerPos.z)) : 0;
        var steps = Math.round(dist / 11.13);
        var dir = playerPos ? cardinalDir(b.mx - playerPos.x, b.mz - playerPos.z) : '?';
        logBoth('  [' + (i+1) + '] Boundary edge tri=' + b.tri + ' midpoint=(' + b.mx.toFixed(1) + ',' + b.mz.toFixed(1) + ') len=' + b.len.toFixed(1) + ' — ' + steps + ' steps ' + dir);
    }
    logFile('[BOUNDARY] All ' + boundaries.length + ' edges logged');
    for (var i = count; i < boundaries.length; i++) {
        var b = boundaries[i];
        logFile('  [' + (i+1) + '] tri=' + b.tri + ' mid=(' + b.mx.toFixed(1) + ',' + b.mz.toFixed(1) + ') len=' + b.len.toFixed(1));
    }
}

function cardinalDir(dx, dz) {
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

// ---- Watch for transitions ----

var watching = false;
var lastRoomId = 0;

// Scan all actors near player and dump their types — called when transition fires
function dumpNearbyActors(label) {
    var ab = rva(0x1FC44E4).readPointer();
    if (ab.isNull()) { logBoth(label + ' Cannot read actors'); return; }

    var pp = {
        x: ab.add(0x0C).readFloat(),
        y: ab.add(0x10).readFloat(),
        z: ab.add(0x14).readFloat()
    };

    logBoth(label + ' Player pos=(' + pp.x.toFixed(2) + ',' + pp.y.toFixed(2) + ',' + pp.z.toFixed(2) + ')');
    logBoth(label + ' Scanning ALL actors at moment of transition:');

    var bad = 0;
    for (var i = 0; i < 128; i++) {
        var addr = ab.add(i * 0x880);
        try {
            var id = addr.add(0x00).readU16();
            var px = addr.add(0x0C).readFloat();
            if (id === 0xCDCD || px < -4e8 || isNaN(px)) { bad++; if (bad >= 10) break; continue; }
            bad = 0;

            var en = addr.add(0x02).readU16();
            var pos = {
                x: addr.add(0x0C).readFloat(),
                y: addr.add(0x10).readFloat(),
                z: addr.add(0x14).readFloat()
            };

            var name = '?';
            try {
                var np = addr.add(0x04).readPointer();
                if (!np.isNull()) { var s = np.readCString(); if (s && s.length > 0 && s.length < 200) name = s; }
            } catch(e) {}

            var dx = pos.x - pp.x, dz = pos.z - pp.z;
            var dist = Math.sqrt(dx * dx + dz * dz);
            var steps = Math.round(dist / 11.13);

            // Log ALL actors, not just nearby — we want to find the pattern
            logFile('  [' + i + '] id=0x' + id.toString(16) + ' en=' + en + ' name="' + name + '" dist=' + steps + ' steps pos=(' + pos.x.toFixed(1) + ',' + pos.y.toFixed(1) + ',' + pos.z.toFixed(1) + ')');

            // Console: only actors within 10 steps
            if (steps <= 10) {
                logBoth('  >>> [' + i + '] id=0x' + id.toString(16) + ' name="' + name + '" — ' + steps + ' steps away');
            }
        } catch (e) { bad++; if (bad >= 10) break; }
    }
}

function startWatching() {
    if (watching) { logBoth('[*] Already watching'); return; }
    watching = true;
    lastRoomId = getRoomId();

    logBoth('[WATCH] Monitoring transitions. Walk to exits.');
    logBoth('[WATCH] When a transition fires, I will dump all nearby actors to find the exit entity type.');
    logBoth('[WATCH] Current room=' + lastRoomId);

    // Hook TkSetEventJump — fires when a scripted transition starts
    Interceptor.attach(rva(0x48E9D0), {
        onEnter: function(args) {
            var room = getRoomId();
            logBoth('[TRANSITION] TkSetEventJump! Room=' + room);
            dumpNearbyActors('[TRANSITION]');
        }
    });

    // Hook graphicFieldMapLoad — fires when a new map loads
    Interceptor.attach(rva(0x2403C0), {
        onEnter: function(args) {
            var room = getRoomId();
            logBoth('[MAPLOAD] graphicFieldMapLoad! Room=' + room);
        }
    });

    // Hook TkSetEventJumpMapNo — fires when map number is set for transition
    Interceptor.attach(rva(0x48EA60), {
        onEnter: function(args) {
            var room = getRoomId();
            logBoth('[TRANSITION] TkSetEventJumpMapNo! Room=' + room);
            dumpNearbyActors('[TRANSITION-MAP]');
        }
    });

    logBoth('[WATCH] Hooks installed. Walk into the inn door — hooks will fire and dump nearby actors.');
}

// ---- Check if room changed (call manually) ----

function checkRoom() {
    var room = getRoomId();
    var spawn = getSpawnpoint();
    var pos = getPlayerPos();
    logBoth('[CHECK] Room=' + room + ' Spawn=' + spawn +
        (pos ? ' Pos=(' + pos.x.toFixed(2) + ',' + pos.y.toFixed(2) + ',' + pos.z.toFixed(2) + ')' : ''));
    if (room !== lastRoomId) {
        logBoth('[CHECK] *** ROOM CHANGED from ' + lastRoomId + ' to ' + room + ' ***');
        lastRoomId = room;
    }
}

rpc.exports = {
    state: function() { scanState(); return 'ok'; },
    zone: function() { dumpZoneData(); return 'ok'; },
    boundary: function() { findBoundaryEdges(); return 'ok'; },
    watch: function() { startWatching(); return 'ok'; },
    check: function() { checkRoom(); return 'ok'; }
};
