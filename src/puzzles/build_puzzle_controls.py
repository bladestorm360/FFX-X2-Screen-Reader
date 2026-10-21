"""Compile opt-in interaction interception and route targets from hashed ATEL.

No game access. A redirected interaction returns at its own original RET.
The native request-completion hook then schedules original reveal/chest scripts.
"""
from pathlib import Path
from datetime import datetime, timezone
import json, hashlib
import sys
from build_utilities import EVIDENCE, DESIGNS, ROOT, binding, cpp_binding, literal
from compile_trials import reachable
from atel_model import instructions
from puzzle_routes import ROUTES
from compile_trials import enrich
from build_utilities import TextEvidence
from tile_paths import compile_tiles
sys.path.insert(0, str(ROOT / "src/navigation"))
from finish_targets import TRIAL_LABELS

# Native player interactions, not arbitrary REQ helpers or camera workers.
INTERACTIONS = {
 'besaid': (list(range(37,47))+[7], {43}),
 'kilika': (list(range(28,37))+[6,41], {34}),
 'djose': (list(range(48,65))+[1,6], {64}),
 'macalania': (list(range(37,44))+[1,46,47], set()),
 'bevelle': (list(range(30,38))+[39], set()),
 'zanarkand': (list(range(44,53))+list(range(56,85)), {50}),
}

def position(flow, local):
    points=[]
    for block in reachable(flow['workers'][local],0):
        for s in block['statements']:
            if s['kind']=='call' and s['id'] in (0x13,0x57,0x126):
                try: points.append(list(map(float,s['args'])))
                except ValueError: pass
            if s['kind']=='call' and s['id']==0x54:
                try:
                    coords=list(map(float,s['args']))
                    points.append([(coords[i]+coords[i+3])/2 for i in range(3)])
                except ValueError: pass
    unique=list({tuple(p):p for p in points if len(p)==3}.values())
    # Conditional model spawn positions must come from the live actor. Only
    # an unambiguous original literal position is a safe static fallback.
    if len(unique)==1: return unique[0]
    return [0,0,0]

