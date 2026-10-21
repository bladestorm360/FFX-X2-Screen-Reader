// Archived ATEL only. Never attaches to a game or opens its installation.
import fs from 'node:fs';
import path from 'node:path';
const root=new URL('../../../',import.meta.url);
const directory=new URL('./output/FFX/20261003_audit/',import.meta.url);
const audit=JSON.parse(fs.readFileSync(new URL('navigation_targets.json',directory),'utf8'));
const maps=JSON.parse(fs.readFileSync(new URL('maps_v3.json',directory),'utf8'));
const source=fs.readFileSync(new URL('src/navigation/atel.js',root),'utf8');
const {scanWorker}=await import('data:text/javascript;base64,'+Buffer.from(source).toString('base64'));
const exits=[];
for(const record of audit) for(const target of record.targets) if(target.category==='Exit') {
    if(!target.entries.length || target.entries.some(entry=>![2,4].includes(entry))) throw new Error('Unvalidated exit entry');
    exits.push({script:record.script,worker:target.worker,type:target.type,room:target.room,
        usefulEntries:target.entries,entryMask:target.entries.reduce((mask,entry)=>mask|(1<<entry),0),limited:target.limited});
}
const agency=maps.events.find(event=>event.script==='kami0100');
if(!agency) throw new Error('Agency evidence missing');
const script={code:Uint8Array.from(Buffer.from(agency.code,'hex')),workers:agency.workers};
const workers=[21,22,23,24].map(index=>{
    const init=scanWorker(script,index,[0]);
    const target=audit.find(record=>record.script===agency.script).targets.find(item=>item.worker===index);
    const calls=[]; scanWorker(script,index,[0],call=>calls.push(call));
    return {worker:index,type:agency.workers[index].type,floats:agency.workers[index].floats,
        initCalls:calls,initWarps:init.warps,target};
});
const report={utc:new Date().toISOString(),scripts: audit.length,exits:exits.length,
    runtimeRule:'Only useful outcomes reachable through native enabled channels and the current request-entry mapping are listed. Actor visibility alone never implies talk availability.',
    geometryRule:'Route to reachable native talk circles/rotated rectangles or across line/rectangle triggers. Respect range and height; no route is fabricated outside the connected floor.',
    promptRule:'Field prompts are spoken by actual native text-window installation/page events. Catalog inference supplies neutral identities/services, never remote dialogue or proximity guesses.',
    agency:{script:agency.script,workers,
        conclusion:'Worker 24 has a real outgoing crossing entry 4 to room 264. Worker 23 crosses to room 256. Archived init configures geometry/request state; this is not evidence that either is enabled in the current story state. Runtime flags/request mappings and reachable floor determine usability; no one-way/story-only claim is assumed.'},
    crossingExits:exits,
    limitation:'Offline audit does not execute arbitrary ATEL story predicates or certify every live transition. Live availability and geometry remain acceptance checks.'};
const stamp=report.utc.replace(/[-:]/g,'').replace(/T/,'_').slice(0,15);
const destination=new URL('navigation_access_'+stamp+'.json',directory);
fs.writeFileSync(destination,JSON.stringify(report,null,2)+'\n');
console.log(`Audited ${report.scripts} scripts and ${report.exits} exit workers; ${path.basename(destination.pathname)}.`);
