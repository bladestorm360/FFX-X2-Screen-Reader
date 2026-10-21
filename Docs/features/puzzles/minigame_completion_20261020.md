# Optional minigame completion — October20

Status: **complete** for the four documented activities in prospective **0.6**.
Expanded native component/regression checks and the warning-free release-flavor
build pass; guarded deployment verifies all seven installed files. Gameplay
confirmation is pending. The packaged0.5.9 release is preserved;
installer rebuilding and packaging remain held for gameplay testing.

## Player behavior and controls

Use **F8 → Miscellaneous**, or the same controller mod menu, before starting.
Each option is independently saved and defaults Off, including when its INI key
is absent. No new field F-key or completion button is required. Existing cue
settings, manual controls and reward dialogues retain their own behavior.

| Option / INI key | Preparation context | Enabled behavior |
|---|---|---|
| Chocobo training auto-success / `chocobo_assist` | Calm Lands South, before choosing training | Completes whichever of the four courses the player starts; Catcher supplies a native perfect-time result. Confirm the normal results/reward dialogue. |
| Remiem race auto-success / `remiem_assist` | Remiem, before starting the race | First victory, then the next unearned chest-count prize from one through five chests, all without poles. After every tier is owned, uses the native five-chest repeat result. |
| Butterfly hunt auto-success / `butterfly_assist` | Either Makalania hunt map, before starting | Supplies seven pickups in that actual course/story phase. The normal30/40-second timer and cleanup remain. Open the game's chest manually. |
| Blitzball auto-win / `blitz_assist` | Team setup or the match resource | A confirmed live Action Commands choice triggers the player's win. Setup, movement, formation, forfeit, cancelled and unrelated choices do not trigger it. Native results, story, standings and prizes remain game-owned. |

Turn an option Off for ordinary manual play. Enabling outside an active attempt
never starts an activity. Availability uses bounded native room/resource headers,
so the menu does not wait for background completion-resource preparation.
Remiem also accepts its area276 room aliases, including room290. Blitz setup is
bltz0200,113266 bytes/seven workers; its known checksum is58a5c0a5. Availability
alone permits a setting row, never a completion mutation.

The mod emits no fabricated result narration or direct inventory grant. Existing
native dialogue/menus and existing race/hunt readers remain the output producers.
New strings share the current English mod settings UI. Full Auto/manual mod UI
localization is the existing project gap; game text continues using native text
and the established Polish fan-translation handling.

## Source, initialization and ordered data flow

`src/native/minigame_completion.cpp/.hpp` is the sole completion producer.
`reader.cpp` installs it after `installMinigames()` and calls its reset during
resource/reset retirement. `settings.cpp/.hpp` append four settings without
renumbering existing keys; `mod_menu.cpp` filters four direct Miscellaneous rows
through `minigameAssistAvailable()`. Keyboard and controller share that menu.
Opening it calls `cancelMinigameCompletion()` before capturing its state.

The existing interpreter0x464180 enter/leave records a thread-local interpreter
worker/epoch, preserving the previous nested frame. The typed setter0x4713a0
leave event accepts only the matching interpreter parent, worker, entry, exact
POPV PC, variable index, scalar element and live declaration. Initial/reset
assignments rearm an attempt; the first qualified running tick or accepted live
Blitz action queues its completion. Unrelated indices and all-Off preferences
return before native/context inspection. These are existing hooked addresses;
no polling, timer, broad capture or additional physical hook target is added.

A `NativeAction` prepares and immediately invokes at the existing outermost
PInput-owner return boundary. Preparation rechecks focus, scene/battle/menu/
console state, resource generation/allocation, exact prepared checksum, global
worker membership/header, active signal/entry, preference revision, attempt and
context serial, live declaration rows and native target entries. It builds the
whole bounded plan before its first write. Original typed setters and requests
then run on the native owner with ordinary nested hooks enabled. No worker holds
or enters an engine pointer and no game callback waits for a worker.

