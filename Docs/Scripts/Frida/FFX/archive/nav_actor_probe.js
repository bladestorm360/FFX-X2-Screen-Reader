// @python
// @desc Actor array probe — validate entity reading, count, names, positions
//
// Keyboard (via Python host):
//   S = Scan all actors (0-127), log details, print summary
//   P = Print player (actor 0) position, rotation, speed
//   Q = Quit

var logPath = 'D:/Games/Dev/Custom/FFX/ffx-x2-screen-reader/Docs/Scripts/Frida/output/FFX/nav_actor_probe.log';
var logHandle = new File(logPath, 'w');
function logFile(msg) { logHandle.write(msg + '\n'); logHandle.flush(); }
function logBoth(msg) { console.log(msg); logFile(msg); }

var base = Process.enumerateModules()[0].base;
function rva(offset) { return base.add(offset); }

// FFX custom character set (from tk_menu_probe.js)
var CHARSET = '0123456789 !"#$%&\'()*+,-./:;<=>?ABCDEFGHIJKLMNOPQRSTUVWXYZ[\\]^_\u2018abcdefghijklmnopqrstuvwxyz{|}~';

logBoth('[*] nav_actor_probe — Actor array discovery probe');
logBoth('[*] Base: ' + base);

// Actor struct constants (from Fahrenheit actor.cs)
var ACTOR_SIZE = 0x880;
var MAX_ACTORS = 128;

// Actor struct offsets
var OFF_CHR_ID       = 0x00;  // ushort
var OFF_CHR_ENABLED  = 0x02;  // ushort
var OFF_CHR_NAME     = 0x04;  // char* pointer
var OFF_CHR_POS      = 0x0C;  // Vector4 (4 floats: X, Y, Z, W)
var OFF_CHR_POS_BK   = 0x1C;  // Vector4 backup
var OFF_CHR_SPEED    = 0x154; // float
var OFF_CHR_ROT      = 0x158; // float (radians)
var OFF_CHR_DIR      = 0x168; // float

// Entity ID classification
function classifyEntity(id) {
    if (id <= 0x0012) return 'Player';
    if (id >= 0x2000 && id < 0x5000) return 'NPC';
    if (id === 0x5001) return 'Save Sphere';
    if (id === 0x5002) return 'Treasure Chest';
    if (id === 0x5004) return 'Shrine';
    if (id === 0x5005) return 'Seal';
    if (id === 0x5006) return 'Ball';
    if (id === 0x5007) return 'Fish';
    if (id === 0x5008) return 'Butterfly';
    if (id === 0x5009) return 'Dog';
    if (id >= 0x5000) return 'Object';
    if (id >= 0x1000 && id < 0x2000) return 'Monster';
    return 'Unknown(0x' + id.toString(16) + ')';
}

// Try to read actor name — test both CString and FFX encoding
function readActorName(actorBase) {
    try {
        var namePtr = actorBase.add(OFF_CHR_NAME).readPointer();
        if (namePtr.isNull()) return '(null)';
        // Try plain CString first
        var name = namePtr.readCString();
        if (name && name.length > 0 && name.length < 200) return name;
        return '(unreadable)';
    } catch (e) {
        return '(error)';
    }
}

function readActorPosition(actorBase) {
    var x = actorBase.add(OFF_CHR_POS).readFloat();
    var y = actorBase.add(OFF_CHR_POS + 4).readFloat();
    var z = actorBase.add(OFF_CHR_POS + 8).readFloat();
    return { x: x, y: y, z: z };
}

function getActorArrayBase() {
    // Actor* actors = *(Actor**)(base + 0x1FC44E4)
    // ptr_at<nint>(0x1FC44E4) => pointer AT that address
    // *ptr => dereference to get Actor*
    var ptrAddr = rva(0x1FC44E4);
    var actorArrayPtr = ptrAddr.readPointer();
    if (actorArrayPtr.isNull()) return null;
    return actorArrayPtr;
}

