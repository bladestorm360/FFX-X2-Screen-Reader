"""
Python host for field_submenu_discover.js
Discovers scene IDs, TkWindow slots, and text sources for all field sub-menus.

Commands:
  s = Scan all 32 TkWindow pool slots + 8 probe array slots
  t = Dump scene table (25 slots)
  h = Read current HELP text buffer
  q = Quit
"""

import frida
import sys
import os

SCRIPT_DIR = os.path.dirname(os.path.abspath(__file__))
JS_PATH = os.path.join(SCRIPT_DIR, "field_submenu_discover.js")

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
    print("  s = Scan TkWindow pools (to log)")
    print("  t = Dump scene table (to log)")
    print("  h = Read HELP text")
    print("  c = Show hook fire counts")
    print("  q = Quit")
    print()

    try:
        while True:
            cmd = input("> ").strip().lower()
            if cmd == 'q':
                break
            elif cmd in ('s', 't', 'h', 'c'):
                script.post({'type': 'hotkey', 'key': cmd})
            else:
                print("Unknown. s=scan, t=scenes, h=help, c=counts, q=quit")
    except (KeyboardInterrupt, EOFError):
        pass
    finally:
        print("Detaching...")
        session.detach()

if __name__ == '__main__':
    main()
