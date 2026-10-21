"""
Extract ATEL call-target names from Karifean's FFXDataParser source and produce
TSV annotation files keyed by (namespace, id) for use as a grep-overlay on the
Ghidra decompile.

Inputs (read-only):
  - D:/Games/Dev/Custom/FFX/FFXDataParser/src/main/java/atel/model/
      ScriptCallTargetLib.java   (call-target name table)
      ScriptOpcode.java          (opcode names + return types)
      ScriptConstants.java       (FUNCSPACES namespace ID -> short name)
  - D:/Games/Dev/Custom/FFX/fahrenheit/core/fh/Atel/
      call.cs                    (namespace runtime base addresses + arg counts)
      insn.cs                    (opcode byte values, cross-check)

Outputs (written to ffx-x2-screen-reader/Docs/Scripts/Ghidra/output/FFX/atel/):
  atel_namespaces.tsv             ns_id, ns_short_name, runtime_base_va, fahrenheit_entry_count
  atel_calls_<ns>.tsv  (one per namespace)
                                  id, name, internal_name, return_type, arg_count_kari,
                                  arg_count_fh, args, notes, agree
  atel_opcodes.tsv                opcode, mnemonic, name, return_type, has_operand
  atel_arg_count_disagreements.tsv  (subset where Fahrenheit and Karifean
                                     disagree on arg count -- expected to be empty
                                     or near-empty if both sources are current)
  README.md                       Provenance + format documentation.

Run:
  python "D:/Games/Dev/Custom/FFX/ffx-x2-screen-reader/Docs/Scripts/Python/extract_atel_names.py"

Idempotent. Re-run after FFXDataParser or Fahrenheit pulls.
"""

import os
import re
import sys

FFXDP_SRC = r"D:/Games/Dev/Custom/FFX/FFXDataParser/src/main/java/atel/model"
FH_SRC = r"D:/Games/Dev/Custom/FFX/fahrenheit/core/fh/Atel"
OUT_DIR = r"D:/Games/Dev/Custom/FFX/ffx-x2-screen-reader/Docs/Scripts/Ghidra/output/FFX/atel"


def read(path):
    with open(path, "r", encoding="utf-8") as f:
        return f.read()


def parse_funcspaces(constants_src):
    out = {}
    for m in re.finditer(r'FUNCSPACES\[(0x[0-9A-Fa-f]+)\]\s*=\s*"([^"]+)"', constants_src):
        out[int(m.group(1), 16)] = m.group(2)
    return out


def parse_fahrenheit_namespaces(call_cs):
    """Returns dict ns_id -> {csharp_name, short, base_va, count}.
    Fahrenheit's enum uses sequential implicit values for most entries
    (Common=0x0 explicit; Math, ChEvent, Camera, Battle, Map, Mount implicit;
    Movie=0xB explicit; Debug, AbiMap implicit). Parsing the enum text
    correctly is fiddly, so we cross-validate against the runtime base
    addresses (which ARE explicit) and the Assert(...Length == N) lines
    that ARE explicit.
    """
    # Names are extracted from the get_internal switch; bases too.
    bases = {}
    for m in re.finditer(
        r"AtelCallTargetNamespace\.(\w+)\s*=>\s*0x([0-9A-Fa-f]+)", call_cs):
        bases[m.group(1)] = int(m.group(2), 16)
    counts = {}
    for m in re.finditer(r"(\w+)\.Length\s*==\s*(\d+)", call_cs):
        counts[m.group(1)] = int(m.group(2))
    short_map = {}
    short_block = call_cs.split("to_string", 1)[1] if "to_string" in call_cs else ""
    for m in re.finditer(r"AtelCallTargetNamespace\.(\w+)\s*=>\s*\"([^\"]+)\"", short_block):
        short_map[m.group(1)] = m.group(2)

    # Walk the enum block in declaration order, tracking the running value.
    enum_ids = {}
    if "public enum AtelCallTargetNamespace" in call_cs:
        enum_block = call_cs.split("public enum AtelCallTargetNamespace", 1)[1]
        # End at first standalone closing brace at column 0
        end = enum_block.find("\n}")
        if end > 0:
            enum_block = enum_block[:end]
        running = 0
        for line in enum_block.splitlines():
            line = line.strip().rstrip(",").strip()
            if not line or line.startswith("//") or line.startswith("{"):
                continue
            if "=" in line:
                lhs, rhs = [s.strip() for s in line.split("=", 1)]
                rhs = rhs.split()[0]
                try:
                    running = int(rhs, 16) if rhs.startswith("0x") else int(rhs)
                except ValueError:
                    continue
                enum_ids[lhs] = running
                running += 1
            elif re.match(r"^\w+$", line):
                enum_ids[line] = running
                running += 1
    out = {}
    for name, nsid in enum_ids.items():
        out[nsid] = {
            "csharp_name": name,
            "short": short_map.get(name, ""),
            "base_va": bases.get(name),
            "count": counts.get(name, 0),
        }
    return out


