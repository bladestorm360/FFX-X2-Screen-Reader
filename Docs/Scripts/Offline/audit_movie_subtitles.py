"""Audit movie caption paths without starting FFX, Frida, or Ghidra.

Reads the retained native archive and explicitly supplied game files. Writes
timestamped local evidence and signatures for the user-run file-only probe.
Does not alter production profiles, readers, builds, or the game installation.
"""

# Game-source inputs/exports live outside every Git tree.
from pathlib import Path as _EvidencePath
import sys as _evidence_sys
_evidence_repo = next(p for p in _EvidencePath(__file__).resolve().parents
                      if (p / "Docs/Scripts/Offline/evidence_paths.py").is_file())
_evidence_sys.path.insert(0, str(_evidence_repo / "Docs/Scripts/Offline"))
from evidence_paths import evidence_path, evidence_relative, write_replay_adapter

import argparse
from datetime import datetime, timezone
import hashlib
import json
from pathlib import Path
import re

import pefile
from read_archive import functions
from reader_profile import pattern, signature
from vbf import Vbf

ROOT = Path(__file__).resolve().parents[3]
REFERENCE = evidence_path(ROOT / "Docs/Scripts/Offline/output/reader_reference_20261003_235502")
INSTALLED_HASH = "0537b2a1047f3266e73495cd4e35f63f0777f4231d417699f979954686da686d"
TARGETS = {
    "movie_load": 0x2d9db0, "movie_start": 0x2d90f0,
    "movie_external_start": 0x2daae0, "movie_stop": 0x2d72e0,
    "movie_update": 0x2d77b0, "ending_tick": 0x2dad80,
    "ending_draw": 0x505d30, "npc_install": 0x4b8980,
    "event_decode": 0x376a50,
}
LEADS = [0x006da680, 0x006d9a10, 0x006d94d0, 0x006d9360,
         0x00857710, 0x008589f0, 0x00860d00]


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--executable", required=True, type=Path)
    parser.add_argument("--vbf", required=True, type=Path)
    args = parser.parse_args()
    installed = args.executable.read_bytes()
    digest = hashlib.sha256(installed).hexdigest()
    if digest != INSTALLED_HASH:
        parser.error("Executable is not the independently supported installed profile")
    pe = pefile.PE(data=installed)
    if pe.FILE_HEADER.Machine != 0x14c or pe.OPTIONAL_HEADER.ImageBase != 0x400000:
        parser.error("Expected the supported x86 PE")
    image = pe.get_memory_mapped_image()
    old = (REFERENCE / "block_00401000.bin").read_bytes()
    names = {"FUN_%08x" % (rva + 0x400000) for rva in TARGETS.values()}
    names.update("FUN_%08x" % va for va in LEADS)
    bodies = {name: dict(file=path.name, line=line, body=body)
              for name, path, line, body in functions(
                  evidence_path(ROOT / "Docs/Scripts/Ghidra/output/FFX/decompile"), names)}
    if names - bodies.keys():
        raise ValueError("Missing archived bodies: " + repr(names - bodies.keys()))
    specs = []
    for label, rva in TARGETS.items():
        body = bodies["FUN_%08x" % (rva + 0x400000)]["body"]
        size = int(re.search(r"// Size:\s+(\d+) bytes", body)[1])
        tokens, _ = signature(old[rva - 0x1000:rva - 0x1000 + size],
                              rva + 0x400000, min(size, 128))
        def matches(data, start, end):
            return [start + m.start() for m in re.finditer(
                pattern(tokens), data[start:end], re.DOTALL)]
        # Mask only instruction address operands. Layout/control bytes remain
        # fixed; each independent bounded search must yield exactly one entry.
        ref_hits = matches(old, rva - 0x1000 - 4096, rva - 0x1000 + 4096)
        hits = matches(image, rva - 4096, rva + 4096)
        if ref_hits != [rva - 0x1000] or len(hits) != 1:
            raise ValueError("Ambiguous signature for " + label)
        specs.append(dict(label=label, reference=rva, installed=hits[0], tokens=tokens))
    stamp = datetime.now(timezone.utc).strftime("%Y%m%d_%H%M%S_%f")
    output = evidence_path(ROOT / "Docs/Scripts/Offline/output") / ("movie_subtitles_" + stamp)
    output.mkdir(parents=True)
    vbf = Vbf(args.vbf)
    asset_names = {
        "videolist.txt": "ffx_data/gamedata/ps3data/video/us/ffx_videolist.txt",
        "timestamp_en.txt": "metamenu/gamedata/ps3data/video/us/timestamp_en.txt",
        "credits.bin": "ffx_ps2/ffx/master/new_uspc/event/obj_psv/cr/credits/credits.bin",
    }
    assets = []
    for filename, name in asset_names.items():
        entry = next(e for e in vbf.entries if e.name.lower() == name)
        data = vbf.read(entry)
        (output / filename).write_bytes(data)
        assets.append(dict(name=name, size=len(data), sha256=hashlib.sha256(data).hexdigest()))
    reference_hash = re.search(r"sha256\t(\w+)", (REFERENCE / "program.tsv").read_text())[1]
    specs_doc = dict(reference_sha256=reference_hash, installed_sha256=digest,
                     signatures=specs)
    evidence = dict(utc=stamp, executable_sha256=digest, reference=str(REFERENCE),
                    signatures=specs, bodies=bodies, assets=assets,
                    live_probe_run=False, game_launched=False,
                    finding="Ordinary movie scripts can use TK dialogue. The separate credits timeline is the post-game audio drama; its renderer bypasses both message readers. Its routines have no direct-call or data-pointer references in the retained FFX image; Will requires an independent caption-only profile.")
    (output / "audit.json").write_text(json.dumps(evidence, indent=2) + "\n", encoding="utf-8")
    (ROOT / "Docs/Scripts/Frida/FFX/movie_subtitles_signatures.json").write_text(
        json.dumps(specs_doc, indent=2) + "\n", encoding="utf-8")
    (evidence_path(ROOT / "Docs/Scripts/Frida/output/FFX")).mkdir(parents=True, exist_ok=True)
    print("Verified %d independent hook signatures; retained %d source bodies and %d assets." %
          (len(specs), len(bodies), len(assets)))
    print(output)


if __name__ == "__main__":
    main()
