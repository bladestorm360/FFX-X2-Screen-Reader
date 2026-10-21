"""Retain the exact native numeric-input/popup evidence outside every Git tree.

Only archived executable/decompile inputs are read. Production observes the
renderer; its fixture replays the original input arithmetic with explicit SDK,
validation, sound and window-pop boundaries. No game launch or installed read.
"""
from pathlib import Path
import hashlib
import json
import capstone as cs
from capstone.x86 import X86_OP_IMM, X86_OP_MEM
from evidence_paths import evidence_root, outside_git

root=evidence_root()
out=outside_git(root/'offline-output/battle_amount_prompts_20261021')
out.mkdir(parents=True,exist_ok=True)
block=root/'offline-output/reader_reference_20261003_235502/block_00401000.bin'
raw=block.read_bytes()
va,size=0x8a8780,618
body=raw[va-0x401000:va-0x401000+size]
assert len(body)==size
decoder=cs.Cs(cs.CS_ARCH_X86,cs.CS_MODE_32);decoder.detail=True
fixups=[]
for ins in decoder.disasm(body,va):
    for op in ins.operands:
        value=(op.imm if op.type==X86_OP_IMM else op.mem.disp if op.type==X86_OP_MEM else 0)&0xffffffff
        if not 0x400000<=value<0x2800000: continue
        offset,width=(ins.imm_offset,ins.imm_size) if op.type==X86_OP_IMM else (ins.disp_offset,ins.disp_size)
        if width!=4: continue
        relative=ins.mnemonic=='call' or ins.group(cs.CS_GRP_JUMP)
        # Decimal powers are integer immediates, even when their numeric value
        # happens to fall inside the executable's VA range. Only memory operands
        # and actual control-flow targets are relocations in this controller.
        if op.type==X86_OP_IMM and not relative: continue
        if relative and va<=value<va+size: continue
        fixups.append([ins.address-va+offset,value,int(relative)])
(out/'amount-cursor.bin').write_bytes(body)
(out/'amount-cursor-fixups.txt').write_text(''.join(f'{a} {b} {c}\n' for a,b,c in fixups))
learn_va,learn_size=0x893560,104
learn=raw[learn_va-0x401000:learn_va-0x401000+learn_size]
learn_fixups=[]
for ins in decoder.disasm(learn,learn_va):
    if ins.mnemonic=='call':
        assert ins.imm_size==4
        learn_fixups.append([ins.address-learn_va+ins.imm_offset,ins.operands[0].imm])
(out/'overdrive-learn.bin').write_bytes(learn)
(out/'overdrive-learn-fixups.txt').write_text(''.join(f'{a} {b}\n' for a,b in learn_fixups))
learn_fnv=14695981039346656037
for byte in learn:learn_fnv=((learn_fnv^byte)*1099511628211)&0xffffffffffffffff
(out/'overdrive-learn-proof.json').write_text(json.dumps(dict(va=learn_va,bytes=learn_size,fnv64=learn_fnv,
    sha256=hashlib.sha256(learn).hexdigest(),calls=learn_fixups,
    boundaries='Original initializer; native text getters and namespace registration are controlled fixture boundaries.'),indent=2)+'\n')
print('Original104-byte learned-mode initializer FNV64',learn_fnv)
fnv=14695981039346656037
for byte in body: fnv=((fnv^byte)*1099511628211)&0xffffffffffffffff
proof={'source':str(block),'source_sha256':hashlib.sha256(raw).hexdigest(),
       'cursor_va':va,'bytes':size,'sha256':hashlib.sha256(body).hexdigest(),'fnv64':fnv,'fixups':fixups,
       'boundaries':'Original input; fixture validation/sound/window pop/security-cookie calls. Real renderer/input scheduling, PRISM and game playback remain unmeasured.'}
for file,names in [('layer2_game_logic.txt',['FUN_008a8780','FUN_008a3250','FUN_008a1fd0','FUN_0068df70','FUN_006fc890','FUN_0068fca0','FUN_00893560','FUN_0089db10','FUN_008b6610','FUN_007983a0','FUN_00875650','FUN_0086c270','FUN_00783bb0','FUN_00785330']),
                   ('layer3_iggy_interface.txt',['FUN_00a28d70'])]:
    archive=root/'ghidra-output/FFX/decompile'/file
    sections=archive.read_text(encoding='utf-8').split('// Function: ')
    for name in names:
        text=next(x for x in sections if x.startswith(name+'\n'))
        (out/(name+'.txt')).write_text('// Function: '+text,encoding='utf-8')
(out/'proof.json').write_text(json.dumps(proof,indent=2)+'\n')
print(f'PASS: original618-byte numeric input exported; FNV64 {fnv}; native popup ABI/body evidence retained outside Git.')
print('External CALL targets: '+','.join(hex(value) for _,value,rel in fixups if rel))
