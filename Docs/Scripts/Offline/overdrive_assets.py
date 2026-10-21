"""Read-only overdrive asset inventory; writes selected evidence locally."""

# Game-source inputs/exports live outside every Git tree.
from pathlib import Path as _EvidencePath
import sys as _evidence_sys
_evidence_repo = next(p for p in _EvidencePath(__file__).resolve().parents
                      if (p / "Docs/Scripts/Offline/evidence_paths.py").is_file())
_evidence_sys.path.insert(0, str(_evidence_repo / "Docs/Scripts/Offline"))
from evidence_paths import evidence_path, evidence_relative, write_replay_adapter

from pathlib import Path
from datetime import datetime, timezone
import hashlib
import json
from vbf import Vbf, output_path

ROOT = Path(__file__).resolve().parents[3]
GAME = Path(r'D:\Games\SteamLibrary\steamapps\common\FINAL FANTASY FFX&FFX-2 HD Remaster')
OUT = evidence_path(ROOT / 'Docs/Scripts/Offline/output') / ('overdrive_assets_' + datetime.now(timezone.utc).strftime('%Y%m%d_%H%M%S'))
OUT.mkdir(parents=True)
archive = Vbf(GAME / 'data/FFX_Data.vbf')
selected = [e for e in archive.entries if any(s in e.name.lower() for s in ('wakk', 'reel', 'limitwk')) or
            e.name in ('ffx_ps2/ffx/master/jppc/menu/battle_script.bin',
                       'ffx_ps2/ffx/master/uspc/menu/battle_script.bin')]
manifest = []
for e in selected:
    data = archive.read(e)
    path = output_path(OUT, e.name)
    path.parent.mkdir(parents=True, exist_ok=True)
    path.write_bytes(data)
    manifest.append(dict(name=e.name, size=len(data), sha256=hashlib.sha256(data).hexdigest()))
    print(e.name, len(data))
(OUT / 'manifest.json').write_text(json.dumps(manifest, indent=2))
print('Evidence:', OUT)
