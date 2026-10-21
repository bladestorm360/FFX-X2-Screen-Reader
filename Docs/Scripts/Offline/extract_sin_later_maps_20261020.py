"""Approved read-only extraction of exactly City and Nucleus map/collision.

The user explicitly approved these two areas on October20. This adapter reads
only their two mapout entries, never launches the game, and keeps originals
outside every Git tree. The independent VBF reader retains its attribution.
"""
import argparse
from datetime import datetime, timezone
import hashlib
import json

from evidence_paths import outside_git
from vbf import Vbf, output_path

parser = argparse.ArgumentParser(description=__doc__)
parser.add_argument("--archive", required=True)
parser.add_argument("--output", required=True)
args = parser.parse_args()
output = outside_git(args.output)
assert not output.exists(), "Use a fresh output to preserve prior evidence"
archive = Vbf(args.archive)
names = {
    "City of Dying Dreams": "ffx_ps2/ffx/master/jppc/map/sins/sins04/bin/mapout.vpa",
    "The Nucleus": "ffx_ps2/ffx/master/jppc/map/sins/sins05/bin/mapout.vpa",
}
entries = {entry.name: entry for entry in archive.entries if entry.name in names.values()}
assert set(entries) == set(names.values()), "Both exact approved map entries must exist"
output.mkdir(parents=True)
assets = []
for area, name in names.items():
    entry = entries[name]
    assert 0 < entry.size <= 256 * 1024 * 1024
    data = archive.read(entry)
    path = output_path(output, name)
    path.parent.mkdir(parents=True, exist_ok=True)
    path.write_bytes(data)
    assets.append(dict(area=area, source=name, bytes=len(data),
                       sha256=hashlib.sha256(data).hexdigest(), file=path.relative_to(output).as_posix()))
stat = archive.path.stat()
manifest = dict(utc=datetime.now(timezone.utc).isoformat(), archive=str(archive.path),
                archive_size=stat.st_size, archive_mtime_ns=stat.st_mtime_ns,
                permission="Explicit user approval: read City of Dying Dreams and The Nucleus map/collision for offline route tests only; no launch/control/probe.",
                attribution="Original Square Enix assets; independent VBF reader format cross-check attribution is in vbf.py.",
                assets=assets)
(output / "asset-manifest.json").write_text(json.dumps(manifest, indent=2), encoding="utf-8")
print("PASS: extracted only the two approved mapout assets; original archive unchanged.")
print(output)
