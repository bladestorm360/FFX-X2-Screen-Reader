"""Generate room->area INDEX data; localized names still come from the game."""
import argparse
import hashlib
import json
from pathlib import Path


def generate(input_path, output_path):
    data = json.loads(Path(input_path).read_text(encoding="utf-8"))
    lines = [
        "// Generated from FFX event ATEL headers by generate_room_areas.py.",
        "// Room/event index: Karifean/FFXDataParser EventFile.java.",
        "// Names are NOT baked: resolve the area index through the game's getter.",
        "// Multiple-area rooms are omitted until their area-selection rule is verified.",
        f"// Evidence SHA256: {hashlib.sha256(Path(input_path).read_bytes()).hexdigest()}",
        "export const ROOM_AREA_INDEX = {",
    ]
    for room, areas in sorted(data["room_areas"].items(), key=lambda item: int(item[0])):
        if len(set(areas)) == 1 and areas[0] > 0:
            lines.append(f"    {room}: {areas[0]},")
    lines.append("};")
    Path(output_path).write_bytes(("\r\n".join(lines) + "\r\n").encode("utf-8"))
    print(f"Generated {len(lines) - 7} room mappings: {output_path}")


if __name__ == "__main__":
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("input", type=Path)
    parser.add_argument("output", type=Path)
    args = parser.parse_args()
    generate(args.input, args.output)
