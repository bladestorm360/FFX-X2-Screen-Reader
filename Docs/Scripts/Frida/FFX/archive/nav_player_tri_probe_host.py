"""
Python host for nav_player_tri_probe.js
Finds player's navmesh triangle and calibrates cardinal directions.

Commands:
  s = Snapshot actor[0] struct (take 2, then diff)
  d = Diff snapshots (find candidate triangle index fields)
  f = Find triangle via point-in-triangle test
  n = North calibration (press, walk screen-up, press again)
  q = Quit
"""

import frida
import sys
import os

SCRIPT_DIR = os.path.dirname(os.path.abspath(__file__))
JS_PATH = os.path.join(SCRIPT_DIR, "nav_player_tri_probe.js")

def on_message(message, data):
    if message['type'] == 'send':
        print(f"[JS] {message['payload']}")
    elif message['type'] == 'error':
        print(f"[ERR] {message['description']}")

def main():
    process_name = "FFX.exe"

    print(f"Attaching to {process_name}...")
    try:
        session = frida.attach(process_name)
    except frida.ProcessNotFoundError:
        print(f"Error: {process_name} not found. Is the game running?")
        sys.exit(1)

    with open(JS_PATH, 'r') as f:
        js_code = f.read()

    script = session.create_script(js_code)
    script.on('message', on_message)
    script.load()

    print()
    print("Commands:")
    print("  s = Snapshot actor struct (take 2 at different positions)")
    print("  d = Diff snapshots (find triangle index candidates)")
    print("  f = Find current triangle (point-in-triangle test)")
    print("  n = North calibration (press, walk screen-up, press again)")
    print("  q = Quit")
    print()

    try:
        while True:
            cmd = input("> ").strip().lower()
            if cmd == 'q':
                break
            elif cmd == 's':
                script.exports_sync.snapshot()
            elif cmd == 'd':
                script.exports_sync.diff()
            elif cmd == 'f':
                script.exports_sync.findtri()
            elif cmd == 'n':
                script.exports_sync.north()
            else:
                print("Unknown command. s=snapshot, d=diff, f=find, n=north, q=quit")
    except (KeyboardInterrupt, EOFError):
        pass
    finally:
        print("Detaching...")
        session.detach()

if __name__ == '__main__':
    main()
