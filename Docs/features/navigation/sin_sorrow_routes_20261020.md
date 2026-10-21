# Sea of Sorrow route dead end

Investigation is **complete** for the concrete reproduced defect. The repair is
**implemented, offline verified and guarded-deployed**. The final developer
build completed warning-free; all seven installed hashes match the delivery
receipt. On October20 the user tested Sea of Sorrow and confirms the repair works.
Later Sin areas after Seymour were not tested and are assigned to a tester.
This report reopens navigation correctness despite earlier general routing
fixture passes.

## Report and evidence scope

The user named `Saves/Dylan/stuck map/ffx_076`, then supplied the latest
game-directory log and clarified that the failing area is **Sin - Sea of Sorrow**.
The save is 26880 bytes, SHA256
`e509f1e843243f92b97dc9358a4b9c028fb3bdc8f5e6f77d9b837f8c13ecc105`.
It starts in room322, Sin - Near Airship, story3205. That is the preceding area,
not evidence that the user's Sea of Sorrow report names the wrong map.

The authorized latest `FFX_SR_Native.log` is61575 bytes, SHA256
`b0bf061d1c3646a71a297ba899914c456f25b27bf065e595aacb8f6519af7c7a`.
The measured identity is retained in the external manifest. At09:09:30 the log enters Sea of
Sorrow; at09:09:35 it starts a Save Sphere route. By09:11:06 the selected target
is Exit2 to Sin. Repeated commands continue producing routes, often returning
to the same first7–9-step direction. There is no recorded no-path, unavailable
floor or Auto walk start. Manual route requests are evidenced; the actual
movement device and exact native pose/camera are not logged.

The current task explicitly authorized reading Sea of Sorrow's installed
map/collision files for offline replay. Only its `map/sins/sins02/bin/mapout.vpa`
was extracted; archive header/index validation is required by the established
VBF reader. No agent game launch/control/probe occurred. No tester submission
was read; the named log is in the ordinary game directory.

## Native floor and script facts

| Evidence | Verified fact |
|---|---|
| Native area | room203, `sins0200`, text area309 |
| Event identity | code14465 bytes, FNV32 `b5ed9bb4`; EBP SHA256 `28d8456847f13f43d3b39092ad505cc4fbd2674697d76977e3a4d60871ffef0f` |
| Approved mapout | 1756304 bytes, SHA256 `fc146863d344aa6ec5d32f1f4b59312155eaeb0f67c2375fc87738e11312cdee` |
| Collision | 1220 triangles,1179 vertices, native scale27.899267578125; all triangles group0 |
| Save Sphere | W12, model5001, original shared position230.9100 /28.6387 /958.8361 |
| Onward exit | W22 crossing near X188–233 / Z1024; story3210 chooses room204 versus296, rather than proving an unavailable callback |

The mod reads the native collision floor. Displayed-minimap absence does not
select a separate routing implementation here. There is no current Sin-specific
cross-water bake. The VPA header's vertex count/scale/vertex offset and aligned
post-vertex triangle array were checked against the previously retained exact
Gagazet extraction. Original vertex, triangle and parsed-script data remain
outside every Git tree.

The floor alone is insufficient. Original type2 workers17–20 are scripted ledge
crossings. They disable player control, request W0 movement/animation entries,
and restore control. W17/18 request entries6/7 and move toward the right-hand
landing near X-110. W19/20 request entries8/9; both original bodies set the
same left-hand destination:
X91.1683731 / Y14.1166182 / Z-40.3403664. W19's finite edge runs approximately
X129–151,Z41 to-81; W20 runs X113,Z36 to-84. Those event semantics make a
geometrically connected reverse walking route invalid.

## Concrete offline reproduction

`sin_route_fixture.cpp` executes production `decodeOwnedNavigationMesh`,
`routePoints`, `path` and `interactionRoute` against the separately retained
original1220/1179 collision. It tests entrance-to-save/exit, save-to-entrance/exit,
and the original W0 landing-to-save/exit. Original script facts and geometry
are size/hash qualified. No native animation, game variable getter or live
actor is simulated as if it were observed gameplay.

