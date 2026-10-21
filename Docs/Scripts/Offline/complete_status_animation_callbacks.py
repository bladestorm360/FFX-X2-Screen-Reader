"""Complete dynamically assigned callbacks in retained DLL analysis; offline only."""

# Game-source inputs/exports live outside every Git tree.
from pathlib import Path as _EvidencePath
import sys as _evidence_sys
_evidence_repo = next(p for p in _EvidencePath(__file__).resolve().parents
                      if (p / "Docs/Scripts/Offline/evidence_paths.py").is_file())
_evidence_sys.path.insert(0, str(_evidence_repo / "Docs/Scripts/Offline"))
from evidence_paths import evidence_path, evidence_relative, write_replay_adapter

from pathlib import Path
from datetime import datetime, timezone
import re
import json

root=Path(__file__).resolve().parents[3]
previous=evidence_path(root/'Docs/Scripts/Ghidra/output/FFX/status_animations_20261009_171247_874998')
out=previous.parent/('status_animation_callbacks_'+datetime.now(timezone.utc).strftime('%Y%m%d_%H%M%S_%f'))
out.mkdir()
modules=[]
lines=["$ErrorActionPreference = 'Stop'", "$env:JAVA_HOME = 'C:/Program Files/Java/jdk-21'"]
for source in sorted(previous.glob('magic_*.dll.txt')):
    text=source.read_text(encoding='utf-8')
    entries=sorted(set(re.findall(r'=\s*&LAB_(100[0-9a-f]{5})',text)))
    if not entries: continue
    name=source.name[:-4]
    modules.append(dict(name=name,entries=entries))
    lines.append("& 'D:/Games/Dev/ghidra_12.0.3_PUBLIC/support/analyzeHeadless.bat' "
                 f"'{previous/'project'}' StatusAnimations -process {name} -noanalysis "
                 f"-scriptPath '{root/'Docs/Scripts/Ghidra/FFX'}' -postScript status_animation_module.java '{out}' "+' '.join(entries))
    lines.append("if ($LASTEXITCODE -ne 0) { throw 'Offline callback analysis failed: "+name+"' }")
(out/'analyze.ps1').write_text('\n'.join(lines)+'\n')
(out/'manifest.json').write_text(json.dumps(dict(previous=str(previous),modules=modules),indent=2)+'\n')
print('Retained callback follow-up:',out)
