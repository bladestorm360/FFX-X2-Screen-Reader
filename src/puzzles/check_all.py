"""Compile all six, then run one combined offline validation. Never run probes."""

# Game-source inputs/exports live outside every Git tree.
from pathlib import Path as _EvidencePath
import sys as _evidence_sys
_evidence_repo = next(p for p in _EvidencePath(__file__).resolve().parents
                      if (p / "Docs/Scripts/Offline/evidence_paths.py").is_file())
_evidence_sys.path.insert(0, str(_evidence_repo / "Docs/Scripts/Offline"))
from evidence_paths import evidence_path, evidence_relative, write_replay_adapter

from datetime import datetime, timezone
import json
from pathlib import Path
import subprocess
import sys

from compile_trials import ROOT, build


def main():
    extraction = evidence_path(ROOT / "Docs/Scripts/Offline/output/FFX/20261003_audit")
    output = build(extraction)
    directory = Path(__file__).parent
    manifest = output / "manifest.json"
    commands = [
        [sys.executable, "-W", "error::ResourceWarning", str(directory / "tests/test_trials.py"), str(manifest)],
        ["node", str(directory / "tests/state_reader_fixture.mjs"), str(manifest)],
        ["node", str(directory / "tests/state_reader_fixture.mjs"), str(manifest), "--live"],
        ["node", str(directory / "tests/state_reader_fixture.mjs"), str(manifest), "--unsupported"],
        ["node", str(directory / "tests/state_reader_fixture.mjs"), str(manifest), "--mixed"],
        ["node", "--check", str(directory / "state_reader.js")],
        ["node", "--check", str(directory / "live_driver.js")],
    ]
    stamp = datetime.now(timezone.utc).strftime("%Y%m%d_%H%M%S_%f")
    log_directory = ROOT / "install"
    log_directory.mkdir(exist_ok=True)
    log_path = log_directory / ("cloister_checks_" + stamp + ".log")
    results = []
    with log_path.open("x", encoding="utf-8") as log:
        log.write("Manifest: " + str(manifest) + "\n")
        for command in commands:
            result = subprocess.run(command, cwd=ROOT, capture_output=True, text=True, encoding="utf-8", errors="replace")
            log.write("\nCommand: " + repr(command) + "\n" + result.stdout + result.stderr)
            results.append(dict(command=command, exit_code=result.returncode))
            if result.returncode:
                print("Combined offline validation failed. Log: " + str(log_path))
                print("\n".join((result.stdout + result.stderr).splitlines()[-15:]))
                return result.returncode
    (output / "validation.json").write_text(json.dumps(dict(status="passed", commands=results, log=str(log_path),
        scope="Offline source and mocked event semantics; no native ABI or live gameplay validation")), encoding="utf-8")
    print("Combined offline validation passed: 11 source checks, both six-trial native-profile fixtures and unsupported/mixed-profile refusal.")
    print("Log: " + str(log_path))
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
