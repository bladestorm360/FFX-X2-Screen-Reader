# Macalania testing, Bevelle entry trigger, Zanarkand directions — October16

Kilika, Djose, Bevelle and Zanarkand's normal first-visit solver are now
USER-CONFIRMED WORKING. The user reported that `warp macalania-test` failed
before transitioning. Its source-map lookup is repaired and passes both offline
build-mode fixtures; the replacement is now guarded-deployed and verified. Zanarkand's
new wording and return treasure remain user-side.

## Macalania: corrected diagnosis and explicit testing warp

The user already loaded an early Salvage Ship save before `warp macalania`.
The previous answer blaming a completed save was incorrect. Ordinary warps
preserve progress, but the destination's original initializer still selects
its layout from that progress. An earlier save alone does not select the
later playable puzzle layout.

Original `mcyt0500` archive SHA256:
`c77e5c79227d264fa9d7c86957cd045a5fd041bcecc6caf687c9244fef2d6a68`.
W30:1 at58980 checks story<1530, which enters the early-story sequence and
eventually writes1530 at59383. Its preceding branch58887 selects bridge removal
only for story>=1550 and the native entrance getter returning1. That branch
hides layers8–11 and restores the associated collision states. Entering through
the ordinary entrance with an early save does not take that branch.

There is a separate initialization selector `L0:map:000280:T0`; W0:0/1 cases2/3/4
have special placements and state writes. This is distinct from native entrance
Common0010 ->856810 ->86C160 ->88D5E0, which reads SaveData+0C. Do not advise
`warp 239 2` as if it selected the special initialization case. That lead was
discarded after following the original getter/transition bodies and table data.

New development-only console target: `warp macalania-test`. It selects room239,
entrance1, and sets the story declaration to1550 immediately before the existing
native transition. It requires current story<=1530 and a zero special selector;
later progress, force/entrance overrides, active utilities, battle, dialogue,
stale context or missing/changed declarations are refused before story writes.
The UI/copied command cannot change game state. The existing bounded native
action resolves the original getter/declaration and setter on its owner; only
its qualified invoke performs the one story write and transition. No new
thread, polling, installed asset read, pointer transfer to a worker or capture.
Ordinary `warp macalania` retains its original state-preserving behavior.

This command intentionally advances the current play session's story for a
solver test. The console response/help describes that effect and says to reload
the original save afterward. The agent never executes it against the game.
It does not reset treasure grants or provide a general-purpose save reset.

## Bevelle: accessible activation before the rails

The user's entrance report is consistent with original data: the pedestal model
W13 has an empty talk entry, while the entrance crossing zone W39 (type2) entry6
at116381 disables the player, requests player/rail camera actions, then pedestal
W13:6 at116450. Original RET is116484. No script REQ caller of W39:6 occurs in
the archived flow; it is the native entrance edge used by the existing verified
pedestal assistance (`pedestal_rules.cpp` control7, `pedestal_input.cpp`).

The generated physical contract now includes that exact W39:6 original entry
and RET. It intercepts before player lock/ride effects, releases through normal
RET and starts the same original reveal/chest/completion sequence. The solver
announcement tells the player to push the rail pedestal onto the entrance track.
The eight sphere/recess controls remain usable once the player has ridden the
rails. Off leaves native rail boarding intact. The user subsequently confirmed
Bevelle's auto solver working on the trial-entry delivery. Total contracts:98 across six
trials, including9 for Bevelle. Worker activation permissions remain Kilika-only.

## Zanarkand: confirmed route and clearer directions

The user confirms the normal solver works but reports that reaching its first
encounter requires leaving, re-entering the trials, then taking the exit called
"Cloister of Trials." Completion, status, enabling at an already open gate,
initial solver guidance and the manual gate step now spell out that route.
The gate directions share one literal where applicable; the authored manual
route is regenerated from its source. First/return boundaries and original
gate scripts are unchanged. Return-visit treasure and the new wording have no
new gameplay confirmation from this report.

## Verification and remaining checks

