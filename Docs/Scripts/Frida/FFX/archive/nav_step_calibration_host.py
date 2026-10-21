"""
Python host for nav_step_calibration.js
Measures game units per walking step for distance reporting.

Commands:
  r = Start/stop recording (walk between presses)
  c <number> = Enter step count after recording (e.g., "c 10")
  p = Print current position
  q = Quit
"""

import frida
import sys
import os

SCRIPT_DIR = os.path.dirname(os.path.abspath(__file__))
JS_PATH = os.path.join(SCRIPT_DIR, "nav_step_calibration.js")

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
    print("  r        = Start/stop recording")
    print("  c <num>  = Enter step count (e.g., 'c 10' after walking 10 steps)")
    print("  p        = Print current position")
    print("  q        = Quit")
    print()
    print("Workflow: r -> walk straight -> r -> c <steps>")
    print()

    try:
        while True:
            cmd = input("> ").strip().lower()
            if cmd == 'q':
                break
            elif cmd == 'r':
                script.exports_sync.record()
            elif cmd.startswith('c '):
                steps = cmd[2:].strip()
                script.exports_sync.calc(steps)
            elif cmd == 'p':
                script.exports_sync.pos()
            else:
                print("Unknown command. r=record, c <num>=calibrate, p=position, q=quit")
    except (KeyboardInterrupt, EOFError):
        pass
    finally:
        print("Detaching...")
        session.detach()

if __name__ == '__main__':
    main()
