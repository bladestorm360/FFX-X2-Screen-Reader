// Frida validation only. Loaded by validation_host.py, never by production.
// Native bodies: FUN_0086c270/c500, FUN_00871280/713a0, FUN_00877770,
// FUN_0086fed0 and FUN_009093b0 in the retained decompile archive.
// All native getters run inside the corresponding game-thread event callback.
// RPC returns copied scalars only. No speech producer, timers or state writes.
'use strict';
const manifest = /* CLOISTER_MANIFEST */ {};
const module = Process.enumerateModules().find(m => m.name.toLowerCase() === 'ffx.exe');
if (!module || Process.pointerSize !== 4) throw new Error('This prototype requires x86 FFX.exe');
const base = module.base;
let generation = 1, sequence = 0, snapshot = null, validatedChunk = null, inside = false;
const listeners = [];
const nativeAddress = offset => base.add(offset);

function range(pointer, bytes) {
    if (!pointer || pointer.isNull() || bytes < 1) return false;
    const memory = Process.findRangeByAddress(pointer);
    return memory && memory.protection.includes('r') &&
        pointer.compare(memory.base) >= 0 && pointer.add(bytes).compare(memory.base.add(memory.size)) <= 0;
}

function callsTo(offset, size, target) {
    const end = nativeAddress(offset + size);
    for (let pc = nativeAddress(offset); pc.compare(end) < 0;) {
        const ins = Instruction.parse(pc);
        if (ins.mnemonic === 'call' && /^0x[0-9a-f]+$/i.test(ins.opStr) &&
            ptr(ins.opStr).equals(nativeAddress(target))) return true;
        if (ins.size <= 0) break;
        pc = ins.next;
    }
    return false;
}

// Current-build entries were disassembled in the authorized live session.
// A whole profile must agree; never apply a blanket RVA delta.
const profiles = [
    {name:'archive', getter:0x46c500, scope:0x46c270, integer:0x4713a0,
        float:0x471280, returned:0x477770, pushFloat:0x46e350, map:0x46fed0, jump:0x48e9d0},
    {name:'live-0537b2a1047f', getter:0x46c570, scope:0x46c2e0, integer:0x471400,
        float:0x4712e0, returned:0x4777f0, pushFloat:0x46e3c0, map:0x46ff40, jump:0x48ea60}
];
const natives = profiles.find(p => callsTo(p.getter,124,p.scope) &&
    callsTo(p.integer,256,p.getter) && callsTo(p.float,288,p.getter) &&
    callsTo(p.returned,144,p.pushFloat) && callsTo(p.map,208,p.jump));
if (!natives || !callsTo(0x5093b0,708,0x509ea0)) {
    throw new Error('Native variable getter/setter body validation failed');
}
const getAddress = new NativeFunction(nativeAddress(natives.getter), 'pointer',
    ['pointer', 'int', 'int'], { abi: 'mscdecl', scheduling: 'exclusive' });

function checksum(bytes) {
    let value = 2166136261;
    for (const b of bytes) value = Math.imul(value ^ b, 16777619) >>> 0;
    return value;
}

function context(worker) {
    if (!range(worker, 0x34)) throw new Error('Unreadable worker');
    const header = worker.readPointer(), chunk = worker.add(4).readPointer();
    if (!range(chunk, 0x38) || !range(header, 0x34)) throw new Error('Unreadable ATEL header');
    const nameOffset = chunk.add(0x0c).readU32();
    if (nameOffset >= 0x100000 || !range(chunk.add(nameOffset), 16)) throw new Error('Invalid script name');
    const nameBytes = new Uint8Array(chunk.add(nameOffset).readByteArray(8));
    const terminator = nameBytes.indexOf(0);
    const name = String.fromCharCode(...nameBytes.slice(0,terminator < 0 ? 8 : terminator));
    const descriptor = manifest.scripts[name];
    if (!descriptor) return null;
    const length = chunk.readU32(), codeOffset = chunk.add(0x30).readU32();
    const count = chunk.add(0x34).readU16();
    const workerIndex = worker.add(0x2e).readU16();
    if (length !== descriptor.code_length || codeOffset >= 0x100000 ||
        count !== descriptor.worker_types.length || workerIndex >= count ||
        chunk.add(0x20).readU32() !== descriptor.event_data_offset) {
        throw new Error('Live script metadata differs from the archive');
    }
    if (!range(chunk.add(0x38), count * 4)) throw new Error('Unreadable worker table');
    const headerOffset = chunk.add(0x38 + workerIndex * 4).readU32();
    if (headerOffset >= 0x100000 || !header.equals(chunk.add(headerOffset))) {
        throw new Error('Worker header does not belong to this script');
    }
    const chunkIdentity = generation + ':' + chunk + ':' + name;
    if (validatedChunk !== chunkIdentity) {
        if (!range(chunk.add(codeOffset), length)) throw new Error('Unreadable script code');
        const code = new Uint8Array(chunk.add(codeOffset).readByteArray(length));
        if (checksum(code) !== descriptor.code_fnv32) throw new Error('Live bytecode differs from the archive');
        validatedChunk = chunkIdentity;
    }
    const variableCount = header.add(2).readU16();
    const variableOffset = header.add(0x14).readU32();
    if (!variableCount || variableCount > 2048 || variableOffset >= 0x100000 ||
        !range(chunk.add(variableOffset), variableCount * 8)) throw new Error('Invalid variable table');
    return { worker, header, chunk, name, descriptor, workerIndex,
        entry: worker.add(0x2c).readU16(), codeOffset, variableCount,
        variables: chunk.add(variableOffset) };
}

