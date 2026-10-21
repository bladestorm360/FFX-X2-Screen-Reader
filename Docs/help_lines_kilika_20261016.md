# October16 Help rows, menu/control separation and Kilika continuation

User confirms Help pages now assemble as buffers. Remaining requests: preserve
each native text line as a buffer row; footer controls only via Shift+I (or
No controls displayed); menu prompt before initial focused item; character
selector must read its visible name/??? slot, and Reference menus must not use
Left/Right as a text buffer. Preserve Basic Terms' native navigation. Kilika
still runs ordinary entry sphere/recess actions after Destruction Sphere pickup.

## Active source checkpoint

Help source is implemented, offline verified and guarded-deployed:
LineBreaks TextPlan operation, worker-only bounded line splitting, separate
rendered controls role and read command, menu prompt/opening/focus roles, no
buffer navigation for native menus, and native font13 name substitution copied
through the font's own8B6610 getter. Literal??? slots become Unknown character;
no hidden native name is read for these placeholders. Shift+I leaves I intact.

Original retained SPS2 confirms control-bar entries56..61 at y356/357 (154
entries across155 pages), distinct from Basic Controls diagram contents. Original
76F960 keeps its entry in EDI and invokes776A50 then the actual font. The decoder
preserves13/30..37;8E4E60/8E75C0 call8E9B80/8E9CA0, which resolve via8B6610.
Worker-owned decoder had skipped13. The native character menu is page24, initial
entry19 (text119, Tidus), prompt91; other entries120..125 branch between a native
name and literal??? according to original conditions.

Retained audit/generator: Scripts/Offline/audit_help_lines_20261016.py; output
Scripts/Offline/output/help_lines_controls_20261016. It records source hashes,
entry layouts, original2740-byte renderer and1422-byte conditional decoder with
relocations/jump tables. No installed assets or live probes were read. Original
SDK story/input/conditions and final font/backend boundaries are explicit in
fixtures. First readers run failed because its old message-only substitution
stub rejected the newly supported native Help name; that fixture is corrected.

Luna gpt-6-luna medium edited only the established real-worker fixture, updating
Help newline expectations and adding held multiline/footer/fallback/close cases.
Sol reviewed and ran it: PASS, zero native heap allocations/frees, synthetic
65-row initial collection526.8us/request-to-first7.79ms. These are harness-only
values, not game/backend timing. Final reader/native/menu checks pass below.

## Kilika investigation checkpoint

### Latest user report: the deployed idle-dispatch repair still stalls

User retested the actual4f9e5a deployment. Own current log07:49 startup confirms
the465390 extension is loaded;07:50:06.145 activation and07:50:06.212 Reveal
occur, without an owned continuation. Repeated entry worker28/29 interactions
retire correctly. The previous repair was deployed, but its fixture missed the
room's disabled target state. Do not describe Kilika as gameplay-confirmed.

Original klyt1000 W16 entry1 at387E deactivates W35. W35 is the destruction
recess, whose entry1 waits atC8D1 in Common005F; its requested full insertion is
entry2 atC8D4/51412. Normal door solution paths at140D/2EEE reactivate35. Native
Common00A7/8576C0 clears only worker+34 bit2; Common00A8/8577F0 restores only
that bit. Original865390 rejects the whole queue when bit2 is absent, whether
or not+90 has an active signal. This original-code gate explains why merely
dispatching an idle queue was insufficient. Live flags were not recorded.

Only Kilika's verified treasure ScriptAction now opts into that activation
prerequisite at the deferred native action boundary. Reservation/cancellation
does not touch flags; a refused enqueue restores its added bit. The scheduler
uses its ordinary-1 priority rule with an active predecessor and normal10 only
when idle. It can preempt the lower-priority halt normally; an equal-priority
running script remains intact. No other trial/raw request activates a worker.
Existing exact owned initialization, native insertion choice/cleanup, repeated
interaction guard and chest/completion handoff remain authoritative. One bounded
request diagnostic records acceptance, prior/current flags and queue/active
identities. No waits, worker-side engine pointers or new polling are introduced.

