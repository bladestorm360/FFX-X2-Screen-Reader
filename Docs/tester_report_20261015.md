# October 15 tester report and follow-ups

Later gameplay follow-ups supersede this delivery's Help, collision/filter,
Quit, Kilika and Zanarkand conclusions: [current repairs, verified deployment
and priority checks](help_navigation_quit_20261015.md). Earlier evidence below
is retained; controller/F6/Macalania/Airship behavior is unchanged by that pass.

The report in the parent workspace, `ffx bug report.txt`, and the user's later
clarifications are the evidence for this work. No Tester Logs were accessed.
These reports concern another machine. The later controller observation and
`own-controller-before.log` are the user's own session, explicitly authorized
for this issue. No game was launched, controlled or probed. Installer rebuilding
and packaging remain held. No Git staging, commit or publication occurred.

Implementation, component checks, the final canonical build and guarded
deployment are complete. The delivery receipt below records the actual outcome. Gameplay confirmation remains
user-side, distinct from every offline result here.

## Controller pass-through

The original owner queued a complete state for every changed analog report.
The native pump replayed only one historical state per frame. A fast-reporting
stick could therefore accumulate the reported delay. The device is DualSense;
there is no evidence of a wireless cause. The user's accepted added-delay limit
is 10ms, with device processing off the native game thread where possible.

`physical_gamepad.cpp` now publishes current analog state continuously and queues
only discrete held-control transitions. `controller.cpp` preserves quick button
presses but takes all analog values from the newest copied frame. The exclusive
SDL worker uses a private 2ms high-resolution waitable timer at normal priority,
replacing below-normal priority and Sleep(4), whose Windows rounding could exceed
the input budget. Only the worker waits; no global timer-resolution change.
Copied physical and bridge source timestamps must both be at most 10ms old;
stale, saturated, disconnected, wrong-focus or retired data hands control back
to the native backend. The native callback has bounded queue work and no SDK call.

The new 48-axis-report regression fails against the original FIFO and passes
with the repair. The final seven-case fake-SDK ownership fixture passes normal,
held initialization, held audio open, incompatible version, foreign owner,
preloaded SDK and failed gamepad initialization. It also covers rapid taps,
overflow, stale publication, focus, resource retirement, disconnect and rumble.
Controlled 96 source-injection to native-read samples: p50 2.500ms, p95 2.578ms,
maximum 2.695ms; maximum native pump plus read 18.6us, direct fake-native read
0.1us. These measure copied-data age, owner/queue wait and usable native output
under controlled scheduling. The direct fake call is a boundary-cost reference,
not a before-build physical-controller latency baseline. Physical USB/Bluetooth,
OS scheduling under game load, frame delay and audible response are unmeasured.
A 10ms freshness cutoff prevents stale replay; it cannot guarantee OS scheduling.

## Help information pages

The menu producer and existing menu speech are preserved. The existing Help
renderer capture now re-arms when the accepted native page identity changes:
resource epoch, page table/index, selected entry and original resource bases.
This compares native event identity, never text content. The existing decoder
return also arms an accepted 76F580 update with its native update-result pointer;
this page-link route bypasses the 776220 input-result path. Only text actually
accepted by the original font renderer enters the buffer; update scripts are
not narrated. I, Autodetail and paragraph review use that context-owned buffer.

The retained 155-page catalog passes all four font ABIs: 620 page/font cases and
6,760 unconditional native text draws. A second fixture executes the original
2,740-byte x86 renderer, 116 relocations and seven-entry switch table, including
actual coordinate arithmetic, loop, font argument order and the CTB paragraph.
It replaces page 10 with page 9 without an initializer or input-result event and
requires the new information buffer. The original 1,422-byte decoder executes
an actual F70B link from the retained SPS2 and returns the native changed result.
That link targets its original page; it is not asserted to be the CTB link.
Graphics, conditional execution, text expansion and runtime numeric conversion
are simulated boundaries. This proves the repaired capture/lifecycle path,
not actual on-screen/audible gameplay behavior. Removing the identity guard makes
the new transition assertion fail (old Trigger Commands buffer instead of CTB).
Earlier renderer-only tests passed the old lifecycle and were insufficient evidence of page transitions.

## Kilika solver

