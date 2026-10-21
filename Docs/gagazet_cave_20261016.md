# Gagazet cave puzzles — evidence and current status

## Scope and map identity

The user explicitly approved offline reads of the installed Gagazet cave map and
graphics for this investigation. The selected 126 files and their hashes are
retained in `Scripts/Offline/output/gagazet_cave_assets_20261016`; original
script proofs are in `Scripts/Offline/output/gagazet_fixture_20261016/proof.json`.
The original EBP is `mtgz0700.ebp`, 32,256 bytes, SHA-256
`20ee7c61feb04f018154ee9eb488896affbf8d39f3812570b60ae56f9ea5bf95`.
The bounded audit and catalog are under `Scripts/Offline/output/gagazet_cave_20261016`.

Room 310 (`mtgz0700`, Gagazet - Submerged Passage) contains both puzzles. The
adjacent room 272 (`mtgz0600`) is Gagazet - Mountain Cave. Room names agree with
the retained native map-name text and room catalog.

## Original puzzle behavior

W22 type 4 entries 6 and 7 are the Wakka target throw (9161) and three-swimmer
choice (10261). The native integer reader is 120 bytes and the native operand
reader is 123 bytes; their hashes, fixups and original-code fixture are recorded
in the proof JSON. Native input sites are `0x24E2` (throw confirm), `0x28C7`
(Rikku) and `0x2A7A` (Tidus). Original choices are 1 for Rikku/right/green,
0 for Tidus/left/blue, and the remaining choice 2 for Wakka/lower red (described
as orange by the tester). The original PPP records 18/17/16 and their positions
match those destinations. Native RGB tints are 100/90/200, 100/200/100 and
200/90/100.

The first successful throw requires both native alignment bits, 64 and 128, in
`L0:map:0000D8:T0` element 2. Normal route completion uses that same D8
declaration's element0 bits1/2 (writes25EE/2E44). AA3 bits1/2 instead belong
to the later story-gated replay/reward branch; they are not normal route status.
Let the original success scripts reveal the passage and
update route state. Automatic throw assistance substitutes only copied result
operands at `0x2550–0x2568` after the original throw; it does not write save or
story flags. The second puzzle uses native direction and confirm checks. A
partly chosen wrong Rikku opening may require Back and re-entry.

## Controls and current implementation state

F8 > Miscellaneous > **Gagazet cave puzzles** is context-only. Its separate
settings are **Gagazet puzzle auto-success** (default Off) and **Gagazet puzzle
cues** (default On), followed by **Read puzzle instructions**, **Read selected
opening**, and **Puzzle status**. F11 reads the current puzzle step; Shift+F11
uses the existing inferred-step route. In the general trial controls, F9/F10
select previous/next guide steps. The F11 routes were checked in
`src/native/mod_menu.cpp`; the menu labels and setting descriptions were checked
in `src/native/mod_menu.cpp` and `src/native/settings.cpp`.

The current source contains the bounded native reader, context/epoch checks,
manual instructions and selection speech, separate setting gates, automatic
native input handling, and targeting-window cues. October19 removes mandatory
calibration and predicted launch cues at the user's request. Listen through a
couple passes and begin before the beep so the ball arrives during alignment.
The original36/19-frame motions and async waits are retained offline; their sum
alone does not establish a fixed ring-clock lead. All30 stages/three speeds are
tested without manual native-state mutation. [Current evidence](features/puzzles/airship_wakka_20261019.md).

Original120/123-byte reader/operand and native predicates pass final development/
release fixtures. Native ring-write cue, independent real SDL audio gates,
settings32 combinations, F8 context and profile checks pass. Canonical warning-free
build and guarded deployment are verified; new gameplay confirmation remains
pending. See [final receipt, ownership, limits and checks](active_puzzles_battle_controls_20261016.md).
No game launch/control, live probe or Tester Logs access occurred.

## October16 return route and misleading exit label

The user reports that the first trial appeared to solve automatically, then
only one chest and the exit back to the cave had routes. The own-session log
at12:35:58..12:37:42 shows the first-trial prompt, Throw/Back controls, successful
navigation to the local chest and Exit1, and refusals for Exits2/3. This does not
prove a failed solver or a collision false-negative. Original W29/30/31 return
from three separate water sections to room272 entrances1/2/3. After the first
trial, return through Exit1 to Mountain Cave: native mtgz0600 W15 consumes the
shared D8 route bits and controls walls52 and49/51/50 there. The next water
section is reached from the cave, not directly across this pool. User is
testing this return; second puzzle/progression remain unconfirmed.

The navigation catalog incorrectly inferred W32 was an exit to Mountain Gate
from a nested late-story reward warp in W22. Original W32 entry2 calls W22:6
and restores field control in this water section. W33 entry2 calls W22:7.
Both are now hash/length-qualified named Event targets, First Trial of Gagazet:
Wakka's ball target and Second Trial of Gagazet: coloured openings. Their actual
entry2 masks and native activation/disappearance rules remain. All three real
water exits keep their identifiers and geometry. Manual guidance explains the
cave return. No route links, barriers or completion flags were changed.

