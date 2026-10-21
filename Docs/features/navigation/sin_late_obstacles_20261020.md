# Later Sin rising and disappearing objects

Implementation is **complete**, offline verified and guarded-deployed. Exact City and Nucleus map assets were explicitly approved for read-only offline extraction. Both original floors pass replay against native collision behavior. Four additional City platform boarding destinations pass exact native rectangle route checks and both171-hook reader profiles. The warning-free current-source build and all seven installed hashes pass. Gameplay is unconfirmed. [Current delivery receipt](../../../install/sin-maps-20261020/verification.json) also links the earlier gate/collection delivery. Settings, named saves and release ZIPs are unchanged; installer/package rebuilding remains held.

The user distinguishes the rising objects after Seymour and before the final
Save Sphere from collection beyond it. Sea of Sorrow's repair is
[already delivered](sin_sorrow_routes_20261020.md); the optional collection
helper has a [separate contract](../puzzles/sin_collection_20261020.md).

## Native mechanics and evidence

Retained scripts identify room204 `sins0400`, City of Dying Dreams,45865 bytes,
100 workers, FNV32 `01aaeb94`, EBP SHA256
`890748eedb4fb7d00623687751816b1dcb41b00f7161e64930f54485d7e5a25a`.
Several type1 workers attach to map groups instead of actor models. These
groups' transforms and collision gates are distinct native mechanisms.

| Worker | Map geometry group | Mutable collision group | Scripted states |
|---|---|---|---|
| 7 | 8 | 48 | 0/1 |
| 8 | 9 | 49 | 0/1 |
| 9 | 10 | 50 | 0/1 |
| 13 | 19 | 51 | 0/1 |
| 21 | 11 | 52 | 0/1 |
| 23 | 21 | 53 | 0/1 |

Original Common4017 reads two integer operands through86DE90 and calls native
83E9E0 from A78130. The setter changes the group table at01301998. Actual
collision83E4E0, its selector/jump tables and the native floor traversal use
these aliases. The retained original x86 collision replay verifies all256 raw
groups against controlled ownership/flags and alias values, including all16
finite aliases and the strict64 boundary. The mod already copied/evaluated
these live remaps for new meshes and queries.

W14–20 are proximity-driven map-group motion. W14 attaches group12, checks player
distance to X320/Z215 and height25–35, then lowers local Y toward-20 near the
player and raises it toward0 farther away. The inspected motion chain86F450 →
90A830 →642850/642870 →66DC90/66E570 updates scene transforms. It does not write
the active ChMap triangle/vertex arrays in that chain. Those workers do not call
Common4017. Do not invent solid cylinders or assume a visual transform moves the
collision floor. Original player movement83D3B0 uses ChMap traversal83E5F0 plus
enabled actor contact;83D960 reads signed-short vertices from01301A4C. Exact
original City floor placement is covered by the approved real-floor replay below;
full native movement remains untested.

All34 type5 workers65–98 attach to the camera and use Common008B crossing
callbacks, with camera filters or empty camera behavior. They are not extra
walls or collectible objects. Original native
85FA30/85FC70/85FF00/8600D0 set worker shape-active bit80 and change bit1, not
the mutable floor-group remap setter43E9E0. Common0060/871240 controls worker
bit200 and player-control updates, not a proved wall toggle. No generic type5
blocking rule was introduced; parser names alone do not establish a barrier.

`verify_sin_gates_20261020.py` checks the exact retained EBP/model hash, six
owners/group IDs/both states and actual CALL operands at original PCs, plus34
camera owners. Original exports/native bodies and provenance stay outside Git
at `Analysis/FFX/SinDynamic_20261020/171143_927729/`; reports stay under
`RepositoryEvidence/offline-output/sin_collection_20261020/`. Karifean/
FFXDataParser/Fahrenheit attribution is retained. No copied game code is in this
document or adapter.

## Repair, data flow and ownership

Previously a43E9E0 event refreshed the owned mesh, but an unmoved selected target
kept its old beacon/Auto walk legs. A new route request could use a closed gate
correctly while the existing route continued steering into it.

`navigation.cpp` now advances a collision serial on an actual changed setter
value and completion of a43EA00 floor replacement. Unchanged setter calls do not
invalidate it. Native callbacks only advance atomics, pause copied route audio
and request the existing bounded observation; they never compute a path or wait
for a worker. Scene capture stamps the serial beside existing copied16 remaps/
actor flags. Old observations and pending commands cannot be accepted as current,
including a close/reopen back to the same value.

`nav_cues.cpp` records each route's serial/remaps. A fresh owned scene replans an
unchanged target when those facts differ; reopened gates can restore the shorter
path. It uses the production solver and copied mesh, without scanning every
retained leg against every floor triangle on each player movement. Directed
Sea of Sorrow event validation remains separate. A no-route result clears
synthetic steering; selection remains available for another request.

`NavLeg` carries its source serial and pending flag. `auto_walk.cpp` compares the
input-mask serial on the same keyboard/controller poll, immediately withholding
stale synthetic input. Small original controller input is preserved while pending.
The native update returns without waiting; a fresh route resumes steering. If no
result arrives within the established600ms field recovery bound, or the fresh
result has no route, Auto walk stops. Existing manual input/focus/battle/dialogue/
cancellation rules remain in force. No wall or player position is changed.

