// Exit detection: scans each Atel worker's bytecode for the two script-level
// room-transition CALLs (`0x0011 transitionToRoom?` and `0x010B warpToRoom?`,
// both in the Common namespace per FFXDataParser). Workers that issue either
// call are exits; workers that use REQ-family opcodes without a warp CALL
// are events; the rest are ambient. The destination map ID is the FIRST of
// the two PUSHII literals that precede the CALL (room_id, then spawnpoint_id).

import { rva } from '../core/memory.js';
import { logFile } from '../core/speech.js';
import { RVA_ATEL_CONTROLLER } from '../core/addresses.js';
import { isValidPtr } from '../core/safeRead.js';
import { distXZ } from './navmesh.js';

// Resolved once at module load — constructing NativeFunction per call is
// wasteful and was happening inside scanExits on every invocation.
const fnGetWorker = new NativeFunction(rva(0x46A7C0), 'pointer', ['int']);

// Diagnostic hook on AtelSetEventJump2 — the native dispatcher every warp
// eventually reaches (gameArchitecture.md:1513-1518). Logs the ACTUAL
// (room, entrance, do_fade) args whenever a real warp fires. Used to
// validate whether the bytecode-walker's captured exitTargetMapId matches
// what the game actually passes when the player walks through an exit.
try {
    Interceptor.attach(rva(0x46FED0), {
        onEnter: function(args) {
            try {
                const room = args[0].toInt32();
                const entrance = args[1].toInt32();
                const doFade = args[2].toInt32();
                logFile('[WARP] AtelSetEventJump2(room=0x' + room.toString(16) +
                    ', entrance=0x' + entrance.toString(16) +
                    ', do_fade=' + doFade + ')');
            } catch (e) {
                logFile('[WARP] hook onEnter threw: ' + e.message);
            }
        }
    });
    logFile('[NAV] AtelSetEventJump2 diagnostic hook installed at RVA 0x46FED0');
} catch (e) {
    logFile('[NAV] AtelSetEventJump2 hook install FAILED: ' + e.message);
}

// Sanity upper bound for excel-bin-style offset reads inside bytecode scan.
const CODE_OFFSET_MAX = 0x100000;

// Map-name resolver. The game itself resolves map IDs to area names via two
// chained calls used by the save/load screen (RVA 0x6F0DA0 around line
// 183776 of the layer2 decompile):
//   encoded = FUN_0086c3c0(mapId, 0)  -> ptr into the engine's encoded
//                                         string table for the area name
//   FUN_006460e0(encoded, outBuf)      -> decodes FFX's custom encoding
//                                         (FhEncoding) into a plain C-string
// We call the same pair from JS. The save/load module is not touched — it
// observes args[2] of FUN_00702E70 unchanged. We just reuse the resolver.
const fnMapNameEncoded = new NativeFunction(rva(0x46C3C0), 'pointer', ['uint32', 'uint32']);
const fnDecodeFhText = new NativeFunction(rva(0x2460E0), 'void', ['pointer', 'pointer']);
const mapNameBuf = Memory.alloc(256);
const mapNameZero = new Uint8Array(256);
const mapNameCache = {};

function resolveMapName(mapId) {
    if (mapId === null || mapId === undefined) return null;
    if (mapNameCache.hasOwnProperty(mapId)) return mapNameCache[mapId];
    let result = null;
    let failReason = null;
    try {
        const encoded = fnMapNameEncoded(mapId, 0);
        if (!isValidPtr(encoded)) {
            failReason = 'null encoded ptr';
        } else {
            mapNameBuf.writeByteArray(mapNameZero);
            fnDecodeFhText(encoded, mapNameBuf);
            // The decoder writes the area name as plain ASCII into the
            // buffer but does NOT always emit a 0x00 terminator after it
            // — formatting/control bytes (>=0x80 or <0x20) may follow
            // directly. Stop at the first non-printable byte instead of
            // relying on readCString to find a terminator.
            let len = 0;
            while (len < 255) {
                const b = mapNameBuf.add(len).readU8();
                if (b < 0x20 || b > 0x7E) break;
                len++;
            }
            if (len >= 2) {
                let s = '';
                for (let i = 0; i < len; i++) {
                    s += String.fromCharCode(mapNameBuf.add(i).readU8());
                }
                result = s.trim();
            } else {
                failReason = 'short ASCII prefix (len=' + len + ')';
            }
        }
    } catch (e) {
        failReason = 'threw: ' + e.message;
    }
    if (result === null) {
        logFile('[NAV] resolveMapName(0x' + mapId.toString(16) + ') -> null (' + failReason + ')');
    }
    mapNameCache[mapId] = result;
    return result;
}

