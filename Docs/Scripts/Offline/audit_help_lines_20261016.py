"""Bounded audit of retained original Help data/code; no game/install access."""

# Game-source inputs/exports live outside every Git tree.
from pathlib import Path as _EvidencePath
import sys as _evidence_sys
_evidence_repo = next(p for p in _EvidencePath(__file__).resolve().parents
                      if (p / "Docs/Scripts/Offline/evidence_paths.py").is_file())
_evidence_sys.path.insert(0, str(_evidence_repo / "Docs/Scripts/Offline"))
from evidence_paths import evidence_path, evidence_relative, write_replay_adapter

from pathlib import Path
import struct, json, hashlib
import capstone as cs
from capstone.x86 import X86_OP_IMM, X86_OP_MEM
root=Path(__file__).resolve().parents[3]
source=evidence_path(root/'Docs/Scripts/Offline/output/help_escape_20261014/native-help-original')
page=(source/'gnm/now_help_page.sps2').read_bytes()
text=(source/'now_help.sps2').read_bytes()
strings={row['id']:row['text'] for row in json.loads((source/'strings-basic.json').read_text())}
directory=struct.unpack_from('<I',page,8)[0]
entries=[]
for p in range(struct.unpack_from('<I',page,4)[0]):
    start,n=struct.unpack_from('<IH',page,directory+p*8)
    for row in range(n):
        at=start+row*0x138
        words=struct.unpack_from('<28h',page,at)
        if p in (0,8,9,24) or (page[at]==0 and 56<=words[11]<=61):
            entries.append(dict(page=p,row=row,offset=at,words=words,text=strings.get(words[11],'')))
archive=evidence_path(root/'Docs/Scripts/Offline/output/reader_reference_20261003_235502/block_00401000.bin')
data=archive.read_bytes();decoder=cs.Cs(cs.CS_ARCH_X86,cs.CS_MODE_32)
body=data[0x76f960-0x401000:0x76f960-0x401000+2740]
out=evidence_path(root/'Docs/Scripts/Offline/output/help_lines_controls_20261016');out.mkdir(exist_ok=True)
(out/'entries.json').write_text(json.dumps(entries,indent=2),encoding='utf-8')
(out/'renderer.txt').write_text('\n'.join('%08x %s %s'%(i.address,i.mnemonic,i.op_str) for i in decoder.disasm(body,0x76f960)),encoding='utf-8')
(out/'provenance.json').write_text(json.dumps(dict(source=str(evidence_relative(source, root)),page_sha256=hashlib.sha256(page).hexdigest(),archive=str(evidence_relative(archive, root)),archive_sha256=hashlib.sha256(data).hexdigest(),renderer_sha256=hashlib.sha256(body).hexdigest()),indent=2),encoding='utf-8')
for p in (0,8,9,24):
    for e in entries:
        if e['page']==p:
            print(p,e['row'],'offset',hex(e['offset']),'words',e['words'][:20],repr(e['text'][:100]))
print('Retained Help entry/layout and original2740-byte renderer audit:',out)
decoder.detail=True
start,size=0x776a50,1422
body=data[start-0x401000:start-0x401000+size];fixups=[]
for ins in decoder.disasm(body,start):
    for operand in ins.operands:
        if operand.type==X86_OP_IMM:
            value=operand.imm&0xffffffff
            if ins.group(cs.CS_GRP_JUMP) or ins.group(cs.CS_GRP_CALL):
                if not start<=value<start+size: fixups.append((ins.address-start+ins.imm_offset,value-0x400000,True))
                continue
            offset=ins.imm_offset
        elif operand.type==X86_OP_MEM:
            value=operand.mem.disp&0xffffffff;offset=ins.disp_offset
        else: continue
        if 0x400000<=value<0x2800000: fixups.append((ins.address-start+offset,value-0x400000,False))
write_replay_adapter(root/'src/native/tests/help_decoder_bytes.inc', '// Original1422-byte Help decoder; isolated offline fixture only.\n'
    'const unsigned char helpDecoderBody[]={\n'+'\n'.join('    '+','.join('0x%02x'%v for v in body[i:i+24])+',' for i in range(0,len(body),24))+'\n};\n'
    'const HelpFixup helpDecoderFixups[]={\n'+'\n'.join('    {%d,0x%x,%s},'%(a,b,str(c).lower()) for a,b,c in fixups)+'\n};\n'
    'const unsigned helpDecoderJumps[]={'+','.join(hex(v-0x400000) for v in struct.unpack_from('<11I',data,0x776fe0-0x401000))+'};\n', encoding='utf-8')
(out/'decoder-provenance.json').write_text(json.dumps(dict(va=start,size=size,body_sha256=hashlib.sha256(body).hexdigest(),fixups=fixups),indent=2),encoding='utf-8')
print('Original conditional Help decoder retained:',size,'bytes;',len(fixups),'relocations')
