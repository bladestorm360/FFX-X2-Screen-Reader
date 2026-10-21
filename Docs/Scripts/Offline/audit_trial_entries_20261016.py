"""Bounded replay of original trial entry bytecode, using retained assets only.

Native graphics/camera/wait calls are recorded, not run. This establishes branch
selection and requested native effects, not rendered/gameplay confirmation.
Opcode/operand conventions follow the existing puzzles/tile_paths.py harness.
"""

# Game-source inputs/exports live outside every Git tree.
from pathlib import Path as _EvidencePath
import sys as _evidence_sys
_evidence_repo = next(p for p in _EvidencePath(__file__).resolve().parents
                      if (p / "Docs/Scripts/Offline/evidence_paths.py").is_file())
_evidence_sys.path.insert(0, str(_evidence_repo / "Docs/Scripts/Offline"))
from evidence_paths import evidence_path, evidence_relative, write_replay_adapter

import hashlib
import json
import sys
from datetime import datetime, timezone
from pathlib import Path

ROOT=Path(__file__).resolve().parents[3]
sys.path.insert(0,str(ROOT/'src/puzzles'))
from build_utilities import EVIDENCE, DESIGNS
from compile_trials import enrich
from field_text import TextEvidence
from atel_model import instructions, signatures, binding, REQS, ENDS

def trace(event,local,entry,story,spawn,stop_pc=None):
    decoded=instructions(bytes.fromhex(event['code']));calls,_=signatures();w=event['workers'][local]
    stack=[];registers={0x25:0,0x2a:0,0x2c:0};values={('L0:map:000a00:T2',0):story}
    observed=[];requests=[];visited=[];writes=[]
    def pop():
        assert stack,'original operand stack underflow'
        return stack.pop()
    pc=w['entries'][entry]
    for _ in range(2000):
        visited.append(pc)
        if pc==stop_pc:return dict(visited=visited,calls=observed,requests=requests,writes=writes,end_pc=pc)
        op,arg,nxt=decoded[pc]
        if op in (0xae,0xad,0xaf):stack.append((arg if arg<32768 else arg-65536) if op==0xae else w['ints' if op==0xad else 'floats'][arg])
        elif op in (0x9f,0xa2):
            element=pop() if op==0xa2 else 0;stack.append(values.get((binding(w,arg),element),0))
        elif op in (0xa0,0xa1,0xa3,0xa4):
            value=pop();element=pop() if op in (0xa3,0xa4) else 0;key=binding(w,arg)
            writes.append((pc,key,element,value));values[key,element]=value
        elif 1<=op<=24:
            right,left=pop(),pop()
            operations={1:lambda:left or right,2:lambda:left and right,3:lambda:left|right,4:lambda:left^right,5:lambda:left&right,
                6:lambda:left==right,7:lambda:left!=right,8:lambda:left>right,9:lambda:left<right,10:lambda:left>right,11:lambda:left<right,
                12:lambda:left>=right,13:lambda:left<=right,14:lambda:left>=right,15:lambda:left<=right,16:lambda:left|right,
                17:lambda:left&~right,18:lambda:left<<right,19:lambda:left>>right,20:lambda:left+right,21:lambda:left-right,
                22:lambda:left*right,23:lambda:left/right,24:lambda:left%right}
            stack.append(operations[op]())
        elif op in registers:registers[op]=pop()
        elif op in (0x26,0x28,0x29):stack.append(registers[{0x26:0x25,0x28:0x2a,0x29:0x2c}[op]])
        elif op==0x2b:stack.append(stack[-1])
        elif op in (0xb0,0xb1,0xb2,0xd5,0xd6,0xd7):
            condition=pop() if op in (0xd5,0xd6,0xd7) else registers[0x2a]
            if op in (0xb0,0xd5) or bool(condition)==(op in (0xb1,0xd6)):nxt=w['jumps'][arg]
        elif op in (0xb5,0xd8):
            signature=calls.get(arg);assert signature and signature['agree']=='yes','unverified native call signature'
            arguments=[pop() for _ in range(int(signature['arg_count_kari']))][::-1]
            observed.append(dict(pc=pc,id=arg,args=arguments))
            if arg==0x5f:return dict(visited=visited,calls=observed,requests=requests,writes=writes,end_pc=pc)
            result=spawn if arg==0x10 else 0
            if op==0xb5:stack.append(result)
            else:registers[0x25]=result
        elif op in REQS:
            target_entry,target,level=pop(),pop(),pop();requests.append((target,target_entry,level));stack.append(1)
        elif op in ENDS:return dict(visited=visited,calls=observed,requests=requests,writes=writes,end_pc=pc)
        elif op in (0,0x9d,0x9e,0x55,0x77):pass
        else:raise ValueError(f'unmodeled original opcode {op:02x} at {pc:x}')
        pc=nxt
    raise ValueError('original entry exceeded the2000-instruction work bound')

def main():
    catalog=json.loads((EVIDENCE/'maps_v3.json').read_text(encoding='utf8'))
    text=TextEvidence(EVIDENCE/'text');records=[]
    for script in ['mcyt0500','bvyt1100']:
        event=enrich(next(e for e in catalog['events'] if e['script']==script),EVIDENCE,text)
        record=dict(script=script,source_sha256=event['sha256'],bytecode_sha256=hashlib.sha256(bytes.fromhex(event['code'])).hexdigest())
        if script=='mcyt0500':
            assert len(event['entrances'])==3
            cases=[]
            for story,spawn in [(0,0),(95,0),(1529,0),(1530,0),(1550,0),(1550,1),(1550,2)]:
                result=trace(event,30,1,story,spawn,58983)
                hidden={c['args'][0] for c in result['calls'] if c['id']==0x8000 and c['args'][1]==0}
                assert {8,9,10,11}.issubset(hidden)==(story>=1550 and spawn==1)
                assert (58983 in result['visited'])==(story<1530)
                cases.append(dict(story=story,entrance=spawn,result=result))
            record['cases']=cases
            print('PASS: original Macalania W30:1 selects playable bridge removal only at story>=1550/entrance1; early story enters its pre-trial sequence.')
        else:
            w=event['workers'][39];assert w['type']==2 and w['entries'][6]==116381
            result=trace(event,39,6,0,0);assert result['end_pc']==116484
            assert result['calls'][0]['id']==0x5e and (13,6,3) in result['requests']
            assert not any(s['kind']=='request' and s['worker']=='39' and s['entry']=='6'
                for worker in json.loads((DESIGNS/'bevelle_flow.json').read_text(encoding='utf8'))['workers'] for block in worker['blocks'] for s in block['statements'])
            record['rail_crossing']=result
            print('PASS: original Bevelle type2 W39:6 locks the player and requests rail pedestal13:6; original RET116484 safely precedes all ride effects when substituted at initialization.')
        records.append(record)
    folder=evidence_path(ROOT/'Docs/Scripts/Offline/output/trial_entries_20261016');folder.mkdir(exist_ok=True)
    target=folder/('replay_'+datetime.now(timezone.utc).strftime('%Y%m%d_%H%M%S')+'.json')
    target.write_text(json.dumps(dict(checked_at_utc=datetime.now(timezone.utc).isoformat(),records=records,
        limitations='Original integer/branch/request bytecode replay. Graphics, camera, waits, requested workers and actual native collision dispatch/rendering are modeled dependencies, not gameplay proof.'),indent=2)+'\n',encoding='utf8')
    print(target)

if __name__=='__main__':main()
