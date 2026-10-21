# Jecht Shot save, cues and assistance — October 14, 2026

**Current status:** the user confirms final auto-success “works perfectly,”
fulfilling the tutorial and eleven-answer pacing test condition. Manual control
prompts and taunt suppression were already confirmed. The latest
[eleven-answer pacing](#eleven-answer-pacing--october-14) and
[tutorial repair](#tutorial-confirm-only-auto-success--october-14) remain intact.
After the subsequent opening/defaults fix, canonical build and actual guarded
deployment verification, the single0.5.6 installer/ZIP was rebuilt and verified.
All auto-success settings default Off for fresh/unset values; existing saved
On/Off survive loading, deployment and upgrades. [Final delivery evidence](title_prompt_and_settings_20261014.md#current-delivery-checkpoint).
Dated pending/held statements below describe preceding checkpoints.

The user authorized reading only `Saves/Dylan/Ject Shot Game` after deployment
blockers were repaired. Its `ffx_000` is 26,880 bytes, SHA256
`1e77fbd0c5cd6b1a2884d3641679c872909e6f80d2272541174936243a359594`.
The 64-byte file prefix plus the Fahrenheit SaveData size 0x68C0 accounts for
the entire file. At payload offsets 0/2/4/BA/C2/C4/BEC the values are current
room191, previous94, map14, saved room191, saved previous94, saved map14,
story385. This interpretation is corroborated by the original script: Winno
sets story385 before its flashback; `znkd0400` sets387 and returns to room94.
The save itself was not changed, loaded into a game or copied into an install.

## Original evidence and scope

Retained [Winno and exact flashback control flow](../../Analysis/FFX/RepositoryEvidence/offline-output/jecht_20261014_074758/manifest.json)
comes from the already-extracted, SHA256-checked original EBP files. Parsing
reuses the production-representative ATEL archive/compiler tools, attributed
to Karifean/FFXDataParser and Fahrenheit. Initial feature discovery needed no
game-install source reads, capture, new Ghidra run or engine pointer on a worker.
The subsequent authorized deployment compatibility check compared exact input
handler bytes in the installed executable; its entry point was never run.

The two original minigame implementations are `swin0000` (38686 code bytes,
57 workers, code checksum5486E5D2) and `swin0200` (91759 bytes,50 workers,
checksum273A7D2B). Their original real-input workers are32/entry8 and24/entry6;
practice is32/entry7 and23/entry89 (corrected from the earlier entry7 assumption
by the caller/entry audit below). Each has five left/center prompt positions
and four right/down positions. Original declarations, entries, exact110 input
CALL PCs and eighteen message-show PCs generate `jecht_data.hpp`.

The Common namespace table is at **RVA850050**, not VA850050. The retained
reference table at VA00C50050 selects integer return handlers at id*16+C:
Common4C=0085E0C0,4D=0085E6B0,50=0085EAA0,51=0085EF20. Full signatures and
the actual held/pressed word getter qualify each installed mapping independently.
Common4C/4D retain their native remapping calls;50/51 are the original raw
queries. [Original dispatch](../../Analysis/FFX/RepositoryEvidence/offline-output/native_bodies_20261014_003237/FUN_00877770.txt)
and [input bodies](../../Analysis/FFX/RepositoryEvidence/offline-output/native_bodies_20261014_003400/FUN_0085e0c0.txt)
were reused/extracted from retained workspace evidence. Adjacent identically
shaped handlers cannot qualify through a blanket address delta.

## Implemented behavior

- **F8 > Miscellaneous > Jecht Shot auto-success**, `jecht_assist`, default Off:
  answer only the original three practice loops and currently pending real
  prompts. Explanatory Confirm pages remain manual. No completion, ability,
  score, saved flag or global input word is written; the original script owns
  scoring, animation and learning the shot.
- **F8 > Navigation audio > Jecht Shot cues**, `jecht_cues`, default On:
  with assistance Off, beep and name every required configured direction plus
  Confirm. Controls must be pressed together; center needs Confirm alone.
  Each component is a single logical control in the copied text recipe, joined
  on the preparation worker with spaces, following the user's later brevity
  request recorded below. Missing bindings suppress an incomplete
  spoken combination. Equal distinct show events remain separate.
- Cues follow **CALL006A**, the actual show event, after its ordinary native
  message callbacks. CALL0064 only prepares text and is not the cue producer.
  Original [008589F0](../../Analysis/FFX/RepositoryEvidence/offline-output/native_bodies_20261014_005318/FUN_008589f0.txt)
  calls008B8980, which owns the ordinary message hook. This order keeps the
  direction as the latest output without text-content deduplication.
- The qualified `swin0200` worker34 trigger is now **Exit 1 to Jecht Shot
  challenge**. Its w21e11 -> w22e16 sequence starts the challenge and later
  warps to flashback room191; the generic room191 Zanarkand area is not a useful
  immediate destination label. Other scripts/hashes retain their own labels.

Native callbacks require the native owner, current resource serial/checksum,
actual primary field controller id0, exact worker/header/signal/entry/level/PC,
focus and closed mod/utility menus. Real input assistance additionally checks
original active flag1, current selector and timer below300. Cinematic state is
intentionally allowed for this exact minigame. Original query results remain
untouched when assistance is Off or qualification fails. Resource resets retire
the caches/TLS context; preparation pending never grants an input override.

## Offline verification and remaining acceptance

`install/reset-resume-20261013/jecht-native-8.log` executes all110 production
query adapters against the actual retained EBP/declarations and preparation
service. It evaluates the original bytecode for all nine real direction
predicates in each script, including negative direction tests and center.
The shared Lightning listeners are installed in the same host. Practice,
visible show-event ordering, equal events, manual physical input, pending/
changed/retired resource, expired window, focus/menu, foreign controller/owner,
ancestry/generation and Native heap0/0 are covered. Engine/input/audio/speech
endpoints remain controlled: this is not a complete engine/reward or SDK test.
Measured Native query/cue costs are in the log, not universal performance targets.

`jecht-profile-7.log` passes both executable-layout variants, qualified getter
ambiguity/refusal, required-dependency refusal and the separate Will profile.
The Will host now supplies its original instruction-derived audio global;
previous fixture setup had omitted that existing operand. `jecht-menu-2.log`
passes actual F8 access, independent defaults/persistence and grouped indexes.
`jecht-lightning-1.log` preserves both Thunder Plains scripts. Final reader
checks in `jecht-final-readers-2.log` pass both144-site profiles, the actual
retained Winno exit correction, and Stone Gaze/similar shared battle paths.

Actual audible cue order/usability, timing under game/real-backend load,
simultaneous manual controls, complete shot acquisition and gameplay handback
remain user-side acceptance. See [the short procedure](gameplay_acceptance_0.5.6.md).
Installer rebuilding remains held. Final deployment evidence belongs in the
threading audit and reset-resume verification record.

The first guarded deployment refused the new Common004D signature before
changing installed files. The actual installed namespace row points to
VA0085E820: its original98-byte body includes a room49/worker11/flag20 guard,
then the remapped pressed query. Reference004D is61 bytes. The resolver now
uses separate exact installed body size and getter CALL offset53 while keeping
reference offset17, with the pressed operand still anchored to471D10+52.
The complete guard is preserved; a changed guard is refused by the fixture.
`jecht-real-profile-diagnostic-2.log` applies the exact production resolver to
mapped installed sections and passes every signature/operand. The anchored
actual Common table confirms4C/4D/50/51 at0085E130/0085E820/0085EC60/0085F0E0.
Exact target evidence is retained in `jecht-real-input-getters.txt`,
`jecht-real-input-bodies.txt` and `jecht-installed-practice-body.txt` alongside
the deployment log. This adds no blanket offsets or runtime probing.

Luna gpt-6-luna medium updated only README and the existing `Docs/controls.md`
for the finalized F8 interface. Sol reviewed the wording against source and
validated the menu/defaults/persistence. The guide path was resolved from the
existing repository; no separate root Controls file was created. No other
delegation was used for this feature's discovery, design or implementation.


## Diagonal practice prompt correction — October 14

The user reported no response to Up+C and then clarified the spoken wording
was Up+C **or** Right+C. The attached clipboard image
`codex-clipboard-2d64dd19-503b-417d-b3f4-a04667a9fdc3.png` visibly shows
**Up and Right plus C, at the same time**. This is user-side gameplay evidence
against the prior offline acceptance; no image text was treated as an instruction
for the agent to control the game.

The practice page is original `swin0000` string72 / `swin0200` string97:
`09 30`, “Press”, directional token `0B 43`, plus, Confirm token `0B 31`,
newline, “at the same time!”. The verified original practice predicate tests
both Up and Right before accepting Confirm. The shared glyph decoders were
joining all multi-direction tokens with “or”, wrongly presenting this diagonal
as alternatives. The new shared constexpr separator uses “plus” for43/46/49/4C
(the four adjacent direction pairs) and preserves “or” for opposed directions,
triples and the complete direction pad. Single directions and configured control
names/remaps retain their original behavior. No input/gameplay automation changed.

Regressions cover legacy configured keyboard/controller glyphs, all four
chords, opposed/full-pad alternatives, owned fallback decoding, and the exact
original practice bytes in both scripts with Confirm configured to C, through
the production copied message decoder and ordinary message producer. The
original practice queries use held directions (Common4C) and a fresh Confirm
press (Common4D), so the player instruction is hold Up and Right, then press C
while holding both. Configured Confirm bindings remain authoritative.

Final optimized warnings-as-errors reader checks pass both144-hook profiles in
`install/reset-resume-20261013/jecht-diagonal-readers-3.log`. The first two
fixture builds failed on a wstring argument and implicit integer narrowing;
both fixture compilation errors were corrected before the successful run.
Canonical `--test-build --no-deploy` x86 native/launcher build passes without
warnings in `jecht-diagonal-native-1.log`; production source stayed unchanged
during compilation. Closed-process guarded deployment passes preflight and
installed FFX/Will checks in `jecht-diagonal-guarded-deploy-1.log`.

The fresh [verification record](../../Analysis/FFX/RepositoryEvidence/historical-install/reset-resume-20261013/final-deployment-verification.json)
confirms all7 installed SHA256 hashes, PRISM0.18.3 x86 receipt and15 locked
files, SDL3/license present and Tolk absent. Deployed proxy SHA256:
`790614875ab3b38045c6dc1d1472298ef8b6d956ef50d07394c61306c6642c30`.
Earlier deployment evidence is preserved as `jecht-before-diagonal-deployment-verification.json`.
Corrected audible wording and manual success still need user gameplay acceptance;
no game/backend was launched or controlled. Installer remains held and the reset
monitor paused. Sol handled this small correction and integrated verification
directly, without delegation. Latest core usage75% used/25% remaining; secondary
unavailable; the2% stop threshold is retained.


## Controls-only Jecht prompts — October 14

The user explicitly requested short reflex prompts such as "Up Right C", with
no "Press", "plus" or "at the same time". The earlier diagonal correction remains
in the shared glyph decoder for other game messages; this presentation request
only changes Jecht practice instructions and timed cue formatting.

The owned message decoder recognizes the three complete original practice
instruction templates (swin0000 strings72-74; swin0200 strings97-99), requiring
exactly the original direction and Confirm native glyph tokens plus complete
matching text. It rebuilds the prompt and review line from the configured single
control names, separated by spaces. It does not shorten explanatory pages,
selectors, matching literal prose without glyphs, or changed/other instructions.
This is a specific user-approved presentation policy, not text deduplication.
Native capture/getters/ownership and worker bounds are unchanged. The retained
English instruction templates are covered; other localizations retain original
text if they do not match these exact templates.

Timed Jecht cue recipes now use SpaceJoin, preserving configured names, missing
binding gates, direction masks, original show event, beep, lifecycle guards and
all native success predicates. No gameplay/input behavior or tutorial explanation
changed. The concise output still requires the controls together.

Both144-hook reader profiles pass all six original practice pages on keyboard
and controller, the ordinary producer/review output, preserved explanatory pages,
literal-prose refusal and unchanged general diagonal/alternative/swimming/remap
behavior: `install/reset-resume-20261013/jecht-concise-readers-2.log`.
The first run reached the negative prose guard but its ASCII fixture helper could
not encode '+'. The corrected negative case inserts original charset byte45;
the successful rerun compiles optimized with warnings treated as errors.

`jecht-concise-cues-1.log` passes all110 original queries, nine original predicates
per script, visible/equal cues and Native heap0/0 while asserting names-only
space-join recipes. Controlled maximum query118.9us/cue9.4us is not real-game/
backend/audible timing. The clean canonical x86 native/launcher tester build
passes in `jecht-concise-native-1.log`; source stayed frozen during compilation.
Closed-process guarded preflight/installed deployment passes FFX/Will in
`jecht-concise-guarded-deploy-1.log`. All7 installed hashes and PRISM0.18.3 x86
receipt/15 locked files are freshly verified, SDL3/license present and Tolk absent.
Current deployed proxy SHA256:
`d37fbb5992f0aee4baa39fd9f39811562a25c3dfc66d02bec2c0847b171c7cef`.
The preceding diagonal deployment verification is preserved separately as
`jecht-diagonal-deployment-verification.json`; the current
[verification record](../../Analysis/FFX/RepositoryEvidence/historical-install/reset-resume-20261013/final-deployment-verification.json)
covers the controls-only payload. Actual audible wording/manual success remain
user-side. No game launch/control, live probes or real backend test occurred.
Installer rebuild/package remains HELD; reset monitor remains PAUSED. Sol handled
this small presentation change and verification directly without delegation.
Latest core usage76% used/24% remaining, secondary unavailable; stop at2% retained.


## Tutorial demonstration cue gate — October 14

**Historical, superseded:** removed by the later user correction and repair
below. Its passing simulated phase checks did not establish real cue audibility.

The user reports ordinary tutorial instructions/glyphs and the mod's extra
button cue both reading during demonstration, and requests phase-based handling
without deduplication. Equal prompts during real play must continue to speak.
The change gates only the extra Jecht speech/beep producer; ordinary message
capture, full explanatory instructions and concise native practice pages remain.

Retained original progression is explicit: swin0000's worker32 writes map byte6
(declaration33, low0C000006) through stages0..6 at PCs23102/23235/23326/23700/
23854/24021/24155. swin0200's worker23 writes map byteD (declaration16,
low0C00000D) through0..6 at70292/70425/70516/70896/71050/71217/71351.
Stages0..5 are demonstration/practice;6 is the completed tutorial. Caller30 in
swin0000 waits for worker32 entries6/7 before requesting its real entry8
(PCs14948/14959/14970). Caller22 in swin0200 waits for worker23 entries88/89
before requesting worker24 real entry6 (PCs52494/52505/52516). There is no
retained bypass into these real entries with an uncompleted phase.

Evidence: install/reset-resume-20261013/jecht-tutorial-stage-evidence.txt,
jecht-pending-flag-evidence.txt, jecht-cue-phase-1.txt and
jecht-game-entry-callers.txt. Both cue workers share the exact phase/active/
selector/timer declarations with their input worker; the generator checks their
raw low/high identities and the complete original0..6 progression. Native
callbacks use existing owner-only fact getters and copied ready declaration
receipts, not new scans, waits or worker engine access. Extra cues require phase6,
active matching input slot/selector and an unexpired timer, checked before and
after the original message-show call. Per-event fields30/31 hold slot/selector;
other users of these fields belong to different hook frames/RVAs. No prompt
text, hash or previous-event history is retained or compared.

The caller/entry audit also exposed the existing second-script practice guard
using entry7/Pc56773, an unrelated dialogue entry. The original tutorial is
worker23 entry89/Pc70289. The generator now derives this entry from the first
original phase-reset write and bounds every practice query to its actual entry.
First-script worker32 entry7/Pc23099 remains unchanged. The callback's entry-PC
guard uses the generated tutorial entry. All110 original query PCs, predicates,
input results, masks and eighteen visible cue rows are retained; only the wrong
second-script practice entry qualification changes.

Intermediate1 focused checks/build passed the phase gate but predate that entry
correction. Final focused3 passes in `jecht-tutorial-cues-3.log`, and the clean
canonical x86 native/launcher tester build2 passes in `jecht-tutorial-native-2.log`;
source stayed frozen during compilation. Focused fixtures cover all six tutorial
stages/all nine original show callbacks per script, pending/expired/selector
checks, phase change during original show, original110 queries and predicates,
Native heap0/0, and two consecutive identical real cues producing two speech
requests and two beeps. They explicitly validate archived practice entries7/89
and refuse second-script dialogue entry7 with assistance enabled. Existing message decoder/readers are
unchanged; prior both144-profile concise-message/tutorial preservation evidence
is reused. No game launch/control, live probe or real backend test occurred.
Guarded deployment passes preflight and installed FFX/Will checks in
`jecht-tutorial-guarded-deploy-1.log`; all7 installed hashes and PRISM0.18.3 x86
receipt/15 locked files are verified. SDL3/license present, Tolk absent. Current
proxy SHA256 `fb7db56519506cb9a29da12343c81db7bb60d05bfbaf29a286a93343786c0268`.
The [current verification record](../../Analysis/FFX/RepositoryEvidence/historical-install/reset-resume-20261013/final-deployment-verification.json)
covers this payload; the previous concise-prompt deployment record is retained
as `jecht-concise-deployment-verification.json`. Final table comparison in
`jecht-final-table-comparison.txt` proves only the13 second-script practice
entry qualifications changed from7 to89; all original PCs/masks/results and18
cue rows remain. Controlled Native query maximum71.0us/cue capture8.5us and
heap0/0 do not claim real game/backend/audible timing. Actual tutorial single
reading, repeat cue audibility and gameplay remain user-side acceptance.
Installer HELD, reset monitor PAUSED, latest77% used/23% remaining (secondary
unavailable); stop at2% retained. Sol handled discovery, implementation and
checks directly without delegation.

## Voice-paced auto-success and cue gate removal — October 14

The user confirmed auto-success completed the minigame, but its immediate
answers ran Jecht's voice clips together. They then corrected the tutorial
report: the apparent duplicate was the original text reading, and practice
prompts spoke while actual minigame prompts were silent after our filter.
This supersedes the earlier simulated phase-gate acceptance. Manual success
remains untested. Speak-always narration may queue original text under the
existing noninterrupting policy; no global speech policy was changed here.

Removed the tutorial-stage/pending-slot/selector/timer filter from manual cue
speech/beeps. Original qualified visible-show events, script/allocation/epoch,
Native owner, focus and mod-menu guards remain. Both equal consecutive events
produce separate speech and beeps; no text/history deduplication was introduced.
The concise control names and corrected second-script practice entry89/Pc70289
remain. The generator no longer emits unused Native tutorial-stage fields.
The original pending-input guards still qualify auto-success input answers.

### Original audio route and ownership

Both retained Winno scripts use the same nine original SFX sequence IDs:
85040,85037,85041,85038,85050,85039,85052,85051,85049. Generated cue rows now
include the unique original parent `playSfx` PC and sequence ID before the
request to each cue worker. The command is opcode **D8/CommonDF**, not the
B5 input-query opcode. All110 original query PCs and18 visible cue rows remain.

Read-only extraction reused the retained native decompile/binary archives:

- `native_bodies_20261014_044224/FUN_0085f150.txt` and
  `native_bodies_20261014_044438/{FUN_00886ae0,FUN_0070d970}.txt`
  retain CommonDF through original sound dispatch/start. The command table at
  VA00C3A360, command33, resolves007082F0 through00707000 to0070D970.
  `native_bodies_20261014_050636/FUN_0081e5f0.txt` confirms the sequence ID is
  preserved by the native call (`param2 & 0x7fffffff`).
- The direct playing-query route through006FA5E0 waits on a mutex/audio command
  thread. Retained `native_bodies_20261014_043834/FUN_006fa5e0.txt` and
  `native_bodies_20261014_044438/FUN_0070cf70.txt` ruled it out for game hooks.
- Original0070D970 scopes the exact sequence on its audio owner. Original
  0070F3F0 starts a constructor-established SfxEvent array: count/capacity at+0/+4,
  array pointer+8, eight-byte event records. Raw retained instructions show
  setCallback using event, 00707B40, record+4 followed by Event::start. The constructor,
  start, destructor and callback are retained in timestamped native_bodies
  outputs043751/044102/044037/044133. Raw callback bytes end in `ret 0x14`,
  confirming the existing five-argument stdcall ABI; types2/4 clear its user word.

New `src/native/jecht_voice.cpp` observes these existing owner callbacks. At
original SfxEvent start it copies only the bounded ready array (maximum16
events, refusing larger/invalid arrays). It publishes sequence/epoch/instance
and opaque event/user identities in fixed lock-free atomic slots. Original
completion types2/4, failed Event::start and SfxEvent disposal retire matching
slots. Neither the callback observer nor game consumer dereferences those
identities. Native object/array reads remain on the original audio owner; game
code uses only mod-owned atomic state. No SDK playing getter, mutex, wait, heap
allocation, worker engine pointer or new timer/polling thread was added.
Per-instance CAS and epoch fencing reject stale replacement/reset observations;
31-bit serial overflow refuses publication and uses fallback.

The five qualified dependencies are RVAs30D970/30F3F0/30E930/307B40/548C9A;
full bodies, the original relative start call and callback absolute operand
are checked by the compiled resolver. Master extension generation now also
preserves the existing input getter schema and exact Installed98-byte4D remap
guard from retained `jecht-installed-practice-body.txt`. Reconstructed bytes
are contiguous/exact; no installed executable/source was restored or patched.
Fresh generator evidence: `native_extensions_20261014_120021_425390.json`.
All named native_bodies outputs above are under `Docs/Scripts/Offline/output`;
discovery logs are `install/reset-resume-20261013/jecht-sfx-discovery-*.log`.

### Pacing, cancellation and limits

On each original sound command, auto-success arms one of the original two
pending slots before that clip starts. Input is synthesized only at its existing
qualified query, with original script/worker/entry/selector/active/timer guards.
It waits for the new sequence instance and tracked Jecht clips to finish, then
leaves125ms before answering; minimum pacing is250ms. Enabling auto mid-prompt
uses a bounded lazy capture. Missing audio falls back after1s, and a2.5s hard
limit prevents a lost completion from stalling assistance. There is also125ms
minimum spacing between synthetic Confirm answers. These are elapsed-time
checks on original input queries, never sleeps or countdown changes. A clip
longer than the hard limit can still be interrupted; real pacing needs gameplay
confirmation. Practice assistance remains immediate and explanation Confirm
stays manual. Toggle/reset clears pacing; allocation/epoch/focus guards remain.

Final optimized `/W4 /WX` production-module fixture
`jecht-paced-native-5.log` passes both original scripts, all110 queries/all nine
original predicates per script, ungated visible cues/equal repeats, manual
physical pass-through, failed/missing/lost audio,16-event completion, owner-only
array access, stale epoch/instance, reset and completion-grace checks. Test
audio objects follow the recovered native layout, on a separate owner thread;
callbacks use a separate thread. The SDK and time are controlled substitutes,
not actual FMOD/PRISM/game execution. Hook heap use is0/0. Maximum controlled
Native query76.7us and cue capture5.9us do not establish whole-game, callback-tail,
real-backend or audible response. Prior unchanged message decoder evidence
(`jecht-concise-readers-2.log`, both144 profiles) is reused.

Intermediate fixture1 had two local shadow warnings;2-4 exposed the sound
opcode error (B5 instead of actualD8), delaying arming until the first query.
That was corrected from original bytes and focused traces; temporary PTRACE
instrumentation was removed before final5/build1. The final profile fixture
`jecht-paced-profile-1.log` passes Reference/Installed/Will, ambiguity/refusal,
transactional failure, old Installed remap guard and all new audio dependencies.
Its synthetic code-operand relocation now uses the already mapped function
instead of treating the callback as arbitrary data.

Warning-free canonical x86 native/launcher `--test-build --no-deploy` completes
in `jecht-paced-build-1.log`, with production source frozen during compilation.
Closed-process canonical guarded deployment passes actual FFX/Will preflight
and installed checks in `jecht-paced-guarded-deploy-1.log`. Fresh
[verification](../../Analysis/FFX/RepositoryEvidence/historical-install/reset-resume-20261013/final-deployment-verification.json)
at2026-10-14T12:23:31Z verifies all7 installed hashes and PRISM0.18.3 x86 receipt/
15 locked files. SDL3/license present, Tolk absent. Deployed proxy SHA256:
`4f58f784a6b015503f404b3c48ca0f2f86c03dddd844c93620b195961ef50c15`.
Prior gate deployment evidence remains `jecht-tutorial-deployment-verification.json`.

No game launch/control, live probe, real SDK/backend/registry test or Tester Logs
access occurred. Sol performed discovery/implementation/review/verification
directly without delegation. [Player acceptance](gameplay_acceptance_0.5.6.md)
prioritizes restored real manual cues and smoother auto-success; their actual
audibility/timing remain user-side. No authorized deployment blockers remain.
Installer HELD, premature do-not-use marker retained, reset monitor PAUSED.
Latest core80% used/20% remaining, secondary unavailable; stop at2% retained.

## Actual show producer, reaction timing and SFX subtitles — October 14

The user reports that completion-driven auto-success waits too long, real manual
cues remain silent, and Jecht's spoken taunt subtitles bypass dialogue suppression.
They want some natural overlap, without instant voice clipping. The later timing
question also requires distinguishing assistance delay from the game's deadline.
This supersedes the prior claim that removing the phase gate alone repaired real
manual cues. Earlier auto-success working/rushed and later slow/silent reports
are gameplay evidence; the replacement still needs user audible/manual testing.

### Cause and exact native route

Archived `FUN_00864180` is a **multi-opcode interpreter invocation**, not one
callback per instruction. It loops through earlier instructions before reaching
CALL006A and advances the cursor after the Common return handler finishes.
Our previous cue fixture incorrectly invoked interpreter entry at each cue PC,
so it passed a route the actual scheduler did not use. Auto input hooks worked
because they already observed the current query inside that loop. Sound arming
at interpreter entry had the same error and could fall back to a later query.

Original cue entries first position the message, make its box transparent,
prepare a field string and set flags3002; only then do they call006A. Their
strings are the nine original taunts, not tutorial button glyphs. The visible
text's position supplies the required direction; the exact original input
predicates supply its controls. Retained compact evidence:
`install/reset-resume-20261013/jecht-real-producer-atel-1.txt`.

The repaired capture uses original Common006A executor **VA008589F0/RVA4589F0**,
full314-byte body, scoped to the current original interpreter/worker/script/
entry/PC. Its original body opens the native message through008B8980. Capture
after that original call publishes the concise control recipe/beep. Generated
rows now include the actual window0/1, preserving all18 show PCs,110 queries,
direction masks and second-script practice entry89. No tutorial/pending filter
or text-content deduplication is added to cue delivery.

Sound correlation observes **VA00886DE0/RVA486DE0**, full40-byte original wrapper,
with the exact DF PC, interpreter worker and sequence argument before submission.
The31-byte CommonDF executor masks to seven neighboring wrappers; generation
correctly refused it as ambiguous. The unique downstream40-byte wrapper is
qualified instead, without a blanket delta. Reference generation and the compiled
resolver independently qualify both new producers. Evidence:
`native_extensions_20261014_130815_544335.json` under Offline/output.

New retained decompile outputs under `Docs/Scripts/Offline/output`:
`native_bodies_20261014_055903` (864180 interpreter and776A50 encoder),
`...055948` (877720/8776B0 command dispatch), `...060020` (8589F0 show),
`...060742` (886DE0 sound wrapper), and `...061046` (wait init/exec and85D080).
The raw Common table at VA00C50050 confirms006A's return executor8589F0,
DF's85F150 and wait's85C3E0/85C570. These are retained archive reads; no new
Ghidra/game process or install-source investigation was needed.

### Reaction window and manual speech

Both originals initialize the shared round counter, loop while it is below300,
increment it with original wait1 yields, and accept success only for a count>10.
Wait85C3E0 stores the count;85C570 normally decrements once per scheduler tick,
with a native speedup branch. The already-retained native conversion divides
ticks by30 (see `thunder_lightning_20261008.md`). This is approximately ten
seconds at ordinary speed, requiring at least eleven successful inputs. Each
of two active memory slots remains pending until its own correct response or
the round end. There is no independent half-second expiration per memory.
Speedup/slowdown and actual wall-clock/backend timing remain user-side.
Original real-entry blocks are retained in `jecht-realtime-window-evidence.txt`.

Auto-success now reacts **450ms after the original visible show**, with125ms
minimum spacing between synthetic Confirms; a mid-prompt toggle can arm from
the next original query. It permits natural voice overlap instead of waiting
for all clips to finish. Practice assistance is still immediate; explanatory
Confirm pages, original input predicates/countdown/scoring/reward ownership
and manual physical input are unchanged. There are no native sleeps or waits.

Manual cues have no intentional delay. The first current cue interrupts old
narration. If the other original slot still has an announced pending cue, the
second appends so it cannot erase the first controls. The original active flag
is read through the qualified owner-side declaration getter; this affects only
speech interruption, never whether an event emits. Every equal distinct show
still emits speech and a beep. Two simultaneous cues necessarily share one
screen reader voice; actual time to first audible/usable controls is unmeasured.

### SFX caption policy and bounds

Each exact sound/show pair carries a copied epoch/sequence/pre-start serial/time
token. The ordinary message page and encoded subtitle producers capture it only
inside that qualified show call/window; workers receive no engine pointers.
Plain taunt captions bypass inherited TK selector cursor state. Existing message
event guards remain, including the old ordinary dialogue guard; no new text
comparison guard was introduced. Control cue output is independent of captions.

The audio owner publishes the actual next instance's successful FMOD start and
nonzero original SfxEvent volume after native start returns. Native disabled/
missing sequences and failed starts publish an unvoiced result. Completion does
not erase a captured caption's successful-playback provenance. Matching captions
use the central Suppress/Braille only/Speak always policy; unrelated SFX/dialogue
are unchanged. This covers ready per-start volume/result, not a new SDK query
for subsequent global mixer/device mute or pause; those real backend states are
not measured by this offline fixture.

Unknown audio holds only the owned subtitle on the preparation worker, maximum16
captions and the shared character bound. Audio readiness notifies preparation.
Missing results become unvoiced after500ms; a fixed16-entry native wake schedule
uses existing interpreter events to wake that fallback, independent of newer
requests replacing a slot. Reset/cancellation retires pending captions. Capacity
refusal falls back to ordinary subtitle speech. Control cues/input never wait
for these captions, audio, a worker, an SDK getter, a lock or new polling timer.

### Verification and deployment checkpoint

`jecht-realtime-native-6.log` passes production Jecht/audio/resource modules with
`/W4 /WX /O2`: one interpreter scope executes earlier opcodes before exact sound/
show callbacks; real REQ priority1, both scripts/all110 queries/nine original
predicates, all visible/equal cues, first/second cue interruption order,450ms
reaction, failed/zero-volume/disabled/missing audio,16-event completion, opaque
instance/epoch/owner/reset guards and Native heap0/0. Maximum controlled Native
query53.0us/cue capture59.0us are fixture costs, not real backend/audio latency.
Earlier native1 lacked the notifyPreparation declaration; adding the speech
domain header fixed compilation. Subsequent runs extended actual event/level/
wake/interruption coverage; their earlier passing subsets are retained.

`jecht-realtime-dialogue-3.log` passes both actual copied message producers,
all three voiced modes, delayed readiness, inherited cursor handling, failed
fallback and reset retirement, alongside the existing generic voice/movie
regressions. Its SFX token/result is controlled; the separate native fixture
tests real module generation/correlation. Native1/dialogue1 fixture compile
errors were repaired (missing declaration/reset function). The dialogue batch
retains its existing CL D9026 command-line warning; this is distinct from
production compiler diagnostics. Both144-site reader profiles pass in
`jecht-realtime-readers-1.log`; qualified Reference/Installed/Will signature/
dependency/ambiguity/transactional checks pass `jecht-realtime-profile-1.log`.

Production source stayed frozen for the warning-free canonical x86 native/
launcher tester build `jecht-realtime-build-1.log`. Closed-process guarded
deployment passes actual FFX/Will preflight and installed checks in
`jecht-realtime-guarded-deploy-1.log`. Fresh
[verification](../../Analysis/FFX/RepositoryEvidence/historical-install/reset-resume-20261013/final-deployment-verification.json)
confirms all7 installed hashes plus PRISM0.18.3 x86 receipt/15 locked files,
SDL3/license present and Tolk absent. Deployed proxy SHA256:
`ba8d3e67edc581ee35b80c8f07c4a8a663834e3eef15abe053873a163ded59ea`.
The preceding deployed payload's verification is preserved in
`jecht-before-realtime-deployment-verification.json`; no older payload or source
was restored. No authorized engineering/deployment blockers remain. Actual
manual cue audibility/450ms pacing/subtitle suppression still need user gameplay.
Installer HELD; reset monitor PAUSED. Sol owns discovery/implementation/review/
verification directly, no delegation. Latest core82% used/18% remaining,
secondary unavailable;2% stop retained. No game control/probes/Tester Logs,
real registry/backend test, installer rebuild, publication or source restoration.

## Complete-round solver and repeated Confirm checks — October 14

The user confirms taunt subtitle suppression works. Paced assistance now stops
after3/4 inputs, whereas the earlier immediate assistance completed the game.
They heard no control prompts but may have left auto-success On. Current control
cue speech/beeps are intentionally manual-only; this latest silence does not
confirm a failure with assistance Off. Preserve the working caption pipeline.

### Reproduced failure and repair

The original center predicate at swin0000 PC25109 (equivalent second-script
predicate retained) eagerly requires **two Common0051 Confirm results ANDed
together**, along with released directions and neutral axes. The first synthetic
Confirm updated the global125ms spacing clock. The second query in that same
calculation was then refused. Repeated attempts at the center refreshed that
clock and could starve the other pending memory too. The former predicate test
advanced virtual time150ms between queries and could also settle1000ms, hiding
this within-frame contradiction.

`jecht-round-native-1.log` reproduces the same-tick center failure. An independent
complete original-bytecode replay, before the production repair, also fails:
`jecht-round-native-4.log`, first-center scenario, two displayed memories and
no award/learned flag after the original round ends. These are retained failures,
not successful gameplay evidence.

Each qualified visible memory now starts a fresh mod-owned response state. Once
its delay is satisfied, **every original query for that pending prompt receives
the same admitted response**. Only its first synthetic Confirm starts spacing;
the later Confirm stays true without refreshing the clock. New source/show,
toggle/reset, allocation/epoch and original active/selector/expiry guards retain
ownership. Repeated identical prompts start fresh response states. This is
input consistency, not text/event deduplication, and no game score/clock/reward/
completion/save field is written by the production mod.

Center also requires native axis readings strictly between64 and190. The original
Common0122 integer-return table slot resolves to **RVA45D100**, full67-byte body;
its original executor pops the index, clamps to0..13 and returns the original
byte getter888B70. The full body independently qualifies through the compiled
profile and startup dependency check. Only the48 exact original pending-memory
axis query PCs return neutral128 under auto-success, using the same script/
entry/worker/selector/active/round guards. The former110 query PCs are retained,
giving158 total; manual axes and unrelated callers pass through. Source evidence:
`native_bodies_20261014_074051/FUN_0085d100.txt` and
`native_extensions_20261014_144747_783310.json` under Offline/output.

With latching alone, `jecht-round-native-5.log` passes the normal-clock center
round but fails the accelerated clock case. Response and Confirm spacing now
use450/125ms targets **or14/4 original round-progress steps**, respectively
(ceil of each target at30 original ticks/second). The existing native round
counter is copied through its qualified owner getter. This keeps pacing inside
the original300-step budget when simulation advances faster than host time.
No extra timer, polling thread, native wait or counter change is introduced.
The short normal-speed pacing and natural voice overlap remain; the working
taunt suppression/audio ownership code is unchanged.

### Complete original-round evidence

`generate_jecht_round_fixture.py` emits test-only original entry/integer/jump
tables and call arities from the retained Winno archives. At runtime the fixture
compares integers/jumps with actual retained EBP bytes at worker header+0x18/
0x24; the existing production guard qualifies the active entry. The pass counter
is derived from the exact
original score>10 bytecode. Initial fixture compilation refused empty C++ arrays
(fixed with unused sentinels/count0); the first jump-table comparison correctly
rejected the fixture's mistaken+1C offset, corrected against the original bytes.
Neither correction changed game/source data or native layout access.

`jecht_round_followups.inc` executes the original main/cue bytecode, stack
operations, eager expressions, switches/jumps, variable/array writes, priorities,
requests and wait yields. One interpreter scope covers all instructions executed
in a native step; there is no artificial time advance between input queries.
Production Jecht/ATEL resource/identity/getter callbacks process that original
flow. The game itself is not launched: rendering, ready loading, sound/effects,
random inputs and scheduler clock are controlled dependencies. Original script
reward writes affect only cloned fixture data, distinct from the read-only
production mod/input-result override.

Final optimized `/W4 /WX /O2` `jecht-round-native-8.log` passes:

- **24 complete rounds:** both original scripts, repeated centers, cyclic all-nine
  coverage and seeded random selectors, neutral/0/255 physical axes,33ms/4ms/
  frozen host-clock steps. All score31–32, exceed the original eleven-success
  requirement, and reach both original achievement and learned-shot branches.
- **Two original manual rounds:** assistance Off produces both visible control
  speech/beep requests, no automatic score/award, and unchanged physical axes.
  Assistance On produces no manual control-cue requests, as designed.
- All158 exact queries, all nine same-tick original predicates per script,
  repeated equal cues, caption result/owner/lifecycle checks, cancellation,
  Native heap0/0 and the unchanged input/focus/mod-menu guards.

Controlled maximum query19.6us/cue4.4us follow repeated warm fixture rounds;
they are not a modded/unmodded runtime comparison or audible latency evidence.
Original simulated rounds end at382 fixture ticks including warmup/cleanup;
only the original300-step memory phase owns the success budget. Actual current
acquisition/pacing and manual-Off audible delivery remain user-side. Do not
repeat the earlier claim of gameplay readiness from isolated input answers.

`jecht-round-profile-1.log` passes full new-axis qualification, Reference/
Installed/Will, old Installed98-byte practice guard, missing dependencies,
ambiguity and transactional refusal. Unchanged message/audio/decoder evidence
is reused from realtime dialogue3/readers1, supplemented by the user's confirmed
taunt suppression; none of those producers were modified in this repair.
Canonical x86 native/launcher `jecht-round-build-1.log` completes without warnings,
with production source frozen. Closed-process guarded deployment passes actual
FFX/Will preflight and installed checks in `jecht-round-guarded-deploy-1.log`.
Fresh [verification](../../Analysis/FFX/RepositoryEvidence/historical-install/reset-resume-20261013/final-deployment-verification.json)
confirms all7 installed SHA256 hashes and PRISM0.18.3 x86 receipt/15 locked files,
SDL3/license present, Tolk absent. Deployed proxy SHA256:
`0e59e673e46e6354fa831a3014a00a390e6541c738fd60437a0a3b109bc33eb9`.
Previous installed evidence is preserved in
`jecht-before-round-deployment-verification.json`. No authorized engineering/
deployment blockers remain; new full gameplay/manual-Off audibility await user.

Sol handled discovery/implementation/review/verification directly without
delegation. Installer HELD and do-not-use marker retained; reset monitor PAUSED.
Latest core84% used/16% remaining, secondary unavailable;2% stop retained.
No game launch/control/probe, Tester Logs, real backend/registry test,
installer rebuild, publication or source/payload restoration occurred.

## Tutorial Confirm-only auto-success — October 14

The user now confirms auto-success beats the real minigame, manual control
prompts work, and voiced taunt suppression works. These supersede the preceding
build's pending gameplay statements. The remaining report is specific to the
tutorial's centered “Press C” practice prompt. Installer rebuilding is held
until the user tests this repair; no automatic packaging follows deployment.

The retained original tutorial has **four** practice predicates: Up Right,
Down, center/Confirm, Down Right. The previous generator selected only blocks
with Common4C positive held-direction calls, so it omitted center. Center
instead requires all four Common50 directions false, four Common122 axis
comparisons around neutral128, and Common51 Confirm true. Original bodies and
phase transitions are retained in
`install/reset-resume-20261013/jecht-center-tutorial-original.txt`.

The exact original center predicate starts at PC23927 in swin0000 worker32/
entry7 and PC71123 in swin0200 worker23/entry89. The prior display calls select
window0/string69 or94, respectively; the loop belongs to original phase4 and
its success branch closes the prompt and changes the phase to5. The generator
requires this original call sequence, display identity and phase transition.
It adds nine scoped query answers per script: four false directions, four
neutral axis128 answers and one true Confirm. The existing exact-resource,
checksum, owner, interpreter, entry, PC, ancestry, generation, focus and mod-menu
guards apply. Production callback/pacing/speech/audio code is unchanged.

The two earlier Common4D explanation/example advance queries remain excluded:
PC23220/23311 and70410/70501. They still require physical Confirm. No explanation
window, completion flag, score, original timer, save or native input word is
written. All132 real-minigame query rows are exactly unchanged, checked against
`jecht-center-real-queries-before.json`; only18 center-practice rows were added
to the fixed176-query table. There are no new hooks, waits, polling or workers.

### Focused verification

`jecht-center-native-1.log` reproduces the omitted predicate on the old table
using original retained bytecode. After the generated table repair,
`jecht-center-native-2.log` passes `/W4 /WX /O2`:

- All four original tutorial predicates in both scripts, with physical axes0
  and255. Manual center still requires Confirm and preserves native axis limits.
- Both original explanation advance loops in each script pass through physical
  input with assistance On; no tutorial page skipping was added.
- All176 direct input identities, nine real same-tick predicates per script,
  24 complete original real-game rounds scoring31–32 and reaching original
  award/learn branches, plus two manual rounds without automatic score.
- Equal consecutive cue output, caption ownership/lifecycle/cancellation,
  focus/native-owner/resource/epoch guards and Native heap0/0.

Controlled warm maximum query22.6us/cue4.8us is fixture evidence, not actual
game/backend latency or an unmodded comparison. Original full-round timing and
confirmed working manual producers were preserved. Existing profile1 evidence
from the preceding repair remains applicable because no resolver dependency
changed; canonical deployment must still qualify the actual installed profiles.

Canonical center-build-1 completed without warnings, but was not deployed:
the user requested eleven-answer pacing while it built. The combined final
build/deployment is recorded below. No game launch/control/probe, Tester Logs access,
real backend/registry action, installer rebuild, publication or prior-build
restoration is authorized by this small follow-up. Sol owns discovery,
implementation and verification directly; no delegation was used.

## Eleven-answer pacing — October 14

After the tutorial repair, the user requested exactly eleven accepted responses
spread across the original ten-second round at about0.75 seconds per input.
The original score>10 reward branch provides a decisive success count; counting
synthetic Confirm queries would be incorrect because center tests Confirm twice
and the original branch can reject an incomplete response.

`jecht-eleven-score-original.txt` retains the exact original score guard/reset
and nine increment branches for both scripts. The score is worker32 declaration29
(`raw_low=0C000002`, map byte2) in swin0000 and worker24 declaration12
(`0C000009`, map byte9) in swin0200, both `raw_high=1`. The generator verifies
the original `PUSHV; PUSHII10; GT` score guard bytes and exact declarations.
Each scoped real input query copies the score with the existing qualified
owner-side variable getter, alongside active/selector/timer. It returns native
physical input once score>=11 or the fact cannot be qualified. The mod never
writes score, timer, reward/save data or native input words. This check precedes
the per-request admission latch, so an admitted prompt cannot keep supplying
answers after the game accepts the eleventh success. Additional physical player
inputs remain native.

Both initial reaction and minimum spacing target750ms, capped by23 original
simulation steps (ceil of0.75*30). Pacing still follows native round progress
under speedup/coarse/frozen host clocks; no sleep or polling is added. Tutorial
practice bypasses real-round pacing and the success cap. Center's two eager
Confirm checks still see a single consistent admitted response. Manual cues,
their immediate producer, voiced-taunt suppression, cancellation and all
existing event guards are unchanged. The fixed query table remains176; only
two scalar score-identity fields were added to each script descriptor.

`jecht-eleven-native-1.log` demonstrates that the preceding solver exceeds the
new limit (score32). Final `/W4 /WX /O2` `jecht-eleven-native-2.log` passes all24
original-bytecode real rounds at **exactly11 successes**, all original award/
learn branches, both slots/all-nine/cyclic/repeated-center/seeded random cases,
physical axes0/128/255 and33ms/4ms/frozen clocks. First accepted scores occur
at native step24/25, last at254/255, minimum gap23 steps, inside the unchanged
300-step budget. At normal speed this finishes the eleven answers around8.5
seconds, leaving roughly1.5 seconds spare. The original script shows13 memory
prompts rather than33: it initially fills both pending slots and displays a
replacement for each accepted input. The last pending prompts simply remain
until the original timer ends; the original VM still completes and learns the
shot without forcing those prompts or shortening the round.

Both manual rounds, all four tutorial predicates/extreme axes, explanation
exclusion, all176 direct queries, score10/11/255 boundaries (including an already
admitted response), actual sound/show scope, repeated equal output, lifecycle/
owner/focus/resource guards and Native heap0/0 pass. The500ms subtitle fallback
remains independent of750ms input pacing. Controlled warm maximum query11.1us/
cue5.5us is fixture evidence only, not real audible latency or a comparable
performance baseline. Rendering/audio/scheduler dependencies remain controlled;
the new pacing and tutorial fix still require user gameplay confirmation.

The intermediate center-only build was retained as evidence, without deployment
or packaging. Final canonical `jecht-eleven-build-1.log` completes without warnings.
Closed-process guarded `jecht-eleven-guarded-deploy-1.log` passes actual FFX/Will
preflight and installed signature/operand/dependency checks, launcher bootstrap
and all seven installed SHA256 comparisons. Fresh
[verification](../../Analysis/FFX/RepositoryEvidence/historical-install/reset-resume-20261013/final-deployment-verification.json)
confirms PRISM0.18.3 x86 receipt/all15 locked hashes, SDL3/license present and
Tolk absent at `2026-10-14T15:31:23.3971724Z`. Final proxy SHA256:
`ed88bfb5f7df466a66fd9569a660f59f53b891f132a82be97287bb7c35ce1764`.
Earlier installed evidence is preserved in
`jecht-before-eleven-deployment-verification.json`; source hashes are retained
in `jecht-eleven-source-verification.json` and unchanged after the build.

There are no remaining engineering/deployment blockers in this follow-up.
The user must still confirm the tutorial center prompt and new smoother pacing
in-game. Manual prompts and earlier real-round success are already confirmed;
this is not a claim that the new timing has been gameplay tested. Installer
rebuild/package stays HELD until the user confirms these tests; the do-not-use
marker remains. Reset monitor PAUSED. Sol handled discovery/implementation/
review/verification directly; no delegation. Latest core85% used/15% remaining,
secondary unavailable,2% stop retained. No game launch/control/probe, Tester Logs,
real backend/registry action, installer compilation/package, publication or
source/payload restoration occurred.

## Final user confirmation and release completion — October 14

After the eleven-answer build above, the user reports auto-success works
perfectly. This supersedes its pending tutorial/pacing acceptance and fulfills
the user's installer condition. No Jecht input or speech behavior changed in
the final title/defaults build; the actual deployed payload and rebuilt package
are verified in [the final delivery record](title_prompt_and_settings_20261014.md#current-delivery-checkpoint).
The earlier manual-cue and taunt-suppression confirmations remain distinct from
offline timing evidence. The reset monitor stays paused.
