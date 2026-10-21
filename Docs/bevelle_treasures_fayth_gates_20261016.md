# Bevelle treasures and native fayth returns — October16

Implemented, offline verified and guarded-deployed. Installed payload is the
new verified Bevelle-treasure/normal-return delivery documented below. Macalania normal trial solver
is now USER-CONFIRMED WORKING; prior fayth choice was untested and retracted.
Kilika, Djose, Bevelle and Zanarkand first-visit confirmations remain valid.

Bevelle: user clarified the tester's "couple" may mean only one extra chest.
Original bvyt1100 contains exactly two native reward calls: W16:8 entry79631
calls Common015B(window1,treasure217) at79699; W17:7 entry79849 calls the same
native grant for treasure102 at79905, RET79938. Approaching mapbvyt1000 and
antechamberbvyt1200 contain no reward calls. The old solver called only W16.
Original W15 guard at79266 checks map01F4AA=0, REQEW W17:7 at79278, then writes
01F4AA=1 at79283 after the original chest returns. Worker0 binding is0C00002A/1.

Production adds a generated optional action/marker to PuzzleCompletion. After
the destruction treasure, the solver waits for the original branch chest,
records that original caller marker only on its exact owned RET, then leaves
through the existing native exit. Already collected chests are skipped. Off
allows a current chest to finish and record its marker without resuming the
exit. Epoch/allocation/worker/signal checks reject stale acknowledgements.
No inventory grant, story seed, new timer or worker is introduced.

Evidence: Docs/Scripts/Offline/audit_bevelle_treasures_20261016.py and timestamped
output under output/bevelle_treasures_20261016/audit_20261016_185004.json. The
original bytecode replay reaches each native grant before its address operand/
REQCHG acknowledgement; the remaining straight-line RET tail is checked from
original instructions. Native inventory/UI/graphics/acknowledgement remain
modeled. Initial attempt to replay the full callback stopped at unsupported
A7; retained audit explicitly bounds that gap. The retained Common015B init/
exec bodies are install/bevelle-treasure-native-20261016.txt; its requested
ret-int function85B360 is absent from the decompile archive, recorded as a gap.
Both build-mode logs install/bevelle-treasures-utilities-final-20261016.log pass
new bonus/skip/Off/failure/epoch/allocation/identity cases plus98 interactions,
38 first-visit gates and the real Salvage Ship source regression. New generated
contract provenance: puzzle_completions_20261016_184642.json.

New user instruction: remove the old Macalania Complete trial/Visit the fayth
chooser. Learn actual temple return eligibility and implement fayth branches
only where the native game allows entry; user suspects a gate after Bevelle,
but do not use that suspicion as a universal threshold. Sol investigates data,
conditions and ownership. Retained candidate flows are under
Docs/Scripts/Offline/output/fayth_gates_20261016: bsyt0500, klyt1100, djyt0700,
djyt0800, mcyt0600, bvyt1200, dome0700, dome0200. Early story choreography
already goes to first-visit fayth rooms and must not be mistaken for a permitted
return. Macalania's old antechamber script has no direct284 warp; distinct late
endg0300 room388 exists. Besaid/endg0200 room387 and Bevelle/endg0400 room390
likewise require tracing. Exact late-story/native door guard remains unresolved.

Bounded Luna delegation: remove_maca_fayth_menu, gpt-6-luna medium, none history;
allowed mod_menu.cpp/.hpp and accessibility_fixture.cpp only. Current deletion
was inspected: old page/declaration/handlers/help/assertions are absent and10
page definitions align. Luna reports an initial exact-text replacement failure
and cannot attribute all deletions; Sol did not edit these files concurrently.
Do not claim all deletion provenance as certain. Integration/test remains Sol's
responsibility. No Astra, runtime launch/control/probe or Tester Logs access.
Installer/package remains held; heartbeat paused. Current usage83% used/17%
remaining, preserve2% stop threshold.


## Final scope after the user's return-flow clarification

The user clarified that a separate fayth branch may be unnecessary: complete
the trial normally on a return, then let native access continue to the fayth.
This matches the recovered normal exits. The old Macalania chooser, goal state,
command handler, enum/API override and menu were removed completely. No new
fayth branch, universal post-Bevelle threshold, chamber warp or permission flag
write was added. The same ordinary first/return completion contracts remain:

| Trial/context | Original continuation | Native destination |
| --- | --- | --- |
| Besaid before164 | W30:5 tail23374 | room103 entrance0; original story164 write |
| Besaid later | W30:6 tail23569 | room103 entrance0; no story write |
| Kilika | W37:4 PC61041 | room45 entrance0 |
| Djose | W66:4 PC86754 | room90 entrance1 |
| Macalania story1545..1552 | W44:4 PC77628 | room106 entrance1, native escape/hall |
| Macalania outside that story phase | W45:4 PC77722 | room80 entrance0, antechamber |
| Bevelle | W56:4 PC118414 | room226 entrance0, antechamber |
| Zanarkand first visit | existing six-insert gate helper | gate only; player takes native encounter route |
| Zanarkand unlocked return | existing treasure helper | remain in trial; no lift/encounter skip |

