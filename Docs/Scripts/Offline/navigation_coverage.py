"""Audit the complete extracted map catalog without game-install access."""
from collections import Counter
from datetime import datetime,timezone
from pathlib import Path
import hashlib,json
from field_text import TextEvidence
ROOT=Path(__file__).resolve().parents[3]
EVIDENCE=Path(__file__).parent/'output/FFX/20261003_audit'
maps=json.loads((EVIDENCE/'maps_v3.json').read_text())['events']
targets=json.loads((EVIDENCE/'navigation_targets.json').read_text())
labels=json.loads(sorted(EVIDENCE.glob('navigation_audit_20261007_*.json'))[-1].read_text())['labels']
old=json.loads((EVIDENCE/'navigation_audit_20261006_160545.json').read_text())['labels']
by_source={r['source']:r for r in targets}
text=TextEvidence(EVIDENCE/'text')
details=[]
for event in maps:
    source=EVIDENCE/'files'/event['source']
    if hashlib.sha256(source.read_bytes()).hexdigest()!=event['sha256']:raise ValueError('Changed source '+event['source'])
    record=by_source[event['source']]
    expected={i for i,w in enumerate(event['workers']) if w['type'] in [1,2,3,5,6]}
    covered={r['worker'] for r in record['targets']+record['excluded']}
    if covered!=expected:raise ValueError('Worker omitted '+event['source'])
    if any(r['limited'] for r in record['targets']):raise ValueError('Truncated traversal '+event['source'])
    for row in record['targets']:
        if any(entry>=len(event['workers'][row['worker']]['entries']) or entry>=320 for entry in row['entries']):raise ValueError('Callback mask bounds')
    details.append(dict(source=event['source'],script=event['script'],sha256=event['sha256'],
        areas=[text.macro(11,i) for i in event['area_indices']],workers=len(expected),
        targets=len(record['targets']),excluded=len(record['excluded']),
        categories=dict(Counter(r['category'] for r in record['targets'])),
        callback_count=max(len(w['entries']) for w in event['workers']),
        exclusions=record['excluded']))
priority=[r for r in details if r['script'].startswith(('mihn','nagi'))]
report=dict(utc=datetime.now(timezone.utc).isoformat(),scripts=len(maps),hashes_verified=len(details),
    catalog_targets=len(labels),previous_targets=len(old),net_additions=len(labels)-len(old),
    catalog_categories=dict(Counter(r['category'] for r in labels)),
    traversal_truncations=0,worker_omissions=0,max_authored_callbacks=max(r['callback_count'] for r in details),
    max_useful_callback=max(entry for r in targets for t in r['targets'] for entry in t['entries']),
    priority=priority,all_maps=details,
    runtime_contract=['Live native channel/entry/priority getters and flags on every navigation key',
        'Native actor identity/enablement and actual geometry, never a baked coordinate',
        'Copied script identity cache is generation-scoped and bounded to four chunks',
        'Current collision getter and mutation hook govern routes and walls',
        'Collected/disabled interactions leave through live flags/remapping, including same-count changes'],
    limitations=['Static paths conservatively include both variable-dependent branches. Unknown computed REQ destinations cannot be named without live evidence.',
        'No claim of gameplay confirmation or of testing every save. Extracted coverage and native fixtures are separate from live acceptance.'])
output=Path(__file__).parent/'output'/('navigation_coverage_'+datetime.now(timezone.utc).strftime('%Y%m%d_%H%M%S')+'.json')
output.write_text(json.dumps(report,indent=2,ensure_ascii=False)+'\n')
print(f"PASS: {len(details)} source hashes, zero worker omissions/truncations, {len(labels)} targets; {len(priority)} priority variants. {output.name}")
