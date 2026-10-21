"""
Python host for nav_trigger_scan.js
Scans memory for objects with positions near MapEntrance locations.

Commands:
  s = Scan
  q = Quit
"""
import frida, sys, os
SCRIPT_DIR = os.path.dirname(os.path.abspath(__file__))
JS_PATH = os.path.join(SCRIPT_DIR, "nav_trigger_scan.js")

def on_message(message, data):
    if message['type'] == 'send': print(f"[JS] {message['payload']}")
    elif message['type'] == 'error': print(f"[ERR] {message['description']}")

def main():
    print("Attaching to FFX.exe...")
    try: session = frida.attach("FFX.exe")
    except frida.ProcessNotFoundError: print("FFX.exe not found"); sys.exit(1)
    with open(JS_PATH, 'r') as f: js_code = f.read()
    script = session.create_script(js_code)
    script.on('message', on_message)
    script.load()
    print("  s = Scan for exit triggers near entrances")
    print("  q = Quit")
    try:
        while True:
            cmd = input("> ").strip().lower()
            if cmd == 'q': break
            elif cmd == 's': script.exports_sync.scan()
    except (KeyboardInterrupt, EOFError): pass
    finally: print("Detaching..."); session.detach()

if __name__ == '__main__': main()
