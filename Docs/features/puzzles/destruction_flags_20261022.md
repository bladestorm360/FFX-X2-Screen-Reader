# Automatic destruction-sphere flags — October22

Implementation, offline verification and guarded deployment are **complete**.
The user confirms Besaid sphere collection in gameplay on October23. The
remaining Baaj check is remote reproduction of the Ruins unlock through normal
gameplay without warps. A tester reported Zanarkand missing, but its flag was
already present in the supplied save; a missing Zanarkand flag is not established.
The original report reopened collection verification:
first Besaid, then Zanarkand, with the other four reported working.
Auto must retain each temple's actual Baaj prerequisite, as well as its reward.

The named `Saves/Dylan/Summon/ffx_000` was read without modification. Its SHA256
is `470724d9bc6176ffa170357ff9a72921a09200ca54395add4a048a252a836f3e`;
file offset0xC9C contains0x3E. Thus Besaid mask1 is missing; Zanarkand mask32
and the other four are present. This file establishes saved state, not which
earlier action failed. The separate [Baaj activation result](../navigation/baaj_seals_20261020.md)
remains valid; it does not establish correct Auto collection.

## Design and native contract

The original chest entries grant their rewards and set saved declaration
01000A70. Native Baaj guards read that same byte. There is no separate
inventory-sphere requirement in those guards. Keep the exact original chest,
dialogue and return sequence; record the matching temple bit only after its
owned final RET, preserve every other bit, and verify it before completion.
No grant or flag follows a refused/interrupted/foreign/stale request. Off lets
the existing chest finish and retain its result without continuing to the exit.

The earlier tests injected collection flags instead of executing chest effects.
This repair adds original-bytecode checks for the actual grant/flag sequence and
native consumer regressions for every flag combination, including the named
save's Besaid-missing state. Fallback injections test the postcondition; they do
not reproduce the original missing flag's unknown cause.

All state and bounded native reads/writes remain on the established native
owner; no new hook, polling, worker pointer transfer or input route is needed.
No game launch/control/probe, Tester Logs or save edits were performed.
The initial maintenance did not rebuild the installer or publish a release.
The later [1.0.1 release request](../../release.md) authorizes that delivery;
gameplay confirmation stays separate from offline checks.

## Original mechanics and evidence

Retained models are outside Git in `Analysis/FFX/Summon_20261020/label_recheck_20261020`.
Original EBP files and enriched declarations remain under
`Analysis/FFX/RepositoryEvidence/offline-output/FFX/20261003_audit`; their
SHA256, bytecode hashes and the exact generated continuation/final RET are
checked by `Docs/Scripts/Offline/audit_destruction_flags_20261022.py`.
The new timestamped result is in `Analysis/FFX/DestructionFlags_20261022`.
No installed game asset was read for this investigation.

| Temple | Native script / chest worker:entry | Collection PC | Grant PC / treasure ID | Saved-bit write PC / mask | Final RET |
|---|---|---:|---|---|---:|
| Besaid | bsyt0100 /16:2 |10538 |10631 /15 |10675 /1 |10831 |
| Zanarkand | dome0700 /32:2 |18509 |18597 /209 |18641 /32 |18664 |
| Kilika | klyt1000 /7:2 |12358 |12423 /19 |12481 /2 |12487 |
| Djose | djyt0600 /24:2 |47845 |48097 /484 |47931 /4 |48139 |
| Macalania | mcyt0500 /19:2 |53593 |53845 /485 |53761 /8 |53887 |
| Bevelle | bvyt1100 /16:8 |79631 |79699 /217 |79749 /16 |79770 |

These are bytecode PCs, not executable addresses. Djose selector69 and
Macalania selector41 retain their original branches. Those two set the bit
before the grant; Besaid/Zanarkand/Kilika/Bevelle set it afterward. All six
effects precede the final RET. The native guards in `bjyt1000` read declaration
01000A70; Fahrenheit's matching SaveData layout names payload0xC5C
`anima_seals_unlocked`, file0xC9C after the64-byte save prefix. A reward name or
inventory count does not establish that this flag was retained.

The replay covers all256 possible byte values per temple,1536 cases total,
and verifies that only the selected mask is added. It executes original integer
instructions/control flow through final RET, with50–104 instructions per chest.
Native grant, animation,
camera, message advancement, request acknowledgement and interaction changes
are recorded/modeled dependencies, not an executed full game. It does not
reproduce why the earlier saved Besaid bit was absent.

## Production data flow, ownership and extension

`src/puzzles/build_utilities.py` generates each chest's unique final RET into
`ScriptAction::returnPc` in `utility_trials_data.hpp`. All other existing
actions retain the default zero/no-new-check contract. The generator selects
the final RET after the actual continuation, excluding Djose/Macalania's earlier
unselected graphic-case return. Regenerating must retain the six masks,
selectors, preparation and original grant calls.