Retained proof: audit_gagazet_returns_20261016.py verifies original EBP hashes,
three return calls, two trial requests, D8 success writes and cave wall guards/
calls. Final output: output/gagazet_fixture_20261016/return_route_20261016_194927.json.
This is static native-script evidence, not executed movement/collision evidence.
The original-EBP reader fixture verifies distinct target roles, unchanged real
exit categories and changed-script refusal. [Current delivery/checks](debug.md#october16-special-features-controls-and-field-results).

For testing, Shift+F1: `warp 310 0` enters the Wakka trial water section;
`warp 310 1` enters the three-swimmer trial water section. Continue by normal
navigation to the trial. These preserve solved/story flags. Native entrance and
trigger proof: Scripts/Offline/output/gagazet_fixture_20261016/warp_entrances.json.


## October16 explicit early-save test setup

Subsequent user report: even after returning to Mountain Cave, only the previous
passage and Fayth Cluster appeared reachable. The second water trial had only
Tidus; Wakka/Rikku were missing from this early warped save. Neither a complete
first-trial route nor the second solver is gameplay-confirmed. Missing swimmers
explain an invalid second-trial context; they do not by themselves prove why the
first cave passage remained blocked.

User explicitly requested `warp gagazet-test`. This development-only command
accepts an original early save (story at most1530, normal selector0), writes
story2625 and resets exactly the three shared cave-state bytes at globals+D8,
then enters dry Mountain Cave room272/entrance0 through the native transition.
It does not set puzzle-success bits, barrier remaps, inventory or party data.
Normal warps still preserve progress. Force/entrance overrides, later saves,
battle/dialogue, stale context, missing declarations and invalid global owners
are refused. Writes occur on the native action owner immediately before warp;
queueing and preparation preserve all globals. No save-file reader/editor or
runtime polling was added.

2625 is selected within the verified native test interval: W15 enables the
>=2625 layout, remains below2680's first-trial-already-complete branch and below
2715's later layout. It is not asserted to be the exact counter of a naturally
reached post-Seymour save. Original cave actors load their models in their init
entries. Cave W33 entry2 requests W15:7, which natively adds Wakka4/Rikku6 and
forms slots[Tidus0,Wakka4,Rikku6]. That event is now named **Prepare swimming
party** in Events, with the original entry2 crossing and hash/length guard.
Original animations, party restoration, water trial inputs/success writes and
cave wall initialization remain in charge.

Offline evidence: audit_gagazet_returns_20261016.py now also checks the three
story gates, party-event request, five native party calls and the swimming
actors' model init calls against the hash-verified original EBP flows. Retained
output: output/gagazet_fixture_20261016/return_route_20261016_200830.json.
The actual production utility path is exercised from original Salvage Ship
ATEL, which declares story but not cave-stateD8. The existing location0 native
story getter qualifies controller+2c before the bounded three-byte copy/reset.
Both development/release matrices verify all globals against an expected copy,
neighbor bytes, exact story at warp, no puzzle requests, refusal/cancellation,
ordinary warp preservation and mutual exclusion with Macalania test setup.
Reader fixtures check the real cave EBP's named Event/entry. Animation, physical
path traversal and native party execution remain gameplay dependencies.

Gameplay procedure, after verified deployment:
1. Load the original early save. Open Shift+F1 and enter `warp gagazet-test`.
2. In Mountain Cave, follow **Prepare swimming party** under Events before the
   Submerged Passage exit. Let the native scene finish. Confirm all three
   swimmers are available on entering water.
3. With Gagazet puzzle auto-success enabled, complete the first trial. Use Puzzle
   status in F8's Active puzzle menu to confirm success, then return through the
   local water exit to Mountain Cave and check the next passage.
4. Reach the next water section normally; complete the coloured openings with
   Tidus/Wakka/Rikku. Confirm Puzzle status and return to check the next cave
   passage. A success status with a blocked cave passage distinguishes a route
   issue from an unfinished native puzzle.
5. Reload the original save after testing.

No live game/probe, installed-asset reread, Tester Logs access, installer rebuild
or model delegation was used for this follow-up. Build/deployment evidence is
recorded in debug.md; gameplay confirmation remains user-side.


## October16 mixed water and land routing

New user report and own-session log distinguish this from puzzle progression:
13:41:10..13:41:24 the cave's second Submerged Passage entrance repeatedly has
no reachable interaction zone, while the first entrance/return are routable.
After manually leaving the water,13:41:54 it produces a valid long cave route.
Room310's separate trial pools also correctly reject a direct pool-to-pool path.
Retained selected own-session evidence: install/gagazet-cross-water-log-20261016.txt.
No Tester Logs were accessed.

The ordinary decoder projects the controlled swimmer's entire current native
component to its present Y. This correctly keeps swimming off unrelated floors,
but erases land heights and movement-mode transitions along a cave route.
Requiring the distant native target's height then rejects a valid shore/land/
water route. Merely disabling target-height checks would lose floor protection.

The original mtgz0600 program has three shoreline edge workers35/36/37 and
regular controllers22/23/24. Directional crossing flags request W0:14 to restore
walking or W0:15/16/17 to establish the corresponding swimming level. The new
schema1 cross-water bake records these exact edges, land/water approach points,
original triangle indices/adjacency, the native return seeds and each water exit.
It contains three disjoint approach regions98/67/66 triangles, including the
second shore's original short straddling connector chain1160/1401/1400/186/... .
Finite near-height straddlers85/862 are included for requests at the shore itself. A first prototype using triangle-center sidedness alone omitted1400/186; the
real-data fixture exposed the disconnected landing. The bake now retains the
original short adjacency chain without adding any graph edge.

Source: approved offline Gagazet map reads. Original cave mapout.vpa SHA256
5a1ea84405707c815b24f374a56dccdc950d24a53c6e8b1cd2e504fc50f5e00c;
script EBP hash remains in the existing flow archive. New retained source paths,
hashes and approvals: output/gagazet_cross_water_20261016/manifest.json.
bake_gagazet_cross_water_20261016.py checks original EBP/PPP, plane-setting calls,
walk/swim requests, unmodified collision data and bounded native connector
chains, then writes cross_water_bake.json and src/native/cross_water_data.hpp.
Runtime requires prepared script name/length/FNV mtgz0600/24363/131b7d53 plus
native geometry1406/1039, scale and FNV aeced264/96b08924. Unknown/changed data
retains ordinary routing and cannot adopt these connections.

Native geometry capture adds only a ready identity and three qualified native
worker/header/callback observations. Worker pointers remain on Native. A missing,
disabled or foreign shoreline callback retires its connection. The copied bake
ID/live mask also participates in route/filter/wall context cache invalidation.
Workers alone check geometry hashes, preserve an owned copy of original terrain
heights/current collision gates, build restricted swim views and search routes.
The planner joins swim-to-shore, walking and shore-to-swim legs through finite
native crossing segments. Each original mode boundary remains an explicit
waypoint; smoothing cannot remove it. Only verified water-exit approaches relax
Y while swimming; final activation still uses the original native interaction
zone. The filter uses the same planner and updates its original-gate-only
counterfactual in both terrain views. No mode, position, velocity, progress or
collision state is written; native scripts still perform every transition.
Normal Auto walk cancellation on native control loss remains in force.

Offline fixture uses actual1406-triangle cave data, reproduces the old
first-to-second swimming failure and checks all9 swim/swim combinations,
land/water, first-trial-only progression, second-trial barriers, closed first
gate, inactive shores, changed script/geometry refusal, source/destination mode
waypoints and cancellation. Separate594-triangle submerged geometry cannot
adopt the cave bake. Native capture fixture uses the original prepared cave
resource and actual callback-fact adapter, with controlled native getter
endpoints; it checks current activation/ownership and zero native heap. Native
animation, physical event traversal, end-to-end audible latency and in-game
crossing remain user-side confirmation, not conclusions from these tests.

Sol retained this geometry/ownership investigation and implementation; no model
delegation. Installer/ZIP remain held. Final build, timing and deployment receipt
are recorded in debug.md after completion.


Final offline timing/check evidence: optimized original-data host nine cold
mesh-decode/route/filter pairs395.603ms aggregate (not one action's latency).
An earlier identical9-pair run before the added boundary-source checks was
313.880ms; concurrent compiler/system activity and changed bake coverage prevent
claiming a like-for-like speedup/regression. Both167-hook reader profiles passed
real prepared cave identity and callback-fact capture:100 controlled captures
2.288/2.611ms aggregate, Native heap0/0. These simulate native getter endpoints
and omit unchanged bulk geometry copy; they are not measured game callback or
action-to-audible latency. Reachability's existing512-target/30000-triangle
pressure/cache host measured cold capture1.849ms, first commit30.388ms,
completion2483.224ms; warm capture0.722ms and first/all commit30.881ms, Native
heap0/0. This extreme host is not a Gagazet latency claim. Native callback loops,
worker geometry/search bounds, epoch/cancellation and existing queue capacities
remain explicit; no new worker wait, timer or input-path work was introduced.

Original finite near-height shore audit found two omitted boundary-source
triangles85/862. Both are now in the bake and actual original-data source-floor
cases pass before delivery. The staging build was stopped before deployment to
include those cases; its partial log is retained as
install/gagazet-cross-water-build-stopped-for-boundary-audit-20261016.log.
The first native-reader capture test disabled the wrong synthetic worker address
(2460000 instead of2451000); its failure log is retained separately, corrected
fixture addresses pass, and no production workaround was added. Canonical final
build has a fresh pre/post source inventory. Gameplay tests: request the second
Submerged Passage while still swimming after returning to Mountain Cave; route
must include leaving the first water section, land travel and the second entry.
Then request a return route while swimming in the other section. Before puzzle
success, the appropriate closed cave route must still be unavailable. Native
shore scenes can still cancel Auto walk when they take control; re-requesting
after the scene uses the updated native mode. Movement/animation remain natural.
