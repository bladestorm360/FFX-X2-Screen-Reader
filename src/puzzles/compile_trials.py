"""Build all six trial designs from existing, hashed workspace evidence.

This tool neither opens a game archive nor attaches to a process. Its output
is a static design/validation asset, not a production narration table.
"""
import argparse
from datetime import datetime, timezone
import hashlib
import json
from pathlib import Path
import sys

from atel_model import ROOT, compile_script, render
from trial_designs import TRIALS, UNRESOLVED

sys.path.insert(0, str(ROOT / "Docs/Scripts/Offline"))
from cloister_inventory import declarations, script_chunk
from extract_maps import room_index
from field_text import TextEvidence, u32


def fnv32(code):
    result = 2166136261
    for b in code:
        result = ((result ^ b) * 16777619) & 0xFFFFFFFF
    return result


def enrich(event, directory, text):
    event = dict(event)
    source = directory / "files" / event["source"]
    data = source.read_bytes()
    if hashlib.sha256(data).hexdigest() != event["sha256"]:
        raise ValueError(f"Evidence hash changed: {event['source']}")
    chunk = script_chunk(data)
    original_workers = event["workers"]
    event["workers"] = []
    for index, worker in enumerate(original_workers):
        header = u32(chunk, 0x38 + index * 4)
        event["workers"].append(dict(worker, index=index,
            declarations=declarations(chunk, header), data_offset=u32(chunk, header + 0x28),
            private_data_offset=u32(chunk, header + 0x2C),
            shared_data_offset=u32(chunk, header + 0x30), event_data_offset=u32(chunk, 0x20)))
    event["english_strings"] = text.field_strings(source)
    event["area_names"] = list(dict.fromkeys(text.macro(11, a) for a in event["area_indices"]))
    return event


def reachable(worker, entry):
    blocks = {b["start"]: b for b in worker["blocks"]}
    pending, seen = [worker["entries"][entry]], set()
    while pending:
        pc = pending.pop()
        if pc in seen:
            continue
        if pc not in blocks:
            raise ValueError("Missing compiled basic block")
        seen.add(pc)
        pending.extend(e["target"] for e in blocks[pc]["edges"])
    return [blocks[pc] for pc in sorted(seen)]


def statements(model):
    for worker in model["workers"]:
        for block in worker["blocks"]:
            for statement in block["statements"]:
                yield worker["index"], block["start"], statement


def entry_graph(model):
    nodes, links, unresolved = [], [], []
    for w in model["workers"]:
        for entry in range(len(w["entries"])):
            node = f"{w['index']}:{entry}"
            blocks = reachable(w, entry)
            nodes.append(dict(id=node, worker=w["index"], entry=entry,
                              pc=w["entries"][entry], blocks=[b["start"] for b in blocks]))
            for block in blocks:
                for s in block["statements"]:
                    if s["kind"] == "subroutine":
                        target, target_entry = s["worker"], 0
                        kind = "subroutine"
                    elif s["kind"] == "request" and s["worker"].isdigit() and s["entry"].isdigit():
                        target, target_entry = int(s["worker"]), int(s["entry"])
                        kind = s["opcode"]
                    elif s["kind"] == "request":
                        unresolved.append(dict(source=node, pc=s["pc"], kind="computed request", expression=s))
                        continue
                    else:
                        continue
                    if target >= len(model["workers"]) or target_entry >= len(model["workers"][target]["entries"]):
                        unresolved.append(dict(source=node, pc=s["pc"], kind="external request", target=target, entry=target_entry))
                        continue
                    links.append(dict(source=node, target=f"{target}:{target_entry}", pc=s["pc"], kind=kind))
    return dict(nodes=nodes, links=links, unresolved=unresolved,
                semantics="Requests are dependencies, not serialized or guaranteed effects.")


def catalog(model, design):
    all_statements = list(statements(model))
    bindings = {}
    for w in model["workers"]:
        for d in w["bindings"]:
            entry = bindings.setdefault(d["key"], dict(key=d["key"], declaration=d, aliases=[]))
            entry["aliases"].append(dict(worker=w["index"], variable=d["index"]))
    fields = []
    for role, key in dict(design["fields"], temple_treasure="L0:map:000a70:T0").items():
        if key not in bindings:
            raise ValueError(f"{design['name']}: missing binding {key}")
        binding = bindings[key]
        writes = [dict(worker=w, block=b, **s) for w, b, s in all_statements
                  if s["kind"] == "write" and s["binding"] == key]
        fields.append(dict(role=role, **binding, writes=writes))
    completion = [dict(worker=w, block=b, **s) for w, b, s in all_statements
                  if w == design["completion"]["worker"] and s["kind"] == "call"
                  and s["id"] in (0x11, 0x10B) and s["args"][0] == str(design["completion"]["room"])]
    treasure = [dict(worker=w, block=b, **s) for w, b, s in all_statements
                if w == design["treasure"]["worker"] and s["kind"] == "call"
                and s["id"] == 0x15B and s["args"][1] == str(design["treasure"]["id"])]
    bit = design["treasure"]["mask"]
    flag_writes = [dict(worker=w, block=b, **s) for w, b, s in all_statements
                   if w == design["treasure"]["worker"] and s["kind"] == "write"
                   and s["binding"] == "L0:map:000a70:T0" and s["value"].endswith(f"| {bit})")]
    if not completion or not treasure or not flag_writes:
        raise ValueError(f"{design['name']}: missing completion/treasure witness")
    geometry = [dict(worker=w, block=b, **s) for w, b, s in all_statements
                if s["kind"] == "call" and s["id"] in (0x13, 0x54, 0x57, 0x58, 0x59, 0x126)]
    actions = []
    wanted = set(design["socket_workers"])
    for route in (design["normal_route"], design["treasure_route"]):
        wanted.update(w for step in route for w in step["workers"])
    for w in model["workers"]:
        if w["index"] not in wanted:
            continue
        for entry in range(len(w["entries"])):
            blocks = reachable(w, entry)
            effects = [dict(block=b["start"], **s) for b in blocks for s in b["statements"]
                       if s["kind"] in ("write", "request", "subroutine", "call", "opaque")]
            if effects:
                actions.append(dict(worker=w["index"], entry=entry, pc=w["entries"][entry],
                                    blocks=[b["start"] for b in blocks], effects=effects))
    return {**design, "fields": fields, "completion_witnesses": completion,
            "treasure_witnesses": treasure, "treasure_flag_witnesses": flag_writes,
            "geometry": geometry, "actions": actions, "unresolved": UNRESOLVED}