def parse_fahrenheit_arg_counts(call_cs):
    """Return dict id (int) -> arg_count (int) from the Fahrenheit `new(0xID,
    count)` literals inside the namespace arrays. Only the 2-arg form
    `new(0xID, count)` carries arg counts; the 3-arg form
    `new("name", args, return_type)` is not used in this codebase, so simple
    regex suffices."""
    out = {}
    for m in re.finditer(r"new\(0x([0-9A-Fa-f]+),\s*(\d+)\)", call_cs):
        out[int(m.group(1), 16)] = int(m.group(2))
    return out


_ENTRY_RE = re.compile(
    r"^(\s*)(putCtWithIdx|putVoidWithIdx|putUnknownCt)\((0x[0-9A-Fa-f]+)\s*,\s*",
    re.MULTILINE,
)


def _extract_until_balanced(src, start):
    """Given index of '(' inside src, return the contents up to the matching
    ')' (exclusive of outer parens), respecting nested parens and strings."""
    depth = 0
    in_str = False
    esc = False
    i = start
    assert src[i] == "("
    i += 1
    begin = i
    while i < len(src):
        c = src[i]
        if in_str:
            if esc:
                esc = False
            elif c == "\\":
                esc = True
            elif c == '"':
                in_str = False
        else:
            if c == '"':
                in_str = True
            elif c == "(":
                depth += 1
            elif c == ")":
                if depth == 0:
                    return src[begin:i], i + 1
                depth -= 1
        i += 1
    raise ValueError("unbalanced parens")


_P_RE = re.compile(r"\bp\s*\(")


def _count_args_and_signature(rest):
    """rest is the content of the outer call after the `idx,` and (for named
    forms) the `new ScriptCallTarget(...)` opening. Returns (count, signature
    string) by counting top-level p(...) occurrences."""
    args = []
    pos = 0
    depth = 0
    i = 0
    while i < len(rest):
        c = rest[i]
        if c == "(":
            depth += 1
        elif c == ")":
            depth -= 1
        elif depth == 0 and rest.startswith("p(", i):
            inner, end = _extract_until_balanced(rest, i + 1)
            args.append(_clean_p_arg(inner))
            i = end
            continue
        i += 1
    sig = ", ".join(args)
    return len(args), sig


def _clean_p_arg(inner):
    parts = [s.strip().strip('"') for s in _split_top_commas(inner)]
    if len(parts) == 0:
        return ""
    if len(parts) == 1:
        s = parts[0]
        if s.isdigit():
            return f"p{s}"
        return s
    if len(parts) == 2:
        if parts[0].isdigit():
            return f"p{parts[0]}:{parts[1]}"
        return f"{parts[0]}:{parts[1]}"
    return ":".join(parts)


