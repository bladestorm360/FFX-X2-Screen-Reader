"""Event-only Frida validation host for the six trial designs.

The default diagnostic copies state without changing game state or speech.
Explicit --test-controls enables mutable native RE commands only when the
user has authorized game operation. Verbose records remain in local JSONL.
"""

# Game-source inputs/exports live outside every Git tree.
from pathlib import Path as _EvidencePath
import sys as _evidence_sys
_evidence_repo = next(p for p in _EvidencePath(__file__).resolve().parents
                      if (p / "Docs/Scripts/Offline/evidence_paths.py").is_file())
_evidence_sys.path.insert(0, str(_evidence_repo / "Docs/Scripts/Offline"))
from evidence_paths import evidence_path, evidence_relative, write_replay_adapter

import argparse
from datetime import datetime, timezone
import hashlib
import json
from pathlib import Path
import threading

ROOT = Path(__file__).resolve().parents[2]
EXTRACTION = evidence_path(ROOT / "Docs/Scripts/Offline/output/FFX/20261003_audit")
OUTPUT = evidence_path(ROOT / "Docs/Scripts/Frida/output/FFX")


def manifest_path(explicit=None):
    if explicit:
        return Path(explicit).resolve()
    candidates = sorted(EXTRACTION.glob("cloister_designs_*/manifest.json"), reverse=True)
    if not candidates:
        raise FileNotFoundError("Compile all six designs before running this prototype")
    return candidates[0]


def script_source(manifest, test_controls=False):
    source = (Path(__file__).parent / "state_reader.js").read_text(encoding="utf-8")
    marker = "/* CLOISTER_MANIFEST */ {}"
    if source.count(marker) != 1:
        raise ValueError("Invalid prototype manifest marker")
    source = source.replace(marker, json.dumps(manifest, ensure_ascii=True))
    if test_controls:
        source += "\n" + (Path(__file__).parent / "live_driver.js").read_text(encoding="utf-8")
    return source


def describe_state(state, manifest):
    if not state:
        return "No current trial snapshot. Examine a puzzle object to produce a native event."
    title = next((t["name"] for t in manifest["trials"] if t["script"] == state["script"]), state["script"])
    lines = [f"{title}; generation {state['generation']}; event {state['sequence']}."]
    for role, value in state["values"].items():
        if isinstance(value, list):
            lines.append(f"{role}: {len(value)} native array elements; full values are in the log.")
        else:
            lines.append(f"{role}: {value}")
    if state["missing"]:
        lines.append("Unresolved fields: " + ", ".join(state["missing"]))
    lines.append("These are source state bindings; movement and live puzzle semantics still require validation.")
    return "\n".join(lines)


def main():
    import frida  # Deliberately imported only by the user-run entry point.
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--manifest", type=Path)
    parser.add_argument("--test-controls", action="store_true", help="Explicit temporary RE testing; autosaves possible")
    args = parser.parse_args()
    path = manifest_path(args.manifest)
    manifest = json.loads(path.read_text(encoding="utf-8"))
    if manifest.get("schema") != 1 or len(manifest.get("trials", [])) != 6:
        raise ValueError("A complete six-trial manifest is required")
    OUTPUT.mkdir(parents=True, exist_ok=True)
    stamp = datetime.now(timezone.utc).strftime("%Y%m%d_%H%M%S_%f")
    log_path = OUTPUT / ("cloister_trials_" + stamp + ".jsonl")
    lock = threading.Lock()
    session = None
    script = None
    detached = threading.Event()
    startup = threading.Event()
    ready = threading.Event()
    with log_path.open("x", encoding="utf-8") as log:
        log.write(json.dumps(dict(type="host", utc=datetime.now(timezone.utc).isoformat(),
                                  manifest=str(path), manifest_sha256=hashlib.sha256(path.read_bytes()).hexdigest(),
                                  frida=frida.__version__)) + "\n")
        log.flush()

        def message(value, data):
            payload = value.get("payload", value)
            with lock:
                log.write(json.dumps(dict(utc=datetime.now(timezone.utc).isoformat(), message=payload), ensure_ascii=False) + "\n")
                log.flush()
            if value.get("type") == "error":
                print("Prototype error; details saved to the log.")
                startup.set()
            elif payload.get("type") == "ready":
                ready.set()
                startup.set()
                print("All six trial models ready. Commands: state, sources, quit.")
            elif payload.get("type") in ("test-command-complete", "test-command-error"):
                print("Test command " + str(payload.get("id")) + ": " + payload["type"])

        def on_detached(reason, crash=None):
            detached.set()
            print("Prototype detached: " + str(reason))

        try:
            session = frida.attach("FFX.exe")
            session.on("detached", on_detached)
            script = session.create_script(script_source(manifest, args.test_controls))
            script.on("message", message)
            script.load()
            if not startup.wait(10) or not ready.is_set():
                raise RuntimeError("Prototype startup failed; see " + str(log_path))
            print("Log: " + str(log_path))
            print("This diagnostic records state; it does not produce gameplay speech.")
            while not detached.is_set():
                command = input("Trial prototype> ").strip().lower()
                if command in ("quit", "q", "exit"):
                    break
                if command == "state":
                    print(describe_state(script.exports_sync.state(), manifest))
                elif args.test_controls and command.startswith("test "):
                    action = json.loads(command[5:])
                    print("Queued test command " + str(script.exports_sync.test_command(action)))
                elif args.test_controls and command == "inspect":
                    state = script.exports_sync.test_inspection()
                    if state:
                        counts = {}
                        for worker in state["workers"]:
                            counts[worker["script"]] = counts.get(worker["script"], 0) + 1
                        print("Live workers: " + ", ".join(name + ": " + str(count) for name, count in counts.items()))
                        print("Worker identities/positions are saved to the diagnostic log.")
                    else:
                        print('Queue test {"action":"inspect"} first.')
                elif command == "sources":
                    print("Design manifest: " + str(path))
                    print("Every trial has normal and treasure dependency designs with worker/bytecode references.")
                elif command:
                    print("Commands: state, sources, quit.")
        except (EOFError, KeyboardInterrupt):
            pass
        finally:
            if script is not None and not detached.is_set():
                try:
                    if ready.is_set():
                        script.exports_sync.stop()
                    script.unload()
                except Exception as error:
                    print("Prototype cleanup: " + str(error))
            if session is not None and not detached.is_set():
                session.detach()
    print("Saved validation events: " + str(log_path))


if __name__ == "__main__":
    main()