def build(directory, output=None):
    directory = Path(directory).resolve()
    maps_path = directory / "maps_v3.json"
    maps = json.loads(maps_path.read_text(encoding="utf-8"))
    events = {s["script"]: s for s in maps["events"]}
    text = TextEvidence(directory / "text")
    stamp = datetime.now(timezone.utc).strftime("%Y%m%d_%H%M%S_%f")
    output = Path(output).resolve() if output else directory / ("cloister_designs_" + stamp)
    output.mkdir(parents=True, exist_ok=False)
    trials, scripts = [], {}
    for design in TRIALS:
        event = enrich(events[design["script"]], directory, text)
        model = compile_script(event)
        graph = entry_graph(model)
        compiled = catalog(model, design)
        compiled["entry_graph"] = graph
        compiled["source"] = dict(path=event["source"], sha256=event["sha256"], script=event["script"])
        name = design["name"].lower()
        (output / (name + "_design.json")).write_text(json.dumps(compiled, ensure_ascii=False), encoding="utf-8")
        (output / (name + "_flow.json")).write_text(json.dumps(model, ensure_ascii=False), encoding="utf-8")
        (output / (name + "_flow.txt")).write_text(render(model), encoding="utf-8")
        scripts[event["script"]] = runtime_script(event, compiled)
        trials.append(dict(name=design["name"], script=event["script"], design_file=name + "_design.json",
                           flow_file=name + "_flow.json", actions=len(compiled["actions"]),
                           dependencies=len(graph["links"]), unresolved_requests=len(graph["unresolved"])))
        print(f"{design['name']}: {len(compiled['actions'])} entry models; completion and treasure witnesses retained.")
    # Include the linked Dome room and keep the other Dome maps as dependency evidence.
    related = []
    rooms = room_index()
    for event in events.values():
        if not event["script"].startswith("dome") or event["script"] == "dome0700":
            continue
        event = enrich(event, directory, text)
        related.append(dict(script=event["script"], source=event["source"], sha256=event["sha256"],
                            rooms=[r for r, s in rooms.items() if s == event["script"]], areas=event["area_names"]))
        if event["script"] == "dome0200":
            model = compile_script(event)
            (output / "zanarkand_linked_flow.json").write_text(json.dumps(model, ensure_ascii=False), encoding="utf-8")
            (output / "zanarkand_linked_flow.txt").write_text(render(model), encoding="utf-8")
            scripts[event["script"]] = runtime_script(event, dict(fields=[]))
    manifest = dict(schema=1, utc=datetime.now(timezone.utc).isoformat(), source=str(maps_path),
                    source_sha256=hashlib.sha256(maps_path.read_bytes()).hexdigest(),
                    status="All six static designs and event-validation assets; not live-validated guidance",
                    references=["Karifean/FFXDataParser", "Fahrenheit", "native decompile archive"],
                    trials=trials, scripts=scripts, linked_dome_evidence=related, unresolved=UNRESOLVED)
    (output / "manifest.json").write_text(json.dumps(manifest, ensure_ascii=False), encoding="utf-8")
    print("Design archive:", output)
    return output


def runtime_script(event, design):
    fields = []
    for f in design["fields"]:
        alias = f["aliases"][0]
        fields.append(dict(role=f["role"], key=f["key"], worker=alias["worker"],
                           variable=alias["variable"], low=f["declaration"]["raw_low"],
                           high=f["declaration"]["raw_high"]))
    return dict(code_length=event["code_length"], code_fnv32=fnv32(bytes.fromhex(event["code"])),
                sha256=event["sha256"], event_data_offset=event["workers"][0]["event_data_offset"],
                fields=fields, worker_types=[w["type"] for w in event["workers"]])


if __name__ == "__main__":
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("directory", type=Path, help="Existing extraction directory containing maps_v3.json")
    parser.add_argument("--output", type=Path, help="New output directory; existing evidence is never overwritten")
    args = parser.parse_args()
    build(args.directory, args.output)
