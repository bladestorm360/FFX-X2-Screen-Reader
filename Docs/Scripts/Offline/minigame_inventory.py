"""Read extracted minigame scripts and retain reviewable ATEL control flow.

No game installation, processes, Frida or Ghidra execution. Layout attribution:
Karifean/FFXDataParser; reuse the existing validated offline script decoder.
"""
from datetime import datetime, timezone
from pathlib import Path
import hashlib
import json
import sys

ROOT = Path(__file__).resolve().parents[3]
sys.path.insert(0, str(ROOT / "src/puzzles"))
from atel_model import compile_script
from cloister_inventory import script_chunk, declarations
from field_text import TextEvidence, u32


def main():
    evidence = Path(__file__).parent / "output/FFX/20261003_audit"
    maps = json.loads((evidence / "maps_v3.json").read_text(encoding="utf8"))
    output = Path(__file__).parent / "output" / ("minigames_" + datetime.now(timezone.utc).strftime("%Y%m%d_%H%M%S"))
    output.mkdir()
    text = TextEvidence(evidence / "text")
    records = []
    for event in maps["events"]:
        if event["script"] not in ("bltz0000", "bltz0002", "nagi0000", "nagi0500"):
            continue
        source = evidence / "files" / event["source"]
        raw = source.read_bytes()
        if hashlib.sha256(raw).hexdigest() != event["sha256"]:
            raise ValueError("Archived script hash changed")
        chunk = script_chunk(raw)
        workers = []
        for i, worker in enumerate(event["workers"]):
            header = u32(chunk, 0x38 + i * 4)
            workers.append(dict(worker, index=i, declarations=declarations(chunk, header),
                                private_data_offset=u32(chunk, header + 0x2c),
                                shared_data_offset=u32(chunk, header + 0x30)))
        strings=text.field_strings(source)
        script = dict(event, workers=workers, event_data_offset=u32(chunk, 0x20),
                      english_strings=strings, area_names=[text.macro(11,a) for a in event['area_indices']])
        compiled = compile_script(script)
        filename = source.stem + ".json"
        (output / filename).write_text(json.dumps(dict(script=script, compiled=compiled,
            english_strings=text.field_strings(source)), ensure_ascii=False), encoding="utf8")
        records.append(dict(script=event["script"], source=event["source"], sha256=event["sha256"], file=filename))
    (output / "manifest.json").write_text(json.dumps(dict(utc=datetime.now(timezone.utc).isoformat(),
        records=records, status="Offline control flow; live timing and actor bindings unvalidated")), encoding="utf8")
    print(f"Retained {len(records)} minigame script variants: {output}")


if __name__ == "__main__":
    main()
