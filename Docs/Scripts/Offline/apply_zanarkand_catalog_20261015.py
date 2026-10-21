"""Apply only verified Zanarkand names to the existing maintained catalog.

Keep every other script's records byte-for-byte identical; adjust generated
indices/chunk boundaries after adding the two original monitor controls.
The full catalog generator reads the same maintained TRIAL_LABELS metadata.
"""

# Game-source inputs/exports live outside every Git tree.
from pathlib import Path as _EvidencePath
import sys as _evidence_sys
_evidence_repo = next(p for p in _EvidencePath(__file__).resolve().parents
                      if (p / "Docs/Scripts/Offline/evidence_paths.py").is_file())
_evidence_sys.path.insert(0, str(_evidence_repo / "Docs/Scripts/Offline"))
from evidence_paths import evidence_path, evidence_relative, write_replay_adapter

import json
from pathlib import Path
import re
import sys
ROOT=Path(__file__).resolve().parents[3]
sys.path.insert(0,str(ROOT/'src/navigation'))
from finish_targets import TRIAL_LABELS
native=ROOT/'src/native'
header=native/'navigation_targets_data.hpp'
body=header.read_text()
parts=re.findall(r'#include "(navigation_targets_\d+\.inc)"',body)
rows=[line for part in parts for line in (native/part).read_text().splitlines() if line.strip()]
pattern=re.compile(r'(    \{"([^"\n]+)", \d+, 0x[0-9a-f]+, )(\d+)(, )(\d+)(\},)')
scripts=list(pattern.finditer(body));selected=next(s for s in scripts if s[2]=='dome0700')
start,count=int(selected[3]),int(selected[5]);before=rows[start:start+count]
target={int(re.match(r'\s*\{(\d+),',line)[1]):line for line in before}
for worker,label in TRIAL_LABELS['dome0700'].items():
    if worker in target:
        target[worker]=re.sub(r'L"(?:[^"\\]|\\.)*"',lambda _: 'L'+json.dumps(label),target[worker],count=1)
    elif worker in [53,54]:
        target[worker]='    {%d, 6, -1, -1, L%s, EntryMask{{0x4,0x0,0x0,0x0,0x0,0x0,0x0,0x0,0x0,0x0}}, 2},'%(worker,json.dumps(label))
    else:raise ValueError('Unexpected absent trial worker')
after=[target[w] for w in sorted(target)];delta=len(after)-count
assert 0<=delta<=2 and all(w in target for w in [53,54]+list(range(56,85)))
changed=rows[:start]+after+rows[start+count:]
assert changed[:start]==rows[:start] and changed[start+len(after):]==rows[start+count:]
for i,part in enumerate(parts):
    updated='\n'.join(changed[i*350:(i+1)*350])+'\n'
    if (native/part).read_text()!=updated:(native/part).write_text(updated)
def descriptor(match):
    offset,n=int(match[3]),int(match[5])
    if match[2]=='dome0700':n=len(after)
    elif offset>=start+count:offset+=delta
    return match[1]+str(offset)+match[4]+str(n)+match[6]
updated=pattern.sub(descriptor,body)
if updated!=body:header.write_text(updated)
out=evidence_path(ROOT/'Docs/Scripts/Offline/output/help_arrows_zanarkand_20261015/catalog-change.json')
out.write_text(json.dumps(dict(script='dome0700',previous_count=count,current_count=len(after),
    added_monitors=delta,all_other_script_records_byte_identical=True,rows=after),indent=2)+'\n')
print('PASS: Zanarkand glyph/display names applied; every other script record unchanged; indices/chunk boundaries adjusted.')
