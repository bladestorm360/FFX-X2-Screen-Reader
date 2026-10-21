import assert from 'node:assert/strict';
import fs from 'node:fs/promises';
import vm from 'node:vm';
const context = vm.createContext({
    Uint8Array, Set, NativeFunction: function() { return () => null; }
});
const memory = { rva: () => null };
const addresses = { playerChrsAddr: null, CHR_SIZE: 0, CHR_RAM_OFFSET: 0, CHARACTER_NAMES: [] };
const safeRead = {
    isValidPtr: value => value instanceof Uint8Array,
    safeReadByteArray: value => value.buffer
};
const source = new vm.SourceTextModule(await fs.readFile(new URL('../../core/charset.js', import.meta.url), 'utf8'), { context });
await source.link(specifier => {
    const values = specifier === './addresses.js' ? addresses : specifier === './memory.js' ? memory : safeRead;
    return new vm.SyntheticModule(Object.keys(values), function() {
        for (const [name, value] of Object.entries(values)) this.setExport(name, value);
    }, { context });
});
await source.evaluate();
const decode = (bytes, stop) => Array.from(source.namespace.decodeFFXSegments(Uint8Array.from(bytes), bytes.length, 0, stop));
assert.deepEqual(decode([0x50, 0, 0x51, 0]), ['A', 'B']); // existing multi-record buffers
assert.deepEqual(decode([0x50, 0, 0x51, 0], true), ['A']); // native terminated text, stale tail excluded
assert.deepEqual(decode([0x50, 7, 0x71, 0x51, 0], true), ['AB']);
assert.deepEqual(decode([0x50, 0x0a, 0, 0x51, 0], true), ['A', 'B']);
assert.deepEqual(decode([0x50, 3, 0x50, 0], true), ['A', 'A']);
console.log('Charset fixtures passed: legacy multi-record decoding preserved; native terminators, opcode payloads, segment ordering and repeats.');
