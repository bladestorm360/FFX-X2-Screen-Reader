"""
Python host for nav_exit_scan.js
Scans Atel bytecode for all map transition (EventJump) calls.

Commands:
  s = Scan bytecode for all exits
  d = Debug (chunk layout, entrances)
  q = Quit
"""

import frida
import sys
import os

SCRIPT_DIR = os.path.dirname(os.path.abspath(__file__))
JS_PATH = os.path.join(SCRIPT_DIR, "nav_exit_scan.js")

sys.path.insert(0, os.path.dirname(os.path.dirname(SCRIPT_DIR)))
from script_paths import load_script

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

    js_code = load_script(JS_PATH)

    script = session.create_script(js_code)
    script.on('message', on_message)
    script.load()

    print()
    print("  s = Scan bytecode for exits")
    print("  d = Debug info")
    print("  q = Quit")
    print()

    try:
        while True:
            cmd = input("> ").strip().lower()
            if cmd == 'q':
                break
            elif cmd == 's':
                script.exports_sync.scan()
            elif cmd == 'd':
                script.exports_sync.debug()
            else:
                print("s=scan d=debug q=quit")
    except (KeyboardInterrupt, EOFError):
        pass
    finally:
        print("Detaching...")
        session.detach()

if __name__ == '__main__':
    main()