The tester's stopping stage is ambiguous. The user's best interpretation is
that it announces "Destruction Sphere placed" and then cannot continue.
The old recipe started at worker 35 PC53283, after the accepted insertion setup.
It omitted native worker5 requests 8,19,6,9 for player attachment, positioning,
associated graphics and camera/control setup, then entered teardown requests
21/22. A previous tail-only CFG proof did not establish the full interaction.

The recipe now enters the complete original worker35 entry2 at PC51412. It
seeds only the existing utility's recess/carry inputs (old recesses empty,
carried sphere6), preserving native control ownership, dialogue and choice,
setup requests, timers, placement, reveal and return. Original code commits the
recess and clears the carried sphere. The solver says "Placing the Destruction
Sphere and preparing its treasure. Confirm any game dialogue." The chest remains
the original worker7 entry2 at12358; item award and temple treasure bit are native.
No direct item grant or skipping of the initial native ownership sequence.

`prove_kilika_insertion_20261015.py` executes the original bytecode expressions
and branches for accepted and cancelled choices, proves setup before teardown,
placement before wall54/reveal, native control return and no item grant. It also
proves the old continuation omits setup. Native request acknowledgement, timers,
graphics and Confirm are controlled boundaries. Both development and release
all-six-trial utility fixtures pass. The exact historical freeze PC is unknown;
this repairs the demonstrable entry-contract violation and still needs gameplay.

## Macalania labels and guide

Upper recess is worker37 at (0,40,110); Ramp recess is worker38 at (-88,40,62).
Original wrappers37/38 call helper30 entries2/3, then worker32 entries3/5,
respectively wall53/52 and distinct map layers. This matches Dylan's report that
swapping the two targets allowed manual completion. Current generator labels,
native rows and all semantic guide steps now agree. Regeneration explicitly
prefers these verified labels over the older timestamped navigation audit.
The seven mapping assertions pass. Corrected-build gameplay is pending.

## Airship before Evrae

The deck exit is above the lower Rin-side floor. The native Confirm elevator
was an Event, so the Exit category led past the necessary transition. Original
hiku0000 workers29/30 and hiku2000 workers17/18 are now Exit-category passages,
labelled Elevator up/down with "Confirm to ride" and the original interaction.
No automatic elevator activation or cross-floor walking path is invented.

A shared staging pass uses qualified original script/part/worker identities and
the copied collision mesh. If the enabled lower elevator has a real route while
the deck exit has none, the inaccessible deck row is removed and the actual lift
reads "Elevator up toward the deck. Confirm to ride, then select the deck exit
again". This happens before ordering, filtering, beacon and Auto walk. On the
connected upper floor the real deck exit returns. Epoch/count/part/worker bounds
reject stale facts. All six retained original airship EBP passage catalogs and
real resource-service qualification pass; controlled separated floors prove the
lower lift/upper deck choices and disabled-lift rejection. Live collision,
progression state and elevator riding remain unconfirmed.

## Sphere Grid F6 and direction audit

The original A56BD0 input decoder accepts both axes and has no down-left inversion.
The original 474-byte A56860 selector plus 37-byte dot helper were replayed
against the production scoring for every present node and all eight inputs:
860 Standard and 805 present Expert nodes, 13,320 comparisons, zero mismatches.
The original selector scores dot/distance-squared and retains the first strict
tie. With unrestricted Browse eligibility, 349 directed linked neighbours have no
direct eight-key selection, including
19 in the Standard Wakka section. Crowded native selection can explain this type
of report; the tester's exact node remains unknown. No direction mapping changed.
Instructions for such a neighbour now describe its position and F6 recovery,
rather than falsely claiming an arrow combination selects it.

F6 is reserved for the open grid. With a stationary cursor in Browse, Move or Use,
it opens a standard labelled Windows edit field on a dedicated parked UI thread.
Type the spoken one-based node number and press Enter; Escape cancels. Outside
the grid F6 retains its prior context action. A queued grid F6 cannot fall through
to the field beacon after closure. Menus/animations reserve F6 but ask the user
to close the menu/wait and retry. Invalid/absent/out-of-range nodes are rejected.

