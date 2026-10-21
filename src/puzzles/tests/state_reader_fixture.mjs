// Combined semantic fixture; no Frida, process attachment or game access.
import assert from 'node:assert/strict';
import fs from 'node:fs';
import path from 'node:path';
import vm from 'node:vm';

const manifestPath = path.resolve(process.argv[2]);
const manifest = JSON.parse(fs.readFileSync(manifestPath, 'utf8'));
const events = JSON.parse(fs.readFileSync(manifest.source, 'utf8')).events;
const sourcePath = new URL('../state_reader.js', import.meta.url);
const source = fs.readFileSync(sourcePath, 'utf8').replace('/* CLOISTER_MANIFEST */ {}', JSON.stringify(manifest));
const memory = Buffer.alloc(0x200000);
const hooks = new Map(), messages = [];
const imageBase = 0x400000, workerAddress = 0x12000, chunkAddress = 0x20000;
const headerAddress = chunkAddress + 0x300, variablesAddress = chunkAddress + 0x500;
let inGameEvent = false, getterCalls = 0;
const liveProfile=process.argv.includes('--live');
const getter=liveProfile?0x46c570:0x46c500;
const integer=liveProfile?0x471400:0x4713a0;
const returned=liveProfile?0x4777f0:0x477770;
const mapRequest=liveProfile?0x46ff40:0x46fed0;

class Pointer {
    constructor(value) { this.value = Number(value) >>> 0; }
    add(value) { return new Pointer(this.value + Number(value)); }
    sub(value) { return new Pointer(this.value - (value instanceof Pointer ? value.value : Number(value))); }
    compare(value) { return Math.sign(this.value - value.value); }
    equals(value) { return this.value === value.value; }
    isNull() { return this.value === 0; }
    toInt32() { return this.value | 0; }
    toUInt32() { return this.value >>> 0; }
    toString() { return '0x' + this.value.toString(16); }
    readU8() { return memory.readUInt8(this.value); }
    readS8() { return memory.readInt8(this.value); }
    readU16() { return memory.readUInt16LE(this.value); }
    readS16() { return memory.readInt16LE(this.value); }
    readU32() { return memory.readUInt32LE(this.value); }
    readS32() { return memory.readInt32LE(this.value); }
    readFloat() { return memory.readFloatLE(this.value); }
    readPointer() { return new Pointer(this.readU32()); }
    readByteArray(length) { return Uint8Array.from(memory.subarray(this.value, this.value + length)).buffer; }
    readUtf8String(length) { return memory.subarray(this.value, this.value + length).toString('utf8'); }
}
const ptr = value => new Pointer(value instanceof Pointer ? value.value : Number(value));
const expectedCalls = new Map(liveProfile ?
    [[0x46c570,0x46c2e0],[0x471400,0x46c570],[0x4712e0,0x46c570],
     [0x4777f0,0x46e3c0],[0x46ff40,0x48ea60],[0x5093b0,0x509ea0]] :
    [[0x46c500,0x46c270],[0x4713a0,0x46c500],[0x471280,0x46c500],
     [0x477770,0x46e350],[0x46fed0,0x48e9d0],[0x5093b0,0x509ea0]]);
if(process.argv.includes('--unsupported'))expectedCalls.clear();
if(process.argv.includes('--mixed')) {
    expectedCalls.delete(0x4713a0);expectedCalls.set(0x471400,0x46c570);
}

function addressOf(low, index) {
    const location = (low >>> 25) & 7, format = low >>> 28;
    const widths = [1, 1, 2, 2, 4, 4, 4];
    assert.ok(location === 0 || location === 6);
    const scope = location === 0 ? 0x180000 : chunkAddress + memory.readUInt32LE(chunkAddress + 0x20);
    return scope + (low & 0xffffff) + index * widths[format];
}

function NativeFunction(address, result, args, options) {
    assert.equal(address.value, imageBase + getter);
    assert.equal(result, 'pointer');
    assert.equal(options.abi, 'mscdecl');
    assert.equal(options.scheduling, 'exclusive');
    return (worker, variable, index) => {
        assert.ok(inGameEvent, 'Game getter ran outside an event callback');
        assert.equal(worker.value, workerAddress);
        const low = memory.readUInt32LE(variablesAddress + variable * 8);
        const high = memory.readUInt32LE(variablesAddress + variable * 8 + 4);
        assert.ok(index >= 0 && index < (high & 0xffff), 'Unclamped native array read');
        ++getterCalls;
        return ptr(addressOf(low, index));
    };
}

