"""Read-only source/document inventory for the October 18 maintenance review.

Only src/native and the explicitly named documentation trees are inspected. This
does not discover logs/assets, launch a program, or treat a comment count as a
quality test. The manifest is deliberately explicit: new translation units must
be assigned after their responsibility has been reviewed.
"""
from __future__ import annotations

# Game-source inputs/exports live outside every Git tree.
from pathlib import Path as _EvidencePath
import sys as _evidence_sys
_evidence_repo = next(p for p in _EvidencePath(__file__).resolve().parents
                      if (p / "Docs/Scripts/Offline/evidence_paths.py").is_file())
_evidence_sys.path.insert(0, str(_evidence_repo / "Docs/Scripts/Offline"))
from evidence_paths import evidence_path, evidence_relative, write_replay_adapter


import hashlib
import json
import re
from pathlib import Path

ROOT = Path(__file__).resolve().parents[3]
TOPICS = {
    "runtime": "reader profile profile_validation profile_extensions profile_dependencies "
        "hooks hook_calls speech speech_capture speech_queue speech_domains prism_backend "
        "log_capture log_history text text_plan game_text ready_text ready_game_text "
        "copied_text copied_controls button_glyphs game_binding_input game_context "
        "resource_reset owned_retirement atel_resources input direct_input controller "
        "controller_bindings physical_gamepad sdl_runtime settings mod_menu prototype_bridge",
    "menus": "field_menus field_details field_targets field_controls aeon_attributes "
        "game_details selection_details system_menus system_menu_capture status_review "
        "status_grid save_load_menu shops confirmations naming monster_arena messages "
        "message_text dialogue_voice movie_dialogue blitz_review help esc_capture esc_settings "
        "esc_details intro_cue sphere_grid sphere_data sphere_owned sphere_paths "
        "sphere_sections sphere_expert sphere_orientation sphere_planner sphere_jump sphere_text",
    "battle": "battle battle_announcements battle_graphics battle_status battle_sensor "
        "battle_results battle_hud status_animation overdrive wakka wakka_identity",
    "navigation": "navigation navigation_observations navigation_scene navigation_targets "
        "navigation_labels navigation_intents atel atel_memory nav_memory nav_geometry "
        "nav_zones nav_interactions nav_exits exit_labels nav_facing nav_cues nav_audio "
        "wall_cues reachability_capture reachability_worker auto_walk movement_keys "
        "swimming_route swimming_navigation pedestal_input pedestal_keys pedestal_state pedestal_rules",
    "puzzles": "puzzle_solver puzzle_completion puzzle_tiles puzzle_guide puzzle_guide_owned "
        "puzzle_guide_targets utility_context utility_memory utility_scripts utility_text "
        "utility_text_plans utility_trials via_purifico gagazet sin_collection zanarkand_patterns "
        "jecht jecht_voice lightning minigames minigame_memory minigame_completion butterfly butterfly_processing "
        "butterfly_navigation butterfly_paths airship_search airship_destinations",
    "distribution": "utilities utility_console utility_feedback utility_warps",
}


