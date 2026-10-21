# Menus, dialogue, Help and Sphere Grid

October21: [Formation membership and parameter confirmation speech](../battle/seymour_amount_prompts_20261021.md)
use the actual front-party roster and copied native prompt/Yes/No values.
Original descriptions and Autodetail ownership remain intact.

The October20 [password subject/input follow-up](../puzzles/airship_keys_20261020.md)
is implemented, offline verified and guarded-deployed with Baaj; gameplay is pending.
N/F11 says Password in the qualified airship password keyboard; ordinary naming
keeps Name. The broader feature statuses are in the [register](../status.md).
Autodetail is persisted and defaults Off; F7 toggles it and I reads the current
description. The complete settings/default and key tables remain authoritative
in [settings](../runtime/settings.md) and [controls](../../controls.md). Native
game-language text comes from game getters; locally authored UI labels remain
English, with no full mod-interface localization catalog yet.

## Native menu/detail flow

`field_menus.cpp::installFieldMenus` registers native field menu events and the
I/N detail/help routes. `field_details.cpp`, `field_targets.cpp`,
`aeon_attributes.cpp` and `game_details.cpp` obtain current native equipment,
items, abilities, quantities and character/Aeon information. Native text getters
and current selected handles are authoritative; labels must not be inferred
from a remembered row number. `selection_details.cpp` captures a context/detail
token so late description assembly cannot replace the newly focused screen.
`TextPlan` carries the ready data to worker formatting and the shared speech
adapter. Autodetail and a manual I request use the same selected context.
The field hook compares pre/post selection keys and captures a prepared text plan
at the native return point; record names/descriptions are resolved through the
screen's native getter family. Manual I dispatches in context priority order
(Escape/results, summon detail, status review, then ordinary selected detail),
so the focused screen retains a single producer. Context tokens and reset paths
retire delayed work when focus or screen identity changes. See the source locators
in the [coverage audit](../documentation_audit_20261020.md).

`system_menu_capture.cpp` and `system_menus.cpp` qualify actual system menu
selection/population events. `status_review.cpp`/`status_grid.cpp` assemble owned
review rows on preparation; native callbacks provide rows, identity, roles and
ordering facts. Reset/close invalidates the context even if a queued packet was
discarded. Keep descriptions, manual review, buffer arrows and Autodetail tied
to their screen rather than installing a global competing reader.

Detailed evidence: [field targeting and Customize](../../field_target_customization_20261008.md),
[menu follow-ups](../../menu_followups_20261014.md),
[inn/item/battle corrections](../../inn_menu_battle_followup_20261008.md),
[native row/buffer and sold-item follow-up](../../help_lines_kilika_20261016.md).
The older documents retain their exact native offsets/tests; later corrections
in the linked follow-ups take precedence.

## System screens

