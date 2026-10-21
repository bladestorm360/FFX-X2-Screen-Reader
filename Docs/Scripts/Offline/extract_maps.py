"""Extract FFX event data and map metadata without launching the game.

EBP/ATEL layouts and the room->event index are from Karifean/FFXDataParser:
EventFile.java, AtelScriptObject.java, and ScriptWorker.java. Reference
names are recorded as evidence, never treated as player-authored names.
"""
import argparse
from datetime import datetime, timezone
import hashlib
import json
from pathlib import Path
import re
import struct
from vbf import Vbf, output_path

ROOT = Path(__file__).resolve().parents[3]
PARSER = ROOT.parent / "FFXDataParser/src/main/java/atel/EventFile.java"


def u16(data, offset):
    return struct.unpack_from("<H", data, offset)[0]


def u32(data, offset):
    return struct.unpack_from("<I", data, offset)[0]


def string_at(data, offset):
    if not 0 < offset < len(data):
        return None
    return data[offset:data.index(b"\0", offset)].decode("utf-8", errors="replace")


def array(data, offset, count, kind):
    size = struct.calcsize(kind)
    if not 0 <= offset <= offset + size * count <= len(data):
        raise ValueError("ATEL table exceeds chunk bounds")
    return list(struct.unpack_from(f"<{count}{kind}", data, offset))


def parse_event(data):
    if data[:4] != b"EV01":
        raise ValueError("Unsupported EBP magic")
    start = u32(data, 4)
    # Optional EBP chunks have zero offsets. The next NONZERO offset is
    # the boundary, not necessarily the immediately following dword.
    offsets = [u32(data, 4 + i * 4) for i in range(11)]
    end = next((n for n in offsets[1:] if start <= n <= len(data) and n != 0), len(data))
    if not 0x30 <= start < end <= len(data):
        raise ValueError("Invalid EBP script chunk")
    chunk = data[start:end]
    code_len, code_start = u32(chunk, 0), u32(chunk, 0x30)
    count = u16(chunk, 0x34)
    if not 0 < count <= 1024 or code_start + code_len > len(chunk):
        raise ValueError("Invalid ATEL bytecode bounds")
    area = u16(chunk, 0x1E)
    area_offset = u32(chunk, 0x28)
    areas = array(chunk, area_offset, area & 0x7FFF, "H") if area & 0x8000 else [area]
    workers = []
    for i in range(count):
        header = u32(chunk, 0x38 + i * 4)
        if header + 0x34 > len(chunk):
            raise ValueError("Invalid ATEL worker header")
        workers.append({
            "type": u16(chunk, header),
            "entries": array(chunk, u32(chunk, header + 0x20), u16(chunk, header + 8), "I"),
            "jumps": array(chunk, u32(chunk, header + 0x24), u16(chunk, header + 0xA), "I"),
            "ints": array(chunk, u32(chunk, header + 0x18), u16(chunk, header + 4), "i"),
            "floats": array(chunk, u32(chunk, header + 0x1C), u16(chunk, header + 6), "f"),
        })
    entrance_start, entrance_end = u32(chunk, 4), u32(chunk, 8)
    entrances = []
    if entrance_start and entrance_start < entrance_end:
        if (entrance_end - entrance_start) % 32:
            raise ValueError("Invalid entrance table length")
        for offset in range(entrance_start, entrance_end, 32):
            entrances.append(dict(zip(("rotation", "x", "y", "z"), struct.unpack_from("<4f", chunk, offset + 8))))
    return {"script": string_at(chunk, u32(chunk, 0xC)), "area_indices": areas,
            "code_length": code_len, "code": chunk[code_start:code_start + code_len].hex(),
            "workers": workers, "entrances": entrances}


def room_index():
    text = PARSER.read_text(encoding="utf-8")
    return {int(room): script for room, script in re.findall(r'EVENT_BY_ROOM_ID\.put\((\d+), "([^"]+)"\)', text)}


def extract(archive, output):
    vbf = Vbf(archive)
    output = Path(output).resolve()
    output.mkdir(parents=True, exist_ok=False)
    events, failures, manifest = [], [], []
    for entry in vbf.entries:
        if "/event/" not in entry.name.lower() or not entry.name.lower().endswith(".ebp"):
            continue
        try:
            data = vbf.read(entry)
            target = output_path(output / "files", entry.name)
            target.parent.mkdir(parents=True, exist_ok=True)
            target.write_bytes(data)
            digest = hashlib.sha256(data).hexdigest()
            manifest.append({"path": entry.name, "bytes": len(data), "sha256": digest})
            event = parse_event(data)
            event["source"] = entry.name
            event["sha256"] = digest
            events.append(event)
        except (ValueError, IndexError, struct.error) as error:
            failures.append({"path": entry.name, "error": str(error)})
    by_script = {}
    for event in events:
        by_script.setdefault(event["script"], []).append(event)
    room_areas = {}
    for room, script in room_index().items():
        variants = by_script.get(script, [])
        if variants:
            room_areas[room] = [area for event in variants for area in event["area_indices"]]
    report = {"archive": str(Path(archive).resolve()), "utc": datetime.now(timezone.utc).isoformat(),
              "reference": str(PARSER), "archive_entries": len(vbf.entries), "files": manifest,
              "failures": failures, "room_areas": room_areas, "events": events}
    (output / "maps.json").write_text(json.dumps(report, ensure_ascii=False), encoding="utf-8")
    print(f"Copied {len(manifest)} files; parsed {len(events)} scripts; mapped {len(room_areas)} room IDs; parse failures: {len(failures)}.")
    print(f"Evidence: {output / 'maps.json'}")
    return report


if __name__ == "__main__":
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("archive", type=Path)
    parser.add_argument("--output", type=Path, default=Path(__file__).parent / "output/FFX" / datetime.now().strftime("%Y%m%d_%H%M%S"))
    args = parser.parse_args()
    extract(args.archive, args.output)
