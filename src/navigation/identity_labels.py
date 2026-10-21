"""User-authorized concise identities from actor-local ATEL and sprite evidence.

Pattern adapted from the user's BDFFHD roles.py: names, service roles, costume,
then conservative general labels. A name mentioned in dialogue is never its speaker.
"""
import re
import json
from pathlib import Path

MODEL_ASSETS = {int(k):v for k,v in json.loads((Path(__file__).parent/"model_assets.json").read_text(encoding="utf8")).items()}
# Visual texture review: 20261006_154301/review_03.png. Belgemine has a
# character model in the monster directory. Cactuar and Chocobo meshes are
# also confirmed complete creatures; a character need not be human.
CHARACTER_EXCEPTIONS = {0x10fa:"Belgemine",0x10d0:"Cactuar",0x5033:"Chocobo",0x5034:"Armored chocobo"}
# Dedicated native CID257 body; an airship dialogue speaker on another worker
# must not supply its identity. This is metadata, not a copied game function.
NAMED_CHARACTER_MODELS = {0x2101: "Cid"}
# Facial overlays and white dummy/proxy textures are not complete NPC sprites.
OVERLAYS = {0x2030,0x2035,0x2038,0x204f,0x2068,0x2069,0x20f9,0x2152}

def model_kind(model):
    asset=MODEL_ASSETS.get(model)
    if model in CHARACTER_EXCEPTIONS: return "character"
    if not asset or not asset.get("textured") or model in OVERLAYS or 0x320<=model<=0x331: return "unknown"
    return "character" if asset["group"] in ("pc","npc") else "object"

PARTY = {0:"Tidus",1:"Yuna",2:"Auron",3:"Kimahri",4:"Wakka",5:"Lulu",6:"Rikku"}

def clean(text):
    return re.sub(r"\s+", " ", re.sub(r"<[^>]*>", "", text)).strip()

def interaction_texts(strings, calls):
    """Native displayFieldText arg 1 and displayFieldChoice arg 1 are text IDs."""
    result = []
    for call in calls:
        values = call.get("literals", [])
        index = values[1] if call["call"] in (0x64, 0x13b) and len(values) >= 2 else None
        if isinstance(index, int) and 0 <= index < len(strings):
            result.append(strings[index])
    return result

def service_identity(strings, calls):
    """Recognize actual rest/commerce callbacks, including free/shared inn service."""
    if not any(c["call"] == 0x13b for c in calls): return ""
    body = clean(" ".join(strings))
    choices = [clean(s) for text in strings for s in re.findall(r"<choice:\d+>([^\n<]*)", text)]
    if any(re.fullmatch(r"Rest[.!]?", choice, re.I) for choice in choices) or re.search(
            r"\b(?:would you like|want) to rest (?:here|at|in)\b", body, re.I):
        return "Innkeeper"
    if any(c["call"] == 0x401d for c in calls) and any(re.fullmatch(
            r"(?:Buy|Sell|Shopping|Got any (?:weapons|items)\?)[.!]?", choice, re.I) for choice in choices):
        return "Shopkeeper"
    return ""

def authored_name(strings):
    names=set()
    for text in strings:
        m=re.fullmatch(r"(?:Talk (?:to|with)|Speak (?:to|with)|Interact with)\s+([A-Z][A-Za-z’' -]{1,39})[.!]?",clean(text))
        if m: names.add(m[1])
    return next(iter(names)) if len(names)==1 else ""

def speaker_name(strings):
    names={}
    for text in set(strings):
        first,sep,rest=text.partition("\n")
        if sep and rest and re.fullmatch(r"[A-Z][A-Za-z’' -]{1,23}",first):
            names[first]=names.get(first,0)+1
    return next(iter(names)) if len(names)==1 and next(iter(names.values()))>=2 else ""

SAVE_SPHERE_MODELS = frozenset((0x5001,0x50ab))

def model_identity(model,asset,description):
    if model in NAMED_CHARACTER_MODELS: return NAMED_CHARACTER_MODELS[model]
    if model in CHARACTER_EXCEPTIONS: return CHARACTER_EXCEPTIONS[model]
    if model in SAVE_SPHERE_MODELS: return "Save Sphere"
    if model==0x5002: return "Treasure chest"
    kind=model_kind(model)
    if kind=="unknown": return ""
    if kind=="object":
        patterns=[(r"BOOK","Book"),(r"(?:BARREL|TARU)","Barrel"),
          (r"(?:CRATE|BOX)","Crate"),(r"(?:SWITCH|SWCH)","Switch"),
          (r"(?:DOOR|GATE)","Door"),(r"(?:LIFT|ELEVATOR)","Lift"),
          (r"CHOCOBO","Chocobo"),(r"SEABIRD","Seabird"),(r"SABO","Cactuar")]
        for pattern,label in patterns:
            if re.search(pattern,asset,re.I): return label
        return "Object"
    stems={"TIDU":"Tidus","YUNA":"Yuna","AURO":"Auron","KIMA":"Kimahri","WAKK":"Wakka","LULU":"Lulu","RIKK":"Rikku"}
    for stem,name in stems.items():
        if re.match(stem+r"(?:_|$)",asset,re.I): return name
    for name in PARTY.values():
        if re.match(re.escape(name)+r"(?:\s*\(|$)",description): return name
    patterns=[(r"ALBD","Al Bhed"),(r"(?:SLD|SOLD|EXPD)","Soldier"),
      (r"(?:HIPRST|PRIEST)","Priest"),(r"GAD_GUARD","Guado guard"),
      (r"GAD_","Guado"),(r"RNS_","Ronso"),(r"(?:SAILOR|SHIPCREW)","Sailor"),
      (r"MAN_BAND","Musician"),(r"(?:JDG|BLZ_STF)","Blitzball official"),
      (r"(?:BLZ|BLZA)","Blitzball player"),(r"MAN_YVN","Priest"),
      (r"CHOCOBO","Chocobo"),(r"SEABIRD","Seabird"),(r"(?:BARREL|TARU)","Barrel"),
      (r"(?:CRATE|BOX)","Crate"),(r"(?:SWITCH|SWCH)","Switch"),
      (r"(?:DOOR|GATE)","Door"),(r"(?:LIFT|ELEVATOR)","Lift")]
    for pattern,label in patterns:
        if re.search(pattern,asset,re.I): return label
    if 0x2000<=model<0x3000: return "Villager"
    if description and not re.search(r"\?|/|unknown|unused|no model",description,re.I):
        value=re.sub(r"\s*\((?:Hi|Lo)-Res\)","",description).strip()
        return re.sub(r"(?<=[a-z])(?=[A-Z])"," ",value)
    return ""

