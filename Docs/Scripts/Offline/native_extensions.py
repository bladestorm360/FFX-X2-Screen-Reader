"""Generate bounded, relocation-masked signatures from the archived reference.

Reads only the extracted reference and existing audit; never reads the game
installation. Runtime matching is restricted to the two supported SHA-256s.
"""

# Game-source inputs/exports live outside every Git tree.
from pathlib import Path as _EvidencePath
import sys as _evidence_sys
_evidence_repo = next(p for p in _EvidencePath(__file__).resolve().parents
                      if (p / "Docs/Scripts/Offline/evidence_paths.py").is_file())
_evidence_sys.path.insert(0, str(_evidence_repo / "Docs/Scripts/Offline"))
from evidence_paths import evidence_path, evidence_relative, write_replay_adapter

from pathlib import Path
import json
import hashlib
import re
import sys
from datetime import datetime, timezone
import capstone as cs
from capstone.x86 import X86_OP_IMM, X86_OP_MEM

ROOT = Path(__file__).resolve().parents[3]
REF = evidence_path(ROOT / 'Docs/Scripts/Offline/output/reader_reference_20261003_235502')
IDLE_DISPATCH_TARGET = 0x465390
GAGAZET_INTEGER_READ = 0x46c6c0
targets = [IDLE_DISPATCH_TARGET,0x385290,0x39fc80,0x39f680,0x4999b0,0x4ac710,0x4ac2a0,0x241290,0x4b1580,0x4b3810,0x4b3c20,0x4b4a00,0x4b4fb0,0x4b4df0,0x4b5510,0x4d19f0,0x38fc00,0x4ddf80,0x464180,0x488bb0,0x471d10,0x45ef20,0x42b260,0x46bd20,0x4acee0,0x471a50,0x4aad20,0x4aaad0,0x28fca0,0x42b230,0x506780,0x4b8510,0x471980,0x43d3b0, 0x649440, 0x649310, 0x648910, 0x28e730, 0x628b20, 0x301760, 0x301f40, 0x5af0, 0x5b10, 0x38f0b0,0x42d200,0x22d770,0x30aec0,0x30b6b0,0x30b250,0x30b8e0,0x30b420,0x30b3a0,0x30b460,0x30a680,0x307b40,0x548c9a,0x548ca6,
           0x6273b0,0x548cf4,0x628330,0x627e90,0x67c90a,0x628350,0x46c240,
           0x2d9db0,0x2d72e0,0x2d90f0,0x2d77b0,0x2dad80,0x505d30,
           0x22e4f0,0x225930,0x4c4230,0x4c3560,0x4c4530,0x4beda0,0x3902f0,0x505ab0,0x4c4fc0,0x4c4f60,
           0x4c7400,0x4c8080,0x4ceff0,0x4cfc00,0x4a9ae0,0x4a9ba0,0x4a9a80,0x4a9b70,
           0x4a9870,0x4a9920,0x4a9940,0x4a9960,0x4a9c20,0x4a97d0,
           0x4f0a70,0x4a9b30,0x384f50,0x385370,0x4d5150,0x4d2760,0x4f4f10,0x4f5000,0x3909c0,0x656160,0x4b6610,0x4de350,0x4de5b0,0x648c80,0x45bf70,0x45c140,0x476360,0x4767e0,0x4d85a0,0x384f40,0x4d85f0,0x4bdb30,0x4bdab0,0x4ba550,0x4ba360,0x38c800,0x3982b0,0x397090,0x22d700,0x22db80,0x22dd50,0x43e4e0,0x43e960,0x43e9e0,0x4aa410]
anchors={0x385290:0x4ac710,0x4ac2a0:0x4ac710,0x241290:0x4ac2a0,0x4ddf80:0x4d19f0,0x38fc00:0x4ddf80,0x4aa410:0x4c3560,0x384f40:0x4d85a0,0x4ba360:0x4ba550,0x5af0:0x301760,0x5b10:0x301f40,0x548c9a:0x30b6b0,0x548ca6:0x30b6b0,
         0x548cf4:0x6273b0,0x67c90a:0x627e90,0x3902f0:0x4c4530,
         0x4beda0:0x4c4530,0x505ab0:0x4c4530,0x225930:0x22e4f0,
         0x4cfc00:0x4ceff0,0x4a9ae0:0x4c7400,0x4a9ba0:0x4c7400,
         0x4a9a80:0x4c8080,0x4a9b70:0x4c8080,
         0x4a9870:0x4c8080,0x4a9920:0x4c8080,0x4a9940:0x4c8080,0x4a9960:0x4c8080,
         0x4a9c20:0x4ceff0,0x4a97d0:0x4ceff0,
         0x4a9b30:0x4f0a70,0x384f50:0x4f0a70,0x385370:0x4f0a70,0x3909c0:0x4f4f10}
