"""Original mod tooling: keep commercial-game evidence outside every Git tree.

Legacy locators remain recognizable, but never identify an in-repository output
directory. Source-free mod catalogs/docs are separate explicitly reviewed writes.
"""
from __future__ import annotations
import hashlib
import json
import os
import re
from pathlib import Path

REPOSITORY = Path(__file__).resolve().parents[3]
PREFIXES = {
    "Docs/Scripts/Offline/output": "offline-output",
    "Docs/Scripts/Ghidra/output": "ghidra-output",
    "Docs/Scripts/Frida/output": "frida-output",
    "Docs/Scripts/SWF/output": "swf-output",
}


def outside_git(path: Path | str) -> Path:
    resolved = Path(path).expanduser().resolve()
    for parent in (resolved, *resolved.parents):
        if (parent / ".git").exists() or (
            (parent / "HEAD").is_file() and (parent / "objects").is_dir()
        ):
            raise ValueError(f"Game evidence must be outside every Git tree: {resolved}")
    return resolved


def evidence_root() -> Path:
    configured = os.environ.get("FFX_EVIDENCE_ROOT")
    return outside_git(configured or REPOSITORY.parent / "Analysis/FFX/RepositoryEvidence")


def evidence_path(legacy: Path | str) -> Path:
    """Translate a historical repository locator to its external evidence home."""
    path = Path(legacy)
    if path.is_absolute():
        try:
            path = path.relative_to(REPOSITORY)
        except ValueError:
            return outside_git(path)
    text = path.as_posix()
    for prefix, destination in PREFIXES.items():
        if text == prefix or text.startswith(prefix + "/"):
            return outside_git(evidence_root() / destination / text[len(prefix):].lstrip("/"))
    raise ValueError(f"Unclassified game-evidence locator: {legacy}")


def evidence_relative(path: Path, root: Path) -> Path:
    """Keep relative mod locators; preserve an absolute external source locator."""
    try:
        return path.relative_to(root)
    except ValueError:
        return path.resolve()


def write_replay_adapter(target: Path, source: str, encoding: str = "utf-8") -> int:
    """Write original test declarations that load separately retained game bytes.

    Only this reviewed numeric-byte-array format is accepted. Relocation tables
    and original mod adapter types stay in source; original bodies never do.
    """
    pattern = re.compile(
        r"(?P<inline>inline\s+)?(?P<static>static\s+)?(?:constexpr|const)\s+"
        r"(?:unsigned char|uint8_t|BYTE)\s+(?P<name>\w+)\s*\[[^\]]*\]\s*=\s*"
        r"\{(?P<data>[^}]+)\}\s*;", re.S
    )
    manifest = []

    def externalize(match: re.Match) -> str:
        values = [value.strip() for value in match["data"].split(",") if value.strip()]
        if not values or not all(re.fullmatch(r"(?:0[xX][0-9a-fA-F]+|\d+)", value) for value in values):
            raise ValueError(f"Unreviewed replay initializer: {target}:{match['name']}")
        body = bytes(int(value, 0) for value in values)
        name = match["name"]
        destination = outside_git(evidence_root() / "native" / (name + ".bin"))
        destination.parent.mkdir(parents=True, exist_ok=True)
        destination.write_bytes(body)
        fingerprint = 14695981039346656037
        for byte in body:
            fingerprint = ((fingerprint ^ byte) * 1099511628211) & 0xffffffffffffffff
        manifest.append({"source": str(target), "name": name, "bytes": len(body),
                         "sha256": hashlib.sha256(body).hexdigest(), "path": str(destination)})
        prefix = (match["inline"] or "") + (match["static"] or "")
        return (f'{prefix}const auto {name}Storage=::ffx_fixture::load<{len(body)}>'
                f'("native/{name}.bin",0x{fingerprint:016x}ull);\n'
                f'{prefix}const auto& {name}={name}Storage.bytes;')

    result, count = pattern.subn(externalize, source)
    if not count:
        raise ValueError(f"No qualified original byte arrays found in replay output: {target}")
    proof = outside_git(evidence_root() / "generator-manifests" / (target.name + ".json"))
    proof.parent.mkdir(parents=True, exist_ok=True)
    proof.write_text(json.dumps(manifest, indent=2) + "\n", encoding="utf-8")
    return target.write_text(result, encoding=encoding)


if __name__ == "__main__":
    import argparse
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--print-root", action="store_true")
    args = parser.parse_args()
    if args.print_root:
        print(evidence_root())