// One-shot diagnostic: enumerate the area-name table for mapId 0x000..0x150
// to see (a) the table's actual span and (b) where the area name a given
// room_id belongs to actually lives. Cannot run at module load because the
// game's area-name table is not yet initialized then (fnMapNameEncoded
// null-derefs at offset 0x2c). Deferred to first successful scanExits run.
let tableEnumerated = false;
function maybeEnumerateMapNameTable() {
    if (tableEnumerated) return;
    tableEnumerated = true;
    try {
        logFile('[TABLE] Area-name table enumeration begin (tableSel=0)');
        for (let i = 0; i <= 0x150; i++) {
            const name = resolveMapName(i);
            if (name) {
                logFile('[TABLE 0x' + i.toString(16) + '] ' + name);
            }
        }
        logFile('[TABLE] enumeration complete');
        for (const k in mapNameCache) { delete mapNameCache[k]; }
    } catch (e) {
        logFile('[TABLE] enumeration error: ' + e.message);
    }
}

// REQ family opcodes — 1-byte. Workers that use these coordinate with other
// workers (player, dialog, camera). Ambient effects don't; they just CALL and RET.
const REQ_OPCODES = {
    0x36: true, 0x37: true, 0x38: true,
    0x39: true, 0x3A: true, 0x3B: true,
    0x45: true, 0x46: true, 0x47: true, 0x48: true, 0x49: true,
    0x4A: true, 0x4B: true, 0x4C: true, 0x4D: true, 0x4E: true,
    0x4F: true, 0x50: true, 0x51: true, 0x52: true, 0x53: true,
    0x77: true, 0x78: true, 0x79: true
};

// Flow-following bytecode scan from each entry point. Returns classification
// hints: isExit if any CALL targets a warp primitive (0x0011 or 0x010B),
// hasReq if any REQ-family opcode is used, plus full call/req diagnostic lists.
function workerScanBytecode(worker) {
    const result = { isExit: false, hasReq: false, exitTargetMapId: null, exitTargetSpawnpoint: null, calls: [], reqs: [], reqTargets: [], bytes: 0, entries: 0, steps: 0, truncated: false };
    try {
        if (!isValidPtr(worker)) return result;
        const headerPtr = worker.readPointer();
        const chunkPtr  = worker.add(0x04).readPointer();
        if (!isValidPtr(headerPtr) || !isValidPtr(chunkPtr)) return result;

        const codeOffset = chunkPtr.add(0x30).readU32();
        const codeLength = chunkPtr.readU32();
        if (codeLength === 0 || codeLength > 0x100000) return result;
        if (codeOffset === 0 || codeOffset > CODE_OFFSET_MAX) return result;
        const codeBase = chunkPtr.add(codeOffset);

        const entryCount = headerPtr.add(0x08).readU16();
        if (entryCount === 0 || entryCount > 256) return result;
        const entryTableOffset = headerPtr.add(0x20).readU32();
        if (entryTableOffset > CODE_OFFSET_MAX) return result;
        const entryTable = chunkPtr.add(entryTableOffset);
        result.entries = entryCount;
        result.bytes = codeLength;

        const visited = {};
        const stack = [];
        for (let e = 0; e < entryCount; e++) {
            const ep = entryTable.add(e * 4).readU32();
            if (ep < codeLength) stack.push(ep);
        }

        let lastPushi1 = null, lastPushi2 = null;
        let lastPushii1 = null, lastPushii2 = null;
        let totalSteps = 0;
        const STEP_CAP = 200000;
        while (stack.length > 0 && totalSteps < STEP_CAP) {
            let pc = stack.pop();
            while (pc < codeLength && totalSteps < STEP_CAP) {
                totalSteps++;
                if (visited[pc]) break;
                visited[pc] = true;

                const opcode = codeBase.add(pc).readU8();

                if (REQ_OPCODES[opcode]) {
                    result.hasReq = true;
                    if (result.reqs.indexOf(opcode) < 0) result.reqs.push(opcode);
                    if (lastPushi1 !== null && lastPushi2 !== null) {
                        result.reqTargets.push({ worker: lastPushi2, fn: lastPushi1 });
                    }
                }

                if ((opcode & 0x80) !== 0) {
                    if (pc + 2 >= codeLength) break;
                    const operand = codeBase.add(pc + 1).readU16();
                    if (opcode === 0xB5 || opcode === 0xD8) {
                        // CALL / CALLPOPA. Warp primitives are Common[0x011]
                        // transitionToRoom? and Common[0x10B] warpToRoom?,
                        // both signature (room, spawnpoint:int). The two
                        // arguments are pushed as PUSHII (0xAE — operand IS
                        // the literal) in source order, so by the time we
                        // reach the CALL: lastPushii2 = room_id (older),
                        // lastPushii1 = spawnpoint_id (newer). First exit
                        // CALL wins.
                        if (operand === 0x0011 || operand === 0x010B) {
                            result.isExit = true;
                            if (result.exitTargetMapId === null && lastPushii2 !== null) {
                                result.exitTargetMapId = lastPushii2;
                                result.exitTargetSpawnpoint = lastPushii1;
                            }
                        }
                        if (result.calls.indexOf(operand) < 0) result.calls.push(operand);
                    }
                    if (opcode === 0xAD) {
                        // PUSHI: operand is an index into the chunk's int
                        // reference table — NOT the literal. Kept for REQ
                        // target diagnostics only.
                        lastPushi2 = lastPushi1;
                        lastPushi1 = operand;
                    }
                    if (opcode === 0xAE) {
                        // PUSHII: operand IS the literal value.
                        lastPushii2 = lastPushii1;
                        lastPushii1 = operand;
                    }
                    if (opcode === 0xB0 || opcode === 0xD5) { pc = operand; continue; }
                    if (opcode === 0xB1 || opcode === 0xB2 ||
                        opcode === 0xD6 || opcode === 0xD7) { stack.push(operand); }
                    if (opcode === 0xB3) { stack.push(operand); }
                    pc += 3;
                } else {
                    if (opcode === 0x34 || opcode === 0x3C || opcode === 0x3D ||
                        opcode === 0x3E || opcode === 0x3F || opcode === 0x40 ||
                        opcode === 0x54) break;
                    pc += 1;
                }
            }
            lastPushi1 = null;
            lastPushi2 = null;
            lastPushii1 = null;
            lastPushii2 = null;
        }
        result.steps = totalSteps;
        result.truncated = totalSteps >= STEP_CAP;
    } catch (e) {
        result.error = e.message;
    }
    return result;
}

