"""Original native exit/gate handoffs, separate from the development warp.

No attachment. Script hashes, entry identity and empty-stack literal prefixes
are checked before generating contracts. These are completion shortcuts, not
replays of every manual sphere/platform animation.
"""
from datetime import datetime, timezone
import json
from build_utilities import ROOT, EVIDENCE, DESIGNS, action, binding, cpp_action, cpp_binding, literal
from atel_model import instructions

# Besaid first-visit tail retains its original 164 write. The later tail
# contains no story write. Zanarkand opens the gate at 2775 before its lift;
# it deliberately does not request the later 2785 story/cutscene worker.
RECIPES = {
    'besaid': ((30,5,23374), (30,6,23569), None,164,[]),
    'kilika': ((37,4,None),(37,4,None),None,0,[]),
    'djose': ((66,4,None),(66,4,None),None,0,[]),
    'macalania': ((44,4,None),(45,4,None),None,1553,[]),
    'bevelle': ((56,4,None),(56,4,None),None,0,[('L6:map:01f4ab:T0',1,0)]),
    # Open only the normal trial gate. Worker41 would take the lift directly
    # to room318, bypassing the native room317 encounter route (worker42).
    'zanarkand': ((39,47,None),(39,47,None),None,2775,[
        ('L6:map:00f6ac:T5',0,0),('L6:map:00f6b4:T5',1,0),
        ('L6:map:00f6b0:T5',2,0),('L6:map:00f6bc:T5',0,0),
        ('L6:map:00f6c0:T5',0,0),('L6:map:00f6c4:T0',6,0)]),
}

def main():
    maps=json.loads((EVIDENCE/'maps_v3.json').read_text(encoding='utf8'))
    rows,records=[],[]
    for name,(first,later,exit_,threshold,seeds) in RECIPES.items():
        flow=json.loads((DESIGNS/(name+'_flow.json')).read_text(encoding='utf8'))
        event=next(e for e in maps['events'] if e['script']==flow['script'])
        if event['sha256'] != flow['sha256']: raise ValueError('Script evidence changed')
        decoded=instructions(bytes.fromhex(event['code']))
        actions=[action(flow,*a) for a in [first,later]+([exit_] if exit_ else [])]
        for a in actions:
            if a['pc'] not in decoded: raise ValueError('Completion is not an instruction boundary')
        # Both Besaid tails begin before their own literal push for fade(30).
        if name=='besaid' and any(decoded[a['pc']][0]!=0xae for a in actions):
            raise ValueError('Besaid completion prefix changed')
        prep=[binding(flow,*s) for s in seeds]
        rows.append('    {%s, %s, %s, %s, %d, {%s}},' % (
            literal(flow['script']),cpp_action(actions[0]),cpp_action(actions[1]),
            cpp_action(actions[2]) if exit_ else '{0,0,0,0,0}',threshold,
            ', '.join(cpp_binding(b) for b in prep)))
        if name=='macalania': rows[-1]=rows[-1][:-2]+', 1545},'
        if name=='bevelle':
            bonus=action(flow,17,7)
            done=binding(flow,'L6:map:01f4aa:T0',1,0)
            # Original W15 guards this chest at79266, REQEW W17:7 at79278,
            # then records the map marker at79283. Never seed it before RET.
            assert bonus['pc']==79849
            rows[-1]=rows[-1][:-2]+', 0, '+cpp_action(bonus)+', '+cpp_binding(done)+'},'
        records.append(dict(script=flow['script'],sha256=flow['sha256'],actions=actions,
                            storyThreshold=threshold,firstMinimum=1545 if name=='macalania' else 0,preparation=prep))
        if name=='bevelle': records[-1].update(bonus=bonus,bonusCollected=done)
    (ROOT/'src/native/puzzle_completion_data.hpp').write_text(
        '// Generated original ATEL completion/gate contracts.\n#pragma once\nnamespace ffx {\n'
        'inline const PuzzleCompletion puzzleCompletions[] = {\n'+'\n'.join(rows)+'\n};\n}\n',encoding='utf8')
    target=EVIDENCE/('puzzle_completions_'+datetime.now(timezone.utc).strftime('%Y%m%d_%H%M%S')+'.json')
    target.write_text(json.dumps(dict(trials=records,policy='Native completion shortcuts; gameplay validation pending.'),indent=2)+'\n',encoding='utf8')
    print('Compiled six original completion handoffs. '+target.name)

if __name__=='__main__': main()
