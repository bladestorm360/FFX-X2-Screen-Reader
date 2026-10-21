# October15 gameplay follow-ups: Help, reachability, Quit and trial phases

Active user reports supersede previous offline-success conclusions. No game
launch/control/probe authorization. Own current mod log is permitted and was
explicitly requested for reachability. No Tester Logs access. Keep installer held;
keep development console enabled in final canonical build/deployment.

**Current checkpoint:** all five source repairs are implemented, offline verified
and guarded-deployed. Seven installed payload hashes, PRISM receipt/15 locked
files and unchanged user settings pass. Gameplay confirmation remains user-side;
installer/ZIP unchanged and held. The investigation hypotheses below are
historical; resolved causes, current contracts and delivery evidence follow.
User explicitly authorized Besaid installed map/collision asset
reads offline in this follow-up. That permission does not authorize game launch,
live probes, unrelated assets or Tester Logs.

## Work queue

1. Help information pages still unreadable. User screenshot Battle Mode: actual
   page8, heading62, paragraphs63-66 plus Exit58 in retained original SPS2. Text is
   native, not baked; no OCR needed for these descriptions. Existing English
   Basic Controls diagram is separate. Working Help menu speech must be preserved.
2. Unreachable filter hides nothing; route to Besaid Door sphere recess is offered
   before Entry glyph activation. Own latest log shows route instructions ending
   in Movement blocked. Need actual closed collision/remap/path/filter integration,
   not a label-only change or hiding geometrically unknown data indiscriminately.
3. Quit confirmation currently concatenates prompt/Yes/No. User wants prompt body
   once and actual highlighted Yes or No separately on focus changes.
4. Kilika automatic solver must replace normal take/place sphere interactions
   which open a native choice UI (entry sphere and entry door recess), then run
   native destruction insertion and trial completion. Preserve destruction-only
   stay versus ordinary-interaction completion rules and opt-out/manual behavior.

## Investigation checkpoints (historical)

- `Scripts/Offline/output/help_navigation_quit_20261015/user-battle-mode.png` is
  the user's provided screenshot; `user-own-session.log` is their own latest log,
  not Dylan/tester evidence. Own log gen12 scene11: Battle Mode headline10:44:51,
  many later input interruptions without manual/F7/review output; CTB10:47:02
  headline only. It supports missing mod-command dispatch, not missing text assets.
- Original Help update76F580 (940 bytes, retained layer2_game_logic lines269787+)
  reads its own native menu input then unconditionally calls776220. Existing
  command drains were only PInput625930/field871D10; they do not establish a Help
  dispatcher boundary. Added existing qualified776220 return listener in input.cpp
  using the same bounded drain, with no field utility/movement pump.
- New input fixture's no-PInput Help-I assertion FAILS baseline, PASS after fix;
  later PInput/field returns cannot duplicate it. Logs
  `install/help-input-owner-before-20261015.log` and `...after...`. Full original
  Battle Mode body + real worker/input integration still to verify.
- Prior Help tests fabricated input-return callbacks and decoder parent36F580
  even though36F580 is not hooked; their passing status did not establish actual
  menu command delivery. Current source still has the harmless qualified decoder
  guard; page identity capture is independent. Do not claim gameplay fixed yet.
- Native Quit68FCA0 sends original localized307/308/309 to showPopUpWithBtn.
  Current esc_capture.cpp Quit concatenates all three. Retained Main.as establishes
  initial No focus and changeSelectedBtn toggles; mouse handlers can also focus
  actual buttons. Need native accepted focus evidence, not blind toggling or
  guessing keyboard inputs. Native popup engine calls and Iggy ABI investigation
  pending. Existing nativeA28B20 value focus plumbing may be reusable.
- Reachability current copies original floor triangles/vertices and collision
  remaps; nav/owned decoder reconstructs blockedForGroup. Need inspect real Besaid
  PSG and original glyph wall changes. Current original-data trial models/maps
  already retained under prior archives; no new game-install asset reads needed.

All discovery/implementation/integration currently Sol-owned; no delegation or
Astra invocation in this follow-up. Preserve prior dirty work/source/history.

5. Zanarkand needs two native-phase solvers and manual guides: first visit solves
   normal trial and stops at its legitimate exit before elevator/boss progression;
   return visit handles Destruction Sphere only when natively unlocked. User relays
   Dylan11:04AM: destruction not available first visit; boss by elevator after
   normal trial, then another larger boss. Preserve both fights, do not skip story
   flags or jump ahead. Must inspect original dome0700+completion scripts and
   native revisit prerequisites, not assume every solve collects destruction.