def _split_top_commas(s):
    out = []
    cur = []
    depth = 0
    in_str = False
    esc = False
    for c in s:
        if in_str:
            cur.append(c)
            if esc:
                esc = False
            elif c == "\\":
                esc = True
            elif c == '"':
                in_str = False
        elif c == '"':
            cur.append(c)
            in_str = True
        elif c == "(":
            depth += 1
            cur.append(c)
        elif c == ")":
            depth -= 1
            cur.append(c)
        elif c == "," and depth == 0:
            out.append("".join(cur))
            cur = []
        else:
            cur.append(c)
    if cur:
        out.append("".join(cur))
    return out


def _parse_sct_constructor(body):
    """body = content between `new ScriptCallTarget(` and its matching `)`.
    Returns (name, return_type, internal_name, arg_count, arg_sig).

    Handles all four constructor shapes:
      (String name, String type, String internalName, boolean brackets)
      (String name, String type, String internalName, ScriptField... inputs)
      (String name, ScriptField... inputs)
      (ScriptField... inputs)
    """
    parts = _split_top_commas(body)
    parts = [p.strip() for p in parts]
    name = ""
    return_type = ""
    internal_name = ""
    string_prefix = 0
    while string_prefix < len(parts):
        p = parts[string_prefix]
        if p == "null" or (p.startswith('"') and p.endswith('"')):
            string_prefix += 1
        else:
            break
    leading_strings = parts[:string_prefix]
    rest = parts[string_prefix:]
    # Strip trailing `true`/`false` brackets sentinel from rest
    has_brackets_sentinel = False
    if rest and rest[-1] in ("true", "false"):
        has_brackets_sentinel = True
        rest = rest[:-1]
    # Disambiguate string-prefix shapes:
    #   3 strings -> (name, type, internalName, ...)
    #   2 strings -> not a documented shape; treat as (name, ...) with second string folded in
    #   1 string  -> (name, ...)
    #   0 strings -> (...)
    if len(leading_strings) >= 3:
        name = leading_strings[0].strip('"') if leading_strings[0] != "null" else ""
        return_type = leading_strings[1].strip('"') if leading_strings[1] != "null" else ""
        internal_name = leading_strings[2].strip('"') if leading_strings[2] != "null" else ""
        if len(leading_strings) > 3:
            # Unexpected; fold extras into rest as if they were positional args
            rest = leading_strings[3:] + rest
    elif len(leading_strings) == 1:
        name = leading_strings[0].strip('"') if leading_strings[0] != "null" else ""
    elif len(leading_strings) == 2:
        name = leading_strings[0].strip('"') if leading_strings[0] != "null" else ""
        # second string isn't a known position; keep as note
    arg_args = []
    for r in rest:
        if r.startswith("p(") or r.startswith("new ScriptField"):
            inner_open = r.find("(")
            inner = r[inner_open + 1:r.rfind(")")]
            arg_args.append(_clean_p_arg(inner))
    return name, return_type, internal_name, len(arg_args), ", ".join(arg_args)


def parse_calltarget_lib(src):
    out = {}
    for m in _ENTRY_RE.finditer(src):
        kind = m.group(2)
        idx = int(m.group(3), 16)
        rest_start = m.end()
        line_end = src.find("\n", rest_start)
        if line_end < 0:
            line_end = len(src)
        line_tail = src[rest_start:line_end]
        comment = ""
        cidx = line_tail.find("//")
        if cidx >= 0:
            comment = line_tail[cidx + 2:].strip()
        outer_open = src.rfind("(", 0, rest_start)
        contents, _ = _extract_until_balanced(src, outer_open)
        head = contents.split(",", 1)
        if len(head) < 2:
            continue
        body = head[1].lstrip()
        name = ""
        internal_name = ""
        return_type = ""
        arg_sig = ""
        arg_count = 0
        if kind in ("putCtWithIdx", "putVoidWithIdx"):
            mm = re.search(r"new\s+(ScriptCallTarget\w*)\s*\(", body)
            if mm:
                paren_idx = mm.end() - 1
                inner, _ = _extract_until_balanced(body, paren_idx)
                name, return_type, internal_name, arg_count, arg_sig = _parse_sct_constructor(inner)
        else:  # putUnknownCt
            parts = _split_top_commas(body)
            parts = [p.strip() for p in parts]
            if len(parts) == 1 and parts[0].isdigit():
                arg_count = int(parts[0])
            elif len(parts) == 2:
                if parts[1].isdigit() and parts[0].startswith('"'):
                    internal_name = parts[0].strip('"')
                    arg_count = int(parts[1])
                elif parts[0].isdigit():
                    arg_count = int(parts[0])
            elif len(parts) >= 3:
                if parts[0].startswith('"'):
                    internal_name = parts[0].strip('"')
                if parts[1].startswith('"'):
                    return_type = parts[1].strip('"')
                if parts[2].isdigit():
                    arg_count = int(parts[2])
                else:
                    tail = ", ".join(parts[2:])
                    arg_count, arg_sig = _count_args_and_signature(tail)
        out[idx] = {
            "id": idx,
            "name": name,
            "internal_name": internal_name,
            "return_type": return_type or "unknown",
            "arg_count": arg_count,
            "args": arg_sig,
            "kind": kind,
            "notes": comment,
        }
    return out


