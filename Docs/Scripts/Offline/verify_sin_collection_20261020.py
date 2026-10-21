"""Verify the retained original collection contract; export facts outside Git.

No installed archive, game process, native variable write or copied game source
is part of this adapter. Original parsing attribution remains with the retained
Karifean/FFXDataParser model and Fahrenheit call/layout evidence.
"""
from pathlib import Path
from datetime import datetime, timezone
import hashlib
import json
import sys

from evidence_paths import REPOSITORY, outside_git, evidence_path

sys.path.insert(0, str(REPOSITORY / "src/puzzles"))
from atel_model import instructions
from cloister_inventory import script_chunk

analysis = outside_git(REPOSITORY.parent / "Analysis/FFX/SinDynamic_20261020/171143_927729")
model = json.loads((analysis / "sins0500.json").read_text(encoding="utf-8"))
source = evidence_path(REPOSITORY / "Docs/Scripts/Offline/output/FFX/20261003_audit/files/ffx_ps2/ffx/master/jppc/event/obj/si/sins0500/sins0500.ebp")
data = source.read_bytes()
assert hashlib.sha256(data).hexdigest() == model["sha256"] == "c81148c2c9d6d8c7394d97b3d2285c3b7a77a839543b74d91dfc03ac764cb229"
chunk = script_chunk(data)
length = int.from_bytes(chunk[:4], "little")
code_at = int.from_bytes(chunk[0x30:0x34], "little")
code = chunk[code_at:code_at + length]
assert length == 60258 and len(model["workers"]) == 44
ops = instructions(code)
rows = []
for index in range(10):
    worker = model["workers"][24 + index]
    pc, variable = 0xbeda + 596 * index, 207 + 3 * index
    declaration = worker["bindings"][variable]
    assert declaration["raw_low"] == 0x66000000 and declaration["raw_high"] == 1
    assert ops[pc][:2] == (0x9f, variable)
    assert ops[pc - 12][:2] == (0xa0, variable)  # Original distance write stays separate.
    assert ops[pc + 3][0] == 0xaf and ops[pc + 6][0] == 0x0b
    assert ops[pc + 7][:2] == (0x9f, 12) and ops[pc + 10][0] == 2  # Native control AND distance.
    statements = [s for block in worker["blocks"] for s in block["statements"]]
    reward = [s for s in statements if s.get("id") == 0x15b]
    assert len(reward) == 1 and reward[0]["args"] == ["1", str(486 + index)]
    assert any(s.get("binding") == f"L0:map:{0x2af + index:06x}:T0" and s.get("value") == "1" for s in statements)
    assert any(s.get("id") == 0 and s.get("args") == ["90"] for s in statements)
    assert any("150" in s.get("condition", "") for s in statements)
    rows.append(dict(worker=24 + index, read_pc=pc, variable=variable, native_control_preserved=True,
                     reward_calls=1, appearance_wait_ticks=90, expiry_iterations=150))
finish = model["workers"][40]
finish_statements = [s for b in finish["blocks"] for s in b["statements"]]
assert any("01a988" in s.get("condition", "") and "01a989" in s.get("condition", "") and "< 10" in s["condition"] for s in finish_statements)
assert any(s.get("id") == 0x10b and s.get("args") == ["325", "0"] for s in finish_statements)
out = evidence_path(REPOSITORY / "Docs/Scripts/Offline/output/sin_collection_20261020")
out.mkdir(parents=True, exist_ok=True)
receipt = dict(utc=datetime.now(timezone.utc).isoformat(), ebp_sha256=model["sha256"], rows=rows,
               native_counter_threshold=10, native_next_room=325,
               limits="Original static bytecode contract; full VM, item backend, animations and game timing remain unexecuted.")
path = out / ("contract-audit-" + datetime.now(timezone.utc).strftime("%H%M%S_%f") + ".json")
path.write_text(json.dumps(receipt, indent=2), encoding="utf-8")
print("PASS: ten original pickup/read/write/control/reward/expiry contracts and native count10 progression; originals stay external.")
print(path)
