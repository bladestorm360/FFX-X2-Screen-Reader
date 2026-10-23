"""Compile native utility contracts from existing hashed, offline evidence.

Original ATEL continuations perform reveal/collection effects. This generator
records their instruction boundaries and worker-local entry identities; it
does not attach to FFX or open the game install.
"""

# Game-source inputs/exports live outside every Git tree.
from pathlib import Path as _EvidencePath
import sys as _evidence_sys
_evidence_repo = next(p for p in _EvidencePath(__file__).resolve().parents
                      if (p / "Docs/Scripts/Offline/evidence_paths.py").is_file())
_evidence_sys.path.insert(0, str(_evidence_repo / "Docs/Scripts/Offline"))
from evidence_paths import evidence_path, evidence_relative, write_replay_adapter

from datetime import datetime, timezone
import hashlib
import json
from pathlib import Path
import re
import sys

ROOT = Path(__file__).resolve().parents[2]
sys.path.insert(0, str(ROOT / "Docs/Scripts/Offline"))
from extract_maps import room_index
from field_text import TextEvidence
from atel_model import instructions
from compile_trials import reachable

EVIDENCE = evidence_path(ROOT / "Docs/Scripts/Offline/output/FFX/20261003_audit")
DESIGNS = EVIDENCE / "cloister_designs_20261004_062020_133594"

# name, reveal worker/PC, ready binding/value/element, preparation, collect
# continuation override/regY. Values refer to original script assignments
# or the guards selecting those assignments, never object-model guesses.
RECIPES = [
    ("besaid", 46, 0xd4d2, ("L6:map:011e6c:T5", 1, 0),
     [("L6:map:011e40:T5", 0, 0), ("L6:map:011e50:T5", 1, 0)], None, 0),
    # Worker 35 is the lower treasure recess. Worker 36 burns the final door;
    # its 010c63=3 branch never reveals the Destruction Sphere chest.
    # Use the complete original interaction, including control ownership,
    # accepted insertion choice, attachment, proxy positioning and GFX68 bind.
    # The old53283 tail detached an actor it had never attached/positioned.
    # Native code commits recess=6 and clears carried=6 itself at52677/52683.
    ("kilika", 35, 51412, ("L6:map:010c61:T0", 6, 0),
     [("L6:map:010c60:T0", 0, 0), ("L6:map:010c61:T0", 0, 0), ("L6:map:010c5b:T0", 6, 0)], None, 0),
    ("djose", 44, 0x13c24, ("L6:map:018233:T0", 2, 0),
     [("L6:map:0181f0:T0", 10, 0), ("L6:map:0181e4:T0", 0, 0)], 47845, 69),
    ("macalania", 32, 0x1244d, ("L6:map:015a91:T0", 10, 0),
     [("L6:map:015a91:T0", 10, 0), ("L6:map:015a8c:T0", 0, 0),
      ("L6:map:015a8f:T0", 0, 0), ("L6:map:015ac5:T0", 0, 0)], 53593, 41),
    ("bevelle", 16, None, ("L6:map:01f4b2:T0", 1, 69),
     [("L6:map:01f4b2:T0", 0, 61), ("L6:map:01f4b2:T0", 1, 69),
      ("L6:map:01f49e:T0", 0, 0)], None, 0),
    ("zanarkand", 49, 35520, ("L6:map:00f6b0:T5", 3, 0),
     [("L6:map:00f6b8:T5", 0, 0)], None, 0),
]

def literal(value):
    # ASCII escaped source avoids compiler locale-dependent English names.
    return json.dumps(value, ensure_ascii=True)

def binding(flow, key, value, element=0):
    candidates = [b for b in flow["workers"][0]["bindings"] if b["key"] == key]
    if len(candidates) != 1:
        raise ValueError(f"Missing/ambiguous worker-zero declaration: {key}")
    b = candidates[0]
    low, high = b["raw_low"], b["raw_high"]
    if element >= (high & 0xffff):
        raise ValueError("Contract exceeds the declared array")
    return dict(low=low, high=high, value=value, element=element, key=key)

