"""Compare archived x86 native bodies to the authorized installed PE, offline.

Only relocation/call operands are masked. Stack/layout/control instructions
must agree. Ambiguous matches and conflicting references remain unresolved.
This produces audit evidence, never writes the installed game or launches it.
"""

# Game-source inputs/exports live outside every Git tree.
from pathlib import Path as _EvidencePath
import sys as _evidence_sys
_evidence_repo = next(p for p in _EvidencePath(__file__).resolve().parents
                      if (p / "Docs/Scripts/Offline/evidence_paths.py").is_file())
_evidence_sys.path.insert(0, str(_evidence_repo / "Docs/Scripts/Offline"))
from evidence_paths import evidence_path, evidence_relative, write_replay_adapter

import argparse
import bisect
from datetime import datetime, timezone
import hashlib
import json
from pathlib import Path
import re

import capstone as cs
from capstone.x86 import X86_OP_IMM, X86_OP_MEM
import pefile

ROOT = Path(__file__).resolve().parents[3]
BASE = 0x400000
LIMIT = 0x2400000
disassembler = cs.Cs(cs.CS_ARCH_X86, cs.CS_MODE_32)
disassembler.detail = True


def references(instruction):
    result = []
    for operand in instruction.operands:
        if operand.type == X86_OP_IMM:
            value = operand.imm & 0xffffffff
            if BASE <= value < LIMIT:
                result.append((instruction.imm_offset, instruction.imm_size, value))
        elif operand.type == X86_OP_MEM:
            value = operand.mem.disp & 0xffffffff
            if BASE <= value < LIMIT:
                result.append((instruction.disp_offset, instruction.disp_size, value))
    return result


def signature(code, va, minimum=64):
    tokens, instructions, size = [], [], 0
    for instruction in disassembler.disasm(code, va):
        bytes_ = list(instruction.bytes)
        for offset, length, _ in references(instruction):
            for j in range(offset, offset + length):
                bytes_[j] = None
        instructions.append(instruction)
        tokens.extend(bytes_)
        size += instruction.size
        if size >= minimum:
            break
    return tokens, instructions


