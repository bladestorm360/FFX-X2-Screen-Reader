"""Check completion locators against retained originals; export native replay outside Git.

Square Enix inputs stay in the existing evidence archive. ATEL parsing/layout
attribution: Karifean/FFXDataParser and the project's Fahrenheit references.
This never opens an installed asset or game process.
"""
from pathlib import Path
import hashlib
import json
import re
import struct
import sys

ROOT = Path(__file__).resolve().parents[3]
sys.path.insert(0, str(ROOT / "Docs/Scripts/Offline"))
from evidence_paths import evidence_root, outside_git
import capstone as cs
from capstone.x86 import X86_OP_IMM, X86_OP_MEM

EVIDENCE = evidence_root()
OUT = outside_git(EVIDENCE / "offline-output/minigame_completion_20261020")
OUT.mkdir(parents=True, exist_ok=True)
MODELS = EVIDENCE / "offline-output/minigames_20261007_104142"
REMIEM = ROOT.parent / "Analysis/FFX/MinigameCompletion_20261020/192607_554908"
ARCHIVE = EVIDENCE / "offline-output/FFX/20261003_audit/files"


def fnv(data, bits=32):
    h, prime = (2166136261, 16777619) if bits == 32 else (14695981039346656037, 1099511628211)
    for b in data:
        h = ((h ^ b) * prime) & ((1 << bits) - 1)
    return h


records = json.loads((MODELS / "manifest.json").read_text())["records"]
records = [r for r in records if r["script"] in ("nagi0000", "bltz0000")]
for n in ("mcfr0100", "mcfr0200"):
    records.append(dict(script=n, file=n+".json", folder=EVIDENCE / "offline-output/butterfly_20261008",
                        source=f"ffx_ps2/ffx/master/jppc/event/obj/mc/{n}/{n}.ebp"))
rm = json.loads((REMIEM / "manifest.json").read_text())
records.append(dict(script="lmyt0000", file="lmyt0000.json", folder=REMIEM, source=rm["source"], sha256=rm["sha256"]))

# Compare reviewed production facts to independent retained decoded statements.
source = (ROOT / "src/native/minigame_completion.cpp").read_text()
rules = {}
for row in re.findall(r'\{Setting::(\w+),"(\w+)",([^}]+)\}', source):
    nums = [int(x.strip(), 0) for x in row[2].split(",")]
    rules[nums[0]] = (row[0], row[1], nums)
proof, cases = [], []
for r in records:
    model = json.loads((r.get("folder", MODELS) / r["file"]).read_text())
    code = bytes.fromhex(model["script"]["code"])
    h = fnv(code)
    kind, name, nums = rules[h]
    _, length, count, local, entry, pc, index, low, init1, init2, *targets = nums
    flow = model["compiled"]
    original = ARCHIVE / r["source"]
    digest = hashlib.sha256(original.read_bytes()).hexdigest()
    assert digest == flow["sha256"] == r.get("sha256", digest)
    assert name == model["script"]["script"] and length == len(code) and count == len(flow["workers"])
    w = flow["workers"][local]
    statements = {s["pc"]: s for b in w["blocks"] for s in b["statements"]}
    trigger = statements[pc]
    assert trigger["kind"] == "write" and trigger["variable"] == index
    assert code[pc] == 0xa0 and struct.unpack_from("<H", code, pc+1)[0] == index
    assert w["bindings"][index]["raw_low"] == low and w["bindings"][index]["raw_high"] == 1
    assert w["entries"][entry] <= pc
    for init in (init1, init2):
        if init:
            assert statements[init]["kind"] == "write" and statements[init]["variable"] == index
    if kind == "ChocoboAssist":
        assert flow["workers"][109]["entries"][2] == targets[0]
        assert flow["workers"][108]["entries"][2] == targets[1]
        # Negative raw time is the native distinction between displayed zero
        # and the perfect result's persistent eligibility bit.
        branches = [s for b in flow["workers"][72]["blocks"] for s in b["statements"]]
        assert any(s.get("binding") == "L0:map:00008d:T0" and "128" in s.get("value", "") for s in branches)
        assert any(s.get("id") == 347 and s.get("args") == ["0", "274"] for s in branches)
    if kind == "RemiemAssist":
        assert flow["workers"][102]["entries"][2] == targets[0]
        finish = [s for b in flow["workers"][102]["blocks"] for s in b["statements"]]
        assert any(s.get("worker") == "37" and s.get("entry") == "6" for s in finish)
        assert any(s.get("id") == 93 for s in finish)
        reward_flow = [s for b in flow["workers"][37]["blocks"] for s in b["statements"]]
        # Native chest tiers have independent saved flags and remain ordered.
        for tier, mask in enumerate((8,4,16,32,64),1):
            assert any(s.get("binding") == "L0:map:000a9b:T0" and s.get("value") == f"(L0:map:000a9b:T0[0] | {mask})" for s in reward_flow)
            assert any(s.get("kind") == "branch" and f"00000b:T0[0] == {tier}" in s.get("condition", "") for s in reward_flow)
    if kind == "ButterflyAssist":
        counter = 36 if count == 55 else 25
        assert flow["workers"][counter]["entries"][6] == targets[0]
        chest = 27 if count == 55 else 17
        counter_flow = [s for b in flow["workers"][counter]["blocks"] for s in b["statements"]]
        assert any(s.get("worker") == str(chest) and s.get("entry") == "13" for s in counter_flow)
        # All seven picked guards are used by the real collision callbacks.
        guards = {b["raw_low"] for worker in flow["workers"] for b in worker["bindings"]}
        assert set(range(0x0c000043, 0x0c00004a)) <= guards
    if kind == "BlitzAssist":
        # All eight real action menu variants converge on this one accepted
        # return assignment; no movement/formation/forfeit text is a trigger.
        delta = 1082 if length == 4646208 else 0
        for menu_pc in (3074033,3074060,3074087,3074114,3074183,3074210,3074237,3074264):
            assert statements[menu_pc-delta]["id"] == 0x13b
        clock = [s for b in flow["workers"][23]["blocks"] for s in b["statements"]]
        assert any(s["pc"] == 2868116 and s.get("variable") == 52 and s.get("value") == "0" for s in clock)
    archive_name = r["source"].replace("\\", "/")
    cases.append(f'{kind} {h} {local} {entry} {pc} {index} {init1} {init2} "{archive_name}"')
    proof.append(dict(model=r["file"], sha256=digest, checksum=h, length=length, worker=local, entry=entry, pc=pc))

