"""Original bounded request/dispatch replay and scheduler signature; retained bytes only."""

# Game-source inputs/exports live outside every Git tree.
from pathlib import Path as _EvidencePath
import sys as _evidence_sys
_evidence_repo = next(p for p in _EvidencePath(__file__).resolve().parents
                      if (p / "Docs/Scripts/Offline/evidence_paths.py").is_file())
_evidence_sys.path.insert(0, str(_evidence_repo / "Docs/Scripts/Offline"))
from evidence_paths import evidence_path, evidence_relative, write_replay_adapter

from pathlib import Path
import hashlib,json
import capstone as cs
from capstone.x86 import X86_OP_IMM,X86_OP_MEM
root=Path(__file__).resolve().parents[3]
source=evidence_path(root/'Docs/Scripts/Offline/output/reader_reference_20261003_235502/block_00401000.bin')
data=source.read_bytes();dis=cs.Cs(cs.CS_ARCH_X86,cs.CS_MODE_32);dis.detail=True
out=evidence_path(root/'Docs/Scripts/Offline/output/atel_queue_20261016');out.mkdir(exist_ok=True)
rows=['// Original ATEL queue/dispatch functions, isolated fixture only.','#pragma once',
    'struct AtelReplayFixup {unsigned offset,va;bool relative;};'];records=[]
for name,va,size in [('queueRequest',0x86e920,68),('queueInsert',0x86e990,408),('queueDispatch',0x865390,205),
    ('queueInit',0x869040,299),('queueFinish',0x869170,377),('queueAfter',0x86dba0,27),
    ('queueBefore',0x86dbc0,28),('queueAppend',0x86dbe0,45),('queueAck',0x8715f0,87)]:
    body=data[va-0x401000:va-0x401000+size];fix=[];tokens=list(body);assembly=[]
    instructions=list(dis.disasm(body,va))
    assert sum(i.size for i in instructions)==size,(name,'incomplete final instruction')
    assert instructions[-1].mnemonic in ('ret','int3'),(name,instructions[-1].mnemonic)
    for ins in instructions:
        assembly.append('%08x %s %s'%(ins.address,ins.mnemonic,ins.op_str))
        for operand in ins.operands:
            relative=False
            if operand.type==X86_OP_IMM:
                value=operand.imm&0xffffffff;offset=ins.imm_offset;width=ins.imm_size
                if ins.group(cs.CS_GRP_JUMP) or ins.group(cs.CS_GRP_CALL):
                    if va<=value<va+size:continue
                    relative=True
            elif operand.type==X86_OP_MEM:
                value=operand.mem.disp&0xffffffff;offset=ins.disp_offset;width=ins.disp_size
            else:continue
            if 0x400000<=value<0x2800000:
                assert width==4,(name,ins.address,width)
                fix.append((ins.address-va+offset,value,relative))
                tokens[ins.address-va+offset:ins.address-va+offset+4]=[-1]*4
    rows+=['inline constexpr unsigned char '+name+'Body[]={'+','.join(map(str,body))+'};',
        'inline constexpr std::array<AtelReplayFixup,'+str(len(fix))+'>'+name+'Fixups={{'+','.join('{%d,0x%x,%s}'%(a,b,str(c).lower()) for a,b,c in fix)+'}};']
    (out/(name+'.txt')).write_text('\n'.join(assembly),encoding='utf-8')
    records.append(dict(name=name,va=va,size=size,sha256=hashlib.sha256(body).hexdigest(),fixups=fix))
    if name=='queueDispatch':
        path=root/'src/native/profile_extensions_data.hpp';text=path.read_text(encoding='utf-8')
        declaration='inline constexpr int signature_465390[]={'+','.join(map(str,tokens))+'};\n'
        if 'signature_465390' not in text:
            text=text.replace('inline constexpr ExtensionSignature extensionSignatures[]={',declaration+'inline constexpr ExtensionSignature extensionSignatures[]={\n    {0x465390,signature_465390,205,0x0,0,0,nullptr},')
            path.write_text(text,encoding='utf-8')
write_replay_adapter(root/'src/native/tests/atel_queue_bytes.inc', '\n'.join(rows)+'\n', encoding='utf-8')
(out/'provenance.json').write_text(json.dumps(dict(source=str(evidence_relative(source, root)),sha256=hashlib.sha256(data).hexdigest(),functions=records),indent=2),encoding='utf-8')
print('PASS: original ATEL request/dispatch bodies and bounded full scheduler signature retained.')
