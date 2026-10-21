"""Match original trial controls, displayed GFX and original FP glyph geometry.

Uses only the explicitly authorized, retained Zanarkand payload and existing
ATEL evidence. No game/archive access. Original PPP relocation/model layouts:
72A8E0/712080/7121D0; pppColor75D2E0 and original GT model counts/pointers.
The existing bounded ATEL trace simulates SDK calls, not the graphics backend.
"""

# Game-source inputs/exports live outside every Git tree.
from pathlib import Path as _EvidencePath
import sys as _evidence_sys
_evidence_repo = next(p for p in _EvidencePath(__file__).resolve().parents
                      if (p / "Docs/Scripts/Offline/evidence_paths.py").is_file())
_evidence_sys.path.insert(0, str(_evidence_repo / "Docs/Scripts/Offline"))
from evidence_paths import evidence_path, evidence_relative, write_replay_adapter

import hashlib
import json
from pathlib import Path
import struct
import sys

ROOT=Path(__file__).resolve().parents[3]
sys.path.insert(0,str(ROOT/'src/puzzles'))
from build_utilities import EVIDENCE, DESIGNS, TextEvidence
from compile_trials import enrich
from tile_paths import trace

OUT=evidence_path(ROOT/'Docs/Scripts/Offline/output/help_arrows_zanarkand_20261015')
asset=OUT/'zanarkand-assets/mapout.vpa'
data=asset.read_bytes()
assert hashlib.sha256(data).hexdigest()==next(r['sha256'] for r in
    json.loads((OUT/'zanarkand-assets-manifest.json').read_text())['assets'] if r['file']=='mapout.vpa')

def words(offset,count):
    assert offset>=0 and offset+count*4<=len(data)
    return struct.unpack_from('<%dI'%count,data,offset)

def short(offset): return struct.unpack_from('<H',data,offset)[0]
fp=words(0x38,1)[0]
assert data[:4]==b'MAP1' and short(fp+4)==108 and short(fp+6)==108
programs=fp+words(fp+0x10,1)[0]
models=fp+words(fp+0x14,1)[0]
assert short(fp+8)==120

