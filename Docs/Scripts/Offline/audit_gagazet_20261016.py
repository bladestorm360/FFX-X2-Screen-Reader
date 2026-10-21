"""Locate named Gagazet cave puzzles using retained map/scripts/text only."""

# Game-source inputs/exports live outside every Git tree.
from pathlib import Path as _EvidencePath
import sys as _evidence_sys
_evidence_repo = next(p for p in _EvidencePath(__file__).resolve().parents
                      if (p / "Docs/Scripts/Offline/evidence_paths.py").is_file())
_evidence_sys.path.insert(0, str(_evidence_repo / "Docs/Scripts/Offline"))
from evidence_paths import evidence_path, evidence_relative, write_replay_adapter

from pathlib import Path
import json,sys,hashlib
root=Path(__file__).resolve().parents[3]
sys.path.insert(0,str(root/'src/puzzles'))
from compile_trials import enrich
from atel_model import compile_script,render
from field_text import TextEvidence
from extract_maps import room_index
base=evidence_path(root/'Docs/Scripts/Offline/output/FFX/20261003_audit')
catalog=json.loads((base/'maps_v3.json').read_text(encoding='utf-8'))
text=TextEvidence(base/'text');rooms=room_index()
out=evidence_path(root/'Docs/Scripts/Offline/output/gagazet_cave_20261016');out.mkdir(exist_ok=True)
found=[]
for event in catalog['events']:
    if not event['script'].startswith('mtgz'):continue
    script=enrich(event,base,text)
    native=script['english_strings']
    hints=[dict(index=i,text=s) for i,s in enumerate(native) if any(word in s.lower() for word in ('trial','throw','target','circle','green','orange','blue','gagazet'))]
    record=dict(script=event['script'],sha256=event['sha256'],source=event['source'],areas=script['area_names'],strings=hints)
    found.append(record)
    if event['script'] in ('mtgz0600','mtgz0700','mtgz0800'):
        model=compile_script(script)
        (out/(event['script']+'.json')).write_text(json.dumps(model,indent=2),encoding='utf-8')
        (out/(event['script']+'.txt')).write_text(render(model),encoding='utf-8')
(out/'catalog.json').write_text(json.dumps(dict(source=str(evidence_relative(base, root)),catalog_sha256=hashlib.sha256((base/'maps_v3.json').read_bytes()).hexdigest(),rooms=rooms,events=found),indent=2),encoding='utf-8')
for r in found:
    print(r['script'],r['areas'],'strings',[(s['index'],s['text'][:160]) for s in r['strings']])
