"""
Python host for nav_zone_dump.js
Dumps and analyzes Atel script chunk area/zone data to find trigger zones.

Commands:
  s = Show current state (room, position, chunk layout)
  z = Dump zone data raw (hex + float/short interpretation)
  a = Scan for trigger zones (bounding box coordinate patterns)
  q = Quit
"""

import frida
import sys
import os

SCRIPT_DIR = os.path.dirname(os.path.abspath(__file__))
JS_PATH = os.path.join(SCRIPT_DIR, "nav_zone_dump.js")

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
    print("  s = Current state")
    print("  z = Dump zone data (raw + interpreted)")
    print("  a = Scan for trigger zones (bounding boxes)")
    print("  q = Quit")
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
            elif cmd == 'a':
                script.exports_sync.scan()
            else:
                print("s=state z=zone a=scan q=quit")
    except (KeyboardInterrupt, EOFError):
        pass
    finally:
        print("Detaching...")
        session.detach()

if __name__ == '__main__':
    main()
