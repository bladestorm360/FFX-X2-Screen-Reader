"""Retain the original Help renderer for the isolated reader fixture only."""

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

ROOT = Path(__file__).resolve().parents[3]
archive = evidence_path(ROOT / 'Docs/Scripts/Offline/output/reader_reference_20261003_235502/block_00401000.bin')
data = archive.read_bytes()
start, size = 0x76f960, 2740
body = data[start-0x401000:start-0x401000+size]
jump_targets=struct.unpack_from('<7I',data,start-0x401000+size)
assert all(start <= x < start+size for x in jump_targets)
decoder = cs.Cs(cs.CS_ARCH_X86, cs.CS_MODE_32)
decoder.detail = True
fixups, calls = [], set()
for ins in decoder.disasm(body, start):
    for operand in ins.operands:
        if operand.type == X86_OP_IMM:
            value = operand.imm & 0xffffffff
            if ins.group(cs.CS_GRP_JUMP) or ins.group(cs.CS_GRP_CALL):
                if not start <= value < start+size:
                    fixups.append((ins.address-start+ins.imm_offset, value-0x400000, True))
                    calls.add(value)
                continue
            offset = ins.imm_offset
        elif operand.type == X86_OP_MEM:
            value = operand.mem.disp & 0xffffffff
            offset = ins.disp_offset
        else:
            continue
        if 0x400000 <= value < 0x2800000:
            fixups.append((ins.address-start+offset, value-0x400000, False))
out = evidence_path(ROOT / 'Docs/Scripts/Offline/output/report_20261015')
out.mkdir(exist_ok=True)
(out/'help-renderer-provenance.json').write_text(json.dumps(dict(
    source=str(evidence_relative(archive, ROOT)), source_sha256=hashlib.sha256(data).hexdigest(),
    va=start, size=size, body_sha256=hashlib.sha256(body).hexdigest(),
    calls=sorted(calls), fixups=fixups, jump_targets=jump_targets), indent=2))
target = ROOT / 'src/native/tests/help_renderer_bytes.inc'
write_replay_adapter(target, '// Original renderer, isolated offline fixture; never linked into the mod.\n'
    'const unsigned char helpRendererBody[]={\n'+
    '\n'.join('    '+','.join(f'0x{x:02x}' for x in body[i:i+24])+',' for i in range(0,len(body),24))+
    '\n};\nstruct HelpFixup {unsigned offset,rva;bool relative;};\nconst HelpFixup helpFixups[]={\n'+
    '\n'.join(f'    {{{offset},0x{rva:x},{str(relative).lower()}}},' for offset,rva,relative in fixups)+'\n};\n'+
    'const unsigned helpJumpTargets[]={'+','.join(hex(x-0x400000) for x in jump_targets)+'};\n')
print('Original Help renderer retained:',len(body),'bytes;',len(fixups),'verified operand relocations; calls:',','.join(hex(c) for c in sorted(calls)))