_OPCODE_RE = re.compile(
    r'putOpcode\(\s*(0x[0-9A-Fa-f]+)\s*,\s*"([^"]*)"\s*(?:,\s*"([^"]*)"\s*,\s*"([^"]*)")?',
)


def parse_opcodes(src):
    out = []
    for m in _OPCODE_RE.finditer(src):
        out.append({
            "opcode": int(m.group(1), 16),
            "name": m.group(2),
            "return_type": m.group(3) or "",
            "mnemonic": m.group(4) or "",
        })
    return out


def parse_fahrenheit_opcodes(insn_cs):
    out = {}
    enum_block = insn_cs.split("public enum AtelOp", 1)[1].split("}", 1)[0]
    for m in re.finditer(r"(\w+)\s*=\s*0x([0-9A-Fa-f]+)", enum_block):
        out[int(m.group(2), 16)] = m.group(1)
    return out


def write_tsv(path, header, rows):
    os.makedirs(os.path.dirname(path), exist_ok=True)
    with open(path, "w", encoding="utf-8", newline="") as f:
        f.write("\t".join(header) + "\n")
        for r in rows:
            f.write("\t".join(str(r.get(h, "")) for h in header) + "\n")


def main():
    constants_src = read(os.path.join(FFXDP_SRC, "ScriptConstants.java"))
    callctlib_src = read(os.path.join(FFXDP_SRC, "ScriptCallTargetLib.java"))
    opcodes_src = read(os.path.join(FFXDP_SRC, "ScriptOpcode.java"))
    fh_call_src = read(os.path.join(FH_SRC, "call.cs"))
    fh_insn_src = read(os.path.join(FH_SRC, "insn.cs"))

    funcspaces = parse_funcspaces(constants_src)
    fh_namespaces = parse_fahrenheit_namespaces(fh_call_src)
    fh_arg_counts = parse_fahrenheit_arg_counts(fh_call_src)
    kari_calls = parse_calltarget_lib(callctlib_src)
    kari_opcodes = parse_opcodes(opcodes_src)
    fh_opcodes = parse_fahrenheit_opcodes(fh_insn_src)

    print(f"Parsed {len(kari_calls)} call targets from FFXDataParser")
    print(f"Parsed {len(fh_arg_counts)} call target IDs from Fahrenheit")
    print(f"Parsed {len(funcspaces)} funcspaces (FFXDataParser)")
    print(f"Parsed {len(fh_namespaces)} namespaces (Fahrenheit)")
    print(f"Parsed {len(kari_opcodes)} opcodes (FFXDataParser)")
    print(f"Parsed {len(fh_opcodes)} opcodes (Fahrenheit)")

    # Namespace TSV
    ns_rows = []
    for nsid in sorted(set(funcspaces) | set(fh_namespaces)):
        kari_short = funcspaces.get(nsid, "")
        fh = fh_namespaces.get(nsid, {})
        ns_rows.append({
            "ns_id": f"0x{nsid:X}",
            "kari_short_name": kari_short,
            "fh_csharp_name": fh.get("csharp_name", ""),
            "fh_short_name": fh.get("short", ""),
            "runtime_base_va": f"0x{fh['base_va']:X}" if fh.get("base_va") else "",
            "fahrenheit_entry_count": fh.get("count", ""),
        })
    write_tsv(os.path.join(OUT_DIR, "atel_namespaces.tsv"),
              ["ns_id", "kari_short_name", "fh_csharp_name", "fh_short_name",
               "runtime_base_va", "fahrenheit_entry_count"], ns_rows)

    # Per-namespace call TSVs
    by_ns = {}
    for cid, e in kari_calls.items():
        nsid = cid >> 12
        by_ns.setdefault(nsid, []).append(e)
    # Also include Fahrenheit-only IDs (named blank)
    for cid in fh_arg_counts:
        if cid not in kari_calls:
            nsid = cid >> 12
            by_ns.setdefault(nsid, []).append({
                "id": cid, "name": "", "internal_name": "", "return_type": "",
                "arg_count": None, "args": "", "kind": "fahrenheit_only", "notes": "",
            })

    disagreements = []
    for nsid in sorted(by_ns):
        rows = []
        for e in sorted(by_ns[nsid], key=lambda r: r["id"]):
            cid = e["id"]
            fh_ac = fh_arg_counts.get(cid)
            kari_ac = e["arg_count"]
            agree = "" if (fh_ac is None or kari_ac is None or e["kind"] == "fahrenheit_only") else \
                ("yes" if fh_ac == kari_ac else "no")
            row = {
                "id": f"0x{cid:04X}",
                "name": e["name"],
                "internal_name": e["internal_name"],
                "return_type": e["return_type"],
                "arg_count_kari": "" if kari_ac is None else kari_ac,
                "arg_count_fh": "" if fh_ac is None else fh_ac,
                "args": e["args"],
                "notes": e["notes"],
                "kind": e["kind"],
                "agree": agree,
            }
            rows.append(row)
            if agree == "no":
                disagreements.append(row)
        ns_short = funcspaces.get(nsid, fh_namespaces.get(nsid, {}).get("short", f"ns{nsid:X}"))
        ns_short_norm = ns_short.lower().replace(" ", "_")
        path = os.path.join(OUT_DIR, f"atel_calls_{nsid:X}_{ns_short_norm}.tsv")
        write_tsv(path,
                  ["id", "name", "internal_name", "return_type",
                   "arg_count_kari", "arg_count_fh", "args", "notes",
                   "kind", "agree"], rows)

    # Disagreements
    write_tsv(os.path.join(OUT_DIR, "atel_arg_count_disagreements.tsv"),
              ["id", "name", "internal_name", "return_type",
               "arg_count_kari", "arg_count_fh", "args", "notes",
               "kind", "agree"], disagreements)
    print(f"Arg-count disagreements: {len(disagreements)}")

    # Opcodes TSV
    op_rows = []
    by_op = {}
    for op in kari_opcodes:
        by_op[op["opcode"]] = op
    for opcode in sorted(set(by_op) | set(fh_opcodes)):
        kari = by_op.get(opcode, {})
        fh_mnem = fh_opcodes.get(opcode, "")
        op_rows.append({
            "opcode": f"0x{opcode:02X}",
            "fh_mnemonic": fh_mnem,
            "kari_mnemonic": kari.get("mnemonic", ""),
            "kari_name": kari.get("name", ""),
            "kari_return_type": kari.get("return_type", ""),
            "has_operand": "yes" if (opcode & 0x80) else "no",
        })
    write_tsv(os.path.join(OUT_DIR, "atel_opcodes.tsv"),
              ["opcode", "fh_mnemonic", "kari_mnemonic", "kari_name",
               "kari_return_type", "has_operand"], op_rows)

    print(f"\nWrote outputs to {OUT_DIR}")


if __name__ == "__main__":
    main()
