// Run only against extracted workspace data. No game process access.
import fs from 'node:fs';
import path from 'node:path';
import { fileURLToPath } from 'node:url';
const here = path.dirname(fileURLToPath(import.meta.url));
const source = fs.readFileSync(path.join(here, '../../../src/navigation/atel.js'), 'utf8');
const { scanWorker, decodeInstructions } = await import('data:text/javascript;base64,' + Buffer.from(source).toString('base64'));
const input = process.argv[2];
if (!input) throw new Error('Usage: node analyze_maps.mjs maps.json');
const data = JSON.parse(fs.readFileSync(input, 'utf8'));
const report = [];
for (const event of data.events) {
    const script = { code: Uint8Array.from(Buffer.from(event.code, 'hex')), workers: event.workers };
    script.instructions = decodeInstructions(script.code);
    const workers = event.workers.map(function(worker, i) {
        const scan = scanWorker(script, i);
        return { index: i, type: worker.type, ...scan };
    });
    report.push({ script: event.script, areas: event.area_indices,
        code_length: event.code_length, source: event.source, workers });
}
const output = path.join(path.dirname(input), 'analysis.json');
fs.writeFileSync(output, JSON.stringify(report));
const exits = report.flatMap(e => e.workers.filter(w => [2, 3, 5, 6].includes(w.type) && w.isExit));
console.log(`Analyzed ${report.length} scripts; ${exits.length} trigger workers reach a warp; ${exits.filter(w => w.exitTargetMapId !== null).length} have one proven constant destination.`);
console.log('Evidence: ' + output);
