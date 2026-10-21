"""Bind offline navigation outcomes to labels and generate native catalogs.

User-authorized ATEL inference: repeated speaker headers, unambiguous named
models, service calls, and trial roles. Ambiguous identities get useful role
labels. Never identify an NPC from a name merely mentioned in dialogue.
"""

# Game-source inputs/exports live outside every Git tree.
from pathlib import Path as _EvidencePath
import sys as _evidence_sys
_evidence_repo = next(p for p in _EvidencePath(__file__).resolve().parents
                      if (p / "Docs/Scripts/Offline/evidence_paths.py").is_file())
_evidence_sys.path.insert(0, str(_evidence_repo / "Docs/Scripts/Offline"))
from evidence_paths import evidence_path, evidence_relative, write_replay_adapter

from datetime import datetime, timezone
import csv
import hashlib
import json
from pathlib import Path
import re
import sys

ROOT = Path(__file__).resolve().parents[2]
sys.path.insert(0, str(ROOT / "Docs/Scripts/Offline"))
from field_text import TextEvidence
from identity_labels import npc_identity, event_identity, object_identity, model_identity, model_kind, MODEL_ASSETS, speaker_name, interaction_texts, service_identity
from identity_labels import SAVE_SPHERE_MODELS

EVIDENCE = evidence_path(ROOT / "Docs/Scripts/Offline/output/FFX/20261003_audit")
CATEGORY = {"Exit":1,"Event":2,"Save Sphere":3,"Treasure Chest":4,"NPC":5,"Object":6,"Shop":7}
PARTY = {0:"Tidus",1:"Yuna",2:"Auron",3:"Kimahri",4:"Wakka",5:"Lulu",6:"Rikku"}
# The model enumeration order differs from the menu's character indices.
MODEL_NAMES = {1:"Tidus",2:"Yuna",3:"Auron",4:"Kimahri",5:"Wakka",6:"Lulu",7:"Rikku"}

TRIAL_LABELS = {
 "bsyt0100":{7:"Pedestal",16:"Destruction Sphere treasure",30:"Exit to the antechamber",
   37:"Entry glyph",38:"Door glyph",39:"Glyph Sphere recess",40:"Door sphere recess 1",
   41:"Door sphere recess 2",42:"Hidden sphere recess",43:"Destruction Sphere",
   44:"Pedestal sphere recess",45:"Wall symbols",46:"Besaid Sphere recess"},
 "klyt1000":{6:"Pedestal",7:"Destruction Sphere treasure",28:"Entry Kilika Sphere",
   29:"Entry door recess",30:"Side-wall storage recess 1",31:"Wall glyph recess",32:"Side-wall storage recess 2",
   33:"Fire chamber wall recess",34:"Destruction Sphere",35:"Lower Kilika Sphere recess",
   36:"Antechamber door recess",37:"Exit to the antechamber",41:"Pedestal reset tile"},
 "djyt0600":{1:"Pedestal",6:"Upper pedestal",24:"Destruction Sphere treasure",61:"Gap switch",62:"Pedestal reset glyph",63:"Central lift",64:"Hidden glyph",66:"Exit to the antechamber"},
 "mcyt0500":{1:"Pedestal",46:"Main floor pedestal reset tile",47:"Upper walkway pedestal reset tile",19:"Destruction Sphere treasure",45:"Exit to the antechamber"},
 "bvyt1100":{13:"Rail pedestal",16:"Destruction Sphere treasure",17:"Second treasure chest",56:"Exit to the antechamber"},
 "dome0700":{32:"Destruction Sphere treasure",33:"Trial encounter",41:"Lift to the Chamber of the Fayth",42:"Exit to the trial encounter",
   44:"Pattern pedestal",45:"Pattern pedestal",46:"Pattern pedestal",47:"Pattern pedestal",
   48:"Besaid Sphere source",49:"Kilika Sphere source and treasure recess",50:"Destruction Sphere",
   51:"Besaid pedestal recess",52:"Kilika pedestal recess"},
}
# Distinguish authored trial controls by native worker order. These numbers do
# not infer compass directions or human identities from unverified models.
TRIAL_LABELS["djyt0600"].update({48:"Entry sphere recess 1",49:"Entry sphere recess 2",50:"Door sphere recess 1",51:"Door sphere recess 2",
    52:"Charging wall recess 1",53:"Charging wall recess 2",54:"Charged Sphere door recess",55:"Left circuit recess",
    **{w:f"Upper pedestal {w-55}" for w in range(56,61)}})
TRIAL_LABELS["mcyt0500"].update({37:"Upper Macalania Sphere recess",38:"Ramp Macalania Sphere recess",39:"Central Glyph Sphere recess",40:"Lower treasure sphere recess",41:"Lower Glyph Sphere reveal recess",42:"Revealed Macalania Sphere",43:"Lower bridge support recess"})
TRIAL_LABELS["bvyt1100"].update({30:"Upper Bevelle Sphere source",31:"Upper rail power recess",32:"Lower Bevelle Sphere source",33:"Lower rail power recess",34:"Glyph Sphere source and Destruction Sphere destination",35:"Glyph Sphere barrier recess",36:"Destruction Sphere source",37:"Upper platform sphere recess"})
TRIAL_LABELS["dome0700"].update({w:f"Pattern pedestal {w-43}" for w in range(44,48)})
TRIAL_LABELS["dome0700"].update({53:"First chamber pattern display",54:"Main chamber pattern display",
    56:"Trial floor activation tile",84:"Return puzzle completion tile"})
