"""Current no-calibration guidance audit, based entirely on retained originals."""

# Game-source inputs/exports live outside every Git tree.
from pathlib import Path as _EvidencePath
import sys as _evidence_sys
_evidence_repo = next(p for p in _EvidencePath(__file__).resolve().parents
                      if (p / "Docs/Scripts/Offline/evidence_paths.py").is_file())
_evidence_sys.path.insert(0, str(_evidence_repo / "Docs/Scripts/Offline"))
from evidence_paths import evidence_path, evidence_relative, write_replay_adapter

from pathlib import Path
import hashlib
import json

ROOT = Path(__file__).resolve().parents[3]
flow = evidence_path(ROOT/'Docs/Scripts/Offline/output/dylan_reports_20261018_011846/mtgz0700.json')
model = json.loads(flow.read_text())
actor = [s for b in model['workers'][1]['blocks'] for s in b['statements'] if 1689 <= s['pc'] < 1772]
motions = [(s['pc'],s['args']) for s in actor if s.get('id') == 0x501a]
assert [args for _,args in motions] == [['332005','0','1','0','34'],['332005','0','1','34','-1'],
    ['332006','0','1','0','-1'],['332007','0','0','0','-1']]
prior = evidence_path(ROOT/'Docs/Scripts/Offline/output/gagazet_timing_20261018/proof.json')
proof = json.loads(prior.read_text())
assert proof['motions']['332005']['frames'] == 36 and proof['motions']['332006']['frames'] == 19
production = (ROOT/'src/native/gagazet.cpp').read_text()
assert all(t not in production for t in ('timing.finish','timing.begin','rotationWait','calibrat','countdown'))
assert 'captureUtilityValue(s,FlagLow,Flags,2,flags)' in production
assert 'flags)&0xc0u)==0xc0u' in production and 'before the tone' in production
out=evidence_path(ROOT/'Docs/Scripts/Offline/output/gagazet_targeting_20261019');out.mkdir(exist_ok=True)
(out/'proof.json').write_text(json.dumps(dict(
    flow_sha256=hashlib.sha256(flow.read_bytes()).hexdigest(),
    retained_motion_proof_sha256=hashlib.sha256(prior.read_bytes()).hexdigest(),motions=motions,
    animation_frames=55,
    native_clock_limit='839C00 initializes fixed-point endpoints;838D10 increments animation using per-actor speed and optional half-step. W22 waits15/10 overlap asynchronous W1 requests and await-motion predicates. Animation frame total alone is not an established constant native ring-clock/input-to-hit lead.',
    decision='User-approved fallback: native simultaneous0x40/0x80 targeting beep on each rising edge; listen through passes and start the throw before the beep. No calibration, predicted launch window, manual success write or runtime measurement.',
    verification_limit='Original schedules and actual production hooks are exercised with controlled native flags. Native animation/request scheduling, actual audio and human reaction remain gameplay limits.'),indent=2)+'\n',encoding='utf-8')
print('PASS: original36+19 animation frames and async script waits; production observes native targeting flags with no mandatory calibration/predicted lead.')
