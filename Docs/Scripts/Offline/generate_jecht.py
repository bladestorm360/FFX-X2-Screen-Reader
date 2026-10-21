"""Generate bounded Jecht input/cue identities from retained original ATEL."""
from pathlib import Path
import json
import re
import sys

ROOT = Path(__file__).resolve().parents[3]
MASKS = [0x9000, 0x1000, 0x8000, 0, 0xc000, 0x3000, 0x2000, 0x4000, 0x6000]

def main(directory):
    scripts, queries, cues = [], [], []
    for number, (name, real, tutorial, realentry, active, selector, timer) in enumerate([
        ('swin0000',32,32,8,[30,31],[46,47],48),
        ('swin0200',24,23,6,[13,14],[18,19],20)]):
        data = json.loads((Path(directory)/(name+'.json')).read_text())
        script, compiled = data['script'], data['compiled']
        w = compiled['workers'][real]; tw = compiled['workers'][tutorial]
        binding = script['workers'][real]['declarations']
        guards=[b for b in w['blocks'] if b['start']>=w['entries'][realentry] and
                any(s['kind']=='branch' and s['condition'].endswith(' > 10)') for s in b['statements'])]
        code=bytes.fromhex(script['code'])
        if len(guards)!=1:raise ValueError('One original eleven-success award guard required')
        score_pc=guards[0]['start']
        if code[score_pc]!=0x9f or code[score_pc+3:score_pc+7]!=bytes.fromhex('ae0a000a'):
            raise ValueError('Original score>10 bytecode required')
        score=int.from_bytes(code[score_pc+1:score_pc+3],'little')
        if binding[score]['raw_low']!=(0x0c000002 if number==0 else 0x0c000009) or binding[score]['raw_high']!=1:
            raise ValueError('Original owner-side score declaration required')
        stage_low = 0x0c000006 if number == 0 else 0x0c00000d
        stage = next(v['index'] for v in binding if v['raw_low'] == stage_low and v['raw_high'] == 1)
        tutorial_binding = script['workers'][tutorial]['declarations']
        tutorial_stage = next(v['index'] for v in tutorial_binding if v['raw_low'] == stage_low and v['raw_high'] == 1)
        stage_writes = sorted((s for b in tw['blocks'] for s in b['statements']
                              if s['kind'] == 'write' and s['variable'] == tutorial_stage), key=lambda s: s['pc'])
        if [s['value'] for s in stage_writes] != [str(i) for i in range(7)]:
            raise ValueError('Changed original tutorial progression 0 through 6')
        tutorial_pc = max(pc for pc in tw['entries'] if pc <= stage_writes[0]['pc'])
        tutorial_entry = tw['entries'].index(tutorial_pc)
        tutorial_end = min(pc for pc in tw['entries'] if pc > tutorial_pc)
        if stage_writes[0]['pc'] != tutorial_pc + 3:
            raise ValueError('Tutorial must begin by resetting its original phase')
        for local in (real + 1, real + 2):
            cue_binding = script['workers'][local]['declarations']
            for index in [stage, *active, *selector, timer]:
                if any(cue_binding[index][key] != binding[index][key] for key in ('raw_low', 'raw_high')):
                    raise ValueError('Cue worker must share the exact phase/input declarations')
        # Code checksum is the existing immutable navigation identity, not a file hash.
        checksum = 0x5486e5d2 if number==0 else 0x273a7d2b
        scripts.append('    {"%s",%d,%d,0x%x,%d,%d,%d,%d,%d,%d,%d,{%d,%d},{%d,%d},{0x%x,0x%x},{0x%x,0x%x},%d,%d,%d},' %
            (name,script['code_length'],len(script['workers']),checksum,real,realentry,w['entries'][realentry],
             timer,binding[timer]['raw_low'],score,binding[score]['raw_low'],*active,*selector,*(binding[i]['raw_low'] for i in active),
             *(binding[i]['raw_low'] for i in selector),tutorial,tutorial_pc,tutorial_entry))
        blocks=w['blocks']; realrows=[]
        for i,b in enumerate(blocks):
            calls=[s for s in b['statements'] if s['kind']=='call' and s.get('id') in (0x50,0x51,0x122)]
            if not calls or b['start']<w['entries'][realentry]:continue
            guard=blocks[i+1]
            branches=[s for s in guard['statements'] if s['kind']=='branch']
            if len(branches)!=1:raise ValueError('Missing original selector guard')
            m=re.fullmatch(r'\(L6:map:([0-9a-f]+):T5\[0\] == (\d)\)',branches[0]['condition'])
            if not m:raise ValueError('Changed selector condition')
            slot=next(i for i,index in enumerate(selector) if binding[index]['offset']==int(m[1],16))
            value=int(m[2]); mask=MASKS[value+(5 if slot else 0)]
            realrows.append((slot,value))
            for call in calls:
                button=int(call['args'][0]); expected=128 if call['id']==0x122 else 1 if call['id']==0x51 else bool(mask&(1<<button))
                queries.append('    {%d,%d,%d,%d,0x%x,%d,%d,%d,%d},' %
                    (number,real,realentry,call['pc'],call['id'],int(expected),slot,value,mask|0x20))
        if sorted(realrows)!=[(0,i) for i in range(5)]+[(1,i) for i in range(4)]:raise ValueError('Nine original directions required')
        # Four practice loops, including neutral-center Confirm. The two
        # explanation/example advance loops are not practice input predicates.
        practice=[]
        for i,b in enumerate(tw['blocks']):
            if not tutorial_pc <= b['start'] < tutorial_end:continue
            calls=[s for s in b['statements'] if s['kind']=='call' and s.get('id') in (0x4c,0x4d,0x50,0x51,0x122)]
            positive=[s for s in calls if s['id']==0x4c]
            if not positive:
                if [(s['id'],s['args']) for s in calls] != [
                    (0x50,['12']),(0x50,['14']),(0x50,['13']),(0x50,['15']),
                    (0x122,['2']),(0x122,['2']),(0x122,['3']),(0x122,['3']),(0x51,['5'])]:continue
                display=[s for s in tw['blocks'][i-2]['statements'] if s['kind']=='call' and s.get('id')==0x64]
                branches=[s for s in tw['blocks'][i-1]['statements'] if s['kind']=='branch']
                writes=[s for s in tw['blocks'][i+1]['statements'] if s['kind']=='write']
                if len(display)!=1 or display[0]['args']!=['0',str(69 if number==0 else 94)] or \
                   len(branches)!=1 or not branches[0]['condition'].endswith(' == 4)') or \
                   len(writes)!=1 or writes[0]['variable']!=tutorial_stage or writes[0]['value']!='5':
                    raise ValueError('Center practice must follow its original visible Confirm prompt and phase4->5')
            else:
                # Preserve the already-qualified directional responses exactly.
                calls=[s for s in calls if s['id']!=0x122]
            mask=sum(1<<int(s['args'][0]) for s in positive)
            practice.append(mask)
            for call in calls:
                if not tutorial_pc <= call['pc'] < tutorial_end:
                    raise ValueError('Practice query outside the original tutorial entry')
                expected=128 if call['id']==0x122 else 1 if call['id'] in (0x4d,0x51) else bool(mask&(1<<int(call['args'][0])))
                queries.append('    {%d,%d,%d,%d,0x%x,%d,2,0,%d},' %
                    (number,tutorial,tutorial_entry,call['pc'],call['id'],int(expected),mask|0x20))
        if practice!=[0x3000,0x8000,0,0x6000]:raise ValueError('Changed original practice sequence')
        for local,start in [(real+1,0),(real+2,5)]:
            worker=compiled['workers'][local]
            for entry,pc in enumerate(worker['entries']):
                if entry<6:continue
                b=next(b for b in worker['blocks'] if b['start']==pc)
                call=next(s for s in b['statements'] if s['kind']=='call' and s.get('id')==0x6a)
                sources=[b for b in w['blocks'] if any(s['kind']=='request' and s.get('worker')==str(local) and s.get('entry')==str(entry) for s in b['statements'])]
                if len(sources)!=1:raise ValueError('One original sound/request producer per cue required')
                sound=next(s for s in sources[0]['statements'] if s['kind']=='call' and s.get('id')==0xdf)
                cues.append('    {%d,%d,%d,%d,%d,%d,%d,%d},' % (number,local,entry,call['pc'],MASKS[start+entry-6]|0x20,sound['pc'],int(sound['args'][0]),int(call['args'][0])))
    output='''// Generated from retained original Winno ATEL by generate_jecht.py.
#pragma once
namespace ffx {
struct JechtScript {const char* name;unsigned length,workers,checksum,local,entry,entryPc,timer,timerLow,score,scoreLow;unsigned active[2],selector[2],activeLow[2],selectorLow[2];unsigned tutorial,tutorialPc,tutorialEntry;};
struct JechtQuery {unsigned script,local,entry,pc,call,expected,slot,value,mask;};
struct JechtCue {unsigned script,local,entry,pc,mask,soundPc,sequence,window;};
inline constexpr JechtScript jechtScripts[]={
'''+ '\n'.join(scripts)+'''
};
inline constexpr JechtQuery jechtQueries[]={
'''+ '\n'.join(queries)+'''
};
inline constexpr JechtCue jechtCues[]={
'''+ '\n'.join(cues)+'''
};
}
'''
    (ROOT/'src/native/jecht_data.hpp').write_text(output)
    print(f'{len(scripts)} scripts, {len(queries)} exact input queries, {len(cues)} original visible cues')

if __name__=='__main__':main(sys.argv[1])
