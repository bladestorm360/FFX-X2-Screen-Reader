# Baaj Antechamber seals

October23 update: the user confirms Besaid sphere collection in gameplay.
The remaining check is a remote tester reproducing Baaj Ruins unlock through
normal gameplay without warps. A tester reported Zanarkand missing, but its flag
was already present in the supplied save; that report does not establish a
missing Zanarkand flag. The earlier records below describe their original saves
and checkpoints, not the newly confirmed Besaid collection.

October22 reopens the separate Auto-collection report. The named save still
contains0x3E: Besaid missing, Zanarkand and the other four present. Earlier
acceptance below concerns native seal activation/labels, not proof that Auto
retains every destruction-sphere flag. [Current collection repair and limits](../puzzles/destruction_flags_20261022.md).

Implementation is **complete**, with **user gameplay confirmation on October20**.
The user confirms Besaid starts and remains inactive because this save lacks
its destruction-sphere treasure, while every eligible seal works. The user
accepts the result and closes the reported seal issue. Final wall opening after
obtaining all six treasures was not confirmed in this save. The Cid/Baaj diagnostic changes passed both reader profiles and a warning-free
developer build, then guarded deployment with all seven hashes matching.
[Current delivery](../../../install/dev-console-20261020/cid-baaj-verification.json).

## October20 gameplay confirmation and installed-save correction

The latest user test supersedes the earlier Macalania/Besaid interpretation.
Besaid was correctly labelled inactive from the start and stayed inactive; the
user confirms that the installed save lacks its destruction-sphere treasure.
Every seal whose treasure is present works. The user says the earlier seal may
have been mistaken for Besaid and explicitly accepts the fixes as good. No
further diagnostic capture is required for this closed report. This confirms
eligible activation and missing-prerequisite inactivity, not final door opening
with all six prerequisites or every unrelated navigation feature.

The installed Baaj file at the game's Documents save directory, `ffx_000`, was
read directly and is byte-for-byte identical to `Saves/Dylan/Summon/ffx_000`:
SHA256 `470724d9bc6176ffa170357ff9a72921a09200ca54395add4a048a252a836f3e`.
The earlier statements calling this an older/different save were wrong and are
withdrawn. The installed-file comparison is preserved outside Git in
`Analysis/FFX/Summon_20261020/installed-save-recheck-20261020.json`.
Neither save was changed. The latest user gameplay report now agrees with
the missing Besaid prerequisite in this file.

Sol traced the six actual temple treasure writes and the six original Baaj
entry2 guards independently. The authored labels in the table below match those
flags. Djose and Macalania contain alternate historical graphic cases; their
actual constant Y selectors,69 and41 respectively, choose masks4 and8. Picking
the first textual write in either script would be insufficient. The original
audit `audit_baaj_seal_labels_20261020.py` resolves those branches and cross-checks
the authored catalog. Its original models remain outside Git under
`Analysis/FFX/Summon_20261020/label_recheck_20261020`.

The expanded regression discovered a separate real error: worker34's missing
Macalania flag jumps to PC7072, a player-control restoration call93 followed by
return. The generic proof stopped at that call and retained availability. The
other five missing-flag branches go directly to return. This defect could
overstate availability. It is a separate verified proof defect; the user's
latest gameplay confirms the missing treasure is Besaid and eligible seals work.

`callback_guard_facts.hpp::unavailableCallbackEpilogue` describes only this exact
script name,7530-byte length,FNVd13cfb8c,worker34,entry2,PC7072 and call93.
`callbackAvailableOwned` recognizes that no-activation branch; the native proof
also requires the prepared allocation identity and matching worker header.
Neither runs or generally skips a game call. Unknown calls/identities still
retain interactions. Preparation remains on the existing owner; native refresh
uses bounded ready lookups, with no parser, hashing, waiting or new hook.

Both171-hook profiles now pass **all64 combinations of six treasure flags**,
checking all six labels and native/owned availability against each mask. Both
also pass540 original native endpoint comparisons each. Wrong script identity
does not inherit the epilogue fact. The build retains the developer console and
exclusive Shift+F1 behavior. Current gameplay/save interpretation remains
separate from these offline results.

