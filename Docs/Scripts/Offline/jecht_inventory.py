"""Retain the original Winno ATEL scripts for the authorized Jecht save task.

Uses the existing archive decoder; game files/processes are never accessed.
Layout attribution: Karifean/FFXDataParser and Fahrenheit.
"""
from datetime import datetime, timezone
from pathlib import Path
import hashlib
import json
import sys

ROOT = Path(__file__).resolve().parents[3]
sys.path.insert(0, str(ROOT / 'src/puzzles'))
from atel_model import compile_script
from cloister_inventory import script_chunk, declarations
from field_text import TextEvidence, u32


def main():
    evidence = Path(__file__).parent / 'output/FFX/20261003_audit'
    maps = json.loads((evidence / 'maps_v3.json').read_text(encoding='utf8'))
    output = Path(__file__).parent / 'output' / ('jecht_' + datetime.now(timezone.utc).strftime('%Y%m%d_%H%M%S'))
    output.mkdir()
    text = TextEvidence(evidence / 'text')
    records = []
    for event in maps['events']:
        if not event['script'].startswith('swin') and event['script']!='znkd0400':
            continue
        source = evidence / 'files' / event['source']
        raw = source.read_bytes()
        if hashlib.sha256(raw).hexdigest() != event['sha256']:
            raise ValueError('Archived script hash changed')
        chunk = script_chunk(raw)
        workers = []
        for i, worker in enumerate(event['workers']):
            header = u32(chunk, 0x38 + i * 4)
            workers.append(dict(worker, index=i, declarations=declarations(chunk, header),
                                private_data_offset=u32(chunk, header + 0x2c),
                                shared_data_offset=u32(chunk, header + 0x30)))
        strings = text.field_strings(source)
        script = dict(event, workers=workers, event_data_offset=u32(chunk, 0x20),
                      english_strings=strings, area_names=[text.macro(11,a) for a in event['area_indices']])
        filename = source.stem + '.json'
        (output / filename).write_text(json.dumps(dict(script=script, compiled=compile_script(script),
            english_strings=strings), ensure_ascii=False), encoding='utf8')
        records.append(dict(script=event['script'], source=event['source'], sha256=event['sha256'],
                            file=filename, workers=len(workers), code_length=event['code_length']))
    (output / 'manifest.json').write_text(json.dumps(dict(utc=datetime.now(timezone.utc).isoformat(),
        records=records, status='Original offline control flow; actual gameplay timing unconfirmed')), encoding='utf8')
    print(f'Retained {len(records)} Winno scripts: {output}')


if __name__ == '__main__':
    main()
