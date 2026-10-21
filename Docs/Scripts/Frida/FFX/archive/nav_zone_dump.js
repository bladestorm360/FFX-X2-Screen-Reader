// @python
// @desc Zone data probe — dump and analyze Atel area/zone trigger data
//
// Reads the script chunk's area data and searches for coordinate-like
// float values that could define trigger zones (invisible exit boundaries).
//
// Keyboard:
//   z = Dump zone/area data with float analysis
//   a = Scan area data for coordinate-range floats (matching map scale)
//   s = Show current state (room, position, chunk info)
//   q = Quit

var logPath = 'D:/Games/Dev/Custom/FFX/ffx-x2-screen-reader/Docs/Scripts/Frida/output/FFX/nav_zone_dump.log';
var logHandle = new File(logPath, 'w');
function logFile(msg) { logHandle.write(msg + '\n'); logHandle.flush(); }
function logBoth(msg) { console.log(msg); logFile(msg); }

var base = Process.enumerateModules()[0].base;
function rva(offset) { return base.add(offset); }

logBoth('[*] nav_zone_dump — Zone trigger data analysis');

function getPlayerPos() {
    var ab = rva(0x1FC44E4).readPointer();
    if (ab.isNull()) return null;
    return { x: ab.add(0x0C).readFloat(), y: ab.add(0x10).readFloat(), z: ab.add(0x14).readFloat() };
}

function getScriptChunk() {
    try {
        var workerPtr = rva(0xF270A4).readPointer();
        if (workerPtr.isNull() || workerPtr.toUInt32() < 0x10000) return null;
        var chunk = workerPtr.add(0x04).readPointer();
        if (chunk.isNull() || chunk.toUInt32() < 0x10000) return null;
        return chunk;
    } catch (e) { return null; }
}

function showState() {
    var pos = getPlayerPos();
    var room = rva(0xD2CA90).readU16();
    logBoth('[STATE] Room=' + room + (pos ? ' Pos=(' + pos.x.toFixed(2) + ',' + pos.y.toFixed(2) + ',' + pos.z.toFixed(2) + ')' : ''));

    var chunk = getScriptChunk();
    if (!chunk) { logBoth('[STATE] No script chunk'); return; }

    var mapStart = chunk.add(0x04).readU32();
    var offsetAuthor = chunk.add(0x08).readU32();
    var zoneBytes = chunk.add(0x1E).readU16();
    var offsetArea = chunk.add(0x28).readU32();
    var offsetOther = chunk.add(0x2C).readU32();
    var offsetCode = chunk.add(0x30).readU32();
    var scriptNum = chunk.add(0x34).readU16();
    var codeLen = chunk.add(0x00).readU32();

    logBoth('[CHUNK] code_length=0x' + codeLen.toString(16) +
        ' map_start=0x' + mapStart.toString(16) +
        ' offset_area=0x' + offsetArea.toString(16) +
        ' offset_other=0x' + offsetOther.toString(16) +
        ' offset_code=0x' + offsetCode.toString(16) +
        ' zone_bytes=0x' + zoneBytes.toString(16) + ' (' + zoneBytes + ')' +
        ' script_num=' + scriptNum);

    // Calculate sizes between sections
    var entranceCount = Math.floor((offsetAuthor - mapStart) / 0x20);
    logBoth('[CHUNK] Entrances: ' + entranceCount +
        ' | Area data at +0x' + offsetArea.toString(16) +
        ' | Code at +0x' + offsetCode.toString(16) +
        ' | Total chunk ~0x' + codeLen.toString(16) + ' bytes');
}

