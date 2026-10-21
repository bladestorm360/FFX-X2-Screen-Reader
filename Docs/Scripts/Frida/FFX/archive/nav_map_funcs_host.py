"""
Python host for nav_map_funcs.js

Commands:
  t = Read Map function table
  w = Hook zone functions (map:2, map:9, map:11, map:13)
  s = State
  q = Quit
"""

import frida
import sys
import os

SCRIPT_DIR = os.path.dirname(os.path.abspath(__file__))
JS_PATH = os.path.join(SCRIPT_DIR, "nav_map_funcs.js")

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

    print("  t = Map function table")
    print("  w = Hook zone functions")
    print("  s = State")
    print("  q = Quit")

    try:
        while True:
            cmd = input("> ").strip().lower()
            if cmd == 'q': break
            elif cmd == 't': script.exports_sync.table()
            elif cmd == 'w': script.exports_sync.watch()
            elif cmd == 's': script.exports_sync.state()
            else: print("t/w/s/q")
    except (KeyboardInterrupt, EOFError):
        pass
    finally:
        print("Detaching...")
        session.detach()

if __name__ == '__main__':
    main()