(OUT / "cases.txt").write_text("\n".join(cases)+"\n", encoding="utf-8")

# Native typed reads/writes, rather than a second C++ interpretation of the
# declaration arithmetic. Keep all original bytes and fixups external.
native = EVIDENCE / "offline-output/reader_reference_20261003_235502/block_00401000.bin"
raw = native.read_bytes()
dis = cs.Cs(cs.CS_ARCH_X86, cs.CS_MODE_32)
dis.detail = True
native_records = []
for name, va, size in (("base",0x86c270,232),("pointer",0x86c500,124),("setter",0x8713a0,224)):
    body = raw[va-0x401000:va-0x401000+size]
    ins = list(dis.disasm(body, va))
    assert sum(i.size for i in ins) == size and ins[-1].mnemonic == "ret"
    fixes = []
    for i in ins:
        for o in i.operands:
            relative = False
            if o.type == X86_OP_IMM:
                v, offset, width = o.imm & 0xffffffff, i.imm_offset, i.imm_size
                if i.group(cs.CS_GRP_CALL) or i.group(cs.CS_GRP_JUMP):
                    if va <= v < va+size:
                        continue
                    relative = True
            elif o.type == X86_OP_MEM:
                v, offset, width = o.mem.disp & 0xffffffff, i.disp_offset, i.disp_size
            else:
                continue
            if 0x400000 <= v < 0x2800000:
                assert width == 4
                fixes.append((i.address-va+offset, v, int(relative)))
    (OUT / (name+".bin")).write_bytes(body)
    (OUT / (name+"-fixups.txt")).write_text("\n".join("%d %d %d" % x for x in fixes)+"\n")
    native_records.append(dict(name=name, va=va, size=size, sha256=hashlib.sha256(body).hexdigest(), fnv64=fnv(body,64)))
for name, va, n in (("base",0x86c358,7),("pointer",0x86c57c,8),("setter",0x871480,7)):
    (OUT / (name+"-table.bin")).write_bytes(raw[va-0x401000:va-0x401000+n*4])
(OUT / "proof.json").write_text(json.dumps(dict(cases=proof, native=native_records,
    native_source_sha256=hashlib.sha256(raw).hexdigest(),
    limitations="Component native/ATEL-data evidence. Complete animation, renderer, audio, story saves and gameplay timing require user confirmation."),indent=2))
print(f"PASS: {len(proof)} original activity variants, exact start/action/finish locators; external typed native replay exported.")
