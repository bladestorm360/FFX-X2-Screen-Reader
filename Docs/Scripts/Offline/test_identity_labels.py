"""Offline identity regression: actual production catalog generator helpers."""
import json
import re
from pathlib import Path
import sys
sys.path.insert(0,str(Path(__file__).resolve().parents[3]/"src/navigation"))
from identity_labels import npc_identity,event_identity,authored_name,model_identity,model_kind,interaction_texts,service_identity
from evidence_paths import REPOSITORY,evidence_path
assert npc_identity(["Cid\nFirst line","Cid\nSecond line"],[(5,"WAKK","Wakka")])[:2]==("Wakka",4)
assert npc_identity(["Wakka\nFirst line","Wakka\nSecond line"],[(0x2101,"CID257","Cid")])[:2]==("Cid",4)
assert npc_identity(["Talk to Yuna"],[(0x2050,"ALBD_HAN080","")],"Shopkeeper")[:2]==("Yuna",4)
assert npc_identity(["Maechen\nLine one","Maechen\nLine two"],[(0x2002,"ZANA_HAN002","")])[:2]==("Maechen",3)
assert npc_identity(["\nTalk about Yuna","\nYuna is nearby"],[(0x2002,"ZANA_HAN002","")])[0]=="Villager"
assert npc_identity([],[(0x2050,"ALBD_HAN080","")])[0]=="Al Bhed"
assert npc_identity([],[(0x207a,"MAN_YVNSLD122","")])[0]=="Soldier"
assert npc_identity([],[(0x207e,"MAN_HIPRST126","")])[0]=="Priest"
assert npc_identity([],[(0x2050,"ALBD_HAN080","")],"Innkeeper")[0]=="Innkeeper"
assert npc_identity(["Auron\nLine one","Yuna\nLine two"],[(0x2002,"ZANA_HAN002","")])[0]=="Villager"
assert authored_name(["Talk to Yuna","Talk to Wakka"])==""
assert event_identity(["Press the switch"],[])[0]=="Switch"
assert event_identity(["An inscription is carved here"],[])[0]=="Inscription"
assert event_identity(["A sphere-shaped recess"],[])[0]=="Sphere recess"
print("PASS: authored names, conservative speakers, services, sprite families and event functions.")

# Check the generated production catalog as well as the inference primitives.
evidence=evidence_path(REPOSITORY/"Docs/Scripts/Offline/output/FFX/20261003_audit")
report=max(evidence.glob("navigation_audit_*.json"))
rows=json.loads(report.read_text(encoding="utf8"))["labels"]
baseline=json.loads((evidence/'navigation_audit_20261007_164736.json').read_text(encoding='utf8'))['labels']
assert [(r['source'],r['worker'],r['entries'],r['models']) for r in rows]==[(r['source'],r['worker'],r['entries'],r['models']) for r in baseline]
assert not any(re.match(r"(?:Talk|Speak|Interact)\b",r["label"]) for r in rows)
for script,worker,name,category in [("nagi0000",10,"Belgemine","NPC"),
    ("nagi0000",22,"Maechen","NPC"),("swin0400",1,"O’aka","Shop")]:
    actors=[r for r in rows if r["script"]==script and r["worker"]==worker]
    assert actors and all(r["label"]==name and r["category"]==category for r in actors)
print(f"PASS: all {len(rows)} production identities and entry membership preserved; named reward NPCs remain NPCs.")

assert model_identity(0x500b,"ALBDBOOK011","")=="Book"
assert model_kind(0x500b)=="object"
assert model_kind(0x10fa)=="character"
assert model_kind(0x2000)=="unknown"
for row in rows:
    if row["category"]=="NPC": assert row["models"] and all(model_kind(m)=="character" for m in row["models"]), row
books=[r for r in rows if r["script"]=="kami0100" and r["worker"] in (9,10)]
assert {r['worker'] for r in books}=={9,10} and all(r["category"]=="Object" and r["label"]=="Book" for r in books)
assert all(r['category']=='Object' and r['label']=='Primer' for r in rows if r['script']=='kami0100' and r['worker']==16)
print("PASS: every NPC has character-model evidence; travel-agency books are Objects.")

assert model_identity(0x10d0,"SABO_02","")=="Cactuar"
assert model_kind(0x10d0)=="character"
assert model_identity(0x5002,"TAKARABOX002","")=="Treasure chest"
assert model_kind(0x5033)==model_kind(0x5034)=="character"
assert interaction_texts(['wrong','<choice:0>Rest.\n<choice:1>Leave.'],[{'call':0x13b,'literals':[4,1,1,0,0,0,0]}])==['<choice:0>Rest.\n<choice:1>Leave.']
assert service_identity(['Would you like to rest here?','<choice:0>Rest.\n<choice:1>Leave.'],[{'call':0x13b}])=='Innkeeper'
assert service_identity(['<choice:0>Buy.\n<choice:1>Sell.'],[{'call':0x13b},{'call':0x401d}])=='Shopkeeper'
assert not service_identity(['<choice:0>Buy something for her.'],[{'call':0x13b},{'call':0x401d}])
assert event_identity(['<choice:0>Rest.'],[{'call':0x13b}])[0]=='Choice event'
assert not any(r['label']=='Menu' for r in rows)
inn=[r for r in rows if r['script']=='guad0100' and r['worker']==20]
assert len(inn)==1 and inn[0]['category']=='Shop' and inn[0]['label']=='Innkeeper'
print('PASS: native choice argument, contextual inn/shop roles, unrelated choice exclusions and Guadosalam innkeeper.')