const sandbox = {
    Uint8Array, Math, Set, Map, Number, Error, String, Object, ptr, NativeFunction,
    Process: {
        pointerSize: 4,
        enumerateModules: () => [{ name: 'FFX.exe', base: ptr(imageBase) }],
        getCurrentThreadId: () => 9,
        findRangeByAddress: pointer => pointer.value >= 0x10000 && pointer.value < memory.length ?
            { base: ptr(0x10000), size: memory.length - 0x10000, protection: 'rw-' } : null
    },
    Instruction: {
        parse: pointer => ({ mnemonic: 'call', opStr: ptr(imageBase + expectedCalls.get(pointer.value - imageBase)).toString(),
                             size: 5, next: pointer.add(5) })
    },
    Interceptor: {
        attach: (address, callbacks) => {
            hooks.set(address.value - imageBase, callbacks);
            return { detach: () => hooks.delete(address.value - imageBase) };
        }
    },
    rpc: { exports: {} },
    send: message => messages.push(structuredClone(message))
};
if(process.argv.includes('--unsupported')||process.argv.includes('--mixed')) {
    assert.throws(()=>vm.runInNewContext(source,sandbox), /body validation failed/);
    assert.equal(hooks.size,0);
    console.log('Unsupported or mixed native profile refused before installing hooks.');
    process.exit(0);
}
vm.runInNewContext(source, sandbox, { filename: 'state_reader.js' });
assert.equal(messages[0].native_profile,liveProfile?'live-0537b2a1047f':'archive');
assert.equal(messages[0].type, 'ready');
assert.equal(hooks.size, 5);

function invoke(offset, args, mutate) {
    const hook = hooks.get(offset), frame = {};
    inGameEvent = true;
    try {
        hook.onEnter?.call(frame, args.map(ptr));
        mutate?.();
        hook.onLeave?.call(frame, ptr(0));
    } finally { inGameEvent = false; }
}

function load(name) {
    invoke(0x5093b0, []);
    memory.fill(0);
    const descriptor = manifest.scripts[name];
    const event = events.find(s => s.script === name);
    memory.writeUInt32LE(headerAddress, workerAddress);
    memory.writeUInt32LE(chunkAddress, workerAddress + 4);
    memory.writeUInt16LE(0, workerAddress + 0x2e);
    memory.writeUInt32LE(chunkAddress + 0x1000, workerAddress + 0x12c + 0x18);
    memory.writeUInt32LE(descriptor.code_length, chunkAddress);
    memory.writeUInt32LE(0x800, chunkAddress + 0x0c);
    memory.writeUInt32LE(descriptor.event_data_offset, chunkAddress + 0x20);
    memory.writeUInt32LE(0x1000, chunkAddress + 0x30);
    memory.writeUInt16LE(descriptor.worker_types.length, chunkAddress + 0x34);
    memory.writeUInt32LE(0x300, chunkAddress + 0x38);
    memory.write(name, chunkAddress + 0x800, 'utf8');
    Buffer.from(event.code, 'hex').copy(memory, chunkAddress + 0x1000);
    memory.writeUInt16LE(descriptor.worker_types[0], headerAddress);
    memory.writeUInt16LE(descriptor.fields.length + 1, headerAddress + 2);
    memory.writeUInt32LE(0x500, headerAddress + 0x14);
    descriptor.fields.forEach((field, index) => {
        memory.writeUInt32LE(field.low, variablesAddress + index * 8);
        memory.writeUInt32LE(field.high, variablesAddress + index * 8 + 4);
        for (let element = 0; element < (field.high & 0xffff); ++element) {
            writeField(field, element, element + 1);
        }
    });
    // One valid declaration that is outside the selected puzzle watch fields.
    memory.writeUInt32LE(0x0c00ffff, variablesAddress + descriptor.fields.length * 8);
    memory.writeUInt32LE(1, variablesAddress + descriptor.fields.length * 8 + 4);
    return descriptor;
}

