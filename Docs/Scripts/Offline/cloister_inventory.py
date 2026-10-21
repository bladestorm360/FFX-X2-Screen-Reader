"""Inventory the six Cloisters using already extracted workspace data.

Not a puzzle solver. Preserve variable records and native text as evidence;
variable roles and action preconditions require native interpreter verification.
ATEL layouts are attributed to Karifean/FFXDataParser. No game access.
"""
import argparse
from collections import Counter
from datetime import datetime, timezone
import hashlib
import json
from pathlib import Path
import struct
from field_text import TextEvidence, u16, u32

TRIALS = {
    "Besaid": ["bsyt0100"], "Kilika": ["klyt1000"],
    "Djose": ["djyt0600"], "Macalania": ["mcyt0500"],
    "Bevelle": ["bvyt1100"], "Zanarkand": ["dome0700", "dome0200"],
}


def script_chunk(data):
    offsets = [u32(data, 4 + i * 4) for i in range(11)]
    start = offsets[0]
    end = next((n for n in offsets[1:] if start <= n <= len(data) and n != 0), len(data))
    return data[start:end]


def declarations(chunk, header):
    count, offset = u16(chunk, header + 2), u32(chunk, header + 0x14)
    if offset + count * 8 > len(chunk):
        raise ValueError("Variable table exceeds script chunk")
    records = []
    for i in range(count):
        low, high = struct.unpack_from("<II", chunk, offset + i * 8)
        props = low >> 24
        records.append({"index": i, "raw_low": low, "raw_high": high,
                        "offset": low & 0xFFFFFF, "properties": props,
                        "reference_format": props >> 4,
                        "reference_location": (props >> 1) & 7,
                        "flag_bit": props & 1,
                        "element_count": high & 0xFFFF, "element_size": high >> 16})
    return records


def bytecode_counts(code):
    counts, pc = Counter(), 0
    while pc < len(code):
        op = code[pc]
        size = 3 if op & 0x80 else 1
        if pc + size > len(code):
            raise ValueError("Truncated instruction")
        counts[op] += 1
        pc += size
    return {"variable_reads": counts[0x9F] + counts[0xA2],
            "variable_writes": sum(counts[op] for op in (0xA0, 0xA1, 0xA3, 0xA4)),
            "conditional_branches": sum(counts[op] for op in (0xB1, 0xB2, 0xD6, 0xD7))}


def inventory(directory):
    directory = Path(directory)
    input_file = directory / "maps_v3.json"
    maps = json.loads(input_file.read_text(encoding="utf-8"))
    text = TextEvidence(directory / "text")
    trials = []
    for title, scripts in TRIALS.items():
        records = []
        for event in maps["events"]:
            if event["script"] not in scripts:
                continue
            path = directory / "files" / event["source"]
            data = path.read_bytes()
            if hashlib.sha256(data).hexdigest() != event["sha256"]:
                raise ValueError(f"Evidence hash changed: {path}")
            chunk = script_chunk(data)
            workers = []
            for index, worker in enumerate(event["workers"]):
                header = u32(chunk, 0x38 + index * 4)
                workers.append({"index": index, **worker,
                                "declarations": declarations(chunk, header),
                                "private_data_offset": u32(chunk, header + 0x2C),
                                "shared_data_offset": u32(chunk, header + 0x30)})
            strings = text.field_strings(path)
            records.append({**event, "workers": workers, "english_strings": strings,
                            "area_names": list(dict.fromkeys(text.macro(11, a) for a in event["area_indices"])),
                            "bytecode_census": bytecode_counts(bytes.fromhex(event["code"]))})
        if set(scripts) != {r["script"] for r in records}:
            raise ValueError(f"Missing trial script: {title}")
        trials.append({"trial": title, "scripts": records})
        primary = next(r for r in records if r["script"] == scripts[0])
        print(title + ": " + ", ".join(scripts) + "; " + str(len(primary["workers"])) +
              " primary workers; " + str(len(primary["english_strings"])) + " native English strings")
    report = {"utc": datetime.now(timezone.utc).isoformat(),
              "source": str(input_file), "source_sha256": hashlib.sha256(input_file.read_bytes()).hexdigest(),
              "reference": "Karifean/FFXDataParser ScriptWorker/ScriptVariable",
              "status": "Inventory only; no validated puzzle solution or runtime state reader",
              "trials": trials}
    stamp = datetime.now(timezone.utc).strftime("%Y%m%d_%H%M%S")
    output = directory / ("cloisters_" + stamp + ".json")
    with output.open("x", encoding="utf-8") as stream:
        json.dump(report, stream, ensure_ascii=False)
    print("Evidence:", output)


if __name__ == "__main__":
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("directory", type=Path)
    inventory(parser.parse_args().directory)