From the original W0 entries8/9 landing, **both** current production routes
return a first leg toward X178.0871 / Y-4.5879 / Z30.1621. That leg crosses
both finite W19/20 edges. Their callbacks request movement back to the very
landing from which the route was calculated. This is a concrete route/event
contradiction and explains a return loop; repeated path requests select the
same invalid reverse shortcut.

Independent corroboration: the final logged distances to the two exit edge
origins yield two rounded X/Z candidate positions. One is approximately
95.18 /-32.35, near the scripted landing on actual triangle459. The other,
249.08 /-34.69, is outside all projected floor triangles. This is an inference
from rounded distances, not a captured native pose or proof of the exact frame
at which a callback ran. The geometric returning-edge reproduction does not
depend on that inferred position.

The source omission is central: `nav_geometry.cpp::path` and `lineOfSight`
evaluate native adjacency/passability/height, and `nav_zones.cpp::interactionRoute`
searches the same floor. `navigation_scene.cpp` captures all worker facts but
does not translate these non-destination ledge callbacks into directed movement
constraints. `nav_cues.cpp`/Auto walk and reachability consume those routes.
Static identity labels are not the failing bake.

## Separate spoken-path defect

`nav_format.hpp::routeDirectionsText` merges consecutive segments with the same
rounded compass label, even when their actual headings differ. The original
entrance route contains9.29,18.53 and22.80-step legs at approximately66.9,44.0
and71.1 world degrees; the log presents them as one51-step northeast leg.
This loses bends. A controlled constant-camera comparison over65 bearings
consistent with those labels finds no straight51-step northeast walk fully
on the projected floor. Actual live camera motion is unobserved, so this
presentation finding is not presented as the sole cause of the reported loop.
Simply splitting the text into repeated identical labels would not by itself
establish usable guidance through a bend.

## Implemented repair and verification boundary

`navigation_events.hpp::captureRouteEvents` recognizes only room203 and the
ready `sins0200` resource (identity14465, FNV32 `b5ed9bb4`,48 workers). It checks
the four local17–20 headers/type2 workers and the actual crossing on channel4,
then copies finite current edge endpoints and height plus the original landing
side into four bounded POD rows. Disabled or remapped crossings remove their
constraint. Missing, foreign or malformed resource/header facts leave the
event overlay unready, so the worker does not invent a route. No parsing,
hashing, allocation or waiting occurs in Native processing; the native
observation copies only scalar facts, while the worker decoder additionally
requires the exact original1220-triangle/1179-vertex floor, scale27.899267578125
and FNV32 triangle/vertex hashes `4024376f`/`a8173459`.

`nav_geometry.cpp` applies the directed crossing rule during shared graph
expansion and interaction search, and checks every segment during smoothing,
including a two-point shortcut. The legal crossing direction remains usable;
the original floor is unchanged. Geometry cache compatibility and reachability
account for event facts. Beacon and Auto walk publication revalidate retained
route legs and replan from a fresh owned player position when changed events or
native relocation invalidate them. Existing native control-disable behavior
still stops Auto walk. The repair does not teleport or suppress native
callbacks.

`nav_format.hpp::routeDirectionsText` now merges only actually collinear,
same-direction legs. Sharing a rounded octant no longer merges a bend. This
preserves separate bends but does not establish exact-angle guidance or perfect
manual gameplay directions.

Collection belongs to existing bounded native scene observations. Ready copied
facts alone cross to the preparation/routing owner; no native pointer, parser,
wait, new polling loop or game-thread path computation is used. Context/epoch/
resource/callback changes retire affected routes. Mesh-cache compatibility
includes changes to directed event facts, so stale geometry compatibility alone
cannot retain an obsolete constraint.

