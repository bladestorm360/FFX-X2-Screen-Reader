"""Retain scoped bodies from the existing decompile; no game/probe execution."""

# Game-source inputs/exports live outside every Git tree.
from pathlib import Path as _EvidencePath
import sys as _evidence_sys
_evidence_repo = next(p for p in _EvidencePath(__file__).resolve().parents
                      if (p / "Docs/Scripts/Offline/evidence_paths.py").is_file())
_evidence_sys.path.insert(0, str(_evidence_repo / "Docs/Scripts/Offline"))
from evidence_paths import evidence_path, evidence_relative, write_replay_adapter

from pathlib import Path
import json
import re
from datetime import datetime

root = Path(__file__).resolve().parents[3]
source = evidence_path(root / 'Docs/Scripts/Ghidra/output/FFX/decompile/layer2_game_logic.txt')
output = evidence_path(root / 'Docs/Scripts/Offline/output') / ('player_reports_' + datetime.now().strftime('%Y%m%d_%H%M%S'))
output.mkdir()
selected = {'FUN_00864180', 'FUN_008725f0', 'FUN_0086c500', 'FUN_0078d7f0',
            'FUN_00782730', 'FUN_00892c20', 'FUN_00892880', 'FUN_00795560', 'FUN_007955a0',
            'FUN_00866920', 'FUN_00866c60', 'FUN_008665b0', 'FUN_0086bd20', 'FUN_0086c240',
            'FUN_0083e5f0', 'FUN_0078f0b0', 'FUN_0078c540', 'FUN_00867fc0', 'FUN_00867bb0',
            'FUN_00876360', 'FUN_008767e0', 'FUN_0085fa30', 'FUN_0085ff00'}
patterns = ('DAT_0112a8e0 = 5;', 'FUN_0078d7f0(param_', 'FUN_008aa0b0(0,', 'FUN_00782730();',
            'FUN_00866920(', 'FUN_00866c60(', 'FUN_0086bd20(', 'FUN_00876470(')
records = []
for body in re.split(r'// Function: ', source.read_text(encoding='utf-8'))[1:]:
    name = body.splitlines()[0].strip()
    if name not in selected and not any(pattern in body for pattern in patterns):
        continue
    (output / (name + '.txt')).write_text('// Function: ' + body, encoding='utf-8')
    records.append({'function': name, 'lines': len(body.splitlines()),
                    'reasons': [p for p in patterns if p in body]})
(output / 'index.json').write_text(json.dumps(records, indent=2) + '\n', encoding='utf-8')
print(evidence_relative(output, root))
print(json.dumps(records))
