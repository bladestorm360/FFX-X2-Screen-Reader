"""Audit original Bevelle chest code and caller markers; retained data only.

Native inventory/UI/graphics and request acknowledgement remain modeled. The
integer/request replay stops at the original address operand before REQCHG;
the remaining straight-line tail and original RET are checked structurally.
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
from audit_trial_entries_20261016 import trace
from build_utilities import EVIDENCE, DESIGNS
from compile_trials import enrich
from field_text import TextEvidence
from atel_model import compile_script, instructions


def main():
    catalog=json.loads((EVIDENCE/'maps_v3.json').read_text(encoding='utf8'))
    text=TextEvidence(EVIDENCE/'text');records=[]
    for name in ['bvyt1000','bvyt1100','bvyt1200']:
        event=enrich(next(e for e in catalog['events'] if e['script']==name),EVIDENCE,text)
        flow=compile_script(event)
        grants=[dict(worker=w['index'],**s) for w in flow['workers'] for b in w['blocks'] for s in b['statements']
                if s['kind']=='call' and s.get('id') in [0x15b,0x1a7,0x1b7]]
        record=dict(script=name,source_sha256=event['sha256'],areas=event['area_names'],grants=grants)
        if name!='bvyt1100':
            assert not grants,'Unexpected adjoining-room reward; investigate before deployment'
            records.append(record);continue
        assert [(s['worker'],s['id'],s['args']) for s in grants]==[(16,0x15b,['1','217']),(17,0x15b,['1','102'])]
        decoded=instructions(bytes.fromhex(event['code']));chests=[]
        for local,entry,end,treasure in [(16,8,79770,217),(17,7,79938,102)]:
            start=event['workers'][local]['entries'][entry]
            body=[(pc,*decoded[pc]) for pc in decoded if start<=pc<=end]
            assert body[-1][1]==0x3c and not any(op in [0xb0,0xb1,0xb2,0xd5,0xd6,0xd7] for _,op,_,_ in body)
            stop=next(pc for pc,op,_,_ in body if op==0xa7)
            replay=trace(event,local,entry,0,0,stop)
            assert [s['args'] for s in replay['calls'] if s['id']==0x15b]==[[1,treasure]]
            assert (0,13,1) in replay['requests'] and (0,14,1) in replay['requests']
            chests.append(dict(worker=local,entry=entry,start=start,ret=end,treasure=treasure,
                               replay_to_native_acknowledgement=replay,instructions=body))
        statements={s['pc']:s for b in flow['workers'][15]['blocks'] for s in b['statements']}
        assert '01f4aa' in statements[79266]['condition'] and '== 0' in statements[79266]['condition']
        assert statements[79278]['opcode']=='REQEW' and statements[79278]['worker']=='17' and statements[79278]['entry']=='7'
        assert statements[79283]['binding']=='L6:map:01f4aa:T0' and statements[79283]['value']=='1'
        binding=next(b for b in flow['workers'][0]['bindings'] if b['key']=='L6:map:01f4aa:T0')
        assert (binding['raw_low'],binding['raw_high'])==(0x0c00002a,1)
        record.update(chests=chests,original_caller=[statements[pc] for pc in [79266,79278,79283]],marker_binding=binding)
        records.append(record)
    out=evidence_path(ROOT/'Docs/Scripts/Offline/output/bevelle_treasures_20261016');out.mkdir(exist_ok=True)
    path=out/('audit_'+datetime.now(timezone.utc).strftime('%Y%m%d_%H%M%S')+'.json')
    path.write_text(json.dumps(dict(checked_at_utc=datetime.now(timezone.utc).isoformat(),records=records,
        map_catalog_sha256=hashlib.sha256((EVIDENCE/'maps_v3.json').read_bytes()).hexdigest(),
        limitations=__doc__),indent=2)+'\n',encoding='utf8')
    print('PASS: original Bevelle has two native reward chests (217,102); branch chest W17:7 RET79938 follows its W15 guard/REQEW/post-return marker. Adjacent approach/antechamber have no reward calls. Native grant/UI/acknowledgement remain modeled.')
    print(evidence_relative(path, ROOT))


if __name__=='__main__':main()