function writeField(field, element, value) {
    const address = addressOf(field.low, element), format = field.low >>> 28;
    if (format === 0) memory.writeUInt8(value & 255, address);
    else if (format === 2) memory.writeUInt16LE(value & 65535, address);
    else if (format === 5) memory.writeInt32LE(value, address);
    else throw new Error('Add a fixture for the new watched format');
}

for (const trial of manifest.trials) {
    const descriptor = load(trial.script);
    const index = descriptor.fields.findIndex(f => f.role === 'carried');
    const field = descriptor.fields[index];
    const args = [workerAddress, index, 0, 2, 0];
    invoke(integer, args, () => writeField(field, 0, 2));
    const first = sandbox.rpc.exports.state();
    assert.equal(first.script, trial.script);
    assert.equal(first.values.carried, 2);
    assert.equal(first.missing.length, 0);
    assert.equal(first.event.pc, 0);
    const firstSequence = first.sequence;
    invoke(integer, args);
    assert.ok(sandbox.rpc.exports.state().sequence > firstSequence, 'Identical writes were incorrectly deduplicated');
    writeField(field, 0, 3);
    assert.equal(first.values.carried, 2, 'Snapshot retained borrowed game values');
    const callsBeforeRpc = getterCalls;
    sandbox.rpc.exports.state();
    assert.equal(getterCalls, callsBeforeRpc, 'RPC called a game getter');
    const beforeUnwatched = messages.length;
    invoke(integer, [workerAddress, descriptor.fields.length, 0, 0, 0]);
    assert.equal(messages.length, beforeUnwatched);
    // Ignore an old function return after a map-generation change.
    const frame = {};
    inGameEvent = true;
    hooks.get(returned).onEnter.call(frame, [0x64, workerAddress].map(ptr));
    invoke(mapRequest, []);
    const beforeLate = getterCalls;
    hooks.get(returned).onLeave.call(frame, ptr(0));
    inGameEvent = false;
    assert.equal(getterCalls, beforeLate);
    assert.equal(sandbox.rpc.exports.state(), null);
}

const bevelle = load('bvyt1100');
const matrix = bevelle.fields.findIndex(f => f.role === 'sphere_matrix');
invoke(integer, [workerAddress, matrix, 999, 0, 0]);
const matrixSnapshot = sandbox.rpc.exports.state();
assert.equal(matrixSnapshot.values.sphere_matrix.length, 72);
assert.equal(matrixSnapshot.values.sphere_matrix[71], 72);

load('bsyt0100');
const beforeCorrupt = getterCalls;
memory[chunkAddress + 0x1000] ^= 1;
invoke(integer, [workerAddress, 0, 0, 0, 0]);
assert.equal(sandbox.rpc.exports.state(), null);
assert.equal(getterCalls, beforeCorrupt);
assert.equal(messages.at(-1).type, 'read-error');

load('bsyt0100');
memory.writeUInt32LE(1, workerAddress);
invoke(integer, [workerAddress, 0, 0, 0, 0]);
assert.equal(sandbox.rpc.exports.state(), null);
assert.equal(messages.at(-1).type, 'read-error');

load('bsyt0100');
invoke(integer,[workerAddress,0,0,0,0]);
const shortNameErrors=messages.filter(m=>m.type==='read-error').length;
memory.fill(0,chunkAddress+0x800,chunkAddress+0x810);
memory.write('test20',chunkAddress+0x800,'utf8');
invoke(integer,[workerAddress,0,0,0,0]);
assert.equal(sandbox.rpc.exports.state(),null);
assert.equal(messages.at(-1).type,'reset');
assert.equal(messages.at(-1).reason,'outside trial');
assert.equal(messages.filter(m=>m.type==='read-error').length,shortNameErrors,
    'A short native script name with NUL padding caused a read failure');

sandbox.rpc.exports.stop();
assert.equal(hooks.size, 0);
console.log('All six state fixtures passed: event-owned getters, copies, repeat identity, generation invalidation, 72-slot bounds and corrupt-data refusal.');
