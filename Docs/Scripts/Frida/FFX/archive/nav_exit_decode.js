// @python
// @desc Exit decoder — disassemble the exit-handler script to find trigger zones
//
// Decodes Worker 23 / entry point 1 (the exit handler) from the current map's
// Atel bytecode. Resolves all PUSHF references to actual coordinate values and
// extracts the position comparisons that define exit trigger zones.
//
// Keyboard:
//   s = Decode exit handler script
//   f = Dump full float table
//   q = Quit

var logPath = 'D:/Games/Dev/Custom/FFX/ffx-x2-screen-reader/Docs/Scripts/Frida/output/FFX/nav_exit_decode.log';
var logHandle = new File(logPath, 'w');
function logFile(msg) { logHandle.write(msg + '\n'); logHandle.flush(); }
function logBoth(msg) { console.log(msg); logFile(msg); }

var base = Process.enumerateModules()[0].base;
function rva(offset) { return base.add(offset); }

logBoth('[*] nav_exit_decode — Exit handler disassembler');

function hasOperand(op) { return (op & 0x80) !== 0; }

var OP_NAMES = {
    0x00:'NOP', 0x01:'LOR', 0x02:'LAND', 0x03:'OR', 0x04:'EOR', 0x05:'AND',
    0x06:'EQ', 0x07:'NE', 0x08:'GTU', 0x09:'LSU', 0x0A:'GT', 0x0B:'LS',
    0x0C:'GTEU', 0x0D:'LSEU', 0x0E:'GTE', 0x0F:'LSE', 0x10:'BON', 0x11:'BOFF',
    0x12:'SLL', 0x13:'SRL', 0x14:'ADD', 0x15:'SUB', 0x16:'MUL', 0x17:'DIV',
    0x18:'MOD', 0x19:'NOT', 0x1A:'UMINUS', 0x1B:'FIXADRS', 0x1C:'BNOT',
    0x25:'POPA', 0x26:'PUSHA', 0x28:'PUSHX', 0x29:'PUSHY', 0x2A:'POPX',
    0x2B:'REPUSH', 0x2C:'POPY', 0x34:'RTS', 0x36:'REQ', 0x37:'REQSW',
    0x38:'REQEW', 0x39:'PREQ', 0x3A:'PREQSW', 0x3B:'PREQEW',
    0x3C:'RET', 0x3D:'RETN', 0x3E:'RETT', 0x3F:'RETTN', 0x40:'HALT',
    0x45:'FREQ', 0x46:'TREQ', 0x47:'BREQ', 0x48:'BFREQ', 0x49:'BTREQ',
    0x4A:'FREQSW', 0x4B:'TREQSW', 0x4C:'BREQSW', 0x4D:'BFREQSW',
    0x4E:'BTREQSW', 0x4F:'FREQEW', 0x50:'TREQEW', 0x51:'BREQEW',
    0x52:'BFREQEW', 0x53:'BTREQEW', 0x54:'DRET',
    0x59:'POPI0', 0x5A:'POPI1', 0x5B:'POPI2', 0x5C:'POPI3',
    0x5D:'POPF0', 0x5E:'POPF1', 0x5F:'POPF2', 0x60:'POPF3',
    0x61:'POPF4', 0x62:'POPF5', 0x63:'POPF6', 0x64:'POPF7',
    0x65:'POPF8', 0x66:'POPF9',
    0x67:'PUSHI0', 0x68:'PUSHI1', 0x69:'PUSHI2', 0x6A:'PUSHI3',
    0x6B:'PUSHF0', 0x6C:'PUSHF1', 0x6D:'PUSHF2', 0x6E:'PUSHF3',
    0x6F:'PUSHF4', 0x70:'PUSHF5', 0x71:'PUSHF6', 0x72:'PUSHF7',
    0x73:'PUSHF8', 0x74:'PUSHF9',
    0x77:'REQWAIT', 0x78:'PREQWAIT', 0x79:'REQCHG', 0x7A:'ACTREQ',
    0x9D:'LABEL', 0x9E:'TAG', 0x9F:'PUSHV', 0xA0:'POPV', 0xA1:'POPVL',
    0xA2:'PUSHAR', 0xA3:'POPAR', 0xA4:'POPARL', 0xA7:'PUSHARP',
    0xAD:'PUSHI', 0xAE:'PUSHII', 0xAF:'PUSHF',
    0xB0:'JMP', 0xB1:'CJMP', 0xB2:'NCJMP', 0xB3:'JSR', 0xB5:'CALL',
    0xC1:'PUSHN', 0xC2:'PUSHT', 0xC3:'PUSHVP', 0xC4:'PUSHFIX',
    0xD5:'POPXJMP', 0xD6:'POPXCJMP', 0xD7:'POPXNCJMP', 0xD8:'CALLPOPA',
    0xF5:'PUSHAINTER', 0xF6:'SYSTEM'
};