The component uses the common native variable getter0x46c500 and typed setter
0x4713a0. The retained original base0x46c270/pointer/setter bodies are independently
replayed in the fixture, including actual declaration type/base semantics.
Native request0x46e920 uses source-global, target-global, argument0, priority7,
entry; dispatch0x465390 receives the target global and -1 when an active signal
exists, otherwise10. The native queue/list/scheduler/entry initializer is replayed
from idle and priority0 halted targets; this is not a replacement game VM.

## Qualified original resources and decisive native locations

PCs are offsets into prepared ATEL code; W numbers are resource-local workers.
Full original EBP SHA256s, variant paths, bytecode/declaration proof and exported
native replay hashes are in the external `proof.json` described below.

| Resource | Code bytes / workers / FNV32 | Accepted trigger | Native finish/result entry |
|---|---|---|---|
| nagi0000 base |225132 /150 /6722a493 | W82:7, PC164090, index104 /0x0c0000b0; initialization163914 | Player W109:2 PC215637; Catcher opponent W108:2 PC215288 |
| nagi0000 variant |225368 /150 /42b3fc26 | W82:7, PC164366, index104; initialization164190 | Player215873; opponent215524 |
| lmyt0000 |37071 /105 /ef16e26d | W35:7, PC23068, index17 /0x56000000; initialization22898 | Original finish-line collision W102:2 PC36147 |
| mcfr0100 Central |78486 /55 /908ca3c3 | W37:8, PC31126, index50 /0x56000000; initial30841/30956 | Counter W36:6 PC30448, seventh-pickup chest W27:13 |
| mcfr0200 North |14255 /40 /c248925f | W26:8, PC12339, index35 /0x56000000; initial12054/12169 | Counter W25:6 PC11661, seventh-pickup chest W17:13 |
| bltz0000 three variants |4647290 /41 /2ba296ba,4c6c4230,ef631536 | W36:6 accepted Action Commands return PC3074303, index258 /0x0c00128d | W23 final-period timeout, then W31/W38/outside-match result engine |
| bltz0000 PSV variant |4646208 /41 /601e7c1c | Same accepted assignment at3073221 | Same clock/reset location as the other variants |

Blitz W23:6 clock reset index52 /0x5c000144 at2868116 rearms the next match.
Its PC does **not** shift with the PSV W36 action code. Eight real Common013B
Action Commands displays converge on the accepted return assignment. Native
return values0..2 are accepted;255/cancel and other values are refused.

## Activity mechanics and decisions

**Calm Lands.** A real W82 running-clock write follows the native course start.
Require course0x0c0000bc in1..4, running0x0c0000e1 equal1 and finish bits6 absent
from0x0d0000c9. Zero the three current timer operands0x0c0000b0..b2, player
hits0x0c0000d5, trainer balloons0x0c0000d4 and trainer hits0x0c0000d6. Set player
balloons0x0c0000d3 to1 for Catcher, otherwise0. This gives Catcher a strictly
negative raw time: displayed0:00 alone does not satisfy the original perfect
predicate. Request the original player finish, and Catcher's opponent finish.
W72 owns records, native perfect eligibility, reward274 and already-owned guards.
The first three training stages use their own normal results rather than a
Catcher-only activation or a guessed displayed-time change.

**Remiem.** A real W35 clock tick follows W70's start. Read, never write, native
saved reward flags0x01000a9b. If first-win bit2 is missing, choose five chests;
otherwise choose the first missing tier1..5 using native masks8,4,16,32,64, then
five for fully earned repeats. Set current poles0x0c00000a to0 and current
chests0x0c00000b to that tier. Request W102:2, including mounted-player/control
cleanup and the native first-win/repeat paths into W37's treasure176/chest prizes.
The original investigation's description of W102's W37 requests as debug-only
was incorrect: W103 disables/re-enables this actual finish collision in the
normal race. Always choosing five chests would leave the lower separate prizes
unearned while assistance stayed On; the ordered tier plan avoids that defect.

