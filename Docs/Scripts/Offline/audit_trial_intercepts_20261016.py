"""Check six generated interaction contracts against retained original ATEL.

Read-only workspace evidence; no installed assets, game process or tester logs.
Writes a timestamped audit beside other offline investigation outputs.
"""

# Game-source inputs/exports live outside every Git tree.
from pathlib import Path as _EvidencePath
import sys as _evidence_sys
_evidence_repo = next(p for p in _EvidencePath(__file__).resolve().parents
                      if (p / "Docs/Scripts/Offline/evidence_paths.py").is_file())
_evidence_sys.path.insert(0, str(_evidence_repo / "Docs/Scripts/Offline"))
from evidence_paths import evidence_path, evidence_relative, write_replay_adapter

import hashlib
import json
import re
import sys
from datetime import datetime, timezone
from pathlib import Path

ROOT = Path(__file__).resolve().parents[3]
sys.path.insert(0, str(ROOT / "src/puzzles"))
from build_utilities import DESIGNS, EVIDENCE, RECIPES
from build_puzzle_controls import INTERACTIONS
from compile_trials import fnv32, reachable
from atel_model import instructions


def main():
    catalog = json.loads((EVIDENCE / "maps_v3.json").read_text(encoding="utf8"))
    generated = (ROOT / "src/native/puzzle_solver_data.hpp").read_text(encoding="utf8")
    utilities = (ROOT / "src/native/utility_trials_data.hpp").read_text(encoding="utf8")
    records = []
    for name, (locals_, destruction) in INTERACTIONS.items():
        flow = json.loads((DESIGNS / (name + "_flow.json")).read_text(encoding="utf8"))
        design = json.loads((DESIGNS / (name + "_design.json")).read_text(encoding="utf8"))
        event = next(e for e in catalog["events"] if e["script"] == flow["script"])
        source = EVIDENCE / "files" / event["source"]
        assert hashlib.sha256(source.read_bytes()).hexdigest() == flow["sha256"] == event["sha256"]
        code = bytes.fromhex(event["code"])
        decoded = instructions(code)
        utility_row = next(line for line in utilities.splitlines() if '"' + flow["script"] + '"' in line)
        identity = re.search(r'"' + flow["script"] + r'", (\d+), 0x([0-9a-f]+)', utility_row)
        assert len(code) == int(identity[1]) and fnv32(code) == int(identity[2], 16)
        row = next(line for line in generated.splitlines() if '"' + flow["script"] + '"' in line)
        compiled = [(int(a), int(b), int(c), int(d), e == "true") for a, b, c, d, e in re.findall(
            r'\{(\d+), (\d+), (\d+), (\d+), (true|false), \{[^}]+\}, \{[^}]+\}\}', row)]
        actions = []
        for local in locals_:
            worker = flow["workers"][local]
            entry = 6 if name == "bevelle" and local == 39 else 2
            statements = [s for b in reachable(worker, entry) for s in b["statements"]]
            returns = [s["pc"] for s in statements if s["kind"] == "end" and s["opcode"] == "RET"]
            if not returns or worker["entries"][entry] == returns[-1]:
                continue
            ret = returns[-1]
            assert decoded[ret][0] == 0x3c
            actions.append(dict(worker=local, entry=entry, entry_pc=worker["entries"][entry], return_pc=ret,
                destruction=local in destruction,
                requests=[s for s in statements if s["kind"] == "request"],
                choices=[s for s in statements if s["kind"] == "call" and s["id"] == 0x13b]))
        assert compiled == [(a["worker"], a["entry"], a["entry_pc"], a["return_pc"], a["destruction"]) for a in actions]
        recipe = next(r for r in RECIPES if r[0] == name)
        owned = [("reveal", recipe[1], 7 if name == "bevelle" else 2),
                 ("collect", design["treasure"]["worker"], 8 if name == "bevelle" else 2)]
        overlaps = [dict(stage=stage, worker=w, entry=e) for stage, w, e in owned
                    if any(a["worker"] == w and a["entry"] == e for a in actions)]
        record = dict(name=name, script=flow["script"], source=event["source"], source_sha256=flow["sha256"],
            bytecode_sha256=hashlib.sha256(code).hexdigest(), bytecode_fnv32=fnv32(code),
            actions=actions, owned_entry_overlaps=overlaps)
        outside = []
        for local, worker in enumerate(flow["workers"]):
            if local in locals_ or len(worker["entries"]) <= 2:
                continue
            statements = [s for b in reachable(worker, 2) for s in b["statements"]]
            choices = [s for s in statements if s["kind"] == "call" and s["id"] == 0x13b]
            if not choices:
                continue
            # All remaining sphere menus belong to internal type4 helpers.
            # Zanarkand W9 is the save/menu producer, not a puzzle action.
            assert worker["type"] == 4 or (name == "zanarkand" and local == 9)
            if worker["type"] != 4:
                assert any(s["kind"] == "call" and s["id"] == 0x16f for s in statements)
                assert any(s["kind"] == "call" and s["id"] == 0x401d for s in statements)
            outside.append(dict(worker=local, type=worker["type"], choices=choices,
                role="internal helper" if worker["type"] == 4 else "native save/menu producer"))
        record["choice_owners_outside_player_contract"] = outside
        if name == "djose":
            record["sphere_helper"] = dict(worker=43, entry=2,
                calls=[s for b in reachable(flow["workers"][43], 2) for s in b["statements"]
                       if s["kind"] == "call" and s["id"] in (0x64, 0x13b)])
        records.append(record)
        print(f"PASS: {name}: {len(actions)} original entry/RET contracts; owned-entry overlaps {overlaps}")
    stamp = datetime.now(timezone.utc).strftime("%Y%m%d_%H%M%S")
    output = evidence_path(ROOT / "Docs/Scripts/Offline/output/trial_intercepts_20261016")
    output.mkdir(exist_ok=True)
    target = output / ("contracts_" + stamp + ".json")
    target.write_text(json.dumps(dict(checked_at_utc=datetime.now(timezone.utc).isoformat(), trials=records,
        limitation="Static original control-flow/identity checks. Native queue/production solver fixtures are separate; UI and animation remain simulated."), indent=2) + "\n", encoding="utf8")
    print(f"PASS: all six retained scripts and {sum(len(r['actions']) for r in records)} contracts match production; {target}")


if __name__ == "__main__":
    main()