def main():
    maps=json.loads((EVIDENCE/'maps_v3.json').read_text(encoding='utf8'))
    rows, route_rows, records = [],[],[]
    for name,(locals_, destruction) in INTERACTIONS.items():
        flow=json.loads((DESIGNS/(name+'_flow.json')).read_text(encoding='utf8'))
        design=json.loads((DESIGNS/(name+'_design.json')).read_text(encoding='utf8'))
        event=next(e for e in maps['events'] if e['script']==flow['script'])
        if event['sha256']!=flow['sha256'] or hashlib.sha256((EVIDENCE/'files'/event['source']).read_bytes()).hexdigest()!=flow['sha256']:
            raise ValueError('Original script changed')
        code=bytes.fromhex(event['code']); decoded=instructions(code)
        interactions=[]
        for local in locals_:
            worker=flow['workers'][local]
            # The entrance rail pedestal crosses W39's native edge, entry6;
            # its talk entry2 is an empty RET and never starts the ride.
            entry=6 if name=='bevelle' and local==39 else 2
            blocks=reachable(worker,entry)
            returns=[s['pc'] for b in blocks for s in b['statements'] if s['kind']=='end' and s['opcode']=='RET']
            if not returns: continue # an empty/looping platform producer is not an interaction
            ret=returns[-1]
            assert decoded[ret][0]==0x3c
            if worker['entries'][entry]==ret: continue
            interactions.append((local,entry,worker['entries'][entry],ret,local in destruction))
        carried=next(f['key'] for f in design['fields'] if f['role']=='carried')
        declaration=binding(flow,carried,0)
        sphere=10 if name in ('djose','macalania') else 6 if name=='kilika' else 3
        if name=='kilika' and not any(s['kind']=='write' and s.get('binding')=='L6:map:010c60:T0' and s.get('value')=='6'
            for b in flow['workers'][34]['blocks'] for s in b['statements']):
            raise ValueError('Kilika Destruction Sphere identity changed')
        if name=='bevelle':
            # Carried is a boolean; Destruction identity is matrix row five.
            declaration=binding(flow,'L6:map:01f4b2:T0',0,61); sphere=1
        def sphere_bindings(a):
            worker=a[0]; empty=dict(low=0,high=1,value=0,element=0)
            if name=='bevelle': return [binding(flow,'L6:map:01f4b2:T0',1,{35:71,36:69,37:70}.get(worker,64+worker-30)),empty] if worker!=39 else [empty,empty]
            keys={
                'besaid':{39:'011e34',40:'011e38',41:'011e38',42:'011e3c',43:'011e40',44:'011e50',46:'011e44',7:'011e50'},
                'kilika':{28:'010c58',29:'010c59',30:'010c5c',31:'010c5d',32:'010c5e',33:'010c5f',34:'010c60',35:'010c61',36:'010c62',6:'010c5a'},
                'djose':{**{w:format(0x181e5+w-48,'06x') for w in range(48,56)},1:'0181ed',6:'0181f0',62:'0181ef'},
                'macalania':{1:'015a8f',37:'015a8d',38:'015a8e',39:'015a90',40:'015a91',41:'015a92',42:'015a93',43:'015a95'},
                'zanarkand':{48:'00f6b4',49:'00f6b0',50:'00f6b8',51:'00f6bc',52:'00f6c0'},
            }.get(name,{})
            key=keys.get(worker)
            kind='T5' if name in ('besaid','zanarkand') else 'T0'
            first=binding(flow,'L6:map:'+key+':'+kind,sphere) if key else empty
            second=binding(flow,'L6:map:0181ee:T0',sphere) if name=='djose' and worker==1 else empty
            return [first,second]
        rows.append('    {%s, %s, %d, {%s}},' % (literal(flow['script']),cpp_binding(declaration),sphere,
          ', '.join('{%d, %d, %d, %d, %s, %s, %s}'%(*a[:4],str(a[4]).lower(),*(cpp_binding(b) for b in sphere_bindings(a))) for a in interactions)))
        if not interactions: raise ValueError('Trial has no physical interaction contracts')
        steps, evidence=[] ,[]
        labels={r["worker"]:r["label"] for r in json.loads(sorted(EVIDENCE.glob("navigation_audit_*.json"))[-1].read_text(encoding="utf8"))["labels"] if r["script"]==flow["script"]}
        # Authored trial labels are the maintained authority for worker roles;
        # newer audit proposals must not silently restore a corrected mapping.
        labels.update(TRIAL_LABELS.get(flow["script"], {}))
        fields={f['role']:f['key'] for f in design['fields']}
        for text,worker,role,value,platform in ROUTES[name]:
            # Sphere model workers animate; separate collision workers receive
            # the player's interaction and have the actual sphere positions.
            if worker>=len(flow['workers']): raise ValueError('Missing route target')
            if worker not in labels or not labels[worker]: raise ValueError("Guide target absent from pathfinder: "+str(worker))
            text="Target: "+labels[worker]+". "+text
            point=position(flow,worker)
            done=binding(flow,fields[role],value) if role else dict(low=0,high=1,value=0,element=0)
            steps.append('        {L%s, %d, {%s}, %s, %s, %s, %s, L%s},' %
             (literal(text),worker,', '.join(repr(v) for v in point),cpp_binding(done),str(bool(role)).lower(),str(platform).lower(),str(role in ("entry_glyph","entry_open","door_open","hidden_recess_open","symbols_touched","treasure_revealed","temple_treasure")).lower(),literal(labels[worker])))
            evidence.append(dict(instruction=text,worker=worker,position=point,guard=done if role else None,platform=platform))
        route_rows.append('    {%s, {\n%s\n    }, %d},' % (literal(flow['script']),'\n'.join(steps),11 if name=='zanarkand' else 0))
        records.append(dict(script=flow['script'],sha256=flow['sha256'],interactions=interactions,
            destructionId=sphere,storedSphereBindings=[dict(worker=a[0],bindings=sphere_bindings(a)) for a in interactions],steps=evidence))
    native=ROOT/'src/native'
    (native/'puzzle_solver_data.hpp').write_text('// Generated from hashed original ATEL.\n#pragma once\nnamespace ffx {\ninline const PuzzleContract puzzleContracts[] = {\n'+'\n'.join(rows)+'\n};\n}\n',encoding='utf8')
    (native/'puzzle_guide_data.hpp').write_text('// Verified targets; paths use current native collision geometry.\n#pragma once\nnamespace ffx {\ninline const PuzzleRoute puzzleRoutes[] = {\n'+'\n'.join(route_rows)+'\n};\n}\n',encoding='utf8')
    floor_event=enrich(next(e for e in maps['events'] if e['script']=='dome0700'),EVIDENCE,TextEvidence(EVIDENCE/'text'))
    floors=compile_tiles(floor_event); floor_flow=json.loads((DESIGNS/'zanarkand_flow.json').read_text(encoding='utf8'))
    tile_rows=[]
    for group in floors:
        for tile in group['tiles']:
            blocks=[]
            for key,element,value in tile['trace']['writes']:
                if value==1 and key in ('L6:map:00f6c7:T0','L6:map:00f6c9:T0','L6:map:00f6cb:T0'):
                    blocks.append(cpp_binding(binding(floor_flow,key,1,element)))
            tile_rows.append('    {%d, %d, {%s}, {%s}},'%(group['stage'],tile['worker'],
              ', '.join(repr(v) for v in position(floor_flow,tile['worker'])),', '.join(blocks)))
    (native/'puzzle_tiles_data.hpp').write_text('// Permitted original branches, including native duplicate-shape guards.\n#pragma once\nnamespace ffx {\ninline const PuzzleTile puzzleTiles[] = {\n'+'\n'.join(tile_rows)+'\n};\n}\n',encoding='utf8')
    stamp=datetime.now(timezone.utc).strftime('%Y%m%d_%H%M%S')
    report=EVIDENCE/('puzzle_controls_'+stamp+'.json')
    report.write_text(json.dumps(dict(trials=records,floorPatterns=floors,policy='Player opt-in only. Action order/path/interaction behavior requires live validation.'),indent=2)+'\n',encoding='utf8')
    print('Compiled six interaction contracts and %d guide actions. %s'%(sum(len(r['steps']) for r in records),report.name))
if __name__=='__main__': main()
