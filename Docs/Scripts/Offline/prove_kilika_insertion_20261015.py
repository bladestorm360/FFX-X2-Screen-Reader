"""Trace the original accepted/cancelled insertion, not a substitute solver VM.

Original ATEL control-flow/expressions come from the archived script compiler.
Native scheduler acknowledgements, animation completion and player Confirm are
controlled boundaries here. This establishes ordering/preconditions, not the
tester's historical runtime PC or gameplay success.
"""

# Game-source inputs/exports live outside every Git tree.
from pathlib import Path as _EvidencePath
import sys as _evidence_sys
_evidence_repo = next(p for p in _EvidencePath(__file__).resolve().parents
                      if (p / "Docs/Scripts/Offline/evidence_paths.py").is_file())
_evidence_sys.path.insert(0, str(_evidence_repo / "Docs/Scripts/Offline"))
from evidence_paths import evidence_path, evidence_relative, write_replay_adapter

import ast
import hashlib
import json
import re
import sys
from pathlib import Path

ROOT=Path(__file__).resolve().parents[3]
sys.path.insert(0,str(ROOT/'src/puzzles'))
from build_utilities import RECIPES
folder=evidence_path(ROOT/'Docs/Scripts/Offline/output/report_20261015')
workers={w['index']:w for w in json.loads((folder/'klyt1000.json').read_text())}
recipe=next(r for r in RECIPES if r[0]=='kilika')
assert recipe[1:3]==(35,51412)
assert recipe[4]==[('L6:map:010c60:T0',0,0),('L6:map:010c61:T0',0,0),('L6:map:010c5b:T0',6,0)]

def trace(choice,start=51412):
    values={key:value for key,value,element in recipe[4]}
    registers=dict(A=0,X=0,Y=0);calls={};events=[];pcs=[]
    blocks={b['start']:b for b in workers[35]['blocks']}
    # The old tail is an interior empty-stack instruction boundary.
    if start==53283:
        original=blocks[53277]
        blocks[start]=dict(original,start=start,statements=[s for s in original['statements'] if s['pc']>=start])
    def value(expression):
        expression=re.sub(r'(L\d:[^\[\s()]+)\[0\]',lambda m:str(values.get(m[1],0)),expression)
        expression=re.sub(r'call@([0-9a-f]+)',lambda m:str(calls[int(m[1],16)]),expression)
        expression=re.sub(r'request@[0-9a-f]+','1',expression)
        expression=re.sub(r'\?([AXY])',lambda m:str(registers[m[1]]),expression)
        expression=expression.replace('true','True').replace('!','not ')
        tree=ast.parse(expression.strip(),mode='eval')
        allowed=(ast.Expression,ast.Constant,ast.Compare,ast.Eq,ast.NotEq,ast.UnaryOp,ast.Not,ast.USub,ast.BinOp,ast.BitOr,ast.BitAnd,ast.Add,ast.Sub,ast.Mult,ast.Div,ast.Load)
        assert all(isinstance(node,allowed) for node in ast.walk(tree)),expression
        return eval(compile(tree,'<original ATEL expression>','eval'),{'__builtins__':{}},{})
    pc=start
    for _ in range(200):
        b=blocks[pc];pcs.append(pc)
        for s in b['statements']:
            kind=s['kind']
            if kind=='write':values[s['binding']]=value(s['value']);events.append(['write',s['pc'],s['binding'],values[s['binding']]])
            elif kind=='register':registers[s['register']]=value(s['value'])
            elif kind=='call':
                args=[value(a) for a in s['args']];events.append(['call',s['pc'],s['id'],args])
                calls[s['pc']]=choice if s['id']==0x13b else 1 if s['id']==0x8f else 0
            elif kind=='request':events.append(['request',s['pc'],int(s['worker']),int(s['entry']),s['opcode']])
            elif kind not in ['branch','end']:raise AssertionError(s)
        if not b['edges']:return dict(choice=choice,start=start,pcs=pcs,events=events,values=values)
        edges=[e for e in b['edges'] if value(e['when'])]
        assert len(edges)==1,(pc,edges)
        pc=edges[0]['target']
    raise AssertionError('Original path exceeded the finite trace bound')

accepted,cancelled,old=trace(0),trace(1),trace(0,53283)
requests=[(e[2],e[3]) for e in accepted['events'] if e[0]=='request']
assert [(5,e) for e in [8,19,6,9,21,22]]==[r for r in requests if r[0]==5]
assert accepted['values']['L6:map:010c61:T0']==6 and accepted['values']['L6:map:010c5b:T0']==0
calls=[(e[2],e[3]) for e in accepted['events'] if e[0]=='call']
assert (0x4017,[54,0]) in calls and (0x5e,[]) in calls and calls[-1]==(0x5d,[])
assert (0x64,[0,17]) in calls and (0x7c,[0]) in calls
assert not any(e[0]=='call' and e[2]==0x15b for e in accepted['events']) # No item grant in reveal.
assert not any(e[0]=='call' and e[2]==0x4017 for e in cancelled['events'])
assert cancelled['values']['L6:map:010c61:T0']==0
old_requests=[(e[2],e[3]) for e in old['events'] if e[0]=='request']
assert old_requests==[(5,21),(5,22)]
helpers={str(entry):next(b for b in workers[5]['blocks'] if b['start']==workers[5]['entries'][entry]) for entry in [6,8,9,19,21,22]}
assert any(s.get('id')==0x502d for s in helpers['8']['statements']) # Player attachment.
assert any(s.get('id')==0x8009 for s in helpers['6']['statements']) # Matching GFX68 bind.
assert any(s.get('id')==0x019d for s in helpers['21']['statements']) # Later detach.
record=dict(recipe=recipe,accepted=accepted,cancelled=cancelled,old_tail=old,original_helpers=helpers,
    model_sha256=hashlib.sha256((folder/'klyt1000.json').read_bytes()).hexdigest(),
    boundaries='Compiled original bytecode expressions; native requests acknowledged, timers/graphics finish and player confirms. No native scene/scheduler/gameplay claim; exact historical freeze PC unknown.')
(folder/'kilika-full-insertion-proof.json').write_text(json.dumps(record,indent=2))
print('PASS: original full Kilika entry, accepted/cancelled branches, attach-position-bind before detach, native placement/message/wall54/control return and no premature item grant. Old tail omits all four setup requests. Gameplay pending.')