// Side-effect: appends Exit/Event entries to the entities array.
export function scanExits(playerPos, entities) {
    try {
        const controllerPtr = rva(RVA_ATEL_CONTROLLER).readPointer();
        if (!isValidPtr(controllerPtr)) return;
        const actorCount = controllerPtr.add(0x0C).readU16();
        if (actorCount === 0 || actorCount > 500) return;

        maybeEnumerateMapNameTable();

        let scanned = 0, exitCount = 0, eventCount = 0;
        const typeCount = {};
        for (let i = 0; i < actorCount; i++) {
            try {
                const worker = fnGetWorker(i);
                if (!isValidPtr(worker)) continue;
                const firstDword = worker.readPointer();
                if (!isValidPtr(firstDword)) continue;
                const actorType = firstDword.readU8();
                typeCount[actorType] = (typeCount[actorType] || 0) + 1;

                // Filter by type BEFORE reading geometry — the type determines
                // the struct layout and a wrong offset can read past the worker
                // allocation. Unknown types get a bytecode-only scan, no
                // position. This filter exists because earlier sessions found
                // that opening the gate (accepting all types) reintroduces
                // event/object stacking and false-positive exits (e.g. the
                // player spawn worker on the salvage ship deck).
                const isKnownType = (actorType === 2 || actorType === 3 ||
                                     actorType === 5 || actorType === 6);

                if (!isKnownType) {
                    const altScan = workerScanBytecode(worker);
                    logFile('[WORKER-ALT ' + i + ' type=' + actorType +
                        ' isExit=' + altScan.isExit + ' hasReq=' + altScan.hasReq +
                        ' steps=' + altScan.steps + (altScan.truncated ? '(TRUNC)' : '') +
                        ' reqs=[' + altScan.reqs.map(function(o){return '0x'+o.toString(16);}).join(',') + ']' +
                        ' reqTargets=[' + altScan.reqTargets.slice(0, 20).map(function(t){return 'w'+t.worker+':fn0x'+t.fn.toString(16);}).join(',') +
                        (altScan.reqTargets.length > 20 ? ',...+' + (altScan.reqTargets.length-20) : '') + ']' +
                        ' calls=[' + altScan.calls.slice(0, 20).map(function(o){return '0x'+o.toString(16);}).join(',') + ']');
                    continue;
                }

                // Known type — safe to read geometry at the type-specific offset.
                const geomOffset = (actorType === 5 || actorType === 6) ? 0x284 : 0x558;
                const x = worker.add(geomOffset).readFloat();
                const y = worker.add(geomOffset + 4).readFloat();
                const z = worker.add(geomOffset + 8).readFloat();
                const hasValidPos = !isNaN(x) && !isNaN(z) &&
                    Math.abs(x) <= 50000 && Math.abs(z) <= 50000 &&
                    !(x === 0 && y === 0 && z === 0);

                scanned++;

                if (!hasValidPos) continue;

                const scan = workerScanBytecode(worker);
                const cls = scan.isExit ? 'EXIT' : (scan.hasReq ? 'EVENT' : 'AMBIENT');
                logFile('[WORKER ' + i + ' type=' + actorType + ' class=' + cls +
                    ' bytes=' + scan.bytes + ' entries=' + scan.entries +
                    ' steps=' + scan.steps + (scan.truncated ? '(TRUNC)' : '') +
                    ' pos=(' + x.toFixed(1) + ',' + z.toFixed(1) + ')' +
                    (scan.error ? ' err=' + scan.error : '') +
                    ' reqs=[' + scan.reqs.map(function(o){return '0x'+o.toString(16);}).join(',') + ']' +
                    ' reqTargets=[' + scan.reqTargets.slice(0, 20).map(function(t){return 'w'+t.worker+':fn0x'+t.fn.toString(16);}).join(',') +
                    (scan.reqTargets.length > 20 ? ',...+' + (scan.reqTargets.length-20) : '') + ']' +
                    ' calls=[' + scan.calls.slice(0, 40).map(function(o){return '0x'+o.toString(16);}).join(',') +
                    (scan.calls.length > 40 ? ',...+' + (scan.calls.length-40) : '') + ']');
                const pos = { x: x, y: y, z: z };
                if (scan.isExit) {
                    exitCount++;
                    const tid = scan.exitTargetMapId;
                    const spawn = scan.exitTargetSpawnpoint;
                    let exitName;
                    if (tid === null || tid === undefined) {
                        exitName = 'Exit ' + exitCount;
                    } else {
                        const resolved = resolveMapName(tid);
                        if (resolved) {
                            exitName = 'Exit to ' + resolved;
                        } else {
                            // Diagnostic speech: hex ID surfaces "we have a
                            // captured room_id but resolveMapName can't turn
                            // it into an area name" — the exact distinction
                            // the user needs to hear to know whether to
                            // investigate the classifier or the resolver.
                            exitName = 'Exit (unverified) to 0x' + tid.toString(16);
                        }
                    }
                    logFile('[NAV] Exit ' + exitCount + ' at (' +
                        pos.x.toFixed(1) + ',' + pos.z.toFixed(1) + ') ' +
                        'room=' + (tid === null ? 'null' : '0x' + tid.toString(16)) +
                        ' spawn=' + (spawn === null ? 'null' : '0x' + spawn.toString(16)));
                    entities.push({ name: exitName, category: 'Exit',
                        chrId: 0xFFFF, pos: pos, dist: distXZ(playerPos, pos) });
                } else if (scan.hasReq) {
                    eventCount++;
                    // Hint at what the event does using the first REQ target
                    // function ID. Distinguishes "Event 1" / "Event 2" when
                    // they sit at different positions on the same map but call
                    // different script functions.
                    let fnHint = '';
                    if (scan.reqTargets.length > 0) {
                        fnHint = ' fn 0x' + scan.reqTargets[0].fn.toString(16);
                    }
                    entities.push({ name: 'Event ' + eventCount + fnHint, category: 'Event',
                        chrId: 0xFFFE, pos: pos, dist: distXZ(playerPos, pos) });
                }
            } catch (e) {}
        }
        const typeSummary = Object.keys(typeCount).map(function(k){return 'type'+k+'='+typeCount[k];}).join(',');
        logFile('[NAV] Worker types in controller: ' + typeSummary);
        logFile('[NAV] Player pos at scan: (' + playerPos.x.toFixed(1) + ',' + playerPos.z.toFixed(1) + ')');
        logFile('[NAV] Triggers: scanned ' + scanned + ', classified ' + exitCount +
            ' exits, ' + eventCount + ' events');
    } catch (e) { logFile('[NAV] Exit scan error: ' + e.message); }
}
