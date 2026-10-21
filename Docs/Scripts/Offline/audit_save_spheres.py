"""Save-sphere-only audit of all retained map variants; no installation access."""
from pathlib import Path
from datetime import datetime, timezone
import json
import re
import sys
import hashlib
ROOT=Path(__file__).resolve().parents[3]
sys.path.insert(0,str(ROOT/'src/navigation'))
from identity_labels import SAVE_SPHERE_MODELS,model_identity
from field_text import TextEvidence
evidence=Path(__file__).parent/'output/FFX/20261003_audit'
reports=sorted(evidence.glob('navigation_audit_*.json'))
current=json.loads(reports[-1].read_text())['labels']
previous=json.loads(reports[-2].read_text())['labels']
maps=json.loads((evidence/'navigation_targets.json').read_text())
by_source={m['source']:m for m in maps}
old={(r['source'],r['worker']):r for r in previous}
text=TextEvidence(evidence/'text')
rows=[]; changed=[]; mixed=[]; hashes={}
for row in current:
    models=set(row['models'])
    if not models.intersection(SAVE_SPHERE_MODELS): continue
    if not models.issubset(SAVE_SPHERE_MODELS):
        mixed.append(dict(script=row['script'],worker=row['worker'],reason='mixed developer model-selection worker; runtime actual model owns classification'))
        continue
    assert row['category']==row['label']=='Save Sphere',row
    source=evidence/'files'/row['source']
    if row['source'] not in hashes:
        hashes[row['source']]=hashlib.sha256(source.read_bytes()).hexdigest()
        assert hashes[row['source']]==by_source[row['source']]['sha256']
    details=dict(script=row['script'],worker=row['worker'],source=row['source'],models=sorted(models),entries=row['entries'])
    rows.append(details)
    prior=old[(row['source'],row['worker'])]
    if (prior['category'],prior['label'])!=(row['category'],row['label']):
        strings=text.field_strings(source)
        target=next(t for t in by_source[row['source']]['targets'] if t['worker']==row['worker'])
        ids=[c['literals'][-1] for c in target['rootCalls'] if c['call']==0x64 and c['literals'] and isinstance(c['literals'][-1],int)]
        native=[strings[i] for i in ids if 0<=i<len(strings) and 'Save Sphere' in strings[i]]
        assert native,details
        changed.append(dict(**details,old_category=prior['category'],old_label=prior['label'],native_evidence=native))
assert {(r['script'],r['worker']) for r in changed}=={('cdsp0700',12),('stbv0000',16),('stbv0000',17)}
for row in current:
    prior=old[(row['source'],row['worker'])]
    assert (row['entries'],row['models'])==(prior['entries'],prior['models'])
    if (row['source'],row['worker']) not in {(r['source'],r['worker']) for r in changed}:
        assert (row['category'],row['label'],row['identity'])==(prior['category'],prior['label'],prior['identity'])
native=(ROOT/'src/native/navigation_labels.cpp').read_text()
helper=re.search(r'bool navigationSaveSphereModel\(.*?\{(.*?)\}',native,re.S)[1]
assert {int(x,16) for x in re.findall(r'0x[0-9a-f]+',helper)}==SAVE_SPHERE_MODELS
assert all(model_identity(m,'','')=='Save Sphere' for m in SAVE_SPHERE_MODELS)
stamp=datetime.now(timezone.utc).strftime('%Y%m%d_%H%M%S_%f')
out=Path(__file__).parent/'output'/('save_spheres_'+stamp+'.json')
out.write_text(json.dumps(dict(map_variants=len(maps),save_targets=len(rows),save_source_hashes_verified=len(hashes),models=sorted(SAVE_SPHERE_MODELS),corrected=changed,mixed_developer_workers=mixed,all_save_targets=rows,previous_catalog=str(reports[-2]),current_catalog=str(reports[-1]),other_labels_and_membership_unchanged=True),indent=2)+'\n')
print(f'PASS: {len(maps)} variants, {len(rows)} save-sphere targets; exactly three underwater entries repaired; all other labels/membership preserved. Evidence: {out.name}')
