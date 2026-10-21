// Read-only audit of the retained, hashed player-entry graphs. No game access.
import fs from 'node:fs';
import path from 'node:path';
const root=path.resolve(import.meta.dirname,'../../..');
const code=fs.readFileSync(path.join(root,'src/navigation/atel.js'),'utf8');
const {scanWorker,decodeInstructions}=await import('data:text/javascript;base64,'+Buffer.from(code).toString('base64'));
const dir=path.join(import.meta.dirname,'output/FFX/20261003_audit');
const maps=JSON.parse(fs.readFileSync(path.join(dir,'maps_v3.json'),'utf8'));
const catalog=JSON.parse(fs.readFileSync(path.join(dir,'navigation_targets.json'),'utf8'));
const events=new Map(maps.events.map(e=>[e.source,e]));
const records=[];
for(const record of catalog) {
    const event=events.get(record.source);
    if(event.sha256!==record.sha256) throw Error('Source identity changed');
    const script={code:Uint8Array.from(Buffer.from(event.code,'hex')),workers:event.workers,scanLimit:Infinity};
    script.instructions=decodeInstructions(script.code);
    const exits=record.targets.filter(t=>t.category==='Exit').sort((a,b)=>a.worker-b.worker);
    for(const [ordinal,target] of exits.entries()) {
        const pending=target.entries.map(entry=>({worker:target.worker,entry})),seen=new Set(),warps=[];
        let incomplete=false;
        while(pending.length) {
            const node=pending.shift(),key=node.worker+':'+node.entry;
            if(seen.has(key)) continue;
            seen.add(key);
            const result=scanWorker(script,node.worker,[node.entry]);
            incomplete ||= result.truncated || result.unresolved;
            warps.push(...result.warps);
            for(const req of result.reqTargets) {
                if(req.worker>=0 && req.worker<script.workers.length && req.fn>=0 && req.fn<script.workers[req.worker].entries.length)
                    pending.push({worker:req.worker,entry:req.fn});
                else incomplete=true;
            }
        }
        const destinations=[...new Map(warps.map(w=>[w.room+':'+w.spawn,{room:w.room,spawn:w.spawn}])).values()];
        records.push({script:record.script,source:record.source,checksum:record.checksum,worker:target.worker,
            number:ordinal+1,total:exits.length,entries:target.entries,previousRoom:target.room,incomplete,destinations});
    }
}
const output=process.argv[2];
if(!output || fs.existsSync(output)) throw Error('Fresh output path required');
fs.writeFileSync(output,JSON.stringify({variants:catalog.length,exits:records},null,2)+'\n');
console.log(`Audited ${catalog.length} map variants, ${records.length} exit targets; retained exact room/arrival-point alternatives.`);
