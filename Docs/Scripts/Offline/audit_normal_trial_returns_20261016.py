"""Replay the original trial completion tails, including normal return exits.

No game access. Native room loading/collision/party/UI effects remain modeled;
the replay records the actual original warp calls, not fayth-room eligibility.
"""

# Game-source inputs/exports live outside every Git tree.
from pathlib import Path as _EvidencePath
import sys as _evidence_sys
_evidence_repo = next(p for p in _EvidencePath(__file__).resolve().parents
                      if (p / "Docs/Scripts/Offline/evidence_paths.py").is_file())
_evidence_sys.path.insert(0, str(_evidence_repo / "Docs/Scripts/Offline"))
from evidence_paths import evidence_path, evidence_relative, write_replay_adapter

import hashlib
import json
import sys
from datetime import datetime, timezone
from pathlib import Path

ROOT=Path(__file__).resolve().parents[3]
sys.path.insert(0,str(ROOT/'src/puzzles'))
from audit_trial_entries_20261016 import trace
from build_utilities import EVIDENCE
from compile_trials import enrich
from field_text import TextEvidence


def main():
    catalog=json.loads((EVIDENCE/'maps_v3.json').read_text(encoding='utf8'))
    text=TextEvidence(EVIDENCE/'text');records=[]
    cases=[('bsyt0100',30,5,23374,150,[103,0],True),
           ('bsyt0100',30,6,23569,3000,[103,0],False),
           ('klyt1000',37,4,61041,3000,[45,0],False),
           ('djyt0600',66,4,86754,3000,[90,1],False),
           ('mcyt0500',44,4,77628,1550,[106,1],False),
           ('mcyt0500',45,4,77722,3000,[80,0],False),
           ('bvyt1100',56,4,118414,3000,[226,0],False)]
    for script,worker,entry,pc,story,destination,story_write in cases:
        event=enrich(next(e for e in catalog['events'] if e['script']==script),EVIDENCE,text)
        original=event['workers'][worker]['entries'][entry]
        # Mirror the production's documented empty-stack continuation, retaining
        # the original entry identity separately. Archived bytes are unchanged.
        event['workers'][worker]['entries'][entry]=pc
        result=trace(event,worker,entry,story,0)
        assert [s['args'] for s in result['calls'] if s['id'] in [0x11,0x10b]]==[destination]
        mutations=[s for s in result['writes'] if s[1]=='L0:map:000a00:T2']
        assert bool(mutations)==story_write
        if story_write:assert mutations[0][3]==164
        records.append(dict(script=script,source_sha256=event['sha256'],worker=worker,
            entry=entry,original_entry=original,continuation=pc,story=story,result=result))
    out=evidence_path(ROOT/'Docs/Scripts/Offline/output/fayth_gates_20261016')
    out.mkdir(exist_ok=True)
    path=out/('normal_returns_'+datetime.now(timezone.utc).strftime('%Y%m%d_%H%M%S')+'.json')
    path.write_text(json.dumps(dict(checked_at_utc=datetime.now(timezone.utc).isoformat(),records=records,
        map_catalog_sha256=hashlib.sha256((EVIDENCE/'maps_v3.json').read_bytes()).hexdigest(),
        conclusion='Preserve normal native exits and downstream entry controls. No separate fayth goal, universal post-Bevelle threshold, access-flag write or forced chamber warp.',
        limitations=__doc__),indent=2)+'\n',encoding='utf8')
    print('PASS: seven original first/return completion tails preserve antechamber/hall destinations and later story progress. Native downstream room gates remain in charge; no special fayth branch.')
    print(evidence_relative(path, ROOT))


if __name__=='__main__':main()
