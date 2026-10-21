// Regression tests run outside the game; exercise the production analyzer.
import assert from 'node:assert/strict';
import fs from 'node:fs';
const source = fs.readFileSync(new URL('../../../src/navigation/atel.js', import.meta.url), 'utf8');
const { scanWorker, decodeInstructions } = await import('data:text/javascript;base64,' + Buffer.from(source).toString('base64'));
const instruction = (op, arg) => arg === undefined ? [op] : [op, arg & 255, arg >>> 8 & 255];
const push = value => instruction(0xAE, value);
const warp = (room, spawn = 0) => [...push(room), ...push(spawn), ...instruction(0xD8, 0x11), 0x3C];
const worker = (entries = [0], jumps = [], ints = []) => ({ entries, jumps, ints });
const script = (code, workers = [worker()]) => ({ code: Uint8Array.from(code), workers });
let cases = 0;
function test(name, run) { run(); cases++; console.log('PASS ' + name); }

test('direct warp preserves room/spawn', () => {
    const s = scanWorker(script(warp(371, 4)), 0);
    assert.equal(s.exitTargetMapId, 371); assert.equal(s.exitTargetSpawnpoint, 4);
});
test('jump operand indexes worker jump table', () => {
    const s = scanWorker(script([...instruction(0xB0, 0), ...warp(999), ...warp(371)], [worker([0], [13])]), 0);
    assert.equal(s.exitTargetMapId, 371);
});
test('JSR targets another worker entry, not operand PC', () => {
    const s = scanWorker(script([...instruction(0xB3, 1), 0x3C, ...warp(371)], [worker(), worker([4])]), 0);
    assert.equal(s.exitTargetMapId, 371);
});
test('PUSHI reads signed integer reference table', () => {
    const s = scanWorker(script([...instruction(0xAD, 0), ...instruction(0xAD, 1), ...instruction(0xB5, 0x10B), 0x3C], [worker([0], [], [371, 8])]), 0);
    assert.equal(s.exitTargetMapId, 371); assert.equal(s.exitTargetSpawnpoint, 8);
});
test('other calls cannot donate stale warp arguments', () => {
    const s = scanWorker(script([...push(371), ...push(0), ...instruction(0xD8, 0x4005), ...instruction(0xD8, 0x11), 0x3C]), 0);
    assert.equal(s.isExit, true); assert.equal(s.exitTargetMapId, null);
});
test('computed argument cannot be mistaken for preceding literal', () => {
    const s = scanWorker(script([...push(371), ...push(0), 0x06, ...instruction(0xD8, 0x11), 0x3C]), 0);
    assert.equal(s.exitTargetMapId, null);
});
test('conditional routes with different rooms remain unnamed', () => {
    const s = scanWorker(script([...instruction(0xD6, 0), ...warp(371), ...warp(372)], [worker([0], [13])]), 0);
    assert.equal(s.warps.length, 2); assert.equal(s.exitTargetMapId, null);
});
test('same room with multiple entrances retains room only', () => {
    const s = scanWorker(script([...instruction(0xD6, 0), ...warp(371, 0), ...warp(371, 1)], [worker([0], [13])]), 0);
    assert.equal(s.exitTargetMapId, 371); assert.equal(s.exitTargetSpawnpoint, null);
});
test('signed negative load/resume request is not destination', () => {
    const s = scanWorker(script(warp(-1)), 0);
    assert.equal(s.warps[0].room, -1); assert.equal(s.exitTargetMapId, null);
});
test('missing jump/entry information prevents naming', () => {
    const s = scanWorker(script([...instruction(0xD6, 5), ...warp(371)]), 0);
    assert.equal(s.unresolved, true); assert.equal(s.exitTargetMapId, null);
    const invalid = scanWorker(script(warp(371), [worker([0, 2])]), 0);
    assert.equal(invalid.unresolved, true); assert.equal(invalid.exitTargetMapId, null);
});
test('graphics activation 8002 is not a warp', () => {
    const s = scanWorker(script([...push(371), ...push(0), ...instruction(0xD8, 0x8002), 0x3C]), 0);
    assert.equal(s.isExit, false);
});
test('cyclic jumps terminate', () => {
    const s = scanWorker(script(instruction(0xB0, 0), [worker([0], [0])]), 0);
    assert.equal(s.truncated, false); assert.ok(s.steps < 5);
});
test('truncated instruction is rejected', () => {
    assert.throws(() => decodeInstructions(Uint8Array.from([0xAE, 2])), /Truncated/);
});
const input = process.argv[2];
if (input) {
    const data = JSON.parse(fs.readFileSync(input, 'utf8'));
    test('real logged Zanarkand exit resolves to room 371, area index 340', () => {
        const event = data.events.find(e => e.script === 'znkd1400');
        assert.equal(event.code_length, 17956);
        const s = scanWorker(script(Buffer.from(event.code, 'hex'), event.workers), 31);
        assert.equal(s.exitTargetMapId, 371); assert.equal(s.exitTargetSpawnpoint, 0);
        assert.deepEqual([...new Set(data.room_areas['371'])], [340]);
    });
}
console.log(`${cases} ATEL regression tests passed.`);
