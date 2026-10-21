# Salvage ship swimming progression — 2026-10-09

The later user test refuted target visibility. The unsupported motion/gravity
and transient scheduler guards, depth guidance and missing destination labels
are corrected in [the later follow-up](exit_status_swimming_20261009.md).
That document supersedes the earlier visibility/guard claims below.

The player reports no navigation target in the swimming section whose HUD has
Dive and Return to ship. The earlier swimming fixes covered native HUD review,
Auto walk cancellation and disconnected ship decks. They did not expose this
scripted progression goal. Sol investigated, implemented and verified alone.
No game launch/control/live probe, game-install source investigation or Tester
Logs access was performed. Only the mod's own current log was searched; it did
not contain the older swimming section. The retained extraction and native
decompile resolve the missing condition offline.

## Verified progression condition

The two-option HUD is `cdsp0000` string107. Deck and Surface share this script,
room71. Worker30 is an invisible type4 controller, which normal physical
navigation discovery excluded. Its entry6 starts at60444, gets worker0's
position at60456, tests its second position component greater than250 at60466,
and transitions to room288 entrance0 at60498. The normal catalog has no
collision target for this worker.

The second component is **Y/depth**, not a southward X/Z boundary. Native
`FUN_0086A280` writes X, Y, Z into the respective position destinations. In this
map depth increases downward: the initial swim uses water59; crossing Y250
advances to Salvage Ship - Underwater. Therefore the new target explains Dive
instead of inventing a horizontal south destination. The subsequent underwater
map already has native line exits to the ruins and back to the surface.

Native `FUN_0082D200` requires swim motion2 and the Dive-enable byte0130077D,
and adds0.2 to actor+504 when held input40 is present. `FUN_0082A910` writes that
vertical velocity. `FUN_00832E90` applies it to actor Y independently of X/Z
movement, including the native buoyancy and floor constraint. Holding Dive can
therefore advance this condition with neutral horizontal input. Native physics
and ATEL retain all movement and transition ownership.

Retained evidence:

- `Docs/Scripts/Offline/output/swimming_progression_20261009_051440_535541/`
  contains compiled cdsp0000/cdsp0100/cdsp0700 control flow and native text.
- `Docs/Scripts/Offline/inspect_swimming_progression.py` rechecks the actual
  archived EBP,63240-byte script/FNVf5f6d993, worker/entry/condition/transition
  and the retained native functions. Fresh output:
  `Docs/Scripts/Offline/output/swimming_goal_20261009_125716_474764.json`.
- Dive-enable RVAf0077d is independently derived from operand1170 of the full
  bounded1620-byte native42D200 body. Signature uniqueness evidence:
  `Docs/Scripts/Offline/output/native_extensions_20261009_124220_894066.json`.
  A missing dependency refuses startup/preflight for the supported profiles.
- Offline Frida prototype `Docs/Scripts/Frida/FFX/swimming_progression.js` and
  `Docs/Scripts/Offline/test_swimming_progression.cjs` validate the same goal
  predicate before the native port. This reference is not bundled or enabled
  as a second production producer.

## Implementation

Shared `swimming_navigation` helpers expose worker30 only after exact existing
catalog/hash qualification. The live guard requires its entry6 signal and PC
60444..60510, the same primary field chunk, room71, controlled/enabled swimmer,
mode/motion/gravity2, enabled Dive and ordinary field control. Refresh retains
the verified chunk identity; returning to the deck, another phase or a context
change retires the goal. No text-content deduplication was added.

The Exit category names the native destination through the existing area getter.
Selection and the route key say “Dive. Hold [configured button] until the map
changes.” The depth goal stays available without a collision mesh and through
the unreachable filter. A vertical route uses current X/Z and endpoint Y251;
arrival tests the actual strict Y greater than250. It does not emit a misleading
horizontal “here” beacon or face an invented interactible.

With Auto walk On, the ordinary route command holds the configured native Dive
control, using shared configuration helpers for primary/alternate keyboard
scans and physical controller buttons/triggers. It leaves horizontal movement
neutral. Manual movement or face buttons, including alternate Return to ship,
retain same-poll priority. Existing focus, menu, dialogue, cutscene, generation,
stale-input and15-second progress guards remain. Vertical progress does not
cause horizontal unstick attempts. No position, velocity, script, timer or
progression-state writes were added.

## Offline verification and delivery

Both production reader profiles pass real archived EBP discovery/native naming,
entity selection, filter, mesh-free route/Auto walk dispatch, strict depth
arrival and all live producer/control guards. Profile dependency/refusal checks,
Auto walk keyboard/controller remaps (including trigger/alternate controls),
manual cancellation, vertical progress/stall release, audio/arrival, navigation
geometry and pedestal regressions pass. Initial fixture failures were corrected:
the native area-name table was too small, and an earlier fixture had left the
Sphere Grid flag set. Neither was a production defect. One build warning from
a binding-table name collision was removed before the final build.

Logs and final delivery record are under `install/swimming-goal-20261009/`.
Final build/deployment verification is recorded in `verification.json`.
Gameplay acceptance remains user-side: target visibility, actual Dive progress
and the resulting map transition. The release ZIP and registry are unchanged.

For testing, scan the Surface section with the Dive/Return to ship HUD, select
the new Exit, and press Backslash with Auto walk On. With it Off, follow the
spoken configured Dive control manually. Manual movement or Return to ship
must cancel assistance. Existing map arrival speech should announce the new
map after the transition completes.
