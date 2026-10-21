"""Extract original field model/texture assets and a hashed model-kind index.

Read-only VBF access; explicit --archive required. Extracts into the workspace,
never beside the archive. --codes accepts n080,f011,...; --all-field extracts
pc/npc/obj DX11 models and textures for offline classification/visual review.
Texture metadata is decoded by phyre_texture, with format attribution there.
"""
import argparse
from datetime import datetime, timezone
import hashlib
import io
import json
from pathlib import Path
import re
from PIL import Image, ImageDraw
from phyre_texture import texture_dds
from phyre_mesh import mesh_sets, write_obj
from vbf import Vbf, output_path

ROOT = Path(__file__).resolve().parents[3]
MODEL_BASE = {"pc": 0, "npc": 0x2000, "obj": 0x5000, "mon": 0x1000}


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--archive", type=Path, required=True)
    parser.add_argument("--output", type=Path)
    parser.add_argument("--codes", default="f011,f016,n080,n122,n126,n234,n001,n999,m250")
    parser.add_argument("--all-field", action="store_true")
    parser.add_argument("--catalog", type=Path)
    args = parser.parse_args()
    stamp = datetime.now(timezone.utc).strftime("%Y%m%d_%H%M%S")
    output = (args.output or Path(__file__).parent / "output" / f"model_assets_{stamp}").resolve()
    if not output.is_relative_to(ROOT) or output == ROOT:
        raise ValueError("Output must be a dedicated workspace subdirectory")
    output.mkdir(parents=True, exist_ok=False)
    vbf = Vbf(args.archive)
    codes = set(args.codes.split(","))
    models, extracted, thumbnails, failures = {}, [], {}, []
    for entry in vbf.entries:
        match = re.search(r"/chr/(pc|npc|obj|mon)/([cnfm])(\d{3})/", entry.name)
        if not match:
            continue
        group, prefix, num = match.groups()
        code = prefix + num
        model = MODEL_BASE[group] + int(num)
        record = models.setdefault(model, dict(group=group, code=code, paths=[]))
        record["paths"].append(entry.name)
        wanted = code in codes or args.all_field and group in ("pc", "npc", "obj")
        if not wanted or not ("/d3d11/" in entry.name or entry.name.endswith(".ahwin32")):
            continue
        data = vbf.read(entry)
        path = output_path(output, entry.name)
        path.parent.mkdir(parents=True, exist_ok=True)
        path.write_bytes(data)
        extracted.append(dict(path=entry.name, size=len(data), sha256=hashlib.sha256(data).hexdigest()))
        if entry.name.endswith(".dae.phyre"):
            try:
                sets = mesh_sets(data)
                write_obj(path.with_suffix(".obj"), sets)
                extracted[-1]["mesh"] = dict(sets=len(sets), faces=sum(len(s["faces"]) for s in sets))
            except ValueError as error:
                failures.append(dict(path=entry.name, error=str(error)))
        if entry.name.endswith(".dds.phyre"):
            try:
                dds, metadata = texture_dds(data)
                path.with_suffix(".dds").write_bytes(dds)
                image = Image.open(io.BytesIO(dds)).convert("RGB")
                image.save(path.with_suffix(".png"))
                extracted[-1]["texture"] = metadata
                image.thumbnail((128, 128))
                cell = Image.new("RGB", (144, 156), "#202020")
                cell.paste(image, ((144-image.width)//2, 20))
                ImageDraw.Draw(cell).text((4, 2), f"{code} / {model:04x}", fill="white")
                if code not in thumbnails or path.name == code + ".dds.phyre":
                    thumbnails[code] = cell
            except (ValueError, KeyError) as error:
                failures.append(dict(path=entry.name, error=str(error)))
    # A dummy with no actual material/texture is not evidence of a character.
    for record in models.values():
        record["textured"] = any("/tex/d3d11/" in p for p in record["paths"])
    thumbnails = sorted(thumbnails.items())
    for page, start in enumerate(range(0, len(thumbnails), 48)):
        sheet = Image.new("RGB", (8*144, 6*156), "#202020")
        for i, (_, cell) in enumerate(thumbnails[start:start+48]):
            sheet.paste(cell, ((i%8)*144, (i//8)*156))
        sheet.save(output / f"textures_{page:02d}.png")
    manifest = dict(utc=datetime.now(timezone.utc).isoformat(), archive=str(args.archive),
                    user_authorization="2026-10-06 extract sprites to verify NPC/object labels",
                    assets=extracted, models=models, decode_failures=failures)
    (output / "manifest.json").write_text(json.dumps(manifest, indent=2)+"\n", encoding="utf8")
    if args.catalog:
        catalog = args.catalog.resolve()
        if not catalog.is_relative_to(ROOT):
            raise ValueError("Catalog outside workspace")
        catalog.write_text(json.dumps(models, indent=2)+"\n", encoding="utf8")
    print(f"Extracted {len(extracted)} assets, decoded {len(thumbnails)} textures; {len(failures)} unsupported. Evidence: {output.name}")


if __name__ == "__main__":
    main()
