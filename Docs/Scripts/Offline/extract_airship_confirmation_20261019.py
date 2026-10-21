"""Authorized read-only extraction of the airship confirmation's exact images.

Uses the existing VBF/Phyre readers; no game execution or archive modification.
Only hiku21..30 layer29 variants0/1 are retained, with provenance and pixel hashes.
"""

# Game-source inputs/exports live outside every Git tree.
from pathlib import Path as _EvidencePath
import sys as _evidence_sys
_evidence_repo = next(p for p in _EvidencePath(__file__).resolve().parents
                      if (p / "Docs/Scripts/Offline/evidence_paths.py").is_file())
_evidence_sys.path.insert(0, str(_evidence_repo / "Docs/Scripts/Offline"))
from evidence_paths import evidence_path, evidence_relative, write_replay_adapter

from pathlib import Path
from io import BytesIO
import hashlib
import json
import re
from PIL import Image, ImageDraw, ImageOps
from vbf import Vbf, output_path
from phyre_texture import texture_dds

ROOT = Path(__file__).resolve().parents[3]
SOURCE = Path('D:/Games/SteamLibrary/steamapps/common/FINAL FANTASY FFX&FFX-2 HD Remaster/data/FFX_Data.vbf')
OUT = evidence_path(ROOT / 'Docs/Scripts/Offline/output/airship_confirmation_20261019')
archive = Vbf(SOURCE)
pattern = re.compile(r'/map/hiku/hiku(2[1-9]|30)/2d/tex/d3d11/0_29_\d+_\d+_([01])\.dds\.phyre$', re.I)
entries = [(entry, pattern.search(entry.name)) for entry in archive.entries if pattern.search(entry.name)]
assert len(entries) == 20 and sum(e.size for e, _ in entries) < 5_000_000
rows = []
sheet = Image.new('RGB', (1100, 10 * 130), '#203040')
draw = ImageDraw.Draw(sheet)
for row, graphic in enumerate(range(21, 31)):
    for entry, match in sorted(entries, key=lambda pair: pair[0].name):
        if int(match[1]) != graphic:
            continue
        variant = int(match[2])
        data = archive.read(entry)
        target = output_path(OUT / 'files', entry.name)
        target.parent.mkdir(parents=True, exist_ok=True)
        target.write_bytes(data)
        dds, metadata = texture_dds(data)
        image = ImageOps.flip(Image.open(BytesIO(dds)).convert('RGBA'))
        png = OUT / f'hiku{graphic}-29-{variant}.png'
        image.save(png)
        preview = image.copy()
        preview.thumbnail((450, 100))
        x, y = variant * 550, row * 130
        draw.text((x + 5, y + 5), f'hiku{graphic} layer29 variant{variant}', fill='white')
        sheet.paste(preview, (x + 5, y + 25), preview)
        rows.append(dict(name=entry.name, bytes=len(data), sha256=hashlib.sha256(data).hexdigest(),
                         graphic=graphic, variant=variant, metadata=metadata,
                         decoded_rgba_sha256=hashlib.sha256(image.tobytes()).hexdigest()))
OUT.mkdir(parents=True, exist_ok=True)
sheet.save(OUT / 'contact-sheet.png')
(OUT / 'manifest.json').write_text(json.dumps(dict(source=str(SOURCE),
    authorization='October19 user authorization for offline airship graphics; narrowly extended to reported confirmation layer29.',
    orientation='Vertical flip follows native UV orientation.', files=rows), indent=2) + '\n', encoding='utf-8')
print(f'PASS: {len(rows)} exact confirmation assets, {sum(r["bytes"] for r in rows)} bytes; {OUT}')
