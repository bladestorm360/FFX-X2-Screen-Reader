"""Retain original native HUD clipping bodies; no installed-game access."""

# Game-source inputs/exports live outside every Git tree.
from pathlib import Path as _EvidencePath
import sys as _evidence_sys
_evidence_repo = next(p for p in _EvidencePath(__file__).resolve().parents
                      if (p / "Docs/Scripts/Offline/evidence_paths.py").is_file())
_evidence_sys.path.insert(0, str(_evidence_repo / "Docs/Scripts/Offline"))
from evidence_paths import evidence_path, evidence_relative, write_replay_adapter

from pathlib import Path
import hashlib,json,struct
import capstone as cs
from capstone.x86 import X86_OP_MEM
root=Path(__file__).resolve().parents[3]
src=evidence_path(root/'Docs/Scripts/Offline/output/reader_reference_20261003_235502/block_00401000.bin')
data=src.read_bytes();out=evidence_path(root/'Docs/Scripts/Offline/output/battle_hud_20261016');out.mkdir(exist_ok=True)
dis=cs.Cs(cs.CS_ARCH_X86,cs.CS_MODE_32);dis.detail=True
constants=(src.parent/'block_00b0c000.bin').read_bytes()
outside=struct.unpack_from('<f',constants,0xb368c0-0xb0c000)[0];assert outside==-100000
rows=['// Original FFX textured/plain clipping. Fixture only.','#pragma once','inline constexpr float hudClipOutside=-100000.f;','struct HudFixup {unsigned offset,rva;};'];proof=[]
for name,va,size in [('hudTextureClip',0x8e5a20,683),('hudPlainClip',0x8e58e0,310)]:
    body=data[va-0x401000:va-0x401000+size];ins=list(dis.disasm(body,va));fix=[]
    assert sum(i.size for i in ins)==size and ins[-1].mnemonic=='ret'
    for i in ins:
        assert i.mnemonic!='call'
        for o in i.operands:
            if o.type==X86_OP_MEM and not o.mem.base and 0x400000<=o.mem.disp<0x2800000:
                assert i.disp_size==4;fix.append((i.address-va+i.disp_offset,o.mem.disp-0x400000))
    rows+=['inline constexpr unsigned char '+name+'Body[]={'+','.join(map(str,body))+'};',
        'inline constexpr HudFixup '+name+'Fixups[]={'+','.join('{%d,0x%x}'%f for f in fix)+'};']
    proof.append(dict(name=name,va=va,size=size,sha256=hashlib.sha256(body).hexdigest(),fixups=fix))
    (out/(name+'.txt')).write_text('\n'.join('%08x %s %s'%(i.address,i.mnemonic,i.op_str) for i in ins),encoding='utf-8')
write_replay_adapter(root/'src/native/tests/battle_hud_bytes.inc', '\n'.join(rows)+'\n', encoding='utf-8')
(out/'proof.json').write_text(json.dumps(dict(source_sha256=hashlib.sha256(data).hexdigest(),functions=proof,
    limits='Actual native clip arithmetic executes. HUD call context, texture identity, status row association and GPU submission are controlled; original callers separately retained and signature-qualified.'),indent=2),encoding='utf-8')
print('PASS: original683/310-byte textured/plain clip bodies, exact globals and hashes retained.')
