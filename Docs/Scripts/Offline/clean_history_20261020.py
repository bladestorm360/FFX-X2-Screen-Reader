"""Prepare/apply the user-authorized local historical game-fragment cleanup.

No checkout/reset, new mod commit, remote push or working-file deletion occurs.
Original Git metadata is archived outside Git before writing clean objects.
"""
from pathlib import Path
import argparse
import hashlib
import json
import re
import subprocess
import zipfile
from evidence_paths import REPOSITORY, outside_git

OUTPUT = outside_git(REPOSITORY.parent / "Analysis/FFX/RepositoryCleanup_20261020")


def git(*args, data=None):
    return subprocess.check_output(["git", "-C", str(REPOSITORY), *args], input=data)


def archive():
    target = OUTPUT / "git-history-before.zip"
    if target.exists():
        return
    assert (REPOSITORY / ".git").is_dir(), "Only the audited standalone repository is supported"
    with zipfile.ZipFile(target, "x", zipfile.ZIP_DEFLATED) as package:
        for path in (REPOSITORY / ".git").rglob("*"):
            if path.is_file():
                package.write(path, path.relative_to(REPOSITORY).as_posix())
    with zipfile.ZipFile(target) as package:
        assert package.testzip() is None
    (OUTPUT / "git-history-before.sha256").write_text(hashlib.sha256(target.read_bytes()).hexdigest() + "\n")


def clean_comment_fragments(body):
    # Only line comments are changed. Native names/ABI locators remain; actual
    # mod statements and their ordering are never rewritten by this operation.
    lines = body.splitlines(keepends=True)
    result = []
    for line in lines:
        stripped = line.lstrip()
        if stripped.startswith("//") and (
            re.search(r"\bundefined[1248]?\b.*\bFUN_[0-9a-fA-F]+\s*\(", stripped)
            or ("DAT_" in stripped and "param_" in stripped and "=" in stripped)
            or ("DAT_" in stripped and re.search(r"\{\s*return\b", stripped))
        ):
            indent = line[:len(line)-len(stripped)]
            names = re.findall(r"(?:FUN|DAT)_[0-9a-fA-F]+", stripped)
            locator = ", ".join(names)
            result.append(indent + "// Native binding/layout evidence" + (": " + locator if locator else "") +
                          ". Refer to the external original; copied implementation removed.\n")
        else:
            result.append(line)
    return "".join(result)


