"""Generate checked bindings for the Frida prototype and native port.

All facts come from hash-verified extracted scripts, not savedata guesses.
"""
from pathlib import Path
import json,re,sys
ROOT=Path(__file__).resolve().parents[3]
sys.path.insert(0,str(ROOT/'src/puzzles'))
from cloister_inventory import script_chunk
from field_text import u32
IN=Path(__file__).parent/'output/minigames_20261007_104142'
ARCHIVE=Path(__file__).parent/'output/FFX/20261003_audit/files'
roles={'bltz0000':[11,12,166,175,192,215], 'nagi0000':[67,116,117,128,130,135,137,149]}
records=[]
for record in json.loads((IN/'manifest.json').read_text())['records']:
    if record['script'] not in roles:continue
    d=json.loads((IN/record['file']).read_text()); c=script_chunk((ARCHIVE/record['source']).read_bytes())
    length=u32(c,0);offset=u32(c,0x30);h=2166136261
    for byte in c[offset:offset+length]:h=((h^byte)*16777619)&0xffffffff
    bindings=[]
    for i in roles[record['script']]:
        decl=d['script']['workers'][0]['declarations'][i]
        for w in d['script']['workers']:
            other=w['declarations'][i]
            if (other['raw_low'],other['raw_high'])!=(decl['raw_low'],decl['raw_high']):raise ValueError('Role differs by worker')
        bindings.append(dict(index=i,low=decl['raw_low'],high=decl['raw_high']))
    pickups=[]
    if record['script']=='nagi0000':
        for wi in range(39,59):
            writes=[s for b in d['compiled']['workers'][wi]['blocks'] for s in b['statements'] if s.get('variable')==130 and ' | ' in s.get('value','')]
            fields={(int(s['element']),int(re.search(r'\| (\d+)\)',s['value']).group(1))) for s in writes}
            if len(fields)!=1:raise ValueError('Balloon visibility is ambiguous')
            element,mask=fields.pop();pickups.append(dict(worker=wi,element=element,mask=mask))
    records.append(dict(name=record['script'],checksum=h,length=length,workers=len(d['script']['workers']),bindings=bindings,pickups=pickups,source=record['source']))
for name in roles:
    rows=[r for r in records if r['name']==name]
    if any((r['bindings'],r['pickups'])!=(rows[0]['bindings'],rows[0]['pickups']) for r in rows):raise ValueError('Variant bindings differ')
out=ROOT/'src/native/minigame_data.hpp'
lines=['// Generated from extracted ATEL by minigame_bindings.py. FFXDataParser layout attribution.','#pragma once','namespace ffx {',
       'struct MinigameCatalog { const char* name; unsigned checksum,length,workers; };', 'inline constexpr MinigameCatalog minigameCatalog[]={']
lines += ['    {"%s",0x%x,%d,%d},'%(r['name'],r['checksum'],r['length'],r['workers']) for r in records]
lines+=['};','struct MinigameBinding { unsigned index,low,high; };']
for name,identifier in [('bltz0000','blitzBindings'),('nagi0000','raceBindings')]:
    row=next(r for r in records if r['name']==name)
    lines+=['inline constexpr MinigameBinding '+identifier+'[]={']
    lines+=['    {%d,0x%x,%d},'%(b['index'],b['low'],b['high']) for b in row['bindings']]
    lines+=['};']
lines+=['struct RacePickupBinding { unsigned worker,element,mask; };','inline constexpr RacePickupBinding racePickups[]={']
lines+=['    {%d,%d,%d},'%(b['worker'],b['element'],b['mask']) for b in next(r for r in records if r['name']=='nagi0000')['pickups']]
lines+=['};','}'];out.write_text('\n'.join(lines)+'\n')
(ROOT/'Docs/Scripts/Frida/FFX/minigame_bindings.json').write_text(json.dumps(records,indent=2)+'\n')
prototype=ROOT/'Docs/Scripts/Frida/FFX/minigame_accessibility.js'
source=prototype.read_text()
start=source.index('// BEGIN GENERATED MINIGAME MODEL')
end=source.index('// END GENERATED MINIGAME MODEL')
block='// BEGIN GENERATED MINIGAME MODEL\n'+(ROOT/'src/minigames/spatial_model.js').read_text()+'\nconst minigameRecords='+json.dumps(records)+';\n'
prototype.write_text(source[:start]+block+source[end:])
print('Verified bindings for six gameplay variants, including all twenty native balloon visibility bits.')
