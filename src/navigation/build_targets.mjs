// Offline navigation audit. Karifean/FFXDataParser and Fahrenheit provide
// layout references. Player entry paths, rather than arbitrary REQ presence,
// determine useful targets. Native English and model/speaker evidence are
// combined by finish_targets.py; no runtime-learned labels.
import fs from 'node:fs';
import path from 'node:path';
const source = fs.readFileSync(new URL('./atel.js', import.meta.url), 'utf8');
const {scanWorker, decodeInstructions} = await import('data:text/javascript;base64,' + Buffer.from(source).toString('base64'));
const input = process.argv[2];
if (!input) throw new Error('Pass the extracted maps_v3.json');
const data = JSON.parse(fs.readFileSync(input, 'utf8'));
const useful = new Set([0x64,0x13b,0x15b,0x401d,0x225,0x136,0x15f,0x1a7,0x7002]);
const modelEvidence=JSON.parse(fs.readFileSync(path.join(path.dirname(input),'npc_calls.json'),'utf8'));
const models=new Map(modelEvidence.map(r=>[r.source+':'+r.worker,r.calls.filter(c=>[1,0x134].includes(c.call)).map(c=>c.literals.at(-1))]));
const report = [];
for (const event of data.events) {
    const script = {code: Uint8Array.from(Buffer.from(event.code, 'hex')), workers: event.workers,scanLimit:Infinity};
    script.instructions = decodeInstructions(script.code);
    let hash = 2166136261;
    for (const byte of script.code) hash = Math.imul(hash ^ byte, 16777619) >>> 0;
    const targets = [], excluded = [], resultCache=new Map();
    for (let index = 0; index < script.workers.length; ++index) {
        const worker = script.workers[index];
        if (![1,2,3,5,6].includes(worker.type)) continue;
        // The game's seven interaction channels can be remapped to any entry.
        // Cover every authored callback, not just default talk 2 / crossing 4.
        // Initialization and idle entries 0/1 are never interaction roots.
        const rootEntries = worker.entries.map((_,e)=>e).filter(e=>e>=2);
        if(rootEntries.some(e=>e>=320)) throw new Error('Native interaction entry mask would overflow');
        const pending = rootEntries.map(entry => ({worker:index, entry, root:true,origin:entry}));
        const outcomes = new Map(rootEntries.map(entry=>[entry,false]));
        const seen = new Set(), calls = [], warps = [], rootCalls = [];
        let limited = false;
        // Authored extracted graphs are finite. Follow every reachable pair;
        // runtime fallback retains its separate bounded safety budget.
        while (pending.length) {
            const node = pending.shift(), key = node.origin+':'+node.worker + ':' + node.entry;
            if (seen.has(key)) continue;
            seen.add(key);
            const resultKey=node.worker+':'+node.entry;
            if(!resultCache.has(resultKey)) {
                const directCalls=[];
                const result=scanWorker(script,node.worker,[node.entry],call=>{if(useful.has(call.call)) directCalls.push(call);});
                resultCache.set(resultKey,{result,directCalls});
            }
            const {result,directCalls}=resultCache.get(resultKey);
            calls.push(...directCalls);
            if(directCalls.length) {outcomes.set(node.origin,true); if(node.root) rootCalls.push(...directCalls);}
            warps.push(...result.warps);
            if(result.warps.length) outcomes.set(node.origin,true);
            limited ||= result.truncated;
            for (const target of result.reqTargets)
                if (target.worker >= 0 && target.worker < script.workers.length &&
                    target.fn >= 0 && target.fn < script.workers[target.worker].entries.length)
                    pending.push({worker:target.worker, entry:target.fn, root:false,origin:node.origin});
        }
        limited ||= pending.length > 0;
        // A person may send requests into a story cutscene that changes maps.
        // Dialogue remains a talk target, rather than becoming an exit.
        const dialogue = calls.some(c => [0x64,0x13b,0x401d,0x225].includes(c.call));
        const chest = calls.some(c => c.call === 0x15b);
        const exit = worker.type !== 1 && warps.length > 0;
        const save=worker.type===1 && (models.get(event.source+':'+index)||[]).includes(0x5001);
        const reward=calls.some(c=>[0x136,0x15f,0x1a7,0x7002].includes(c.call));
        let category = exit ? 'Exit' : worker.type === 1 && chest ? 'Treasure Chest' :
            save ? 'Save Sphere' : worker.type === 1 && dialogue ? 'NPC' : dialogue || chest || reward ? 'Event' : null;
        if (!category) { excluded.push({worker:index, type:worker.type, reason:'no useful player-entry outcome'}); continue; }
        const known = warps.filter(w => Number.isInteger(w.room) && w.room >= 0 && Number.isInteger(w.spawn));
        const rooms = new Set(known.map(w => w.room));
        const room = !limited && known.length === warps.length && rooms.size === 1 ? known[0].room : -1;
        targets.push({worker:index, type:worker.type, category, room,
            calls, rootCalls, entries:save?rootEntries:rootEntries.filter(entry=>outcomes.get(entry)), limited, dependencies:[...seen]});
    }
    report.push({script:event.script, source:event.source, sha256:event.sha256,
        length:script.code.length, checksum:hash, targets, excluded});
}
const target = path.join(path.dirname(input), 'navigation_targets.json');
fs.writeFileSync(target, JSON.stringify(report));
console.log('Audited ' + report.length + ' scripts; ' +
    report.reduce((n,s)=>n+s.targets.length,0) + ' useful candidates, ' +
    report.reduce((n,s)=>n+s.excluded.length,0) + ' non-interactive workers excluded.');
