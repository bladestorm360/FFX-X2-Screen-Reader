"""Map-specific NPC proposals with recorded evidence; not shipping narration.

Call IDs/model labels are reference leads, not verified proper names. A text
topic alone never promotes an actor to a service role. Icon typing is not yet
verified. Keep proposals separate until the user runs the validation probe.
"""
import argparse
import csv
import json
from pathlib import Path
import re
from field_text import TextEvidence

ROOT = Path(__file__).resolve().parents[3]


def infer(directory):
    directory = Path(directory)
    data = json.loads((directory / "npc_calls.json").read_text())
    text = TextEvidence(directory / "text")
    models = {}
    with (ROOT.parent / "FFXDataParser/src/main/resources/enums/models.csv").open(encoding="utf-8") as stream:
        for row in csv.reader(stream):
            if len(row) >= 4 and row[0] == "model":
                models[int(row[1], 16)] = row[3]
    strings_by_map, records = {}, []
    for worker in data:
        script = worker["script"]
        if script not in strings_by_map:
            strings_by_map[script] = text.field_strings(directory / "files" / worker["source"])
        strings = strings_by_map[script]
        model_ids, dialogue, menus = set(), [], set()
        is_player, prices = False, False
        for call in worker["calls"]:
            values = call["literals"]
            if call["call"] in (1, 0x134) and values and values[-1] is not None:
                model_ids.add(values[-1] & 0xFFFF)
            if call["call"] == 0x43:
                is_player = True
            if call["call"] == 0x401D and values and values[-1] is not None:
                menus.add(values[-1])
            if call["call"] == 0x225:
                prices = True
            if call["call"] == 0x64 and len(values) >= 2 and values[-1] is not None:
                string_id = values[-1]
                if 0 <= string_id < len(strings):
                    dialogue.append({"id": string_id, "text": strings[string_id], "offset": call["offset"]})
        if is_player or not model_ids:
            continue
        labels = [models.get(i, f"model 0x{i:04X}") for i in sorted(model_ids)]
        combined = "\n".join(d["text"] for d in dialogue)
        # A choice with a service verb plus a modular-menu call supports a
        # provisional role. It still requires menu-id and actor binding proof.
        commerce_choice = re.search(r"<choice:\d+>[^\n<]*(?:Buy|Sell|Shopping)", combined, re.I)
        role = "Merchant candidate" if menus and (commerce_choice or prices) else None
        # Named dialogue has a speaker on the first line. Unnamed text starts
        # with a newline. Require one repeated, consistent speaker; never use
        # names merely mentioned in a sentence or an asset's guessed identity.
        prefixes = {}
        for d in dialogue:
            first, separator, rest = d["text"].partition("\n")
            if separator and rest and re.fullmatch(r"[A-Z][A-Za-z’' -]{1,23}", first):
                prefixes.setdefault(first, set()).add(d["id"])
        named = list(prefixes) if len(prefixes) == 1 else []
        speaker = named[0] if named and len(prefixes[named[0]]) >= 2 else None
        records.append({"script": script, "worker": worker["worker"], "models": sorted(model_ids),
                        "model_descriptions": labels, "proposed_role": role,
                        "proposed_speaker": speaker, "speaker_string_ids": sorted(prefixes.get(speaker, [])),
                        "modular_menu_ids": sorted(menus), "changes_shop_prices": prices,
                        "dialogue": dialogue, "calls": worker["calls"],
                        "verification": "offline candidate; icon type and runtime actor binding unverified"})
    report = {"references": ["Karifean/FFXDataParser", "BDFFHD scripts/mapdata/roles.py"],
              "policy": "No generic model proper-name guesses. No runtime-learned labels. No shipping changes.",
              "workers": records}
    output = directory / "npc_inference.json"
    output.write_text(json.dumps(report, ensure_ascii=False), encoding="utf-8")
    roles = [r for r in records if r["proposed_role"]]
    speakers = [r for r in records if r["proposed_speaker"]]
    print(f"Analyzed {len(records)} non-player model-bearing workers; {len(roles)} provisional merchant roles; {len(speakers)} repeated speaker-header proposals.")
    for record in [r for r in roles if not r["script"].startswith("test")][:3]:
        print(record["script"], "worker", record["worker"], record["proposed_role"], "menus", [hex(i) for i in record["modular_menu_ids"][:3]])
    print("Evidence:", output)


if __name__ == "__main__":
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("directory", type=Path)
    infer(parser.parse_args().directory)
