// @python
// @desc Exit finder — hook EventJump callers to identify which Atel function triggers exits
//
// Strategy:
//   1. Hook TkSetEventJump — when it fires, inspect the Atel worker state
//   2. Read the current Atel worker's script bytecode position to find the CALL instruction
//   3. Identify the Atel function ID that triggers map transitions
//   4. Scan ALL scripts in the current chunk for calls to that function
//   5. Extract trigger parameters (map_id, spawn_point)
//
// Keyboard:
//   w = Install hooks (TkSetEventJump + SgEvent table monitor)
//   s = Scan current state
//   e = Enumerate all SgEvent functions and their addresses
//   q = Quit

var logPath = 'D:/Games/Dev/Custom/FFX/ffx-x2-screen-reader/Docs/Scripts/Frida/output/FFX/nav_exit_find.log';
var logHandle = new File(logPath, 'w');
function logFile(msg) { logHandle.write(msg + '\n'); logHandle.flush(); }
function logBoth(msg) { console.log(msg); logFile(msg); }

var base = Process.enumerateModules()[0].base;
function rva(offset) { return base.add(offset); }

logBoth('[*] nav_exit_find — Atel EventJump trigger finder');

// RVAs
var RVA_TkSetEventJump     = 0x48E9D0;
var RVA_TkSetEventJumpMapNo = 0x48EA60;
var RVA_SgEvent_Table      = 0x488D88; // SgEvent namespace function table
var RVA_Current_Worker     = 0xF270A4;

// AtelCallTarget struct: 0x10 bytes (init, exec, ret_float, ret_int)
var ATEL_CT_SIZE = 0x10;
// SgEvent has 71 functions (0x4000-0x4046)
var SG_EVENT_COUNT = 71;

function getPlayerPos() {
    var ab = rva(0x1FC44E4).readPointer();
    if (ab.isNull()) return null;
    return { x: ab.add(0x0C).readFloat(), y: ab.add(0x10).readFloat(), z: ab.add(0x14).readFloat() };
}

function getCurrentWorker() {
    try {
        var wp = rva(RVA_Current_Worker).readPointer();
        if (wp.isNull() || wp.toUInt32() < 0x10000) return null;
        return wp;
    } catch (e) { return null; }
}

function showState() {
    var pos = getPlayerPos();
    var room = rva(0xD2CA90).readU16();
    logBoth('[STATE] Room=' + room + (pos ? ' Pos=(' + pos.x.toFixed(2) + ',' + pos.y.toFixed(2) + ',' + pos.z.toFixed(2) + ')' : ''));

    var worker = getCurrentWorker();
    if (worker) {
        var chunkPtr = worker.add(0x04).readPointer();
        var workerIdx = worker.add(0x2E).readU16();
        var entryPt = worker.add(0x2C).readU16();
        var threadPri = worker.add(0x32).readU8();
        logBoth('[WORKER] idx=' + workerIdx + ' entry=' + entryPt + ' thread_pri=' + threadPri + ' chunk=' + chunkPtr);
    }
}

// Enumerate SgEvent function table to find which functions exist
function enumSgEvents() {
    logBoth('[SGEVENT] Enumerating SgEvent function table at rva(0x' + RVA_SgEvent_Table.toString(16) + '):');
    var tableBase = rva(RVA_SgEvent_Table);

    var tkSetEJ = rva(RVA_TkSetEventJump);
    var tkSetEJMN = rva(RVA_TkSetEventJumpMapNo);

    for (var i = 0; i < SG_EVENT_COUNT; i++) {
        var ct = tableBase.add(i * ATEL_CT_SIZE);
        var initFunc = ct.readPointer();
        var execFunc = ct.add(4).readPointer();
        var retFloatFunc = ct.add(8).readPointer();
        var retIntFunc = ct.add(12).readPointer();

        // Log to file; console only for non-null exec funcs
        var id = '0x' + (0x4000 + i).toString(16);
        var line = '[' + id + '] exec=' + execFunc + ' init=' + initFunc;

        if (!execFunc.isNull() && execFunc.toUInt32() > 0x10000) {
            logBoth(line);
        } else {
            logFile(line);
        }
    }
}

