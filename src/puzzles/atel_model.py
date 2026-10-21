"""Compile archived ATEL into reviewable control flow; never access the game.

Opcode names and call signatures: Karifean/FFXDataParser and Fahrenheit.
Native location decoding: archived FUN_0086c270 / FUN_0086c500. Expressions
at block boundaries deliberately remain symbolic; this is not a game VM.
"""
import csv
import json
import sys
from pathlib import Path

ROOT = Path(__file__).resolve().parents[2]
sys.path.insert(0, str(ROOT / "Docs/Scripts/Offline"))
from evidence_paths import evidence_path
TABLES = evidence_path(ROOT / "Docs/Scripts/Ghidra/output/FFX/atel")
BRANCHES = {0xB0, 0xB1, 0xB2, 0xD5, 0xD6, 0xD7}
ENDS = {0x34, 0x3C, 0x3D, 0x3E, 0x3F, 0x40, 0x54}
REQS = {0x36, 0x37, 0x38, 0x45, 0x46, 0x47, 0x48, 0x49,
        0x4A, 0x4B, 0x4C, 0x4D, 0x4E, 0x4F, 0x50, 0x51, 0x52, 0x53}
BINARY = dict(enumerate(("or", "and", "|", "^", "&", "==", "!=",
                         ">u", "<u", ">", "<", ">=u", "<=u", ">=", "<=",
                         "bit_on", "bit_off", "<<", ">>", "+", "-", "*", "/", "%"), 1))


def signatures():
    calls = {}
    for path in sorted(TABLES.glob("atel_calls_*.tsv")):
        with path.open(encoding="utf-8") as stream:
            for row in csv.DictReader(stream, delimiter="\t"):
                calls[int(row["id"], 16)] = row
    with (TABLES / "atel_opcodes.tsv").open(encoding="utf-8") as stream:
        names = {int(r["opcode"], 16): r["fh_mnemonic"] for r in csv.DictReader(stream, delimiter="\t")}
    return calls, names


def instructions(code):
    result, pc = {}, 0
    while pc < len(code):
        op = code[pc]
        size = 3 if op & 0x80 else 1
        if pc + size > len(code):
            raise ValueError("Truncated ATEL instruction")
        arg = int.from_bytes(code[pc + 1:pc + size], "little") if size == 3 else None
        result[pc] = (op, arg, pc + size)
        pc += size
    return result


def binding(worker, index):
    if not 0 <= index < len(worker["declarations"]):
        raise ValueError(f"Variable {index} outside declaration table")
    d = worker["declarations"][index]
    loc, offset = d["reference_location"], d["offset"]
    base = {2: worker.get("data_offset"), 3: worker["private_data_offset"],
            4: worker["shared_data_offset"], 6: worker.get("event_data_offset")}.get(loc)
    # Loc 3 may be relocated by the native controller callback. Keep its owner.
    scope = f"w{worker['index']}" if loc in (3, 5) else "map"
    address = offset + base if base is not None else offset
    return f"L{loc}:{scope}:{address:06x}:T{d['reference_format']}"