function dumpZoneData() {
    var chunk = getScriptChunk();
    if (!chunk) { logBoth('[!] No script chunk'); return; }

    var offsetArea = chunk.add(0x28).readU32();
    var offsetCode = chunk.add(0x30).readU32();
    var codeLen = chunk.add(0x00).readU32();
    var zoneBytes = chunk.add(0x1E).readU16();

    if (offsetArea === 0) { logBoth('[!] offset_area is 0'); return; }

    var areaAddr = chunk.add(offsetArea);
    // Read up to 1024 bytes or until end of chunk
    var maxRead = Math.min(1024, codeLen - offsetArea);
    if (maxRead <= 0) maxRead = 1024; // area might be after code

    logBoth('[ZONE] Area data at chunk+0x' + offsetArea.toString(16) + ', reading ' + maxRead + ' bytes');
    logBoth('[ZONE] zone_bytes field = 0x' + zoneBytes.toString(16) + ' (' + zoneBytes + ')');

    // Hex dump (to log only)
    for (var off = 0; off < maxRead; off += 16) {
        var hex = '', ascii = '';
        for (var b = 0; b < 16 && (off + b) < maxRead; b++) {
            var byte = areaAddr.add(off + b).readU8();
            hex += (byte < 16 ? '0' : '') + byte.toString(16) + ' ';
            ascii += (byte >= 32 && byte < 127) ? String.fromCharCode(byte) : '.';
        }
        logFile('+' + off.toString(16).padStart(4, '0') + ': ' + hex.padEnd(48) + ' ' + ascii);
    }

    // Try interpreting every 4-byte aligned offset as a float
    logBoth('[ZONE] Float scan (values in plausible coordinate range -2000 to 2000):');
    var coordFloats = [];
    for (var off = 0; off + 4 <= maxRead; off += 4) {
        var f = areaAddr.add(off).readFloat();
        if (!isNaN(f) && isFinite(f) && Math.abs(f) > 0.1 && Math.abs(f) < 2000) {
            coordFloats.push({ off: off, val: f });
        }
    }

    if (coordFloats.length === 0) {
        logBoth('[ZONE] No coordinate-like floats found in area data');
    } else {
        logBoth('[ZONE] Found ' + coordFloats.length + ' coordinate-like floats:');
        for (var i = 0; i < coordFloats.length && i < 50; i++) {
            logBoth('  +0x' + coordFloats[i].off.toString(16).padStart(4, '0') + ': ' + coordFloats[i].val.toFixed(2));
        }
        if (coordFloats.length > 50) logBoth('  ... and ' + (coordFloats.length - 50) + ' more (see log)');
        for (var i = 50; i < coordFloats.length; i++) {
            logFile('  +0x' + coordFloats[i].off.toString(16).padStart(4, '0') + ': ' + coordFloats[i].val.toFixed(2));
        }
    }

    // Also try interpreting as short pairs (like navmesh vertices)
    logBoth('[ZONE] Short scan (values in range -10000 to 10000):');
    var coordShorts = [];
    for (var off = 0; off + 2 <= maxRead; off += 2) {
        var s = areaAddr.add(off).readS16();
        if (Math.abs(s) > 1 && Math.abs(s) < 10000) {
            coordShorts.push({ off: off, val: s });
        }
    }
    logBoth('[ZONE] Found ' + coordShorts.length + ' plausible shorts (see log for details)');
    for (var i = 0; i < coordShorts.length; i++) {
        logFile('  short +0x' + coordShorts[i].off.toString(16).padStart(4, '0') + ': ' + coordShorts[i].val);
    }

    // Look for repeating struct patterns
    logBoth('[ZONE] Checking for repeating struct patterns...');
    // Try struct sizes 8, 12, 16, 20, 24, 32
    var sizes = [8, 12, 16, 20, 24, 32];
    for (var si = 0; si < sizes.length; si++) {
        var sz = sizes[si];
        var records = Math.floor(maxRead / sz);
        if (records < 2) continue;
        // Check if first bytes of each record follow a pattern
        var firstU16s = [];
        for (var r = 0; r < Math.min(records, 10); r++) {
            firstU16s.push(areaAddr.add(r * sz).readU16());
        }
        // Check if they're sequential, constant, or have a pattern
        var allSame = firstU16s.every(function(v) { return v === firstU16s[0]; });
        var sequential = true;
        for (var r = 1; r < firstU16s.length; r++) {
            if (firstU16s[r] !== firstU16s[r-1] + 1) { sequential = false; break; }
        }
        if (allSame || sequential) {
            logBoth('  Size ' + sz + ': first U16s ' + (allSame ? 'all=' + firstU16s[0] : 'sequential from ' + firstU16s[0]) + ' (' + records + ' records)');
        }
    }
}

