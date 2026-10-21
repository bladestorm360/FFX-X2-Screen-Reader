"""Retained text, native glyph rows and input consumers; no game/log access."""

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
import re
import struct
from field_text import TextEvidence

ROOT = Path(__file__).resolve().parents[3]
AUDIT = evidence_path(ROOT / 'Docs/Scripts/Offline/output/FFX/20261003_audit')
functions = {}
addresses = ['008f8ee0', '006e6860', '006e7180', '006e6590', '006e67a0', '00631770', '0082d200']
for path in (evidence_path(ROOT / 'Docs/Scripts/Ghidra/output/FFX/decompile')).glob('*.txt'):
    text = path.read_text(errors='replace')
    for address in addresses:
        match = re.search(r'(?im)^// Function: FUN_' + address + r'\s*$', text)
        if match:
            end = text.find('// ========================================', match.end() + 180)
            functions[address] = dict(source=str(evidence_relative(path, ROOT)), body=text[match.start():end])
assert len(functions) == len(addresses)
body = functions['006e6860']['body']
values = {0: int(re.search(r'\*in_ECX = (0x[0-9a-f]+|\d+);', body)[1], 0)}
for index, value in re.findall(r'in_ECX\[(0x[0-9a-f]+|\d+)\] = (0x[0-9a-f]+|\d+);', body):
    values[int(index, 0)] = int(value, 0)
tokens = {values[i * 2]: values[i * 2 + 1] for i in range(22)}
expected = {0x30:6, 0x31:4, 0x32:5, 0x33:7, 0x34:8, 0x35:10, 0x36:9, 0x37:11,
            0x38:12, 0x39:13, 0x41:0, 0x42:3, 0x44:1, 0x48:2}
assert all(tokens[token] == action for token, action in expected.items())
assert 'iVar7 = param_1 + 0xe' in functions['006e6590']['body']
assert '+ param_1 * 4' in functions['006e67a0']['body']
assert 'uVar7 & 0x40)' in functions['0082d200']['body']
text_evidence = TextEvidence(AUDIT / 'text')
prompts = []
for area, index, token in [('cdsp0000',107,0x33), ('cdsp0700',0,0x31), ('cdsp0700',1,0x33)]:
    path = AUDIT / f'files/ffx_ps2/ffx/master/jppc/event/obj/cd/{area}/{area}.ebp'
    blob = path.read_bytes()
    begin, end = struct.unpack_from('<II', blob, 20)
    chunk = blob[begin:end]
    offset = struct.unpack_from('<I', chunk, index * 8)[0] & 0xffff
    raw = chunk[offset:chunk.index(0, offset) + 1]
    assert bytes([11, token]) in raw
    if area == 'cdsp0000': assert bytes([11, 0x32]) in raw
    prompts.append(dict(area=area,index=index,source=str(evidence_relative(path, ROOT)),
                        sha256=hashlib.sha256(blob).hexdigest(),hex=raw.hex(' '),
                        text=text_evidence.field_strings(path)[index]))
model = json.loads((evidence_path(ROOT / 'Docs/Scripts/Offline/output/swimming_progression_20261009_051440_535541/cdsp0000.json')).read_text())
calls = [s for b in model['workers'][16]['blocks'] for s in b['statements'] if s.get('id') == 77]
assert any(s['pc'] == 42161 and s['args'] == ['7'] for s in calls)
bindings = json.loads((evidence_path(ROOT / 'Docs/Scripts/Offline/output/auron_bindings_20261009_120127_655143.json')).read_text())
assert bindings['rows'][4]['mask'] == 0x20 and bindings['rows'][5]['mask'] == 0x40 and bindings['rows'][7]['mask'] == 0x80
record = dict(utc=datetime.now(timezone.utc).isoformat(),functions=functions,prompts=prompts,
              glyph_actions=expected, return_worker16_call=calls, dive_mask=0x40,
              runtime_tested=False,tester_logs_accessed=False)
output = evidence_path(ROOT / 'Docs/Scripts/Offline/output') / ('swimming_controls_' + datetime.now(timezone.utc).strftime('%Y%m%d_%H%M%S_%f') + '.json')
output.write_text(json.dumps(record,indent=2) + '\n',encoding='utf-8')
print('PASS: original swimming/Examine strings, native glyph/action table, Return handler and Dive input consumer.')
print(evidence_relative(output, ROOT))
