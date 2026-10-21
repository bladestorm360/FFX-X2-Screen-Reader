// @python
// @desc Exit scanner — find ALL map transitions in Atel bytecode
//
// Scans the current script chunk's bytecode for CALLPOPA 0x0002 (Common function 2)
// which triggers TkSetEventJump. Extracts target map IDs and spawn points from
// preceding PUSH instructions.
//
// Keyboard:
//   s = Scan all scripts for exit calls
//   d = Debug — show chunk layout and worker info
//   q = Quit

var logPath = 'D:/Games/Dev/Custom/FFX/ffx-x2-screen-reader/Docs/Scripts/Frida/output/FFX/nav_exit_scan.log';
var logHandle = new File(logPath, 'w');
function logFile(msg) { logHandle.write(msg + '\n'); logHandle.flush(); }
function logBoth(msg) { console.log(msg); logFile(msg); }

var base = Process.enumerateModules()[0].base;
function rva(offset) { return base.add(offset); }

logBoth('[*] nav_exit_scan — Bytecode exit scanner');

// Atel opcodes we care about
var OP_PUSHII = 0xAE;  // Push immediate int (2-byte operand)
var OP_PUSHF  = 0xAF;  // Push immediate float (2-byte operand = index into float table)
var OP_PUSHI  = 0xAD;  // Push immediate int (2-byte operand = index into int table)
var OP_CALLPOPA = 0xD8; // Call and pop accumulator (2-byte operand = function ID)
var OP_CALL   = 0xB5;   // Call (2-byte operand = function ID)
var OP_RET    = 0x3C;
var OP_RETN   = 0x3D;

function hasOperand(op) { return (op & 0x80) !== 0; }

function getScriptChunk() {
    try {
        var wp = rva(0xF270A4).readPointer();
        if (wp.isNull() || wp.toUInt32() < 0x10000) return null;
        var chunk = wp.add(0x04).readPointer();
        if (chunk.isNull() || chunk.toUInt32() < 0x10000) return null;
        return chunk;
    } catch (e) { return null; }
}

