"""Qualify Customize's native controller from the retained reference only.

No executable is run and no game-installation file is read. The output is new
review evidence; retained archives and existing generated artifacts are unchanged.
"""

# Game-source inputs/exports live outside every Git tree.
from pathlib import Path as _EvidencePath
import sys as _evidence_sys
_evidence_repo = next(p for p in _EvidencePath(__file__).resolve().parents
                      if (p / "Docs/Scripts/Offline/evidence_paths.py").is_file())
_evidence_sys.path.insert(0, str(_evidence_repo / "Docs/Scripts/Offline"))
from evidence_paths import evidence_path, evidence_relative, write_replay_adapter

from pathlib import Path
import json
import re
import capstone as cs
from capstone.x86 import X86_OP_IMM, X86_OP_MEM

root = Path(__file__).resolve().parents[3]
reference = evidence_path(root / "Docs/Scripts/Offline/output/reader_reference_20261003_235502")
data = (reference / "block_00401000.bin").read_bytes()
symbols = (evidence_path(root / "Docs/Scripts/Ghidra/output/FFX/functions.csv")).read_text(encoding="utf-8")
rva = 0x4d5800
assert re.search(r",008d5800,", symbols, re.I), "Not an archived function entry"
dis = cs.Cs(cs.CS_ARCH_X86, cs.CS_MODE_32)
dis.detail = True
tokens, operands, instructions = [], [], []
for ins in dis.disasm(data[rva-0x1000:rva-0x1000+768], rva+0x400000):
    raw = list(ins.bytes)
    instructions.append(f"{ins.address:08x}: {ins.mnemonic} {ins.op_str}")
    for operand in ins.operands:
        value = (operand.imm if operand.type == X86_OP_IMM else operand.mem.disp if operand.type == X86_OP_MEM else 0) & 0xffffffff
        if not 0x400000 <= value < 0x2800000:
            continue
        offset, size = (ins.imm_offset, ins.imm_size) if operand.type == X86_OP_IMM else (ins.disp_offset, ins.disp_size)
        if size == 4 and value == 0x186aa28:
            operands.append(dict(reference=value-0x400000, owner=rva, offset=len(tokens)+offset))
        for i in range(offset, offset+size):
            raw[i] = None
    tokens.extend(raw)
    if len(tokens) >= 128 or ins.mnemonic == "ret":
        break
first, last = rva-4096-0x1000, rva+4096-0x1000
matches = [position+0x1000 for position in range(first, last-len(tokens)+1)
           if all(token is None or data[position+i] == token for i, token in enumerate(tokens))]
assert matches == [rva], matches
assert operands, "Missing native Customize state operand"
output = root / "install/field-butterfly-20261008"
output.mkdir(exist_ok=True)
record = dict(reference=str(reference), rva=rva, tokens=tokens, operands=operands,
              archive_matches=matches, instructions=instructions)
(output / "customization-signature.json").write_text(json.dumps(record, indent=2)+"\n", encoding="utf-8")
print("Unique archived Customize controller: RVA4D5800; signature bytes:", len(tokens))
print("inline constexpr int signature_4d5800[]={"+",".join("-1" if b is None else str(b) for b in tokens)+"};")
for operand in operands:
    print("Operand:", operand)