def prepare():
    assert not git("diff", "--cached", "--name-only").strip(), "Preserve staged changes before historical migration"
    OUTPUT.mkdir(parents=True, exist_ok=True)
    archive()
    changes = []; tree_cache = {}; blob_cache = {}

    def blob(oid, name):
        key = (oid, name)
        if key in blob_cache:
            return blob_cache[key]
        original = git("cat-file", "blob", oid)
        if name.startswith("Docs/") and name.endswith(".md"):
            # Historical documents mix copied implementations with prose. Keep
            # complete originals externally and preserve factual current records
            # in the untouched working tree, rather than risk partial redaction.
            location = OUTPUT / "history-originals" / oid / name
            location.parent.mkdir(parents=True, exist_ok=True)
            location.write_bytes(original)
            replacement = ("# Historical documentation externalized — October 20, 2026\n\n"
                "This historical document mixed research with copied game implementation. "
                "The complete original is preserved outside Git in "
                "`Analysis/FFX/RepositoryCleanup_20261020/history-originals/" + oid + "/" + name + "`. "
                "Current source-safe behavior, architecture and evidence are indexed in "
                "[the feature documentation](features/index.md). "
                "No original game implementation is retained in this historical blob.\n").encode()
        elif name.endswith((".js", ".cpp", ".hpp", ".h", ".c")):
            text = original.decode("utf-8-sig")
            replacement = clean_comment_fragments(text).encode("utf-8")
            if replacement != original and text == clean_comment_fragments(text):
                replacement = original  # Preserve an unrelated original BOM.
        else:
            replacement = original
        if replacement == original:
            result = oid
        else:
            result = git("hash-object", "-w", "--stdin", data=replacement).decode().strip()
            changes.append({"path": name, "old_blob": oid, "new_blob": result,
                            "kind": "historical mixed documentation" if name.startswith("Docs/") else "comment-only fragment"})
        blob_cache[key] = result
        return result

    def tree(oid, prefix=""):
        key = (oid, prefix)
        if key in tree_cache:
            return tree_cache[key]
        entries = []
        for row in git("ls-tree", "-z", oid).split(b"\0"):
            if not row:
                continue
            meta, raw_name = row.split(b"\t", 1)
            mode, kind, raw_oid = meta.split(b" ")
            name = raw_name.decode(); child = raw_oid.decode(); path = prefix + name
            updated = tree(child, path + "/") if kind == b"tree" else blob(child, path)
            entries.append(mode + b" " + kind + b" " + updated.encode() + b"\t" + raw_name + b"\0")
        result = git("mktree", "-z", data=b"".join(entries)).decode().strip()
        tree_cache[key] = result
        return result

    commits = {}; ordered = git("rev-list", "--reverse", "--topo-order", "--all").decode().splitlines()
    for old in ordered:
        raw = git("cat-file", "commit", old)
        headers, message = raw.split(b"\n\n", 1)
        assert b"gpgsig " not in headers, "A signed history requires a separately reviewed signature migration"
        lines = []
        for line in headers.splitlines():
            if line.startswith(b"tree "):
                line = b"tree " + tree(line[5:].decode()).encode()
            elif line.startswith(b"parent "):
                line = b"parent " + commits[line[7:].decode()].encode()
            lines.append(line)
        cleaned = b"\n".join(lines) + b"\n\n" + message
        commits[old] = git("hash-object", "-t", "commit", "-w", "--stdin", data=cleaned).decode().strip()
    refs = []
    for row in git("for-each-ref", "--format=%(refname) %(objectname) %(objecttype)").decode().splitlines():
        name, old, kind = row.split()
        if kind == "tag":
            raw = git("cat-file", "tag", old)
            assert b"BEGIN PGP SIGNATURE" not in raw
            target = raw.splitlines()[0].split()[1].decode()
            assert target in commits
            raw = raw.replace(b"object " + target.encode() + b"\n", b"object " + commits[target].encode() + b"\n", 1)
            new = git("hash-object", "-t", "tag", "-w", "--stdin", data=raw).decode().strip()
        else:
            assert kind == "commit" and old in commits
            new = commits[old]
        refs.append({"ref": name, "old": old, "new": new})
    plan = {"repository": str(REPOSITORY), "commits": commits, "refs": refs, "changed_blobs": changes,
            "working_tree_untouched": True, "staged_changes": False,
            "archive": str(OUTPUT / "git-history-before.zip"), "remote_action": "not performed"}
    (OUTPUT / "history-plan.json").write_text(json.dumps(plan, indent=2) + "\n")
    print(f"Prepared {len(commits)} sanitized commits/{len(refs)} refs; {len(changes)} historical blob replacements. No refs or working files changed.")


def apply_plan():
    plan = json.loads((OUTPUT / "history-plan.json").read_text())
    assert plan["repository"] == str(REPOSITORY)
    # One atomic ref transaction, with old-object leases. Remote-tracking refs
    # are retired locally until the separately authorized remote cleanup occurs.
    commands = ["start"]
    for row in plan["refs"]:
        if row["ref"].startswith("refs/remotes/"):
            commands.append(f"delete {row['ref']} {row['old']}")
        else:
            commands.append(f"update {row['ref']} {row['new']} {row['old']}")
    commands += ["prepare", "commit"]
    git("update-ref", "--stdin", data=("\n".join(commands) + "\n").encode())
    git("read-tree", "HEAD")  # Index only: never reset/checkout the working files.
    git("reflog", "expire", "--expire=now", "--all")
    git("gc", "--prune=now")
    plan["local_cleanup_applied"] = True
    plan["old_refs_retained_locally"] = False
    (OUTPUT / "history-plan.json").write_text(json.dumps(plan, indent=2) + "\n")
    print("Applied sanitized local history/index and pruned old reflogs/objects. Working files and remotes were not modified.")


if __name__ == "__main__":
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--apply", action="store_true")
    args = parser.parse_args()
    apply_plan() if args.apply else prepare()
