"""Export original HP/MP playback/getters outside Git for bounded offline replay."""
from pathlib import Path
import hashlib,json
import capstone as cs
from capstone.x86 import X86_OP_MEM
from evidence_paths import evidence_root,outside_git

root=evidence_root();out=outside_git(root/'offline-output/battle_hp_20261021');out.mkdir(exist_ok=True)
source=root/'offline-output/reader_reference_20261003_235502/block_00401000.bin'
rdata_source=source.with_name('block_00b0c000.bin');rdata=rdata_source.read_bytes()
data=source.read_bytes();decoder=cs.Cs(cs.CS_ARCH_X86,cs.CS_MODE_32);decoder.detail=True
records=[];constants={};constant_bytes=bytearray()
for va,size in ((0x78d330,291),(0x78e2f0,263),(0x78e400,94),(0x79a0d0,26),(0x79ade0,22),(0x79ae00,22),(0x79ae60,22),(0x79ae80,22),(0x892c20,1071)):
    body=data[va-0x401000:va-0x401000+size];fixups=[]
    for ins in decoder.disasm(body,va):
        if ins.mnemonic=='call':
            target=ins.operands[0].imm
            assert ins.imm_size==4
            if not va<=target<va+size:fixups.append((ins.address-va+ins.imm_offset,target,1))
        for op in ins.operands:
            if op.type==X86_OP_MEM and not op.mem.base and ins.disp_size==4:
                target=op.mem.disp&0xffffffff
                if 0x400000<=target<0x2800000:
                    fixups.append((ins.address-va+ins.disp_offset,target,0))
                    if va==0x892c20 and target==0x893050:
                        table=data[target-0x401000:target-0x401000+16]
                        (out/'renderer-jump-table.bin').write_bytes(table)
                    elif va==0x892c20 and target not in constants:
                        assert op.size<=16 and 0xb0c000<=target<0xc0a000
                        constants[target]=len(constant_bytes)
                        constant_bytes.extend(rdata[target-0xb0c000:target-0xb0c000+16])
    stem=f'{va:08x}';(out/(stem+'.bin')).write_bytes(body)
    (out/(stem+'-fixups.txt')).write_text(''.join(f'{a} {b} {c}\n' for a,b,c in fixups))
    fnv=14695981039346656037
    for byte in body:fnv=((fnv^byte)*1099511628211)&0xffffffffffffffff
    records.append(dict(va=va,bytes=size,sha256=hashlib.sha256(body).hexdigest(),fnv64=fnv,fixups=fixups))
    print(stem,size,fnv)
assert len(constant_bytes)<=1024
constant_bytes.extend(bytes(1024-len(constant_bytes)))
(out/'renderer-constants.bin').write_bytes(constant_bytes)
(out/'renderer-constants-map.txt').write_text(''.join(f'{a} {b}\n' for a,b in constants.items()))
fnv=14695981039346656037
for byte in constant_bytes:fnv=((fnv^byte)*1099511628211)&0xffffffffffffffff
print('renderer-constants',1024,fnv)
fnv=14695981039346656037
for byte in table:fnv=((fnv^byte)*1099511628211)&0xffffffffffffffff
print('renderer-jump-table',16,fnv)
(out/'playback-proof.json').write_text(json.dumps(dict(source=str(source),source_sha256=hashlib.sha256(data).hexdigest(),
    rdata_source=str(rdata_source),rdata_source_sha256=hashlib.sha256(rdata).hexdigest(),
    renderer_constants_sha256=hashlib.sha256(constant_bytes).hexdigest(),renderer_constant_offsets=constants,
    renderer_jump_table_sha256=hashlib.sha256(table).hexdigest(),
    records=records,boundaries='Original normal HP/MP playback, effective-capacity mutation, Switch renderer and clamp/getters. Encounter, eligibility, actor lookup, display scaling/graphics/color submission controlled; no Regen scheduler/VM/game/SDK/frame/audio execution.'),indent=2)+'\n')
