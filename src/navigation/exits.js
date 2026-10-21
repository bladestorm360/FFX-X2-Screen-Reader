// Exit detection from worker-local ATEL control flow. Room IDs and area
// name indices are separate domains; see the 2026-10-03 architecture audit.

import { rva } from '../core/memory.js';
import { logFile } from '../core/speech.js';
import { RVA_ATEL_CONTROLLER } from '../core/addresses.js';
import { isValidPtr } from '../core/safeRead.js';
import { distXZ } from './navmesh.js';
import { scanWorker } from './atel.js';
import { readScript } from './atelMemory.js';
import { ROOM_AREA_INDEX } from './roomAreas.js';

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

// Native area-name getter, also used by the save/load screen (RVA 0x2F0DA0).
// FUN_0086c3c0 expects an AREA INDEX, not a room ID. Out-of-range indexes
// are clamped to entry zero. Translate verified room data first, then
// use the game's localized getter and FUN_006460e0 text decoder.
const fnMapNameEncoded = new NativeFunction(rva(0x46C3C0), 'pointer', ['uint32', 'uint32']);
const fnAreaTable = new NativeFunction(rva(0x4B7F10), 'pointer', ['uint32']);
const fnDecodeFhText = new NativeFunction(rva(0x2460E0), 'void', ['pointer', 'pointer']);
const mapNameBuf = Memory.alloc(256);
const mapNameZero = new Uint8Array(256);
const mapNameCache = {};

function resolveMapName(roomId) {
    const mapId = ROOM_AREA_INDEX[roomId];
    if (mapId === undefined) {
        logFile('[NAV] room=' + roomId + ' has no verified single-area mapping');
        return null;
    }
    if (mapNameCache.hasOwnProperty(mapId)) return mapNameCache[mapId];
    let result = null;
    let failReason = null;
    try {
        const table = fnAreaTable(0);
        if (!isValidPtr(table) || mapId >= (table.readU16() >> 2)) {
            logFile('[NAV] Area table not ready or index out of range: ' + mapId);
            return null;
        }
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
    if (result !== null) mapNameCache[mapId] = result;
    return result;
}

// Cache JS-owned bytecode only for this scan. The root is matched by its
// actual header offset, rather than assuming runtime worker order equals
// the source header-table order.
function workerScanBytecode(worker, scripts) {
    const chunk = worker.add(4).readPointer();
    const key = chunk.toString();
    if (!scripts.has(key)) scripts.set(key, readScript(chunk));
    const script = scripts.get(key);
    const headerOffset = worker.readPointer().sub(chunk).toInt32();
    const index = script.workers.findIndex(w => w.headerOffset === headerOffset);
    if (index < 0) throw new Error('ATEL worker header absent from chunk');
    return scanWorker(script, index);
}

// Side-effect: appends Exit/Event entries to the entities array.
export function scanExits(playerPos, entities) {
    try {
        const controllerPtr = rva(RVA_ATEL_CONTROLLER).readPointer();
        if (!isValidPtr(controllerPtr)) return;
        const actorCount = controllerPtr.add(0x0C).readU16();
        if (actorCount === 0 || actorCount > 500) return;

        const scripts = new Map();

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
                    const altScan = workerScanBytecode(worker, scripts);
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

                const scan = workerScanBytecode(worker, scripts);
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
                            exitName = 'Exit ' + exitCount;
                        }
                    }
                    logFile('[NAV] Exit ' + exitCount + ' at (' +
                        pos.x.toFixed(1) + ',' + pos.z.toFixed(1) + ') ' +
                        'room=' + (tid === null ? 'null' : '0x' + tid.toString(16)) +
                        ' spawn=' + (spawn === null ? 'null' : '0x' + spawn.toString(16)) +
                        ' warps=' + JSON.stringify(scan.warps) +
                        ' unresolved=' + scan.unresolved);
                    entities.push({ name: exitName, category: 'Exit',
                        chrId: 0xFFFF, workerIdx: i, pos: pos, dist: distXZ(playerPos, pos) });
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
                        chrId: 0xFFFE, workerIdx: i, pos: pos, dist: distXZ(playerPos, pos) });
                }
            } catch (e) { logFile('[NAV] Worker ' + i + ' scan failed: ' + e.message); }
        }
        const typeSummary = Object.keys(typeCount).map(function(k){return 'type'+k+'='+typeCount[k];}).join(',');
        logFile('[NAV] Worker types in controller: ' + typeSummary);
        logFile('[NAV] Player pos at scan: (' + playerPos.x.toFixed(1) + ',' + playerPos.z.toFixed(1) + ')');
        logFile('[NAV] Triggers: scanned ' + scanned + ', classified ' + exitCount +
            ' exits, ' + eventCount + ' events');
    } catch (e) { logFile('[NAV] Exit scan error: ' + e.message); }
}