Scripts/Offline/prepare_atel_activation_replay_20261016.py retains the original
deactivate36/activate35-byte bodies alongside all nine prior queue functions in
output/atel_queue_activation_20261016, preserving the prior evidence directory.
Its activation proof validates the real script statements/entries and hashes;
own_log_evidence.json retains the current own-session evidence separately.
Development/release utilities PASS in install/kilika-activation-final-20261016.log:
original disabled active-halt queue reproduces the stall; production matches
original activation flags, starts51412 once and reaches chest/completion. Native
animation/dialogue effects remain simulated. Equal-priority predecessor, idle
dispatch, repeated player actions, opt-out, refusal rollback and stale-context
cancellation pass. The first run exposed a synchronous SDK-mock assumption
about dispatch; the mock is corrected, and the actual original-code replay
remains the gate test. Preexisting choice fixture C4459 warning is unrelated;
canonical production build has zero warnings/errors and561 unchanged source
hashes. Profile, real native-action adapters and both154-hook reader profiles
pass. Guarded Preflight/Installed deployment verifies7 artifacts, PRISM15 files
and receipt, original launcher bootstrap, unchanged INI and held installer/ZIP.
Proxy1bbe9997a0959ebf42b581f5bb091e9541703dfa76628f9bbc4f828770ed9247.
[Latest deployment receipt](../../Analysis/FFX/RepositoryEvidence/historical-install/kilika-activation-deployment-verification-20261016.json),
[source snapshot](../../Analysis/FFX/RepositoryEvidence/historical-install/kilika-activation-source-20261016.json).
Reload the Kilika save, enable the solver and activate the entry sphere/recess;
full insertion should now progress to native treasure collection/completion.
Gameplay confirmation remains user-side. No game launch/control or Tester Logs
access occurred.

Sol owns this discovery, implementation, review and verification; no delegation
or Astra. Gagazet guidance plus optional default-Off automatic solving remains
authorized and queued behind this renewed Kilika blocker.

### Later user report: interception works, owned solver does not start

User's own current log gen12 at07:21:48 accepts activation and says Placing the
Destruction Sphere. It never records a utility continuation; repeated worker28
entry2 player requests retire in Reveal. This proves the filter is active and
the requested initialization is missing; the log does not capture exact native
priority/flags. Earlier fixtures incorrectly initialized every accepted request
immediately, bypassing the native queue. Do not call that runtime-verified solving.

Retained actual86E920 only queues through86E990. Actual865390 compares requested
priority against worker+32; an idle worker can retain its prior priority after
actual869170 RET, so an equal-level request is accepted but ordinary -1 dispatch
never initializes it. The native RET path uses865390(global,10) to select the
next queued request. Production now mirrors that path for owned utility actions
only when worker+90 has no active signal. Existing scripts are not displaced;
the original scheduler still owns gating/order/init. This is a demonstrated
native failure mode consistent with the log, not a recorded live priority value.

Scripts/Offline/prepare_atel_queue_replay_20261016.py retains nine original x86
functions (request68, insert408, dispatch205, init299, RET377, list27/28/45,
ack87 bytes), relocations, disassembly and hashes in output/atel_queue_20261016.
The insert's decompiler size403 omitted its final5-byte failure epilogue; actual
instruction extent checked before execution. Descriptor raw operand1325B60 plus
field offsets is preserved (decompiler names1325D58/5C are fields, not array base).
Production scheduler465390 is separately full-signature-qualified and required
by dependency validation. Future native_extensions generation retains it.

