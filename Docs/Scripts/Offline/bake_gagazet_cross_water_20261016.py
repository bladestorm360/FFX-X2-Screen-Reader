"""Bake exact Gagazet shore crossings from approved original scripts/map data."""

# Game-source inputs/exports live outside every Git tree.
from pathlib import Path as _EvidencePath
import sys as _evidence_sys
_evidence_repo = next(p for p in _EvidencePath(__file__).resolve().parents
                      if (p / "Docs/Scripts/Offline/evidence_paths.py").is_file())
_evidence_sys.path.insert(0, str(_evidence_repo / "Docs/Scripts/Offline"))
from evidence_paths import evidence_path, evidence_relative, write_replay_adapter

from pathlib import Path
import json,struct,math,hashlib,collections
root=Path(__file__).resolve().parents[3]
out=evidence_path(root/'Docs/Scripts/Offline/output/gagazet_cross_water_20261016')
f=json.loads((evidence_path(root/'Docs/Scripts/Offline/output/gagazet_cave_20261016/mtgz0600.json')).read_text())
original=evidence_path(root/'Docs/Scripts/Offline/output/FFX/20261003_audit/files/ffx_ps2/ffx/master/jppc/event/obj/mt/mtgz0600/mtgz0600.ebp')
assert hashlib.sha256(original.read_bytes()).hexdigest()==f['sha256']
b=(out/'mtgz06_mapout.vpa').read_bytes()
manifest=json.loads((out/'manifest.json').read_text())
assert hashlib.sha256(b).hexdigest()==manifest['assets'][0]['sha256']
h=struct.unpack_from('<I',b,24)[0];scale=struct.unpack_from('<f',b,h+12)[0]/10
raw=(out/'mtgz06_0_triangles.bin').read_bytes();rv=(out/'mtgz06_0_vertices.bin').read_bytes()
ts=[struct.unpack_from('<6hI',raw,i) for i in range(0,len(raw),16)]
vs=[tuple(v/scale for v in struct.unpack_from('<3h',rv,i)) for i in range(0,len(rv),8)]
assert len(ts)==1406 and len(vs)==1039
centers=[tuple(sum(vs[j][k] for j in t[:3])/3 for k in range(3)) for t in ts]
def height(i,p):
 a,q,c=[vs[j] for j in ts[i][:3]];d=(q[2]-c[2])*(a[0]-c[0])+(c[0]-q[0])*(a[2]-c[2])
 if abs(d)<1e-7:return None
 u=((q[2]-c[2])*(p[0]-c[0])+(c[0]-q[0])*(p[2]-c[2]))/d
 v=((c[2]-a[2])*(p[0]-c[0])+(a[0]-c[0])*(p[2]-c[2]))/d
 return u*a[1]+v*q[1]+(1-u-v)*c[1] if u>=-1e-6 and v>=-1e-6 and u+v<=1+1e-6 else None
def call(worker,common):
 return next(s for bl in f['workers'][worker]['blocks'] for s in bl['statements'] if s.get('id')==common)
def fnv(data):
 value=2166136261
 for byte in data:value=((value^byte)*16777619)&0xffffffff
 return value
seeds=[(178.5891,88.16030883789062,-351.6188),(232.9568,-198.8135986328125,-130.0165),(569.6378,-127.19363403320312,-950.2076)]
# Plane values come from Common4001 in W0 entries15/16/17; X/Z seed locations
# are the original PPP return entrances1/2/3. Prove each against the EBP below.
offset=struct.unpack_from('<I',original.read_bytes(),4)[0]
chunk=original.read_bytes()[offset:]
start,end=struct.unpack_from('<II',chunk,4)
for i,seed in enumerate(seeds,1):
 xyz=struct.unpack_from('<3f',chunk,start+i*32+12)
 assert abs(xyz[0]-seed[0])<.01 and abs(xyz[2]-seed[2])<.01
