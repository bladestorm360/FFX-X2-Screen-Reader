"""Read only the user's named Summon save and retained native/script evidence.

No game/installed asset reads or execution. ATEL parsing retains the existing
Karifean/FFXDataParser/Fahrenheit attribution through the shared compiler.
"""

# Game-source inputs/exports live outside every Git tree.
from pathlib import Path as _EvidencePath
import sys as _evidence_sys
_evidence_repo = next(p for p in _EvidencePath(__file__).resolve().parents
                      if (p / "Docs/Scripts/Offline/evidence_paths.py").is_file())
_evidence_sys.path.insert(0, str(_evidence_repo / "Docs/Scripts/Offline"))
from evidence_paths import evidence_path, evidence_relative, write_replay_adapter

from pathlib import Path
from datetime import datetime, timezone
import argparse
import hashlib
import json
import struct
import sys
import shutil
import capstone as cs

ROOT = Path(__file__).resolve().parents[3]
parser = argparse.ArgumentParser(description=__doc__)
parser.add_argument("--evidence-root", type=Path,
                    default=ROOT.parent / "Analysis/FFX/Summon_20261020",
                    help="Local analysis directory outside every Git tree")
args = parser.parse_args()
evidence_root = args.evidence_root.resolve()
if any((parent / ".git").exists() for parent in (evidence_root, *evidence_root.parents)):
    raise ValueError("Game-source analysis output must remain outside every Git tree")
evidence_root.mkdir(parents=True, exist_ok=True)
sys.path.insert(0, str(ROOT / "src/puzzles"))
from compile_trials import enrich
from atel_model import compile_script, render
from field_text import TextEvidence

out = evidence_root / ("summon_statues_" +
                       datetime.now(timezone.utc).strftime("%Y%m%d_%H%M%S_%f"))
out.mkdir()
save = ROOT.parent / "Saves/Dylan/Summon/ffx_000"
original = save.read_bytes()
assert len(original) == 64 + 0x68c0
payload = original[64:]
state = {name: struct.unpack_from("<H", payload, offset)[0] for name, offset in {
    "room": 0, "previous_room": 2, "area": 4, "saved_room": 0xba,
    "saved_previous_room": 0xc2, "saved_area": 0xc4, "story": 0xbec,
}.items()}
assert state["room"] == state["saved_room"] == 196
base = evidence_path(ROOT / "Docs/Scripts/Offline/output/FFX/20261003_audit")
catalog = json.loads((base / "maps_v3.json").read_text(encoding="utf-8"))
event = next(e for e in catalog["events"] if e["script"] == "bjyt1000" and "/psv_" not in e["source"])
script = enrich(event, base, TextEvidence(base / "text"))
model = compile_script(script)
(out / "bjyt1000.json").write_text(json.dumps(model, indent=2) + "\n", encoding="utf-8")
(out / "bjyt1000.txt").write_text(render(model), encoding="utf-8")
manifest = {
    "save": str(save), "save_bytes": len(original),
    "save_sha256": hashlib.sha256(original).hexdigest(), "save_state": state,
    "script": event["script"], "source": event["source"],
    "script_sha256": event["sha256"], "workers": len(model["workers"]),
    "boundaries": "Save contains persistent flags, not live worker enable/geometry state. Native scheduling and live input remain untested.",
}
manifest["baaj_prerequisites"] = {
    "script_saved_offset": "0xA70", "payload_offset": "0xC5C",
    "file_offset": "0xC9C",
    "value": payload[0x1ec + 0xa70],
    "six_prerequisite_bits": payload[0x1ec + 0xa70] & 0x3f,
    "final_seal_open": bool(payload[0x1ec + 0xa70] & 0x40),
}
# Independent layout witness: Fahrenheit names this exact payload byte for the
# Anima seals. Do not infer the tester's later/current runtime state from a save.
import re
layout = (ROOT.parent / "fahrenheit/core/fh/FFX/savedata.cs").read_text(encoding="utf-8-sig")
match = re.search(r"FieldOffset\(0x([0-9A-Fa-f]+)\)\][^\n]*\banima_seals_unlocked\b", layout)
assert match and int(match[1], 16) == 0x1ec + 0xa70
manifest["baaj_prerequisites"]["independent_layout"] = "Fahrenheit FFX SaveData.anima_seals_unlocked at0xC5C"

