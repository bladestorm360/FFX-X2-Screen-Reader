"""
Python host for diff_title_state.js
Snapshots game memory and diffs to find cursor state changes.

Workflow:
  1. Attach → script captures game object
  2. ENTER → snapshot A
  3. Press UP or DOWN in the game (one press only)
  4. ENTER → snapshot B, shows diff (changed bytes = cursor state)
  5. Repeat to confirm pattern
"""

import frida
import sys
import os

SCRIPT_DIR = os.path.dirname(os.path.abspath(__file__))
JS_PATH = os.path.join(SCRIPT_DIR, "diff_title_state.js")

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
    print("  s      = status")
    print("  q      = quit")
    print()
    print("Step 1: Press ENTER to take snapshot A")
    print("Step 2: Press ONE arrow key in the game")
    print("Step 3: Press ENTER to diff and see changes")
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
                result = script.exports_sync.snapshot()
                print(result)
    except (KeyboardInterrupt, EOFError):
        pass
    finally:
        print("Detaching...")
        session.detach()

if __name__ == '__main__':
    main()
