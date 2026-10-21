const assert = require('node:assert/strict');
const { gridConfirmation, movementConfirmed, preparingMovement } = require('../Frida/FFX/sphere_confirmations.js');
assert.equal(gridConfirmation(11, 0x656160), 9);
assert.equal(gridConfirmation(11, 0x656060), 10);
assert.equal(gridConfirmation(7, 0x656330), -1);
assert.equal(gridConfirmation(11, 0x656330), -1);
assert.equal(movementConfirmed(0, 1, 0, 5, 10), true);
for (const args of [[1, 1, 0, 5, 10], [0, 0, 0, 5, 10], [0, 1, 5, 5, 10], [0, 1, -1, 5, 10], [0, 1, 0, 10, 10]])
    assert.equal(movementConfirmed(...args), false);
console.log('PASS: native movement/exit prompt slots, confirmed destination, No/cancel, unchanged and invalid positions.');
assert.equal(preparingMovement(0, 0, 43, 1024, false), true);
for (const args of [[0, 0, 0, 1024, false], [7, 0, 43, 1024, false], [0, 0, 43, 1024, true], [0, -1, 43, 1024, false], [0, 0, 1024, 1024, false]])
    assert.equal(preparingMovement(...args), false);
console.log('PASS: preparing real movement only, inactive/invalid/unchanged/busy rejection.');
