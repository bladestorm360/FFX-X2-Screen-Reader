"""
Python host for nav_exit_find.js
Hooks EventJump to capture Atel worker state and identify exit trigger mechanism.

Commands:
  s = Show current state (room, worker, position)
  e = Enumerate SgEvent function table (find which functions exist)
  w = Watch — install hooks, then walk through a door/exit
  q = Quit

Walk through any transition with hooks active. The script captures:
- The Atel function ID that triggered the jump
- The worker's script position (bytecode PC)
- Atel register values (may contain trigger coordinates)
- The return address chain
"""

import frida
import sys
import os

SCRIPT_DIR = os.path.dirname(os.path.abspath(__file__))
JS_PATH = os.path.join(SCRIPT_DIR, "nav_exit_find.js")

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
    print("  s = State")
    print("  e = Enumerate SgEvent functions")
    print("  w = Watch (hook transitions)")
    print("  q = Quit")
    print()

    try:
        while True:
            cmd = input("> ").strip().lower()
            if cmd == 'q':
                break
            elif cmd == 's':
                script.exports_sync.state()
            elif cmd == 'e':
                script.exports_sync.enumsg()
            elif cmd == 'w':
                script.exports_sync.watch()
            else:
                print("s=state e=enum w=watch q=quit")
    except (KeyboardInterrupt, EOFError):
        pass
    finally:
        print("Detaching...")
        session.detach()

if __name__ == '__main__':
    main()