## Additional findings/checkpoints

Help input-only regression passes before/after as recorded. Luna gpt-6-luna,
medium, fresh bounded help_diagram_completion added exact verified image labels
for pages6/7 using the existing page5 pattern, and original-renderer page8 checks.
Sol read actual image pixels (help15/Menu, help16/Battle), verified module48 IDs
29/30 across all four original page variants, reviewed all three returned files.
First fixture run failed: injected invalid/not-ready images never retried a valid
image before asserting successful full-page reading. Sol took over the fixture,
split rejected and subsequent ready renders and added wrong-image/variable guards.
`install/help-pages-followup-reviewed2-20261015.log` now passes original620 cases /
6770 unconditional text draws, three English control diagrams, original renderer
CTB/Battle Mode paragraphs through four ABIs, original grid audit and reference
reader profile. Canonical combined/Installed checks and deployment still pending.
No new native address for input: listener added to already-qualified376220.

Reachability review: caller captures copied native triangle/vertex arrays and
remaps49..64, then worker route/filter. No blanket unknown=unreachable change.
Own latest saved INI currently hide_unreachable=0 and the retained log contains
no filter-toggle output; that does not dismiss the reported route failure or
establish what was active during the test. Besaid original initial worker30 sets
walls48..54, and entry glyph worker38 changes wall48. Native83e4e0 raw alias logic
and setter83e9e0 verified in retained decompile; actual missing barrier cause
still open. No PSG files retained in current offline outputs; EBP/flow models
are retained. Never dump maps_v3.json as text (217MB single JSON line); parse and
print selected facts only.

Quit: actual Iggy A28BA0 wrapper loops Sleep5 while creating a fast name. Do not
call it from a new observation callback. Original Main.as initial No focus and
changeSelectedBtn/mouse focus paths recovered. Need a bounded actual focus read
through qualified Iggy value-path getters or equivalent native accepted focus
producer; no blind keyboard toggle inference.

Kilika original workers28/29 DO have nonempty entry2 choice/pickup/placement
paths and ARE included in generated contracts. Need identify why native requests
are not intercepted; do not just add invented menu branch. Source intercept's
signal source check compares +10 to controller+A current worker, a candidate to
verify against original request/initialization semantics. Actual worker28/29
models saved in output/help_navigation_quit_20261015/kilika-interactions.json.

## Resolved causes and implementation

### Reachability and No path

The decompiler mislabeled every83E4E0 switch case by one. The actual123-byte
x86 body indexes a63-byte selector table at83E574 and six pointers at83E55C.
Its real groups are:1 always blocked;2 blocks a non-controlled actor;13 uses
actor+198 bit2;14 blocks the controlled actor;48..63 resolve mutable aliases.
The former source incorrectly used2/3/14/15/49..64. Initial masking is only
raw&127; alias values remain full32-bit. The existing16-hop/cycle bound remains
for malformed aliases that would hang original code.

Corrected collision_rules.hpp, both native snapshot/readMesh remap starts and
the gate counterfactual range. Complete function and both original dispatch
tables now qualify during executable preflight for each supported profile;
wrong gate48 selection refuses startup. No new walking/movement producer.

User-authorized extraction reads only the Besaid bsyt01 mapout.vpa payload,
with its archive catalog for location. Retained manifest/hash at
`Scripts/Offline/output/help_navigation_quit_20261015/besaid/manifest.json`.
SHA256a6d453d0dbbbafa8dbf1f6235f3c3c3ce89aae408bd63e2f1a848e3d12b8845f;
979 triangles,657 vertices. Exact entrance0 and worker40 original recess
coordinates are taken from retained bsyt0100. Original initial walls48..54=1
block the passage; Door glyph worker38 PC33914 sets48=0. Real floor + production
decoder/route/filter has No path/hidden before, route/visible after.

A second bug cleared all unreachable flags on every command's fresh entity
vector, and canceled/restarted even identical jobs. Owned reachability now
retains the committed prefix and exact-context/target results, restores them
before filtering/selection and consumes fresh worker prefixes. Changed actor,
pose, floor, collision, geometry, target or generation still invalidates them.
No waiting, native pointer dereference on workers or periodic capture added.

