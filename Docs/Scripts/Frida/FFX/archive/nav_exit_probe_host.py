"""
Python host for nav_exit_probe.js
Discovers map exit trigger locations by hooking transitions and analyzing zone data.

Commands:
  s = Scan current state (room ID, player position)
  z = Dump zone/area data from Atel script chunk
  b = Find navmesh boundary edges (potential exit locations)
  w = Start watching for transitions (hooks fire when you cross an exit)
  c = Check if room changed since last check
  q = Quit

Testing workflow:
  1. Press 'w' to start watching for transitions
  2. Press 's' to see current room ID
  3. Walk around the map edges — when you cross an exit, the hooks log your position
  4. Press 'c' after transitioning to confirm room change
  5. Press 'b' to see navmesh boundaries (edges of walkable area)
  6. Press 'z' to dump the raw zone data for analysis
"""

import frida
import sys
import os

SCRIPT_DIR = os.path.dirname(os.path.abspath(__file__))
JS_PATH = os.path.join(SCRIPT_DIR, "nav_exit_probe.js")

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
    print("=== Exit Trigger Probe ===")
    print("  s = Current state (room, position)")
    print("  z = Dump zone/area data")
    print("  b = Find navmesh boundary edges")
    print("  w = Watch for transitions (hooks)")
    print("  c = Check if room changed")
    print("  q = Quit")
    print()
    print("Workflow: w -> walk to exits -> hooks log your position at transition")
    print()

    try:
        while True:
            cmd = input("> ").strip().lower()
            if cmd == 'q':
                break
            elif cmd == 's':
                script.exports_sync.state()
            elif cmd == 'z':
                script.exports_sync.zone()
            elif cmd == 'b':
                script.exports_sync.boundary()
            elif cmd == 'w':
                script.exports_sync.watch()
            elif cmd == 'c':
                script.exports_sync.check()
            else:
                print("s=state z=zone b=boundary w=watch c=check q=quit")
    except (KeyboardInterrupt, EOFError):
        pass
    finally:
        print("Detaching...")
        session.detach()

if __name__ == '__main__':
    main()