function startWatching() {
    logBoth('[WATCH] Hooking TkSetEventJump + TkSetEventJumpMapNo...');

    // Hook TkSetEventJump — capture Atel worker state when it fires
    Interceptor.attach(rva(RVA_TkSetEventJump), {
        onEnter: function(args) {
            var pos = getPlayerPos();
            var room = rva(0xD2CA90).readU16();

            logBoth('[EVENT-JUMP] TkSetEventJump fired! Room=' + room);
            logBoth('[EVENT-JUMP] param_1 (map)=' + args[0] + ' param_2 (spawn)=' + args[1]);
            if (pos) logBoth('[EVENT-JUMP] Player pos=(' + pos.x.toFixed(2) + ',' + pos.y.toFixed(2) + ',' + pos.z.toFixed(2) + ')');

            // Read Atel worker state to find what script called this
            var worker = getCurrentWorker();
            if (worker) {
                var chunkPtr = worker.add(0x04).readPointer();
                var workerIdx = worker.add(0x2E).readU16();
                var entryPt = worker.add(0x2C).readU16();
                var threadPri = worker.add(0x32).readU8();

                logBoth('[EVENT-JUMP] Worker idx=' + workerIdx + ' entry=' + entryPt + ' thread=' + threadPri);

                // Try to read current thread's PC (program counter)
                // threads start at offset 0x12C, each AtelWorkThread is 0x4C bytes
                // pc is at offset 0x18 within the thread
                var threadOffset = 0x12C + threadPri * 0x4C;
                var pcPtr = worker.add(threadOffset + 0x18).readPointer();
                var codeBase = null;
                if (!chunkPtr.isNull() && chunkPtr.toUInt32() > 0x10000) {
                    var offsetCode = chunkPtr.add(0x30).readU32();
                    codeBase = chunkPtr.add(offsetCode);
                }
                if (codeBase && !pcPtr.isNull()) {
                    var pcOffset = pcPtr.toUInt32() - codeBase.toUInt32();
                    logBoth('[EVENT-JUMP] PC offset in code: 0x' + pcOffset.toString(16));

                    // Read a few bytes around PC to see what instruction called us
                    var nearby = '';
                    for (var b = -10; b < 10; b++) {
                        try {
                            var byte = pcPtr.add(b).readU8();
                            nearby += (byte < 16 ? '0' : '') + byte.toString(16) + ' ';
                        } catch(e) { nearby += '?? '; }
                    }
                    logBoth('[EVENT-JUMP] Bytecode around PC: ' + nearby);
                }

                // Also dump Atel float registers (may contain position thresholds)
                logBoth('[EVENT-JUMP] Atel float registers:');
                for (var r = 0; r < 10; r++) {
                    var fval = worker.add(0x58 + r * 4).readFloat();
                    if (!isNaN(fval) && Math.abs(fval) > 0.01 && Math.abs(fval) < 10000) {
                        logBoth('  f' + r + '=' + fval.toFixed(2));
                    }
                }
                // Int registers
                logBoth('[EVENT-JUMP] Atel int registers:');
                for (var r = 0; r < 4; r++) {
                    var ival = worker.add(0x48 + r * 4).readS32();
                    logBoth('  i' + r + '=' + ival);
                }
            }

            // Dump return address / call stack
            logBoth('[EVENT-JUMP] Return addr: ' + this.returnAddress);
        }
    });

    // Hook TkSetEventJumpMapNo
    Interceptor.attach(rva(RVA_TkSetEventJumpMapNo), {
        onEnter: function(args) {
            logBoth('[EVENT-JUMP-MAP] TkSetEventJumpMapNo! params=(' + args[0] + ',' + args[1] + ')');
        }
    });

    logBoth('[WATCH] Hooks installed. Walk through a transition to capture Atel state.');
}

rpc.exports = {
    state: function() { showState(); return 'ok'; },
    enumsg: function() { enumSgEvents(); return 'ok'; },
    watch: function() { startWatching(); return 'ok'; }
};
