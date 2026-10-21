const assert = require('node:assert/strict');
const { sphereUnavailable, requiredSphere } = require('../Frida/FFX/sphere_requirements.js');
assert.equal(sphereUnavailable(0), false);
assert.equal(sphereUnavailable(1), true);
const catalog = [
    { id: 0x2046, special: 0, flags: 5 }, { id: 0x2047, special: 0, flags: 2 },
    { id: 0x2048, special: 0, flags: 8 }, { id: 0x2049, special: 0, flags: 0x400 },
    { id: 0x204a, special: 0, flags: 0x10 },
    ...[1, 2, 3, 4].map(special => ({ id: 0x2050 + special, special, flags: 0 }))
];
for (const [flags, id] of [[1, 0x2046], [4, 0x2046], [2, 0x2047], [8, 0x2048], [0x400, 0x2049], [0x10, 0x204a]])
    assert.equal(requiredSphere(flags, 2, catalog), id);
for (const [type, special] of [[0x12, 1], [0x11, 2], [0x00, 3], [0x10, 4]])
    assert.equal(requiredSphere(0x8000, type, catalog), 0x2050 + special);
for (const [flags, type] of [[0, 1], [0x8000, 1], [0x200, 2]])
    assert.equal(requiredSphere(flags, type, catalog), 0);
assert.equal(requiredSphere(4, 2, []), 0);
console.log('PASS: disabled-byte polarity, native activation flag masks, all four lock selectors, empty and missing catalogs.');