install/kilika-idle-dispatch-third-20261016.log: dev/release PASS. Actual native
RET leaves idle priority7; enqueue+normal dispatch reproduces no initialization;
the production solver starts full original entry51412 exactly once with real
queue signal reuse. Repeated entry interactions preserve it. A running predecessor
is not forced out; its actual RET starts the queued insertion. Native thread
stack/SDK descriptors and interpreter animation/UI are bounded simulated
dependencies, explicit in atel_queue_replay.inc. First fixture compile exposed
zero-length C arrays, replaced with std::array; second exposed a missing raw
descriptor relocation, corrected from original bytes. No source rollback.
Final compatibility/profile, real native-action adapters, both154-hook readers
and dev/release utilities PASS. Canonical development native/original-launcher
build has zero warnings/errors and561 unchanged source hashes. Guarded
Preflight/Installed verification passes;7 artifacts/PRISM15 files/receipt,
launcher bootstrap and unchanged INI/held EXE/ZIP verified. New installed proxy
4f9e5ad2192f7f5532a25296e13b9c53351b39851e9f4b9071f5a22100599b74.
[Current later receipt](../../Analysis/FFX/RepositoryEvidence/historical-install/kilika-idle-dispatch-deployment-verification-20261016.json).
User gameplay confirmation remains pending. No delegation or Astra in this fix.

User queued next work after Kilika: locate Gagazet cave's Wakka moving-target
throw and three swimmer/coloured-circle puzzles from retained map/script data;
verify labels, timing and character-colour mapping before implementation. No
save/map name supplied; no new install/runtime/Tester Logs permission inferred.
Found room272 mtgz0600 Mountain Cave and room310 mtgz0700 Submerged Passage.
Both puzzles are in310. User explicitly chose guidance plus a separate default-off
automatic solve option. Retained script/text models and exact hashes are in
Scripts/Offline/output/gagazet_cave_20261016; ongoing findings will be linked
from the separate Gagazet record.

Current own installed FFX_SR_Native.log (not Tester Logs) gen15:
06:31:27 automatic solver On;06:31:40.323 interaction intercepted and ordinary
trial objective announced;06:31:40.388 destruction insertion requested;
06:31:42.356/06:31:48.029 ordinary "A Kilika Sphere is set here";
06:31:43.391/06:31:48.796 "You can only hold one sphere at a time."
This confirms initial activation occurs, then a normal entry interaction runs
while insertion is pending. Previous source ignored ALL interactions outside
Idle, including during Reveal, so queued/repeated player interactions run their
normal scripts and can seize the choice UI. Need retain owned utility requests
while neutralizing only qualified Kilika player requests during the active
solver sequence. Production now neutralizes only verified player-origin Kilika
interactions throughout Reveal/Chest/Completion, retaining initial objective.
An exact worker/signal/epoch receipt from the preceding serialized utility
listener preserves each owned original request, including same-signal reuse.
Original entry table/allocation/physical RET guards remain; no new automation.
Both development/release utilities pass in
install/kilika-queued-interactions-third-20261016.log. First failed regression
was a fixture's wrong signal-level assumption (native request uses7). Second
exposed a real objective overwrite by a later held sphere; fixed by guarding
continuations before any new objective selection. No gameplay claim.

## Battle results and Customize follow-up

User requires earned rewards and sphere levels with Autodetail Off, retaining
detailed distribution/drops for review. FFX's original result path displays AP,
not a separate EXP counter. Production copies each earned AP/initial sphere
level and initialized gil at8BDB30 before8BA620 countdown; native-order earned
rows speak regardless of Autodetail. Counter/current-stat rows remain available.
After the original award/Confirm loop ends, each nonzero sphere-level gain
speaks once per results episode. Drops retain existing Autodetail behavior.
Final readers/worker verification passes below.

Customize already uses current table146A9F8, so the index hypothesis is not the
root cause. Original7ABD30 retains unowned inventory tombstones (old name and
abilities intact);8D6730/8D5720 skip ownership byte+2==0 and flags+3 bit2. Reader
now applies exactly this visibility predicate and invalidates stale detail,
including ownership changes at an unchanged row/handle. It still follows the
game's current table on rebuilt indices; flags4/8 remain visible. Retained
original135-byte getter/176-byte list/76-byte predicate audit and isolated replay:
Scripts/Offline/audit_customize_slots_20261016.py and output/customize_slots_20261016.
The list's decompiler size172 omits the trailing epilogue; actual bytes include176.
No game source-install read was needed. Native controller/backend remain mocked.

## Final offline verification

