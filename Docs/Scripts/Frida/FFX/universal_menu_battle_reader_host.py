"""
Python host for universal_menu_battle_reader.js
Battle menus, field menu, config, items, sphere grid, shops, and hotkeys.

Commands:
  t = Announce whose turn it is
  h = Health check (active character HP/MP)
  n = Read help/description text (works in ALL menus)
  s = Sphere Grid status dump
  q = Quit
"""

import frida
import sys
import os

SCRIPT_DIR = os.path.dirname(os.path.abspath(__file__))
JS_PATH = os.path.join(SCRIPT_DIR, "universal_menu_battle_reader.js")

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
    print("  t = Whose turn")
    print("  h = Health check (HP/MP)")
    print("  n = Read help/description (all menus)")
    print("  s = Sphere Grid status")
    print("  q = Quit")
    print()

    try:
        while True:
            cmd = input("> ").strip().lower()
            if cmd == 'q':
                break
            elif cmd in ('t', 'h', 'n', 's'):
                script.post({'type': 'hotkey', 'key': cmd})
            else:
                print("Unknown. t=turn, h=health, n=description, s=sphere grid, q=quit")
    except (KeyboardInterrupt, EOFError):
        pass
    finally:
        print("Detaching...")
        session.detach()

if __name__ == '__main__':
    main()
