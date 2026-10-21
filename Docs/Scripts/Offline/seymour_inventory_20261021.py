"""Approved read-only Seymour/Mortiphasm investigation; originals outside Git.

ATEL/chunk layout and declarations: Karifean/FFXDataParser, reused through
the project's existing bounded parser/compiler. No runtime or game writes.
"""
import hashlib
import json
from pathlib import Path
import sys
import itertools
from vbf import Vbf, output_path
from evidence_paths import evidence_root, outside_git
from extract_maps import u16, u32, array, string_at
from cloister_inventory import declarations
from field_text import TextEvidence

ROOT=Path(__file__).resolve().parents[3]
sys.path.insert(0,str(ROOT/'src/puzzles'))
import atel_model
from atel_model import compile_script, render, instructions
# Java's accessor subtype constructs its input list dynamically; the exported
# generic TSV has zero counts. Qualify these exact accessors for this report.
base_signatures=atel_model.signatures
def signatures():
    calls,names=base_signatures()
    for call,count in {0x700f:2,0x7018:3,0x70aa:1,0x70ab:2,0x701a:2}.items():
        calls[call]=dict(calls[call],arg_count_kari=str(count),agree='no')
    return calls,names
atel_model.signatures=signatures

def wheel_matrix(script,out):
    """Execute original m131's count/affinity entry; model only its12 opcodes.

    The native property setter is a recorded boundary. This is original ATEL
    control flow/data, not a replacement battle simulation or graphics replay.
    """
    w=script['workers'][1];code=instructions(bytes.fromhex(script['code']))
    result=bytearray()
    for positions in itertools.product(range(4),repeat=4):
        values={};stack=[];y=0;wheel=None;colors=[None]*4;pc=w['entries'][2]
        for _ in range(1000):
            op,a,nxt=code[pc]
            if op==0xae:stack.append(a if a<32768 else a-65536)
            elif op==0x9f:
                d=w['declarations'][a]
                if d['reference_location']==1 and d['offset'] in (4,8,12,16):
                    wheel=d['offset']//4-1;stack.append(positions[wheel])
                else:stack.append(values.get(a,0))
            elif op==0xa0:
                v=stack.pop()
                if a in (7,8,9,10) and v==values.get(a,0)+1:
                    element={7:0,8:3,9:1,10:2}[a]
                    assert colors[wheel] in (None,element)
                    colors[wheel]=element
                values[a]=v
            elif op in (6,0x14):
                b=stack.pop();c=stack.pop();stack.append(int(c==b) if op==6 else c+b)
            elif op==0x29:stack.append(y)
            elif op==0x2c:y=stack.pop()
            elif op==0xb0:nxt=w['jumps'][a]
            elif op in (0xd6,0xd7):
                truth=bool(stack.pop())
                if truth==(op==0xd6):nxt=w['jumps'][a]
            elif op==0xd8:
                assert a==0x7018
                value=stack.pop();prop=stack.pop();actor=stack.pop()
                assert actor==20 and 98<=prop<=116 and value in (0,1)
            elif op==0x3c:break
            else:raise ValueError(f'Unmodeled original opcode {op:x} at {pc:x}')
            pc=nxt
        else:raise ValueError('Original entry exceeded its bounded instruction budget')
        counts=[values.get(i,0) for i in (7,9,10,8)]
        assert None not in colors and sum(counts)==4
        result.extend(bytes(positions+tuple(colors)+tuple(counts)))
    assert len(result)==3072
    (out/'wheel-matrix.bin').write_bytes(result)
    fnv=14695981039346656037
    for byte in result:fnv=((fnv^byte)*1099511628211)&0xffffffffffffffff
    (out/'wheel-matrix-proof.json').write_text(json.dumps(dict(cases=256,bytes=len(result),
        sha256=hashlib.sha256(result).hexdigest(),fnv64=fnv,source_sha256=script['sha256'],
        entry=w['entries'][2],boundaries='Original ATEL entry; recorded native property setters. Native VM scheduling, visual animation and audio not replayed.'),indent=2)+'\n')
    print('PASS: original m131 entry, all256 wheel combinations; matrix FNV64',fnv)

