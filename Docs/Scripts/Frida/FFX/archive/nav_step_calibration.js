// @python
// @desc Step calibration — measure game units per walking step
//
// Keyboard (via Python host):
//   R = Start/stop recording (walk between presses, report step count after)
//   P = Print current player position
//   Q = Quit

var logPath = 'D:/Games/Dev/Custom/FFX/ffx-x2-screen-reader/Docs/Scripts/Frida/output/FFX/nav_step_calibration.log';
var logHandle = new File(logPath, 'w');
function logFile(msg) { logHandle.write(msg + '\n'); logHandle.flush(); }
function logBoth(msg) { console.log(msg); logFile(msg); }

var base = Process.enumerateModules()[0].base;
function rva(offset) { return base.add(offset); }

logBoth('[*] nav_step_calibration — Step distance measurement');

var recording = false;
var startPos = null;
var endPos = null;
var sampleCount = 0;
var totalDistance = 0;
var lastSamplePos = null;

// Hook MsMoveChr (RVA 0x3AABA0) to sample position during movement
// This fires when the game processes character movement — event-driven, not polling
var moveHook = null;

function getPlayerPos() {
    var actorsBase = rva(0x1FC44E4).readPointer();
    if (actorsBase.isNull()) return null;
    return {
        x: actorsBase.add(0x0C).readFloat(),
        y: actorsBase.add(0x10).readFloat(),
        z: actorsBase.add(0x14).readFloat()
    };
}

function getPlayerSpeed() {
    var actorsBase = rva(0x1FC44E4).readPointer();
    if (actorsBase.isNull()) return 0;
    return actorsBase.add(0x154).readFloat();
}

function distXZ(a, b) {
    var dx = a.x - b.x;
    var dz = a.z - b.z;
    return Math.sqrt(dx * dx + dz * dz);
}

function startRecording() {
    startPos = getPlayerPos();
    if (!startPos) {
        logBoth('[!] Cannot read player position');
        return;
    }

    recording = true;
    sampleCount = 0;
    totalDistance = 0;
    lastSamplePos = { x: startPos.x, y: startPos.y, z: startPos.z };

    logBoth('[REC] Recording started at (' +
        startPos.x.toFixed(2) + ', ' + startPos.y.toFixed(2) + ', ' + startPos.z.toFixed(2) + ')');
    logBoth('[*] Walk in a straight line, then press R to stop.');
    logBoth('[*] After stopping, enter the number of steps you took.');

    // Install movement hook to sample positions
    if (!moveHook) {
        moveHook = Interceptor.attach(rva(0x3AABA0), {
            onEnter: function(args) {
                if (!recording) return;
                var pos = getPlayerPos();
                if (!pos) return;

                var d = distXZ(lastSamplePos, pos);
                if (d > 0.01) { // Only count meaningful movement
                    totalDistance += d;
                    sampleCount++;
                    lastSamplePos = { x: pos.x, y: pos.y, z: pos.z };
                }
            }
        });
        logFile('[*] MsMoveChr hook installed at rva(0x3AABA0)');
    }
}

function stopRecording() {
    recording = false;
    endPos = getPlayerPos();
    if (!endPos || !startPos) {
        logBoth('[!] Missing position data');
        return;
    }

    var straightLine = distXZ(startPos, endPos);
    var speed = getPlayerSpeed();

    logBoth('[REC] Recording stopped at (' +
        endPos.x.toFixed(2) + ', ' + endPos.y.toFixed(2) + ', ' + endPos.z.toFixed(2) + ')');
    logBoth('[REC] Straight-line distance (XZ): ' + straightLine.toFixed(2) + ' game units');
    logBoth('[REC] Accumulated distance (XZ): ' + totalDistance.toFixed(2) + ' game units (' + sampleCount + ' samples)');
    logBoth('[REC] Current chr_speed: ' + speed.toFixed(4));
    logBoth('[*] How many steps did you walk? Enter the count via the "c" command.');
}

function calculateRatio(stepCount) {
    if (!endPos || !startPos) {
        logBoth('[!] No recording data. Press R to record first.');
        return;
    }

    var straightLine = distXZ(startPos, endPos);
    var n = parseInt(stepCount);
    if (isNaN(n) || n <= 0) {
        logBoth('[!] Invalid step count: ' + stepCount);
        return;
    }

    var ratioStraight = straightLine / n;
    var ratioAccum = totalDistance / n;

    logBoth('[CALIBRATION]');
    logBoth('  Steps walked: ' + n);
    logBoth('  Straight-line: ' + straightLine.toFixed(2) + ' units = ' + ratioStraight.toFixed(2) + ' units/step');
    logBoth('  Accumulated:   ' + totalDistance.toFixed(2) + ' units = ' + ratioAccum.toFixed(2) + ' units/step');
    logBoth('  Recommended units/step: ' + ratioStraight.toFixed(2));

    logFile('[CALIBRATION RESULT] ' + ratioStraight.toFixed(4) + ' game units per step (straight-line)');
    logFile('[CALIBRATION RESULT] ' + ratioAccum.toFixed(4) + ' game units per step (accumulated)');
}

function printPosition() {
    var pos = getPlayerPos();
    if (!pos) {
        logBoth('[!] Cannot read player position');
        return;
    }
    var speed = getPlayerSpeed();
    logBoth('[POS] (' + pos.x.toFixed(2) + ', ' + pos.y.toFixed(2) + ', ' + pos.z.toFixed(2) +
        ') speed=' + speed.toFixed(4));
}

rpc.exports = {
    record: function() {
        if (!recording) startRecording();
        else stopRecording();
        return 'ok';
    },
    calc: function(steps) { calculateRatio(steps); return 'ok'; },
    pos: function() { printPosition(); return 'ok'; }
};
