"""Locate call contexts in existing decompile text without running a probe."""

# Game-source inputs/exports live outside every Git tree.
from pathlib import Path as _EvidencePath
import sys as _evidence_sys
_evidence_repo = next(p for p in _EvidencePath(__file__).resolve().parents
                      if (p / "Docs/Scripts/Offline/evidence_paths.py").is_file())
_evidence_sys.path.insert(0, str(_evidence_repo / "Docs/Scripts/Offline"))
from evidence_paths import evidence_path, evidence_relative, write_replay_adapter

from pathlib import Path
import sys
root = Path(__file__).resolve().parents[3]
wanted = ['FUN_' + value.lower().removeprefix('fun_').removeprefix('0x').zfill(8) for value in sys.argv[1:]]
for archive in (evidence_path(root / 'Docs/Scripts/Ghidra/output/FFX/decompile')).glob('*.txt'):
    function = ''
    for number, line in enumerate(archive.read_text(encoding='utf8', errors='replace').splitlines(), 1):
        if line.startswith('// Function: '):
            function = line[13:]
        elif any(name in line for name in wanted) and not line.strip().startswith(('void FUN_', 'int FUN_', 'undefined')):
            print(f'{archive.name}:{number} {function}: {line.strip()}')
