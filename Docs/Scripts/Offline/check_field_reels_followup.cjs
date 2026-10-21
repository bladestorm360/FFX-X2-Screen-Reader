const assert = require('node:assert/strict');
const p = require('../Frida/FFX/field_reels_followup.js');
const field = {controller: 1, control: 2, workers: 1, actors: 0x20000, capacity: 1,
    player: 0x20000, enabled: 1, collision: 0, position: [0, 0, 0], chunk: 1,
    begin: 128, end: 192, entrance: 1};
assert(p.activeFieldSnapshot(field));
for (const delta of [{player: 0}, {control: 0}, {enabled: 0}, {collision: 0xcdcdcdcd},
    {position: [0, NaN, 0]}, {chunk: 0}, {entrance: 2}]) assert(!p.activeFieldSnapshot({...field, ...delta}));
assert.equal(p.nativeOverdrivePercent(33, 150), 22);
assert.equal(p.nativeOverdrivePercent(0, 100), 0);
assert.equal(p.nativeOverdrivePercent(150, 100), 100);
assert.equal(p.nativeOverdrivePercent(10, 0), null);
const attack = [[4, 5, 6], [6, 4, 5], [5, 6, 4]];
assert.equal(p.automaticReelMatch(attack), 6);
assert(!p.cueSymbol(attack, 4));
assert(p.cueSymbol(attack, 5)); assert(p.cueSymbol(attack, 6));
assert.equal(p.automaticReelMatch([[0, 1, 2, 3], [0, 1, 2, 3], [0, 1, 2, 3]]), null);
assert.equal(p.automaticReelMatch([[7, 8, 9], [7, 8, 9], [7, 8, 9]]), null);
assert.equal(p.automaticReelMatch([[0, 10, 7], [10, 8, 1], [2, 10, 9]]), null);
assert.equal(p.automaticReelMatch([[0, 10, 7], [10, 8, 1], [2, 3, 9]]), null);
const observed = {table: 0xeb38, begin: 0x1000, initial: 0x1270, control: 0x1340};
assert(p.qualifyReelOverlay(0x5ba50000, 0x5ba5eb38, observed, [0x5ba51000, 0, 0, 0, 0x5ba51270]));
assert(!p.qualifyReelOverlay(0x5ba50000, 0x5ba5eb38, {...observed, table: 0xeb40}, [0x5ba51000, 0, 0, 0, 0x5ba51270]));
for (let command = 0x3074; command <= 0x3077; ++command) {
    assert.equal(p.effectSource(0x128004, command), 'global');
    assert.equal(p.effectSource(0x120204, command), 'character');
    assert.equal(p.effectSource(0x128005, command), null);
}
assert.equal(p.effectSource(0x120204, 0x3000), null);
assert.equal(p.effectSource(0x120204), null);
console.log('PASS: offline Frida field readiness, observed Wakka table, native effect sources, Attack-only automatic match, Aurochs choice requirement, manual cues and bar percentages; no game/probe executed.');