`utility_scripts.cpp::checkUtilityScript` verifies the optional final RET lies
inside the qualified bytecode and remains opcode3C. `Continuation` retains
that PC with the existing exact worker/signal/generation/allocation receipt.
At the existing469170 entry hook, `finishing` captures the current requested
thread PC before native release/dispatch can recycle its signal. The original
interpreter `FUN_00864180` calls `FUN_00869170` while the PC still points at its
RET; the retained `layer2_game_logic.txt` bodies establish this ordering.
`finished` reports a different return as interruption rather than success.
Existing original x86 enqueue/dispatch/initialization/return replays still run.

`puzzle_solver.cpp::pumpPuzzleSolver` reserves a collection receipt before
requesting the original chest. `puzzleCompleted` accepts only that worker and
signal, successful final RET, current trial, matching allocation address and
serial. It reads the current saved byte; if its temple bit is absent it uses
the existing typed `utilityWrite` setter to add just that bit, then reads back
and verifies it. A normal native commit performs no second setter. This is an
explicit postcondition for the user-authorized Auto collection, not a second
reward producer or a global save repair. Other temples/reserved bits survive.
The existing final-phase check still refuses completion without collection.

Off retains the running chest receipt so the already-started game dialogue and
flag can finish, but does not restart completion or exit. Reset/generation
changes retire it; refusal/interruption clears it without a marker. An
allocation replaced at the same address cannot inherit the old completion.
Workers never receive these pointers. Added native work is constant-size:
bounded ready owner/entry checks, one thread-PC read, two byte reads, at most
one typed setter and one copied diagnostic record per chest. There is no new
scan, parser, queue/backlog, timer, wait or native-thread blocking operation.
Whole-game input-to-speech/animation timing and native tail cost are unmeasured;
offline placement alone is not a responsiveness claim.

Controls/defaults remain the existing per-trial default-Off F8 automatic solver.
The change adds no setting, native interaction or normal-flow message. The
existing interruption feedback and normal chest dialogue remain its output;
technical failure diagnostics retain the existing English utility-error
fallback and do not establish complete mod UI localization. Zanarkand's
story-gated return puzzle stays separate from its normal
first visit. Existing guides, display review, Bevelle bonus chest and normal
completion/exit producers are retained.

## Checks and remaining gameplay

Both development/release `run_tests.bat --utilities` passed384 production
flag-state cases, Besaid0x3E first/Zanarkand0x1F second, reserved bits,
already-collected reward skips, no redundant normal setter, exact worker/signal,
Off, early native return, explicit interruption and generation/allocation
changes. Additional destruction-only stay cases use each trial's real
carried-sphere goal where there is no dedicated destruction entry. The98-entry
intercept matrix,38 first-visit Zanarkand gate cases and original native queue
hosts remain included. The final additional-case rerun passes in both modes:
`install/destruction-flags-utilities-verified2-20261022.log`.

An external copy of the prior solver, built with the current fixture/contracts,
fails the injected Besaid missing-marker case; the updated solver passes.
`install/destruction-flags-baseline-20261022.log` preserves that expected
failure. This isolates the newly required reconciliation, not a proven historic
gameplay root cause. Initial sandbox runs were unable to canonicalize their
source file before main; host-access reruns resolved that filesystem boundary.
The pre-existing fixture C4459 in `utility_choice_replay.inc` is not a production
warning. No delegation was used: discovery, implementation and checks stayed
with Sol.

The full canonical release/launcher build is warning-free. Both173-hook reader
profiles, both compiled-image/dependency preflights and guarded deployment pass.
All seven installed files match staged hashes. Accessibility INI, named save
and published1.0 ZIP hashes match the pre-deployment snapshot. No installer,
package, tag, remote or published release was changed. Current receipt:
`install/destruction-flags-20261022/verification.json`, with before/after hashes
and exact source/log fingerprints. Native build, readers, preflight and deploy
logs use the corresponding `install/destruction-flags-*-20261022.log` names.

The source/documentation inventory still maps all143 production units and37
settings, with no broken local/source links. Existing comprehensive feature
topics are reused for unchanged code; this repair adds its own indexed topic.
Compared with the prior419-input receipt, production differences outside the
four changed trial contracts/implementation files are the previously released
1.0 boot stamp/build resources. No unrelated feature behavior changed. The
current-content/all-local-object audit finds no game-source bodies; its walk
now explicitly excludes every `Tester Logs` directory and keeps the existing
Git exclusions/vendor classification. Original analysis/bytecode remains
outside all Git trees.

The user has now confirmed ordinary Besaid sphere collection. Prioritize remote
reproduction of Baaj Ruins unlock through normal gameplay without warps.
Zanarkand was already present in the supplied save; retain a separate regression
only if a new missing-flag state is demonstrated. For trial regressions expect the
native treasure plus its retained seal prerequisite, no repeated reward and no
exit during a destruction-only solve. A hang, interruption message, repeat grant
or inactive eligible seal is a failure; the own-session log now records exact
temple mask, before/after byte and whether reconciliation was needed. No live
capture or extra diagnostic request is needed at this stage.
