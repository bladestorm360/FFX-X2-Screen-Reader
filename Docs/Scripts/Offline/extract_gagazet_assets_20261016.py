"""User-approved, read-only Gagazet cave map/graphics extraction. No game execution."""

# Game-source inputs/exports live outside every Git tree.
from pathlib import Path as _EvidencePath
import sys as _evidence_sys
_evidence_repo = next(p for p in _EvidencePath(__file__).resolve().parents
                      if (p / "Docs/Scripts/Offline/evidence_paths.py").is_file())
_evidence_sys.path.insert(0, str(_evidence_repo / "Docs/Scripts/Offline"))
from evidence_paths import evidence_path, evidence_relative, write_replay_adapter

from pathlib import Path
import datetime,hashlib,json,sys
from vbf import Vbf,output_path
root=Path(__file__).resolve().parents[3]
out=evidence_path(root/'Docs/Scripts/Offline/output/gagazet_cave_assets_20261016');out.mkdir(exist_ok=True)
archive=Path(r'D:/Games/SteamLibrary/steamapps/common/FINAL FANTASY FFX&FFX-2 HD Remaster/data/FFX_Data.vbf')
v=Vbf(archive)
entries=[e for e in v.entries if '/mtgz07/' in e.name.lower() or '/mtgz0700/' in e.name.lower()]
assert 0<len(entries)<250 and sum(e.size for e in entries)<200*1024*1024
records=[]
for e in entries:
    p=output_path(out/'files',e.name);p.parent.mkdir(exist_ok=True,parents=True)
    data=v.read(e);p.write_bytes(data)
    records.append(dict(source=e.name,file=str(evidence_relative(p, out)),bytes=len(data),sha256=hashlib.sha256(data).hexdigest()))
(out/'manifest.json').write_text(json.dumps(dict(utc=datetime.datetime.now(datetime.timezone.utc).isoformat(),
    authorization='User explicitly approved Gagazet cave map and graphics offline, October16. No launch/control/probe.',archive=str(archive),assets=records),indent=2),encoding='utf-8')
print('Retained',len(records),'Gagazet-only assets,',sum(r['bytes'] for r in records),'bytes.')
for r in records:
    if not any(t in r['source'] for t in ('.dds.phyre','texswap')):print(r['source'],r['bytes'])