imports={0x548c9a,0x548ca6,0x548cf4,0x67c90a}
full_bodies={0x39fc80:138,0x39f680:806,0x4ac710:48,0x4ac2a0:5,0x4b1580:7485,0x4b3810:474,0x4b3c20:69,0x4b4fb0:173,0x4d19f0:255,0x38fc00:245,0x4ddf80:24,0x471d10:211,0x506780:911,0x22d770:279,0x4d85a0:47,0x30b6b0:237,0x6273b0:668,0x4c3560:1812,0x4c4530:537,0x4c7400:459,0x4c8080:1826,0x4ceff0:1486,0x4f0a70:1852,0x4d5150:794}
required_data={0x11360b0,0x11360b4,0x11360b8,0x1866214,0x186621c,0x1866224,0x1866228,0x186624a,0x1866250,0x1327080,0x1327084,0x1327088,0x132708c,0x1327090,0x1327094,0xc30e90,0x1a860e0,0x1a860e4,0x1300740,
               0x186a218,0x186a21c,0x186a228,0x186a22c,0x186a230,0x186a240,0x186a351,0xc5a790,0x186ab60}
full_bodies[IDLE_DISPATCH_TARGET] = 205
dis = cs.Cs(cs.CS_ARCH_X86, cs.CS_MODE_32)
targets[0:0] = [0x4d5800, 0x3810f0, 0x38e680]
targets[0:0] = [0x38fa30,0x38f9e0,0x38f810,0x4913f0,0x4939a0,0x491a30]
targets[0:0] = [0x5da370,0x48dfe0]
targets[0:0] = [0x498a40]
targets[0:0] = [0x388eb0,0x388e20,0x395560,0x3955a0]
targets[0:0] = [0x395a30,0x3b09c0]
full_bodies.update({0x395a30:49,0x3b09c0:47})
targets[0:0] = [0x65a270,0x659800,0x658ec0]
full_bodies.update({0x65a270:99,0x659800:84,0x658ec0:289})
full_bodies.update({0x388eb0:32,0x388e20:110,0x395560:50,0x3955a0:50})
required_data.add(0x1133360)
full_bodies[0x498a40] = 154
required_data.update({0x133d6b4,0x133d6c0,0x133d6c4})
full_bodies.update({0x38fa30:77,0x38f9e0:75,0x38f810:290,0x4913f0:242})
required_data.update({0x133c8fe,0x133d1b4,0x133d6a0,0x133f73c,0x133f748,0x133f74c,0x133f750})
required_data.add(0x186aa28)
targets.append(GAGAZET_INTEGER_READ)
full_bodies[GAGAZET_INTEGER_READ]=120
targets.extend((0x51a830,0x51a7c0,0x51ab30,0x51a930)) # Accepted airship visibility/position/choice texture.
full_bodies.update({0x51a830:135,0x51a7c0:100,0x51ab30:142,0x51a930:60})
required_data.add(0x193bf80)
required_data.add(0x1330804) # Original91AB30 room382 airship cursor adjustment.
# Render-owned battle HUD: native frame/status/CTB owners and final clip routes.
battle_hud_bodies={0x49b2d0:530,0x4a0520:1546,0x4a1660:1540,0x49e5d0:1576,0x503bb0:806,0x4e5a20:683,0x4963c0:3482,0x496050:879,0x4e58e0:310,0x4f4b20:280}
battle_hud_bodies.update({0x39af20:21,0x505890:148,0x4fb940:405})
targets.extend(battle_hud_bodies)
full_bodies.update(battle_hud_bodies)
anchors.update({0x4963c0:0x49b2d0,0x4a0520:0x49b2d0,0x4a1660:0x4a0520,0x49e5d0:0x4a1660,0x503bb0:0x49e5d0,0x4e5a20:0x503bb0,0x496050:0x4963c0,0x4f4b20:0x496050,0x4e58e0:0x4f4b20})
anchors.update({0x39af20:0x4963c0,0x505890:0x4963c0})
required_data.add(0x133d742)
required_data.update({0x133f76c,0x25d0aa4,0x1871690,0x1871694,0x18716b8,0x18716bc,0x18716c0,0x18716c4,0x1871668,0x187166c})
full_bodies[0x42d200] = 1620
required_data.add(0x130077d)
required_data.add(0xc59554)
required_data.add(0xcded2c)
full_bodies[0x2d90f0]=445
required_data.add(0x1301998)
# Collision classification is a two-stage machine switch, not the decompile's
# off-by-one labels. Qualify the complete body and its actual tables as well.
full_bodies[0x43e4e0]=123
required_data.update({0x83e55c,0x83e574})
# Generic accepted renderer qualifies Monster Arena captures after native expansion.
targets.append(0x36f960)
full_bodies[0x36f960]=2740
# Actual hiku2100 coordinate producer and its Common00A3 owner boundary.
targets.extend([0x456cc0,0x46a280])
full_bodies.update({0x456cc0:109,0x46a280:156})
targets.append(0x43ea00)
full_bodies[0x43ea00]=209
required_data.update({0x1301a44,0x1301a48,0x1301a4c,0x1301a50,0x1301a54})
# Exact EV parent allocation: EDI holds logical file size at the loader-owned cdecl allocation call.
targets.extend([0x48d230,0x2871c0,0x46ca90,0x48d490])
full_bodies.update({0x48d230:197,0x2871c0:9,0x46ca90:1208,0x48d490:50})
anchors[0x2871c0]=0x48d230
dis.detail = True