`install/collision-native-besaid-20261015.log`: original x86 + both tables12,243
comparisons (raw IDs, controlled/noncontrolled, flags, full alias values, finite
16-link chains and every real Besaid triangle); real route and native/owned worker
filter pass, including eight fresh command vectors retaining the hidden result.
Malformed cycles are mod-only bounded checks, never run in original code.
Existing synthetic512-target/30,000-triangle cold check: capture2.035ms,
first commit29.746ms, complete2545.396ms; warm0.616/30.636/30.636ms, zero native
new/delete. Those are controlled fixture costs, not gameplay/audible latency.

### Quit popup

Actual Main.as showPopUpWithBtn defaults to No. Both changeSelectedBtn and
mouse drag-over branches emit playEscSound("change") exactly before switching
from the opposite button. Observe that accepted event in native68FB60 with its
registered menu owner, epoch, live global menu and visibility checks. Native
A28EF0 visibility close plus accepted click/cancel retire popup state/queued
captures. This avoids raw-key inference and calls no Iggy function/wait.
New hooks independently qualify complete279/13-byte bodies; +70 visibility is
unchanged in both pinned layouts. No new private callback-field assumptions.

Esc capture emits prompt and initial No separately; the initial focus queues
without interrupting the prompt. Later accepted focus changes emit only the
localized Yes/No. I retains only the prompt. Reader fixtures verify option focus,
repeat/reopen, unrelated movie/sound rejection and popup closure; the worker
fixture verifies separate packet-owned outputs and existing native no-allocation
transport. Gameplay confirmation remains user-side.

### Kilika

The retained own log10:52:35 proves entry-door interaction substitution and
reveal scheduling already occurred. The full original51412 insertion then
opened its own Insert/Cancel menu (two Cancels appear in the log), rather than
progressing automatically. This supersedes the earlier source-filter hypothesis;
workers28/29 and their native source checks were already present.

Keep the full original entry: control ownership, operation flag, positioning,
accepted insertion, attachment/GFX68 and cleanup must run. Its verified empty
recess/carried6 branch asks Common013B at52593 and accepts0 into52613. The
generated reveal action owns one automatic selection0, scoped to its exact
worker/chunk/signal/generation and window1, after native initialization. Observe
the original859A80 exec return, write its native signed16 frame+4 and complete
with1. Original85A590 ret-int closes the menu, restores the operand stack and
returns0 to ATEL. The flag is consumed once; other workers/windows/actions stay
manual. The owned allocation serial also must match: replacing the chunk at the
same address cannot inherit consent. No script/code rewriting or synthesized
keypress.

Utility fixture covers both original entry sphere28 and recess29 activations,
full51412 reveal, other-worker/window rejection, one accepted menu only, native
collection and completion, opt-out/staleness. It also replays the original167-byte
choice exec and94-byte ret-int against controlled SDK/stack endpoints to verify
waiting -> production completion -> original Insert return/menu cleanup. Real
animations, game script scheduler and hardware/backend remain gameplay checks.

### Zanarkand first visit versus return

Original dome0700 worker37 initializes normal floor state before2775, opens
the native encounter route at exactly2775, retains story setup2780..2870 and
selects alternate stage5 only at2875+. Use these native boundaries, not a
fabricated treasure prerequisite or a story write to unlock it. Unknown middle
2776..2874 phases refuse assistance until native progression finishes.

First-visit solver skips all Destruction Sphere preparation/collection and
requests original worker39 entry47 (28615), with its existing six-insertion
preparation. That script opens the normal gate and records2775 itself. Stop
there; no automatic exit/lift/warp/encounter request. Player takes worker42's
native exit to dome0200 room317. Worker33 contains the subsequent encounter
sequence and2785 write; worker41 goes directly to room318. Neither is invoked
by first-visit assistance. Developer trial finish now uses only the normal gate
and no longer writes2785/warps Zanarkand ahead; return finish stays in the trial.

Return solver, available only at native2875+, performs the original destruction
reveal/chest, stays in the trial and preserves story. Manual guide bounds are
separate:11 first-visit steps ending at the actual worker42 exit;4 return steps
ending at the chest. It cannot advance/back into the other phase, refreshes on
same-map story changes, and provides native-sequence guidance in the middle.
Corrected navigation labels distinguish trial encounter33, lift41 and exit42.
The original floor tile/direction and encounter scripts are unchanged.

New Zanarkand fixtures cover first2768/2774, gate2775, intervening story phases
and return2875/3000/3120/3121, no early treasure/story/warp/lift effects, separate
manual bounds and same-map phase changes. Offline checks and build/deployment
passed as recorded below; gameplay remains pending.

