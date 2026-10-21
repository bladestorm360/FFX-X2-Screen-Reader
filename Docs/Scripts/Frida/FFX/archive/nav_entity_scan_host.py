"""
Python host for nav_entity_scan.js
Entity scanning + crow-flies navigation with cardinal directions.

Commands:
  s = Scan all entities (list with distance + direction)
  n = Next entity in list
  p = Previous entity
  c = Crow-flies update (recalculate to selected entity)
  f = Flip axis mapping (cycle through 4 interpretations)
  d = Debug (print raw positions)
  q = Quit

Testing workflow:
  1. Press 's' to scan entities
  2. Note direction to an entity (e.g., "Chest — 5 steps north")
  3. Walk toward it using the d-pad
  4. Press 'c' repeatedly — distance should decrease
  5. If direction seems wrong, press 'f' to try a different axis mapping
"""

import frida
import sys
import os

SCRIPT_DIR = os.path.dirname(os.path.abspath(__file__))
JS_PATH = os.path.join(SCRIPT_DIR, "nav_entity_scan.js")

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
    print("  c = Crow-flies update (recalculate distance/direction)")
    print("  f = Flip axis mapping (try different coordinate interpretation)")
    print("  + = Increase scan range (+200 units)")
    print("  - = Decrease scan range (-200 units)")
    print("  d = Debug (print raw positions)")
    print("  q = Quit")
    print()
    print("Test: scan, walk toward entity, press 'c' — distance should decrease.")
    print("If direction is wrong, press 'f' to cycle axis modes.")
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
            elif cmd == 'f':
                script.exports_sync.flip()
            elif cmd == '+':
                script.exports_sync.rangeUp()
            elif cmd == '-':
                script.exports_sync.rangeDown()
            elif cmd == 'd':
                script.exports_sync.debug()
            else:
                print("Unknown. s=scan n=next p=prev c=crow-flies f=flip +/- range d=debug q=quit")
    except (KeyboardInterrupt, EOFError):
        pass
    finally:
        print("Detaching...")
        session.detach()

if __name__ == '__main__':
    main()
