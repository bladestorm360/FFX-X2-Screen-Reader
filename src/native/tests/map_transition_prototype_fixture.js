// Offline Frida callback fixture. No process attachment or game code execution.
const assert = require('assert');
const values = new Map([[0xd2ca90, 221], [0xd2ca9c, 1], [0xd2a8e0, 0],
    [0xf26ae8, 0x2000], [0x21f8, 0x3000], [0x3004, 32], [0x3008, 96], [0x4000, 200]]);
class Pointer {
    constructor(value) { this.value = value; }
    add(offset) { return new Pointer(this.value + offset); }
    isNull() { return this.value === 0; }
    toUInt32() { return this.value; }
    readU8() { return values.get(this.value) || 0; }
    readU16() { return this.readU8(); }
    readU32() { return this.readU8(); }
    readPointer() { return new Pointer(this.readU8()); }
}
let callbacks, text = 'Guadosalam - Shop', entrance;
const emitted = [];
global.NativeFunction = function (address) {
    return ({0x4b7f10: () => new Pointer(0x4000),
        0x46bd20: (chunk, value) => { assert.equal(chunk.value, 0x3000); entrance = value; return 34; },
        0x46c3c0: () => new Pointer(0x5000),
        0x2460e0: (_, buffer) => { buffer.text = text; }})[address.value];
};
global.Memory = {alloc: () => ({writeByteArray() {}, readCString() { return this.text; }})};
global.Interceptor = {attach: (address, handlers) => {
    assert.equal(address.value, 0x48dfe0); callbacks = handlers; return {detach() {}};
}};
const {installMapTransitionPrototype} = require('../../../Docs/Scripts/Frida/FFX/map_transition');
installMapTransitionPrototype(value => new Pointer(value), (...args) => emitted.push(args));
function arrive(room) {
    const before = emitted.length;
    const state = {}; callbacks.onEnter.call(state, [new Pointer(room)]);
    assert.equal(emitted.length, before); callbacks.onLeave.call(state);
}
const state = {}; callbacks.onEnter.call(state, [new Pointer(221)]);
assert.equal(emitted.length, 0); callbacks.onLeave.call(state);
assert.deepEqual(emitted, [['Entering Guadosalam - Shop', false, 'map-transition']]);
assert.equal(entrance, 1); arrive(221); assert.equal(emitted.length, 2);
arrive(220); values.set(0xd2ca90, 23); arrive(23);
values.set(0xd2ca90, 221); values.set(0xd2a8e0, 1); arrive(221);
values.set(0xd2a8e0, 0); values.set(0xd2ca9c, 3); arrive(221);
values.set(0xd2ca9c, 1); text = ''; arrive(221);
assert.equal(emitted.length, 2);
console.log('PASS: offline map-return prototype, native entrance names, repeated arrivals, stale/title/battle/invalid bounds and unknown-name guards.');
