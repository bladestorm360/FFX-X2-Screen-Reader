"""Retain scoped native evidence and validate the complete exit-label audit."""

# Game-source inputs/exports live outside every Git tree.
from pathlib import Path as _EvidencePath
import sys as _evidence_sys
_evidence_repo = next(p for p in _EvidencePath(__file__).resolve().parents
                      if (p / "Docs/Scripts/Offline/evidence_paths.py").is_file())
_evidence_sys.path.insert(0, str(_evidence_repo / "Docs/Scripts/Offline"))
from evidence_paths import evidence_path, evidence_relative, write_replay_adapter

from datetime import datetime,timezone
from hashlib import sha256
from pathlib import Path
import json
import re
import sys
import capstone as cs

ROOT=Path(__file__).resolve().parents[3]
out=evidence_path(ROOT/'Docs/Scripts/Offline/output')/('exit_status_'+datetime.now(timezone.utc).strftime('%Y%m%d_%H%M%S_%f'))
out.mkdir(exist_ok=False)
names={'FUN_0083d7a0','FUN_00870e90','FUN_0086bd20','FUN_00785300',
       'FUN_0082d200','FUN_00a48c20','FUN_00a48f20','FUN_00a59800','FUN_00a5a270',
       'FUN_00a58ec0','FUN_00a4fa30','FUN_00a54b40','FUN_00a4f250','FUN_00a4f9a0','FUN_008f0590','FUN_00a459e0'}
pieces=[]; archives={}
for path in (evidence_path(ROOT/'Docs/Scripts/Ghidra/output/FFX/decompile')).glob('layer[234]*.txt'):
    text=path.read_text(encoding='utf8')
    for match in re.finditer(r'// Function: (\w+)\n.*?(?=// Function: |\Z)',text,re.S):
        if match.group(1) in names:
            names.remove(match.group(1)); pieces.append(match.group(0))
            archives[str(evidence_relative(path, ROOT))]=sha256(path.read_bytes()).hexdigest()
assert not names,names
(out/'native_functions.txt').write_text('\n'.join(pieces),encoding='utf8')
block=evidence_path(ROOT/'Docs/Scripts/Offline/output/reader_reference_20261003_235502/block_00401000.bin')
data=block.read_bytes(); dis=cs.Cs(cs.CS_ARCH_X86,cs.CS_MODE_32)
(out/'grid_status_switch.asm').write_text('\n'.join(f'{ins.address:x}: {ins.mnemonic} {ins.op_str}'
    for ins in dis.disasm(data[0xa45010-0x401000:0xa452aa-0x401000],0xa45010))+'\n',encoding='utf8')
audit=Path(sys.argv[1]); report=json.loads(audit.read_text(encoding='utf8')); exits=report['exits']
assert len(exits)==657 and report['variants']==397
assert all(not e['incomplete'] for e in exits)
assert len({(e['script'],e['checksum'],e['worker']) for e in exits})==657
assert all(name and not name.startswith('<') and name!='None' for e in exits for name in e['names'])
assert sum(bool(e['areas']) for e in exits)==650
by_script={}
for e in exits: by_script.setdefault((e['script'],e['checksum']),[]).append(e)
for group in by_script.values():
    assert sorted(e['number'] for e in group)==list(range(1,len(group)+1))
    assert all(e['total']==len(group) for e in group)
sources={}
for pattern in ('*.cpp','*.hpp','*.inc'):
    for path in (ROOT/'src/native').glob(pattern):
        count=len(path.read_text(encoding='utf8').splitlines())
        assert count<=500,(path.name,count)
        sources[path.name]=count
manifest=dict(utc=datetime.now(timezone.utc).isoformat(),summary=report['summary'],
    audit=str(audit),audit_sha256=sha256(audit.read_bytes()).hexdigest(),archives=archives,
    code_block_sha256=sha256(data).hexdigest(),max_production_lines=max(sources.values()),
    validation='Complete retained player-entry graphs, native name bounds, unique stable identities and <=500-line production files pass.',
    gameplay='pending; no game launch/control/probe')
(out/'manifest.json').write_text(json.dumps(manifest,indent=2)+'\n',encoding='utf8')
print('PASS: 397 variants, 657 exits, 650 native destinations, seven numbered fallbacks, stable identities and production file bounds.')
print('Evidence:',out)