function scanExits() {
    var chunk = getScriptChunk();
    if (!chunk) { logBoth('[!] No script chunk'); return; }

    var codeLen = chunk.add(0x00).readU32();
    var offsetCode = chunk.add(0x30).readU32();
    var scriptNum = chunk.add(0x34).readU16();
    var room = rva(0xD2CA90).readU16();

    var codeBase = chunk.add(offsetCode);
    var codeSize = codeLen - offsetCode;
    if (codeSize <= 0 || codeSize > 500000) {
        logBoth('[!] Invalid code size: ' + codeSize);
        return;
    }

    logBoth('[SCAN] Room=' + room + ' | ' + scriptNum + ' scripts, code size=' + codeSize + ' bytes');
    logBoth('[SCAN] Scanning bytecode for CALLPOPA 0x0002 (EventJump) and CALL patterns...');

    // Read ALL bytecode into a JS array for fast scanning
    var code = new Uint8Array(codeBase.readByteArray(codeSize));

    // Read ALL script headers to resolve PUSHF/PUSHI references
    // Worker has script_header at +0x00, and we can access table_float via the header
    var worker = rva(0xF270A4).readPointer();
    var scriptHeaders = [];
    var floatValues = []; // resolved float table
    var intValues = [];   // resolved int table
    try {
        if (worker && !worker.isNull()) {
            var header = worker.readPointer(); // worker->script_header
            if (header && !header.isNull() && header.toUInt32() > 0x10000) {
                // AtelScriptHeader offsets (from Fahrenheit script_header.cs)
                var floatTableOff = header.add(0x14).readU32(); // offset_float_table
                var intTableOff = header.add(0x10).readU32();   // offset_int_table
                var floatCount = header.add(0x0A).readU16();    // ref_float_count
                var intCount = header.add(0x08).readU16();      // ref_int_count

                logBoth('[TABLES] Float table: ' + floatCount + ' entries at header+0x' + floatTableOff.toString(16));
                logBoth('[TABLES] Int table: ' + intCount + ' entries at header+0x' + intTableOff.toString(16));

                // Float table: each entry is a 4-byte float, offset from chunk start
                for (var fi = 0; fi < Math.min(floatCount, 256); fi++) {
                    var fval = chunk.add(floatTableOff + fi * 4).readFloat();
                    floatValues.push(fval);
                }
                // Int table: each entry is a 4-byte int
                for (var ii = 0; ii < Math.min(intCount, 256); ii++) {
                    var ival = chunk.add(intTableOff + ii * 4).readS32();
                    intValues.push(ival);
                }

                // Dump all float values (they may contain trigger coordinates)
                logBoth('[FLOATS] All ' + floatValues.length + ' float constants:');
                for (var fi = 0; fi < floatValues.length; fi++) {
                    var fv = floatValues[fi];
                    logFile('  f[' + fi + '] = ' + fv.toFixed(4));
                    // Print coordinate-like values to console
                    if (Math.abs(fv) > 0.1 && Math.abs(fv) < 5000) {
                        logBoth('  f[' + fi + '] = ' + fv.toFixed(2));
                    }
                }
            }
        }
    } catch (e) {
        logFile('[TABLES] Error reading tables: ' + e.message);
    }

    // Scan for CALLPOPA 0x0002 and CALL to various IDs
    var exits = [];
    var allCalls = {};

    var i = 0;
    while (i < codeSize) {
        var op = code[i];

        if (op === OP_CALLPOPA || op === OP_CALL) {
            if (i + 2 < codeSize) {
                var operand = code[i + 1] | (code[i + 2] << 8);

                // Track all unique CALL targets
                var key = (op === OP_CALLPOPA ? 'CALLPOPA' : 'CALL') + ' 0x' + operand.toString(16);
                allCalls[key] = (allCalls[key] || 0) + 1;

                // Look for EventJump patterns: CALLPOPA 0x0002 or CALL 0x0002
                if (operand === 0x0002) {
                    // Found a potential exit! Read preceding instructions to find pushed params
                    var params = extractPrecedingPushes(code, i);
                    // Decode 60 bytes before the call with resolved float/int references
                    var decoded = [];
                    var pos = Math.max(0, i - 60);
                    while (pos < i + 3) {
                        var opc = code[pos];
                        if (hasOperand(opc) && pos + 2 < code.length) {
                            var opr = code[pos + 1] | (code[pos + 2] << 8);
                            var desc = '0x' + opc.toString(16) + ' ' + (opc < 16 ? '0' : '') + opc.toString(16);
                            if (opc === OP_PUSHF && opr < floatValues.length) {
                                desc = 'PUSHF[' + opr + ']=' + floatValues[opr].toFixed(2);
                            } else if (opc === OP_PUSHI && opr < intValues.length) {
                                desc = 'PUSHI[' + opr + ']=' + intValues[opr];
                            } else if (opc === OP_PUSHII) {
                                var sv = opr > 0x7FFF ? opr - 0x10000 : opr;
                                desc = 'PUSHII ' + sv;
                            } else if (opc === OP_CALLPOPA) {
                                desc = 'CALLPOPA 0x' + opr.toString(16);
                            } else if (opc === OP_CALL) {
                                desc = 'CALL 0x' + opr.toString(16);
                            } else {
                                desc = 'op(0x' + opc.toString(16) + ' 0x' + opr.toString(16) + ')';
                            }
                            decoded.push(desc);
                            pos += 3;
                        } else {
                            var names = {0x3C:'RET',0x3D:'RETN',0x25:'POPA',0x26:'PUSHA',0x0E:'GTE',0x0F:'LSE',
                                0x0A:'GT',0x0B:'LS',0x06:'EQ',0x07:'NE',0x14:'ADD',0x15:'SUB',
                                0x28:'PUSHX',0x29:'PUSHY',0x2A:'POPX',0x2C:'POPY',0x2B:'REPUSH'};
                            var n = names[opc] || 'op(0x' + opc.toString(16) + ')';
                            // Register pushes
                            if (opc >= 0x67 && opc <= 0x6A) n = 'PUSHI' + (opc - 0x67);
                            if (opc >= 0x6B && opc <= 0x74) n = 'PUSHF' + (opc - 0x6B);
                            decoded.push(n);
                            pos += 1;
                        }
                    }
                    exits.push({
                        offset: i,
                        opcode: op === OP_CALLPOPA ? 'CALLPOPA' : 'CALL',
                        funcId: operand,
                        params: params,
                        decoded: decoded
                    });
                }

                // Also check for other potentially relevant functions
                // SgEvent namespace = 0x4000+
                if (operand >= 0x4000 && operand < 0x4100) {
                    // SgEvent call — might be transition-related
                }

                i += 3;
                continue;
            }
        }

        // Skip instruction
        i += hasOperand(op) ? 3 : 1;
    }

    // Report all unique CALL targets (to log only)
    logFile('[CALLS] All unique CALL/CALLPOPA targets in bytecode:');
    var callKeys = Object.keys(allCalls).sort();
    for (var ci = 0; ci < callKeys.length; ci++) {
        logFile('  ' + callKeys[ci] + ' — ' + allCalls[callKeys[ci]] + ' occurrences');
    }

    // Report exits
    if (exits.length === 0) {
        logBoth('[SCAN] No CALLPOPA/CALL 0x0002 found in bytecode');
        logBoth('[SCAN] Checking for other Common function calls that might be transitions...');

        // List most frequent Common namespace calls
        var commonCalls = [];
        for (var ci = 0; ci < callKeys.length; ci++) {
            var match = callKeys[ci].match(/0x([0-9a-f]+)/);
            if (match) {
                var id = parseInt(match[1], 16);
                if (id < 0x1000) { // Common namespace
                    commonCalls.push({ key: callKeys[ci], count: allCalls[callKeys[ci]], id: id });
                }
            }
        }
        commonCalls.sort(function(a, b) { return b.count - a.count; });
        for (var ci = 0; ci < Math.min(commonCalls.length, 15); ci++) {
            logBoth('  ' + commonCalls[ci].key + ' x' + commonCalls[ci].count);
        }
    } else {
        logBoth('[EXITS] Found ' + exits.length + ' EventJump calls:');
        for (var ei = 0; ei < exits.length; ei++) {
            var ex = exits[ei];
            var paramStr = '';
            for (var pi = 0; pi < ex.params.length; pi++) {
                var p = ex.params[pi];
                paramStr += p.type + '=' + (p.type === 'int' ? '0x' + p.value.toString(16) + ' (' + p.value + ')' : p.value.toFixed(2));
                if (pi < ex.params.length - 1) paramStr += ', ';
            }
            logBoth('  [' + (ei + 1) + '] at code+0x' + ex.offset.toString(16) + ': ' + ex.opcode + ' 0x0002 | params: ' + (paramStr || 'none found'));
            if (ex.decoded) {
                logBoth('  decoded: ' + ex.decoded.join(' | '));
            }
        }
    }
}

