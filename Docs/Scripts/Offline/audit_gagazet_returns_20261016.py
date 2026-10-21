"""Audit retained native Gagazet returns; no game access or execution."""

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

root = Path(__file__).resolve().parents[3]

base = evidence_path(root / 'Docs/Scripts/Offline/output/gagazet_cave_20261016')
original = evidence_path(root / 'Docs/Scripts/Offline/output/FFX/20261003_audit/files/ffx_ps2/ffx/master/jppc/event/obj/mt')
flows = {}
sources = []
for name in ('mtgz0700', 'mtgz0600'):
    flow = json.loads((base / (name + '.json')).read_text())
    path = original / name / (name + '.ebp')
    data = path.read_bytes()
    assert hashlib.sha256(data).hexdigest() == flow['sha256']
    flows[name] = flow
    sources.append({'path': str(path), 'sha256': flow['sha256']})

def statement(name, worker, pc):
    return next(s for b in flows[name]['workers'][worker]['blocks']
                for s in b['statements'] if s['pc'] == pc)

proof = []
for worker, pc, entry in ((29, 0x388c, 1), (30, 0x38da, 2), (31, 0x3928, 3)):
    s = statement('mtgz0700', worker, pc)
    assert s['kind'] == 'call' and s['id'] == 0x11 and s['args'] == ['272', str(entry)]
    proof.append({'worker': worker, 'pc': pc, 'native_return': [272, entry]})
for worker, entry in ((32, 6), (33, 7)):
    calls = [s for b in flows['mtgz0700']['workers'][worker]['blocks'] for s in b['statements']
             if s['kind'] == 'request' and s.get('worker') == '22' and s.get('entry') == str(entry)]
    assert len(calls) == 1
    if worker == 32:
        assert calls[0]['pc'] == 0x3976
    assert flows['mtgz0700']['workers'][worker]['entries'][2] <= calls[0]['pc']
    proof.append({'worker': worker, 'trial_request': calls[0], 'physical_callback': 2})
for pc, bit in ((0x25ee, 1), (0x2e44, 2)):
    s = statement('mtgz0700', 22, pc)
    assert s['kind'] == 'write' and s['binding'] == 'L0:map:0000d8:T0'
    assert s['element'] == '0' and s['value'] == '(L0:map:0000d8:T0[0] | %d)' % bit
    proof.append({'water_success_write': s})
for pc, bit in ((0x3a85, 1), (0x3aa7, 2)):
    s = statement('mtgz0600', 15, pc)
    assert s['condition'] == '(!(!(!(L0:map:0000d8:T0[0] & %d))))' % bit
    proof.append({'cave_wall_guard': s})
for pc, wall in ((0x3a8e, 52), (0x3ab0, 49), (0x3ac2, 51), (0x3ad4, 50)):
    s = statement('mtgz0600', 15, pc)
    assert s['id'] == 0x4017 and s['args'] == [str(wall), '1']
    proof.append({'cave_initialization_wall_call': s})

# Explicit early-save test context: 2625 selects the regular cave layout and
# retains both unfinished trials. It is a verified test interval, not a claim
# about the exact story counter of a naturally reached save.
for pc, condition in ((14951, '(L0:map:000a00:T2[0] >= 2680)'),
                      (15080, '(L0:map:000a00:T2[0] < 2715)'),
                      (15181, '(L0:map:000a00:T2[0] >= 2625)')):
    s = statement('mtgz0600', 15, pc)
    assert s['condition'] == condition
    proof.append({'test_story_gate': s})
s = statement('mtgz0600', 33, 0x580c)
assert s['kind'] == 'request' and s['worker'] == '15' and s['entry'] == '7'
assert flows['mtgz0600']['workers'][33]['entries'][2] == 0x57f4
proof.append({'native_swimming_party_event': s})
for pc, common, args in ((16364, 0xca, ['4']), (16370, 0xca, ['6']),
                         (16403, 0xe7, ['0', '0']), (16412, 0xe7, ['1', '4']),
                         (16421, 0xe7, ['2', '6'])):
    s = statement('mtgz0600', 15, pc)
    assert s['kind'] == 'call' and s['id'] == common and s['args'] == args
    proof.append({'native_swimming_party_call': s})
for worker, pc, model in ((0, 10, 1), (3, 3524, 5), (5, 5589, 7)):
    s = statement('mtgz0600', worker, pc)
    assert s['kind'] == 'call' and s['id'] == 1 and s['args'] == [str(model)]
    assert s['pc'] < flows['mtgz0600']['workers'][worker]['entries'][1]
    proof.append({'swimming_actor_native_initialization': s})

out = evidence_path(root / 'Docs/Scripts/Offline/output/gagazet_fixture_20261016')
stamp = datetime.now(timezone.utc).strftime('%Y%m%d_%H%M%S')
path = out / ('return_route_' + stamp + '.json')
path.write_text(json.dumps({'sources': sources, 'proof': proof,
    'interpretation': 'Success sets shared D8 route bits. Return to Mountain Cave; its initialization gates walls52 and49/51/50 on those bits. W32/33 are trial triggers, not direct pool-to-pool exits.',
    'limits': 'Static original bytecode/flow evidence. Physical movement, collision geometry and game presentation are not executed.'}, indent=2) + '\n')
print('PASS: original three water-to-cave returns, two trial triggers, shared success bits and four cave wall calls; test story interval and native swimming-party event/calls. ' + str(path))