[Read-only original bytecode replay](Scripts/Offline/audit_trial_entries_20261016.py)
and [timestamped entry evidence](../../Analysis/FFX/RepositoryEvidence/offline-output/trial_entries_20261016/replay_20261016_173349.json):
seven Macalania story/entrance cases establish the early path and the exact
bridge-removal branch; original Bevelle W39:6 establishes the lock/rail requests
and RET. Integer/branch/request instructions execute with real script tables;
native graphics/camera/wait effects and requested workers remain modeled.
[All98 original contracts](../../Analysis/FFX/RepositoryEvidence/offline-output/trial_intercepts_20261016/contracts_20261016_173158.json)
match entry/RET boundaries, source hashes and production bytecode identities.

`install/trial-entry-utilities-final-20261016.log` passes in both development and
release. The new `trial_entry_fixture.inc` exercises production parsing/native
deferred action: no story mutation while parsing/queued/preparing; exactly one
setter before transition; later/special/stale/force/entrance/battle/dialogue
refusal; ordinary warp preservation. Original Bevelle type2 crossing reaches
reveal/chest/completion with no original boarding effects and preserves Off.
The full98-case/38-first-gate matrix and existing Kilika/Djose/Gagazet/Zanarkand/
Via/owned-choice tests pass. Zanarkand directions are asserted in its fixture.
The first compiler run found a missing utility_scripts.hpp dependency; fixed
before the final passing run. Generator first refused an out-of-range sphere
binding for the new non-sphere edge; it now correctly has no sphere binding.

`install/trial-entry-pedestal-20261016.log` passes all-trial contact/Space/remap/
cancellation checks. `install/trial-entry-profile-20261016.log` passes native
signatures, operands, dependencies and transactional refusal. Canonical
development native/original-launcher build has zero warnings/errors and572
stable source hashes. [Build/source provenance](../../Analysis/FFX/RepositoryEvidence/historical-install/trial-entry-source-20261016.json).
Guarded Preflight/Installed checks pass; all7 installed artifacts,15 locked
PRISM files/receipt, launcher bootstrap, unchanged user INI and unchanged held
installer/ZIP are verified. Installed proxy:
`7adb93a07c214f7cdd2457578b7211bd6fa2509d8bc202272e37e12cc4342483`.
[Delivery receipt](../../Analysis/FFX/RepositoryEvidence/historical-install/trial-entry-deployment-verification-20261016.json).
Game and launcher were closed throughout deployment; no game entry point/hooks/
speech ran during signature validation. No installer rebuild/package.

Priority gameplay steps after deployment:

1. Load the original early Salvage Ship save. Open Shift+F1; run
   `warp macalania-test`. Enable Macalania's solver and interact with a sphere
   or recess; choose the intended goal. Expected: unfinished layout, treasure
   collection and selected normal completion. Reload the original save afterward.
2. Bevelle auto solver is now user-confirmed working; no repeat requested.
3. As progression permits, confirm Zanarkand now announces leave/re-enter and
   "Cloister of Trials." Its working gate/encounter behavior is preserved.

Actual rendered layouts, collision/player animations, dialogue, completion and
end-to-end responsiveness require user gameplay checks. No game launch/control,
runtime probes, Tester Logs or installed Macalania/Bevelle source reads. Sol
owns discovery/implementation/review/checks; no delegation or Astra. Installer
and ZIP rebuilding remains held; reset monitor remains PAUSED.


## Follow-up: failed testing warp from the actual Salvage Ship source

The user's own session reported "The field transition was cancelled or failed."
The earlier bounded own-log read established Salvage Ship entry at11:13:27.776,
test request at11:13:44.847, then "native action preparation failed" and the
reported refusal at11:13:44.952. No room transition followed. This is a source
preparation failure, not a failure of the destination initializer. No Tester
Logs were accessed.

The original `cdsp0000.ebp` SHA256 is
`c0a1460ce2ca0f019d1cb0cef6b2cefc2e4e6d9c4b0c171fb0db8559064ecc21`.
Its78272-byte ATEL has worker0 header0xC8 and35 declarations: story
`20000A00/1` exists at index0, but selector `00000280/1` does not exist.
The earlier fixture synthesized Macalania's declaration table for the source,
including the absent selector. Production's failed declaration lookup threw;
the real action executor catches that exception and completes unsuccessfully.
The old fixture wrongly let preparation exceptions escape to the parser.

