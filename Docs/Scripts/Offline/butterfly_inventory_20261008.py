
# Game-source inputs/exports live outside every Git tree.
from pathlib import Path as _EvidencePath
import sys as _evidence_sys
_evidence_repo = next(p for p in _EvidencePath(__file__).resolve().parents
                      if (p / "Docs/Scripts/Offline/evidence_paths.py").is_file())
_evidence_sys.path.insert(0, str(_evidence_repo / "Docs/Scripts/Offline"))
from evidence_paths import evidence_path, evidence_relative, write_replay_adapter

from pathlib import Path
import sys,json,hashlib
ROOT=Path.cwd()
sys.path.insert(0,str(ROOT/'Docs/Scripts/Offline'))
sys.path.insert(0,str(ROOT/'src/puzzles'))
from atel_model import compile_script
from cloister_inventory import script_chunk,declarations
from field_text import TextEvidence,u32
archive=evidence_path(ROOT/'Docs/Scripts/Offline/output/FFX/20261003_audit')
out=evidence_path(ROOT/'Docs/Scripts/Offline/output/butterfly_20261008')
out.mkdir(exist_ok=True)
text=TextEvidence(archive/'text')
for event in json.loads((archive/'maps_v3.json').read_text())['events']:
 if event['script'] not in ('mcfr0100','mcfr0200'):continue
 source=archive/'files'/event['source'];raw=source.read_bytes()
 assert hashlib.sha256(raw).hexdigest()==event['sha256']
 chunk=script_chunk(raw)
 workers=[]
 for i,worker in enumerate(event['workers']):
  header=u32(chunk,0x38+i*4)
  workers.append(dict(worker,index=i,declarations=declarations(chunk,header),private_data_offset=u32(chunk,header+0x2c),shared_data_offset=u32(chunk,header+0x30)))
 script=dict(event,workers=workers,event_data_offset=u32(chunk,0x20),english_strings=text.field_strings(source),area_names=[text.macro(11,a) for a in event["area_indices"]])
 (out/(source.stem+'.json')).write_text(json.dumps(dict(script=script,compiled=compile_script(script)),ensure_ascii=False),encoding='utf8')
 print(event['script'],len(workers),event['source'])