`save_load_menu.cpp` and system slot capture distinguish populated, empty and
unavailable slots, native focus and native help text. `shops.cpp` handles native
shop/sell/customization lists and prices; `confirmations.cpp` owns confirmation
prompt/choice changes. Customize's live equipment table is rebuilt after sales:
the selected handle and original visibility predicate reject sold/hidden
tombstones even when the numeric row has not changed. A stale menu index is not
an equipment identity. `naming.cpp` supports the game's actual naming keyboard
and selected text. [Password/name controls](../puzzles/airship_password_20261019.md#passwordname-keyboard)
explain contextual N/F11 readback, copied four-byte cells, the default-Off
punctuation-name setting, ownership and verification. These readers do not edit
saves or invent undisplayed items.

`monster_arena.cpp` is a separate conditional-menu reader for scene11, native
menu kinds8/11 and room307. Initializers arm an opening announcement; the input
return arms changed focus. Only the outer native draw at RVA36F960 may accept
expanded text from376A50. It matches native source identities for the selected
entry, optional opening title and displayed instructions, then copies the ready
bytes into `CopiedTextPacket`. This avoids speaking raw conditional script text
or treating the prompt as a selectable monster. Closing/result2, context or epoch
changes prevent a pending frame from committing. Native `drawDepth` and the frame
are thread-local; the worker receives bytes, not native source permissions.
The normal copied-text output/detail path remains authoritative. Monster Arena
render/focus cases are in the reader fixture; they are not an automatic battle
or capture solver.

Native menu rows/settings and owned review pages are different contexts. Most
screen readers do not have independent enable switches: Speech gates event
speech globally, while Autodetail independently controls descriptions. F8
settings and manual read commands stay available when speech is disabled. New
screens require their real native focus/population event, label/detail getters,
row identity and close/reset path. Test initial focus, rapid changes, empty/locked
rows, equal labels, no-Autodetail/manual I and leaving during preparation.
`run_tests.bat --readers` covers both reference and installed profiles; related
real getter/predicate replays are included in that host. Its offline coverage
does not establish every localized image page or physical reader output.

## Dialogue and review

`messages.cpp::installMessages` observes native TK windows/pages and subtitles,
preserving prompt, option and current focus as separate facts. `message_text.cpp`
decodes copied native tokens; `copied_controls.cpp` retains only displayed footer
actions. T repeats the current dialogue when its context is active. Repeated
native events with the same wording still have separate output. Native tutorial
protection and cancellation anchors prevent reader input from erasing a pending
page; those are lifecycle guards, not content deduplication.

`dialogue_voice.cpp` ties suppression to actual native voice playback, including
failed/muted/unvoiced paths. The Voiced dialogue setting offers suppression,
braille only, or speech anyway; silent braille mode does not substitute speech
when there is no braille display. `movie_dialogue.cpp` reads native captions and
audio ownership, including the separately supported Will executable.
`blitz_review.cpp` exposes contextual Blitzball information/review without a
second dialogue producer or automatic Blitzball solver. Jecht's special prompt
classification is scoped to its qualified minigame; it does not globally mute
dialogue. See [voice/PRISM](../../prism_migration_20261012.md),
[Blitzball and navigation](../../navigation_blitz_autowalk_20261007.md) and
[Jecht ownership](../../jecht_shot_20261014.md).

Run `--dialogue`, `--movie`, `--readers` and the affected speech-domain fixture.
Native voice/render/device dependencies simulated by those hosts remain
distinct from user-side playback confirmation.

## Help pages

These are the out-of-battle Tab-menu Help category, including pages titled
**Battle Mode**. `help.cpp::isHelp` qualifies the Help scene; it is not a battle
state reader. `drawing/decoded/rendered/drawn` observe native page identity,
accepted text/image draws, coordinates and selected entry. Native menu prompts,
characters, content and controls have separate `HelpRowRole` values. Preparation
sorts/splits the native line structure into owned buffer rows.

Entry announces the native prompt before the focused item. Unknown character
slots retain the displayed unknown value; the prompt is never row0. Ordinary
information pages support arrow review and Autodetail; I retains full-page
reading. The displayed footer is omitted from body rows and read through
Shift+I; a context with no footer says No controls displayed. Native directional
links take priority on menus. Reference menu left/right do not accidentally
become buffer reads; Basic Terms keeps its ordinary native arrow navigation.
Only verified locale-specific diagram words are reconstructed from images.

Page/context/serial guards admit legitimate same-page moves while rejecting a
late assembly after closing or changing pages. Keep captures at the original
Help update/draw phase: input PInput/field returns alone miss Help I/F7/review
commands. [Help identity, prompts and native links](../../help_navigation_quit_20261015.md),
[native lines/controls](../../help_lines_kilika_20261016.md),
[opening/escape corrections](../../opening_escape_help_20261014.md) and
`help_capture_followups.inc`, `help_native_replay.inc`,
`status_buffer_worker_fixture.cpp` provide
the retained original/worker cases. User confirmed buffer assembly; individual
new locales/image diagrams need their own evidence and confirmation.

## Escape and title

`esc_capture.cpp`, `esc_settings.cpp`, `esc_details.cpp` and system confirmation
handlers capture native focus, captions, values and control rows. Special
Features displays F1–F4 instructions; arrows/Enter are not substitutes for those
native toggle keys. Feature changes on the field announce feature and resulting
state. Quit prompt and highlighted Yes/No are separate events. `intro_cue.cpp`
owns the generic title instruction Press B or Menu/Start; it does not try to
guess first-launch input hardware.

[Title/settings](../../title_prompt_and_settings_20261014.md),
[opening and Escape](../../opening_escape_help_20261014.md),
[quit correction](../../help_navigation_quit_20261015.md) and
[current release note](../../release.md) explain native owners and checks.
New value readers must capture the actual accepted value/change, not merely the
key press. Preserve native input and independent F8 accessibility settings.

## Sphere Grid

`sphere_grid.cpp` owns native open/close/selection/character/activation/movement
events and contextual keys. `sphere_data.cpp` captures the selected native node,
layout links, positions, stock and text. `sphere_owned.cpp` transfers bounded
snapshots through tickets to its preparation domain; no borrowed getter/pointer
is used to compute routes on a worker. A maximum1024-node/1023-link fixture checks
the transport, plus source reuse/getter denial, reset recovery and no native
heap work. These component timings do not establish real audible/hardware latency.

`sphere_paths.cpp`, `sphere_sections.cpp`, `sphere_expert.cpp`,
`sphere_orientation.cpp` and `sphere_planner.cpp` own route/cost/branch behavior.
Standard character sections are qualified against the preserved full layout;
Expert uses its separate layout/stat focus. Ability planning belongs to the active
character and preserves independent destinations. Movement costs retain native
quarter-level travel history, locks split rounded movement commits, and key
requirements are cumulative. Directions use the recovered native selection
rules, distinguishing a required diagonal from alternative single-arrow choices.
Do not replace these with a simple angle rounding or relabel working links.

F2 chooses a character/stat path; F3 chooses an ability. F4/F5 guide forward/back
from the character; Backslash/Shift+Backslash guide from the preview cursor.
View+X/Y/A/B mirror F2/F3/F4/F5 and LT/RT mirror cursor backward/forward. The
October18 FIFO repair aligns both controller paths with the keyboard's native
return phase; it preserves the intentional difference between pawn and cursor.
These commands describe routes; they do not move the pawn or activate nodes.

`sphere_jump.cpp` provides F6's node-number edit dialog on a separate Windows UI
owner. It validates current Grid/context, range/native mode and defers the native
cursor operation to its approved owner. UI root/child focus is excluded from
game hotkey capture. Other F6 contexts remain independent.

See [Grid controls/directions/F6 evidence](../../tester_report_20261015.md),
[owned capture and pipeline](../../threading_audit_20261012.md),
[current controller report and limits](../../dylan_reports_20261018.md), plus
`sphere_fixture.cpp`, `sphere_sections_followups.inc`, `sphere_owned_followups.inc`,
`grid_native_replay.inc` and `input_dialog_focus_fixture.cpp`.
Run `--sphere`, `--readers`, `--input`, `--controller` and the dialog fixture for
changes across these boundaries. Extend layout/data/guidance coherently; never
infer missing direction errors by altering99% working navigation speculatively.