**Butterfly.** Require active0x0c000041 equal1 and count0x0c000042 in0..6. Count7
marks the attempt handled without another write/request. Set the seven actual
picked guards0x0c000043..49 to1 and count42 to7, then enter the original counter's
seventh-pickup display path. All seven guards are needed to prevent a later blue
touch incrementing the count to8. Current course, story and phase own the chest
and reward, including early prizes and later sigil conditions. W19/W9's ordinary
starter timer/cleanup and W13/W7:8 result processing continue; the player opens
the normal chest. No timer shortcut, direct sigil grant or fixed blue-actor
assumption is used. Existing manual cue/path producers remain separate.

**Blitzball.** At the exact accepted W36 Action Commands assignment, require both
scores0x1452/1453 in0..99, period0x1454 in0..1 and native limit0x5c000148 in1..36000.
Cap opponent at98, retain or raise own score to at least opponent+1, set period1
and set clock0x5c000144 to that limit. Native W23 final-period expiry preempts
the live action and reaches the original result flow. A leading score alone,
or a guessed score10 mercy rule, would not establish completion. No standings,
prize, overdrive, weapon, story or league variable is written. Story special
cases, cutscenes and actual prize presentation still need gameplay confirmation.

## Ownership, bounds, cancellation and recovery

Four owner-local `Attempt` states and four fixed `Job` slots use serial/ticket
identity, one pending action per activity, plus resource/allocation/signal identity.
The common native action queue holds at most eight actions. A plan performs at
most eight typed scalar writes and two native request/dispatch pairs; the rules
array has nine fixed variants. Global membership checks use the existing bounded
native worker table (up to500); no scene-wide producer scan is added. No producer
heap allocation was observed in the measured fixture paths.

An actual setting change increments its atomic revision; Off/On before dispatch
therefore refuses the old job. A no-op setting assignment does not invalidate it.
Opening the mod menu, native window-open0x4aa0b0 or battle-entry0x4a0d70 changes
the context serial. Reset also clears attempt/TLS state. Resource generation,
allocation, signal, declaration or active-context mismatch refuses preparation.
Lost focus, active scene/battle/menu/console are checked again before dispatch.
This never authorizes writing while unfocused or reusing a prepared old resource.

Queue pressure, unavailable preparation, disabled target, empty native free-signal
pool (worker+0x88), changed target entry or failed qualification refuse before any
write. Retiring every action releases its job even on stale-generation discard.
Fresh eligible native events may retry a preparation refusal. Once invoke starts,
mark the attempt handled before mutations: if a later native request refuses,
log it and do not replay a partial completion. A new native attempt rearms it.
Already earned rewards remain native decisions; no text-content deduplication or
new speech suppression is introduced.

## Evidence and verification

Original assets/decoded models and copied native bodies remain outside every
Git tree. Inputs reuse `Analysis/FFX/RepositoryEvidence/offline-output/FFX/`,
`minigames_20261007_104142/` and `butterfly_20261008/` evidence. The additional
Remiem model is `Analysis/FFX/MinigameCompletion_20261020/192607_554908/lmyt0000.json`;
its sibling manifest retains identity/purpose/attribution. Remiem original EBP
SHA256 is81a41edf83d0dd3c956a914f725d4151e7a8cd9b61d7a671682fd926ac63b6d0.
Karifean/FFXDataParser parsing and existing Fahrenheit layout attribution remain.
No new installed asset investigation, game launch/control/probe or Tester Logs
access was performed.

`src/native/tests/prepare_minigame_completion.py` independently matches all nine
production rows to original compiled ATEL statements, actual POPV bytes,
declarations, entries, reset/start PCs, eight Blitz menu sites and reward masks.
It verifies the original native bodies before exporting replay inputs outside
Git to `Analysis/FFX/RepositoryEvidence/offline-output/minigame_completion_20261020/`.
`proof.json` and `cases.txt` retain exact identities and checks. The existing
external native reference is `reader_reference_20261003_235502/block_00401000.bin`.

