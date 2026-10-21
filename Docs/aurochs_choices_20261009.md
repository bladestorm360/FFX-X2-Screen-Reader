# Aurochs Reels paused outcome list — 2026-10-09

The user now requests a choice list for Aurochs Reels because its elemental
and status matches have useful effects. With Overdrive auto-success On,
Attack Reels alone applies its12-hit result immediately. Element, Status and
Aurochs pause the native sequence and countdown for player selection.
Auto-success Off retains ordinary manual play and the existing cue setting.
Sol implemented, reviewed and verified alone; no game launch/control/probe,
Tester Logs, real registry change or release ZIP change was performed.

## Evidence and implementation

The retained native patterns for565/644/691–698 all have exactly these common
Aurochs symbols:0/1/2/3/7/8/9/10. The actual module bytes were already hash-
qualified during the detection repair, so no new game-install investigation
or native extraction was needed. Reused evidence:
`Docs/Scripts/Offline/output/wakka_variants_20261009_144514/qualified_layouts.json`.
The [native result/effects evidence](swimming_controls_20261009.md#aurochs-matching-effects-clarified-in-this-turn)
explains the tactical reason for the choices.

automaticMatch now returns a result only for the verified Attack pattern.
Aurochs uses the existing openChoice builder and event/input owner,
which offers only symbols available on all three actual columns. It reads
localized command names through the established native getter:

1. Fire Shot
2. Ice Shot
3. Water Shot
4. Thunder Shot
5. Havoc Shot
6. Time Shot
7. Break Shot
8. Aurochs Shot

Up/Down browse; Home/End or Left/Right choose the first/last entry. Enter/Space
applies; controller D-pad/A provides the same mod-menu operation. Escape or
controller B resumes manual reels for that cast. The next cast opens a fresh
list. Applying a choice stops all three columns at the exact native accepted
symbol position and continues the ordinary native result processor.

Countdown F3F73C pauses at1 and restores its saved word; adjacent data remains.
Native callback ownership, exact PE hashes, actor/command gates, focus/scene/
generation cancellation, controller capture and manual cue traversal remain.
No extra producer, input automation, polling or text deduplication was added.

## Verification and delivery

The updated offline Frida `field_reels_followup.js` prototype and its pure
`check_field_reels_followup.cjs` check require a list even when all three
Aurochs columns contain10. They perform no runtime attachment or game call.
The changed native fixture fails against the prior production helper at
`Element, Status and Aurochs reels pause for selectable matches`.

After the repair, all ten real native PE byte fixtures pass all eight Aurochs
choices: fresh initial announcement, wrap/order, native callback pause,
unaccepted spinning columns, exact saved countdown/adjacent field, each actual
accepted symbol, Enter/Space continuation and manual cancellation without
reopening. The native DLL entry points/callbacks are never executed in tests;
the fixture substitutes its isolated native-control callback.
Attack still applies triple6 for12 hits; Element/Status retain their lists.
Existing activation/identity/context/cue/restoration regressions also pass.

Build, regression and guarded deployment evidence:
`install/aurochs-choices-20261009/verification.json`.
Both reader profiles, controller choice routing and the fresh proxy check pass.
The canonical x86 native/launcher build is warning-free. Closed-process
deployment, both compiled native profiles, launcher bootstrap and all13
installed hashes pass; the source/staged/fresh-fixture/installed proxy hashes
match. The ready development build is deployed.
Gameplay acceptance remains user-side.

## Swimming follow-up raised during this change

The user suggests verified minimap markers as a fallback for progression
locations without explicit events. The post-salvage dive is already exposed
using its invisible native type4 controller: entry6 tests depth Y greater
than250 and transitions to room288/entrance0. The native-named Exit, Dive
guidance and ordinary Auto walk target are already deployed. No marker
fallback is needed there. Retain context-qualified, verified minimap markers
as an option when another location lacks a usable native producer; this turn
does not claim a generic minimap-marker implementation. See
[the actual swimming transition](swimming_progression_20261009.md).
