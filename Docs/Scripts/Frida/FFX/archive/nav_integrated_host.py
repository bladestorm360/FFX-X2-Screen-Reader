"""
Python host for nav_integrated.js
Unified navigation: entity scan + crow-flies + A* pathfinding.

Commands:
  s = Scan nearby entities
  n = Next entity
  p = Previous entity
  c = Crow-flies (straight-line distance + direction)
  g = Go — pathfind with turn-by-turn directions
  r = Repeat last announcement
  d = Debug (triangles, collision info)
  + = Increase scan range
  - = Decrease scan range
  q = Quit
"""

import frida
import sys
import os

SCRIPT_DIR = os.path.dirname(os.path.abspath(__file__))
JS_PATH = os.path.join(SCRIPT_DIR, "nav_integrated.js")

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
    print("=== FFX Navigation ===")
    print("  s = Scan entities")
    print("  n/p = Next / Previous")
    print("  c = Crow-flies (distance + direction)")
    print("  g = Go (A* pathfind, turn-by-turn)")
    print("  r = Repeat last")
    print("  d = Debug info")
    print("  +/- = Adjust range")
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
            elif cmd == 'c':
                script.exports_sync.crow()
            elif cmd == 'g':
                script.exports_sync.go()
            elif cmd == 'r':
                script.exports_sync.repeat()
            elif cmd == 'd':
                script.exports_sync.debug()
            elif cmd == '+':
                script.exports_sync.rangeUp()
            elif cmd == '-':
                script.exports_sync.rangeDown()
            else:
                print("s=scan n=next p=prev c=crow g=pathfind r=repeat d=debug +/-=range q=quit")
    except (KeyboardInterrupt, EOFError):
        pass
    finally:
        print("Detaching...")
        session.detach()

if __name__ == '__main__':
    main()
