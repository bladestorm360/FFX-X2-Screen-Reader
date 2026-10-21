"""Confirm the missing native depth transition using retained evidence only."""

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
import re
import struct
from datetime import datetime, timezone

ROOT = Path(__file__).resolve().parents[3]
ARCHIVE = evidence_path(ROOT / 'Docs/Scripts/Offline/output/swimming_progression_20261009_051440_535541')
source = evidence_path(ROOT / 'Docs/Scripts/Offline/output/FFX/20261003_audit/files/ffx_ps2/ffx/master/jppc/event/obj/cd/cdsp0000/cdsp0000.ebp')
blob = source.read_bytes()
start, end = struct.unpack_from('<II', blob, 4)
chunk = blob[start:end]
length = struct.unpack_from('<I', chunk)[0]
code_offset = struct.unpack_from('<I', chunk, 0x30)[0]
code = chunk[code_offset:code_offset + length]
checksum = 2166136261
for byte in code:
    checksum = ((checksum ^ byte) * 16777619) & 0xffffffff
assert length == 63240 and checksum == 0xf5f6d993
model = json.loads((ARCHIVE / 'cdsp0000.json').read_text())
worker = model['workers'][30]
assert worker['type'] == 4 and worker['entries'][6] == 60444
statements = [s for b in worker['blocks'] for s in b['statements']]
branch = next(s for s in statements if s['pc'] == 60466)
transition = next(s for s in statements if s['pc'] == 60498)
assert branch['condition'] == '(L3:w30:0130b8:T6[0] > 250.0)'
assert transition['id'] == 17 and transition['args'] == ['288', '0']
functions = {}
for path in (evidence_path(ROOT / 'Docs/Scripts/Ghidra/output/FFX/decompile')).glob('*.txt'):
    text = path.read_text(errors='replace')
    for name in ('FUN_0086a280', 'FUN_0082d200', 'FUN_0082a910', 'FUN_00832e90'):
        match = re.search(r'\n[^\n]*\b' + name + r'\([^\n]*\)\s*\n\s*\{', text)
        if match:
            begin = match.start()
            finish = text.find('\n//', begin + 1)
            functions[name] = text[begin:finish]
assert set(functions) == {'FUN_0086a280', 'FUN_0082d200', 'FUN_0082a910', 'FUN_00832e90'}
assert 'param_1 + 0x504' in functions['FUN_0082a910']
assert 'uVar7 & 0x40' in functions['FUN_0082d200'] and 'DAT_0130077d' in functions['FUN_0082d200']
assert '0x504) * local_10' in functions['FUN_00832e90']
record = dict(utc=datetime.now(timezone.utc).isoformat(), source=str(evidence_relative(source, ROOT)),
    sha256=hashlib.sha256(blob).hexdigest(), script_length=length, script_fnv=f'{checksum:08x}',
    worker=30, worker_type=4, entry=6, producer_pc=[60444,60510],
    condition=branch, transition=transition, axis='Y (depth increases downward)',
    dive_input=0x40, dive_enabled_rva=0xf0077d, native_functions=functions,
    runtime_tested=False, tester_logs_accessed=False)
output = evidence_path(ROOT / 'Docs/Scripts/Offline/output') / ('swimming_goal_' + datetime.now(timezone.utc).strftime('%Y%m%d_%H%M%S_%f') + '.json')
output.write_text(json.dumps(record, indent=2) + '\n')
print('PASS: archived hash-qualified depth producer, strict transition and native Dive/vertical movement evidence.')
print(evidence_relative(output, ROOT))
