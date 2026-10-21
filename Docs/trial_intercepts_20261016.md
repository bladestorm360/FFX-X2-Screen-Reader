# Trial solver interaction ownership — October16

User reports Djose's ordinary sphere prompt ran alongside its automatic solver:
the character removed a Djose Sphere during treasure collection and interrupted
completion. User then requested the same intercept check for all six trials.
Kilika remains user-confirmed working; its full original insertion and scoped
disabled-worker activation are preserved.

Implemented, offline verified in development/release and guarded-deployed.
Djose is USER-CONFIRMED WORKING after this deployment. Other newly expanded
trial intercept cases remain offline verified, with gameplay confirmation pending.

## Evidence and cause

Own installed `FFX_SR_Native.log`, not tester submissions:09:42:01.951 original
Djose reveal W44:2 PC80932;09:42:07.857 chest W24:2 PC47845;09:42:07.858
"A Djose Sphere is inserted here";09:42:08.125 "Take sphere";09:42:09.359
"Obtained Magic Sphere!";09:42:09.659 "Removed Djose Sphere."
[Narrow issue excerpt, source path/hash/time](../../Analysis/FFX/RepositoryEvidence/offline-output/trial_intercepts_20261016/own_log_excerpt.json).

`puzzle_solver.cpp` previously returned immediately on every non-Idle phase
except Kilika. It also lost active completion protection once new activations
were disarmed. Thus the initial intercept succeeded, but subsequent queued
player interactions ran their ordinary scripts during solving.

Retained original Djose `djyt0600` archive SHA256:
`0423e1fa35d1771bf387bdd07d6a7ca794dec5aaf7fb18104a25ca2d9f2938bb`.
Original W48:2 Entry sphere recess1 at82470 disables player control, sets its
socket selector and requests W43:2 at82503. W43 owns the normal sphere UI.
W49–55 similarly request W43 entries3–9. Retiring each verified wrapper at its
original RET before its first instruction prevents those competing prompts;
the original native scheduler releases requests and acknowledgements normally.

[Reproducible read-only audit](Scripts/Offline/audit_trial_intercepts_20261016.py)
verifies each retained archive SHA256, production bytecode length/FNV32,
original entry/RET instruction boundaries and generated physical contracts.
[Timestamped all-six audit](../../Analysis/FFX/RepositoryEvidence/offline-output/trial_intercepts_20261016/contracts_20261016_165709.json).
The audit also checks choice-producing entry2 bodies outside the contracts:
Besaid W31/32, Djose W43 and Macalania W30 are internal type4 helpers;
Zanarkand W9 is its native save/menu producer (saved-room and modular-menu
calls). No omitted type2/type3 choice producer was found. These remain ordinary
native helpers/menus, rather than being added to the puzzle-player intercept.

| Trial | Verified physical interactions | Solver entry shared with a physical interaction |
| --- | ---: | --- |
| Besaid | 11 | Reveal W46:2 |
| Kilika | 11 | Reveal W35:2 |
| Djose | 19 | None |
| Macalania | 10 | None |
| Bevelle | 8 | None |
| Zanarkand | 38 | Return reveal W49:2 |

## Production repair

The existing player-entry intercept now remains active throughout all six
solver sequences, including the goal selection and disarmed completion phases.
It retains the original objective and never starts another solve in response
to queued input. Protection ends with the sequence, opt-out or generation.

The preceding utility listener's exact worker/signal/epoch initialization receipt
still excludes every owned reveal/chest/completion request before the intercept.
This is essential for the shared entries above and real native signal reuse.
Other workers, entries and internal source workers remain outside the player
intercept. Allocation, original entry table, bytecode RET and level guards are
unchanged. No choice menus are blanket blocked or automatically accepted.

Zanarkand's first-visit original gate action writes2775 before returning. That
specific in-progress gate completion remains protected after its native write;
the solver still stops at the gate and leaves the native encounter exit to the
player. Return treasure remains a separate objective; no boss/lift/story skip.

All reads and the one request-PC write remain on the existing native owner
initialization callback. The search is bounded by the current trial contract,
at most38 records, with no new polling, waits, worker pointers or input route.
This repair has no measured target-game latency claim.

## Offline validation

- `install/djose-utilities-before-20261016.log`: regression fails against the
  previous source on the first queued Djose interaction during reveal.
