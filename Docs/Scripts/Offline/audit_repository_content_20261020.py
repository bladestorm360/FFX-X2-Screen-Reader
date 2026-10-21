"""Audit current files and every local Git blob without copying game bodies.

Pattern evidence complements the explicit provenance/migration review; a symbol
name alone is allowed and is deliberately not treated as an implementation.
"""
from pathlib import Path
import collections
import hashlib
import json
import os
import re
import subprocess
from evidence_paths import REPOSITORY, outside_git

REPORT = outside_git(REPOSITORY.parent / "Analysis/FFX/RepositoryCleanup_20261020")
TEXT = {".md", ".txt", ".json", ".cpp", ".hpp", ".inc", ".h", ".c", ".js", ".cs", ".py", ".java", ".asm", ".log", ".ps1", ".bat", ".rc", ".def", ".mjs"}
BODY = re.compile(r"^\s*(?:(?:undefined[1248]?|void|int|char|float|double|uint|bool)\s+)+"
                  r"(?:__\w+\s+)?FUN_[0-9a-fA-F]{6,}\s*\([^\n]*\)\s*(?:\n\s*)?\{", re.M)
DISASSEMBLY = re.compile(r"^\s*[0-9a-fA-F]{6,16}[: ]+\s*(?:[0-9a-fA-F]{2}\s+)*"
                        r"(?:mov|push|pop|ret|call|jmp|lea|xor|test|cmp|jz|jnz)\b", re.M)
COPIED_RETURN = re.compile(r"^\s*//\s*\{\s*return\b[^\n]*DAT_[0-9a-fA-F]+", re.M)
RAW_ARRAY = re.compile(r"(?:constexpr|const)\s+(?:uint8_t|unsigned char|BYTE)\s+"
                       r"\w*(?:Body|Getter)\w*\s*\[[^\]]*\]\s*=\s*\{\s*(?:0x[0-9a-fA-F]+|\d+)\s*,", re.M)


def findings(body, name):
    result = []
    for label, pattern in (("decompiled body", BODY), ("addressed disassembly", DISASSEMBLY),
                           ("copied native return", COPIED_RETURN)):
        for match in pattern.finditer(body):
            result.append({"kind": label, "line": body.count("\n", 0, match.start()) + 1})
    if name.startswith("src/native/tests/"):
        for match in RAW_ARRAY.finditer(body):
            result.append({"kind": "embedded original-body candidate", "line": body.count("\n",0,match.start())+1})
    return result


def main():
    files = []; violations = []; classes = collections.Counter()
    for base, directories, names in os.walk(REPOSITORY):
        directories[:] = [name for name in directories if name not in (".git", "node_modules", "__pycache__")]
        for name in names:
            path = Path(base) / name; relative = path.relative_to(REPOSITORY).as_posix()
            size = path.stat().st_size; files.append({"path":relative,"bytes":size})
            if relative.startswith("install/sdl3-x86/"):
                classes["licensed SDL build artifacts"] += 1; continue
            if path.suffix.lower() in {".ebp", ".phyre", ".as", ".dds"} or re.fullmatch(r"block_[0-9a-fA-F]+\.bin", name):
                violations.append({"path":relative,"kind":"original game-input/asset candidate"})
            if path.suffix.lower() in TEXT:
                body=path.read_text(encoding="utf-8-sig",errors="replace")
                hits=findings(body,relative)
                if hits:violations.append({"path":relative,"findings":hits})
    object_rows=subprocess.check_output(["git","-C",str(REPOSITORY),"cat-file","--batch-all-objects","--batch-check=%(objectname) %(objecttype)"],text=True).splitlines()
    blobs=0;history=[]
    for row in object_rows:
        oid,kind=row.split()
        if kind!="blob":continue
        blobs+=1;raw=subprocess.check_output(["git","-C",str(REPOSITORY),"cat-file","blob",oid])
        if b"\0" in raw[:8192]:continue
        hits=findings(raw.decode("utf8",errors="replace"),"")
        if hits:history.append({"oid":oid,"findings":hits})
    report={"repository":str(REPOSITORY),"working_files":len(files),"working_bytes":sum(row["bytes"] for row in files),
            "working_findings":violations,"local_git_blobs":blobs,"git_findings":history,
            "classified_vendor":dict(classes),"scope_limit":"All non-vendor current text and all local Git blobs scanned; provenance/archives/remote review is recorded separately."}
    REPORT.mkdir(parents=True,exist_ok=True)
    (REPORT/"current-content-audit.json").write_text(json.dumps(report,indent=2)+"\n")
    (REPORT/"current-file-inventory.json").write_text(json.dumps(files,indent=2)+"\n")
    print(json.dumps({key:value for key,value in report.items() if key!="repository"},indent=2))
    if violations or history:raise SystemExit(1)


if __name__=="__main__":main()