function readScalar(pointer, format) {
    const widths = [1, 1, 2, 2, 4, 4, 4];
    if (format > 6 || !range(pointer, widths[format])) throw new Error('Unreadable variable value');
    switch (format) {
    case 0: return pointer.readU8();
    case 1: return pointer.readS8();
    case 2: return pointer.readU16();
    case 3: return pointer.readS16();
    case 4: return pointer.readU32();
    case 5: return pointer.readS32();
    case 6: {
        const value = pointer.readFloat();
        if (!Number.isFinite(value)) throw new Error('Non-finite puzzle value');
        return value;
    }
    }
}

function copyFields(ctx) {
    const declarationIndex = new Map();
    for (let i = 0; i < ctx.variableCount; ++i) {
        const declaration = ctx.variables.add(i * 8);
        const key = declaration.readU32() + ':' + declaration.add(4).readU32();
        if (!declarationIndex.has(key)) declarationIndex.set(key, i);
    }
    const values = {}, missing = [];
    for (const field of ctx.descriptor.fields) {
        const index = declarationIndex.get(field.low + ':' + field.high);
        const count = field.high & 0xffff;
        if (index === undefined || count < 1 || count > 1024) {
            missing.push(field.role);
            continue;
        }
        const array = [];
        for (let i = 0; i < count; ++i) {
            // Native getter resolves scope and clamps. We check bounds before it.
            const address = getAddress(ctx.worker, index, i);
            array.push(readScalar(address, field.low >>> 28));
        }
        values[field.role] = count === 1 ? array[0] : array;
    }
    return { values, missing };
}

function publish(ctx, event) {
    if (inside) return;
    inside = true;
    try {
        const fields = copyFields(ctx);
        snapshot = { script: ctx.name, generation, sequence: ++sequence,
            thread: Process.getCurrentThreadId(), worker: ctx.workerIndex, entry: ctx.entry,
            values: fields.values, missing: fields.missing, event };
        send({ type: 'snapshot', snapshot });
    } finally { inside = false; }
}

function sourcePc(ctx) {
    let priority = ctx.worker.add(0x32).readU8();
    const type = ctx.header.readU16();
    priority = Math.min(priority, type === 5 || type === 6 ? 1 : 8);
    const thread = ctx.worker.add(0x12c + priority * 0x4c);
    if (!range(thread, 0x20)) return null;
    const pc = thread.add(0x18).readPointer().sub(ctx.chunk.add(ctx.codeOffset)).toInt32();
    return pc >= 0 && pc < ctx.descriptor.code_length ? pc : null;
}

function invalidate(reason) {
    ++generation;
    validatedChunk = null;
    snapshot = null;
    send({ type: 'reset', generation, sequence: ++sequence, reason });
}

function failure(error) {
    snapshot = null;
    send({ type: 'read-error', generation, sequence: ++sequence, error: String(error) });
}

function installSetter(offset) {
    listeners.push(Interceptor.attach(nativeAddress(offset), {
        onEnter(args) {
            if (inside) return;
            this.epoch = generation;
            try {
                const ctx = context(args[0]);
                if (!ctx) {
                    if (snapshot) invalidate('outside trial');
                    return;
                }
                const index = args[1].toInt32();
                if (index < 0 || index >= ctx.variableCount) throw new Error('Invalid write variable');
                const declaration = ctx.variables.add(index * 8);
                const low = declaration.readU32(), high = declaration.add(4).readU32();
                if (!ctx.descriptor.fields.some(f => f.low === low && f.high === high)) return;
                this.worker = args[0];
                this.event = { type: 'variable-write', setter: offset, variable: index,
                    element: args[2].toInt32(), low, high, limited: args[4].toInt32() !== 0,
                    pc: sourcePc(ctx) };
            } catch (error) { failure(error); }
        },
        onLeave() {
            if (!this.worker || this.epoch !== generation) return;
            try {
                const ctx = context(this.worker);
                if (ctx) publish(ctx, this.event);
            } catch (error) { failure(error); }
            this.worker = null;
        }
    }));
}

// Return dispatch runs once when a native CALL completes, unlike the wait/exec
// dispatcher which can repeat every frame. Native prompts remain owned by C++.
const calls = new Set([0x64, 0x13b, 0x15b, 0x11, 0x10b, 0x8003, 0x8005, 0x34, 0x35]);
try {
    installSetter(natives.integer);
    installSetter(natives.float);
    listeners.push(Interceptor.attach(nativeAddress(natives.returned), {
        onEnter(args) {
            if (inside || !calls.has(args[0].toUInt32())) return;
            this.epoch = generation;
            this.worker = args[1];
            this.call = args[0].toUInt32();
        },
        onLeave() {
            if (!this.worker || this.epoch !== generation) return;
            try {
                const ctx = context(this.worker);
                if (ctx) publish(ctx, { type: 'native-call-complete', call: this.call, pc: sourcePc(ctx) });
                else if (snapshot) invalidate('outside trial');
            } catch (error) { failure(error); }
            this.worker = null;
        }
    }));
    for (const [offset, reason] of [[natives.map, 'map-request'], [0x5093b0, 'resource-load-start']]) {
        listeners.push(Interceptor.attach(nativeAddress(offset), {
            onEnter() { if (!inside) invalidate(reason); }
        }));
    }
} catch (error) {
    for (const listener of listeners) listener.detach();
    throw error;
}

rpc.exports = {
    state() { return snapshot; },
    stop() {
        for (const listener of listeners) listener.detach();
        invalidate('detached');
    }
};
send({ type: 'ready', trials: manifest.trials.map(t => t.name), native_profile:natives.name,
    status: 'Event-only copied state; production speech remains unchanged' });