# Main.as accepted popup focus event and visibility close boundary. Both are
# bounded observers; no Iggy invocation or private popup-field offsets.
targets.extend([0x28fb60,0x628ef0])
full_bodies.update({0x28fb60:279,0x628ef0:13})
targets.append(0x459a80)
full_bodies[0x459a80]=167
targets.extend([0x30d970,0x30f3f0,0x30e930])
full_bodies.update({0x30d970:81,0x30f3f0:362,0x30e930:183})
anchors[0x30f3f0]=0x30d970
targets.extend([0x4589f0,0x486de0])
full_bodies.update({0x4589f0:314,0x486de0:40})
targets.append(0x45d100)
full_bodies[0x45d100]=67
required_data.add(0x707b40) # Original SfxEvent callback operand; already independently qualified.

# Startup logos/autosave completion. Qualify the AL getter through the original
# boot-loop call, rather than a generic six-byte getter search.
targets.extend([0x241870,0x257b60,0x260580])
full_bodies.update({0x241870:773,0x257b60:116,0x260580:6})
anchors.update({0x257b60:0x241870,0x260580:0x241870})
required_data.add(0xccb9c2)
# OpeningScreen frame156 requests its actual localized autosave warning. Copy
# the returned UTF8 after the exact native newline converter, never registration.
targets.extend([0x2e96d0,0x2e99b0,0x2e9840])
full_bodies.update({0x2e96d0:273,0x2e99b0:139,0x2e9840:161})
anchors.update({0x2e99b0:0x2e96d0,0x2e9840:0x2e99b0})
# Final Help glyph draw, reached only through the qualified native renderer.
targets.append(0x4e52e0)
full_bodies[0x4e52e0]=45
anchors[0x4e52e0]=0x36f960
targets.append(0x374630)
full_bodies[0x374630]=332
anchors[0x374630]=0x36f960
targets.extend([0x4e75c0,0x4e52b0,0x4ee210])
full_bodies.update({0x4e75c0:714,0x4e52b0:45,0x4ee210:45})
anchors.update({0x4e75c0:0x36f960,0x4e52b0:0x36f960,0x4ee210:0x36f960})

# Key Items renderer 8C7C00 and focus helper 8C7620 both use this exact
# ownership predicate, reached through 8AB270. Never name an unowned slot.
targets.append(0x390850)
full_bodies[0x390850]=64
required_data.add(0x1130f1c)
targets.append(0x4d2b10)
full_bodies[0x4d2b10]=568
required_data.add(0x186a9e8)