TRIAL_LABELS["dome0700"].update({r['worker']:r['label'] for r in
    json.loads((ROOT/'src/puzzles/zanarkand_glyphs.json').read_text(encoding='utf8'))['glyphs']})

def clean(text):
    return re.sub(r"\s+", " ", re.sub(r"<[^>]*>", "", text)).strip()

def main():
    audit = json.loads((EVIDENCE / "navigation_targets.json").read_text(encoding="utf8"))
    maps = json.loads((EVIDENCE / "maps_v3.json").read_text(encoding="utf8"))
    events = {e["source"]:e for e in maps["events"]}
    npc = {(r["script"],r["worker"]):r for r in json.loads((EVIDENCE / "npc_inference.json").read_text(encoding="utf8"))["workers"]}
    # Older proposals discarded anyone activated as a player in ANY entry.
    # Party members can still have genuine talk entries. Keep their model
    # evidence here; the runtime separately excludes the actual player actor.
    calls_by_worker = {(r["source"],r["worker"]):r["calls"] for r in
        json.loads((EVIDENCE / "npc_calls.json").read_text(encoding="utf8"))}
    text = TextEvidence(EVIDENCE / "text")
    model_labels = {}
    model_assets = {}
    with (ROOT.parent / "FFXDataParser/src/main/resources/enums/models.csv").open(encoding="utf8") as stream:
        for row in csv.reader(stream):
            if len(row)>=3 and row[0]=="model":
                model_assets[int(row[1],16)] = row[2]
                model_labels[int(row[1],16)] = row[3] if len(row)>=4 else ""
    rows, scripts, records = [], [], []
    for record in sorted(audit, key=lambda r:r["script"]):
        event = events[record["source"]]
        source = EVIDENCE / "files" / record["source"]
        if hashlib.sha256(source.read_bytes()).hexdigest() != record["sha256"]:
            raise ValueError("Navigation source hash changed")
        strings = text.field_strings(source)
        targets = {r["worker"]:r for r in record["targets"]}
        overrides = dict(TRIAL_LABELS.get(record["script"], {}))
        for worker,label in overrides.items():
            if worker >= len(event["workers"]): raise ValueError("Trial role outside script")
            targets.setdefault(worker,dict(worker=worker,category="Object",room=-1,calls=[],rootCalls=[],entries=[2],limited=False))
        start = len(rows)
        for worker, target in sorted(targets.items()):
            n = npc.get((record["script"],worker),{})
            if not n:
                model_ids = set()
                for call in calls_by_worker.get((record["source"],worker),[]):
                    if call["call"] in (1,0x134) and call["literals"] and isinstance(call["literals"][-1],int):
                        model_ids.add(call["literals"][-1]&0xffff)
                n = dict(models=sorted(model_ids))
            category = target["category"]
            name, evidence = "", ["player-entry outcome"]
            proper,identity = -1,0
            used=interaction_texts(strings,target["calls"])
            # Speakers belong to this actor's own calls, not a shared helper or
            # unrelated cutscene worker reached by REQ.
            local=[strings[c["literals"][-1]] for c in calls_by_worker.get((record["source"],worker),[]) if c["call"]==0x64 and
                c["literals"] and isinstance(c["literals"][-1],int) and 0<=c["literals"][-1]<len(strings)]
            loaded_models=set(n.get("models",[]))
            # Rewards are also given by named NPCs, including Belgemine. Only
            # this actor's own speaker or human sprite evidence changes a chest.
            if category=="Treasure Chest" and any(
                model_kind(m)=="character" for m in loaded_models):
                category="NPC"
            kinds={model_kind(m) for m in loaded_models}
            if category=="NPC" and kinds!={"character"}:
                category="Object"
                evidence.append("object or unverified sprite; ATEL actor type is not an NPC proof")
            role=service_identity(used,target["calls"])
            service = role or n.get("proposed_role") or any(c["call"]==0x225 for c in target["calls"]) and any(c["call"]==0x401d for c in target["calls"])
            if not role and service:
                role = "Innkeeper" if re.search(r"Rest\.|Travel Agency",clean(" ".join(used)),re.I) else "Shopkeeper"
            if worker in overrides:
                name = overrides[worker]; identity=2
                category = "Exit" if name.startswith("Exit ") else "Treasure Chest" if name.lower().endswith(("treasure","chest")) else "Object"
                evidence.append("trial ATEL role")
            else:
                models=set(n.get("models",[]))
                if models and models.issubset(SAVE_SPHERE_MODELS): category,name,identity="Save Sphere","Save Sphere",2
                elif models=={0x5002}: category,name,identity="Treasure Chest","Treasure chest",2
                elif category=="Treasure Chest": name,identity="Treasure chest",2
                elif category=="NPC":
                    name,identity,reason=npc_identity(local,[(m,model_assets.get(m,""),model_labels.get(m,"")) for m in models],role)
                    proper=next((i for i,v in PARTY.items() if v==name),-1)
                    evidence.append(reason)
                elif category=="Exit": name,identity="Exit",2
                elif category=="Object" and models:
                    labels={model_identity(m,model_assets.get(m,""),model_labels.get(m,"")) for m in models}
                    labels.discard("")
                    name,identity=(next(iter(labels)),1) if len(labels)==1 else ("Object",0)
                    if len(labels)!=1: name="Object"
                    functional,score,reason=object_identity(local,calls_by_worker.get((record["source"],worker),[]),models)
                    if score:
                        name,identity=functional,score
                        evidence.append(reason)
                    elif record["script"] in TRIAL_LABELS and models.issubset({0x506e,0x5071,0x5074}):
                        # Sphere animation workers share insertion/removal text
                        # for several colours. They are spheres, not recesses.
                        name,identity="Sphere",2
                        evidence.append("trial sphere animation model")
                    elif name=="Object":
                        inferred,score,reason=event_identity(used,target["calls"])
                        if score: name,identity=inferred,score
                    evidence.append("actual archive model class and sprite/interaction evidence")
                else:
                    name,identity,reason=event_identity(used,target["calls"])
                    evidence.append(reason)
            if category in ("NPC","Event","Object") and service:
                category="Shop"
                if identity<3: name,identity=role,2
                evidence.append("player-entry commerce service")
            if any(entry<0 or entry>=320 for entry in target["entries"]): raise ValueError("Native entry mask overflow")
            words=[sum(1<<(entry%32) for entry in target["entries"] if entry//32==word) for word in range(10)]
            mask="EntryMask{{%s}}" % ",".join("0x%x" % word for word in words)
            rows.append("    {%d, %d, %d, %d, L%s, %s, %d}," % (worker,CATEGORY[category],target["room"],proper,json.dumps(name,ensure_ascii=True),mask,identity))
            records.append(dict(script=record["script"],source=record["source"], worker=worker,category=category,label=name,identity=identity,entryMaskWords=words,entries=target["entries"],models=sorted(loaded_models),evidence=evidence))
        scripts.append("    {%s, %d, 0x%x, %d, %d}," % (json.dumps(record["script"]),record["length"],record["checksum"],start,len(rows)-start))
    directory = ROOT / "src/native"
    includes=[]
    for part,start in enumerate(range(0,len(rows),350)):
        filename=f"navigation_targets_{part}.inc"
        (directory/filename).write_text("\n".join(rows[start:start+350])+"\n",encoding="utf8")
        includes.append(f'#include "{filename}"')
    (directory/"navigation_targets_data.hpp").write_text(
        "// Generated from hashed ATEL player-entry paths and English evidence.\n#pragma once\nnamespace ffx {\n"
        "inline const NavigationTarget navigationTargets[] = {\n"+"\n".join(includes)+"\n};\n"
        "inline const NavigationScript navigationScripts[] = {\n"+"\n".join(scripts)+"\n};\n}\n",encoding="utf8")
    model_rows=[]
    for model in sorted(MODEL_ASSETS):
        label=model_identity(model,model_assets.get(model,""),model_labels.get(model,""))
        kind=model_kind(model)
        if kind!="unknown":
            model_rows.append("    {0x%x, L%s, %d},"%(model,json.dumps(label or "NPC",ensure_ascii=True),1 if kind=="character" else 2))
    model_includes=[]
    for part,start in enumerate(range(0,len(model_rows),350)):
        filename=f"navigation_models_{part}.inc"
        (directory/filename).write_text("\n".join(model_rows[start:start+350])+"\n",encoding="utf8")
        model_includes.append(f'#include "{filename}"')
    (directory/"navigation_models_data.hpp").write_text("// Generated from actual hashed VBF asset classes and attributed FFXDataParser identities.\n#pragma once\nnamespace ffx {\nstruct NavigationModel { unsigned model; const wchar_t* label; unsigned kind; };\ninline constexpr NavigationModel navigationModels[]={\n"+"\n".join(model_includes)+"\n};\n}\n",encoding="utf8")
    stamp=datetime.now(timezone.utc).strftime("%Y%m%d_%H%M%S")
    report=EVIDENCE/("navigation_audit_"+stamp+".json")
    report.write_text(json.dumps(dict(utc=datetime.now(timezone.utc).isoformat(),
        user_authorization="2026-10-06 concise NPC/event identities, actor-local script and sprite inference",
        labels=records, excluded=sum(len(s["excluded"]) for s in audit),
        unknown_identity=sum(r["identity"]==0 for r in records),
        live_validation="pending"),ensure_ascii=False,indent=2)+"\n",encoding="utf8")
    print(f"Generated {len(rows)} navigation targets across {len(scripts)} verified scripts; audit {report.name}")

if __name__=="__main__": main()
