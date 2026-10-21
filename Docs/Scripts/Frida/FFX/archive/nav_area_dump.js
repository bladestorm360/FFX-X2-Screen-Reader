// @python
// @desc Area data dump — read the FULL offset_area block from the Atel script chunk
//
// The offset_area data contains trigger zone definitions that determine where
// map transitions (exits) are located. Previous probes failed because they
// assumed area data was before the code section — it's actually AFTER it.
//
// Keyboard:
//   s = Show chunk layout (all offsets and sizes)
//   a = Dump area data (raw hex + structure analysis)
//   m = Dump MapEntrance data with all fields
//   q = Quit

var logPath = 'D:/Games/Dev/Custom/FFX/ffx-x2-screen-reader/Docs/Scripts/Frida/output/FFX/nav_area_dump.log';
var logHandle = new File(logPath, 'w');
function logFile(msg) { logHandle.write(msg + '\n'); logHandle.flush(); }
function logBoth(msg) { console.log(msg); logFile(msg); }

var base = Process.enumerateModules()[0].base;
function rva(offset) { return base.add(offset); }

logBoth('[*] nav_area_dump — Atel area/trigger zone reader');

function getScriptChunk() {
    try {
        var wp = rva(0xF270A4).readPointer();
        if (wp.isNull() || wp.toUInt32() < 0x10000) return null;
        var chunk = wp.add(0x04).readPointer();
        if (chunk.isNull() || chunk.toUInt32() < 0x10000) return null;
        return chunk;
    } catch (e) { return null; }
}

function showLayout() {
    var chunk = getScriptChunk();
    if (!chunk) { logBoth('[!] No chunk'); return; }

    var room = rva(0xD2CA90).readU16();

    // Read ALL chunk header fields
    var codeLength   = chunk.add(0x00).readU32();
    var mapStart     = chunk.add(0x04).readU32();
    var offsetAuthor = chunk.add(0x08).readU32();
    var offsetName   = chunk.add(0x0C).readU32();
    var offsetJumpsEnd = chunk.add(0x10).readU32();
    var field14      = chunk.add(0x14).readU16();
    var field16      = chunk.add(0x16).readU16();
    var mainScriptIdx = chunk.add(0x18).readU16();
    var field1A      = chunk.add(0x1A).readU16();
    var field1C      = chunk.add(0x1C).readU16();
    var zoneBytes    = chunk.add(0x1E).readU16();
    var offsetEventData = chunk.add(0x20).readU32();
    var field24      = chunk.add(0x24).readU32();
    var offsetArea   = chunk.add(0x28).readU32();
    var offsetOther  = chunk.add(0x2C).readU32();
    var offsetCode   = chunk.add(0x30).readU32();
    var scriptNum    = chunk.add(0x34).readU16();
    var scriptNumExSub = chunk.add(0x36).readU16();

    var entranceCount = (mapStart > 0 && offsetAuthor > mapStart) ? Math.floor((offsetAuthor - mapStart) / 0x20) : 0;

    logBoth('[LAYOUT] Room=' + room + ' chunk=' + chunk);
    logBoth('[LAYOUT] code_length=0x' + codeLength.toString(16) + ' (' + codeLength + ')');
    logBoth('[LAYOUT] map_start=0x' + mapStart.toString(16));
    logBoth('[LAYOUT] offset_author=0x' + offsetAuthor.toString(16));
    logBoth('[LAYOUT] offset_name=0x' + offsetName.toString(16));
    logBoth('[LAYOUT] offset_jumps_end=0x' + offsetJumpsEnd.toString(16));
    logBoth('[LAYOUT] main_script_idx=' + mainScriptIdx);
    logBoth('[LAYOUT] zone_bytes=0x' + zoneBytes.toString(16) + ' (' + zoneBytes + ')');
    logBoth('[LAYOUT] offset_event_data=0x' + offsetEventData.toString(16));
    logBoth('[LAYOUT] offset_area=0x' + offsetArea.toString(16));
    logBoth('[LAYOUT] offset_other=0x' + offsetOther.toString(16));
    logBoth('[LAYOUT] offset_code=0x' + offsetCode.toString(16));
    logBoth('[LAYOUT] script_num=' + scriptNum + ' (excl subs=' + scriptNumExSub + ')');
    logBoth('[LAYOUT] entrances=' + entranceCount);

    // Try to read author and name strings
    try {
        var author = chunk.add(offsetAuthor).readCString();
        var name = chunk.add(offsetName).readCString();
        logBoth('[LAYOUT] author="' + (author || '?') + '" name="' + (name || '?') + '"');
    } catch (e) {}

    // Show memory layout order
    var sections = [
        { name: 'header', off: 0 },
        { name: 'script_headers', off: 0x38 },
        { name: 'map_entrances', off: mapStart },
        { name: 'author_string', off: offsetAuthor },
        { name: 'name_string', off: offsetName },
        { name: 'event_data', off: offsetEventData },
        { name: 'area_data', off: offsetArea },
        { name: 'other', off: offsetOther },
        { name: 'code', off: offsetCode },
        { name: 'jumps_end', off: offsetJumpsEnd }
    ].filter(function(s) { return s.off > 0; });
    sections.sort(function(a, b) { return a.off - b.off; });

    logBoth('[LAYOUT] Section order:');
    for (var i = 0; i < sections.length; i++) {
        var nextOff = (i + 1 < sections.length) ? sections[i + 1].off : codeLength;
        var size = nextOff - sections[i].off;
        logBoth('  0x' + sections[i].off.toString(16).padStart(5, '0') + ' ' + sections[i].name + ' (' + size + ' bytes)');
    }
}