// Atel call namespace names
function callName(id) {
    var ns = id >> 12;
    var fn = id & 0xFFF;
    var nsNames = {0:'std', 1:'math', 4:'sg', 5:'ch', 6:'cam', 7:'btl', 8:'map', 9:'mnt', 0xB:'mov', 0xC:'dbg'};
    return (nsNames[ns] || '?') + ':' + fn;
}

function decode() {
    var worker = rva(0xF270A4).readPointer();
    if (worker.isNull() || worker.toUInt32() < 0x10000) { logBoth('[!] No worker'); return; }

    var chunk = worker.add(0x04).readPointer();
    if (chunk.isNull() || chunk.toUInt32() < 0x10000) { logBoth('[!] No chunk'); return; }

    var header = worker.readPointer();
    var codeLen = chunk.add(0x00).readU32();
    var offsetCode = chunk.add(0x30).readU32();
    var scriptNum = chunk.add(0x34).readU16();
    var room = rva(0xD2CA90).readU16();

    // Read float and int reference tables
    var floatValues = [], intValues = [];
    try {
        var floatTableOff = header.add(0x14).readU32();
        var intTableOff = header.add(0x10).readU32();
        var floatCount = header.add(0x0A).readU16();
        var intCount = header.add(0x08).readU16();

        for (var i = 0; i < Math.min(floatCount, 512); i++)
            floatValues.push(chunk.add(floatTableOff + i * 4).readFloat());
        for (var i = 0; i < Math.min(intCount, 512); i++)
            intValues.push(chunk.add(intTableOff + i * 4).readS32());
    } catch (e) { logFile('[!] Table read error: ' + e.message); }

    // Read entry points table
    var entryPointsOff = header.add(0x1C).readU32(); // offset_entry_points_table (guess)
    var entryPointCount = header.add(0x0E).readU16(); // entry_point_count (guess)

    logBoth('[DECODE] Room=' + room + ' scripts=' + scriptNum + ' entry_points=' + entryPointCount);
    logBoth('[DECODE] Float table: ' + floatValues.length + ' entries, Int table: ' + intValues.length + ' entries');

    // Read all entry point offsets
    var entryPoints = [];
    for (var i = 0; i < Math.min(entryPointCount, 100); i++) {
        var epOff = chunk.add(entryPointsOff + i * 4).readU32();
        entryPoints.push(epOff);
    }

    logFile('[DECODE] Entry points: ' + entryPoints.map(function(e) { return '0x' + e.toString(16); }).join(', '));

    // Read full bytecode
    var codeBase = chunk.add(offsetCode);
    var codeSize = codeLen - offsetCode;
    if (codeSize <= 0 || codeSize > 500000) { logBoth('[!] Bad code size'); return; }
    var code = new Uint8Array(codeBase.readByteArray(codeSize));

    // Decode the ENTIRE bytecode, collecting all position-related patterns
    // Focus on: PUSHF (float references) near comparison ops (GTE, LSE, GT, LS)
    // and near CALLPOPA 0x0002 (exit trigger)

    logBoth('[DECODE] Disassembling full bytecode (' + codeSize + ' bytes)...');

    var exitCalls = [];      // Offsets of CALLPOPA 0x0002
    var posChecks = [];      // Float comparisons that might be position checks
    var allPushFloats = {};  // All PUSHF references and their values

    var i = 0;
    var prevOps = []; // sliding window of recent operations

    while (i < codeSize) {
        var op = code[i];
        var opName = OP_NAMES[op] || ('0x' + op.toString(16));
        var operand = -1;
        var instrLen = 1;

        if (hasOperand(op) && i + 2 < codeSize) {
            operand = code[i + 1] | (code[i + 2] << 8);
            instrLen = 3;
        }

        var desc = opName;

        if (op === 0xAF && operand >= 0 && operand < floatValues.length) {
            // PUSHF — resolved
            desc = 'PUSHF[' + operand + ']=' + floatValues[operand].toFixed(2);
            allPushFloats[operand] = floatValues[operand];
        } else if (op === 0xAD && operand >= 0 && operand < intValues.length) {
            desc = 'PUSHI[' + operand + ']=' + intValues[operand];
        } else if (op === 0xAE) {
            var sv = operand > 0x7FFF ? operand - 0x10000 : operand;
            desc = 'PUSHII(' + sv + ')';
        } else if (op === 0xD8 || op === 0xB5) {
            desc = (op === 0xD8 ? 'CALLPOPA' : 'CALL') + ' ' + callName(operand) + ' (0x' + operand.toString(16) + ')';
            if (operand === 0x0002) {
                exitCalls.push(i);
            }
        } else if (op === 0xB0) {
            desc = 'JMP +0x' + operand.toString(16);
        } else if (op === 0xB1) {
            desc = 'CJMP +0x' + operand.toString(16);
        } else if (op === 0xB2) {
            desc = 'NCJMP +0x' + operand.toString(16);
        } else if (operand >= 0) {
            desc = opName + ' 0x' + operand.toString(16);
        }

        // Track comparison ops near float pushes (potential position checks)
        if (op >= 0x06 && op <= 0x0F) { // EQ, NE, GT, LS, GTE, LSE, etc.
            // Check if recent ops included a PUSHF with coordinate-like value
            for (var pi = Math.max(0, prevOps.length - 6); pi < prevOps.length; pi++) {
                var prev = prevOps[pi];
                if (prev.op === 0xAF && prev.operand >= 0 && prev.operand < floatValues.length) {
                    var fv = floatValues[prev.operand];
                    if (Math.abs(fv) > 0.5 && Math.abs(fv) < 5000) {
                        posChecks.push({
                            offset: i,
                            cmpOp: opName,
                            floatIdx: prev.operand,
                            floatVal: fv
                        });
                    }
                }
            }
        }

        logFile('+' + i.toString(16).padStart(5, '0') + ': ' + desc);

        prevOps.push({ offset: i, op: op, operand: operand, desc: desc });
        if (prevOps.length > 20) prevOps.shift();

        i += instrLen;
    }

    // Report
    logBoth('[EXITS] ' + exitCalls.length + ' CALLPOPA 0x0002 at offsets: ' +
        exitCalls.map(function(o) { return '0x' + o.toString(16); }).join(', '));

    logBoth('[POS-CHECKS] ' + posChecks.length + ' float comparisons found:');
    // Group by float value to find clusters
    var byValue = {};
    for (var pi = 0; pi < posChecks.length; pi++) {
        var pc = posChecks[pi];
        var key = pc.floatVal.toFixed(1);
        if (!byValue[key]) byValue[key] = [];
        byValue[key].push(pc);
    }

    var sortedValues = Object.keys(byValue).map(Number).sort(function(a, b) { return a - b; });
    for (var vi = 0; vi < sortedValues.length; vi++) {
        var v = sortedValues[vi].toFixed(1);
        var checks = byValue[v];
        logBoth('  ' + v + ' — ' + checks.length + 'x (' + checks.map(function(c) { return c.cmpOp; }).join(', ') + ')');
    }

    // Also show all unique coordinate-like floats in the table
    logBoth('[COORDS] Coordinate-like floats in table:');
    var coordFloats = [];
    for (var fi = 0; fi < floatValues.length; fi++) {
        var fv = floatValues[fi];
        if (Math.abs(fv) > 0.5 && Math.abs(fv) < 5000 && !isNaN(fv) && isFinite(fv)) {
            coordFloats.push({ idx: fi, val: fv });
        }
    }
    coordFloats.sort(function(a, b) { return a.val - b.val; });
    for (var ci = 0; ci < coordFloats.length; ci++) {
        logBoth('  f[' + coordFloats[ci].idx + '] = ' + coordFloats[ci].val.toFixed(2));
    }
}

function dumpFloatTable() {
    var worker = rva(0xF270A4).readPointer();
    if (worker.isNull()) { logBoth('[!] No worker'); return; }
    var chunk = worker.add(0x04).readPointer();
    var header = worker.readPointer();

    try {
        var floatTableOff = header.add(0x14).readU32();
        var floatCount = header.add(0x0A).readU16();
        logBoth('[FLOATS] ' + floatCount + ' entries:');
        for (var i = 0; i < Math.min(floatCount, 512); i++) {
            var fv = chunk.add(floatTableOff + i * 4).readFloat();
            logBoth('  f[' + i + '] = ' + fv);
        }
    } catch (e) { logBoth('[!] Error: ' + e.message); }
}

rpc.exports = {
    scan: function() { decode(); return 'ok'; },
    floats: function() { dumpFloatTable(); return 'ok'; }
};
