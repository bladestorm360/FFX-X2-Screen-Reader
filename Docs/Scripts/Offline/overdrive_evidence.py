"""Read retained overdrive code/operands only; never opens the game install."""

# Game-source inputs/exports live outside every Git tree.
from pathlib import Path as _EvidencePath
import sys as _evidence_sys
_evidence_repo = next(p for p in _EvidencePath(__file__).resolve().parents
                      if (p / "Docs/Scripts/Offline/evidence_paths.py").is_file())
_evidence_sys.path.insert(0, str(_evidence_repo / "Docs/Scripts/Offline"))
from evidence_paths import evidence_path, evidence_relative, write_replay_adapter

from pathlib import Path
import bisect
import re
import struct
import sys
import capstone

ROOT = Path(__file__).resolve().parents[3]
ARCHIVE = evidence_path(ROOT / 'Docs/Scripts/Ghidra/output/FFX')
REFERENCE = evidence_path(ROOT / 'Docs/Scripts/Offline/output/reader_reference_20261003_235502')

def refs(values):
    entries = sorted(set(int(s, 16) for s in re.findall(
        r',([0-9a-fA-F]{8}),', (ARCHIVE / 'functions.csv').read_text())))
    for path in sorted(REFERENCE.glob('block_*.bin')):
        base = int(path.stem.split('_')[1], 16)
        data = path.read_bytes()
        for value in values:
            needle = struct.pack('<I', value)
            at = data.find(needle)
            while at >= 0:
                va = at + base
                owner = entries[bisect.bisect_right(entries, va) - 1]
                print(f'{value:08x}: {va:08x} nearest archived entry FUN_{owner:08x}')
                at = data.find(needle, at + 1)

def bodies(keys):
    for path in sorted((ARCHIVE / 'decompile').glob('*.txt')):
        for body in path.read_text(encoding='utf-8').split('// Function: ')[1:]:
            if any(key in body for key in keys):
                print(path.name, body.splitlines()[0], len(body))

def calls(target):
    data = (REFERENCE / 'block_00401000.bin').read_bytes()
    for opcode in (b'\xe8', b'\xe9'):
        at = data.find(opcode)
        while at >= 0 and at + 5 <= len(data):
            if at + 0x401005 + struct.unpack_from('<i', data, at + 1)[0] == target:
                print(f'branch {target:08x}: {at + 0x401000:08x}')
            at = data.find(opcode, at + 1)

def disassemble(start, end):
    data = (REFERENCE / 'block_00401000.bin').read_bytes()
    decoder = capstone.Cs(capstone.CS_ARCH_X86, capstone.CS_MODE_32)
    for ins in decoder.disasm(data[start-0x401000:end-0x401000], start):
        print(f'{ins.address:08x}: {ins.mnemonic} {ins.op_str}')

def data_at(start, count):
    for path in REFERENCE.glob('block_*.bin'):
        base = int(path.stem.split('_')[1], 16)
        data = path.read_bytes()
        if base <= start and start + count*4 <= base + len(data):
            for i in range(count):
                print(f'{start+i*4:08x}: {struct.unpack_from("<I", data, start-base+i*4)[0]:08x}')
            return

if __name__ == '__main__':
    if sys.argv[1] == 'refs':
        refs([int(value, 16) for value in sys.argv[2:]])
    elif sys.argv[1] == 'calls':
        calls(int(sys.argv[2], 16))
    elif sys.argv[1] == 'disasm':
        disassemble(*[int(value, 16) for value in sys.argv[2:]])
    elif sys.argv[1] == 'data':
        data_at(int(sys.argv[2], 16), int(sys.argv[3]))
    else:
        bodies(sys.argv[2:])