function dumpAreaData() {
    var chunk = getScriptChunk();
    if (!chunk) { logBoth('[!] No chunk'); return; }

    var offsetArea = chunk.add(0x28).readU32();
    var zoneBytes = chunk.add(0x1E).readU16();
    var codeLength = chunk.add(0x00).readU32();
    var room = rva(0xD2CA90).readU16();

    logBoth('[AREA] Room=' + room + ' offset_area=0x' + offsetArea.toString(16) + ' zone_bytes=' + zoneBytes);

    if (offsetArea === 0) { logBoth('[!] No area data defined'); return; }

    var areaAddr = chunk.add(offsetArea);

    // Determine how much to read — try up to 2048 bytes
    var readSize = 2048;

    logBoth('[AREA] Reading ' + readSize + ' bytes from chunk+0x' + offsetArea.toString(16) + '...');

    // Read raw bytes
    var bytes;
    try {
        bytes = new Uint8Array(areaAddr.readByteArray(readSize));
    } catch (e) {
        logBoth('[!] Failed to read area data: ' + e.message);
        // Try smaller reads
        for (var trySize = 1024; trySize >= 64; trySize = Math.floor(trySize / 2)) {
            try {
                bytes = new Uint8Array(areaAddr.readByteArray(trySize));
                readSize = trySize;
                logBoth('[AREA] Read ' + trySize + ' bytes successfully');
                break;
            } catch (e2) {}
        }
        if (!bytes) { logBoth('[!] Cannot read area data at all'); return; }
    }

    // Hex dump to log
    for (var off = 0; off < readSize; off += 16) {
        var hex = '';
        for (var b = 0; b < 16 && (off + b) < readSize; b++) {
            hex += (bytes[off + b] < 16 ? '0' : '') + bytes[off + b].toString(16) + ' ';
        }
        logFile('+' + off.toString(16).padStart(4, '0') + ': ' + hex);
    }

    // Analyze structure: try different record sizes
    logBoth('[AREA] Structure analysis:');

    // First, check if it starts with a count or header
    var first4 = bytes[0] | (bytes[1] << 8) | (bytes[2] << 16) | (bytes[3] << 24);
    var firstU16 = bytes[0] | (bytes[1] << 8);
    logBoth('  First U16=' + firstU16 + ' First U32=0x' + first4.toString(16) + ' (' + first4 + ')');

    // Read as DataView for float interpretation
    var buf = new ArrayBuffer(readSize);
    var dv = new DataView(buf);
    for (var i = 0; i < readSize; i++) dv.setUint8(i, bytes[i]);

    // Scan for float values at every 4-byte boundary
    var floats = [];
    for (var off = 0; off + 4 <= readSize; off += 4) {
        var f = dv.getFloat32(off, true); // little-endian
        if (!isNaN(f) && isFinite(f) && Math.abs(f) > 0.1 && Math.abs(f) < 5000) {
            floats.push({ off: off, val: f });
        }
    }

    logBoth('  ' + floats.length + ' coordinate-range floats found');
    for (var i = 0; i < Math.min(floats.length, 30); i++) {
        logBoth('  +0x' + floats[i].off.toString(16).padStart(3, '0') + ': ' + floats[i].val.toFixed(2));
    }
    if (floats.length > 30) logBoth('  ... ' + (floats.length - 30) + ' more in log');

    // Look for repeating structures by checking for consistent patterns
    logBoth('[AREA] Searching for record structure...');
    // Try to find the record size by looking for repeating U16 or U32 patterns
    for (var stride = 4; stride <= 48; stride += 2) {
        var records = Math.floor(readSize / stride);
        if (records < 3) continue;

        // Check if byte[0] of each record has a consistent range
        var firstBytes = [];
        for (var r = 0; r < Math.min(records, 20); r++) {
            firstBytes.push(bytes[r * stride]);
        }
        var allSmall = firstBytes.every(function(b) { return b < 20; });
        var allSame = firstBytes.every(function(b) { return b === firstBytes[0]; });

        // Check for sequential u16 at start of each record
        var seqU16 = true;
        for (var r = 1; r < Math.min(records, 10); r++) {
            var prev = bytes[(r-1)*stride] | (bytes[(r-1)*stride+1] << 8);
            var cur = bytes[r*stride] | (bytes[r*stride+1] << 8);
            if (cur !== prev + 1) { seqU16 = false; break; }
        }

        if (allSame || seqU16 || allSmall) {
            logBoth('  stride=' + stride + ': ' + records + ' records, first_bytes=[' +
                firstBytes.slice(0, 5).join(',') + '...]' +
                (allSame ? ' ALL_SAME' : '') + (seqU16 ? ' SEQUENTIAL' : '') + (allSmall ? ' SMALL' : ''));

            // Dump first 3 records at this stride
            for (var r = 0; r < Math.min(3, records); r++) {
                var recHex = '';
                for (var b = 0; b < stride; b++) {
                    recHex += (bytes[r * stride + b] < 16 ? '0' : '') + bytes[r * stride + b].toString(16) + ' ';
                }
                logBoth('    [' + r + '] ' + recHex);
            }
        }
    }
}

