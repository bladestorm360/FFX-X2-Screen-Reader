"""One-time original-tool path migration; backups/manifests stay outside Git.

Only known legacy evidence path expressions and original-byte replay writers are
transformed. AST locations bound each edit; syntax is checked before saving.
"""
from pathlib import Path
import ast
import json
import re
from evidence_paths import REPOSITORY, outside_git

BACKUP = outside_git(REPOSITORY.parent / "Analysis/FFX/RepositoryCleanup_20261020/source-originals")
PREFIXES = ("Docs/Scripts/Offline/output", "Docs/Scripts/Ghidra/output",
            "Docs/Scripts/Frida/output", "Docs/Scripts/SWF/output")
REPLAY_NAMES = {p.name for p in (REPOSITORY / "src/native/tests").glob("*bytes.inc")}


def offsets(text):
    positions = [0]
    for line in text.splitlines(keepends=True):
        positions.append(positions[-1] + len(line))
    return positions


def span(node, positions):
    return positions[node.lineno-1] + node.col_offset, positions[node.end_lineno-1] + node.end_col_offset


def apply(text, edits):
    for start, end, replacement in sorted(edits, reverse=True):
        text = text[:start] + replacement + text[end:]
    return text


def path_components(node):
    if isinstance(node, ast.BinOp) and isinstance(node.op, ast.Div):
        left = path_components(node.left)
        if left is not None and isinstance(node.right, ast.Constant) and isinstance(node.right.value, str):
            return left + [node.right.value]
        if left is not None and isinstance(node.right, ast.JoinedStr) and node.right.values and isinstance(node.right.values[0], ast.Constant):
            return left + [str(node.right.values[0].value) + "<dynamic>"]
    if isinstance(node, ast.Name):
        return [node.id]
    return None


def migrate(path):
    original = path.read_text(encoding="utf-8-sig")
    text = original
    tree = ast.parse(text); positions = offsets(text)
    parents = {child: parent for parent in ast.walk(tree) for child in ast.iter_child_nodes(parent)}
    path_edits = []
    for node in ast.walk(tree):
        parts = path_components(node)
        if not parts or not any("/".join(parts[1:]).startswith(prefix) for prefix in PREFIXES):
            continue
        child_parts = path_components(node.left) if isinstance(node, ast.BinOp) else None
        if child_parts and any("/".join(child_parts[1:]).startswith(prefix) for prefix in PREFIXES):
            continue  # Wrap the minimum complete legacy prefix, once.
        parent = parents.get(node)
        if isinstance(parent, ast.Call) and isinstance(parent.func, ast.Name) and parent.func.id == "evidence_path":
            continue
        start, end = span(node, positions)
        path_edits.append((start, end, "evidence_path(" + text[start:end] + ")"))
    text = apply(text, path_edits)
    tree = ast.parse(text); positions = offsets(text)
    aliases = set()
    for node in ast.walk(tree):
        if isinstance(node, ast.Assign) and any(isinstance(value, ast.Constant) and
                isinstance(value.value, str) and any(name in value.value for name in REPLAY_NAMES)
                for value in ast.walk(node.value)):
            aliases.update(target.id for target in node.targets if isinstance(target, ast.Name))
    writer_edits = []
    for node in ast.walk(tree):
        if not isinstance(node, ast.Call) or not isinstance(node.func, ast.Attribute):
            continue
        if node.func.attr == "write_text":
            target = ast.get_source_segment(text, node.func.value)
            qualified = any(name in target for name in REPLAY_NAMES) or (
                isinstance(node.func.value, ast.Name) and node.func.value.id in aliases)
            if qualified:
                args = [ast.get_source_segment(text, arg) for arg in node.args]
                keywords = [(keyword.arg + "=" if keyword.arg else "**") +
                            ast.get_source_segment(text, keyword.value) for keyword in node.keywords]
                start, end = span(node, positions)
                writer_edits.append((start, end, "write_replay_adapter(" + ", ".join([target] + args + keywords) + ")"))
    text = apply(text, writer_edits)
    if text == original:
        return None
    # Relocated evidence can be outside ROOT. Metadata keeps absolute external
    # locators instead of raising ValueError or inventing a repository location.
    tree = ast.parse(text); positions = offsets(text); relative_edits = []
    for node in ast.walk(tree):
        if isinstance(node, ast.Call) and isinstance(node.func, ast.Attribute) and node.func.attr == "relative_to" and len(node.args) == 1:
            start, end = span(node, positions)
            relative_edits.append((start, end, "evidence_relative(" + ast.get_source_segment(text,node.func.value) + ", " + ast.get_source_segment(text,node.args[0]) + ")"))
    text = apply(text, relative_edits)
    if "from evidence_paths import" not in text:
        tree = ast.parse(text); insertion = 0
        for index, node in enumerate(tree.body):
            if (index == 0 and isinstance(node, ast.Expr) and isinstance(node.value, ast.Constant)
                    and isinstance(node.value.value, str)) or (isinstance(node, ast.ImportFrom) and node.module == "__future__"):
                insertion = offsets(text)[node.end_lineno]
            else:
                break
        setup = ('\n# Game-source inputs/exports live outside every Git tree.\n'
                 'from pathlib import Path as _EvidencePath\n'
                 'import sys as _evidence_sys\n'
                 '_evidence_repo = next(p for p in _EvidencePath(__file__).resolve().parents\n'
                 '                      if (p / "Docs/Scripts/Offline/evidence_paths.py").is_file())\n'
                 '_evidence_sys.path.insert(0, str(_evidence_repo / "Docs/Scripts/Offline"))\n'
                 'from evidence_paths import evidence_path, evidence_relative, write_replay_adapter\n\n')
        text = text[:insertion] + setup + text[insertion:]
    ast.parse(text)
    backup = BACKUP / path.relative_to(REPOSITORY)
    backup.parent.mkdir(parents=True, exist_ok=True)
    if not backup.exists():
        backup.write_bytes(path.read_bytes())
    path.write_text(text, encoding="utf-8")
    return {"path": path.relative_to(REPOSITORY).as_posix(), "evidence_paths": len(path_edits),
            "replay_writers": len(writer_edits), "metadata_locators": len(relative_edits)}


def main():
    changes = []
    # Native test preparers belong to a separate one-writer migration handoff.
    for directory in ("Docs/Scripts/Offline", "Docs/Scripts/Frida", "src/puzzles", "src/navigation"):
        for path in (REPOSITORY / directory).rglob("*.py"):
            if path.name in (Path(__file__).name, "evidence_paths.py"):
                continue
            changed = migrate(path)
            if changed:
                changes.append(changed)
    output = BACKUP.parent / "python-path-migration.json"
    output.write_text(json.dumps(changes, indent=2) + "\n", encoding="utf-8")
    print(f"Updated {len(changes)} original tools; {sum(x['evidence_paths'] for x in changes)} evidence paths, "
          f"{sum(x['replay_writers'] for x in changes)} source-free replay writers; all syntax checks pass.")


if __name__ == "__main__":
    main()
