# Player reports: navigation, battle and lightning — 2026-10-08

Sol investigated, implemented and reviewed this follow-up independently. No
delegation, game launch/control, runtime probe or Tester Logs access occurred.
The user's current game-directory FFX_SR_Native.log was authorized evidence.
Older tester review and deployment PASS records do not establish gameplay
success for these new reports. Release packaging is outside this follow-up.

## Navigation evidence and repairs

The own-session log shows Guadosalam Inn route failure at10:38:14,24,28,31 on
resource generation12, while distance fell from12.4 to7.1 walking units; the
user reports success nearby. Destination discovery and the graph search have no
player-to-target distance cap. The exact live mesh is not in that log, so it
does not prove which geometric cause applied to this encounter.

Three concrete failure mechanisms are repaired. Walking now uses the native
controlled actor's collision triangle (+824, written by83E5F0) at the player's
snapshot position, with bounds/passability/containment validation. A global
height comparison could previously choose a disconnected overlapping polygon.
Native adjacent polygons now build portals from their identical physical edge
coordinates; exported seams with distinct vertex IDs are valid. Route A* skips
adjacency without a physical shared edge so a bad shortest shortcut cannot
prevent trying an existing valid route. All validated interaction approaches
are retained and tried in cost order, rather than stopping after one failed
non-line approach. These apply centrally to exits and other interactibles.

Unreachable filtering remains conservative: unknown origin/endpoint, missing
geometry or malformed portal is not proof of an unavailable exit. Only the
existing native mutable-gate counterfactual can remove an otherwise enabled
exit for connectivity loss. Callback flags/guards separately determine whether
a story event or cutscene has disabled the exit itself. Geometry and filtering
fixtures cover arbitrary distance, exported seams, malformed shortcuts,
overlapping origin floors, closed/reopened native gates and unknown floors.

M now reads the field controller's primary script chunk at+1F8 and the saved
entrance through46BD20 and the native area-name getters. Controller+A is the
controlled collision worker (866680), not a current-map index; its worker can
belong to another loaded script. The entrance fixture deliberately gives that
worker a conflicting chunk. Destination labels retain their existing getter.

## Active events and interactions

866680 and85FA30/85FF00 prove worker+34 bit80 is the physical active flag.
Interaction discovery already checked it, actor enablement, callback enablement,
native priority, remapping and catalog semantics. The additional missing test
was the callback's **current script guard**, rather than static possible outcomes.

A shared read-only guard evaluator in nav_interactions.cpp follows pure
saved world/story integer variable reads, registers, logic/comparisons and branches before any
native call/write/request. A proven immediate return has no playable outcome
and is omitted. Unknown operation/type, unreadable state, malformed data or a
bounded guard-review limit retains the interaction. This limit cannot hide a
target and does not bound pathfinding distance or graph traversal. Per-worker
motion/proximity and other transient variable locations remain unknown; they
cannot remove an enabled exit as the player walks around. No native
effect is executed and no state is written by this evaluator.

Real archived kami0000 worker46 entry4 permits the Tidus conversation only at
story progress2385..2875, then otherwise returns; worker56 entry2's Menu-labelled
scene permits1210..1300. Fixtures load the original EBP and verify enabled,
future/completed and active-bit-disabled conditions through the production helper.
Guadosalam worker32's Menu-labelled scene enables its native shape only at
story1190; no label-based suppression is added. NPCs and other ordinary targets
use the same live helper; authored trial-control exceptions remain intact.

## Battle producers and Summon Help

The own log announces one initial formation at10:23:08, then speaks
Pre-emptive strike at10:27:33 without another formation; the user also reports
normal encounters affected. Field/battle teardown alone left formationRead
latched between same-map encounters.3810F0 is the actual per-encounter native
initializer for normal/direct battle loads. Its hook resets battle reader
ownership at the new encounter. Existing visible enemy bounds, complete
appearance gating, queued native banners and speech protection remain. Fixtures
cover consecutive identical normal, pre-emptive and ambushed formations without
an artificial field teardown, and refuse repeats within one encounter.

Tidus's Cheer action is present at10:31:28 without a status change; H later
reports Cheer1 at10:32:12.38D7F0 writes the six buff stack bytes+65E..663 and is
called by38E680, the command damage/status processor. Existing38F0B0/39A1A0 hooks
did not cover this path.38E680 now owns before/after status snapshots for its
target ID (arg2), cross-checked against its target pointer (arg3). Nested clears
belong to that mutation, preserving producer ownership without text deduplication.
Fixtures cover repeated Cheer stacks, Protect application/removal, unchanged
effects, target mismatches and existing KO/revival behavior. No effect is announced
before a committed state change.3810F0/38E680 signatures were uniquely qualified
from the retained native reference and registered in dependency validation.

Summon details combine native Help atF3F4F8 with the selected aeon's displayed
HP/MP. I reads both freshly at keypress, even if Help/MP changed after the highlight
callback or automatic details are Off. The Magus Sisters retain all three actors.
Change remains automatic on highlight; H retains the active-turn owner.

492C20 draws current HP and MP numbers and492880's Overdrive bar;395560/3955A0
return limit_charge/limit_charge_max atactor+5BC/+5BD. There is no MP bar or maximum
number in these rows. The user's HP percentage/max-number preference is retained;
MP stays numeric. Earlier HP-bar documentation is corrected.

## Lightning and verification

Both failed lightning features shared a cursor guard that expected the native
CALL to have finished advancing. It had not; the exact current PCs are34B4 and
65EB. The retained VM bodies prove the return hook runs before cursor advancement.
The real EBP fixtures now model that point. Routine route audio cancellation
also no longer erases the independent warning. See [full lightning evidence](thunder_lightning_20261008.md).

Implemented, offline-verified and deployed through the canonical guarded entry
point with the game/launcher closed. Full native suite:39 PASS summaries.
After the final conservative SaveData-only guard review, both reader profiles
passed again (10 summaries). Final canonical x86 game/launcher build and freshly
built proxy check are clean. Deployment verified compiled FFX and Will-caption
profiles, launcher bootstrap and13 installed SHA256 hashes. No game entry point,
speech or hooks ran during compiled-profile verification. Current evidence:
install/player-reports-20261008/verification.json. Release ZIP unchanged.
Gameplay remains user-side: distant Guadosalam Inn/M, inactive Menu/Tidus events,
consecutive battle formations/Cheer, Summon I and both Thunder Plains modes.
The exact live inn mesh and real warning/dodge timing still require acceptance.