def main():
    archive=Path('D:/Games/SteamLibrary/steamapps/common/FINAL FANTASY FFX&FFX-2 HD Remaster/data/FFX_Data.vbf')
    out=outside_git(evidence_root()/'offline-output/seymour_20261021')
    vbf=Vbf(archive)
    text=TextEvidence(evidence_root()/'offline-output/FFX/20261003_audit/text')
    names={f'ffx_ps2/ffx/master/jppc/battle/mon/_m{x}/m{x}.bin' for x in ('106','118','131','150','154')}
    selected=[e for e in vbf.entries if e.name in names or '/chr/mon/m106/' in e.name]
    # The verified model has155 small animation/texture records (~4MB).
    assert len(selected)<=170 and sum(e.size for e in selected)<8*1024*1024
    records=[]
    for e in selected:
        raw=vbf.read(e); target=output_path(out/'files',e.name)
        target.parent.mkdir(parents=True,exist_ok=True);target.write_bytes(raw)
        digest=hashlib.sha256(raw).hexdigest()
        records.append(dict(name=e.name,size=len(raw),sha256=digest))
        if e.name not in names: continue
        count=u32(raw,0)-1
        offsets=[u32(raw,4+i*4) for i in range(count+1)]
        chunk=raw[offsets[0]:offsets[1]]
        if target.stem in ('m106','m131'):
            header=chunk[:max(0x80,u32(chunk,0xc)+4)]
            (out/('header-'+target.stem[1:]+'.bin')).write_bytes(header)
            fnv=14695981039346656037
            for byte in header:fnv=((fnv^byte)*1099511628211)&0xffffffffffffffff
            print('Header',target.stem,len(header),'FNV64',fnv)
        english=raw[offsets[6]:offsets[7]]
        print(target.stem,[text.decode(english,0x80+u16(english,i)) for i in (0,4,8,12,16)])
        workers=[]
        for i in range(u16(chunk,0x34)):
            h=u32(chunk,0x38+i*4)
            workers.append(dict(type=u16(chunk,h),entries=array(chunk,u32(chunk,h+0x20),u16(chunk,h+8),'I'),
                jumps=array(chunk,u32(chunk,h+0x24),u16(chunk,h+0xa),'I'),
                ints=array(chunk,u32(chunk,h+0x18),u16(chunk,h+4),'i'),
                floats=array(chunk,u32(chunk,h+0x1c),u16(chunk,h+6),'f'),
                declarations=declarations(chunk,h),private_data_offset=u32(chunk,h+0x2c),
                shared_data_offset=u32(chunk,h+0x30)))
        start,length=u32(chunk,0x30),u32(chunk,0)
        assert start+length<=len(chunk)
        script=dict(script=string_at(chunk,u32(chunk,0xc)),sha256=digest,
            code=chunk[start:start+length].hex(),workers=workers,english_strings=[],area_names=[])
        compiled=compile_script(script)
        (out/(target.stem+'.json')).write_text(json.dumps(dict(script=script,compiled=compiled),indent=2))
        (out/(target.stem+'.txt')).write_text(render(compiled),encoding='utf-8')
        if target.stem=='m131':wheel_matrix(script,out)
        print(target.stem,script['script'],len(workers),'workers',length,'code bytes')
    (out/'manifest.json').write_text(json.dumps(dict(archive=str(archive),archive_size=archive.stat().st_size,
        archive_mtime_ns=archive.stat().st_mtime_ns,records=records),indent=2)+'\n')
    print('PASS:',len(records),'approved records retained outside Git;',sum(x['size'] for x in records),'bytes')

if __name__=='__main__': main()
