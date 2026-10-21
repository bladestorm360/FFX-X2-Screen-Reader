"""
Python host for nav_mesh_probe.js
Validates VPA navmesh reading — triangles, vertices, collision groups, neighbour semantics.

Commands:
  m = Navmesh summary (tri count, scale, blocked count, pointer pattern validation)
  t = Dump all triangles to log file
  v = Print first 20 vertices to console
  q = Quit
"""

import frida
import sys
import os

SCRIPT_DIR = os.path.dirname(os.path.abspath(__file__))
JS_PATH = os.path.join(SCRIPT_DIR, "nav_mesh_probe.js")

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
    print("  m = Navmesh summary + pointer pattern validation")
    print("  t = Dump all triangles to log file")
    print("  v = Print first 20 vertices")
    print("  q = Quit")
    print()

    try:
        while True:
            cmd = input("> ").strip().lower()
            if cmd == 'q':
                break
            elif cmd == 'm':
                script.exports_sync.mesh()
            elif cmd == 't':
                script.exports_sync.dump()
            elif cmd == 'v':
                script.exports_sync.verts()
            else:
                print("Unknown command. m=mesh, t=dump, v=verts, q=quit")
    except (KeyboardInterrupt, EOFError):
        pass
    finally:
        print("Detaching...")
        session.detach()

if __name__ == '__main__':
    main()
