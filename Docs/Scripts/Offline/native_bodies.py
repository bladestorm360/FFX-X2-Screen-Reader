"""Read existing decompile archives; never run Ghidra or touch a game install."""

# Game-source inputs/exports live outside every Git tree.
from pathlib import Path as _EvidencePath
import sys as _evidence_sys
_evidence_repo = next(p for p in _EvidencePath(__file__).resolve().parents
                      if (p / "Docs/Scripts/Offline/evidence_paths.py").is_file())
_evidence_sys.path.insert(0, str(_evidence_repo / "Docs/Scripts/Offline"))
from evidence_paths import evidence_path, evidence_relative, write_replay_adapter

from pathlib import Path
from datetime import datetime
import re
import sys

root = Path(__file__).resolve().parents[3]
output = Path(__file__).parent / 'output' / ('native_bodies_' + datetime.now().strftime('%Y%m%d_%H%M%S'))
output.mkdir(parents=True, exist_ok=True)
wanted = {arg.lower().removeprefix('fun_').removeprefix('0x').zfill(8) for arg in sys.argv[1:]}
for archive in (evidence_path(root / 'Docs/Scripts/Ghidra/output/FFX/decompile')).glob('*.txt'):
    text = archive.read_text(encoding='utf-8', errors='replace')
    headers = list(re.finditer(r'// Function: (\w+)\n', text))
    for i, match in enumerate(headers):
        if match[1].lower().removeprefix('fun_') not in wanted:
            continue
        end = headers[i + 1].start() if i + 1 < len(headers) else len(text)
        body = text[match.start():end]
        (output / (match[1] + '.txt')).write_text(body, encoding='utf-8')
        print(f'{match[1]}: {len(body.splitlines())} lines')
print(output)