def npc_identity(strings,models,service=""):
    # A different speaker can appear in this actor's conversation. Dedicated
    # model families establish whose body is actually loaded before text does.
    model_names={label for model,asset,desc in models if (label:=model_identity(model,asset,desc))}
    if len(model_names)==1 and next(iter(model_names)) in set(PARTY.values())|set(NAMED_CHARACTER_MODELS.values()):
        return next(iter(model_names)),4,"dedicated character model family"
    name=authored_name(strings)
    if name: return name,4,"authored interaction name"
    name=speaker_name(strings)
    if name: return name,3,"repeated actor-local speaker header"
    if service: return service,2,"player-entry service"
    body=clean(" ".join(strings))
    roles=[(r"\b(?:I am|I'm|we are|we're) (?:a |the )?(?:temple )?priest\b","Priest"),
      (r"\b(?:on duty|my post|guard post|I am a soldier|I'm a soldier)\b","Soldier"),
      (r"\b(?:I am|I'm) (?:a |the )?sailor\b","Sailor")]
    for pattern,label in roles:
        if re.search(pattern,body,re.I): return label,2,"actor-local role dialogue"
    labels={label for model,asset,desc in models if (label:=model_identity(model,asset,desc))}
    if len(labels)==1: return next(iter(labels)),1,"sprite family"
    return "NPC",0,"unknown character identity"

def object_identity(strings,calls,models):
    """Functional labels need this object's own script, not its sprite alone."""
    text=clean(" ".join(strings))
    if 0x500b in models and (any(c["call"]==0x10d for c in calls) or
            re.search(r"Obtained Al Bhed Primer",text,re.I)):
        return "Primer",2,"book model with primer collection/text branch"
    # f016_model.png: upright console with an oval panel and projector lens.
    # kami0100 worker 11 opens tutorial menus and launches tutorial battles.
    if models=={0x5010} and any(c["call"]==0x401d for c in calls):
        return "Tutorial terminal",2,"f016 sprite and actor-local tutorial menu calls"
    return "",0,""

def conversation_identity(strings):
    names=[]
    for text in strings:
        header,sep,_=text.partition("\n")
        if not sep: continue
        match=re.search(r"<control:13:(3[0-6])>",header)
        name=PARTY[int(match[1],16)-0x30] if match else ""
        if not name and re.fullmatch(r"[A-Z][A-Za-z’' -]{1,23}",header): name=header
        if name and name not in names: names.append(name)
    if len(names)==2 and "Tidus" in names:
        return "Tidus talks to "+next(n for n in names if n!="Tidus")
    if 0<len(names)<=3: return "Conversation with "+", ".join(names)
    return ""

def event_identity(strings,calls):
    name=authored_name(strings)
    if name: return name,4,"authored interaction name"
    text=clean(" ".join(strings))
    patterns=[(r'copy of|\b(?:read|reading).{0,25}\bbook\b',"Book"),
      (r"Destruction Sphere","Destruction Sphere recess"),
      (r"sphere.shape|sphere-shaped|Sphere is inserted","Sphere recess"),
      (r"\bglyph\b","Glyph"),(r"\b(?:sign|inscription)\b","Inscription"),
      (r"(?:open|close|locked|sealed|unlock).{0,30}\b(?:door|gate)\b","Door"),
      (r"\b(?:press|push|pull|activate).{0,25}\b(?:switch|lever|button)\b","Switch"),
      (r"\b(?:ride|board).{0,20}\b(?:lift|elevator)\b","Lift")]
    for pattern,label in patterns:
        if re.search(pattern,text,re.I): return label,2,"player-entry event text/function"
    conversation=conversation_identity(strings)
    if conversation: return conversation,2,"speaker headers on reachable event dialogue"
    if any(c["call"]==0x13b for c in calls):
        if re.search(r"Compile Al Bhed dictionary", text, re.I):
            return "Al Bhed compilation sphere",2,"native dictionary compilation choice"
        if re.search(r"Obtained Jecht.s sphere", text, re.I):
            return "Jecht's sphere",2,"native sphere collection dialogue"
        return "Choice event",1,"native choice with unresolved interaction identity"
    return "Event",0,"general event fallback"