## Retained diagnostic contract and offline evidence

`Scene` now carries three owned diagnostic facts: readiness, treasure flags and
successful-activation count. During the existing native scene observation,
`captureBaajFacts` requires room196, original `bjyt1000` length7530/FNVd13cfb8c,
37 workers and W30's current prepared allocation identity/header. It performs
only two bounded read-only native variable getters: raw declarations01000A70
(saved treasure byte) and0C000000 (map-local counter at resolved map offset2CCC).
The latter's raw offset is zero; its resolved map address must not be mistaken
for the raw declaration. No new hook, timer, game write or worker pointer is added.

On the owned preparation side, `navigation.cpp::execute` logs one state header
and at most six seal rows only for explicit Scan/Next/Previous/Distance/Path/
Filter commands in room196. Background movement/Refresh does not emit the dump.
The header includes query/epoch, copied-bank validity/flags, independent native
flags/counter and readiness. Rows identify local/global workers, captured native
channels, final inactive/geometry state and current position. This bounded event
guard does not deduplicate speech or suppress separate user commands.

The diagnostics were added while the earlier Macalania-labelled activation
report was unresolved. A counter below6 with a missing prerequisite differs
from all six successful sequences followed by an unlock failure. Copied-bank validity and native flags separate an unknown proof
from actual game state. Static models and synthetic callbacks cannot establish
those values in the reported live session; no agent game launch/control/probe
is authorized. The existing native-reader own log is the evidence destination.
No Tester Logs were accessed.

The actual-capture fixture verifies the original W30 declarations/resource
service with controlled native getter values, including valid native facts when
the copied saved-bank owner is unknown. The diagnostic build is guarded-deployed.
The actual native counter was not observed by the agent. The user's subsequent
gameplay confirmation closes the report without requiring another log request;
retain these read-only diagnostics for a concrete future recurrence.

## Save and prerequisite finding

The named save `Saves/Dylan/Summon/ffx_000` has SHA256
`470724d9bc6176ffa170357ff9a72921a09200ca54395add4a048a252a836f3e` and was
last written `2026-10-20T10:18:16Z`. The independent Fahrenheit SaveData
definition names payload offset `0xC5C` as `anima_seals_unlocked`; the save
contains `0x3E` there (`0xC9C` in the file including its 64-byte prefix).
Native Besaid script `bsyt0100`, worker 16, sets mask `0x1`; that bit is clear
in this supplied save. This establishes only the saved state of this file. It
does not establish arbitrary later unsaved progress. The user's latest test
independently confirms the missing Besaid treasure and correct seal inactivity.
No save data was edited.

The saved room identity is Baaj's Ruins – Antechamber, native script
`bjyt1000`; the prior room is underwater passage `bjyt0800`. The retained
original event has 37 workers, code length 7530 and code FNV `d13cfb8c`.
Evidence was generated outside Git at
`D:/Games/Dev/Custom/FFX/Analysis/FFX/Summon_20261020`; see the
[repository-content record](../repository_content_20261020.md) for the
completed external-input/history migration. No Tester Logs or installed game
assets were read.

## Implementation and data flow

`navigation_passages_data.hpp` adds six qualified overlays for `bjyt1000`
(length 7530, code FNV `d13cfb8c`), identity 3 and entry mask 4:

| Worker | Seal label | Region center X/Y/Z | Half-width/depth | Native entry prerequisite mask |
|---|---|---|---|---:|
| 30 | Kilika | 25 / 0 / -30 | 9 / 9 | 2 |
| 31 | Djose | 25 / 0 / 0 | 9 / 9 | 4 |
| 32 | Bevelle | 25 / 0 / 30 | 9 / 9 | 16 |
| 33 | Besaid | -25 / 0 / 30 | 9 / 9 | 1 |
| 34 | Macalania | -25 / 0 / 0 | 9 / 9 | 8 |
| 35 | Zanarkand | -25 / 0 / -30 | 9 / 9 | 32 |