def pattern(tokens):
    return b''.join(b'.' if value is None else re.escape(bytes([value])) for value in tokens)


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('reference', type=Path)
    parser.add_argument('executable', type=Path)
    parser.add_argument('--xrefs', type=Path)
    parser.add_argument('--extra', action='append', type=lambda value:int(value,0), default=[], help='Additional archived functions to verify their call contexts')
    args = parser.parse_args()
    old_text = (args.reference / 'block_00401000.bin').read_bytes()
    installed_bytes = args.executable.read_bytes()
    pe = pefile.PE(data=installed_bytes)
    image = pe.get_memory_mapped_image()
    symbols = {}
    with (evidence_path(ROOT / 'Docs/Scripts/Ghidra/output/FFX/functions.csv')).open(encoding='utf-8') as stream:
        # Some recovered C++ symbol names contain unquoted commas in this export.
        for line in stream:
            location = re.search(r',([0-9a-fA-F]{8}),', line)
            if location:
                symbols[int(location[1], 16) - BASE] = line[:location.start()].strip('"')
    starts = sorted(symbols)
    literals = set()
    explicit = set()
    for source in (ROOT / 'src/native').glob('*'):
        if source.suffix in ('.cpp', '.hpp') and source.name not in ('profile.hpp', 'profile.cpp'):
            # Generated checksums/ATEL declarations are data, not PE RVAs.
            # Utility native addresses are all explicit address()/hook() calls.
            if not source.name.endswith('_data.hpp') and not source.name.startswith(('utility','puzzle')):
                literals.update(int(value, 16) for value in re.findall(r'0x([0-9a-fA-F]{6,8})', source.read_text()))
            explicit.update(int(value, 16) for value in re.findall(r'(?:address|hook)\(0x([0-9a-fA-F]+)', source.read_text()))
    required = sorted({value for value in literals if 0x100000 < value < LIMIT - BASE} | (explicit - {0}) | set(args.extra))
    functions = [value for value in required if value in symbols]
    mappings, references_, proofs, ambiguous = {}, {}, [], []
    for offset in functions:
        end_index = bisect.bisect_right(starts, offset)
        body_size = starts[end_index] - offset if end_index < len(starts) else 128
        size = min(body_size, 128)
        tokens, old_instructions = signature(old_text[offset - 0x1000:offset - 0x1000 + size], BASE + offset, size)
        if len(tokens) < 5:
            ambiguous.append(dict(old=offset, reason='Short or undecoded function'))
            continue
        left, right = max(0x1000, offset - 4096), min(len(image), offset + 4096)
        matches = [left + m.start() for m in re.finditer(pattern(tokens), image[left:right], re.DOTALL)]
        if len(matches) != 1:
            ambiguous.append(dict(old=offset, candidates=matches))
            continue
        current = matches[0]
        mappings[offset] = current
        body_size = min(body_size, 32768)
        old_instructions = list(disassembler.disasm(old_text[offset - 0x1000:offset - 0x1000 + body_size], BASE + offset))
        current_instructions = list(disassembler.disasm(image[current:current + body_size], BASE + current))
        pairs = []
        verified_bytes = 0
        for old, new in zip(old_instructions, current_instructions):
            old_tokens, _ = signature(bytes(old.bytes), old.address, old.size)
            new_tokens, _ = signature(bytes(new.bytes), new.address, new.size)
            if old_tokens != new_tokens:
                break
            verified_bytes += old.size
            old_refs, new_refs = references(old), references(new)
            assert len(old_refs) == len(new_refs)
            for (_, _, old_va), (_, _, new_va) in zip(old_refs, new_refs):
                references_.setdefault(old_va - BASE, set()).add(new_va - BASE)
                pairs.append([old_va - BASE, new_va - BASE])
        proofs.append(dict(old=offset, current=current, name=symbols[offset],
                           bytes=len(tokens), matched_body_bytes=verified_bytes,
                           source_body_bytes=body_size, reference_pairs=pairs))
    xref_proofs = []
    if args.xrefs:
        for line in args.xrefs.read_text().splitlines():
            old_hex, anchor_hex, start_hex, code_hex = line.split('\t')
            target, anchor, start = int(old_hex, 16), int(anchor_hex, 16), int(start_hex, 16)
            if target in mappings:
                continue
            code = bytes.fromhex(code_hex)
            tokens, old_instructions = signature(code, BASE + start, len(code))
            left, right = max(0x1000, start - 4096), min(len(image), start + 4096)
            matches = [left + m.start() for m in re.finditer(pattern(tokens), image[left:right], re.DOTALL)]
            if len(matches) != 1:
                continue
            current = matches[0]
            new_instructions = list(disassembler.disasm(image[current:current + len(tokens)], BASE + current))
            if len(old_instructions) != len(new_instructions):
                continue
            for old, new in zip(old_instructions, new_instructions):
                if old.address != BASE + anchor:
                    continue
                for (_, _, old_va), (_, _, new_va) in zip(references(old), references(new)):
                    if old_va - BASE == target:
                        references_.setdefault(target, set()).add(new_va - BASE)
                        xref_proofs.append(dict(old=target, current=new_va - BASE,
                            source_anchor=anchor, current_anchor=new.address - BASE,
                            context_bytes=len(tokens)))
    conflicts = []
    for old, values in references_.items():
        if len(values) != 1:
            conflicts.append(dict(old=old, candidates=sorted(values)))
        elif old in mappings and mappings[old] not in values:
            conflicts.append(dict(old=old, candidates=sorted(values | {mappings[old]})))
        else:
            mappings[old] = next(iter(values))
    unresolved = [value for value in required if value not in mappings]
    output = args.reference / ('reader_audit_' + datetime.now(timezone.utc).strftime('%Y%m%d_%H%M%S') + '.json')
    report = dict(executable_sha256=hashlib.sha256(installed_bytes).hexdigest(),
        reference_info=(args.reference / 'program.tsv').read_text(),
        required=required, functions=len(functions), matched=proofs, ambiguous=ambiguous,
        conflicts=conflicts, unresolved=unresolved, xref_proofs=xref_proofs,
        mappings={hex(old): hex(new) for old, new in sorted(mappings.items())},
        scope='Static native body matching and reference consistency; not live gameplay validation')
    output.write_text(json.dumps(report, indent=2) + '\n', encoding='utf-8')
    print(f'Native bodies matched: {len(proofs)}/{len(functions)}; unresolved required addresses: {len(unresolved)}; conflicts: {len(conflicts)}.')
    print('Audit: ' + str(output))


if __name__ == '__main__':
    main()
