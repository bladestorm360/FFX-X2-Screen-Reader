"""Retain original selector/cursor code for the isolated x86 reader host."""

# Game-source inputs/exports live outside every Git tree.
from pathlib import Path as _EvidencePath
import sys as _evidence_sys
_evidence_repo = next(p for p in _EvidencePath(__file__).resolve().parents
                      if (p / "Docs/Scripts/Offline/evidence_paths.py").is_file())
_evidence_sys.path.insert(0, str(_evidence_repo / "Docs/Scripts/Offline"))
from evidence_paths import evidence_path, evidence_relative, write_replay_adapter

import hashlib
import json
import struct
from pathlib import Path
import capstone as cs
from capstone.x86 import X86_OP_IMM, X86_OP_MEM

ROOT=Path(__file__).resolve().parents[3]
archive=evidence_path(ROOT/'Docs/Scripts/Offline/output/reader_reference_20261003_235502/block_00401000.bin')
data=archive.read_bytes()
dis=cs.Cs(cs.CS_ARCH_X86,cs.CS_MODE_32);dis.detail=True
parts=[];evidence=[]
for name,start,size in [('gridDot',0x62fe90,37),('gridSelector',0xa56860,474),('gridCursor',0xa48d20,65),('gridCursorInner',0xa48d70,198),('helpTransition',0x776a50,1422)]:
    body=data[start-0x401000:start-0x401000+size]
    fixes=[]
    for ins in dis.disasm(body,start):
        for operand in ins.operands:
            relative=False
            if operand.type==X86_OP_IMM:
                value=operand.imm&0xffffffff;offset=ins.imm_offset
                if ins.group(cs.CS_GRP_JUMP) or ins.group(cs.CS_GRP_CALL):
                    if start<=value<start+size:continue
                    relative=True
            elif operand.type==X86_OP_MEM:
                value=operand.mem.disp&0xffffffff;offset=ins.disp_offset
            else:continue
            if 0x400000<=value<0x2800000:fixes.append((ins.address-start+offset,value-0x400000,relative))
    parts.append('const unsigned char '+name+'Body[]={\n'+'\n'.join('    '+','.join(f'0x{x:02x}' for x in body[i:i+24])+',' for i in range(0,size,24))+'\n};\n')
    if fixes:parts.append('const ReplayFixup '+name+'Fixups[]={\n'+'\n'.join(f'    {{{o},0x{v:x},{str(r).lower()}}},' for o,v,r in fixes)+'\n};\n')
    evidence.append(dict(name=name,va=start,size=size,sha256=hashlib.sha256(body).hexdigest(),fixups=fixes))
target=ROOT/'src/native/tests/grid_replay_bytes.inc'
write_replay_adapter(target, '// Original code for isolated offline fixtures only. Never linked into the mod.\nstruct ReplayFixup {unsigned offset,rva;bool relative;};\n'+''.join(parts))
(evidence_path(ROOT/'Docs/Scripts/Offline/output/report_20261015/grid-replay-provenance.json')).write_text(json.dumps(dict(source=str(evidence_relative(archive, ROOT)),source_sha256=hashlib.sha256(data).hexdigest(),parts=evidence),indent=2))
print('Retained original grid selector/cursor and Help transition:',[(p['name'],p['size']) for p in evidence])