rows=[]
for slot,(worker,seed) in enumerate(zip([35,36,37],seeds)):
 edge=call(worker,84);a,c=[tuple(map(float,edge['args'][i:i+3])) for i in [0,3]]
 lo,hi=f['workers'][0]['entries'][15+slot:17+slot]
 planes=[s for bl in f['workers'][0]['blocks'] for s in bl['statements'] if lo<=s['pc']<hi and s.get('id')==0x4001]
 assert len(planes)==1 and abs(float(planes[0]['args'][0])-seed[1])<.001
 def side(p):return (c[0]-a[0])*(p[2]-a[2])-(c[2]-a[2])*(p[0]-a[0])
 sense=1 if side(seed)>0 else -1
 floors=[(height(i,seed),i) for i in range(len(ts)) if height(i,seed) is not None]
 seed_index=min((y,i) for y,i in floors if y>=seed[1]-.1)[1]
 reached={seed_index};pending=[seed_index]
 while pending:
  at=pending.pop()
  for n in ts[at][3:6]:
   if n>=0 and n not in reached and side(centers[n])*sense>=0:reached.add(n);pending.append(n)
 mid=tuple((a[k]+c[k])/2 for k in range(3));dx=c[0]-a[0];dz=c[2]-a[2];length=math.hypot(dx,dz)
 land=(mid[0]+sense*dz/length*2,mid[1],mid[2]-sense*dx/length*2)
 water=(mid[0]-sense*dz/length*2,seed[1],mid[2]+sense*dx/length*2)
 land_y,land_i=min((abs(height(i,land)-land[1]),i) for i in range(len(ts)) if height(i,land) is not None)
 assert land_y<25
 land=(land[0],height(land_i,land),land[2])
 water_i=min((y,i) for i in range(len(ts)) if (y:=height(i,water)) is not None and y>=water[1]-.1)[1]
 # The finite shoreline cuts a native corridor. Include the two straddling
 # landing triangles so a real side-to-side crossing can finish beyond it.
 reached.update([land_i,water_i])
 # Sloped/vertical native connector triangles can straddle the trigger plane.
 # Preserve the original short adjacency chain from the water landing to the
 # scripted return seed, rather than treating triangle-center sidedness as a
 # physical wall. This adds original triangles only, never synthetic edges.
 parents={water_i:None};pending=collections.deque([water_i])
 while pending:
  at=pending.popleft()
  if at==seed_index:break
  for n in ts[at][3:6]:
   if n>=0 and n not in parents:parents[n]=at;pending.append(n)
 connector=[];at=seed_index
 while at is not None:connector.append(at);at=parents[at]
 connector.reverse()
 assert len(connector)<=16 and all(ts[i][6]&127==0 for i in connector)
 reached.update(connector)
 # A request may arrive while the actor still owns a native triangle that
 # straddles the finite shore trigger, even though its center is on land.
 # Include physically adjacent near-height straddlers, not overlapping decks.
 straddlers=[]
 for i,t in enumerate(ts):
  if i in reached or not any(n in reached for n in t[3:6]):continue
  ps=[vs[j] for j in t[:3]];sides=[side(p) for p in ps]
  for j in range(3):
   first,last=sides[j],sides[(j+1)%3]
   if first*last>=0:continue
   u=first/(first-last);p=tuple(ps[j][k]+u*(ps[(j+1)%3][k]-ps[j][k]) for k in range(3))
   along=((p[0]-a[0])*dx+(p[2]-a[2])*dz)/(length*length)
   if 0<=along<=1 and abs(p[1]-(a[1]+along*(c[1]-a[1])))<25:
    straddlers.append(i);break
 reached.update(straddlers)
 exit_edge=call(28+slot,84)
 row=dict(worker=worker,edge_a=a,edge_b=c,seed=seed,seed_triangle=seed_index,land=land,land_triangle=land_i,
          water=water,water_triangle=water_i,shore_connector=connector,shore_straddlers=straddlers,triangles=sorted(reached),exit_edge=exit_edge,plane_call=planes[0])
 rows.append(row)
assert all(not (set(rows[i]['triangles'])&set(rows[j]['triangles'])) for i in range(3) for j in range(i+1,3))
# The script's crossing flags drive its regular W0:14 walk /15..17 swim
# requests. No extra interaction or teleported connection is invented.
for i in range(3):
 ss=[s for bl in f['workers'][22+i]['blocks'] for s in bl['statements']]
 assert any(s['kind']=='request' and s.get('worker')=='0' and s.get('entry')=='14' for s in ss)
 assert any(s['kind']=='request' and s.get('worker')=='0' and s.get('entry')==str(15+i) for s in ss)
proof=dict(schema=1,script_sha256=f['sha256'],map_sha256=hashlib.sha256(b).hexdigest(),
           triangles_fnv=fnv(raw),vertices_fnv=fnv(rv),scale=scale,rows=rows,
           limits='Native geometry and scripted crossings; water-side triangle-center flood classifies only each approach basin, not a movement shortcut. All runtime legs still prove native adjacency, finite shoreline crossing and live collision gates.')
(out/'cross_water_bake.json').write_text(json.dumps(proof,indent=2)+'\n')
lines=['// Generated by bake_gagazet_cross_water_20261016.py; original script/map hashes in its receipt.',
       '#pragma once','#include "navigation.hpp"','#include <cstdint>','namespace ffx::nav {',
       'struct WaterCrossing {Point a,b,land,water;int landTriangle,waterTriangle;const uint16_t* triangles;unsigned count;Point exitA,exitB;};',
       'struct CrossWaterBake {unsigned triangleHash,vertexHash;std::array<WaterCrossing,3> crossings;};']
def point(p):return '{'+','.join(format(v,'.17g') for v in p)+'}'
for i,row in enumerate(rows):lines.append('inline constexpr uint16_t gagazetWater'+str(i)+'[]={'+','.join(map(str,row['triangles']))+'};')
lines.append('inline constexpr CrossWaterBake gagazetCrossWater={0x%08xu,0x%08xu,{{'%(fnv(raw),fnv(rv)))
for i,row in enumerate(rows):
 e=list(map(float,row['exit_edge']['args']))
 lines.append('{'+','.join([point(row['edge_a']),point(row['edge_b']),point(row['land']),point(row['water']),str(row['land_triangle']),str(row['water_triangle']),'gagazetWater'+str(i),str(len(row['triangles'])),point(e[:3]),point(e[3:])])+'},')
lines+=['}}};','}']
(root/'src/native/cross_water_data.hpp').write_text('\n'.join(lines)+'\n')
print('PASS: Gagazet cross-water bake, three native shore transitions, disjoint approach basins',*[len(r['triangles']) for r in rows],'; geometry FNV %08x/%08x'%(fnv(raw),fnv(rv)))
