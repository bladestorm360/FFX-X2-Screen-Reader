# Thunder Plains lightning assistance — 2026-10-08

Later player follow-up: Auto dodge is gameplay-confirmed. Apparent manual
success was withdrawn because Auto dodge was enabled. On explicit request the
warning now plays at elapsed0, the native visual flash start, exposing the full
twelve-tick manual window (about0.4seconds at normal speed). Auto dodge remains
at its working elapsed6. Help names the primary configured C/controller dodge
binding, omitting unconfirmed Enter, and instructs one press with Auto dodge Off
for manual practice. Final affected checks/build/proxy and guarded deployment
passed both executable profiles/launcher/13 installed hashes. Manual acceptance
remains pending. Earlier elapsed6 warning descriptions below are historical.
[Latest implementation, evidence and limits](inn_menu_battle_followup_20261008.md).

Requested behavior: a short beep about 0.2 seconds before the native bolt's
impact and independently togglable Auto dodge. This is user-authorized in-mod
assistance; no agent game launch/control/probe is authorized by this feature.

## Corrected offline investigation

The first investigation incorrectly searched script declarations for raw
SaveData offsets 3FC/3FE/400. Archived FUN_00871130 initializes controller
location-0 base to SaveData **plus 1EC**. FUN_0086C270 adds a declaration's
24-bit offset to controller+2C; FUN_0086C500 selects the element width.
Consequently lightning declarations use **210/212/214**. The earlier
"no counter declarations/no processor identified" finding is superseded.

SaveData itself is discovery VA0112CA90/RVA D2CA90: FUN_00785300 returns its
address. Local Fahrenheit core/fh/FFX/{gstate.cs,savedata.cs} independently
documents unsigned 16-bit total bolts +3FC, total dodges +3FE and best streak
+400. The original script owns these counters and all awards.

Existing Ghidra decompile evidence is in
Docs/Scripts/Ghidra/output/FFX/decompile/layer2_game_logic.txt, with matching
bodies in other decompile layers. Full existing program blocks are under
Docs/Scripts/Offline/output/reader_reference_20261003_235502/. blocks.tsv lists
the discovery VA and length for each binary block. This investigation reads
those existing archives; it does not execute Ghidra, Frida or a game.

Common's Fahrenheit core/fh/Atel/call.cs address850050 is an RVA, corresponding
to discovery VA00C50050. Its616 entries occupy16 bytes each: init+0, exec+4,
float-return+8, integer-return+C. The actual table resides in
block_00C0A000.bin at offset46050. Script name/argument attribution comes from
FFXDataParser/src/main/java/atel/model/{ScriptCallTargetLib.java,ScriptConstants.java}
and the derived Docs/Scripts/Ghidra/output/FFX/atel TSVs. These are external
research references, not adopted loaders.

Investigated unknown Common handlers00F2,00F3,0148,0151,016E resolve to
8576F0,857AE0,858250,8592B0,85C170 respectively. They manipulate a controller
flag, a graphics-manager setting, worker fields, a bitset and saved+B4/B6;
none supplies the ordinary lightning timing hook. kami0000 workers38–40 are
tutorial layers/fades. Scripted SFX15403 atworker28 PC8CEF is a story bolt;
the ordinary bolt below uses SFX13CB5 (81077). A global flash/SFX hook would
mistake cutscenes for playable lightning attempts.

## Native playable event and timing

Extracted sources are under
Docs/Scripts/Offline/output/FFX/20261003_audit/files/ffx_ps2/ffx/master/jppc/event/obj/ka/.
Existing maps_v3.json and src/puzzles/atel_model.py decode the original flow.

| Identity/operation | South | North |
|---|---|---|
| Script | kami0000 | kami0300 |
| Code length / workers |46958 /64|56818 /84|
| Code FNV1a |4B4A7773|36D996BE|
| Lightning worker / entry |12 /1|18 /6|
| Entry start |3414|6535|
| Total-bolts write |3495, variable15|65CC, variable17|
| CALL4003 warning |34A4|65DB|
| CALL0051(5) |34B4|65EB|
| Cursor during query return callback |34B4|65EB|
| Cursor after query operand |34B7|65EE|
| Elapsed variable / declaration |24 /0C000008|27 /0C00000A|
| Press-count variable / declaration |23 /0C000007|26 /0C000009|
| Impact/control disable |381B|6832|
| Bolt GFX / SFX calls |383F /3845|6856 /685C|
| Exactly-one-press branch |384F|6866|
| Current streak / declaration |31 /2C000012|30 /2C00000E|
| Saved dodge increment |3863, variable16|687A, variable18|
| Saved best write |3873, variable17|688A, variable19|
| Native success request |396E, worker0 entry30|6985, worker0 entry29|
| Native failure request |3988, worker0 entry29|699F, worker0 entry28|

South EBP SHA256:
7e9d613e25b47fcf8809847ca54c08c1a0497edf56939a463b6660e0b8c709cb.
North EBP SHA256:
5724b8edb25b0121b2e37b62dba30b62e88d73faa758a7664252285b03e0479a.

Both scripts increment total bolts, start CALL4003(12,207,207,255), then check
elapsed<12. While the native attempt continues, CALL0051(5) counts press edges,
wait1 yields, and elapsed increments. At12 the script disables player control,
starts bolt graphics/sound and accepts **count==1**. Zero and multiple edges
fail. Success increments current/total dodges and best streak, grants native
threshold flags5/10/20/50/100/150/200 and requests the native dodge animation.
Failure resets the current streak and requests the native hit animation.
Both success/failure preserve native total-bolt threshold flags30/80.

Native success uses actor motion portion69777 frames8..24 then25..end, the
ordinary movement destination and await-motion calls. Failure uses motion70285.
The mod never writes counters, changes geometry, requests these animations or
awards items itself.

