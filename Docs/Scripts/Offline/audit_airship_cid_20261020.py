"""Inspect retained airship actor ownership; originals remain outside Git.

No installed assets, game process or live pointers are accessed. Original ATEL
parsing is attributed to the existing Karifean/FFXDataParser-based compiler.
"""
from pathlib import Path
from datetime import datetime, timezone
import hashlib
import json
import sys

from evidence_paths import REPOSITORY, evidence_path, outside_git

sys.path.insert(0, str(REPOSITORY / "src/puzzles"))
from compile_trials import enrich
from atel_model import compile_script
from field_text import TextEvidence


def main():
    base = evidence_path(REPOSITORY / "Docs/Scripts/Offline/output/FFX/20261003_audit")
    catalog = json.loads((base / "maps_v3.json").read_text())
    output = outside_git(REPOSITORY.parent / "Analysis/FFX/AirshipCid_20261020" /
                         datetime.now(timezone.utc).strftime("%H%M%S_%f"))
    output.mkdir(parents=True)
    text = TextEvidence(base / "text")
    report = []
    for event in catalog["events"]:
        if not event["script"].startswith("hiku") or "/psv_" in event["source"]:
            continue
        model = compile_script(enrich(event, base, text))
        (output / (event["script"] + ".json")).write_text(json.dumps(model, indent=2))
        actors = []
        for worker in model["workers"]:
            if worker["type"] != 1:
                continue
            statements = [s for b in worker["blocks"] for s in b["statements"]]
            calls = [s for s in statements if s["kind"] == "call"]
            models = [s["args"] for s in calls if s["id"] == 1]
            if not models:
                continue
            actor = {"worker": worker["index"], "model_calls": models,
                     "actor_links": [s["args"] for s in calls if s["id"] == 66],
                     "entry_count": len(worker["entries"]),
                     "talk_entry": worker["entries"][2] if len(worker["entries"]) > 2 else None}
            actors.append(actor)
        report.append({"script": event["script"], "code_length": event["code_length"],
                       "source": event["source"], "source_sha256": event["sha256"],
                       "actors": actors})
    (output / "actor-summary.json").write_text(json.dumps(report, indent=2) + "\n")
    print("External original models:", output)
    for item in report:
        print(item["script"], json.dumps(item["actors"], separators=(",", ":")))


if __name__ == "__main__":
    main()