All six stay represented in the scene catalog, including entries whose native
callback is inactive; an inactive seal remains listed and is identified as
inactive. Native walking triggers the seal; destination selection
and route arrival do not inject Talk or Confirm. Besaid's seal remains subject
to the game's own prerequisite and callback state.

The shared `native/interaction_region.hpp` decoder is used by copied-scene and
live interaction paths. It accepts only player-reference (`-1`) type-6,
channel-5 regions with valid finite, non-collapsed geometry. Native stores the
type-6 angle with the opposite sign from the existing type-3 route
representation: the original type-6 body negates it at RVA `0x467C09` using
the retained double `-1` at `0xB92340`. The shared decoder applies that
negation when normalizing to the existing shape-3 interaction-zone form,
marks it as a crossing trigger, sets route range to zero and applies a 0.25
inward margin.
This intentionally routes to a point assured to lie inside the region. The
original native region test owns actual entry detection, including near-envelope
behavior; channel 5 is the entry trigger. The mod does not make channel 6
(leave) an interaction destination.

`playerEntryRegion` is the sole conversion used by
`navigation_scene.cpp::sceneInteractionZone` and
`nav_interactions.cpp::interactionZone`. It consumes the existing76-byte geometry
snapshot, never a native pointer on the preparation owner. Native geometry is at
worker+284: center+0/4/8, signed reference+28, half-width+30, stored angle+34,
half-depth+38, height+1C and near envelope+48. Native callback capture checks
worker+34 enable bit80 and worker+AB channel5. Bits100/40 are occupancy history,
not disabling flags. The ordinary resource serial/epoch and current callback
facts retire stale selections through the existing scene/intent pipeline.

`navigation_scene.cpp::buildPart` limits this support to qualified type-6,
channel-5 triggers and keeps known identity-3 records even when the native
callback is inactive. `nav_interactions.cpp` captures and refreshes the native
entry-channel region through the shared decoder. `nav_exits.cpp` retains the
same interpretation for the legacy route path. The normal copied-scene owner,
native-thread affinity, refresh and cancellation rules remain in force; this
adds no hook, polling loop or wait for mod work on the game thread.

The native entry guard uses indexed scalar load opcode A2, element0. Both
bounded callback proofs now support that saved-scalar form; nonzero/dynamic
indices remain unknown and retain interactions. The first regression exposed
all six as active despite the clear Besaid flag because the old proofs stopped
at that unknown opcode. Current native-adapter and owned proofs agree on the
supplied five-bit state and an independent all-prerequisite copied state. The
original save is never modified by those matrix cases.
The owned proof is `owned_callback_proof.hpp::callbackAvailableOwned`, called
through the prepared resource lease with copied `AtelSavedFacts`; the native
refresh proof remains `nav_interactions.cpp::callbackAvailable` with its native
getter on the native owner. Neither proof executes an ATEL write or interaction
callback; only the native version enters the existing read-only variable getter.

Native type-6 dispatch is at dispatcher RVA `0x466680` to handler RVA
`0x467BB0`. The original angle negation is at RVA `0x467C09`. Default callback
resolution is at RVA `0x476360`; the type-6 callback table is at `0x852B08`,
where entry channel 5 resolves to script entry 2 and leave channel 6 to entry
3. Native priority resolution is at RVA `0x4767E0`. The offline fixture
`baaj_navigation_followups.inc` exercises production endpoints against the
retained original native event body (1027 bytes) and distance body (50 bytes).
The original sine/cosine wrappers correspond to CRT IAT calls; the fixture
models their x87 `fsin`/`fcos` boundaries and controls native callbacks and
floor behavior. It covers prerequisite flags, all six identities, inactive
records, trigger geometry, several rotations, production endpoint behavior
and rejection of invalid references or geometry. Both171-hook reader profiles pass, including540 original native endpoint
comparisons per profile. The rotated-angle case initially failed until the
shared decoder followed the original angle negation. Native indexed-guard and
owned-guard results, inactive retention, invalid-reference/collapsed geometry
and unknown nonzero array indices also pass. The offline audit tool validates
the independent save layout and writes source-bearing evidence outside Git.
The current complete discovery output is
`summon_statues_20261020_110729_078230`.

