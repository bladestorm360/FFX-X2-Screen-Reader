"""Read Auron's native sequence table from retained offline memory blocks."""

# Game-source inputs/exports live outside every Git tree.
from pathlib import Path as _EvidencePath
import sys as _evidence_sys
_evidence_repo = next(p for p in _EvidencePath(__file__).resolve().parents
                      if (p / "Docs/Scripts/Offline/evidence_paths.py").is_file())
_evidence_sys.path.insert(0, str(_evidence_repo / "Docs/Scripts/Offline"))
from evidence_paths import evidence_path, evidence_relative, write_replay_adapter

from datetime import datetime, timezone
from pathlib import Path
import json
import struct

ROOT = Path(__file__).resolve().parents[3]
ARCHIVE = evidence_path(ROOT / "Docs/Scripts/Offline/output/reader_reference_20261003_235502")


def retained(address, size):
    for path in ARCHIVE.glob("block_*.bin"):
        start = int(path.stem.removeprefix("block_"), 16)
        if start <= address and address + size <= start + path.stat().st_size:
            with path.open("rb") as source:
                source.seek(address - start)
                return source.read(size), str(evidence_relative(path, ROOT))
    raise ValueError(f"No retained block contains {address:08x}+{size:x}")


def main():
    rows = []
    for index in range(4):
        sequence, block = retained(0xC86B70 + index * 32, 32)
        words = list(struct.unpack("<16h", sequence))
        if -1 not in words:
            raise ValueError("Native sequence has no terminator")
        timer, _ = retained(0xC86B60 + index * 4, 4)
        rows.append({"sequence_index": index, "button_masks": words[:words.index(-1)],
                     "timer_value": struct.unpack("<f", timer)[0], "source": block})
    stamp = datetime.now(timezone.utc).strftime("%Y%m%d_%H%M%S_%f")
    output = evidence_path(ROOT / f"Docs/Scripts/Offline/output/auron_sequences_{stamp}.json")
    output.write_text(json.dumps({"source_program": (ARCHIVE / "program.tsv").read_text(),
                                 "native_getter": "00a5ca60", "sequences": rows}, indent=2) + "\n")
    print(evidence_relative(output, ROOT))
    for row in rows:
        print(row["sequence_index"], row["button_masks"], row["timer_value"])


if __name__ == "__main__":
    main()
