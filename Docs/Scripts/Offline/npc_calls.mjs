// Extract worker-local model, dialogue, and service leads. No game access.
import fs from 'node:fs';
import path from 'node:path';
const source = fs.readFileSync(new URL('../../../src/navigation/atel.js', import.meta.url), 'utf8');
const { scanWorker, decodeInstructions } = await import('data:text/javascript;base64,' + Buffer.from(source).toString('base64'));
const input = process.argv[2];
const data = JSON.parse(fs.readFileSync(input, 'utf8'));
const interesting = new Set([1, 0x134, 0x43, 0x64, 0x13B, 0x401D, 0x225, 0x15B]);
const output = [];
for (const event of data.events) {
    const script = { code: Uint8Array.from(Buffer.from(event.code, 'hex')), workers: event.workers };
    script.instructions = decodeInstructions(script.code);
    for (let index = 0; index < event.workers.length; index++) {
        const worker = event.workers[index];
        if (worker.type !== 1) continue;
        const calls = [], seen = new Set();
        scanWorker(script, index, undefined, function(call) {
            // Shared subroutine behavior must not silently become an actor role.
            if (call.worker !== index || !interesting.has(call.call)) return;
            const key = JSON.stringify(call);
            if (!seen.has(key)) { seen.add(key); calls.push(call); }
        });
        if (calls.length) output.push({ script: event.script, source: event.source, worker: index, calls });
    }
}
const target = path.join(path.dirname(input), 'npc_calls.json');
fs.writeFileSync(target, JSON.stringify(output));
console.log(`Recorded model/dialogue/service evidence for ${output.length} character workers: ${target}`);
