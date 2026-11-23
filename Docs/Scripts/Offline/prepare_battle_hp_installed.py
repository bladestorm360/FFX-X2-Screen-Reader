"""Rebuild the battle HP replay inputs from the installed Steam FFX.exe.

The original inputs came from the reference build. Eight of the nine
functions are the same code at new addresses in the installed build, so
rewriting their call and data fields to reference addresses reproduces the
original bytes and hashes. The HP apply function (reference 0x78e2f0) grew
in the installed build, and the renderer's constants moved by varying
amounts, so those two files keep installed addresses where no reference
address is known. The test models those extra addresses.

Usage: uv run --with capstone python prepare_battle_hp_installed.py [FFX.exe]
Output goes to <evidence root>/offline-output/battle_hp_20261021.
"""
import struct
import sys
from pathlib import Path

import capstone as cs
from capstone.x86 import X86_OP_MEM

from evidence_paths import evidence_root, outside_git

DEFAULT_EXE = (r"C:\Program Files (x86)\Steam\steamapps\common"
               r"\FINAL FANTASY FFX&FFX-2 HD Remaster\FFX.exe")
exe = Path(sys.argv[1] if len(sys.argv) > 1 else DEFAULT_EXE)
raw = exe.read_bytes()
pe = struct.unpack_from('<I', raw, 0x3c)[0]
count = struct.unpack_from('<H', raw, pe + 6)[0]
optional = struct.unpack_from('<H', raw, pe + 20)[0]
image_base = struct.unpack_from('<I', raw, pe + 52)[0]
sections = []
for i in range(count):
    o = pe + 24 + optional + i * 40
    vsize, rva, rsize, rptr = struct.unpack_from('<IIII', raw, o + 8)
    sections.append((image_base + rva, vsize, rptr))


def read(va, size):
    for start, vsize, rptr in sections:
        if start <= va < start + vsize:
            return raw[rptr + va - start:rptr + va - start + size]
    raise ValueError(hex(va))


# installed VA -> (reference VA, size)
FUNCTIONS = {
    0x78d270: (0x78d330, 291), 0x78e230: (0x78e2f0, 371),
    0x78e3b0: (0x78e400, 94), 0x79a0c0: (0x79a0d0, 26),
    0x79add0: (0x79ade0, 22), 0x79adf0: (0x79ae00, 22),
    0x79ae50: (0x79ae60, 22), 0x79ae70: (0x79ae80, 22),
    0x892cb0: (0x892c20, 1071),
}
# installed VA -> reference VA for callees and globals the test models
TARGETS = {
    0x794020: 0x794030, 0x7816b0: 0x781780, 0x78d3a0: 0x78d460,
    0x79fa00: 0x79fa20, 0x644820: 0x6449d0, 0x6447e0: 0x644990,
    0x8ac2f0: 0x8ac2a0, 0x892910: 0x892880, 0x903bb0: 0x903bb0,
    0x905890: 0x905890, 0x78bf00: 0x78bfc0, 0xc421ec: 0xc421dc,
    0x113338c: 0x113338c, 0x8930e0: 0x893050,
}
TARGETS.update({k: v[0] for k, v in FUNCTIONS.items()})
# Installed-only flags in the grown HP apply function; kept as installed VAs.
INSTALLED_ONLY = {0x112ca15, 0x11334c4}
RENDERER, JUMP_TABLE = 0x892cb0, 0x8930e0


def fnv64(data):
    h = 14695981039346656037
    for byte in data:
        h = ((h ^ byte) * 1099511628211) & 0xffffffffffffffff
    return h


root = evidence_root()
out = outside_git(root / 'offline-output/battle_hp_20261021')
out.mkdir(parents=True, exist_ok=True)
decoder = cs.Cs(cs.CS_ARCH_X86, cs.CS_MODE_32)
decoder.detail = True
constants, constant_bytes = {}, bytearray()
for va, (reference, size) in FUNCTIONS.items():
    body = bytearray(read(va, size))
    fixups = []
    for ins in decoder.disasm(bytes(body), va):
        if ins.mnemonic == 'call' and ins.imm_size == 4:
            target = ins.operands[0].imm
            if not va <= target < va + size:
                offset = ins.address - va + ins.imm_offset
                original = TARGETS[target]
                body[offset:offset + 4] = ((original - (reference + offset + 4))
                                           & 0xffffffff).to_bytes(4, 'little')
                fixups.append((offset, original, 1))
        for op in ins.operands:
            if op.type != X86_OP_MEM or op.mem.base or ins.disp_size != 4:
                continue
            target = op.mem.disp & 0xffffffff
            if not 0x400000 <= target < 0x2800000:
                continue
            offset = ins.address - va + ins.disp_offset
            if target in TARGETS:
                original = TARGETS[target]
            elif target in INSTALLED_ONLY:
                original = target
            else:
                assert va == RENDERER and 0xb0c000 <= target < 0xc0a000, hex(target)
                original = target
                if target not in constants:
                    constants[target] = len(constant_bytes)
                    constant_bytes.extend(read(target, 16))
            body[offset:offset + 4] = original.to_bytes(4, 'little')
            fixups.append((offset, original, 0))
    stem = f'{reference:08x}'
    (out / (stem + '.bin')).write_bytes(body)
    (out / (stem + '-fixups.txt')).write_text(
        ''.join(f'{a} {b} {c}\n' for a, b, c in fixups))
    print(stem, size, fnv64(body))
table = bytearray()
for i in range(4):
    label = struct.unpack_from('<I', read(JUMP_TABLE, 16), i * 4)[0]
    table += struct.pack('<I', label - RENDERER + 0x892c20)
(out / 'renderer-jump-table.bin').write_bytes(table)
print('renderer-jump-table', 16, fnv64(table))
assert len(constant_bytes) <= 1024
constant_bytes.extend(bytes(1024 - len(constant_bytes)))
(out / 'renderer-constants.bin').write_bytes(constant_bytes)
(out / 'renderer-constants-map.txt').write_text(
    ''.join(f'{a} {b}\n' for a, b in constants.items()))
print('renderer-constants', 1024, fnv64(constant_bytes))