# Results controller owns both native fast-forward loops and displayed counters.
targets.append(0x4ba620)
full_bodies[0x4ba620]=3955
required_data.update({0x1869ec8,0x1869ed4,0x1869ed8,0x1869ee6})

# Esc Special Features captions and focus records share this native table.
targets.append(0x302740)
full_bodies[0x302740]=894
targets.append(0x2e88b0)
full_bodies[0x2e88b0]=159 # Original Special Features Flash notification/OnPlay.
required_data.add(0xc3a2bc)
targets.append(0x290130)
full_bodies[0x290130]=1089
required_data.add(0xccc868)
targets.extend([0x45e0c0,0x45e6b0,0x45eaa0])
full_bodies.update({0x45e0c0:61,0x45e6b0:61,0x45eaa0:48,0x45ef20:48})
word_queries={0x45e0c0:(17,34,0x1327080),0x45e6b0:(17,52,0x1327090),
              0x45eaa0:(20,34,0x1327080),0x45ef20:(20,52,0x1327090)}
data = (REF / 'block_00401000.bin').read_bytes()
# Numbered Sphere Grid selection uses the original cursor-only animation.
# Mode identities come from the original mode setters' pointer operands.
targets.extend([0x648d20,0x65b7b0,0x65b930,0x65b980])
full_bodies.update({0x648d20:65,0x65b930:70,0x65b980:324})
required_data.update({0xa44ef0,0xa45010,0xa452d0})
# Actual numeric battle renderer and three-string Escape popup invocation.
# Derive amount/digit globals from their rendered operands, not a build delta.
targets.extend([0x4a3250,0x28df70,0x385330])
targets.append(0x3983a0)
full_bodies.update({0x4a3250:970,0x28df70:124,0x385330:23,0x3983a0:298})
required_data.update({0x1340424,0x1340418,0x11307e8,0x1134568})
symbols=(evidence_path(ROOT/'Docs/Scripts/Ghidra/output/FFX/functions.csv')).read_text(encoding='utf-8')
for target in targets:
    if not re.search(r',%08x,' % (target+0x400000), symbols, re.IGNORECASE):
        raise ValueError('Target is not an archived function entry: '+hex(target))