function dumpEntrances() {
    var chunk = getScriptChunk();
    if (!chunk) { logBoth('[!] No chunk'); return; }

    var mapStart = chunk.add(0x04).readU32();
    var offsetAuthor = chunk.add(0x08).readU32();
    var room = rva(0xD2CA90).readU16();

    var count = Math.floor((offsetAuthor - mapStart) / 0x20);
    logBoth('[ENTRANCES] Room=' + room + ' — ' + count + ' entries:');

    for (var i = 0; i < count; i++) {
        var addr = chunk.add(mapStart + i * 0x20);
        // Read all 0x20 bytes
        var hex = '';
        for (var b = 0; b < 0x20; b++) {
            var byte = addr.add(b).readU8();
            hex += (byte < 16 ? '0' : '') + byte.toString(16) + ' ';
        }

        var s00 = addr.readS16();
        var s02 = addr.add(0x02).readS16();
        var s04 = addr.add(0x04).readS16();
        var s06 = addr.add(0x06).readS16();
        var rot = addr.add(0x08).readFloat();
        var x = addr.add(0x0C).readFloat();
        var y = addr.add(0x10).readFloat();
        var z = addr.add(0x14).readFloat();
        var u18 = addr.add(0x18).readS32();
        var u1C = addr.add(0x1C).readS32();

        logBoth('  [' + i + '] fields=(' + s00 + ',' + s02 + ',' + s04 + ',' + s06 + ')' +
            ' rot=' + rot.toFixed(3) + ' pos=(' + x.toFixed(2) + ',' + y.toFixed(2) + ',' + z.toFixed(2) + ')' +
            ' tail=(' + u18 + ',' + u1C + ')');
        logFile('  [' + i + '] hex: ' + hex);
    }
}

rpc.exports = {
    layout: function() { showLayout(); return 'ok'; },
    area: function() { dumpAreaData(); return 'ok'; },
    entrances: function() { dumpEntrances(); return 'ok'; }
};
