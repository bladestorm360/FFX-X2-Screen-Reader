"""Replay retained chest bytecode; never open the game or change a save.

Original bytecode/data remain outside Git. This mod-authored bounded interpreter
executes integer/control-flow instructions; native grant/UI/request/REQCHG
effects are recorded dependencies. It proves script order and mask preservation,
not the cause of an earlier session's missing bit or runtime completion.
"""
from pathlib import Path
from datetime import datetime, timezone
import hashlib
import json
import sys

ROOT = Path(__file__).resolve().parents[3]
sys.path.insert(0, str(ROOT / "src/puzzles"))
from build_utilities import EVIDENCE, DESIGNS, RECIPES
from compile_trials import enrich, fnv32
from field_text import TextEvidence
from atel_model import binding, instructions, signatures
from evidence_paths import outside_git


def replay(event, local, start, reg_y, flags, calls):
    worker = event["workers"][local]
    decoded = instructions(bytes.fromhex(event["code"]))
    values = {("L0:map:000a70:T0", 0): flags}
    stack, registers = [], {0x25: 0, 0x2a: 0, 0x2c: reg_y}
    observed, writes, visited = [], [], []
    pc = start
    for _ in range(2000):
        visited.append(pc)
        op, arg, nxt = decoded[pc]
        if op == 0xae:
            stack.append(arg if arg < 32768 else arg - 65536)
        elif op == 0xad:
            stack.append(worker["ints"][arg])
        elif op in (0x9f, 0xa2, 0xa7):
            element = stack.pop() if op != 0x9f else 0
            key = binding(worker, arg), element
            stack.append(("pointer", key) if op == 0xa7 else values.get(key, 0))
        elif op in (0xa0, 0xa1, 0xa3, 0xa4):
            value = stack.pop()
            element = stack.pop() if op in (0xa3, 0xa4) else 0
            key = binding(worker, arg), element
            values[key] = value
            writes.append(dict(pc=pc, binding=key[0], element=element, value=value))
        elif op == 0x2b:
            stack.append(stack[-1])
        elif op in registers:
            registers[op] = stack.pop()
        elif op in (0x26, 0x28, 0x29):
            stack.append(registers[{0x26: 0x25, 0x28: 0x2a, 0x29: 0x2c}[op]])
        elif op in (3, 6):
            right, left = stack.pop(), stack.pop()
            stack.append(left | right if op == 3 else int(left == right))
        elif op in (0xb0, 0xb1, 0xb2, 0xd5, 0xd6, 0xd7):
            condition = stack.pop() if op >= 0xd5 else registers[0x2a]
            if op in (0xb0, 0xd5) or bool(condition) == (op in (0xb1, 0xd6)):
                nxt = worker["jumps"][arg]
        elif op in (0xb5, 0xd8):
            signature = calls[arg]
            assert signature["agree"] == "yes", "Unverified native arity"
            args = [stack.pop() for _ in range(int(signature["arg_count_kari"]))][::-1]
            observed.append(dict(pc=pc, kind="native", id=arg, args=args))
            result = 1 if arg == 0x8f else 0  # Besaid's native text-advance boundary.
            if op == 0xb5:
                stack.append(result)
            else:
                registers[0x25] = result
        elif op in (0x36, 0x38):
            args = [stack.pop() for _ in range(3)][::-1]
            observed.append(dict(pc=pc, kind="request", opcode=op, args=args))
            stack.append(1)
        elif op == 0x79:
            args = [stack.pop() for _ in range(3)][::-1]
            assert args[0][0] == "pointer"
            observed.append(dict(pc=pc, kind="interaction_change", args=args))
        elif op == 0x3c:
            assert not stack, "Unbalanced operand stack at original final RET"
            return dict(final_flags=values["L0:map:000a70:T0", 0],
                        ret=pc, instructions=len(visited), calls=observed, writes=writes)
        else:
            raise AssertionError(f"Unmodeled original opcode {op:02x} at {pc}")
        pc = nxt
    raise AssertionError("Original chest exceeded the 2000-instruction work bound")


def main():
    catalog = json.loads((EVIDENCE / "maps_v3.json").read_text(encoding="utf8"))
    text = TextEvidence(EVIDENCE / "text")
    generated = (ROOT / "src/native/utility_trials_data.hpp").read_text(encoding="utf8")
    calls, _ = signatures()
    records = []
    # The explicit report order is Besaid first, Zanarkand second.
    order = [RECIPES[n] for n in (0, 5, 1, 2, 3, 4)]
    for name, _, _, _, _, collect_pc, reg_y in order:
        design = json.loads((DESIGNS / (name + "_design.json")).read_text(encoding="utf8"))
        event = enrich(next(e for e in catalog["events"] if e["script"] == design["script"]), EVIDENCE, text)
        source = EVIDENCE / "files" / event["source"]
        assert hashlib.sha256(source.read_bytes()).hexdigest() == event["sha256"]
        local, mask = design["treasure"]["worker"], design["treasure"]["mask"]
        entry = 8 if name == "bevelle" else 2
        start = event["workers"][local]["entries"][entry] if collect_pc is None else collect_pc
        witness = replay(event, local, start, reg_y, 0, calls)
        grants = [c for c in witness["calls"] if c["kind"] == "native" and c["id"] == 0x15b]
        flag_writes = [w for w in witness["writes"] if w["binding"] == "L0:map:000a70:T0"]
        assert len(grants) == len(flag_writes) == 1
        assert grants[0]["args"] == [1, design["treasure"]["id"]]
        assert max(grants[0]["pc"], flag_writes[0]["pc"]) < witness["ret"]
        if name in ("besaid", "zanarkand"):
            assert grants[0]["pc"] < flag_writes[0]["pc"]
        row = next(row for row in generated.splitlines() if '"' + event["script"] + '"' in row)
        assert f"0x{fnv32(bytes.fromhex(event['code'])):x}," in row
        assert f"{{{local}, {entry}, {event['workers'][local]['entries'][entry]}, {start}, {reg_y}, -1, false, {witness['ret']}}}" in row
        max_instructions = 0
        for flags in range(256):
            result = replay(event, local, start, reg_y, flags, calls)
            assert result["final_flags"] == flags | mask and result["ret"] == witness["ret"]
            max_instructions = max(max_instructions, result["instructions"])
        records.append(dict(temple=name, script=event["script"], source_sha256=event["sha256"],
                            bytecode_sha256=hashlib.sha256(bytes.fromhex(event["code"])).hexdigest(),
                            worker=local, entry=entry, start=start, mask=mask, native_witness=witness,
                            flag_cases=256, max_instructions=max_instructions))
    save = ROOT.parent / "Saves/Dylan/Summon/ffx_000"
    data = save.read_bytes()
    assert data[0xc9c] == 0x3e
    output = outside_git(ROOT.parent / "Analysis/FFX/DestructionFlags_20261022")
    output.mkdir(parents=True, exist_ok=True)
    target = output / ("audit_" + datetime.now(timezone.utc).strftime("%Y%m%d_%H%M%S") + ".json")
    target.write_text(json.dumps(dict(utc=datetime.now(timezone.utc).isoformat(), trials=records,
        save=dict(path=str(save), sha256=hashlib.sha256(data).hexdigest(), flags=data[0xc9c], modified=False),
        limit=__doc__), indent=2) + "\n", encoding="utf8")
    print("PASS: Besaid then Zanarkand, all6 original chest continuations/all256 byte values (1536 cases): reward and correct Baaj bit precede exact final RET; reserved/other bits preserved. Native/UI effects modeled.")
    print(target)


if __name__ == "__main__":
    main()
