"""Cross-check authored seal names against original treasure writes and guards.

Only original mod analysis code belongs here. Original compiled ATEL models and
their source implementations are separately retained outside Git.
"""
from pathlib import Path
import hashlib
import json
import re
from evidence_paths import outside_git, REPOSITORY

BASE = outside_git(REPOSITORY.parent / "Analysis/FFX/Summon_20261020")
MODELS = BASE / "label_recheck_20261020"
BAAJ = BASE / "summon_statues_20261020_110729_078230/bjyt1000.json"
TRIALS = [("Besaid", "bsyt0100", 16), ("Kilika", "klyt1000", 7),
          ("Djose", "djyt0600", 24), ("Macalania", "mcyt0500", 19),
          ("Bevelle", "bvyt1100", 16), ("Zanarkand", "dome0700", 32)]


def writes(block):
    return [statement for statement in block["statements"]
            if statement["kind"] == "write" and "000a70:" in statement.get("binding", "")]


def treasure_write(worker):
    candidates = [statement for block in worker["blocks"] for statement in writes(block)]
    if len(candidates) == 1:
        return candidates[0], None
    # Djose/Macalania share two historical graphic cases. Resolve their actual
    # constant Y selector before selecting the matching switch edge, instead of
    # assigning a temple from the order of OR-mask writes in the source listing.
    selectors = [statement for block in worker["blocks"] for statement in block["statements"]
                 if statement["kind"] == "register" and statement.get("register") == "Y"
                 and statement.get("value") in ("69", "41")]
    assert len(selectors) == 1
    selector = int(selectors[0]["value"])
    branches = [block for block in worker["blocks"] if any(
        statement["kind"] == "branch" and statement.get("condition") == f"({selector} == ?Y)"
        for statement in block["statements"])]
    assert len(branches) == 1
    edge = next(edge for edge in branches[0]["edges"] if edge["when"] == f"({selector} == ?Y)")
    block = next(block for block in worker["blocks"] if block["start"] == edge["target"])
    selected = writes(block)
    assert len(selected) == 1
    return selected[0], {"selector": selector, "selector_pc": selectors[0]["pc"],
                         "switch_pc": branches[0]["start"], "selected_block": block["start"]}


def main():
    mapping = {}
    proof = []
    for temple, script, local in TRIALS:
        path = MODELS / (script + ".json")
        worker = json.loads(path.read_text())["workers"][local]
        selected, switch = treasure_write(worker)
        match = re.fullmatch(r"\(L0:map:000a70:T0\[0\] \| (\d+)\)", selected["value"])
        assert match
        mask = int(match[1]); assert mask not in mapping
        mapping[mask] = temple
        proof.append({"temple": temple, "script": script, "local_worker": local,
                      "saved_byte_offset": "0xA70", "mask": mask,
                      "write_pc": selected["pc"], "switch": switch,
                      "external_model_sha256": hashlib.sha256(path.read_bytes()).hexdigest()})
    model = json.loads(BAAJ.read_text())
    authored = (REPOSITORY / "src/native/navigation_passages_data.hpp").read_text()
    rows = []
    for local in range(30, 36):
        worker = model["workers"][local]
        start, end = worker["entries"][2:4]
        branches = [statement for block in worker["blocks"] for statement in block["statements"]
                    if start <= statement["pc"] < end and statement["kind"] == "branch"
                    and "000a70:" in statement.get("condition", "")]
        assert len(branches) == 1
        mask = int(re.search(r"& (\d+)", branches[0]["condition"])[1])
        expected = mapping[mask] + " seal"
        match = re.search(r'\{"bjyt1000",7530,0xd13cfb8c,' + str(local) +
                          r',\{[^\n]*?L"([^"]+)"', authored)
        assert match and match[1] == expected, (local, expected, match[1] if match else None)
        rows.append({"worker": local, "label": expected, "prerequisite_mask": mask,
                     "native_guard_pc": branches[0]["pc"]})
    output = BASE / "label_recheck_20261020/label-proof.json"
    output.write_text(json.dumps({"temple_writes": proof, "seal_rows": rows,
        "limit": "Static original callback/constant-switch proof; not fresh gameplay confirmation."}, indent=2) + "\n")
    print("PASS: all six authored seal labels match original temple treasure writes and Baaj guards.")
    for row in rows:
        print(f"Worker{row['worker']}: {row['label']}, mask{row['prerequisite_mask']}")


if __name__ == "__main__":
    main()