def compile_script(script):
    calls, names = signatures()
    code = bytes.fromhex(script["code"])
    ins = instructions(code)
    workers = []
    for index, original in enumerate(script["workers"]):
        w = dict(original, index=index)
        starts = set(w["entries"] + w["jumps"])
        for pc, (op, arg, nxt) in ins.items():
            if op in BRANCHES or op in ENDS or op == 0xB3:
                if nxt in ins:
                    starts.add(nxt)
        blocks, pending = {}, list(w["entries"])
        while pending:
            start = pending.pop()
            if start in blocks:
                continue
            if start not in ins:
                raise ValueError(f"{script['script']} w{index}: invalid target {start}")
            stack, regs, statements, edges = [], {}, [], []

            def pop():
                return stack.pop() if stack else "?stack"

            def emit(pc, kind, **values):
                statements.append(dict(pc=pc, kind=kind, **values))

            pc = start
            while pc in ins:
                if pc != start and pc in starts:
                    edges.append(dict(target=pc, when="true"))
                    break
                op, arg, nxt = ins[pc]
                name = names.get(op, f"OP{op:02x}")
                if op in (0xAE, 0xAD, 0xAF):
                    value = (arg if arg < 32768 else arg - 65536) if op == 0xAE else w["ints" if op == 0xAD else "floats"][arg]
                    stack.append(str(value))
                elif op in (0x9F, 0xA2, 0xA7):
                    ref = binding(w, arg)
                    element = pop() if op != 0x9F else "0"
                    stack.append(f"{'&' if op == 0xA7 else ''}{ref}[{element}]")
                elif op in (0xA0, 0xA1, 0xA3, 0xA4):
                    value = pop()
                    element = pop() if op in (0xA3, 0xA4) else "0"
                    emit(pc, "write", variable=arg, binding=binding(w, arg), element=element,
                         value=value, limited=op in (0xA1, 0xA4))
                elif op in BINARY:
                    right, left = pop(), pop()
                    stack.append(f"({left} {BINARY[op]} {right})")
                elif op in (0x19, 0x1A, 0x1C):
                    stack.append(f"({'!' if op == 0x19 else '-' if op == 0x1A else '~'}{pop()})")
                elif op in (0x25, 0x2A, 0x2C):
                    key = {0x25: "A", 0x2A: "X", 0x2C: "Y"}[op]
                    regs[key] = pop()
                    emit(pc, "register", register=key, value=regs[key])
                elif op in (0x26, 0x28, 0x29):
                    key = {0x26: "A", 0x28: "X", 0x29: "Y"}[op]
                    stack.append(regs.get(key, "?" + key))
                elif op == 0x2B:
                    value = pop()
                    stack.extend([value, value])
                elif op in (0xB5, 0xD8):
                    sig = calls.get(arg)
                    count = int(sig["arg_count_kari"]) if sig else 0
                    args = [pop() for _ in range(count)][::-1]
                    if sig is None:
                        # No signature means unknown argument ownership. Never
                        # leak an earlier call's literals into a later call.
                        stack.clear()
                    emit(pc, "call", id=arg, name=(sig["name"] or sig["internal_name"]) if sig else "unknown",
                         args=args, signature_known=sig is not None,
                         signature_agrees=sig is not None and sig["agree"] == "yes")
                    result = f"call@{pc:04x}"
                    # CALL returns a stack value; CALLPOPA consumes it into rA.
                    if op == 0xB5:
                        stack.append(result)
                    else:
                        regs["A"] = result
                elif op in REQS:
                    entry, target, level = pop(), pop(), pop()
                    emit(pc, "request", opcode=name, worker=target, entry=entry, level=level)
                    stack.append(f"request@{pc:04x}")
                elif op == 0xB3:
                    emit(pc, "subroutine", worker=arg)
                    edges.append(dict(target=nxt, when="after subroutine"))
                    break
                elif op in BRANCHES:
                    if arg >= len(w["jumps"]):
                        raise ValueError(f"{script['script']} w{index}: jump {arg} outside table")
                    target = w["jumps"][arg]
                    condition = pop() if op in (0xD5, 0xD6, 0xD7) else regs.get("X", "?X")
                    if op in (0xB0, 0xD5):
                        edges.append(dict(target=target, when="true"))
                    else:
                        positive = op in (0xB1, 0xD6)
                        edges.extend([dict(target=target, when=condition if positive else f"!({condition})"),
                                      dict(target=nxt, when=f"!({condition})" if positive else condition)])
                    emit(pc, "branch", opcode=name, jump_index=arg, condition=condition)
                    break
                elif op in ENDS:
                    emit(pc, "end", opcode=name)
                    break
                elif 0x59 <= op <= 0x66:
                    emit(pc, "register", register=name, value=pop())
                elif 0x67 <= op <= 0x74:
                    stack.append("?" + name)
                elif op not in (0x00, 0x9D, 0x9E):
                    emit(pc, "opaque", opcode=name, operand=arg)
                    stack.clear()
                pc = nxt
            blocks[start] = dict(start=start, statements=statements, edges=edges)
            pending.extend(e["target"] for e in edges)
        workers.append(dict(index=index, type=w["type"], entries=w["entries"],
                            bindings=[dict(**d, key=binding(w, d["index"])) for d in w["declarations"]],
                            blocks=sorted(blocks.values(), key=lambda b: b["start"])))
    return dict(script=script["script"], sha256=script["sha256"],
                strings=script["english_strings"], areas=script["area_names"], workers=workers)


def render(model):
    lines = [f"Script {model['script']} SHA256 {model['sha256']}"]
    for w in model["workers"]:
        lines.append(f"\nWORKER {w['index']} type {w['type']} entries {w['entries']}")
        for block in w["blocks"]:
            lines.append(f"  BLOCK {block['start']:04x}")
            for s in block["statements"]:
                prefix = f"    {s['pc']:04x} "
                if s["kind"] == "write":
                    body = f"{s['binding']}[{s['element']}] = {s['value']} (var {s['variable']})"
                elif s["kind"] == "call":
                    body = f"{s['name']}[{s['id']:04x}]({', '.join(s['args'])})"
                    if s["id"] in (0x64, 0x13B) and len(s["args"]) > 1 and s["args"][1].isdigit():
                        i = int(s["args"][1])
                        if i < len(model["strings"]):
                            body += " TEXT " + repr(model["strings"][i])
                else:
                    body = json.dumps({k: v for k, v in s.items() if k != "pc"})
                lines.append(prefix + body)
            for e in block["edges"]:
                lines.append(f"    -> {e['target']:04x} when {e['when']}")
    return "\n".join(lines) + "\n"
