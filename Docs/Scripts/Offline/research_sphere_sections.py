"""Preserve public grid reference data for the user-requested offline research.

No game process, game installation, save or reference-tree file is modified.
"""
from pathlib import Path
from datetime import datetime
import urllib.request
import json
import hashlib
import sys

root = Path(__file__).resolve().parents[3]
output = Path(__file__).parent / 'output' / ('sphere_sections_' + datetime.now().strftime('%Y%m%d_%H%M%S_%f'))
output.mkdir()
base = 'https://raw.githubusercontent.com/Grayfox96/FFX-Sphere-Grid-viewer/main/'
names = ['nodes_dat03.csv', 'links_dat03.csv', 'dat11.csv', 'panel.csv'] if '--expert' in sys.argv else ['nodes_dat02.csv', 'links_dat02.csv', 'dat10.csv', 'panel.csv']
files = ['ffx_sphere_grid_viewer/data/data_files/' + name for name in names]
files += ['ffx_sphere_grid_viewer/data/layout.py', 'LICENSE']
sources = []
for filename in files:
    data = urllib.request.urlopen(base + filename, timeout=20).read()
    (output / Path(filename).name).write_bytes(data)
    sources.append(dict(url=base + filename, file=Path(filename).name, sha256=hashlib.sha256(data).hexdigest()))
    print(Path(filename).name, len(data))
(output / 'sources.json').write_text(json.dumps(sources, indent=2))
print(output)