- `src/native/tests/djose_solver_fixture.inc`: all19 Djose interactions retain
  reveal/chest/completion ownership and original objective; all8 real type3
  wrappers replay the archived x86 enqueue/list/dispatch/init/RET bodies using
  actual bytecode and production utility/solver listeners. Their intercepted PC
  is the original RET; owned continuation PC/request/write/speech is preserved.
  Off restores the exact original entry PC and native RET frees the request.
- `src/native/tests/trial_intercept_fixture.inc`: all97 actions across six trials,
  original reveal/collection/ordinary completion, Macalania goal selection,
  shared-worker receipt, unrelated internal-source/entry exclusions, opt-out and
  original objective preservation after the carried sphere changes. All38
  Zanarkand actions also run the normal first-visit gate, queued input both
  before/after its2775 write, and no treasure grant/encounter shortcut.
- Full utilities also retain the earlier Kilika disabled/halt/scheduler/signal
  replay, owned choice ABI replay, Gagazet native success/manual preservation,
  Zanarkand displayed patterns, Via chest and development/release console checks.

Fixture corrections during expansion: its old name storage at+100 overlapped
the100-entry worker table when local50/51 was loaded; moved only fixture name
storage to+1D0. The matrix initially expected Besaid's later completion on its
first visit; corrected to its original first-visit continuation. These failures
did not require changes to game contracts. Test-only existing C4459 in
`utility_choice_replay.inc` is separate from production build diagnostics.

The queue replay executes original x86 scheduler functions; interpreter
animation, camera, dialogue, sphere-state and chest effects remain controlled
dependencies. No game launch/control/probes or installed Djose source reads.
Tester Logs was not accessed. Sol owns this fix, audit and validation; no
delegation or Astra was needed for this bounded repair.

Final verification: `install/djose-all-trials-utilities-final-20261016.log` passes
in both development/release (two complete97-action/38-gate matrix results),
and `install/trial-intercepts-profile-20261016.log` passes signature/operand/
dependency/refusal checks. Canonical development native/original-launcher
`src/proxy/build_dinput8.bat --no-deploy` has zero warnings/errors and571 stable
source hashes. [Build/source provenance](../../Analysis/FFX/RepositoryEvidence/historical-install/trial-intercepts-source-20261016.json).
Guarded Preflight/Installed checks pass; all7 installed artifacts,15 locked PRISM
files/receipt, launcher bootstrap, unchanged INI and unchanged held installer/ZIP
are verified. Installed proxy:
`7bccac5ed0dcb296f63b179533706e751677ff3d201faf452b89d9999f123d24`.
[Deployment receipt](../../Analysis/FFX/RepositoryEvidence/historical-install/trial-intercepts-deployment-verification-20261016.json).
Game and launcher remained closed. No process entry point/hooks/speech were run
by compiled signature validation. Installer/ZIP remains held.

## User gameplay check

Priority: enable Djose automatic solving, then interact with Entry sphere
recess1 again. Confirm treasure dialogue normally. Expected: the treasure is
collected and the native trial completion proceeds, without an ordinary
"Take sphere" prompt or "Removed Djose Sphere" during solving. Repeated Confirm
must retain the original solve. With the solver Off, normal sphere prompts work.

As progression permits, repeat one ordinary sphere/recess activation in Besaid,
Kilika, Macalania and Bevelle. In Macalania select the intended goal. First-visit
Zanarkand must open its gate and remain there for the native exit/encounters;
the return solver collects treasure and stays. Actual animation/dialogue and
end-to-end responsiveness remain user-side confirmations. Earlier feature
checks remain in the active puzzle/battle record. Installer/ZIP remains held.

## Subsequent user confirmation and Macalania test prerequisite

User confirms Djose worked with the deployed repair. User then reports
`warp macalania` loaded an already completed trial. The current development
warp only changes room/entrance and deliberately preserves story/puzzle state;
`trial` commands support status/treasure/collect/finish, with no reset/restart
command in that build. The initial answer said an earlier save would suffice;
the user corrected this: an early Salvage Ship save has the same behavior.
That diagnosis is superseded by the verified native-initialization finding and
separate testing command in [the entry follow-up](trial_entry_followups_20261016.md).