The UI owns its controls and copied numeric tokens only. It never dereferences
grid/game pointers, calls getters or creates a window on the native callback.
At PInput return a bounded command is revalidated, then the existing safe native
action boundary invokes original cursor-only A48D20 (and A48D70). Browse can inspect
present nodes; Move/Use retain original CAN_TARGET flags, confirmations, costs,
locks and character position. The original functions themselves are replayed by
the fixture. Epoch, mode, character, count, focus and native menu changes cancel
stale requests. Enter is released before returning to the game; held keys drain.

The registered dialog is excluded from production game focus, including child
controls, despite being in the same process. Native DirectInput keyboard/pad and
XInput fallback are neutral while typing, independently of SDL ownership. A
production settings.cpp focus fixture passes foreign/game/dialog/hidden/cleared
cases and 1,000 allocation-free queries. Fake-Win32 UI tests pass lifecycle,
labels, focus, parsing, cancellation, queue bounds and creation retry. HWND/window
manager/message execution are simulated; real NVDA edit-field interaction and
in-game cursor behavior require user confirmation.

## Delegation and review

- gpt-6-luna, medium, fresh bounded `macalania_mapping`: six allowed mapping,
  guide/generator/test files; returned the correction and seven passing tests.
  Sol reviewed every worker/coordinate change and reran the assertions.
- gpt-6-luna, medium, fresh bounded `grid_ui_fixture`: only the two fake-Win32
  fixture/batch files, later two separate production-focus fixture/batch files.
  Sol owned UI design/implementation, reviewed the fixture in full, corrected a
  hotkey stub signature, added registration coverage and reran both successfully.
  These tests simulate Windows; actual NVDA/game focus remains pending.

Sol retained all discovery, original-function/bytecode analysis, production
implementation, integration and final verification. No Astra invocation or gate.

## Evidence and delivery

Original models, hashes, replay provenance and Kilika proof:
`Scripts/Offline/output/report_20261015/`. Archive-only graphics/security/math
helpers occupy private test slots; only production functions use qualified
profile addresses. Private helper mapping fixes in the installed-profile replay
do not add production signatures. Initial fixture omissions (renderer switch
table/FPU ABI and large stack temporary) and intermediate failures remain in
`install/*20261015*.log`. Commercial originals stay local, outside the release.

The final canonical checks and deployment receipt follow. The old
0.5.6 installer/ZIP remains unchanged and does not contain these new repairs.

## Short gameplay checks, in priority order

1. Controller: ordinary menu left-stick movements and walking should respond
   immediately without delayed replay; check a quick button tap and one mod command.
2. Help: open an information page such as CTB. I reads its full text; comma/period
   review paragraphs; F7 Autodetail follows another page. Help menu names still speak.
3. Grid: at a stationary cursor, F6, a valid spoken node number, Enter. Cursor
   changes; F6 then Escape leaves it unchanged. Move/Use still reject unavailable
   nodes and preserve normal confirmation/cost. Outside the grid F6 keeps its action.
4. Kilika: activate the solver from the reported state. Confirm native dialogue
   if requested; placement/reveal should continue to the native chest interaction.
   Report the last spoken line if it stops.
5. Macalania: manual Upper and Ramp targets match their respective recesses;
   the former step8 no longer removes the ramp unexpectedly.
6. Airship by Rin before Evrae: select Exits; follow Elevator up, Confirm to ride,
   then select the real deck exit again. Route should stay on the current floor.


## Verified delivery — October 15, 15:02 UTC

Clean canonical command: `src/proxy/build_dinput8.bat --test-build --no-deploy`.
Final build exit0, no compiler warnings/errors; native proxy and original launcher
both rebuilt. An earlier harmless shadowed-parameter warning was corrected and
the canonical build rerun. No installer build was invoked.

- `install/report-build-final2-20261015.log`: final x86 build.
- `install/report-readers-final2-20261015.log`: both152-hook profiles, Help
  catalog/native replay, both-grid original selector/cursor and Airship staging.
- `install/report-controller-final-20261015.log`: real x86 SDL virtual gamepad
  and native XInput import bridge (physical drivers disabled).
- `install/sdl-owner-final-20261015.log`: seven fake-SDK owner/failure cases,
  analog regression and controlled96 timing samples; current source and modal
  XInput fallback suppression included.
- `install/report-input-final-20261015.log`: native keyboard/context/F6 capture,
  edit-dialog state/buffer suppression, held-release draining, native pad fallback
  and actual x86 DirectInput adapters.
