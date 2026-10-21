"""
Python host for nav_pathfind.js
A* navmesh pathfinding with turn-by-turn cardinal directions.

Commands:
  s = Scan entities
  n = Next entity
  p = Previous entity
  g = Go — pathfind to selected entity (turn-by-turn directions)
  c = Crow-flies to selected entity (straight line)
  d = Debug (show player triangle + target triangle)
  q = Quit

Example output:
  [PATH] Treasure Chest "f002" (12 steps): 5 north, 3 east, 4 northeast
"""

import frida
import sys
import os

SCRIPT_DIR = os.path.dirname(os.path.abspath(__file__))
JS_PATH = os.path.join(SCRIPT_DIR, "nav_pathfind.js")

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
    print("  s = Scan entities")
    print("  n = Next entity")
    print("  p = Previous entity")
    print("  g = Go — pathfind with turn-by-turn directions")
    print("  c = Crow-flies (straight line distance + direction)")
    print("  d = Debug (show triangles)")
    print("  q = Quit")
    print()

    try:
        while True:
            cmd = input("> ").strip().lower()
            if cmd == 'q':
                break
            elif cmd == 's':
                script.exports_sync.scan()
            elif cmd == 'n':
                script.exports_sync.next()
            elif cmd == 'p':
                script.exports_sync.prev()
            elif cmd == 'g':
                script.exports_sync.go()
            elif cmd == 'c':
                script.exports_sync.crow()
            elif cmd == 'd':
                script.exports_sync.debug()
            else:
                print("Unknown. s=scan n=next p=prev g=pathfind c=crow-flies d=debug q=quit")
    except (KeyboardInterrupt, EOFError):
        pass
    finally:
        print("Detaching...")
        session.detach()

if __name__ == '__main__':
    main()
