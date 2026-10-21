"""Audit factual City gate ownership against retained original script data.

This is an offline adapter, not a copy of game implementation. Original models,
EBP, native bodies and exported reports remain outside all Git trees. Native
movement/geometry behavior is tested separately by production x86 hosts.
"""
from datetime import datetime, timezone
import hashlib
import json
import sys

from evidence_paths import REPOSITORY, evidence_path, outside_git

sys.path.insert(0, str(REPOSITORY / "src/puzzles"))
from atel_model import instructions
from cloister_inventory import script_chunk

analysis = outside_git(REPOSITORY.parent / "Analysis/FFX/SinDynamic_20261020/171143_927729")
model = json.loads((analysis / "sins0400.json").read_text(encoding="utf-8"))
source = evidence_path(REPOSITORY / "Docs/Scripts/Offline/output/FFX/20261003_audit/files/ffx_ps2/ffx/master/jppc/event/obj/si/sins0400/sins0400.ebp")
data = source.read_bytes()
assert hashlib.sha256(data).hexdigest() == model["sha256"] == "890748eedb4fb7d00623687751816b1dcb41b00f7161e64930f54485d7e5a25a"
chunk = script_chunk(data)
length = int.from_bytes(chunk[:4], "little")
code_at = int.from_bytes(chunk[0x30:0x34], "little")
assert length == 45865 and len(model["workers"]) == 100
ops = instructions(chunk[code_at:code_at + length])
rows = []
for worker_index, group, geometry_group in [(7,48,8),(8,49,9),(9,50,10),(13,51,19),(21,52,11),(23,53,21)]:
    worker = model["workers"][worker_index]
    statements = [s for b in worker["blocks"] for s in b["statements"]]
    calls = [s for s in statements if s.get("kind") == "call"]
    assert worker["type"] == 1
    assert any(s.get("id") == 4 and s.get("args") == [str(geometry_group)] for s in calls)
    gates = [s for s in calls if s.get("id") == 0x4017]
    assert {tuple(s["args"]) for s in gates} == {(str(group),"0"),(str(group),"1")}
    # Require actual CALL operands at the original model's bytecode locators.
    assert all(ops[s["pc"]][:2] == (0xd8, 0x4017) for s in gates)
    rows.append(dict(worker=worker_index, collision_group=group, geometry_group=geometry_group,
                     setter_pcs=[s["pc"] for s in gates], states=[0,1]))

camera = []
for index in range(65,99):
    worker = model["workers"][index]
    calls = [s for b in worker["blocks"] for s in b["statements"] if s.get("kind") == "call"]
    assert worker["type"] == 5 and any(s.get("id") == 2 for s in calls)
    assert any(s.get("id") == 0x8b for s in calls) and not any(s.get("id") == 0x4017 for s in calls)
    camera.append(index)

output = evidence_path(REPOSITORY / "Docs/Scripts/Offline/output/sin_collection_20261020")
output.mkdir(parents=True, exist_ok=True)
report = dict(utc=datetime.now(timezone.utc).isoformat(), ebp_sha256=model["sha256"],
              gate_workers=rows, camera_workers=camera,
              limits="Static source identity/ownership only; no installed City floor or gameplay replay. Original native collision selector and live route/input adapters have separate hosts.")
path = output / ("gates-audit-" + datetime.now(timezone.utc).strftime("%H%M%S_%f") + ".json")
path.write_text(json.dumps(report, indent=2), encoding="utf-8")
print("PASS: six original moving gate owners/groups/states and34 separate camera volumes; originals stay external.")
print(path)