## Verified delivery

Canonical development build `src/proxy/build_dinput8.bat --no-deploy` finished
with exit0 and no compiler warnings/errors. Both native proxy and original
launcher were rebuilt. Shift+F1 development console remains enabled. Source
did not change during the final build; the post-build474-file hash snapshot is
`install/help-navigation-trials-source-20261015.json`.

Final checks pass: both154-hook reader profiles,620 original Help page cases/
6770 text draws plus the original renderer and three diagrams; original collision
function/tables and real Besaid floor; native profile/dependency qualification;
keyboard/COM input; packet-owned Quit worker; and both development/release trial
fixtures including original choice exec/return, allocation replacement rejection
and separate Zanarkand solver/manual phases. Exact six logs are linked from the
[deployment receipt](../../Analysis/FFX/RepositoryEvidence/historical-install/help-navigation-trials-deployment-verification-20261015.json).
The prior13,320 original grid-selector comparisons still pass with zero direction
mismatches; no direction remapping was introduced.

Guarded `src/proxy/deploy.bat` finished with exit0. Its preflight and Installed
phases verify the compiled current profile against installed FFX and Will without
running their entry points, hooks or speech. All seven installed SHA256 values,
PRISM receipt/15 locked files, launcher bootstrap and unchanged predeployment
INI pass. Game and launcher were closed before deployment and at verification.
Proxy SHA256 `488999c3d62cece107ef710db5e4d18b82632f7f8cc308eab0cb765c3d963039`.
Build/deployment logs and source snapshot are retained beside the receipt.

Verification environment corrections: the standalone native preflight cannot
map its DLL in the sandbox (Windows Illegal System DLL Relocation); the same
read-only check and final guarded deployment passed with authorized elevation.
The receipt script's shared PRISM helper requires Windows PowerShell rather
than PowerShell7's incompatible System.Web; rerunning with powershell.exe passed.
Neither issue required a production source change. No game launch/control/probe,
Tester Logs access, registry repair or package rebuild occurred. Existing held
EXE/ZIP hashes remain exactly the October14 values in the receipt.

Delegation: gpt-6-luna medium completed the bounded Help diagram/renderer fixture
task described above; Sol reviewed the exact pixels and production edits, took
over the initial faulty fixture and passed both final reader profiles. Remaining
uncertainty is actual in-game/backend operation. All other discovery, repairs,
integration and final verification in this follow-up were Sol-owned. No Astra.

## Priority gameplay checks after deployment

1. Tab > Help: enable Autodetail, then open an information page. Expect its body
   immediately without I. Down/Up review the individual paragraphs; returning
   to the heading must not repeat the whole page. I remains a whole-page read
   after arrow review. Check the Battle Mode, CTB and Basic Controls Help pages.
   Failure: title/menu only, arrows silent or changing the wrong screen, lost
   whole-page I, wrong/stale text. Latest repair/evidence:
   [Help arrows and Zanarkand displays](zanarkand_patterns_20261015.md).
2. Quit: open its confirmation. Expect prompt then No; move the choice and expect
   only Yes/No for the highlighted option. I reads the prompt. Cancel/reopen must
   reset to No. Do not confirm Yes unless intending to quit.
3. Besaid: before the entry passage opens, enable Hide unreachable. Door sphere
   recess1 should disappear; with the filter Off, its route should say No path.
   Activate the original glyphs, then expect the destination and route to return.
   Warps preserve existing story/puzzle progress; an already-open save cannot
   exercise the closed case. Real979-triangle closed/open proof is offline above.
4. Kilika: enable the solver and interact with entry sphere/recess. Confirm normal
   dialogue when asked; its owned Insert choice completes automatically, then
   the original reveal/chest/completion continues. A destruction-only interaction
   must keep the player in the trial. Failure: choice waits/cancels, old take/place
   action runs, or reveal never advances to the chest.
5. Zanarkand when progression permits: first visit announces the normal trial,
   opens its gate and stops in the chamber; manual guide ends at the actual exit.
   Take the native exit and play the encounter/story sequence normally. Later
   return announces the separate treasure objective, uses only four return guide
   steps and collects/stays. Failure: early treasure, room318/lift jump, skipped
   encounter or a guide crossing between visits. Both fights remain native;
   combat/story progression has not been exercised by an agent.

No baseline/new-mod work is being handed off early: these are follow-up
regressions and a narrowly scoped native-phase correction. Installer/ZIP stays
held while the user confirms gameplay. Preserve the user's saved settings.
