"""Retained original Gagazet predicates, typed bindings, graphics and x86 operand reader."""

# Game-source inputs/exports live outside every Git tree.
from pathlib import Path as _EvidencePath
import sys as _evidence_sys
_evidence_repo = next(p for p in _EvidencePath(__file__).resolve().parents
                      if (p / "Docs/Scripts/Offline/evidence_paths.py").is_file())
_evidence_sys.path.insert(0, str(_evidence_repo / "Docs/Scripts/Offline"))
from evidence_paths import evidence_path, evidence_relative, write_replay_adapter

from pathlib import Path
import hashlib,json,struct,sys
import capstone as cs
from capstone.x86 import X86_OP_IMM,X86_OP_MEM
root=Path(__file__).resolve().parents[3]
sys.path.insert(0,str(root/'Docs/Scripts/Offline'))
from cloister_inventory import script_chunk
base=evidence_path(root/'Docs/Scripts/Offline/output/gagazet_cave_20261016')
assets=evidence_path(root/'Docs/Scripts/Offline/output/gagazet_cave_assets_20261016')
out=evidence_path(root/'Docs/Scripts/Offline/output/gagazet_fixture_20261016');out.mkdir(exist_ok=True)
flow=json.loads((base/'mtgz0700.json').read_text(encoding='utf-8'))
manifest=json.loads((assets/'manifest.json').read_text(encoding='utf-8'))
event=next(r for r in manifest['assets'] if r['source'].endswith('mtgz0700.ebp'))
data=(assets/event['file']).read_bytes();assert hashlib.sha256(data).hexdigest()==flow['sha256']==event['sha256']
chunk=script_chunk(data);code=chunk[struct.unpack_from('<I',chunk,0x30)[0]:];length=struct.unpack_from('<I',chunk)[0];code=code[:length]
assert length==15853 and len(flow['workers'])==42
stage=root/'install/native-tests';stage.mkdir(exist_ok=True,parents=True);(stage/'mtgz0700.bin').write_bytes(code)
bindings=flow['workers'][22]['bindings']
rows=['// Original Gagazet W22 declarations and worker entries. Fixture only.','#pragma once',
    'inline constexpr unsigned gagazetDeclarations[][2]={'+','.join('{0x%x,0x%x}'%(b['raw_low'],b['raw_high']) for b in bindings)+'};',
    'inline constexpr unsigned gagazetEntries[]={'+','.join(map(str,flow['workers'][22]['entries']))+'};']
native=evidence_path(root/'Docs/Scripts/Offline/output/reader_reference_20261003_235502/block_00401000.bin');native_bytes=native.read_bytes()
dis=cs.Cs(cs.CS_ARCH_X86,cs.CS_MODE_32);dis.detail=True
rows+=['struct GagazetFixup {unsigned offset,va;bool relative;};']
functions=[]
for name,va,size in [('gagazetInteger',0x86c6c0,120),('gagazetOperand',0x86e3d0,123)]:
    body=native_bytes[va-0x401000:va-0x401000+size];instructions=list(dis.disasm(body,va));fix=[]
    assert sum(i.size for i in instructions)==size and instructions[-1].mnemonic=='ret'
    for i in instructions:
        for o in i.operands:
            relative=False
            if o.type==X86_OP_IMM:
                value=o.imm&0xffffffff;offset=i.imm_offset;width=i.imm_size
                if i.group(cs.CS_GRP_CALL) or i.group(cs.CS_GRP_JUMP):
                    if va<=value<va+size:continue
                    relative=True
            elif o.type==X86_OP_MEM:value=o.mem.disp&0xffffffff;offset=i.disp_offset;width=i.disp_size
            else:continue
            if 0x400000<=value<0x2800000:
                assert width==4;fix.append((i.address-va+offset,value,relative))
    rows+=['inline constexpr unsigned char '+name+'Body[]={'+','.join(map(str,body))+'};',
        'inline constexpr GagazetFixup '+name+'Fixups[]={'+','.join('{%d,0x%x,%s}'%(a,b,str(c).lower()) for a,b,c in fix)+'};']
    if name=='gagazetInteger':
        jumps=struct.unpack_from('<7I',native_bytes,0x86c738-0x401000)
        assert all(va<=j<va+size for j in jumps)
        rows+=['inline constexpr unsigned gagazetIntegerJumps[]={'+','.join(str(j-va) for j in jumps)+'};']
    (out/(name+'.txt')).write_text('\n'.join('%08x %s %s'%(i.address,i.mnemonic,i.op_str) for i in instructions),encoding='utf-8')
    functions.append(dict(name=name,va=va,size=size,sha256=hashlib.sha256(body).hexdigest(),fixups=fix))
write_replay_adapter(root/'src/native/tests/gagazet_bytes.inc', '\n'.join(rows)+'\n', encoding='utf-8')
asset=next(r for r in manifest['assets'] if r['source'].endswith('mapout.vpa'));data=(assets/asset['file']).read_bytes();assert hashlib.sha256(data).hexdigest()==asset['sha256']
u=lambda o:struct.unpack_from('<I',data,o)[0]
h=lambda o:struct.unpack_from('<H',data,o)[0]
fp=u(0x38);programs=fp+u(fp+0x10);graphics=[]
for index,choice,expected in [(18,0,(100,90,200)),(17,1,(100,200,100)),(16,2,(200,90,100))]:
    rec=fp+32+index*80;pos=struct.unpack_from('<3f',data,rec);start=fp+u(programs+u(rec+0x34)*4);node=start+16;colors=[]
    for n in range(h(node+0x26)):
        c=struct.unpack_from('<4I',data,node+0x28+n*16)
        if c[0]==11:colors.append(tuple(v//128 for v in struct.unpack_from('<4h',data,start+c[2]+8)[:3]))
    assert expected in colors
    graphics.append(dict(record=index,choice=choice,position=pos,colors=colors))
statements=[s for b in flow['workers'][22]['blocks'] for s in b['statements']]
query={s['pc']:int(s['args'][0]) for s in statements if s.get('id')==0x4c}
for pc,button in [(0x24e2,5),(0x28c7,5),(0x2a7a,5),(0x2962,13),(0x2b1c,15)]:assert query[pc]==button
success=next(s for s in statements if s['pc']==0x2e36)
assert '005e44' in success['condition'] and '== 1' in success['condition'] and '005e48' in success['condition'] and '== 0' in success['condition']
(out/'proof.json').write_text(json.dumps(dict(script=event,original_functions=functions,native_source_sha256=hashlib.sha256(native_bytes).hexdigest(),
    graphics_asset=asset,choice_graphics=graphics,input_queries=query,second_success=success,
    limitations='Native variable-pointer service, field animation/camera/render backend and worker timing are controlled fixture dependencies. Manual alignment cue does not predict projectile travel.'),indent=2),encoding='utf-8')
print('PASS: original Gagazet code/declarations,120/123-byte native integer/operand bodies, input sites and asset panel coordinates/colors retained.')
