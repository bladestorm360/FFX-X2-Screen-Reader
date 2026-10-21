"""Retain original airship cursor/count bodies for isolated offline replay.

Only reads the archived reference; Phyre translation is a modeled test boundary.
No installed source read, game execution, or graphics/runtime capture.
"""

# Game-source inputs/exports live outside every Git tree.
from pathlib import Path as _EvidencePath
import sys as _evidence_sys
_evidence_repo = next(p for p in _EvidencePath(__file__).resolve().parents
                      if (p / "Docs/Scripts/Offline/evidence_paths.py").is_file())
_evidence_sys.path.insert(0, str(_evidence_repo / "Docs/Scripts/Offline"))
from evidence_paths import evidence_path, evidence_relative, write_replay_adapter

from pathlib import Path
import hashlib
import json
from datetime import datetime, timezone
import capstone as cs

ROOT = Path(__file__).resolve().parents[3]
source = evidence_path(ROOT / 'Docs/Scripts/Offline/output/reader_reference_20261003_235502/block_00401000.bin')
data = source.read_bytes()
dis = cs.Cs(cs.CS_ARCH_X86, cs.CS_MODE_32)
dis.detail = True
lines = ['// Generated from retained original .text by audit_airship_render_20261019.py.',
         '// Isolated tests only; ReplayFixup is supplied by the shared native replay host.']
proof = []
for name, address, size in [('airshipCount', 0x91a3d0, 29), ('airshipPosition', 0x91ab30, 142), ('airshipTexture', 0x91a930, 60)]:
    body = data[address - 0x401000:address - 0x401000 + size]
    instructions = list(dis.disasm(body, address))
    assert sum(i.size for i in instructions) == size and instructions[-1].mnemonic == 'ret'
    fixes = []
    for ins in instructions:
        for op in ins.operands:
            value = (op.imm if op.type == cs.x86.X86_OP_IMM else op.mem.disp if op.type == cs.x86.X86_OP_MEM else 0) & 0xffffffff
            if not 0x400000 <= value < 0x2800000:
                continue
            offset, width = (ins.imm_offset, ins.imm_size) if op.type == cs.x86.X86_OP_IMM else (ins.disp_offset, ins.disp_size)
            if width != 4:
                # Internal short branches retain their relative displacement
                # when the complete function body moves as one unit.
                assert op.type == cs.x86.X86_OP_IMM and ins.mnemonic.startswith('j') and address <= value < address + size
                continue
            fixes.append((ins.address - address + offset, value - 0x400000, ins.mnemonic in ('call', 'jmp')))
    if name == 'airshipPosition':
        assert any(i.mnemonic == 'cmp' and i.op_str == 'dword ptr [0x1330804], 0x17e' for i in instructions)
        assert {v for _, v, _ in fixes} == {0x51a3d0, 0x153bf80, 0xf30804, 0x239ef0}
    lines.append(f'const unsigned char {name}Body[]={{' + ','.join(f'0x{x:02x}' for x in body) + '};')
    lines.append(f'const ReplayFixup {name}Fixups[]={{' + ','.join(f'{{{o},0x{v:x},{str(r).lower()}}}' for o, v, r in fixes) + '};')
    proof.append(dict(address=hex(address), bytes=size, sha256=hashlib.sha256(body).hexdigest(),
                      disassembly=[f'{i.address:08x} {i.mnemonic} {i.op_str}' for i in instructions]))
write_replay_adapter(ROOT / 'src/native/tests/airship_render_bytes.inc', '\n'.join(lines) + '\n', encoding='utf-8')
stamp = datetime.now(timezone.utc).strftime('%Y%m%d_%H%M%S_%f')
output = evidence_path(ROOT / f'Docs/Scripts/Offline/output/airship_render_{stamp}.json')
output.write_text(json.dumps(dict(source=str(source), functions=proof, boundary='Phyre translation call is modeled; original position storage/arithmetic and count getter execute.'), indent=2) + '\n', encoding='utf-8')
print(f'PASS: original29-byte count,142-byte cursor and60-byte texture bodies; room382 operand and complete fixups. {output}')
