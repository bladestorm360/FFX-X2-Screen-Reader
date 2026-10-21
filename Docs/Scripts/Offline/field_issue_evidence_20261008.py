"""Inspect retained Guadosalam callbacks without opening game/tester files."""

# Game-source inputs/exports live outside every Git tree.
from pathlib import Path as _EvidencePath
import sys as _evidence_sys
_evidence_repo = next(p for p in _EvidencePath(__file__).resolve().parents
                      if (p / "Docs/Scripts/Offline/evidence_paths.py").is_file())
_evidence_sys.path.insert(0, str(_evidence_repo / "Docs/Scripts/Offline"))
from evidence_paths import evidence_path, evidence_relative, write_replay_adapter

from pathlib import Path
import json
import re
import sys
sys.path.insert(0,str(root_path := Path(__file__).resolve().parents[3] / 'src/puzzles'))
from atel_model import compile_script
from cloister_inventory import script_chunk, declarations
from field_text import u32

root = Path(__file__).resolve().parents[3]
audit = evidence_path(root / 'Docs/Scripts/Offline/output/FFX/20261003_audit')
maps = json.loads((audit / 'maps_v3.json').read_text(encoding='utf-8'))
targets = json.loads((audit / 'navigation_targets.json').read_text(encoding='utf-8'))
records = []
for target in targets:
    if target['script'] not in ('guad0000', 'kami0000'):
        continue
    event = next(e for e in maps['events'] if e['source'] == target['source'])
    code = bytes.fromhex(event['code'])
    chunk=script_chunk((audit / 'files' / event['source']).read_bytes())
    enriched=[]
    for index,worker in enumerate(event['workers']):
        header=u32(chunk,0x38+index*4)
        enriched.append(dict(worker,index=index,declarations=declarations(chunk,header),
            private_data_offset=u32(chunk,header+0x2c),shared_data_offset=u32(chunk,header+0x30)))
    compiled=compile_script(dict(event,workers=enriched,english_strings=[],area_names=[]))
    compiled_path=root / ('install/player-reports-20261008/'+target['script']+'-compiled.json')
    compiled_path.write_text(json.dumps(compiled,indent=2)+'\n',encoding='utf-8')
    record = {'script': target['script'], 'targets': []}
    for item in target['targets']:
        index = item['worker']
        worker = event['workers'][index]
        record['targets'].append({'target': item, 'worker': worker,
            'entry_prefixes': {str(e): code[worker['entries'][e]:worker['entries'][e]+32].hex()
                               for e in item['entries']}})
    records.append(record)
output = root / 'install/player-reports-20261008/field-callback-evidence.json'
output.write_text(json.dumps(records, indent=2)+'\n', encoding='utf-8')
print('Retained field callback evidence:', evidence_relative(output, root))
rows = []
for index in range(10):
    rows.extend((root / f'src/native/navigation_targets_{index}.inc').read_text(encoding='utf-8').splitlines())
rows = [line for line in rows if re.match(r'\s*\{\d+,', line)]
for first, count in ((1228, 33), (1450, 17)):
    for line in rows[first:first+count]:
        if 'Menu"' in line or 'Conversation with Tidus"' in line or ', 1,' in line:
            print(line.strip())
