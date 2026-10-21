"""Copy the specifically authorized buff/debuff DLLs and stage offline Ghidra.

2026-10-09 user approval: offline asset analysis. No game entry point or probe.
Uses the retained kernel's actual overlay IDs; never enumerates Tester Logs.
"""

# Game-source inputs/exports live outside every Git tree.
from pathlib import Path as _EvidencePath
import sys as _evidence_sys
_evidence_repo = next(p for p in _EvidencePath(__file__).resolve().parents
                      if (p / "Docs/Scripts/Offline/evidence_paths.py").is_file())
_evidence_sys.path.insert(0, str(_evidence_repo / "Docs/Scripts/Offline"))
from evidence_paths import evidence_path, evidence_relative, write_replay_adapter

from datetime import datetime, timezone
from pathlib import Path
import hashlib
import json
import shutil
import struct
import pefile

ROOT = Path(__file__).resolve().parents[3]
GAME = Path(r'D:\Games\SteamLibrary\steamapps\common\FINAL FANTASY FFX&FFX-2 HD Remaster')
stamp = datetime.now(timezone.utc).strftime('%Y%m%d_%H%M%S_%f')
out = evidence_path(ROOT / 'Docs/Scripts/Offline/output') / ('status_animations_' + stamp)
analysis = evidence_path(ROOT / 'Docs/Scripts/Ghidra/output/FFX') / ('status_animations_' + stamp)
out.mkdir(); analysis.mkdir()
kernel = evidence_path(ROOT / 'Docs/Scripts/Offline/output/wakka_labels_20261008_215730/command.bin')
blob = kernel.read_bytes()
records = struct.unpack_from('<H', blob, 10)[0]
# The kernel includes one sentinel record before its English string pool.
pool = 20 + (records + 1) * 96
assert blob[pool + struct.unpack_from('<H', blob, 20 + 0x1a*96)[0]:][:6] == bytes.fromhex('527774748100')
selected = [0x301a,0x301b,0x301c,0x301d,0x301e,0x301f,0x3036,0x3038,0x303a,0x303b,0x304d,0x3010,0x3008]
commands=[]; overlays=set()
for command in selected:
    record=blob[20+(command&0xfff)*96:20+((command&0xfff)+1)*96]
    effects=struct.unpack_from('<HH',record,16); overlays.update(effects)
    commands.append(dict(command=hex(command),effects=list(effects),buff_mask=struct.unpack_from('<H',record,86)[0]))
manifest=[]
batch=["$ErrorActionPreference = 'Stop'", "$env:JAVA_HOME = 'C:/Program Files/Java/jdk-21'",
       f"[void][IO.Directory]::CreateDirectory('{analysis / 'project'}')"]
for number in sorted(overlays):
    name=f'magic_{number:04d}.dll'; source=GAME/'magicFiles/FFX'/name; target=out/name
    shutil.copyfile(source,target)
    binary=target.read_bytes(); pe=pefile.PE(data=binary); base=pe.OPTIONAL_HEADER.ImageBase
    getter=next(e.address for e in pe.DIRECTORY_ENTRY_EXPORT.symbols if e.name==b'GetEffectOverlayTable')
    code=pe.get_data(getter,32); pos=code.index(b'\xb8'); table=struct.unpack_from('<I',code,pos+1)[0]-base
    callbacks=struct.unpack('<5I',pe.get_data(table,20))
    assert all(base<=value<base+pe.OPTIONAL_HEADER.SizeOfImage for value in callbacks)
    manifest.append(dict(name=name,source=str(source),sha256=hashlib.sha256(binary).hexdigest(),getter=hex(getter),table=hex(table),callbacks=[hex(v-base) for v in callbacks]))
    entries=' '.join(f'{v:x}' for v in sorted(set(callbacks)))
    batch.append(f"& 'D:/Games/Dev/ghidra_12.0.3_PUBLIC/support/analyzeHeadless.bat' '{analysis / 'project'}' StatusAnimations -import '{target}' -scriptPath '{ROOT / 'Docs/Scripts/Ghidra/FFX'}' -postScript status_animation_module.java '{analysis}' {entries}")
    batch.append("if ($LASTEXITCODE -ne 0) { throw 'Offline analysis failed: "+name+"' }")
(out/'manifest.json').write_text(json.dumps(dict(kernel=str(evidence_relative(kernel, ROOT)),kernel_sha256=hashlib.sha256(blob).hexdigest(),commands=commands,modules=manifest,analysis=str(evidence_relative(analysis, ROOT))),indent=2)+'\n')
(analysis/'analyze.ps1').write_text('\n'.join(batch)+'\n')
print('Copied',len(manifest),'authorized buff/debuff overlays; no DLL executed.')
print('Analysis script:',analysis/'analyze.ps1')