Both native reader profiles passed all16 callback subsets at the final171-hook
callback. Coverage includes disabled/remapped events, moving geometry,
foreign-resource rejection, room checks and native-owner guards. Actual queued
intention checks pass for an unchanged start and cancellation on changed
callbacks. The original1220-triangle/1179-vertex floor host passes six critical
routes (entrance/save/exit/return/W0 landing), four ledge directions, all16
enabled subsets, smoothing and reachability, and rejects foreign resources or a
missing floor. Shared reachability, navigation geometry, beacon/arrival/retained
route refresh, Auto walk and native-actions regressions also pass. The canonical
final developer build with `--no-deploy` exited0 without warnings. Guarded deployment completed with all seven installed hashes verified. The
[delivery receipt](../../../install/sin-route-20261020/verification.json) records
compiled source identities, test logs and unchanged INI, saves and release ZIPs. Gameplay confirmation remains a
separate later step.

Controlled offline performance evidence is scoped to components. The original
Sin host's same landing/exit route measured55.518ms first and a warm-five mean
of57.604ms (maximum60.008ms); the repaired route measured129.418ms first and a
warm-five mean of127.848ms (maximum131.744ms). The old shortcut is invalid, so
this compares valid route work against the prior implementation's measured
work, not equivalent route quality. A1000-callback native-ready capture took
17.092ms in the reference and19.107ms in the installed profile, with zero heap
allocations in either case under controlled ready-getters. An existing large
reachability component with512 targets and30000 triangles measured30.822ms
cold first,2913.640ms for the full cold computation, and31.131ms warm. These
figures do not measure in-game action-to-audible latency, queue age, or user
responsiveness. No game launch/control/probe was performed.

The repair also copies and revalidates `RouteEvents` for native movement
intentions; wall-mesh cache compatibility compares copied event facts. Each
graph/segment performs at most four finite edge checks. Retained-leg validation
runs on the worker, cancels per leg, and adds no new scan on ordinary maps with
zero events. Native observation uses ready getters only, with no pointer
handoff, parsing, waiting or allocation.

No new control, setting or mod-owned user-facing string is introduced. Defaults
and localization/HRTF gaps are unchanged. Local current-content/all83-Git-blob
source-pattern review has no findings. The known old GitHub object remains a
separate cleanup limit. Native production cost and end-to-end audible
responsiveness remain unmeasured. Further broad capture is unnecessary to establish this
defect. Baaj is accepted/closed; the Cid report is a separate gameplay item. The
save starts in room322 while the authorized log reports room203; this does not
invalidate the named Sea of Sorrow report. The user subsequently confirmed the
corrected Sea of Sorrow routes work after verified deployment.

## Retained evidence and provenance

Original parsed models and native bodies:
`Analysis/FFX/SinRoutes_20261020/161328_770132/`, outside Git.
The manifest records save/log identities and approved asset provenance.
The original script parser retains Karifean/FFXDataParser attribution; the
existing VBF reader retains Topher/VBFTool attribution; native VPA layouts are
cross-checked with Fahrenheit and the prior exact extraction.

Fixture inputs and metadata:
`Analysis/FFX/RepositoryEvidence/offline-output/sin_routes_20261020/`, including
`geometry.json`, `provenance.json`, `topology-audit.json`,
`merged-direction-audit.json` and `oneway-loop-proof.json`.
Current x86 build/output:
`Analysis/FFX/RepositoryEvidence/test-build/sin-route-20261020/`.
Sol retained discovery, route/event correctness, integration and verification.
A Luna known-fact documentation task updated this record and the navigation
index; Sol reviewed source/evidence alignment and corrected final deployment
and verification wording. No delegated edit remains unresolved. No new installed asset read or game
runtime action occurred in that documentation task.

The user's newer request to audit Sin's later rising barriers after Seymour and
to investigate optional Nucleus collection autosolve is separate work and is
implemented, offline verified and deployed separately. Neither behavior is
established by the Sea of Sorrow gameplay confirmation; later testing remains
with a tester. See [later Sin](sin_late_obstacles_20261020.md) and
[Nucleus collection](../puzzles/sin_collection_20261020.md).