## Native behavior and limits

Original script entry 2 handles region entry; entry 3 returns. Startup entry 1
disables each region after final completion. On successful activation the game
retires that region, updates its native visual/camera effects and increments
a map-local count. The sixth activation opens wall 48 and sets persistent mask
`0x40`. The save flag does not disclose current per-region enablement, live
counter state, exact player position or a safe route.

This task adds only the six qualified Baaj identities. The decoder can serve
other catalogued player-entry type-6 regions when their useful destination
identity and callback contract are verified; geometry alone is not sufficient
to catalog an interaction. Preserve native trigger semantics and cancellation;
do not synthesize interaction input.

The user-facing labels are currently English. Translation into supported game
languages and any appropriate language catalog work remain an open localization
item; current source/offline completion does not imply gameplay confirmation.

## Prioritized gameplay checks

User-confirmed: Besaid starts/remains inactive without its treasure; all seals
with their treasures activate correctly. The user accepts the result and closes
the report. No repeat test or additional diagnostic request is required.

If progression later permits, final wall opening after all six prerequisites
and leaving/reloading or stale-route retirement remain broader regression
coverage. These are not unresolved failures in the accepted save. Do not mark
final door opening gameplay-confirmed from the five eligible seals alone.

## Investigation provenance

The project-owned audit is `Docs/Scripts/Offline/audit_summon_keys_20261020.py`.
It reads only the named save and retained evidence, preserves timestamped
outputs and refuses to write under any Git tree. Native and save analysis was
performed by Sol without delegated reverse engineering or gameplay probes.
The source-bearing archives and original replay bodies now live outside Git;
the repository-content record describes the loaders, guards and history cleanup.

## Delivery, bounds and review

Historical first Baaj delivery: canonical release/launcher build `src/proxy/build_dinput8.bat --release --no-deploy`
passed without warnings. Canonical two-profile readers, geometry/cross-water/
availability/routes and all-trial pedestal input/retirement passed. Reader
compilation retains the existing C4389 warning in the older airship fixture;
there is no new fixture warning. Canonical `src/proxy/deploy.bat` then passed
closed-process, native profile/dependency, launcher and installed-hash checks.
All seven installed artifacts match the verified payload; the save, INI and
0.5.7/0.5.8 ZIPs are unchanged. The deployed DLL SHA256 is
38a07209838c4b81820d64b8c4eedfff45f77b7abfe6bfba0060b2e9ef395f84.
The prior audio/Shift+M/function-key/password repairs are included; no installer
or ZIP was rebuilt. Logs and source hashes are retained in the linked receipt.

The original six-region change reused76 geometry bytes per worker, with a500-worker
bound. The later diagnostic adds three owned scalar fields and exactly two
qualified scalar getters in room196, as described above; no getter loop is added. The shared region
conversion is fixed scalar work with no allocation or waiting. Native/owned
saved guards retain their256-instruction bound; indexed element0 consumes one
existing stack value. Other array elements remain unknown rather than adding
an array walk. Prepared routing owns the existing finite rectangle anchors and
mesh work/cancellation limits; the native owner still supplies live geometry,
callback flags and resource identity. There is no new hook, timer, worker join,
game-thread parser or text-content deduplication. Offline generated samples and
controlled/native replay do not measure actual game callback cost, data age or
player-to-audible latency; no new runtime responsiveness claim is made.

Safe extensions require a qualified script identity and useful callback, correct
reference ownership and native region semantics, then both copied/live decoder
and route/arrival checks. Identity3 is only for explicitly authored region
records that must remain identifiable while inactive. Do not apply it broadly
to all technical zones or change the native prerequisite/completion flags.

Luna (`gpt-6-luna`, medium) updated the bounded Baaj/player documentation from
Sol's verified facts. Sol reviewed against source, corrected inactive-retention
and supplied-save wording, incorporated the A2/angle discoveries and validated
the production/native tests. No unresolved issue remains in that delegated
scope. Sol retained all discovery, implementation, verification and deployment;
no Astra or delegated reverse engineering was used. Actual gameplay and the
remaining gameplay confirmation remains distinct from offline verification.