Run `src\native\tests\run_tests.bat --minigame-completion`. The production
component host uses complete real EBP resources, original typed getter/setter and
request/list/scheduler/entry initialization, with controlled frame/interpreter
scheduling, worker getters, animation/rendering and native stack boundaries.
The passing matrix covers both Chocobo variants/all four courses; all64 Remiem
reward-flag combinations; both butterfly courses at six story boundaries;
four Blitz variants, both periods, score combinations0/3/98/99, accepted/cancel
choices and five story values. It also covers Off/missing/pending/changed data,
wrong PC/entry/interpreter, setting round-trip cancellation, context/epoch
cancellation, queue pressure, disabled/full native targets, one-shot/rearm and
idle/halted original target initialization. Story values remaining unchanged
prove the adapter does not alter them, not that a full story VM has been run.

The component log is [minigame-completion-tests-20261020.log](../../../install/minigame-completion-tests-20261020.log).
Controlled native-owner measurements from that run:

| Comparable path | Observed cost | Heap alloc/free |
|---|---|---|
| Cold event through first original finish-entry initialization, normal table |12.500 microseconds |0/0 |
| Same cold path,500 loaded global workers |27.600 microseconds |0/0 |
|1000 original baseline callbacks |0.066 milliseconds |0/0 |
|1000 default-Off callbacks |0.092 milliseconds |0/0 |
|1000 already-handled On callbacks |2.257 milliseconds |0/0 |

These measure this component through native entry initialization, including its
fixed deferred action, with an immediate controlled return boundary. They do not
measure the entire hooked frame, real input/queue delay, resource cold-load age,
original animations/timer, audible/visible usable results or gameplay latency.
No universal timing guarantee is inferred. Production has no batching delay;
resource preparation may refuse a tick until a subsequent fresh event is ready.
The maximum completion/cancellation tail under real gameplay remains unmeasured.

Settings/defaults exercise all1024 saved combinations and absent keys; the shared
menu host checks each row's independent context, toggle, Back/reopen and close.
Final regression/build/deployment results are retained in the
[delivery receipt](../../../install/minigame-verification-20261020/verification.json).
Settings/defaults, shared menu, existing minigame readers (both profiles), both
butterfly courses/routes, both171-hook full readers, native profile extensions,
ABI, native actions, startup and developer/release utility gates pass. One
existing reader-test C4389 signed/unsigned comparison warning is retained at
`airship_runtime_followups.inc:43`; production and the new component build are
warning-free. All329 production input hashes match the build snapshot, and seven
protected INI/package files are unchanged. Deployment did not write saves.
The feature/source audit maps143 native units and37 settings; original-body
content scanning has zero current/local-history pattern findings. The older
known retained GitHub object remains the separately documented cleanup blocker. The release-flavor build uses
`src\proxy\build_dinput8.bat --release --no-deploy`; it excludes the development
console. Guarded deployment uses `src\proxy\deploy.bat`, never a running session.
See [prioritized accessible gameplay checks](../verification.md#optional-minigame-completion-06).

## Safe extension and review

Add a rule only after recovering the exact start/action, declaration, accepted
choice, actual native finish/cleanup and reward route for that resource variant.
Keep hashes, live operand/entry checks, bounds, one-shot/rearm and owner dispatch
synchronized with independent original-data preparation tests. A new result
mechanism must prove the whole native path, not just a displayed score/count.
Update the setting/menu/default/persistence and cancellation fixtures together,
then the source map, settings catalog, register and player test procedure.

Luna (`gpt-6-luna`, medium, concise fresh handoffs) completed three bounded tasks:
known-pattern setting/menu/default persistence additions; menu fixture coverage;
and supplied-fact controls/index/README/changelog edits. Sol reviewed/integrated
all results, added revision/lifecycle/native checks, corrected the menu fixture
so indexed-count assertions enable Menu indexing, and completed component, regression, build and deployment validation.
No delegated issue remains beyond the documented gameplay/localization limits.
Sol owns discovery, design, native implementation, verification and deployment;
no Astra was used.

## Historical release checkpoint

The earlier paused0.5.9 preparation omitted existing.py/.h inputs from one
source-freeze filter; it stopped before that build and was a checkpoint-tool
omission. The subsequent user-requested0.5.9 release was independently built,
deployed and packaged; its receipts remain authoritative for those old bytes.
This resumed0.6 work supersedes the earlier no-implementation/deferred notes.
The old0.5.9 package is preserved and contains none of these four assists.