def cpp_binding(b):
    return "{0x%x, 0x%x, %d, %d}" % (b["low"], b["high"], b["value"], b["element"])

def action(flow, worker, entry, pc=None, reg_y=0):
    original = flow["workers"][worker]["entries"][entry]
    return dict(worker=worker, entry=entry, entryPc=original,
                pc=original if pc is None else pc, regY=reg_y)

def cpp_action(a):
    fields=[a[k] for k in ("worker", "entry", "entryPc", "pc", "regY")]
    if 'choice' in a or a.get('activateWorker') or a.get('returnPc'): fields.append(a.get('choice',-1))
    if a.get('activateWorker') or a.get('returnPc'): fields.append('true' if a.get('activateWorker') else 'false')
    if a.get('returnPc'): fields.append(a['returnPc'])
    return '{'+', '.join(map(str,fields))+'}'

def contracts(maps):
    manifest = json.loads((DESIGNS / "manifest.json").read_text(encoding="utf8"))
    if hashlib.sha256((EVIDENCE / "maps_v3.json").read_bytes()).hexdigest() != manifest["source_sha256"]:
        raise ValueError("Map evidence changed")
    output, rows = [], []
    for name, worker, pc, ready, seeds, collect_pc, reg_y in RECIPES:
        flow = json.loads((DESIGNS / (name + "_flow.json")).read_text(encoding="utf8"))
        design = json.loads((DESIGNS / (name + "_design.json")).read_text(encoding="utf8"))
        script = design["script"]
        event = next(e for e in maps["events"] if e["script"] == script)
        source = EVIDENCE / "files" / event["source"]
        if hashlib.sha256(source.read_bytes()).hexdigest() != event["sha256"] or event["sha256"] != flow["sha256"]:
            raise ValueError("Trial evidence changed")
        bytecode = bytes.fromhex(event["code"])
        decoded = instructions(bytecode)
        reveal_entry = 7 if name == "bevelle" else 2
        reveal = action(flow, worker, reveal_entry, pc)
        if name=='kilika':
            # The complete original insertion owns its setup/cleanup. Its
            # empty-recess/carried6 branch asks Common013B at52593; selection0
            # continues at52613 through attachment/GFX68 before revealing.
            # Complete that owned choice using the original exec/return ABI.
            reveal['choice']=0
            # W16 entry1 deactivates W35 at387E. The normal door solution
            # uses Common00A8(35); native8577F0 only restores worker flag2.
            # Requesting this disabled worker otherwise queues forever.
            reveal['activateWorker']=True
        chest_entry = 8 if name == "bevelle" else 2
        collect = action(flow, design["treasure"]["worker"], chest_entry, collect_pc, reg_y)
        # Djose/Macalania start after the unselected graphic-case RET. Keep
        # the unique final RET following this verified collection continuation.
        returns = [s["pc"] for b in reachable(flow["workers"][collect["worker"]], chest_entry)
                   for s in b["statements"] if s["kind"] == "end" and s["opcode"] == "RET"
                   and s["pc"] >= collect["pc"]]
        if len(returns) != 1 or decoded[returns[0]][0] != 0x3c:
            raise ValueError("Collection must have one verified final RET")
        collect['returnPc'] = returns[0]
        if reveal["pc"] not in decoded or collect["pc"] not in decoded:
            raise ValueError("Continuation is not an instruction boundary")
        # A continuation starts before its own literal pushes, never midway
        # through the operand stack. Zanarkand skips only completed dialogue.
        if name == "zanarkand" and decoded[pc][0] != 0xae:
            raise ValueError("Unexpected Zanarkand commit prefix")
        ready_binding = binding(flow, *ready)
        preparation = [binding(flow, *s) for s in seeds]
        if any(b["key"] == "L0:map:000a70:T0" for b in preparation):
            raise ValueError("Preparation must not mark treasure collected")
        room = {"besaid":122,"kilika":108,"djose":214,"macalania":239,"bevelle":306,"zanarkand":320}[name]
        witness = design["completion_witnesses"][0]
        record = dict(name=design["name"], script=script, room=room,
            length=len(bytecode), checksum=manifest["scripts"][script]["code_fnv32"],
            exitRoom=design["completion"]["room"], exitSpawn=int(witness["args"][1]),
            mask=design["treasure"]["mask"], treasure=reveal, collect=collect,
            ready=ready_binding, preparation=preparation,
            firstStory=0, endStory=164 if name=="besaid" else 0,
            source=event["source"], sha256=event["sha256"],
            treasure_witnesses=design["treasure_witnesses"],
            completion_witnesses=design["completion_witnesses"])
        output.append(record)
        rows.append("    {L%s, %s, %d, 0x%x, %d, %d, %d, %d, %s, %s, %s, {%s}, %d, %d}," % (
            literal(record["name"]), literal(script), record["length"], record["checksum"],
            room, record["exitRoom"], record["exitSpawn"], record["mask"],
            cpp_action(reveal), cpp_action(collect), cpp_binding(ready_binding),
            ", ".join(cpp_binding(b) for b in preparation), record["firstStory"], record["endStory"]))
    target = ROOT / "src/native/utility_trials_data.hpp"
    target.write_text("// Generated by puzzles/build_utilities.py from original ATEL scripts.\n"
        "#pragma once\nnamespace ffx {\ninline const TrialUtility trialUtilities[] = {\n" +
        "\n".join(rows) + "\n};\n}\n", encoding="utf8")
    return output