# Original inputs/replay bodies stay external. The repository fixture contains
# only original mod adapter code and loads these separately supplied inputs.
inputs = evidence_root / "inputs"
inputs.mkdir(exist_ok=True)
shutil.copyfile(base / "files" / event["source"], inputs / "bjyt1000.ebp")
archive = evidence_path(ROOT / "Docs/Scripts/Offline/output/reader_reference_20261003_235502")
text = (archive / "block_00401000.bin").read_bytes()
dis = cs.Cs(cs.CS_ARCH_X86, cs.CS_MODE_32)
dis.detail = True
for name, start, size in (("type6", 0x867bb0, 1027), ("distance_squared", 0x88c330, 50)):
    body = text[start - 0x401000:start - 0x401000 + size]
    instructions = list(dis.disasm(body, start))
    assert sum(i.size for i in instructions) == size and instructions[-1].mnemonic == "ret"
    fixes = []
    for instruction in instructions:
        for operand in instruction.operands:
            value = (operand.imm if operand.type == cs.x86.X86_OP_IMM else operand.mem.disp if operand.type == cs.x86.X86_OP_MEM else 0) & 0xffffffff
            if not 0x400000 <= value < 0x2800000 or start <= value < start + size:
                continue
            immediate = operand.type == cs.x86.X86_OP_IMM
            at, width = (instruction.imm_offset, instruction.imm_size) if immediate else (instruction.disp_offset, instruction.disp_size)
            assert width == 4
            fixes.append((instruction.address - start + at, value - 0x400000,
                          int(immediate and instruction.mnemonic in ("call", "jmp"))))
    (inputs / (name + ".bin")).write_bytes(body)
    (inputs / (name + ".fixups")).write_bytes(b"".join(struct.pack("<III", *fix) for fix in fixes))
constant = (archive / "block_00b0c000.bin").read_bytes()[0xb92340 - 0xb0c000:0xb92348 - 0xb0c000]
assert struct.unpack("<d", constant)[0] == -1.0
(inputs / "negative_one.bin").write_bytes(constant)
manifest["external_fixture_inputs"] = {p.name: hashlib.sha256(p.read_bytes()).hexdigest() for p in inputs.iterdir() if p.is_file()}
# Identify the prerequisite by actual temple writes, rather than assigning a
# temple from an assumed bit order. This is source evidence, not save editing.
temple_writes = []
for name in ("bsyt0100", "klyt1000", "djyt0600", "mcyt0500", "bvyt1100", "dome0700"):
    original_event = next(e for e in catalog["events"] if e["script"] == name and "/psv_" not in e["source"])
    trial = compile_script(enrich(original_event, base, TextEvidence(base / "text")))
    writes = [{"worker": w["index"], "pc": hex(s["pc"]), "value": s["value"]}
              for w in trial["workers"] for b in w["blocks"] for s in b["statements"]
              if s["kind"] == "write" and "L0:map:000a70:" in s.get("binding", "")]
    temple_writes.append({"script": name, "source_sha256": original_event["sha256"], "writes": writes})
manifest["temple_prerequisite_writes"] = temple_writes
(out / "prerequisite-writes.json").write_text(json.dumps(temple_writes, indent=2) + "\n", encoding="utf-8")
(out / "manifest.json").write_text(json.dumps(manifest, indent=2) + "\n", encoding="utf-8")
assert hashlib.sha256(save.read_bytes()).hexdigest() == manifest["save_sha256"]
print("Save:", state)
print("Original bjyt1000:", len(model["workers"]), "workers; external output", out)
print("Prerequisite evidence:", len(temple_writes), "temple scripts; details remain external")
