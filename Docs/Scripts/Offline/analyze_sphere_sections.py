"""Analyze preserved standard-grid records, without opening a game or save."""
from pathlib import Path
from collections import deque
import json
import struct
import sys

source = Path(sys.argv[1])
def rows(name):
    return [bytes(int(x, 16) for x in line.split(',')) for line in (source / name).read_text().splitlines()]
def u16(b, at=0):
    return struct.unpack_from('<H', b, at)[0]
charset = '0123456789 !"#$%&\'()*+,-./:;<=>?ABCDEFGHIJKLMNOPQRSTUVWXYZ[\\]^_‘abcdefghijklmnopqrstuvwxyz{|}~'
panels = rows('panel.csv')
strings = panels.pop()
def name(i):
    text = []
    for b in strings[u16(panels[i]):]:
        if b == 0: break
        if 0 <= b-0x30 < len(charset): text.append(charset[b-0x30])
    return ''.join(text)
nodes = rows('nodes_dat02.csv')
types = [r[0] for r in rows('dat10.csv')]
edges = [(u16(r), u16(r,2)) for r in rows('links_dat02.csv')]
adj = [[] for _ in nodes]
for a,b in edges:
    adj[a].append(b); adj[b].append(a)
locks = {i for i,t in enumerate(types) if u16(panels[t],22) in (0,16,17,18)}
groups = []
seen = set(locks)
for i in range(len(nodes)):
    if i in seen: continue
    group = []; queue = deque([i]); seen.add(i)
    while queue:
        at = queue.popleft(); group.append(at)
        for nxt in adj[at]:
            if nxt not in seen: seen.add(nxt); queue.append(nxt)
    groups.append(sorted(group))
for n,group in enumerate(sorted(groups, key=len, reverse=True)):
    abilities = [(i,name(types[i]),u16(panels[types[i]],18)) for i in group if u16(panels[types[i]],18)]
    ends = [(i,name(types[i])) for i in group if len([v for v in adj[i] if v not in locks]) <= 1]
    boundary = sorted({v for i in group for v in adj[i] if v in locks})
    print(n, 'nodes',len(group),'abilities',abilities,'ends',ends,'locks',boundary)
(source / 'components.json').write_text(json.dumps(dict(nodes=[dict(x=struct.unpack_from('<h',r)[0], y=struct.unpack_from('<h',r,2)[0], type=t, name=name(t), ability=u16(panels[t],18)) for r,t in zip(nodes,types)], edges=edges, locks=sorted(locks), groups=groups),indent=2))
