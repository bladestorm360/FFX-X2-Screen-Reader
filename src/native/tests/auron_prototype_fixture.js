// Retained native arrays only; no process attachment or game code execution.
const assert = require('assert');
const {sequences} = require('../../../Docs/Scripts/Offline/output/auron_sequences_20261009_112337_188983.json');
const {installAuronSequencePrototype} = require('../../../Docs/Scripts/Frida/FFX/auron_sequence');
let hooks, count, pointer = 1, phase = 1, buttons;
const state = {battle: true, focused: true, scene: 0, menu: false, utility: false,
    cues: true, autoSuccess: false, controller: false};
const said = [];
global.Interceptor = {attach(address, handlers) { assert.equal(address, 0x498a40); hooks = handlers; }};
const source = {isNull: () => !pointer, add: offset => ({readU16: () => buttons[offset / 2]})};
function resolve(address) {
    return ({0x498a40: address, 0xf3d6b4: {readU16: () => phase},
        0xf3d6c0: {readU32: () => count}, 0xf3d6c4: {readPointer: () => source}})[address];
}
installAuronSequencePrototype(resolve, () => state, (action, pad) => (pad ? 'Pad ' : 'Key ') + action,
    (...args) => said.push(args));
const actions = [[6, 5, 7, 4, 2, 3, 4], [1, 2, 0, 3, 8, 10, 5, 4],
    [0, 8, 1, 10, 3, 2, 6], [4, 3, 10, 2, 8, 6]];
for (let i = 0; i < sequences.length; ++i) {
    buttons = sequences[i].button_masks.map(value => value & 0xffff).concat(0xffff);
    count = buttons.length - 1;
    for (const pad of [false, true]) {
        state.controller = pad; hooks.onLeave();
        assert.deepEqual(said.at(-1), ['Sequence: ' + actions[i].map(action => (pad ? 'Pad ' : 'Key ') +
            (action + (pad ? 14 : 0))).join(', '), true, 'overdrive-auron-sequence']);
    }
}
const before = said.length;
hooks.onLeave(); hooks.onLeave(); assert.equal(said.length, before + 2);
const quiet = said.length;
for (const [key, value] of [['autoSuccess', true], ['cues', false], ['battle', false], ['focused', false],
    ['scene', 4], ['menu', true], ['utility', true]]) {
    const previous = state[key]; state[key] = value; hooks.onLeave(); state[key] = previous;
}
phase = 4; hooks.onLeave(); phase = 1;
pointer = 0; hooks.onLeave(); pointer = 1;
count = 16; hooks.onLeave(); count = buttons.length - 1;
buttons[count] = 0; hooks.onLeave(); buttons[count] = 0xffff;
buttons[0] = 0x2001; hooks.onLeave();
assert.equal(said.length, quiet);
console.log('PASS: offline Auron initialization prototype, all four actual arrays, keyboard/controller action ordering, repeated casts, toggles/context and malformed-sequence guards.');