def warps(maps):
    by_script = {e["script"]:e for e in maps["events"]}
    text = TextEvidence(EVIDENCE / "text")
    rows, records = [], []
    for room, script in sorted(room_index().items()):
        event = by_script.get(script)
        if not event or not event["entrances"] or script.startswith("test"):
            continue
        names = list(dict.fromkeys(text.macro(11, a) for a in event["area_indices"]))
        name = re.sub(r"<[^>]+>", "", " / ".join(n for n in names if n)).strip()
        name = name or script
        records.append(dict(room=room, script=script, name=name, entrances=len(event["entrances"])))
        rows.append("    {%d, %d, %s, L%s}," % (room, len(event["entrances"]), literal(script), literal(name)))
    target = ROOT / "src/native/utility_warps_data.hpp"
    includes = []
    for part, start in enumerate(range(0, len(rows), 350)):
        name = f"utility_warps_{part}.inc"
        (target.parent / name).write_text("\n".join(rows[start:start+350])+"\n", encoding="utf8")
        includes.append(f'#include "{name}"')
    target.write_text("// Generated from extracted entrance tables; no invented spawn range.\n"
        "#pragma once\nnamespace ffx {\ninline const WarpLocation warpLocations[] = {\n" +
        "\n".join(includes) + "\n};\n}\n", encoding="utf8")
    return records

def main():
    maps = json.loads((EVIDENCE / "maps_v3.json").read_text(encoding="utf8"))
    report = dict(utc=datetime.now(timezone.utc).isoformat(),
        source="maps_v3.json", trials=contracts(maps), warps=warps(maps),
        policy="Explicit native utilities; hash/entry guarded, no background automation. Gameplay validation pending.")
    stamp = datetime.now(timezone.utc).strftime("%Y%m%d_%H%M%S")
    target = EVIDENCE / ("native_utilities_" + stamp + ".json")
    target.write_text(json.dumps(report, ensure_ascii=False, indent=2)+"\n", encoding="utf8")
    print(f"Compiled {len(report['trials'])} trial utilities and {len(report['warps'])} warp rooms. Evidence: {target.name}")

if __name__ == "__main__":
    main()