`navigation_intents.cpp` copies/rechecks the same serial before Start,
AssistStart and ArmAndStart. Old queued actions cannot start even if a gate
changes back before dispatch. The existing audio-context serial invalidates
queued route audio when the native wall callback pauses it.

## Verification and limits

`nav_cues_fixture` passes direct open route, immediate pending leg, late pre-gate
scene refusal, closed-gate detour with unchanged goal, ABA change, reopening,
independently changed copied remap and completely closed connection. Its floor
is controlled geometry, not extracted City data. `auto_walk_fixture` passes
same-poll keyboard suppression, original controller input preservation,
asynchronous detour recovery and bounded refusal. Both reader profiles exercise
the actual wall callback, unchanged-value guard and copied intention/dispatch
path with changed/recovered serials. The original native collision-selector
replay supplies remap semantics; full native City motion/timing is not executed.

There is no new setting or key. Native invalidation adds scalar/atomic work;
route computation stays on the owned worker with existing cancellation and
bounded scene storage. No frame, audible latency or whole-game responsiveness
claim follows from component tests. User confirmation when progression permits:
request the onward route in City of Dying Dreams. Changing obstacles should
produce a detour or an unavailable-route stop, rather than steering into an old
obstruction.

The user explicitly approved these two read-only map asset reads. No game
launch/control/probe or Tester Logs access occurred for this investigation.


## Approved map replay and platform destinations

The approved assets were `ffx_ps2/ffx/master/jppc/map/sins/sins04/bin/mapout.vpa` and the `sins05` counterpart, extracted read-only outside Git at `Analysis/FFX/SinMaps_20261020/184012_314750`. City is 3912464 bytes (SHA256 `ed9dafc1a2ed678ab4400bc71165a05a4369103e82ef210cf2c196f400705947`); Nucleus is 1579808 bytes (SHA256 `5dd7c0c7e17f294fb35c51c31ac9837382f2da9a9a8618a1c031f30acf89cdd1`). Both retain original 83EA00 ChMap format. City has 876 triangles/825 vertices and collision groups 48–53 counts 4,5,6,8,7,3. Nucleus has 146 triangles/91 vertices, one connected group 0 floor. Adapters are `Docs/Scripts/Offline/extract_sin_later_maps_20261020.py` and `prepare_sin_later_maps_20261020.py`; reports are external under `RepositoryEvidence/offline-output/sin_later_maps_20261020`.

The canonical `--sin-later-maps` host runs original 83E4E0 and actual jump tables over both real floors with controlled ownership/remaps: 56210 native predicate comparisons (City 56064, Nucleus 146). All 64 combinations of six gates produce 192 valid entrance 0-to-W39/40/41 City routes across 270912 sampled floor points. W40 is 1915.611400 open and 1930.588669 with group 53 closed; W41 is 2201.854314 and 2216.831583 respectively. Group 53 (mask bit 5) creates the real detour while all other gates remain enforced and main progression stays open. Nucleus reaches all 146 triangle centers from the native entrance with 15814 route samples; all 40401 integer X/Z points from -100 to 100 are on the floor. Both endpoint candidates around native `GetRandomInRange(200)-100` are covered, but the RNG/full spawn controller did not execute. Off-floor Nucleus exit lines have no walking route; the original count 10 W40 warp 325 remains progression.

The ordinary City catalog omitted four non-actor boarding touch regions. Three optional chest positions have no direct entrance route until native platform movement. New metadata in `navigation_passages_data.hpp` for exact `sins0400` hash `01aaeb94`, category Object 6, identity 2, EntryMask `0x10`: W42 First moving platform, stop 1; W43 First moving platform, stop 2; W59 Second moving platform, stop 1; W60 Second moving platform, stop 2. Original NativeCommon 57/59/5A rectangles request W0 entries 25/26/27/28 and native platform movement. Boarding is walking into the region; no Confirm action is invented. Live flags suppress unavailable or moving stops, and copied geometry follows the current pose. Host coverage passes all four native rectangle routes from their real floor components and endpoints. `sin_platform_followups.inc` passes native/owned discovery, all16 enablement combinations, live geometry, remapped callbacks and foreign IDs in both reader profiles. No fixed floor connection or automatic multi-platform ride is invented.

The four rows reuse `navigationTarget` overlays and the existing native capture/owned scene pipeline. Exact script length/hash and current callback entry4 are required. Worker shape-active bit80 and channel4 enablement qualify availability; this is identity2, without the inactive-seal override. Native Common0059 at860610 supplies width/depth/height;005A at855FB0 supplies angle and0085 at856300 supplies range. Retained source locators and original bodies remain outside Git. The catalog change adds no native callback, timer, scan or speech producer. Scene/resource cancellation, target selection lifetime, native control and Auto walk stopping remain the established navigation owners. Extend only with an exact resource identity, actual callback/shape evidence and real-floor endpoint coverage.

The audit batch took 28.156 seconds including validation; this is not usable-route latency. No whole-game, frame-time or audible-performance claim is established. Native full-ride VM behavior and gameplay remain untested. User-side check when progression permits: in City, the Object list should include active platform stops; request a route and walk into its boarding region so the game owns the ride. Request the next route after native control returns.