The repair retains the selector guard but obtains its one byte using the
original native location0 owner. Retained `FUN_0086c270` (VA0086C270,
RVA0046C270,232 bytes) resolves location0 to
the bytecode base pointer stored at controller offset 0x2C, plus the low 24 bits of raw_low (references: 0xFFFFFF). The existing native story getter
(RVA0046C500, cdecl(worker,index,element)->pointer) must first agree with that
base+0xA00, after qualification against the exact prepared source declaration
and live allocation. Only then are the2-byte story value and1-byte selector at
base+0x280 copied. Null, overflowing or mismatched owners refuse before any
native flag/story mutation. No source selector declaration is required.
The original setter/transition ABI and single owner-local story write remain.
Specific refusal literals survive deferred completion for unavailable script/
declaration, invalid owner, later story and special selector; these cases no
longer fall through to the generic preparation failure message.

`prepare_utilities.py` verifies the archived EBP hash and stages the complete
original Salvage Ship ATEL with a declaration/hash provenance JSON.
[Staged provenance](../../Analysis/FFX/RepositoryEvidence/historical-install/native-tests/cdsp0000_provenance.json) records
ATEL SHA256 `da4405d47784f1e55e3cdf6d7b1c9f081dfc1a697de31acecb0070d78b83ebfd`.
The regression loads that original header/table, explicitly asserts the absence
of selector, and uses recovered original location0 addressing in the mock
native getter. Both native fixture modes now follow the real executor's
exception/completion behavior. Tested progress0/95/1529/1530, later progress,
nonzero selector without a declaration, null/overflow/mismatched owners,
missing story declaration, queued/read-only preparation, a full immediate
source transition, stale/battle/dialogue/force/entrance refusal and normal warp
preservation. Both98-interaction/38-first-gate matrices still pass.
[New offline log](../../Analysis/FFX/RepositoryEvidence/historical-install/macalania-source-warp-utilities-20261016.log).
The pre-existing fixture-only C4459 shadowing warning remains; no new warnings.

This adds a constant bounded owner/pointer check and a single byte copy on the
existing native action path. It adds no polling, new hook, worker, scan, wait or
queued retry. Source/target state remains generation-owned; cancellation before
invoke changes no story data. Native runtime timing and actual arrival layout
remain unmeasured/user-side. Native getters/setters/transition and animation/UI
are modeled dependencies in this source fixture; original destination branch
replay remains separate evidence. No game launch/control/probe or installed
game-source read was performed. Sol retained this scoped ownership repair and
verification; no new delegation or Astra. Installer remains held.


Final repair delivery: canonical `src/proxy/build_dinput8.bat --no-deploy`
completed with zero compiler warnings/errors and572 stable source hashes.
[Build/source record](../../Analysis/FFX/RepositoryEvidence/historical-install/macalania-source-warp-source-20261016.json).
Guarded preflight and installed checks passed with game/launcher closed.
All7 installed artifact hashes and15 locked PRISM files/receipt match; user
INI and held installer/ZIP are unchanged. Existing unchanged pedestal/profile
fixture evidence is retained, and the guarded preflight revalidated native
signatures/dependencies against the newly compiled payload. No unrelated
fixture rerun or package rebuild was needed.
[Final installed verification](../../Analysis/FFX/RepositoryEvidence/historical-install/macalania-source-warp-deployment-verification-20261016.json).
Installed proxy SHA256: `6d856336e8959d0a7b98c04a521b39bf5f4c3edaf4f732ab00ec10f0861b5897`.
[Repair/test/resolver input hashes](../../Analysis/FFX/RepositoryEvidence/historical-install/macalania-source-warp-regression-inputs-20261016.json).

Next user check: load the original early Salvage Ship save, run
`warp macalania-test`, then enable and test Macalania's solver. Expected: the
command reaches room239 entrance1 in the unfinished playable layout instead
of failing in source preparation. Reload the original save after testing,
because this explicit testing command advances the current session's story.
The repair is implemented/offline-verified/deployed; actual arrival and solver
gameplay confirmation are still pending. Bevelle needs no repeat test.
