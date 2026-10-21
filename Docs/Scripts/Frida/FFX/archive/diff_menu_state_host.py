"""
Python host for diff_menu_state.js
Press ENTER to take memory snapshots and diff them.
Press 's' + ENTER for status. Press 'q' + ENTER to quit.

Workflow:
  1. Attach to FFX.exe, load the JS script
  2. Press ENTER → takes snapshot A
  3. In the game: press up or down on the title screen
  4. Press ENTER → takes snapshot B, shows diff (which bytes changed)
  5. The changed bytes reveal the menu selection index
"""

import frida
import sys
import os
from pathlib import Path

SCRIPT_DIR = os.path.dirname(os.path.abspath(__file__))
JS_PATH = os.path.join(SCRIPT_DIR, "diff_menu_state.js")
REPOSITORY = Path(__file__).resolve().parents[5]
sys.path.insert(0, str(REPOSITORY / "Docs" / "Scripts" / "Offline"))
from evidence_paths import evidence_path

OUTPUT_DIR = str(evidence_path(REPOSITORY / "Docs" / "Scripts" / "Frida" / "output" / "FFX"))

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
    print("  ENTER  = take snapshot / diff")
    print("  s      = show status")
    print("  q      = quit")
    print()
    print("Step 1: Press ENTER to take snapshot A")
    print("Step 2: Press up/down in the game")
    print("Step 3: Press ENTER to take snapshot B and see diff")
    print()

    try:
        while True:
            cmd = input("> ").strip().lower()
            if cmd == 'q':
                break
            elif cmd == 's':
                result = script.exports_sync.status()
                print(result)
            else:
                # ENTER or any other key = take snapshot
                result = script.exports_sync.snapshot()
                print(result)
    except (KeyboardInterrupt, EOFError):
        pass
    finally:
        print("Detaching...")
        session.detach()

if __name__ == '__main__':
    main()
