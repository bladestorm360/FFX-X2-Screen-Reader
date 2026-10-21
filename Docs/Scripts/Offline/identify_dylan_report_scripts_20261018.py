"""Retained original scripts for the explicitly requested Dylan DM follow-up.

No installed game reads or game execution. Original parsing attribution remains
Karifean/FFXDataParser; production names/text still use native getters.
"""

# Game-source inputs/exports live outside every Git tree.
from pathlib import Path as _EvidencePath
import sys as _evidence_sys
_evidence_repo = next(p for p in _EvidencePath(__file__).resolve().parents
                      if (p / "Docs/Scripts/Offline/evidence_paths.py").is_file())
_evidence_sys.path.insert(0, str(_evidence_repo / "Docs/Scripts/Offline"))
from evidence_paths import evidence_path, evidence_relative, write_replay_adapter

from pathlib import Path
from datetime import datetime, timezone
import json, sys, hashlib

root = Path(__file__).resolve().parents[3]
sys.path.insert(0, str(root / 'src/puzzles'))
from compile_trials import enrich
from atel_model import compile_script, render
from field_text import TextEvidence

base = evidence_path(root / 'Docs/Scripts/Offline/output/FFX/20261003_audit')
catalog = json.loads((base / 'maps_v3.json').read_text(encoding='utf-8'))
text = TextEvidence(base / 'text')
out = evidence_path(root / ('Docs/Scripts/Offline/output/dylan_reports_20261018_' +
                            datetime.now(timezone.utc).strftime('%H%M%S')))
out.mkdir()
selected = {'hiku2100', 'mtgz0800', 'mtgz1000', 'dome0600', 'mtgz0700'}
manifest = []
for event in catalog['events']:
    if event['script'] not in selected or '/psv_' in event['source']:
        continue
    script = enrich(event, base, text)
    model = compile_script(script)
    name = event['script']
    (out / (name + '.json')).write_text(json.dumps(model, indent=2), encoding='utf-8')
    (out / (name + '.txt')).write_text(render(model), encoding='utf-8')
    manifest.append({'script': name, 'source': event['source'],
                     'sha256': event['sha256'], 'areas': script['area_names'],
                     'strings': script['english_strings']})
    print(name, script['area_names'], len(model['workers']), 'workers')
(out / 'manifest.json').write_text(json.dumps({'source_catalog_sha256':
    hashlib.sha256((base/'maps_v3.json').read_bytes()).hexdigest(),
    'scripts': manifest}, indent=2), encoding='utf-8')
print(evidence_relative(out, root))
