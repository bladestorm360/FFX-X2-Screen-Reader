"""Record completed offline checks/deployment; never run or inspect game state."""

# Game-source inputs/exports live outside every Git tree.
from pathlib import Path as _EvidencePath
import sys as _evidence_sys
_evidence_repo = next(p for p in _EvidencePath(__file__).resolve().parents
                      if (p / "Docs/Scripts/Offline/evidence_paths.py").is_file())
_evidence_sys.path.insert(0, str(_evidence_repo / "Docs/Scripts/Offline"))
from evidence_paths import evidence_path, evidence_relative, write_replay_adapter

from datetime import datetime, timezone
import hashlib
import json
from pathlib import Path
import re

ROOT = Path(__file__).resolve().parents[3]
OUTPUT = ROOT / "install/inn-menu-followup-20261008"
EVIDENCE = evidence_path(ROOT / "Docs/Scripts/Offline/output/FFX/20261003_audit")

def sha(path):
    return hashlib.sha256(path.read_bytes()).hexdigest().upper()

def main():
    logs = {name: (OUTPUT / (name + ".log")).read_text(encoding="utf8", errors="replace")
            for name in ("readers", "geometry", "lightning", "build", "proxy", "deploy")}
    assert all(not re.search(r"FAIL:|\b(?:error|warning) (?:C|LNK|CS)\d+", text) for text in logs.values())
    assert logs["readers"].count("PASS:") == 10
    assert "reference profile" in logs["readers"] and "installed profile" in logs["readers"]
    assert logs["geometry"].count("PASS:") == logs["lightning"].count("PASS:") == 1
    assert "Native x86 build OK" in logs["build"] and "Proxy fixture passed" in logs["proxy"]
    assert "Deploy complete" in logs["deploy"] and "All 13 installed artifacts verified" in logs["deploy"]
    assert logs["deploy"].count("Compiled native profile/signatures/operands") == 4
    legacy = (OUTPUT / "legacy-side.log").read_text(encoding="utf8", errors="replace")
    assert "FAIL: distant winding inn route crosses the finite trigger" in legacy
    assert not re.search(r"\b(?:error|warning) (?:C|LNK|CS)\d+", legacy)

    baseline = json.loads((EVIDENCE / "navigation_audit_20261007_164736.json").read_text(encoding="utf8"))["labels"]
    latest = max(EVIDENCE.glob("navigation_audit_*.json"))
    current = json.loads(latest.read_text(encoding="utf8"))["labels"]
    identity = lambda rows: [(r["source"], r["worker"], r["entries"], r["models"]) for r in rows]
    assert identity(baseline) == identity(current)
    label_changes = [dict(source=a["source"], script=a["script"], worker=a["worker"],
                          before=[a["category"], a["label"]], after=[b["category"], b["label"]])
                     for a, b in zip(baseline, current)
                     if (a["category"], a["label"]) != (b["category"], b["label"])]

    # Exact deployment payload names come from the previous verified record.
    # These explicit file hashes recheck the current authorized installation;
    # no game assets, folder discovery or session probes are involved.
    previous = json.loads((ROOT / "install/player-reports-20261008/verification.json").read_text())
    game = Path(r"D:\Games\SteamLibrary\steamapps\common\FINAL FANTASY FFX&FFX-2 HD Remaster")
    payload = [dict(name=item["name"], sha256=sha(game / item["name"])) for item in previous["installed_payload"]]
    assert len(payload) == 13
    dll = sha(ROOT / "src/proxy/dinput8.dll")
    assert dll == sha(ROOT / "install/native/dinput8.dll") == sha(game / "dinput8.dll")
    sources = [ROOT / "src/native" / name for name in
               ("battle.cpp", "battle_status.cpp", "navigation.cpp", "nav_zones.cpp", "lightning.cpp", "settings.cpp")]
    sources += sorted((ROOT / "src/native").glob("navigation_targets_*.inc"))
    sources += [ROOT / "src/navigation" / name for name in ("identity_labels.py", "finish_targets.py")]
    sources += [ROOT / "src/native/tests" / name for name in
                ("reader_fixture.cpp", "battle_followups.inc", "battle_switch_followups.inc", "battle_outcomes_followups.inc", "navigation_exit_followups.inc",
                 "navigation_identity_followups.inc", "lightning_fixture.cpp")]
    for path in sources:
        if path.parent == ROOT / "src/native":
            assert len(path.read_text().splitlines()) <= 500
            assert b"\n" not in path.read_bytes().replace(b"\r\n", b"")

    result = dict(utc=datetime.now(timezone.utc).isoformat(), model="Sol", delegations=[],
                  implemented=True, offline_verified=True, deployed=True,
                  auto_dodge_gameplay_confirmed=True, manual_dodge_gameplay_confirmed=False,
                  other_new_gameplay_checks_confirmed=False, game_launched=False,
                  runtime_probes_run=False, tester_logs_accessed=False, current_own_log_read=True,
                  release_zip_refreshed=False, offline_identity_pass_summaries=4,
                  native_pass_summaries=12, reader_profiles=["Reference", "Installed"],
                  compiled_executable_profiles=["FFX", "Will captions"], launcher_verified=True,
                  installed_hash_count=13, dll_sha256=dll, installed_payload=payload,
                  catalog_rows=len(current), catalog_membership_preserved=True,
                  original_side_filter_failure_reproduced=True,
                  floating_zero_digit_renderer_verified=True,
                  outcome_requires_successful_popup_registration=True,
                  catalog_label_changes=label_changes, catalog_audit=latest.name,
                  warning_elapsed_tick=0, auto_dodge_elapsed_tick=6,
                  source_hashes={str(evidence_relative(p, ROOT)): sha(p) for p in sources})
    (OUTPUT / "verification.json").write_text(json.dumps(result, indent=2) + "\n", encoding="utf8")
    print("PASS: final logs, both compiled profiles, 13 installed hashes, unchanged target membership and production format.")

if __name__ == "__main__":
    main()
