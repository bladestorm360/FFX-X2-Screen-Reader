"""User-approved offline airship graphics/map evidence; never executes game code."""

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
ARCHIVE = Path('D:/Games/SteamLibrary/steamapps/common/FINAL FANTASY FFX&FFX-2 HD Remaster/data/FFX_Data.vbf')
OUT = evidence_path(ROOT / 'Docs/Scripts/Offline/output/airship_assets_20261019')
OUT.mkdir(exist_ok=True)
archive = Vbf(ARCHIVE)
def selected_asset(name):
    name = name.lower()
    if '/hiku21/' in name or '/hiku2100/' in name:
        return True
    # Native 6403C0 substitutes maps22..30 by UI language. Retain only their
    # header and first list-row variants; other rows reuse the same label IDs.
    return any(f'/hiku{code}/2d/' in name for code in range(22, 31)) and (
        '/0_11_' in name or name.endswith('.ahwin32'))

selected = [entry for entry in archive.entries if selected_asset(entry.name)]
assert 0 < len(selected) < 1000 and sum(entry.size for entry in selected) < 100 * 1024 * 1024
records = []
for entry in selected:
    data = archive.read(entry)
    path = output_path(OUT / 'files', entry.name)
    path.parent.mkdir(parents=True, exist_ok=True)
    if path.exists():
        assert path.read_bytes() == data, 'preserve prior extracted evidence'
    else:
        path.write_bytes(data)
    records.append(dict(source=entry.name, file=evidence_relative(path, OUT).as_posix(),
                        bytes=len(data), sha256=hashlib.sha256(data).hexdigest()))
(OUT / 'manifest.json').write_text(json.dumps(dict(
    utc=datetime.now(timezone.utc).isoformat(), archive=str(ARCHIVE),
    authorization='User October19: you may read the graphics offline; airship graphics/maps only. No game execution.',
    attribution='Original Square Enix assets; VBF reader and Phyre decoder retain their source attribution.',
    assets=records), indent=2) + '\n', encoding='utf-8')
print(f'Retained {len(records)} airship-only assets, {sum(row["bytes"] for row in records)} bytes.')