- `install/report-sphere-final-20261015.log`: routes/locks/costs/materials/contexts.
- `install/report-profile-final-20261015.log`: extension/signature/data guards.
- `install/report-utilities-20261015.log`: six-trial development/release checks.
- `install/grid-ui-reviewed-final-20261015.log` and
  `install/input-dialog-focus-reviewed-20261015.log`: reviewed UI/focus fixtures.
- `install/help-identity-before-20261015.log`: expected old-guard failure;
  `install/kilika-proof-20261015.log`: full original insertion bytecode proof.
- Python `src/navigation/tests/test_review_labels.py`: seven assertions pass.

Game and launcher were closed. `src/proxy/deploy.bat` passed Preflight and
Installed phases against actual FFX and Will profiles, compiled signatures and
feature dependencies without running executable entry points, hooks or speech.
All seven installed artifact hashes match; PRISM0.18.3 receipt and15 locked files
match. User INI is byte-identical across deployment, SHA256
`03482f3fc62ca20607570a1cdf1ba1c89fb5dd5576580f72346f632999cf5185`.
Current proxy SHA256
`7dc39e037d9e471fc1b3601d4d13c9d8f77f90b34781444aaa7b94b0bde0cd24`.
[Deployment log](../../Analysis/FFX/RepositoryEvidence/historical-install/report-deploy-20261015.log),
[seven-file/settings/PRISM/package receipt](../../Analysis/FFX/RepositoryEvidence/historical-install/report-deployment-verification-20261015.json).

Held EXE SHA256 `8388eb5599c0e7dd11ab96f9c2abc6c557f46731c9d1daf1765e999297f6a7de`
and ZIP SHA256 `c343f931f391e79ce737704ef7359631e80c1f0b38ba680480765242ffa21343`
remain unchanged. They exclude these new fixes. No deployment blocker remains;
gameplay acceptance and an explicitly authorized later package remain separate.
Monitor remains PAUSED; fresh core usage8% used/92% remaining, secondary
unavailable, retain2% remaining stop. The short checks above are the handoff.

## Development console restored for six-trial testing

The user's next request was for warp commands to test every solver. Review found
that the initial delivery used `--test-build`, which intentionally excludes the
development console. Sol corrected that build-mode mistake without reverting any
source: canonical `src/proxy/build_dinput8.bat --no-deploy` enables the established
`FFX_DEV_UTILITIES` path. The clean x86/native/launcher build and guarded deployment
pass. No game was run, controlled or closed by the agent.

The existing console fixture initially failed to link because it omitted the
production utility-feedback translation unit. It now includes that actual unit;
`install/console-enabled-fixture-final-20261015.log` passes UI/feedback/history,
focus/stale/retry guards and native new/delete0/0 with fake Win32 boundaries.
The earlier linker failure is retained at `install/console-enabled-fixture-20261015.log`.
No production code change was needed for this follow-up, only the build-mode
correction, fixture linkage and documentation.

Current build/deploy logs: `install/console-enabled-build-20261015.log` and
`install/console-enabled-deploy-20261015.log`.
[Current seven-file/PRISM/settings/package receipt](../../Analysis/FFX/RepositoryEvidence/historical-install/console-enabled-deployment-verification-20261015.json).
Current proxy SHA256 `5bf9f5717d0c838ca4e8772a2df033426e187f1e0583e4f6ae40284d6828fcab`.
Both actual executable profiles pass Preflight/Installed checks, all seven hashes
and PRISM15 files/receipt match, user INI is unchanged, installer/ZIP remain
unchanged and rebuilding remains held. Earlier15:02 delivery hashes above are
historical. Gameplay confirmation is still user-side.

Open Shift+F1 in the field, type one command and press Enter:

- `warp besaid` — room122, entrance0.
- `warp kilika` — room108, entrance0.
- `warp djose` — room214, entrance0.
- `warp macalania` — room239, entrance0.
- `warp bevelle` — room306, entrance0.
- `warp zanarkand` — room320, entrance0.

These aliases use trial chambers and preserve story progression. Close native
game dialogue before issuing a warp. The aliases/catalog were read directly from
current source; runtime warp execution remains the user's action.
