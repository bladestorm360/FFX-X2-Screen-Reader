# Mushroom Rock and shared route-cost repair — November1

The user requested **Fix and deploy** after the initial investigation. The shared
route smoother is repaired, offline verified and guarded-deployed. All seven
installed files match the scanned current-source build; settings are unchanged.
Actual gameplay/frame/audible responsiveness remains unconfirmed.

[The single scope ledger](../../plan.md#november1-mushroom-rock-lag-investigation--scope-ledger)
records the fixed acceptance and closure. No game launch/control/probe,
personal/tester save read, Tester Logs, installer/package, publication or
delegation occurred. Map collision reads were authorized for this follow-up.

## Diagnosis and map coverage

FFX routes use native collision meshes. Generated catalogs identify script/worker
targets; live channels, actors, collision remaps and interaction geometry still
determine current targets and valid routes.

The metadata audit checked397 retained EV files (377 unique names),397 catalog
records and3191 target rows. Catalog ranges and matched worker extents pass;
worker IDs are sorted/unique per slice.394 retained files match catalog identities.
The three small startup/test scripts soundtes, startmap and testbatt use the
existing general-parser fallback. All ten Mushroom Rock identities match.

The authorized follow-up parsed all433 mapout assets:327 nonempty first native
floors and106 empty/no-floor assets. No invalid array extents, vertex indices or
neighbor indices were found. Originals/collision exports remain outside Git;
the installed archive's size/mtime was unchanged.

| Mushroom Rock script | Workers | Catalog targets | Floor triangles |
|---|---:|---:|---:|
| kino0000 |68|27|2948|
| kino0100 |74|21|17665|
| kino0200 |60|26|4132|
| kino0300 |23|2|250|
| kino0400 |42|17|1010|
| kino0500 |61|19|6837|
| kino0600 |10|0|1300|
| kino0700 |37|16|765|
| kino0800 |31|0|604|
| kino0900 |23|12|12824|

Mushroom Rock is ordinary in worker/target count, but Valley and Beach are the
two largest collision floors found. Calm Lands has12307 triangles. The original
smoother tested distant shortcuts by scanning and clipping every triangle for
each candidate. Long winding routes rejected many shortcuts, generating millions
of repeated visits. A separate original-source profile attributes about97–98%
of the slow Sea/City request time to smoothing; graph search is under2ms there.
This establishes the shared computation bottleneck, not a malformed map bake.

## Implemented behavior and ownership

nav_geometry.cpp::VisibilityIndex builds a temporary balanced X/Z hierarchy over
the current owned walkable triangles for one smooth() call. Conservative boxes
retain the original barycentric tolerance; ill-conditioned/nonfinite rows retain
direct exact clipping. A near-to-far heap holds hierarchy entries and exact
coverage intervals. It can stop at the first proved gap or complete coverage,
without scanning distant floors on failed shortcuts.

The existing triangle clipper, height tolerance, directed-event constraints and
greedy waypoint choices remain authoritative. This is not a sampled-grid route.
Mesh/public route interfaces, native hooks and route producers are unchanged.
navigationRouteOwned still feeds the shared route to speech, beacon and Auto walk.

The index borrows the preparation worker's owned mesh only within the call.
There is no shared cache, native pointer handoff or wait. Storage/heap size is
O(floor), median splits give logarithmic depth (at most16 levels within the
32767-triangle native transport), and construction/comparison/traversal check
cancellation. Scratch capacity is reused within a smoothing call and released
with it. Cold setup adds up to about2ms in sampled short cases; the large winding
route improvement is the demonstrated reason for that bounded extra cost.

A new visibility_reference.hpp test oracle retains the prior mod implementation,
not game code. It is excluded from production. navigation_fixture.cpp compares
parallel epsilon edges, height boundaries, narrow gaps, overlapping floors,
ill-conditioned triangles and directed events against that oracle. Existing
cancellation, crossing and gate checks remain.

## Comparable offline results

The same x86 /O2 /W4 /WX host uses retained original floor/entrance/exit facts.
Controlled remaps and an empty event overlay isolate shared route computation;
this is not a replay of a complete live native state. Separate original-native
Sea/City/Gagazet fixtures cover their qualified gates/events/water transitions.

| Retained interaction route,20 runs | Before median | Fixed median |
|---|---:|---:|
| Sea of Sorrow exit |276.06ms|12.47ms|
| City open exit1 |285.35ms|14.15ms|
| City open exit2 |79.15ms|5.69ms|
| City blocked exit1 |300.55ms|14.15ms|
| City nearby exit0 |2.62ms|2.44ms|

| Mushroom Rock native-floor sample, one cold route | Before | Fixed |
|---|---:|---:|
| Plateau kino00 |112.73ms|5.33ms|
| Valley kino01 |54.44ms|8.47ms|
| Ridge kino02 |93.12ms|8.73ms|
| Beach kino09 |390.14ms|21.87ms|

Mushroom samples start at retained original entrances and end at actual triangle
centers reached through native adjacency. They are controlled geometry stress
routes, not current selected-target/game-state replays. Valley's sample starts
in a2000-triangle component of its17665-triangle floor. Calm Lands' sampled12123
triangle component improves1637.34ms→22.15ms. Every completed long-route output
matches the previous mod exactly; none reached the offline reference deadline.

All327 nonempty native floors pass20928 original/current visibility comparisons
and654 exact short-route comparisons across controlled remaps and fixed/ordinary
height cases. These sampled checks do not establish every dynamic native state
or every possible route. Job-throttled fixture timings are not measured game
frame, input-to-audio, full queue-tail or gameplay responsiveness. No universal
25ms game/frame or route-response guarantee is inferred.

## Runtime consumers and remaining limits

Domain10 runs navigation computation synchronously on the single speech
preparation owner. A long route can delay later speech/control preparation;
already prepared output has a separate backend worker. The game thread does
not wait for this solver. The repair reduces its measured computation, but full
speech-tail/data-age/audible response has not been timed in the game.

With Hide unreachable enabled, reachability's exact player-position context
comparison still invalidates decoded/verdict caches on movement. New requests
cancel old computation, but actual repeated/canceled work frequency is unmeasured.
This behavior is unchanged. Hide unreachable defaults Off and the inspected
installed INI has hide_unreachable=0, beacon=0 and auto_walk=0. Current values
cannot establish which settings were enabled during a past visit.

Native scene capture is unchanged: current worker/actor/saved facts and active
raw floor arrays are copied when requested. The fixed Scene is1279392bytes with
four observation slots. Native capture duration/aggregate frequency is unmeasured;
size alone is not proof of a game-thread stall. Observation revisions cancel on
resource/reset changes rather than every movement/key.

The own current log was read through a shared read/write/delete handle and the
host view:170741bytes, last written October23 at07:58:34, with no Mushroom Rock
session. Slow original-game spans there do not establish mod-caused lag.

## Build, safety and guarded delivery

Existing geometry/Gagazet water, Sea/all16 event states, City/all64 gate states,
reachability, audio/route ownership and native-action checks pass. Both complete
173-hook reader profiles pass. The canonical release-flavor native/launcher build
is warning-free, peaking at1.23GB under the enforced2GB aggregate job ceiling.

The resource runner exposed a GNU MAKEFLAGS=-j1 incompatibility with Windows
NMake. build_sdl3.bat clears incompatible parallelism requests only in its local,
inherently serial NMake environment. Kernel memory/CPU/shared-slot admission is
retained. No global runner or unrelated project was changed.

The reader suite initially stopped at an absent hardcoded personal Summon save.
Baaj/Via fixtures now use their already documented scalar saved states while
retaining original EV/native replay matrices and behavior assertions. This models
the recorded state without rereading personal/tester save files or changing game
behavior. The failed setup and successful full-profile rerun are preserved.

Defender4.18.26100.4, engine1.1.26100.4, definitions1.463.76.0 reports no detections
on the exact seven-file payload. Initial sandbox CIM/module-discovery failures
are retained; host scanning with native Windows PowerShell modules passed.
Protections and security preferences were unchanged. No trusted signing identity
is configured; this scan does not establish download reputation or other vendors'
acceptance. Existing PRISM is verified by the canonical guard.

Canonical deployment verified closed game/launcher, both executable profiles,
PRISM/launcher bootstrap and all seven installed hashes. Separate read-back
matches installed/staged/scanned bytes and the unchanged INI SHA256
67d6c6ed723a427ff46da7b9e43d75c374aae29243f857edd687688a3895ba13.
No installer/package was rebuilt.

## Evidence and closure

The source baseline is e8c57490211ed3103fd98529a5aef03984470a8f; its reviewed paths
matched release1.0 commit625af61. The private visibility repair is **Closed** at
geometry SHA2563927dc16a121975df300aec9697407131b6947904873ad1ee17c9e6f58fd1b64
and installed native DLL SHA256
952524c81b2c3960efea2424d64457d612162271cb97bcca266578388fa9057a.

Analysis/FFX/MushroomRockFix_20261101/verification.json retains522 source/test
input hashes, payload/INI hashes, original/current results, case limits, kernel
budgets and scan/deployment receipts. Original numeric results remain in its
baseline-verification.json and the exact prior mod geometry copy. The older
audit route-cost output/executable path was reused for an intermediate run;
use that preserved receipt as the original timing record. Game originals,
analysis exports and original native replays remain outside Git.

Further redesign is human-user-triggered only. Remaining facts are actual native
aggregate/frequency, queue/data-age tails, audible response and state-dependent
gameplay. At the next natural Mushroom Rock visit, select an exit and press
Backslash; expect prompt route speech and responsive walking. Delayed speech,
a route pause or movement hitch remains a reportable failure. No coordinates
or live capture are requested. [Accumulated accessible checks](../verification.md).