specs = []
for rva in targets:
    tokens = []
    refs = []
    calls={}
    for ins in dis.disasm(data[rva-0x1000:rva-0x1000+max(768,full_bodies.get(rva,0))], rva+0x400000):
        raw = list(ins.bytes)
        for operand in ins.operands:
            value = (operand.imm if operand.type == X86_OP_IMM else operand.mem.disp if operand.type == X86_OP_MEM else 0) & 0xffffffff
            if not 0x400000 <= value < 0x2800000:
                continue
            offset, size = (ins.imm_offset, ins.imm_size) if operand.type == X86_OP_IMM else (ins.disp_offset, ins.disp_size)
            if size == 4 and value in required_data:
                refs.append(dict(reference=value-0x400000, offset=len(tokens)+offset))
            if size==4 and ins.mnemonic in ('call','jmp'):
                calls.setdefault(value-0x400000,dict(offset=len(tokens)+offset,end=len(tokens)+ins.size))
            for i in range(offset, offset+size):
                raw[i] = None
        tokens.extend(raw)
        if len(tokens) >= (6 if rva in imports else full_bodies[rva] if rva in full_bodies else 600 if rva in anchors.values() else 151 if rva==0x649310 else 128) or (ins.mnemonic == 'ret' and rva not in ({0x649310} | full_bodies.keys())):
            break
    specs.append(dict(rva=rva, tokens=tokens, refs=refs,calls=calls))
    if rva == 0x28e730:
        # Exact installed SHA-256 profile: FFEscMenu's native guard moved
        # from +EC to +FC. The reader observes getters, never this field.
        # Keep the layout byte fixed in each profile instead of masking it.
        if tokens[68:75] != [0x80, 0xbb, 0xec, 0, 0, 0, 0]:
            raise ValueError('Unexpected archived FFEscMenu guard instruction')
        specs[-1]['installed_tokens'] = list(tokens)
        specs[-1]['installed_tokens'][70] = 0xfc
    if rva == 0x290130:
        # Exact pinned Installed update body: private fields moved by16 and a
        # timing block was added. +70 visibility is unchanged. Its menu global
        # operand also moved; do not read the new timing global at old CCC868.
        body=(evidence_path(ROOT/'Docs/Scripts/Offline/output/esc_installed_20261014_180743/menu-installed-290130-full.bin')).read_bytes()
        if hashlib.sha256(body).hexdigest()!='d529f321e4c2ceb8b9631e3789e65ab6bc2f8f3b02d73e89e0c615b4c777ec77' or len(body)!=1174 or body[-1]!=0xc3 or body[0x466:0x468]!=b'\xff\x35':
            raise ValueError('Complete retained Installed Esc update body required')
        installed=[]
        for ins in dis.disasm(body,0x68ffb0):
            raw=list(ins.bytes)
            for operand in ins.operands:
                value=(operand.imm if operand.type==X86_OP_IMM else operand.mem.disp if operand.type==X86_OP_MEM else 0)&0xffffffff
                if 0x400000<=value<0x2800000:
                    offset,size=(ins.imm_offset,ins.imm_size) if operand.type==X86_OP_IMM else (ins.disp_offset,ins.disp_size)
                    for i in range(offset,offset+size):raw[i]=None
            installed.extend(raw)
        specs[-1]['installed_tokens']=installed
        for ref in specs[-1]['refs']:
            if ref['reference']==0x8cc868:
                ref['installed_offset']=0x468
    if rva == 0x45e6b0:
        # Preserve the already verified Installed98-byte room49/worker11/flag20
        # variant. Its captured body is retained evidence, not a game-install read.
        import struct
        captured=evidence_path(ROOT/'Docs/Scripts/Offline/output').parent/'historical-install/reset-resume-20261013/jecht-installed-practice-body.txt'
        body=bytearray()
        for line in captured.read_text().splitlines():
            m=re.match(r'0x([0-9a-f]+)\s+([0-9a-f]+)\s',line)
            if m:
                offset=int(m[1],16)
                if offset>=98:break
                if offset!=len(body):raise ValueError('Noncontiguous retained Installed practice body')
                body.extend(bytes.fromhex(m[2]))
        if len(body)!=98:raise ValueError('Complete Installed practice body required')
        installed=[]
        for ins in dis.disasm(bytes(body),0x85e820):
            raw=list(ins.bytes)
            for operand in ins.operands:
                value=(operand.imm if operand.type==X86_OP_IMM else operand.mem.disp if operand.type==X86_OP_MEM else 0)&0xffffffff
                if 0x400000<=value<0x2800000:
                    offset,size=(ins.imm_offset,ins.imm_size) if operand.type==X86_OP_IMM else (ins.disp_offset,ins.disp_size)
                    for i in range(offset,offset+size):raw[i]=None
            installed.extend(raw)
        if len(installed)!=98 or body[53]!=0xe8:raise ValueError('Verified Installed getter CALL53 required')
        specs[-1]['installed_tokens']=installed
        specs[-1]['installed_word_call']=53
for spec in specs:
    owner=anchors.get(spec['rva'])
    if owner:
        parent=next(s for s in specs if s['rva']==owner)
        spec['anchor']=dict(owner=owner,**parent['calls'][spec['rva']])
    # Verify the signature against the retained reference before emitting it.
    first=max(0x1000,spec['rva']-4096)-0x1000
    last=min(len(data),spec['rva']+4096-0x1000+len(spec['tokens']))
    positions=range(spec['rva']-0x1000,spec['rva']-0x1000+1) if owner else range(first,last-len(spec['tokens'])+1)
    def qualify_word(position):
        if spec['rva'] not in word_queries:
            return True
        # Common0051's body has same-shaped neighboring raw-word handlers.
        # Prove its relative call20 resolves MOV AX,[pressed word]; RET.
        import struct
        call,offset,word=word_queries[spec['rva']]
        getter=position+call+5+struct.unpack_from('<i',data,position+call+1)[0]
        return data[getter:getter+2]==b'\x66\xa1' and data[getter+6]==0xc3 and struct.unpack_from('<I',data,getter+2)[0]==word
    matches=[position+0x1000 for position in positions if all(token is None or data[position+i]==token for i,token in enumerate(spec['tokens'])) and qualify_word(position)]
    if spec['rva'] in word_queries:
        call,offset,word=word_queries[spec['rva']]
        spec['word_getter']=dict(owner=0x471d10,call=call,offset=offset)
    if matches != [spec['rva']]:
        raise ValueError('Archived signature is not unique: '+hex(spec['rva'])+' '+repr(matches))