def program(gfx):
    matches=[fp+32+i*80 for i in range(108) if short(fp+32+i*80+0x44)==gfx]
    assert len(matches)==1
    rec=matches[0];index=words(rec+0x34,1)[0];assert index<108
    start=fp+words(programs+index*4,1)[0]
    node=start+16;commands=[];seen=set()
    for _ in range(32):
        assert node not in seen;seen.add(node)
        count=short(node+0x26);assert count<50
        for i in range(count): commands.append(words(node+0x28+i*16,4))
        next_=words(node,1)[0]
        if not next_:break
        node=start+next_
    else:raise ValueError('PPP chain bound')
    model_ids=[words(start+c[2]+4,1)[0] for c in commands if c[0]==80]
    colors={struct.unpack_from('<4h',data,start+c[2]+8)[:3] for c in commands if c[0]==11}
    assert len(model_ids)==2 and len(colors)==1
    return model_ids,tuple(v//128 for v in colors.pop())

def geometry(index):
    assert index<120
    row=words(models+index*32,8);n,triangles=row[0],row[2]
    blob=fp+row[7]
    # Cross-check the parsed FP descriptor against the model's own header.
    assert words(blob,4)==(5,32,row[5]-row[7],row[6]-row[7])
    assert short(blob+16)==triangles and short(blob+18)==n
    assert 0<n<=32 and 0<triangles<=32
    vertices=list(struct.iter_unpack('<3h',data[fp+row[5]:fp+row[5]+n*6]))
    faces=[struct.unpack_from('<3H',data,blob+0x3c+i*32) for i in range(triangles)]
    assert all(max(t)<n and len(set(t))==3 for t in faces)
    return vertices,faces

def cross(a,b,c):return (b[0]-a[0])*(c[1]-a[1])-(b[1]-a[1])*(c[0]-a[0])
def inside(q,vertices,face):
    a,b,c=[(vertices[i][0]*2,vertices[i][2]*2) for i in face]
    d=[cross(a,b,q),cross(b,c,q),cross(c,a,q)]
    return all(x>=0 for x in d) or all(x<=0 for x in d)

def cells(vertices,faces):
    xs=sorted({v[0] for v in vertices});zs=sorted({v[2] for v in vertices})
    assert len({v[1] for v in vertices})==1
    assert all(b-a==100 for a,b in zip(xs,xs[1:])) and all(b-a==100 for a,b in zip(zs,zs[1:]))
    result=set()
    for y,(z0,z1) in enumerate(zip(zs,zs[1:])):
        for x,(x0,x1) in enumerate(zip(xs,xs[1:])):
            if any(inside((x0+x1,z0+z1),vertices,face) for face in faces):result.add((x,y))
    # Triangle area must equal the complete, gap-free set of unit squares.
    area=sum(abs(cross(*[(vertices[i][0],vertices[i][2]) for i in face])) for face in faces)
    assert area==len(result)*20000
    return result

def normalize(c):
    mx=min(x for x,y in c);my=min(y for x,y in c)
    return frozenset((x-mx,y-my) for x,y in c)
def rotations(c):
    result=[]
    for _ in range(4):result.append(normalize(c));c={(y,-x) for x,y in c}
    return result
SHAPES={
 'T, four squares':{(0,0),(1,0),(2,0),(1,1)},
 'straight line, four squares':{(0,0),(0,1),(0,2),(0,3)},
 'zigzag, four squares':{(1,0),(0,1),(1,1),(0,2)},
 'mirrored zigzag, four squares':{(1,0),(2,0),(0,1),(1,1)},
 'L, four squares':{(0,0),(0,1),(0,2),(1,2)},
 'corner, three squares':{(0,0),(0,1),(1,1)},
 'square, four squares':{(0,0),(1,0),(0,1),(1,1)}
}
COLORS={(255,131,78):'orange',(255,216,87):'yellow',(0,197,197):'cyan',
        (48,11,222):'blue',(221,64,64):'red',(255,255,255):'white'}

flow=json.loads((DESIGNS/'zanarkand_flow.json').read_text())
maps=json.loads((EVIDENCE/'maps_v3.json').read_text())
event=enrich(next(e for e in maps['events'] if e['script']=='dome0700'),EVIDENCE,TextEvidence(EVIDENCE/'text'))
assert event['sha256']==flow['sha256']=='a530da97346bc9d9bee90ab5239663d283d128d13203fe3e55bc4e308c1d29c6'
glyphs=[];pictures=[]
for worker in range(57,84):
    gfx={int(s['args'][0]) for b in flow['workers'][worker]['blocks'] for s in b['statements']
         if s['kind']=='call' and s['id']==0x8003}
    assert len(gfx)==1;gfx=gfx.pop();model_ids,rgb=program(gfx)
    vertices,faces=geometry(model_ids[0]);mask=cells(vertices,faces)
    vertices2,faces2=geometry(model_ids[1]);assert mask==cells(vertices2,faces2)
    matches=[name for name,candidate in SHAPES.items() if normalize(mask) in rotations(candidate)]
    assert len(matches)==1,(worker,mask,matches)
    shape=matches[0];label=f'Floor panel {worker-55}: {COLORS[rgb]} {shape}'
    glyphs.append(dict(worker=worker,gfx=gfx,models=model_ids,rgb=rgb,shape=shape,
                       cells=sorted(mask),label=label))
    pictures.append((label,mask,rgb))

gfx_to_worker={r['gfx']:r['worker'] for r in glyphs};assert len(gfx_to_worker)==27
sequences=[]
for worker,stages in [(53,[0]),(54,[1,2,3,4])]:
    for stage in stages:
        observed=trace(event,worker,stage)
        ids=[c['args'][0] for c in observed['calls'] if c['id']==0x8003]
        assert ids and len(ids)<=9 and all(i in gfx_to_worker for i in ids)
        sequences.append(dict(worker=worker,stage=stage,entryPc=flow['workers'][worker]['entries'][2],
            endPc=observed['endPc'],gfx=ids,workers=[gfx_to_worker[i] for i in ids]))
for stage in [0,5,6]:
    assert not any(c['id']==0x8003 for c in trace(event,54,stage)['calls'])

report=dict(source_sha256=hashlib.sha256(data).hexdigest(),script_sha256=flow['sha256'],
            glyphs=glyphs,sequences=sequences,
            limits='Original PPP geometry and color; original ATEL replay uses existing explicit SDK stubs. Display camera/graphics/backend not run. Completion observer must reject interruption, wrong final PC, replaced allocation and changed stage.')
(OUT/'zanarkand-glyph-evidence.json').write_text(json.dumps(report,indent=2)+'\n')
# Human-readable metadata is the only generated input consumed by source tools.
(ROOT/'src/puzzles/zanarkand_glyphs.json').write_text(json.dumps(dict(glyphs=[dict(worker=g['worker'],label=g['label']) for g in glyphs],sequences=sequences),indent=2)+'\n')
rows=['// Generated from verified Zanarkand ATEL and FP glyph geometry.','#pragma once','#include <array>','namespace ffx {',
      'struct ZanarkandGlyph {unsigned worker;const wchar_t* label;};','inline constexpr ZanarkandGlyph zanarkandGlyphs[]={']
rows += ['    {%d,L%s},'%(g['worker'],json.dumps(g['label'])) for g in glyphs]
rows += ['};','struct ZanarkandSequence {unsigned worker,stage,entryPc,endPc,count;std::array<unsigned,9> glyphs;};',
         'inline constexpr ZanarkandSequence zanarkandSequences[]={']
rows += ['    {%d,%d,%d,%d,%d,{%s}},'%(s['worker'],s['stage'],s['entryPc'],s['endPc'],len(s['workers']),','.join(map(str,s['workers']))) for s in sequences]
rows += ['};','}']
(ROOT/'src/native/zanarkand_glyph_data.hpp').write_text('\n'.join(rows)+'\n')

from PIL import Image,ImageDraw
sheet=Image.new('RGB',(6*220,5*174),(30,30,30));draw=ImageDraw.Draw(sheet)
for i,(label,mask,rgb) in enumerate(pictures):
    x,y=i%6*220,i//6*174
    draw.text((x+5,y+4),'Panel '+str(glyphs[i]['worker']-55),fill='white')
    draw.text((x+5,y+19),label.split(': ')[1],fill='white')
    for a,b in mask:draw.rectangle((x+12+a*28,y+42+b*28,x+38+a*28,y+68+b*28),fill=rgb,outline='white')
sheet.save(OUT/'zanarkand-glyph-geometry.png')
print('PASS: 27 original floor worker/GFX/paired model/shape/color matches; five original display sequences (5/6/7/9/7 glyphs), invalid main-display stages empty. Source metadata/header generated; no game code run.')