def main() -> None:
    assignments: dict[str, str] = {}
    for topic, stems in TOPICS.items():
        for stem in stems.split():
            if stem in assignments:
                raise ValueError(f"duplicate source assignment: {stem}")
            assignments[stem] = topic
    sources = sorted((ROOT / "src/native").glob("*.cpp"))
    actual = {p.stem for p in sources}
    if actual != assignments.keys():
        raise ValueError(f"unmapped={sorted(actual - assignments.keys())}; "
                         f"absent={sorted(assignments.keys() - actual)}")
    units = []
    for source in sources:
        raw = source.read_bytes()
        body = raw.decode("utf-8-sig")
        topic = assignments[source.stem]
        units.append({
            "source": evidence_relative(source, ROOT).as_posix(),
            "sha256": hashlib.sha256(raw).hexdigest(),
            "lines": len(body.splitlines()),
            "topic": f"Docs/features/{topic}/index.md",
            "entry_points": sorted(set(re.findall(
                r"\b(?:void|bool|unsigned)\s+((?:install|start|reset|initialize|configure|register)\w+)\s*\(", body))),
            "comment_markers": len(re.findall(r"//|/\*", body)),
            "quoted_local_headers": re.findall(r'^#include\s+"([^"]+)"', body, re.M),
        })

    settings = (ROOT / "src/native/settings.cpp").read_text(encoding="utf-8-sig")
    options = [dict(zip(("name", "description", "key", "default", "minimum", "maximum", "step"), m))
               for m in re.findall(r'\{L"([^"]+)",L"([^"]*)",L"([^"]+)",(-?\d+),(-?\d+),(-?\d+),(-?\d+)\}', settings)]
    setting_rows = ["# Settings authority", "",
        "Generated from `src/native/settings.cpp` by the documentation audit. Numeric",
        "values are the persisted INI values; F8 presents their native mod value labels.",
        "Boolean 0/1 means Off/On. Voiced dialogue 0/1/2 means Suppress/Braille only/",
        "Speak anyway; Combat detail 0/1 means Normal/Detailed; Polish text 0/1 means",
        "Auto/Polish. Backend 0 is Automatic; available backend choices are supplied",
        "by PRISM. This is not a mod UI language catalog. See [ownership and extension](index.md#input-and-settings).", "",
        "Setting descriptions below are the current shipped help strings. For battle",
        "maximum MP, Shift+H/right stick down are the current all-panel controls; the",
        "older setting string still says H. [Controls](../../controls.md) is authoritative.", "",
        "| Setting / INI key | Default; range; step | Behavior |",
        "|---|---|---|"]
    for option in options:
        setting_rows.append(f"| {option['name']} / `{option['key']}` | {option['default']}; "
                            f"{option['minimum']}..{option['maximum']}; {option['step']} | {option['description']} |")
    (ROOT / "Docs/features/runtime/settings.md").write_text("\n".join(setting_rows) + "\n", encoding="utf-8")
    source_rows = ["# Production source map", "",
        "Generated by `Docs/Scripts/Offline/audit_feature_docs_20261018.py` from the",
        "explicitly reviewed responsibility manifest. Every top-level native C++",
        "translation unit compiled by the wildcard build is listed. Shared headers",
        "and native evidence are explained in the linked topics; the JSON inventory",
        "also retains source hashes, entry-point candidates and local dependencies.",
        "This inventory supplements explanatory review; it does not prove behavior",
        "or comment quality by counting files. Proxy, launcher and installer source",
        "are covered in [Distribution](distribution/index.md).", "",
        "| Production module | Primary maintenance topic |",
        "|---|---|"]
    for unit in units:
        name = Path(unit["source"]).name
        topic = assignments[Path(name).stem]
        source_rows.append(f"| [{name}](../../{unit['source']}) | [{topic}]({topic}/index.md) |")
    (ROOT / "Docs/features/source-map.md").write_text("\n".join(source_rows) + "\n", encoding="utf-8")

    # Local links and code filenames are checked only in the new explanatory
    # topics. Historical evidence retains its original text and named gaps.
    documents = sorted((ROOT / "Docs/features").rglob("*.md")) + [ROOT / "CODEX_MEMORY.md"]
    missing = []
    code_names = {p.name for p in (ROOT / "src").rglob("*") if p.is_file()}
    for document in documents:
        body = document.read_text(encoding="utf-8-sig")
        for target in re.findall(r"\]\(([^)]+)\)", body):
            target = target.strip("<>").split("#", 1)[0]
            if not target or re.match(r"[a-zA-Z][a-zA-Z0-9+.-]*:", target):
                continue
            if not (document.parent / target).exists():
                missing.append(f"{evidence_relative(document, ROOT)}: {target}")
        # Current-topic anchors should also remain usable when headings change.
        # Historical research has its own dated anchors; do not rewrite those
        # archives as a side effect of maintaining the new feature topics.
        for target, anchor in re.findall(r"\]\(([^)#]+)#([^)]+)\)", body):
            linked = document.parent / target.strip("<>")
            if linked.is_file() and "features" in linked.parts and linked.suffix == ".md":
                headings = re.findall(r"^#{1,6}\s+(.+)$", linked.read_text(encoding="utf-8-sig"), re.M)
                slugs = {re.sub(r"[^\w -]", "", h.lower()).replace(" ", "-") for h in headings}
                if anchor not in slugs:
                    missing.append(f"{evidence_relative(document, ROOT)}: anchor {target}#{anchor}")
        for name in re.findall(r"\b([A-Za-z0-9_]+\.(?:cpp|hpp|inc))\b", body):
            if name not in code_names:
                missing.append(f"{evidence_relative(document, ROOT)}: source filename {name}")
    if missing:
        raise ValueError("broken documentation references:\n" + "\n".join(sorted(set(missing))))

    bootstrap = (ROOT / "src/native/reader.cpp").read_text(encoding="utf-8-sig")
    output = evidence_path(ROOT / "Docs/Scripts/Offline/output/feature_docs_20261018")
    output.mkdir(parents=True, exist_ok=True)
    report = {
        "scope": "explicit native production translation units, settings, bootstrap and current topic links",
        "quality_limit": "source mapping and comment counts do not prove explanatory correctness; see reviewed topics/evidence",
        "units": units, "settings": options,
        "bootstrap_install_calls": sorted(set(re.findall(r"\b(install\w+)\(\)", bootstrap))),
        "documents": [{"path": evidence_relative(p, ROOT).as_posix(),
                       "sha256": hashlib.sha256(p.read_bytes()).hexdigest()} for p in documents],
        "broken_references": missing,
    }
    target = output / "inventory.json"
    target.write_text(json.dumps(report, indent=2) + "\n", encoding="utf-8")
    print(f"PASS: {len(units)} production units mapped, {len(options)} settings, "
          f"{len(documents)} current documents, no broken local/source links. {evidence_relative(target, ROOT)}")


if __name__ == "__main__":
    main()