// Walk backward from a CALL instruction to find preceding PUSH values
function extractPrecedingPushes(code, callOffset) {
    var params = [];
    var pos = callOffset - 1;
    var maxLookback = 30; // Don't look back more than 30 bytes

    while (pos >= 0 && (callOffset - pos) < maxLookback && params.length < 5) {
        var op = code[pos];

        if (op === OP_PUSHII && pos + 2 < code.length) {
            var val = code[pos + 1] | (code[pos + 2] << 8);
            // Sign extend 16-bit
            if (val > 0x7FFF) val = val - 0x10000;
            params.unshift({ type: 'int', value: val });
            pos -= 3; // PUSHII is 3 bytes, move to previous instruction
            continue;
        }

        if (op === OP_PUSHI && pos + 2 < code.length) {
            var idx = code[pos + 1] | (code[pos + 2] << 8);
            params.unshift({ type: 'int_ref', value: idx });
            pos -= 3;
            continue;
        }

        if (op === OP_PUSHF && pos + 2 < code.length) {
            var idx = code[pos + 1] | (code[pos + 2] << 8);
            params.unshift({ type: 'float_ref', value: idx });
            pos -= 3;
            continue;
        }

        // Check for 1-byte push opcodes (PUSHI0-PUSHI3 = 0x67-0x6A)
        if (op >= 0x67 && op <= 0x6A) {
            params.unshift({ type: 'int_reg', value: op - 0x67 });
            pos -= 1;
            continue;
        }

        // PUSHF0-PUSHF9 = 0x6B-0x74
        if (op >= 0x6B && op <= 0x74) {
            params.unshift({ type: 'float_reg', value: op - 0x6B });
            pos -= 1;
            continue;
        }

        // Hit a non-PUSH instruction — stop looking
        break;
    }

    return params;
}

function showDebug() {
    var chunk = getScriptChunk();
    if (!chunk) { logBoth('[!] No script chunk'); return; }

    var codeLen = chunk.add(0x00).readU32();
    var mapStart = chunk.add(0x04).readU32();
    var offsetAuthor = chunk.add(0x08).readU32();
    var offsetArea = chunk.add(0x28).readU32();
    var offsetCode = chunk.add(0x30).readU32();
    var scriptNum = chunk.add(0x34).readU16();
    var zoneBytes = chunk.add(0x1E).readU16();
    var room = rva(0xD2CA90).readU16();

    logBoth('[DEBUG] Room=' + room + ' chunk=' + chunk);
    logBoth('[DEBUG] code_length=0x' + codeLen.toString(16) +
        ' scripts=' + scriptNum +
        ' code_offset=0x' + offsetCode.toString(16) +
        ' area_offset=0x' + offsetArea.toString(16) +
        ' zone_bytes=0x' + zoneBytes.toString(16));

    var entranceCount = Math.floor((offsetAuthor - mapStart) / 0x20);
    logBoth('[DEBUG] ' + entranceCount + ' map entrances');
    for (var i = 0; i < entranceCount; i++) {
        var addr = chunk.add(mapStart + i * 0x20);
        var unk04 = addr.add(0x04).readS16();
        var x = addr.add(0x0C).readFloat();
        var y = addr.add(0x10).readFloat();
        var z = addr.add(0x14).readFloat();
        logBoth('  [' + i + '] unk04=' + unk04 + ' pos=(' + x.toFixed(1) + ',' + y.toFixed(1) + ',' + z.toFixed(1) + ')');
    }
}

rpc.exports = {
    scan: function() { scanExits(); return 'ok'; },
    debug: function() { showDebug(); return 'ok'; }
};
