# Battle readers, HUD, results and Overdrives

October21 follow-up: [HP/MP totals and effective Switch maximum](hp_sources_20261021.md)
traces committed values, base/effective capacity and action scratch forecasts;
percentages and the maximum-HP preference are preserved.

October21: [Mortiphasm elements, gil amounts and learned-mode announcements](seymour_amount_prompts_20261021.md)
documents the native producers, exact scripts/ABIs, copied substitutions,
offline evidence and separate deployment/gameplay status.

## Commands and event speech

`battle.cpp::installBattle` joins native action/damage/message/pop/menu/target
events with `battle_announcements.cpp`, `battle_status.cpp` and
`battle_graphics.cpp`. Action ownership, actor/target identity and native command
getters supply labels; `TextPlan` copies ready operands for worker formatting.
The installer path is centralized in `installBattle()`; it installs HUD,
graphics, damage/action hooks, result/announcement readers, status, Sensor and
Overdrive handlers. Hook callbacks capture facts while the game-owned arguments
are valid; prepared plans own text operands before the worker renders or sends
them through PRISM. Battle exit/reset clears actor/command state and context
tokens reject stale prepared output. Source-specific entry points are inventoried
in the [coverage audit](../documentation_audit_20261020.md).
The command menu/target cursor is authoritative, including target changes and
native descriptions. Combat detail and Autodetail add detail independently of
essential battle events. Context reset clears prior commands/actors rather than
reusing a completed cast on a new battle.

Graphical Miss/Guard/Overkill words are qualified by original sprite identity,
draw ownership and clip position. Status effects and damage are not inferred
from an English label. A new spell/ability must follow its actual native action,
overlay/result/callback path; do not globally speak every similarly named string.
[Original battle/cast corrections](../../inn_menu_battle_followup_20261008.md),
[native effects and related titles](../../reported_issues_20261010.md) and
[Stone Gaze/stack effects](../../open_issue_repairs_20261012.md) retain the detailed
offsets, module identities, body evidence and producer failures.

## Visible HUD

`battle_hud.cpp` associates final accepted/clipped native identifying and numeric
draws with a bounded per-frame snapshot. CTB order is derived from drawn actor
slots and screen order, not the longer hidden combat queue. Transport capacity
is32 turns; actual output contains only accepted visible turns. The same rule
admits at most three drawn Overdrive meter rows and HP/MP panels. A character
being in the party/engine table alone does not make its meter visible.

Shift+T/right stick right read visible turns. O or the battle mod face binding
reads the active character's meter; Shift+O/right stick left read all visible
meters. Shift+H/right stick down read visible HP/MP panels. Show maximum HP/MP
settings control presentation; current HP can use percent versus maximum. Native
right-stick input is consumed in battle for these commands. Existing saved face
bindings are respected. Combat detail defaults Normal/off, Autodetail defaults
Off, Overdrive auto-success defaults Off, manual Overdrive cues default On,
maximum HP/MP defaults Off, and Speech defaults On; all values persist in the
Accessibility INI. Exact key/binding names and ranges are in the linked controls
and settings references. `battle.cpp` owns these requests; do not add a competing
controller HUD reader. [HUD and controls](../../battle_panels_controls_20261016.md)
and [clipped render evidence](../../active_puzzles_battle_controls_20261016.md)
document the exact hooks, copied operands, native display modes and fixtures.

## Details and effects

`battle_sensor.cpp` exposes the game's actually available Sensor information and
context-owned review keys. `battle_status.cpp` reads current native statuses;
`status_animation.cpp` qualifies real per-target stack-buff applications through
the actual native effect/DLL callback lifetime. Direct/nested native messages,
Miss/Guard, consumed petrify/shatter and six independent party buff applications
must not be collapsed to one cast label or deduplicated by text. Ability titles
come from native command ownership so Stone Gaze and related actions are read
without hardcoded timing guesses. [Sensor and meters](../../sensor_overdrive_swimming_20261008.md),
[stack animation evidence](../../status_animation_timing_20261009.md) and the
corresponding reader fixtures explain exact table/callback ownership and limits.

## Results

`battle_results.cpp` captures initialized native reward data and per-character
AP/level baselines at8BDB30; completion at8BA620 produces actual gains. Owned
`status_review.cpp` rows distinguish mandatory earned totals from optional detail.
EXP earned, AP earned, gil and Sphere Levels gained speak with Autodetail Off.
Per-character EXP distribution and item/equipment drops remain review/manual
detail or Autodetail content. Reopening/review and multiple equal gains preserve
their events. Native before/after baselines prevent uninitialized/stale values
from being read as new rewards. [Current result-row contract](../../help_lines_kilika_20261016.md)
and `results_esc_followups.inc` are the authoritative regression.
Dylan explicitly accepted the drops/manual-detail behavior in the current report.

## Overdrives

`overdrive.cpp` separates default-Off auto-success from default-On manual cues.
Tidus uses the actual success-zone phase; Auron initialization captures his
ordered native button sequence and configured binding names; Lulu reports native
cast count. Only qualified active minigame state receives optional native success
writes. `auron_sequence_20261009.md` and `wakka_sensor_beeps_20261008.md` explain
the recovered data/control semantics, not inferred generic timing.

`wakka.cpp`/`wakka_identity.cpp` qualify loaded effect images and callback/state
layouts before reading symbols or assisting. Scene-dependent overlays and
initialization-to-running handoff matter: a requested/starting effect is not yet
permission to read its eventual worker table. Attack Reels auto chooses the
double-hit icons; Element/Status/Aurochs use a paused native best-match choice
menu. Aurochs Shot is the verified triple/result, correcting the old Power Shot
label. Manual cues track native visible symbols. No guessed DLL offsets or
unqualified same-shaped callback are accepted.
[Wakka module ownership](../../wakka_detection_20261009.md),
[Aurochs choices](../../aurochs_choices_20261009.md),
[Auron sequence](../../auron_sequence_20261009.md) and
[field/reels details](../../field_reels_mp_followup_20261009.md) retain the evidence.
Other characters do not acquire an automatic timing solver merely because the
Overdrive assistance setting exists.

## Safe changes and verification

Add native HUD information at the accepted render producer and carry owned
identity/order/clip facts through worker formatting. Add effects at their real
cast/result ownership, retaining one producer per feature. Context exit, focus
loss, scene changes, setting Off and stale snapshots must retire output and cue
audio. `nav_audio.cpp` owns the audio callback; a battle callback never opens a
device or waits for speech.

Run `run_tests.bat --readers` for both profiles and the affected `--minigames`,
`--navigation-audio`, speech/worker fixtures. These category names identify the
host fixture suites, not a claim that every individual feature is gameplay
confirmed. The Oct20 register marks Stone Gaze implementation complete while
gameplay confirmation remains pending. Original native row/overlay replay is
stronger than a synthetic expected string, but it still does not prove real
render/device latency. Current receipts identify native getter/SDK simulations
and remaining gameplay checks. Runtime captures or game control require separate
permission; do not convert an old working report into a new confirmation.