# Runtime resolves caller-qualified signatures in one pass: emit owners first.
ordered=[];pending=list(specs)
while pending:
    ready=[s for s in pending if not s.get('anchor') or s['anchor']['owner'] in {x['rva'] for x in ordered}]
    if not ready: raise ValueError('Cyclic or missing signature owner')
    for spec in ready: ordered.append(spec);pending.remove(spec)
specs=ordered
lines = ['// Generated by Docs/Scripts/Offline/native_extensions.py from archived .text.',
         '// Null tokens mask only absolute addresses and relative call operands.',
         '#pragma once', '#include <array>', 'namespace ffx {',
         'struct ExtensionSignature { unsigned rva; const int* tokens; unsigned size, owner, offset, end; const int* installedTokens; unsigned wordOwner=0,wordCall=0,wordOffset=0,installedSize=0,installedWordCall=0; };']
for spec in specs:
    lines.append('inline constexpr int signature_%x[]={%s};' % (spec['rva'], ','.join('-1' if b is None else str(b) for b in spec['tokens'])))
    if 'installed_tokens' in spec:
        lines.append('inline constexpr int installed_signature_%x[]={%s};' % (spec['rva'], ','.join('-1' if b is None else str(b) for b in spec['installed_tokens'])))
lines.append('inline constexpr ExtensionSignature extensionSignatures[]={')
for spec in specs:
    anchor=spec.get('anchor',dict(owner=0,offset=0,end=0))
    variant = 'installed_signature_%x' % spec['rva'] if 'installed_tokens' in spec else 'nullptr'
    word=spec.get('word_getter')
    suffix=',0x%x,%d,%d,%d,%d' % (word['owner'],word['call'],word['offset'],len(spec.get('installed_tokens',[])),spec.get('installed_word_call',0)) if word else (',0,0,0,%d,0'%len(spec['installed_tokens']) if 'installed_tokens' in spec else '')
    lines.append('    {0x%x,signature_%x,%d,0x%x,%d,%d,%s%s},' % (spec['rva'], spec['rva'], len(spec['tokens']),anchor['owner'],anchor['offset'],anchor['end'],variant,suffix))
lines.extend(['};', 'struct ExtensionOperand { unsigned reference, owner, offset, installedOffset=0; };',
              'inline constexpr ExtensionOperand extensionOperands[]={'])
for spec in specs:
    for ref in spec['refs']:
        lines.append('    {0x%x,0x%x,%d,%d},' % (ref['reference'], spec['rva'], ref['offset'],ref.get('installed_offset',0)))
lines.extend(['};', '}'])
(ROOT / 'src/native/profile_extensions_data.hpp').write_text('\n'.join(lines)+'\n')
(ROOT / 'Docs/Scripts/Frida/FFX/native_extensions.json').write_text(json.dumps(specs, indent=2)+'\n')
prototype=ROOT/'Docs/Scripts/Frida/FFX/accessibility_events.js'
if prototype.exists():
    source=prototype.read_text(encoding='utf-8')
    start=source.index('// BEGIN GENERATED SIGNATURES')
    end=source.index('// END GENERATED SIGNATURES')
    value='// BEGIN GENERATED SIGNATURES\nvar signatures='+json.dumps([s for s in specs if s['rva'] in (0x43d3b0,0x648910,0x38f0b0,0x42d200)])+';\n'
    prototype.write_text(source[:start]+value+source[end:],encoding='utf-8')
print('Generated archived signatures: '+', '.join(hex(s['rva']) for s in specs))
evidence=evidence_path(ROOT/'Docs/Scripts/Offline/output')/('native_extensions_'+datetime.now(timezone.utc).strftime('%Y%m%d_%H%M%S_%f')+'.json')
evidence.write_text(json.dumps(dict(reference=str(REF),verified=[hex(s['rva']) for s in specs],operands=[r for s in specs for r in s['refs']]),indent=2)+'\n')
print('Archived uniqueness evidence: '+str(evidence))