function scanForTriggerZones() {
    var chunk = getScriptChunk();
    if (!chunk) { logBoth('[!] No script chunk'); return; }

    var pos = getPlayerPos();
    var offsetArea = chunk.add(0x28).readU32();
    var codeLen = chunk.add(0x00).readU32();

    if (offsetArea === 0) { logBoth('[!] No area data'); return; }

    var areaAddr = chunk.add(offsetArea);
    var maxRead = Math.min(2048, codeLen > offsetArea ? codeLen - offsetArea : 2048);

    logBoth('[SCAN] Searching area data for trigger zone coordinate clusters...');

    // Look for groups of 4 consecutive floats that form bounding boxes
    // A trigger zone would be: x_min, z_min, x_max, z_max (or similar)
    var zones = [];
    for (var off = 0; off + 16 <= maxRead; off += 4) {
        var f0 = areaAddr.add(off).readFloat();
        var f1 = areaAddr.add(off + 4).readFloat();
        var f2 = areaAddr.add(off + 8).readFloat();
        var f3 = areaAddr.add(off + 12).readFloat();

        // All 4 should be plausible coordinates
        if (isNaN(f0) || isNaN(f1) || isNaN(f2) || isNaN(f3)) continue;
        if (!isFinite(f0) || !isFinite(f1) || !isFinite(f2) || !isFinite(f3)) continue;

        var allCoord = Math.abs(f0) < 5000 && Math.abs(f1) < 5000 &&
                       Math.abs(f2) < 5000 && Math.abs(f3) < 5000;
        if (!allCoord) continue;

        // Check if it looks like a bounding box (min < max pattern)
        var looksLikeBBox = (f0 < f2 && f1 < f3) || (f0 > f2 && f1 > f3);
        // Or check if it looks like 2 points (trigger line)
        var area = Math.abs(f2 - f0) * Math.abs(f3 - f1);

        if (area > 1 && area < 1000000) {
            var cx = (f0 + f2) / 2, cz = (f1 + f3) / 2;
            var dist = pos ? Math.sqrt((cx-pos.x)*(cx-pos.x) + (cz-pos.z)*(cz-pos.z)) : 0;
            zones.push({
                off: off, f0: f0, f1: f1, f2: f2, f3: f3,
                cx: cx, cz: cz, area: area, dist: dist, bbox: looksLikeBBox
            });
        }
    }

    if (zones.length === 0) {
        logBoth('[SCAN] No bounding-box-like coordinate groups found');
    } else {
        zones.sort(function(a, b) { return a.dist - b.dist; });
        logBoth('[SCAN] Found ' + zones.length + ' potential trigger zones:');
        for (var i = 0; i < zones.length && i < 20; i++) {
            var z = zones[i];
            var steps = Math.round(z.dist / 11.13);
            logBoth('  +0x' + z.off.toString(16) + ': (' + z.f0.toFixed(1) + ',' + z.f1.toFixed(1) + ') to (' +
                z.f2.toFixed(1) + ',' + z.f3.toFixed(1) + ')' +
                (z.bbox ? ' [BBOX]' : '') +
                ' center=(' + z.cx.toFixed(1) + ',' + z.cz.toFixed(1) + ') ~' + steps + ' steps');
        }
    }
}

rpc.exports = {
    state: function() { showState(); return 'ok'; },
    zone: function() { dumpZoneData(); return 'ok'; },
    scan: function() { scanForTriggerZones(); return 'ok'; }
};
