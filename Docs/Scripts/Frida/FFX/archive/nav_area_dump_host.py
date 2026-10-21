"""
Python host for nav_area_dump.js
Reads the full area/trigger zone data from the Atel script chunk.

Commands:
  s = Show chunk layout (all section offsets and sizes)
  a = Dump area data (hex + float scan + structure detection)
  m = Dump MapEntrance data with all fields
  q = Quit

Run on different maps to compare area data structure.
"""

import frida
import sys
import os

SCRIPT_DIR = os.path.dirname(os.path.abspath(__file__))
JS_PATH = os.path.join(SCRIPT_DIR, "nav_area_dump.js")

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
    print("  s = Chunk layout")
    print("  a = Area data dump")
    print("  m = MapEntrance dump")
    print("  q = Quit")
    print()

    try:
        while True:
            cmd = input("> ").strip().lower()
            if cmd == 'q':
                break
            elif cmd == 's':
                script.exports_sync.layout()
            elif cmd == 'a':
                script.exports_sync.area()
            elif cmd == 'm':
                script.exports_sync.entrances()
            else:
                print("s=layout a=area m=entrances q=quit")
    except (KeyboardInterrupt, EOFError):
        pass
    finally:
        print("Detaching...")
        session.detach()

if __name__ == '__main__':
    main()