[Original-bytecode return audit](Scripts/Offline/audit_normal_trial_returns_20261016.py)
executes seven original empty-stack continuations with their archived native
call/table data and checks the destination calls and bounded story writes.
[Timestamped results](../../Analysis/FFX/RepositoryEvidence/offline-output/fayth_gates_20261016/normal_returns_20261016_190204.json).
This proves the outbound continuations and preserves their downstream room
scripts. Actual downstream permission, collision/party/door/cutscene behavior
is still native and gameplay-unconfirmed; no exact universal gate was inferred.
The distinct endg0200/0300/0400 sequence was not used as return-access evidence.
An initial full-catalog instruction search was stopped after proving too slow;
the replacement used exact destination operand candidates, then actual compiled
callers. No unchanged failed runtime capture or game probe occurred.

The user asked whether repeat solving can skip the Destruction Sphere. It
already does: Prepare and Collect check the original shared temple collection
bit first. A collected chest skips reveal and collection; an objective-ready
but unopened chest proceeds straight to its native collection. Bevelle's new
optional marker is independent. Zanarkand's first/return eligibility remains
separate. The new Bevelle tests explicitly start with destruction treasure
already collected and collect only the still-missing branch chest.

Final Sol integration removed the Macalania goal producer and updated native
fixtures to solve normally without a menu, preserve first/return routing,
retain Destruction-only stay behavior, and reject the obsolete fayth command
without effects. [Both build-mode regressions](../../Analysis/FFX/RepositoryEvidence/historical-install/bevelle-normal-returns-utilities-20261016.log)
pass98 interactions/38 first gates, Bevelle extra chest/Off/skip/identity cases,
real Salvage Ship warp, and existing Kilika/Djose/Gagazet/Zanarkand/Via checks.
[Menu/controller fixture](../../Analysis/FFX/RepositoryEvidence/historical-install/bevelle-normal-returns-accessibility-20261016.log)
passes after chooser deletion. Sol reviewed Luna's shared menu result, verified
all stale declarations/handlers are gone, and validated Page/menu alignment.
The helper's uncertain per-edit attribution is retained above; no functional
issue remains. Production/native hooks and actual thread/pointer ownership
remain Sol's responsibility. No Astra.

The extra chest adds one fixed metadata action and one native request with an
exact owned-return acknowledgement. Pending work retains native-owner-only
worker/signal identity plus generation and resource serial. No borrowed engine
pointer is sent to a worker; no new timer, scan, queue, thread or wait is added.
Original native animations and inventory/dialogue still run normally. Actual
callback/end-to-end timings are unmeasured; fixed work bounds are source
facts, not a responsiveness claim. The existing8-action native queue and its
cancellation/capacity rules remain unchanged.


## Final delivery and short gameplay checks

Canonical development/native/original-launcher build completed with zero
warnings/errors and573 unchanged source hashes.
[Build/source provenance](../../Analysis/FFX/RepositoryEvidence/historical-install/bevelle-normal-returns-source-20261016.json).
[Production/generator/test/audit input hashes](../../Analysis/FFX/RepositoryEvidence/historical-install/bevelle-normal-returns-inputs-20261016.json).
Guarded Preflight and Installed checks passed with game/launcher closed. All7
installed artifacts and15 locked PRISM files/receipt match, launcher bootstrap
is verified, and user INI/held installer/ZIP remain unchanged.
[Final installation receipt](../../Analysis/FFX/RepositoryEvidence/historical-install/bevelle-normal-returns-deployment-verification-20261016.json).
Installed proxy SHA256: `d7cc3d04d3b99df6b49f4e5ff07d33920fdde9550355ff6ae87120efa481b7dc`.
The first final-verifier invocation ran before deployment's process had ended
and found no preflight marker; it created no success receipt. After deployment
exited0 with both guarded phases passed, final verification passed, retained in
install/bevelle-normal-returns-verify-final-20261016.log. No game was launched,
controlled or probed. No installed game-source/assets or Tester Logs were read.

1. Bevelle: run the solver normally; confirm native chest messages. Expect the
   existing Destruction Sphere reward, then the branch chest, then native exit.
   A chest already opened in that trial session is skipped.
2. When normal progression permits, return through a temple and solve normally.
   Expect no special Macalania goal chooser. The game handles onward entry;
   native permission/collision/dialogue remains authoritative. Zanarkand still
   keeps its original first/return and encounter boundaries.

Earlier Kilika/Djose/Macalania/Bevelle/Zanarkand-normal behavior is user-confirmed.
The user subsequently confirmed Bevelle bonus-treasure collection and Macalania
normal story advancement without the fayth branch on this delivery. Other
temples' later native fayth access remains unconfirmed; no universal gate claim.
Installer/ZIP rebuilding stays held. Reset heartbeat remains paused. Latest
account observation85% used/15% remaining; preserve2% stop threshold.


Subsequent user confirmations: Bevelle works with bonus-treasure collection;
Macalania advances the story normally without a fayth branch. Both apply to the
verified d7cc3d04 delivery above. These supersede pending status for that Bevelle
sequence and Macalania cleanup only. The user also requested the Gagazet test
warps again: Shift+F1 console `warp 310 0` first water section/Wakka puzzle;
`warp 310 1` second water section/coloured openings. Both are valid original
entrances of room310 mtgz0700, Gagazet - Submerged Passage; normal navigation
continues from the entrance to the nearby puzzle. No progression reset implied.
