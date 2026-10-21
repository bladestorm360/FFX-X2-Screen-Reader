"""Recover only named original functions from retained workspace bytes."""

# Game-source inputs/exports live outside every Git tree.
from pathlib import Path as _EvidencePath
import sys as _evidence_sys
_evidence_repo = next(p for p in _EvidencePath(__file__).resolve().parents
                      if (p / "Docs/Scripts/Offline/evidence_paths.py").is_file())
_evidence_sys.path.insert(0, str(_evidence_repo / "Docs/Scripts/Offline"))
from evidence_paths import evidence_path, evidence_relative, write_replay_adapter

from pathlib import Path
import hashlib, json
import capstone as cs
from capstone.x86 import X86_OP_IMM, X86_OP_MEM
root=Path(__file__).resolve().parents[3]
source=evidence_path(root/'Docs/Scripts/Offline/output/reader_reference_20261003_235502/block_00401000.bin')
data=source.read_bytes()
decoder=cs.Cs(cs.CS_ARCH_X86,cs.CS_MODE_32);decoder.detail=True
out=evidence_path(root/'Docs/Scripts/Offline/output/customize_slots_20261016');out.mkdir(exist_ok=True)
lines=['// Original functions for isolated offline tests only; never shipped.']
records=[]
for name,start,size in [('customizeLookup',0x7abbf0,135),('customizeList',0x7abd30,176),('customizeVisible',0x8d5720,76)]:
    body=data[start-0x401000:start-0x401000+size];fixups=[];assembly=[]
    for ins in decoder.disasm(body,start):
        assembly.append('%08x %s %s'%(ins.address,ins.mnemonic,ins.op_str))
        for operand in ins.operands:
            if operand.type==X86_OP_IMM:
                value=operand.imm&0xffffffff
                if ins.group(cs.CS_GRP_JUMP) or ins.group(cs.CS_GRP_CALL):
                    if not start<=value<start+size: fixups.append((ins.address-start+ins.imm_offset,value-0x400000,True))
                    continue
                offset=ins.imm_offset
            elif operand.type==X86_OP_MEM:
                value=operand.mem.disp&0xffffffff;offset=ins.disp_offset
            else: continue
            if 0x400000<=value<0x2800000: fixups.append((ins.address-start+offset,value-0x400000,False))
    lines+=['const unsigned char '+name+'Body[]={',*['    '+','.join('0x%02x'%v for v in body[i:i+24])+',' for i in range(0,len(body),24)],'};',
        'const ReplayFixup '+name+'Fixups[]={',*['    {%d,0x%x,%s},'%(a,b,str(c).lower()) for a,b,c in fixups],'};']
    (out/(name+'.txt')).write_text('\n'.join(assembly),encoding='utf-8')
    records.append(dict(name=name,va=start,size=size,sha256=hashlib.sha256(body).hexdigest(),fixups=fixups))
write_replay_adapter(root/'src/native/tests/customize_replay_bytes.inc', '\n'.join(lines)+'\n', encoding='utf-8')
(out/'provenance.json').write_text(json.dumps(dict(source=str(evidence_relative(source, root)),sha256=hashlib.sha256(data).hexdigest(),functions=records),indent=2),encoding='utf-8')
print('Retained original Customize lookup/list/visibility:',out)
