// Offline and runtime ATEL analysis. Layout/opcodes verified against
// Fahrenheit.Atel and Karifean's FFXDataParser (see gameArchitecture.md).
// A jump operand indexes the CURRENT worker's jump table; JSR names a
// subroutine worker. Literal arguments belong to one uninterrupted path.

const WARP_CALLS = { 0x0011: true, 0x010B: true };
const REQ_OPS = new Set([
    0x36, 0x37, 0x38, 0x39, 0x3A, 0x3B,
    0x45, 0x46, 0x47, 0x48, 0x49, 0x4A, 0x4B,
    0x4C, 0x4D, 0x4E, 0x4F, 0x50, 0x51, 0x52, 0x53,
    0x77, 0x78, 0x79
]);
const RETURNS = new Set([0x34, 0x3C, 0x3D, 0x3E, 0x3F, 0x40, 0x54]);

export function decodeInstructions(code) {
    const instructions = new Map();
    for (let pc = 0; pc < code.length;) {
        const opcode = code[pc];
        const size = (opcode & 0x80) ? 3 : 1;
        if (pc + size > code.length) throw new Error('Truncated ATEL instruction');
        const operand = size === 3 ? code[pc + 1] | (code[pc + 2] << 8) : null;
        instructions.set(pc, { opcode, operand, next: pc + size });
        pc += size;
    }
    return instructions;
}

// Conservative static analysis: unresolved data and conditional destinations
// remain unresolved. No first-call-wins guess and no gameplay learning.
export function scanWorker(script, index, entryIndices, onCall) {
    const result = { isExit: false, hasReq: false, exitTargetMapId: null,
        exitTargetSpawnpoint: null, warps: [], calls: [], reqs: [],
        reqTargets: [], bytes: script.code.length, entries: 0, steps: 0,
        truncated: false, unresolved: false };
    const instructions = script.instructions || decodeInstructions(script.code);
    const root = script.workers[index];
    if (!root) return result;
    const entries = entryIndices || root.entries.map(function(_, i) { return i; });
    const pending = [];
    result.entries = entries.length;
    const visited = new Set();
    const limit=script.scanLimit===undefined?200000:script.scanLimit;
    const warpSites = new Set();
    function enqueue(worker, pc) {
        if (instructions.has(pc)) pending.push({ worker, pc, literals: [] });
        else result.unresolved = true;
    }
    entries.forEach(function(i) { enqueue(index, root.entries[i]); });
    while (pending.length && result.steps < limit) {
        const path = pending.pop();
        const worker = script.workers[path.worker];
        let pc = path.pc;
        let literals = path.literals;
        while (instructions.has(pc) && result.steps < limit) {
            // Include the argument suffix: distinct paths can meet at a CALL.
            const key = path.worker + ':' + pc + ':' + literals.join(',');
            if (visited.has(key)) break;
            visited.add(key);
            result.steps++;
            const ins = instructions.get(pc);
            const op = ins.opcode, arg = ins.operand;
            if (op === 0xAE || op === 0xAD) {
                const value = op === 0xAE ? (arg << 16 >> 16) : worker.ints[arg];
                literals.push(value === undefined ? null : value);
                if (literals.length > 8) literals.shift();
            } else if (op === 0xB5 || op === 0xD8) {
                if (onCall) onCall({ worker: path.worker, offset: pc, call: arg, literals: literals.slice() });
                if (result.calls.indexOf(arg) < 0) result.calls.push(arg);
                if (WARP_CALLS[arg]) {
                    result.isExit = true;
                    const room = literals.length >= 2 ? literals[literals.length - 2] : null;
                    const spawn = literals.length >= 2 ? literals[literals.length - 1] : null;
                    const siteKey = path.worker + ':' + pc + ':' + room + ':' + spawn;
                    if (!warpSites.has(siteKey)) {
                        warpSites.add(siteKey);
                        result.warps.push({ worker: path.worker, offset: pc, call: arg, room, spawn });
                    }
                }
                // Any call consumes its own arguments. Unknown return values
                // must never inherit literals belonging to an earlier call.
                literals = [];
            } else if (op === 0xB3) {
                // JSR targets entry zero of another worker, not code[arg].
                const callee = script.workers[arg];
                if (callee && callee.entries.length) enqueue(arg, callee.entries[0]);
                else result.unresolved = true;
                literals = [];
            } else if (op === 0xB0 || op === 0xD5 || op === 0xB1 ||
                       op === 0xB2 || op === 0xD6 || op === 0xD7) {
                enqueue(path.worker, worker.jumps[arg]);
                // Condition evaluation may consume stack values; do not use
                // constants from the condition as the following call's args.
                literals = [];
                if (op === 0xB0 || op === 0xD5) break;
            } else if (RETURNS.has(op)) {
                break;
            } else if (REQ_OPS.has(op)) {
                result.hasReq = true;
                if (result.reqs.indexOf(op) < 0) result.reqs.push(op);
                if (literals.length >= 3) {
                    const target = literals[literals.length - 2];
                    const fn = literals[literals.length - 1];
                    if (target !== null && fn !== null) result.reqTargets.push({ worker: target, fn });
                }
                literals = [];
            } else if (op !== 0x00) {
                // Computed/variable arguments are deliberately unresolved.
                literals = [];
            }
            pc = ins.next;
        }
    }
    result.truncated = pending.length > 0 || result.steps >= limit;
    const known = result.warps.filter(function(w) { return w.room !== null && w.spawn !== null && w.room >= 0; });
    const rooms = new Set(known.map(function(w) { return w.room; }));
    if (!result.truncated && !result.unresolved && known.length === result.warps.length && rooms.size === 1) {
        result.exitTargetMapId = known[0].room;
        const spawns = new Set(known.map(function(w) { return w.spawn; }));
        if (spawns.size === 1) result.exitTargetSpawnpoint = known[0].spawn;
    }
    return result;
}
