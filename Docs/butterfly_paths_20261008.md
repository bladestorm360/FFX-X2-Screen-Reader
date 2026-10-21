# Butterfly body-hazard routes — 2026-10-08

Owner: Astra (`gpt-6-astra`) geometry/algorithm delegation; primary integrates
navigation, audio, input, build registration and full verification. No game was
launched or controlled; no runtime probe, game-install read or tester-log access.

## Evidence and decisions

Before implementation, inspected `navigation.hpp`, `nav_geometry.cpp`,
`nav_interactions.cpp`, `nav_zones.cpp` and the existing native fixture workflow.
There is no `nav_paths.cpp`; normal mesh paths live in `nav_geometry.cpp`.
Its smoothed route does not account for live red actor bodies, and can project
off-mesh destinations to boundaries. A normal route must not be described as
safe for a butterfly course merely because it is walkmesh-connected.

Sol's actor investigation established body contact rather than generic event
interaction range. Independently checked the preserved native body:
`Scripts/Offline/output/native_bodies_20261008_092001/FUN_0083D3B0.txt`,
especially native radius recomputation and pair overlap around lines 74–130.
The combined horizontal radius for each actor is the average of its X and Z
scales multiplied by its base radius; the actor and player radii are then
summed. A native
conditional vertical overlap applies only when both actors' flag `0x40000000`
is set. Body-disabled actors are omitted by the producer. Native equality can
touch; routes reject tangency conservatively.

The agreed `Entity.zone` contract is a valid shape-1 **body cylinder**, with
`range` holding that combined radius and `contactMinY/contactMaxY` holding the
native allowed player-origin Y interval. `contactRadius` is zero; the negative
one height sentinel
does not mean a sphere here. Generic `inInteractionZone` must not test butterfly
arrival: its shape-1 negative-height interpretation is a sphere. The dedicated
`butterflyInTarget` checks the actual horizontal radius and vertical interval.
Coordinates, radii, paths and margins use unchanged native map units.

## Solver and safety boundary

`butterflyRoute` constructs an explicit graph on the current collision mesh.
Nodes remain inside convex native triangles. Only declared native neighbours
with two identical shared vertex IDs and coordinates bridge triangle groups.
Reciprocal native portals share one construction; one-way adjacency stays
one-way. No smoothing or straight geometric shortcut can cross disconnected
floor, a U-shaped gap, a wall, or a missing native portal.

Candidate nodes include floor centers, closest blue floor points, blue interior
anchors, five fractions of each portal, and 32 circumscribed red-ring samples.
Every finite edge is independently checked against every currently active red
body. Its `[0,1]` segment interval is first clipped by the native vertical overlap
interval; the exact closest XZ point over that clipped interval determines
contact. A line's extension never counts as the traversed segment. Tangency,
origin inside red, malformed active hazard geometry, or a blocked blue arrival
fail closed. Disabled hazards are ignored. An empty result means no proved route,
not proof that the destination is permanently unreachable.

Ring radius is `(combined radius + 0.25) / cos(pi/32)`. This supplies candidates,
not a safety assertion: all chords are still validated, including against other
hazards. Origin-to-floor tolerance and conservative red expansion use eight
native-float epsilons at the actual coordinate magnitude, rather than an
arbitrary world-distance or alternate-floor allowance. The actual origin Y is
preserved. There is no path-length or search-distance cap.

The graph is conservative and incomplete: a very narrow safe portal portion or
blue arrival sliver between samples may exist without a graph route. It then
returns empty. Do not fall back to a generic direct route or report success.
Open swimming planes and missing/degenerate mesh geometry are unsupported here.

## Dynamic layouts and assisted input

Sol found native crossing workers 45–49 on the south course requesting alternate
entries of butterflies 20–26, which change colors and positions during movement.
Every returned route is a proof against its **current snapshot only**. A layout
change, collection, availability, bounds, radius, position, or color delta must
invalidate/recompute the route through the existing event-driven integration.
The helper retains no runtime cache and never changes game mechanics.

Integration must recheck butterflyRouteClear using player, intendedSteerPoint, red
against fresh bodies before assisted input, and cancel when it fails. Existing
autowalk rejoin/unstick lateral segments are not automatically part of the route
proof. Do not allow an unchecked sidestep. Refresh after native crossing callbacks
before continuing stale route instructions. Keep manual cancellation and normal
default-Off autowalk consent; this module does not generate or inject input.

## Offline validation and limits

`src/native/tests/butterfly_paths_fixture.cpp` compiles warning-free with the
configured MSVC x86 toolchain, `/W4 /EHsc /std:c++17 /MT /O2`, linking
`butterfly_paths.cpp` and `nav_geometry.cpp`.

Passing coverage: crossing versus finite-line extension, tangency, red start and
goal, clipped height crossings, separate heights, missing/malformed geometry,
disabled/changing hazards, native float rounding, separate floor rejection,
proved blue arrival, disconnected touching triangles, connected U-shaped floor,
million-unit paths, and a 288-triangle seven-hazard synthetic course. The final
synthetic timing was 74 ms on this host (earlier unoptimized portal construction
measured 290 ms). This is not a gameplay performance guarantee. Full route search
belongs on route request/layout invalidation, not on each input poll; the fresh
intended-segment guard is cheap.

Actual-course reachability/performance remains unverified. Existing extracted
`maps_v3.json` and `mcfr0100/0200.ebp` supplied scripts/entrances, not a verified
offline ChMap floor and live scaled player/actor bound snapshot. The production
`readMesh` resolves native player-specific mutable collision through `43E4E0`.
No matching offline collision parser or current native radius snapshot was found
in the scoped extraction scripts. Synthetic radii must not be represented as
verified course body sizes. Runtime acceptance remains user-side.

Primary review/integration and full build verification are recorded by the
primary in the main butterfly session document/memory.

## Primary steering review

Sol found that the ordinary Auto walk controller tests an ideal route vector
before quantizing it into camera-relative octants with hysteresis. Its injected
movement can leave a narrow detour and enter a red trigger. A clear ideal segment
does not prove that input safe. Primary therefore keeps butterfly movement manual,
always narrates its requested route even when ordinary Auto walk is On, and
blocks automatic route steering for these explicitly tagged targets. Live route
changes still invalidate/recompute copied guidance; no unchecked unstick is used.
Recognized red actors with unknown body bounds remain in the hazard list so the
existing conservative edge check rejects them. These are source-confirmed issues
resolved during review, separate from unconfirmed actual-course geometry.
