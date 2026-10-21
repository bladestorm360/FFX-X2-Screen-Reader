"""Prepare retained real-floor inputs for the production later-Sin route host.

ChMap field/layout facts follow native83EA00 and the already verified Sea floor
adapter. This is independent mod tooling; originals and reports stay outside
Git. Script/EBP parsing retains Karifean/FFXDataParser attribution.
"""
import argparse
import collections
from datetime import datetime, timezone
import hashlib
import json
import struct

from cloister_inventory import script_chunk
from evidence_paths import REPOSITORY, evidence_path, outside_git

parser = argparse.ArgumentParser(description=__doc__)
parser.add_argument("--assets", required=True)
args = parser.parse_args()
assets = outside_git(args.assets)
manifest = json.loads((assets / "asset-manifest.json").read_text(encoding="utf-8"))
analysis = outside_git(REPOSITORY.parent / "Analysis/FFX/SinDynamic_20261020/171143_927729")
output = evidence_path(REPOSITORY / "Docs/Scripts/Offline/output/sin_later_maps_20261020")
output.mkdir(parents=True, exist_ok=True)


def retain(path, data):
    if path.exists():
        assert path.read_bytes() == data, f"Preserve prior input: {path}"
    else:
        path.write_bytes(data)


def fingerprint(data):
    value = 14695981039346656037
    for byte in data:
        value = ((value ^ byte) * 1099511628211) & 0xffffffffffffffff
    return f"{value:016x}"


rows = []
for stem, script_name, source_hash in [
    ("city", "sins0400", "890748eedb4fb7d00623687751816b1dcb41b00f7161e64930f54485d7e5a25a"),
    ("nucleus", "sins0500", "c81148c2c9d6d8c7394d97b3d2285c3b7a77a839543b74d91dfc03ac764cb229"),
]:
    source_name = f"ffx_ps2/ffx/master/jppc/map/sins/{script_name[:6]}/bin/mapout.vpa"
    item = next(a for a in manifest["assets"] if a["source"] == source_name)
    data = (assets / item["file"]).read_bytes()
    assert hashlib.sha256(data).hexdigest() == item["sha256"]
    u32 = lambda at: struct.unpack_from("<I", data, at)[0]
    s16 = lambda at: struct.unpack_from("<h", data, at)[0]
    root = u32(0x18)
    scale = struct.unpack_from("<f", data, root + 12)[0] / 10.0
    scale = struct.unpack("<f", struct.pack("<f", scale))[0]  # Native stored float.
    vertex_count = s16(root + 10)
    groups = root + u32(root + 28)
    triangle_count = s16(groups + 8)
    triangle_at = root + u32(groups + 12)
    vertex_at = root + u32(root + 24)
    assert 0 < vertex_count <= 32767 and 0 < triangle_count <= 32767 and scale > 0
    assert 0 <= triangle_at < triangle_at + triangle_count * 16 <= len(data)
    assert 0 <= vertex_at < vertex_at + vertex_count * 8 <= len(data)
    raw_triangles = data[triangle_at:triangle_at + triangle_count * 16]
    raw_vertices = data[vertex_at:vertex_at + vertex_count * 8]
    triangles = [struct.unpack_from("<6hI", raw_triangles, i * 16) for i in range(triangle_count)]
    assert all(all(0 <= v < vertex_count for v in t[:3]) and all(-1 <= n < triangle_count for n in t[3:6]) for t in triangles)
    retain(output / (stem + "_triangles.bin"), raw_triangles)
    retain(output / (stem + "_vertices.bin"), raw_vertices)
    model = json.loads((analysis / (script_name + ".json")).read_text(encoding="utf-8"))
    original = evidence_path(REPOSITORY / f"Docs/Scripts/Offline/output/FFX/20261003_audit/files/ffx_ps2/ffx/master/jppc/event/obj/si/{script_name}/{script_name}.ebp")
    ebp = original.read_bytes()
    assert hashlib.sha256(ebp).hexdigest() == model["sha256"] == source_hash
    chunk = script_chunk(ebp)
    start, end = struct.unpack_from("<II", chunk, 4)
    assert 0 < start <= end <= len(chunk) and (end - start) % 32 == 0
    entrances = [struct.unpack_from("<3f", chunk, at + 12) for at in range(start, end, 32)]
    exits, chests, controls = [], [], []
    for worker in model["workers"]:
        calls = [s for block in worker["blocks"] for s in block["statements"] if s.get("kind") == "call"]
        if worker["type"] == 2 and any(s.get("id") == 0x11 for s in calls):
            edge = next(s for s in calls if s.get("id") == 0x54)
            size = next(s for s in calls if s.get("id") == 0x7a)
            exits.append(dict(worker=worker["index"], edge=list(map(float, edge["args"])), size=list(map(float, size["args"]))))
        if any(s.get("id") == 1 and s.get("args") == ["20482"] for s in calls):
            position = next(s for s in calls if s.get("id") == 0x126)
            chests.append(dict(worker=worker["index"], position=list(map(float, position["args"]))))
        if stem == "city" and worker["index"] in [42,43,59,60]:
            assert worker["type"] == 3
            center = next(s for s in calls if s.get("id") == 0x57)
            size = next(s for s in calls if s.get("id") == 0x59)
            angle = next(s for s in calls if s.get("id") == 0x5a)
            low, high = worker["entries"][4:6]
            requests = [s for b in worker["blocks"] for s in b["statements"] if low<=s["pc"]<high and s.get("kind")=="request"]
            expected = {42:25,43:26,59:27,60:28}[worker["index"]]
            assert any(s.get("worker")=="0" and s.get("entry")==str(expected) for s in requests)
            controls.append(dict(worker=worker["index"],center=list(map(float,center["args"])),
                                 size=list(map(float,size["args"])),angle=float(angle["args"][0]),player_entry=expected))
    lines = [str(scale), str(len(entrances)), *(" ".join(map(str, p)) for p in entrances), str(len(exits))]
    lines += [" ".join(map(str, [e["worker"], *e["edge"], *e["size"]])) for e in exits]
    lines += [str(len(chests)), *(" ".join(map(str, [c["worker"], *c["position"]])) for c in chests)]
    lines += [str(len(controls)), *(" ".join(map(str,[c["worker"],*c["center"],*c["size"],c["angle"]])) for c in controls)]
    retain(output / (stem + "_facts_v2.txt"), ("\n".join(lines) + "\n").encode("ascii"))
    rows.append(dict(stem=stem, map=item, ebp_sha256=source_hash, triangles=triangle_count, vertices=vertex_count,
                     scale=scale, root=root, triangle_offset=triangle_at-root, vertex_offset=vertex_at-root,
                     triangle_fnv64=fingerprint(raw_triangles), vertex_fnv64=fingerprint(raw_vertices),
                     collision_groups=dict(collections.Counter(t[6]&127 for t in triangles)),
                     entrances=entrances, exits=exits, chests=chests, controls=controls))
report = dict(utc=datetime.now(timezone.utc).isoformat(), original_assets=str(assets), areas=rows,
              limits="ChMap/static script facts; no game process, active native flag observation or full moving-object VM.")
(output / "geometry-manifest.json").write_text(json.dumps(report, indent=2), encoding="utf-8")
for row in rows:
    print(row["stem"], row["triangles"], row["vertices"], row["triangle_fnv64"], row["vertex_fnv64"])
print(output)