- install/help-rewards-customize-readers-final-20261016.log: canonical full
  readers PASS, both154-hook profiles. Original155 Help pages/604 cases/6770
  unconditional native draws plus original renderer/conditional decoder replay,
  CTB line rows, footer-only Shift+I, prompts, native menu arrows, locked/actual
  empty character slots and controls-only pages pass. Native Basic Terms links
  retain their unchanged original handling. Current rewards/On/Off/countdown/
  Confirm/capped level/retirement and original Customize code replay pass.
- install/help-rewards-worker-final-20261016.log: PASS, real delayed worker with
  production StatusBatch/TextPlan/selection-detail/speech-domain pipeline. Native
  heap allocations/frees zero; multiline controls, I-after-controls, cancellation,
  mandatory reward total and silent optional drops covered. Initial65-row
  collection800.6us; request-to-first10.60ms under this synthetic host. Actual
  game/controller/audible backend timing is not measured. This is a different
  harness path from the earlier2.695ms controller evidence; do not compare them.
- install/kilika-queued-interactions-third-20261016.log: development/release
  PASS, all-six original trial contracts and Zanarkand first/return/display guards.
  Kilika workers28/29 repeated player interactions during Reveal/Chest/Completion
  retire only their original request, retain owned insertion PC/request/choice,
  preserve completion versus destruction-only objective, and release on opt-out.

First full readers failure was the prior exact expected AutoDetail text missing
the new earned total. Focused runs then exposed two fixture errors: an empty
description owns a valid selection with a native-description-unavailable response,
and a pre/post callback change must be made inside the native event. Corrected
fixtures; source visibility rule remains exactly the original rule. The original
list byte-size correction was identified before executing it. Reader-only C4389
warning is the preexisting signed/unsigned comparison in airship_runtime_followups
line43; canonical production build status is recorded separately.

Sol owned discovery/implementation/review/integration/checks. Luna gpt-6-luna
medium's bounded status worker fixture changes were reviewed, extended by Sol for
mandatory totals, and rerun successfully. No remaining delegation issue or Astra.

## Priority user gameplay checks after deployment

1. Help CTB page: Down speaks one displayed line per press. I reads the body
   with line breaks; Shift+I reads Exit only. With no footer, Shift+I says
   No controls displayed. F7 On reads body without footer.
2. Reference/Overdrives: entry speaks Select a topic then focus; character
   selector speaks Select a character then Tidus/Unknown character. Left/Right
   does not read a buffer. Basic Terms arrows retain their native function.
3. With Autodetail Off, finish a battle: earned AP/gil speak; nonzero sphere-level
   gains speak once after counting/Confirm. Drops/stat details stay reviewable
   with arrows/I. No repeated gain during an idle completed screen.
4. Sell a weapon and armor, reopen Customize: navigate remaining gear. No sold
   name/abilities should be spoken. Visible disabled gear should remain readable.
5. Kilika solver On: activate entry sphere/recess and confirm any native owned
   insertion menu. Repeating entry interactions during reveal must not open the
   normal sphere pickup/placement menu or stop treasure/completion progression.

These are final native-integration/gameplay checks, not a request to review code.
No agent runtime action or broader test/data access was authorized or performed.

No Tester Logs access, game launch/control/probe, registry repair or packaging.
Current installed proxy:
edda314eb91c7d5d8ff9b3ad36fdeedd64468984015968c8501d95673e9fb852.
Canonical development native/original-launcher build has zero compiler warnings
or errors and559 unchanged pre/post source hashes. Guarded Preflight/Installed
checks pass; seven installed artifact hashes, PRISM15 locked files/receipt,
launcher bootstrap and unchanged INI verified. Game/launcher closed throughout;
no executable entry point, speech backend or game hooks were run in verification.
[Deployment receipt](../../Analysis/FFX/RepositoryEvidence/historical-install/help-rewards-customize-deployment-verification-20261016.json),
[source snapshot](../../Analysis/FFX/RepositoryEvidence/historical-install/help-rewards-customize-source-20261016.json).
Installer/ZIP hashes remain unchanged; packaging held and reset monitor PAUSED.