function scanActors() {
    var actorsBase = getActorArrayBase();
    if (!actorsBase) {
        logBoth('[!] Actor array pointer is NULL — not in field?');
        return;
    }
    logBoth('[*] Actor array base: ' + actorsBase);

    var counts = { Player: 0, NPC: 0, 'Save Sphere': 0, 'Treasure Chest': 0, Object: 0, Monster: 0, Other: 0 };
    var activeCount = 0;
    var consecutiveEmpty = 0;
    var effectiveMax = MAX_ACTORS;

    for (var i = 0; i < MAX_ACTORS; i++) {
        var actorAddr = actorsBase.add(i * ACTOR_SIZE);
        try {
            var chrId = actorAddr.add(OFF_CHR_ID).readU16();
            var chrEnabled = actorAddr.add(OFF_CHR_ENABLED).readU16();

            // 0xCDCD = MSVC debug heap fill — uninitialized memory, skip
            var posX = actorAddr.add(OFF_CHR_POS).readFloat();
            if (chrId === 0xCDCD || posX < -400000000 || isNaN(posX)) {
                consecutiveEmpty++;
                if (consecutiveEmpty >= 10) {
                    effectiveMax = i - 9;
                    break;
                }
                continue;
            }
            consecutiveEmpty = 0;

            if (chrEnabled === 0 && chrId === 0) {
                logFile('[' + i + '] EMPTY');
                continue;
            }

            if (chrEnabled === 0) {
                logFile('[' + i + '] id=0x' + chrId.toString(16) + ' DISABLED');
                continue;
            }

            activeCount++;
            var name = readActorName(actorAddr);
            var pos = readActorPosition(actorAddr);
            var speed = actorAddr.add(OFF_CHR_SPEED).readFloat();
            var rot = actorAddr.add(OFF_CHR_ROT).readFloat();
            var dir = actorAddr.add(OFF_CHR_DIR).readFloat();
            var category = classifyEntity(chrId);

            // Count by category
            if (category === 'Player') counts.Player++;
            else if (category === 'NPC') counts.NPC++;
            else if (category === 'Save Sphere') counts['Save Sphere']++;
            else if (category === 'Treasure Chest') counts['Treasure Chest']++;
            else if (category === 'Monster') counts.Monster++;
            else if (category.indexOf('Object') >= 0 || category === 'Ball' || category === 'Fish' ||
                     category === 'Butterfly' || category === 'Dog' || category === 'Shrine' ||
                     category === 'Seal') counts.Object++;
            else counts.Other++;

            // Detailed log to file only
            logFile('[' + i + '] id=0x' + chrId.toString(16) +
                ' enabled=' + chrEnabled +
                ' cat=' + category +
                ' name="' + name + '"' +
                ' pos=(' + pos.x.toFixed(2) + ', ' + pos.y.toFixed(2) + ', ' + pos.z.toFixed(2) + ')' +
                ' speed=' + speed.toFixed(2) +
                ' rot=' + rot.toFixed(4) +
                ' dir=' + dir.toFixed(4));
        } catch (e) {
            logFile('[' + i + '] READ ERROR: ' + e.message);
            consecutiveEmpty++;
            if (consecutiveEmpty >= 10) {
                effectiveMax = i - 9;
                break;
            }
        }
    }

    // Console summary
    var summary = '[SCAN] ' + activeCount + ' active actors (scanned ' + effectiveMax + '): ';
    var parts = [];
    if (counts.Player > 0) parts.push(counts.Player + ' Players');
    if (counts.NPC > 0) parts.push(counts.NPC + ' NPCs');
    if (counts['Save Sphere'] > 0) parts.push(counts['Save Sphere'] + ' Save Spheres');
    if (counts['Treasure Chest'] > 0) parts.push(counts['Treasure Chest'] + ' Chests');
    if (counts.Object > 0) parts.push(counts.Object + ' Objects');
    if (counts.Monster > 0) parts.push(counts.Monster + ' Monsters');
    if (counts.Other > 0) parts.push(counts.Other + ' Other');
    summary += parts.join(', ');
    logBoth(summary);
    logBoth('[*] Full details in log file.');
}

function printPlayer() {
    var actorsBase = getActorArrayBase();
    if (!actorsBase) {
        logBoth('[!] Actor array pointer is NULL');
        return;
    }

    var actor0 = actorsBase;
    var chrId = actor0.add(OFF_CHR_ID).readU16();
    var chrEnabled = actor0.add(OFF_CHR_ENABLED).readU16();
    var name = readActorName(actor0);
    var pos = readActorPosition(actor0);
    var speed = actor0.add(OFF_CHR_SPEED).readFloat();
    var rot = actor0.add(OFF_CHR_ROT).readFloat();
    var dir = actor0.add(OFF_CHR_DIR).readFloat();

    logBoth('[PLAYER] id=0x' + chrId.toString(16) +
        ' enabled=' + chrEnabled +
        ' name="' + name + '"' +
        ' pos=(' + pos.x.toFixed(2) + ', ' + pos.y.toFixed(2) + ', ' + pos.z.toFixed(2) + ')' +
        ' speed=' + speed.toFixed(2) +
        ' rot=' + rot.toFixed(4) + ' rad' +
        ' dir=' + dir.toFixed(4));
}

// RPC exports for Python host
rpc.exports = {
    scan: function() { scanActors(); return 'ok'; },
    player: function() { printPlayer(); return 'ok'; }
};
