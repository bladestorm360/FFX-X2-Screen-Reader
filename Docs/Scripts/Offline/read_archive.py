"""Print selected complete functions from the existing decompile archive.

Offline only: does not open the game, run Ghidra, or change its project.
"""
from pathlib import Path
import argparse
import re


def functions(archive, names):
    for path in sorted(Path(archive).glob("*.txt")):
        text = path.read_text(encoding="utf-8", errors="replace")
        for match in re.finditer(
            r"// Function: (\S+)\n(.*?)(?=// ========================================\n// Function:|\Z)",
            text, re.S,
        ):
            if match[1] not in names:
                continue
            line = text.count("\n", 0, match.start()) + 1
            yield match[1], path, line, match[0]


if __name__ == "__main__":
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("names", nargs="+")
    args = parser.parse_args()
    archive = Path(__file__).resolve().parents[1] / "Ghidra/output/FFX/decompile"
    found = set()
    for name, path, line, body in functions(archive, set(args.names)):
        if name in args.names:
            print(f"{path.name}:{line}\n{body}")
            found.add(name)
    missing = set(args.names) - found
    if missing:
        parser.error("Missing functions: " + ", ".join(sorted(missing)))
