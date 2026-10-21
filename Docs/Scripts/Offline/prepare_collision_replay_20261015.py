"""Retain the actual x86 collision body AND dispatch tables for isolated tests.

The decompile's reconstructed switch labels are wrong. Read only the existing
reference archive; no installation/process access. All external dependencies
are explicit fixture stubs or copied memory.
"""

# Game-source inputs/exports live outside every Git tree.
from pathlib import Path as _EvidencePath
import sys as _evidence_sys
_evidence_repo = next(p for p in _EvidencePath(__file__).resolve().parents
                      if (p / "Docs/Scripts/Offline/evidence_paths.py").is_file())
_evidence_sys.path.insert(0, str(_evidence_repo / "Docs/Scripts/Offline"))
from evidence_paths import evidence_path, evidence_relative, write_replay_adapter

from pathlib import Path
import struct, hashlib, json
import capstone as cs
from capstone.x86 import X86_OP_IMM, X86_OP_MEM
ROOT=Path(__file__).resolve().parents[3]
source=evidence_path(ROOT/'Docs/Scripts/Offline/output/reader_reference_20261003_235502/block_00401000.bin')
archive=source.read_bytes();start=0x83e4e0;end=0x83e5b3
body=archive[start-0x401000:end-0x401000]
dis=cs.Cs(cs.CS_ARCH_X86,cs.CS_MODE_32);dis.detail=True
fix=[]
for i in dis.disasm(body[:123],start):
    for op in i.operands:
        if op.type==X86_OP_MEM and 0x400000<=op.mem.disp<0x2800000:
            fix.append((i.address-start+i.disp_offset,op.mem.disp,False))
        elif op.type==X86_OP_IMM and i.mnemonic=='call':
            fix.append((i.address-start+i.imm_offset,op.imm,True))
for i in range(6):fix.append((0x83e55c-start+i*4,struct.unpack_from('<I',archive,0x83e55c-0x401000+i*4)[0],False))
rows=['// Original83E4E0, including the actual two-stage switch tables.','#pragma once',
      'struct CollisionReplayFixup {unsigned offset,va;bool relative;};',
      'inline constexpr unsigned char collisionReplayBody[]={'+','.join(map(str,body))+'};',
      'inline constexpr CollisionReplayFixup collisionReplayFixups[]={'+','.join('{%d,0x%x,%s}'%(o,v,str(r).lower()) for o,v,r in fix)+'};']
for name,va,size in [('choiceExec',0x859a80,167),('choiceReturn',0x85a590,94)]:
    code=archive[va-0x401000:va-0x401000+size]
    calls=[(i.address-va+i.imm_offset,i.operands[0].imm) for i in dis.disasm(code,va) if i.mnemonic=='call']
    rows+=['inline constexpr unsigned char '+name+'Body[]={'+','.join(map(str,code))+'};',
           'inline constexpr CollisionReplayFixup '+name+'Fixups[]={'+','.join('{%d,0x%x,true}'%(o,v) for o,v in calls)+'};']
write_replay_adapter(ROOT/'src/native/tests/collision_replay_bytes.inc', '\n'.join(rows)+'\n')
jump=struct.unpack_from('<6I',archive,0x83e55c-0x401000)
mapping={g:hex(jump[archive[0x83e574-0x401000+g-1]]) for g in range(1,64)}
out=evidence_path(ROOT/'Docs/Scripts/Offline/output/help_navigation_quit_20261015/collision-native-dispatch.json')
out.write_text(json.dumps(dict(source=str(evidence_relative(source, ROOT)),body_sha256=hashlib.sha256(body).hexdigest(),start=hex(start),end=hex(end),group_targets=mapping),indent=2)+'\n')
print('Retained original123-byte collision function,24-byte jump table and63-byte selector table.')
