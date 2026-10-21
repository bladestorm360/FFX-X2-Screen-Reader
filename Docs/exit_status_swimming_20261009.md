# Exit labels, Grid Status and swimming follow-up — 2026-10-09

October12 source maintenance implements Airship internal exits/elevators and menu speech, Polish text detection/override, Grid Status detail/review, displayed Overkill, current Overdrive help, the linked Salvage Ship underwater exit, Kilika treasure continuation, and Macalania goal choice. Via Purifico adds both progression exits plus a manual panel guide and optional native-script collect-all action. Offline validation/build/deployment status: [repair record](open_issue_repairs_20261012.md). Gameplay confirmation remains user-side; the October9 release ZIP is unchanged.


## Player-reported open issues carried into 0.5.5

The latest user test supersedes the earlier offline-only Grid Status and Dive
coverage claims below. V cycles five Grid Status pages but unconditionally
speaks the full copied buffer even with Auto detail Off. Only one page reads
details; the other pages announce only the character's name (reported as Tidus).
The copied review buffer and private-slot hooks do not establish five-page
coverage or correct automatic-detail policy. Both issues remain open.

After the characters jump from the Salvage Ship, the southward approach to
the passage containing the Salvage Ship and Underwater Ruins exits is still
absent from navigation. This is an approach/passage marker, distinct from the
already labelled exits inside that passage and the implemented descent target.
Players must currently swim south manually. A future marker must reflect
diving state and actual reachability; do not claim this route is solved based
on the retained depth threshold alone. These issues are explicitly documented
in README.md, CHANGELOG.md and Docs/controls.md for release 0.5.5.

The release request is to document these limitations and package the existing
implementation. No feature fix or gameplay confirmation is part of that request.

The user's current-session log confirms Surface navigation reported zero exits
while retaining ship actors. Later Underwater Ruins scans found workers37/38
with room71, whose destination name was omitted. The Sphere Grid tutorial
used scene22 and produced no Status-page speech. These reports supersede the
earlier offline-only success claims in the swimming and Status notes.

Sol investigated and implemented alone. No game launch/control/live probe,
Tester Logs, third-party source changes, registry actions or publication.
Existing retained EBP, native decompile and reference machine code sufficed;
no new game-install asset reads or gameplay probe were needed. The referenced
GhidraReference guide remains unavailable; the retained read-only workflow
and timestamped analysis conventions were used.

## Destinations and dungeon passages

`audit_exit_destinations.mjs` follows every useful player-entry graph and its
REQ destinations in all397 retained variants. It records657 physical exits
and exact room/arrival-point alternatives. All graphs are complete. Previous
records lost39 room identities (often because the arrival point was variable)
and omitted42 known rooms with multiple area names. The new table resolves
native destination names for650 exits. Seven retain numbered labels because
some destination rooms have no extracted area data; their exact identities
are listed in the audit. No floor or destination is invented.

Exit numbering uses authored worker order within the hashed script. It is
independent of distance sorting, filtering and temporary availability. Multiple
passages with the same native name remain distinct. Underwater Ruins worker28
goes to room380, whose area24 is also named Underwater Ruins; it is Exit1.
Workers37/38 both return to room71/0 and are Exit2 and Exit3 to Salvage Ship.
The underwater swim's worker5 is Exit1 to Underwater Ruins; worker6 is Exit2
to Salvage Ship. Names always use the native localized getter.

Area selectors are **not entrance IDs**. Native470E90 selects the header area
table using collision/ATEL state; actor+828 bits11..12 supply one selector.
46BD20 instead returns geometry MapNo. For ambiguous destination areas, the
label uses a shared native location prefix or states the native alternatives.
It does not guess Deck/Surface or a dungeon floor from the warp spawn value.
Current-map/arrival names now use the published native saved area at field+E,
validated against the primary chunk's area table, preserving ActiveField/title
guards. This also corrects the earlier fixture's incorrect MapNo-as-name stub.

Central producer: `src/native/exit_labels.cpp`; generated data comes from the
complete offline audit. Existing target membership, callbacks, geometry,
save-sphere classifier and navigation controls are preserved.

## Dive progression

The depth goal retains the real cdsp0000 worker30 identity and strict Y>250
transition to room288/0. Native42D200 checks controlled actor+186 motion2 and
the Dive-enable byte. It does not require actor+185 or gravity+182 to equal2.
The scheduler's current signal/PC is transient and need not be in entry6 at
the instant navigation is requested. These unsupported visibility conditions
are removed. The goal still requires the exact catalog, primary chunk,
room71/native area20 (Surface), active control and valid controlled swimmer.
Refresh/cancellation retain these conditions.

The target says Dive down and names the actual configured Dive control.
Navigation review reports the remaining percentage using native Surface
entrance Y25 and the real Y250 transition threshold. It updates from current
player Y whenever guidance is requested. Auto walk keeps neutral horizontal
input and uses native Dive. No fixed seconds are promised: native buoyancy,
vertical velocity and frame progression affect duration. The subsequent
underwater physical exit supplies ordinary direction and route guidance.

## Actual Sphere Grid Status

The regular Status controller4D1DD0 is not used by this panel. The previous
fixture simulated that controller with the Grid flag and therefore could not
reproduce the real failure. Constructor654B40 builds private pool slots1..5.
65A270 opens a native window; 659800 closes it; 658EC0 changes Grid character.
These actual mutations publish/retire pages through the shared Status review
buffer. Native fade updates do not repeat page speech. No draw polling,
timers, passive memory watching or text deduplication was added.

64FA30 verifies the ten ordered stat kinds and maximum-only HP/MP renderer.
The reader uses the actual localized row labels and those saved values; it
does not insert current/max pairs, equipment, Overdrive or Next AP from the
regular Status page. Ability rows use the actual39A4C0 getter called by4F0590,
and skip the native hidden/unlearned entries. Arrows and comma/period review
the copied rows. Grid planner keys and Grid node Help yield to active Status
review, and normal Grid controls return on close.

All three new producer bodies have unique relocation-masked archived signatures,
including the full native character-switch body; startup/preflight requires
their independently resolved addresses. Ordinary Status behavior is retained.

## Verification

Reader regressions pass with131 hook sites for both executable profiles:
actual private-window mutations, ordered stat/ability rows, character changes,
fade silence, close behavior, sparse real EBP scripts, stable same-name exits,
changed-script rejection, unscheduled depth-target visibility, depth progress,
native controls, route/Auto walk dispatch and live cancellation guards.
Profile, navigation, final build/proxy and guarded deployment results are
recorded in `install/exit-status-followup-20261009/verification.json`.
The final warning-free native build, fresh proxy forwarding and guarded
deployment passed. Both actual compiled executable profiles, launcher
bootstrap and all13 installed SHA256s were verified. Scoped retained evidence
is `Docs/Scripts/Offline/output/exit_status_20261009_200145_700895/`.
Gameplay confirmation remains user-side.

Test Surface: choose the depth Exit, hear the configured Dive control and
remaining descent, then hold Dive or use enabled Auto walk until the map
changes. Choose Exit1 to Underwater Ruins for the following swimming route.
Check distinct numbered Ruins passages and the Grid Status opening/arrow
review/character-switch/close behavior.