Common0000 wait init85C3E0 stores the count; exec85C570 normally decrements it
once per scheduler tick. Its native speedup branch consumes645F40's clock
difference. Native timing conversion70EAE0 divides ticks by30; input repeat
898F0 multiplies configured ticks by0.03333333. The cue at elapsed6 leaves
six ordinary simulation ticks, approximately0.2seconds before impact.
Slowdown/speedup can change wall-clock duration. No wall-clock timer guesses
or polling producer are introduced; live timing remains user-side acceptance.

## Exact input hook and ownership

Common0051's integer-return table slot resolves to discovery VA0085EF20,
RVA45EF20. Arguments are worker in stackarg0, call-local result storage in
arg1 and native argument-pop context inarg2. The original body calls86DE90
with arg0/arg2 to pop the bit selector, then86AF00 to read the raw16-bit
pressed aggregate atRVAF27090. It tests1<<(selector&31). Native871D10 populates
that aggregate from888DF0's history-bank+6 press edges. Unlike mapped Common
input queries,0051 does **not** call488BB0. Selector5 is rawbit20.

Configured captions must therefore use keyboard native actions4/53 and
controller action18, through nativeBindingName/configured binding helpers.
Action5 is not this action; inverse-mapped Confirm is not justified here.

Reference45EF20 bytes:
55 8B EC 56 FF 75 10 FF 75 08 E8 61 EF 00 00 83 C4 08 8B F0 E8 C7 BF 00 00
8B CE 0F B7 C0 BA 01 00 00 00 D3 E2 5E 23 C2 F7 D8 1B C0 F7 D8 5D C3.

lightning.cpp hooks464180 enter/leave to retain thread-local active interpreter
ancestry (Event.before24/25, avoiding pedestal's0/1). The45EF20 leave handler
requires that ancestry, event/resource generation, exact hashed script,
worker/header/local index, native signal entry/level, verified entry start,
exact CALL cursor and declaration low/high pairs. It reads live values
through46C500. Resource identity checks are cached within one generation,
matching immutable native script-resource handling in existing minigames.

### Player failure and corrected cursor — later 2026-10-08

The user reports both warning and Auto dodge failing; earlier deployment and
synthetic PASS results were not gameplay confirmation. The old fixture placed
the cursor at34B7/65EE and reproduced the implementation's mistaken assumption.
Archived869D00 reads the opcode without advancing its frame cursor;464180
advances frame+18 only after877770 and the Common integer-return hook finish.
The live callback therefore sees34B4/65EB. Production and fixtures now use that
position, explicitly reject the post-instruction position and retain the actual
South/North code hashes and declarations. Routine route clears/losses now pause
route audio alone; full unsafe-context teardown still cancels every cue. This
prevents independent lightning warnings being erased by ordinary navigation.
Once-per-strike diagnostic lines record warning/injection and native elapsed
time in the own-session mod log. [Combined evidence and verification](player_reports_20261008.md).

Strike identity consists of generation, worker, chunk, native signal and
original total-bolt counter. At elapsed6 the warning plays once, if enabled.
Auto dodge supplies one true query result only if the original returnedfalse,
native count iszero and no prior mod override exists. An existing physical
press wins; later physical presses remain untouched, including native multiple
press failure. Synthetic input affects only the exact native lightning query,
never aggregate input, another script, physical controls or menus.

cancelLightningWarning retains cancellation for the same native strike.
Focus/menu return cannot rearm it; the next actual bolt counter/resource
generation can. Public hooks are installLightning/resetLightning and
cancelLightningWarning. Audio uses navAudioLightningWarning(epoch) and
navAudioCancelLightningWarning; the primary integrates those mixer APIs.
The reader also rejects battle, scene, mod menu, utility console, missing
field control and nonzero46C840 mode. Reset/central focus and menu handlers
must invoke cancellation promptly; an audio callback must not inspect game
pointers or invoke native UI functions.

## Verification and outstanding acceptance

The archived 864180 CALL case invokes 877720 for initialization, 8776B0
for execution and 877770 for the return value. Common0051 has no exec
callback; 8776B0 returns5. 877770 calls the integer-return callback with
worker, local-return storage and pop context, then sends its EAX through
86E380 to the native VM stack. These intermediate dispatchers have no
installed hooks, so hooks.cpp's nearest-hook parent is464180. The query
also requires nativeGameThread(), whose owner is established by225930.

tests/lightning_fixture.cpp loads both exact archived EBP resources and fires
synthetic native interpreter/query events. It covers one injected edge/native
count semantics, single/multiple/no manual edges, independent settings,
repeated queries, next-strike rearming, focus/menu cancellation, ancestry,
generation, cursor/entry/level/worker, script hash and declaration failures.
It does not pretend to execute the actual game VM or confirm live animations.

Primary integration is complete: settings/audio/context cancellation and fixtures
pass. The 48-byte raw query signature has five lookalike archived handlers;
its call at +20 must resolve to the exact word getter whose operand matches
471D10's pressed aggregate at +52. This independently qualified query is unique;
held-word lookalikes and true ambiguity are rejected without partial publication.
The actual SDL dummy mixer checks the independent centered 60ms cue and cancellation.
Lightning warning defaults On under Navigation audio; auto dodge defaults Off under
Miscellaneous. The warning-free canonical development build and guarded local
deployment passed both executable profiles, launcher checks and 13 payload hashes.
See [console/integration evidence](console_thread_hardening_20261008.md).
Gameplay timing, user-configured controls, actual dodge animation
and long consecutive streak behavior remain user-side acceptance. No Tester
Logs were accessed. A rejected broad root inventory never ran; all subsequent
reads stayed within specifically named source/reference/archive folders.
