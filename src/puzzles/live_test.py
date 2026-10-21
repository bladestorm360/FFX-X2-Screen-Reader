"""Run an explicit RE sequence and retain every native result. Autosaves are possible.

Requires the user's authorization to attach to FFX and operate the game.
This is an offline development tool; no test controls enter production.
"""
import argparse
from datetime import datetime, timezone
import json
from pathlib import Path
import threading

from validation_host import OUTPUT, manifest_path, script_source


def main():
    import frida
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("sequence", type=Path)
    args = parser.parse_args()
    scenario = json.loads(args.sequence.read_text(encoding="utf-8"))
    commands = scenario["commands"]
    manifest = json.loads(manifest_path().read_text(encoding="utf-8"))
    stamp = datetime.now(timezone.utc).strftime("%Y%m%d_%H%M%S_%f")
    path = OUTPUT / ("cloister_sequence_" + stamp + ".jsonl")
    ready, completed = threading.Event(), threading.Event()
    expected = len(commands)
    state, errors = {}, []
    lock = threading.Lock()
    with path.open("x", encoding="utf-8") as log:
        log.write(json.dumps(dict(type="scenario", scenario=scenario,
                                 manifest=str(manifest_path()))) + "\n")

        def message(value, _data):
            payload = value.get("payload", value)
            with lock:
                log.write(json.dumps(dict(utc=datetime.now(timezone.utc).isoformat(),
                                         message=payload)) + "\n")
                log.flush()
            if payload.get("type") == "ready":
                ready.set()
            if payload.get("type") == "snapshot":
                state.update(payload["snapshot"])
            if value.get("type") == "error" or payload.get("type") in ("read-error", "test-command-error"):
                errors.append(payload)
            if payload.get("type") == "test-command-error":
                completed.set()
            if payload.get("type") == "test-command-complete" and payload.get("id") == expected:
                completed.set()

        session = frida.attach("FFX.exe")
        script = session.create_script(script_source(manifest, True))
        script.on("message", message)
        try:
            script.load()
            if not ready.wait(10):
                raise RuntimeError("Prototype startup failed")
            script.exports_sync.test_commands(commands)
            seconds = 30 + sum(c.get("delay", 60) for c in commands) / 15
            if not completed.wait(seconds):
                raise RuntimeError("Native sequence did not finish; see " + str(path))
            copied = script.exports_sync.state()
            print("Sequence: " + scenario["name"])
            print("Commands: " + str(expected) + "; diagnostic errors: " + str(len(errors)))
            print("Final copied state: " + json.dumps(copied.get("values") if copied else None))
            print("Evidence: " + str(path))
        finally:
            try:
                script.exports_sync.stop()
                script.unload()
            finally:
                session.detach()
    if errors:
        raise RuntimeError("Sequence produced diagnostic errors; see " + str(path))


if __name__ == "__main__":
    main()
