# Open issue repairs — 2026-10-12

Implementation and offline verification are in progress. Gameplay acceptance is
user-side. No game launch/control/probe, Tester Logs access, registry change,
commit/push, publication or replacement of the 0.5.5 ZIP is authorized/performed.
User authorized relevant Saves/Dylan evidence and offline installed text/graphics
asset reads. The Polish package was supplied in the FFX workspace.

## Architecture and evidence

- Airship scripts include physical lines that call camera/layer transitions
  (8006 and a worker0 request) within the same EBP rather than a room warp.
  Exact supplemental contracts qualify by script name, length, FNV and worker:
  hiku0000 (42244/ffb34f6f):26,27; hiku0200 (21789/858fecc9):20,22,23,24;
  hiku0500 (80410/733360dc):23,25,26,27,28,29;
  hiku2000 (13902/2372f284):14,15. Use native live/remap/geometry guards.
  These are Exit targets with stable passage labels, without a guessed room.
- Elevators hiku0000:29/30 and hiku2000:17/18 have entry4 prompt and entry2
  actual Talk/interact action. Mask14 (hex) exposes Events named Elevator up/down
  only while the native channel is enabled. Their callback runs the actual
  cutscene/layer transfer; navigation never activates or changes progression.
- Dylan/ffx_044 (26880 bytes, 64-byte header) has saved room194, Airship Bridge
  hiku0800. SaveData room offset BA, story BEC. Saves omit live worker enable
  flags/renderer state, so this establishes context rather than reproducing a
  running cutscene state. Do not confuse ATEL L0 map000A00 with SaveData A00.
- Macalania mcyt0500 worker44 entry4 PC77628 goes north to room106/entrance1
  (Macalania Hall); worker45 PC77722 goes south to room80/0 (Antechamber).
  mcyt0200 worker35 only enters the escape sequence at story1545..1552 and
  then advances to1557. The completion helper chooses worker44 for this bounded
  first story phase for Default callers. Explicit user goals now select the requested exit on repeat visits as detailed below. No direct story/warp writes.
- Overdrive renderer4D19F0 -> wrapper4DDF80 -> help getter38FC00 reads the
  independent command-record help offset8/12; name getter38FD40 reads offset0/4.
  Mode IDs1033..104F must decode the help field using central text services.
  Caller-qualified signatures resolve the otherwise identical name/help wrappers.
- Grid Status private native pool slots1..5, stride340; count1E, rows40/12,
  hidden byte4, kind8. Slot6 provides the actual shared Status heading. Native
  65A270/659800/658EC0 mutations own updates. Copy every displayed row into the
  virtual buffer, expose manual I, honor AutoDetail Off/On and preserve review.
- Polish glyph substitutions were reconstructed from the supplied package's
  actual font textures and encoded area dictionary. Indices:115Ą,116Ś,119Ć,
  121Ę,123Ł,128Ń,133Ż,134Ź,138ą,142ć,144ę,150ł,151ń,154ś,157ź,159ż.
  Ó/ó are unchanged. Automatic detection compares raw native area0/44 against
  stock/Polish fingerprints on the game thread; unknown results retry later.
  A persistent forced Polish setting bypasses detection. One encoding snapshot
  applies throughout each decode, including native area/exit names.
- Correction: modal namespace200000 is save/load, not Airship. Controller4B1580 phase10
  owns the visible two-option popup; renderer4B3810 draws menuText3/2000 title
  and2001/2002 choices, highlighting global1466224. Popup146621C is constructed
  by4B3C20 and retired through +41/clearing the pointer. Continue investigation
  and use mutation/ownership guards rather than independent UI polling.
- Native battle popup39FA20 kind5 selects sprite61 from C4249C; kind3 selects
  sprite59 MISS. Native79F680 later draws accepted entries. Installed icon
  atlases contain rendered Overkill wording; no KO word was found there.
  Keep existing committed KO producer unchanged. Atlas locale enum is native
  UI locale241290 (Japanese0,English1,French2,Spanish3,German4,Italian5,Korean9,
  Chinese10), not3852B0, which returns the saved English-text bit.

Retained reference: Docs/Scripts/Ghidra/output/FFX/decompile/layer*_*.txt;
Docs/Scripts/Offline/output/FFX/20261003_audit/maps_v3.json and original EBP files;
Docs/Scripts/Offline/output/reader_reference_20261003_235502/block_*.bin.
Fresh task scripts/evidence are staged in the Codex task work directory; copy
reusable outputs into Docs/Scripts/Offline before closing this task.

## Delegation and review

Sol owns behavior discovery, pointers/ownership, architecture, integration and
verification under ../../../codex/MODEL_POLICY.md (2026-10-12 revision).
Three bounded Luna medium handoffs used verified patterns/contracts:
Grid Status policy/buffer; exact Airship supplemental mappings; exact Polish
glyph table/persistent override. Returned edits integrated and reviewed; affected offline checks pass.
Sol corrected localized fixture expectations and preserved the original ASCII
prefix/stock Ç. Record final checks and any remaining issues here at completion.

## Validation status and remaining evidence

The source repairs and focused checks are complete, including the additional dungeon requests below. Canonical tester build, full native suite and guarded deployment are in progress. Final results will be recorded here. Gameplay acceptance remains user-side.

The historical exact field-freeze cause, Windows Application Control rejection of magic_0260, and separately reported manual Kilika Fire barrier target remain unconfirmed. This pass does not change Windows security policy or claim those reports solved. Current manual labels distinguish Wall glyph recess (31), Fire chamber wall recess (33) and lower recess (35); a live failure of the older Fire barrier label cannot be reproduced from saved context alone.

## Clarified Salvage Ship exit and floor evidence

### October 12 save and Discord evidence steering

The user explicitly requested reading the Salvage Ship save under Documents,
retrieving Dylan's latest Discord message attachment into the workspace Saves
folder, and using both for the two swimming-exit issues, including the newer
"no native map" report. Discord must be closed after retrieval. This steering
does not cancel the strict threading conversion or the other active repairs.
It does not authorize game launch/control/probes or Tester Logs access.

The actual local save directory is
`C:/Users/pmoen/OneDrive/Documents/Square Enix/FINAL FANTASY X&X-2 HD Remaster/FINAL FANTASY X`.
Bounded header/context reads identified `ffx_006` as the Salvage Ship save:
26,880 bytes, 64-byte header, current/saved room71, current area dictionary19,
story66, current spawn0 and saved spawn2. Header location19 agrees with
SaveData+E. Fahrenheit's retained FFX/savedata.cs and save.cs independently
document the offsets. A byte-identical copy and SHA256 provenance are retained
at `../../Saves/Own/SalvageShip-20261012/ffx_006` and `evidence.json`.
SHA256: `579c41b61285ea4c4f7d801de2259cd8647761c7c6d30fba4e4ece251caa7e59`.
The original was read only. This anchors the shared Deck/Surface script but
does not reproduce live swimmer motion, pointers, worker flags or floor state.

`nav_exits.cpp::announceCurrentMap` is the producer of "No native map name is
available." It is separate from collision-route failure speech. Its current
source already uses the primary field chunk+48 and the published area+E; the
room71 fallback is intentionally absent because Deck/Surface is ambiguous.
Do not invent a fixed room-name fallback. Dylan's attachment/context remains
required to identify the newer report precisely.

The separate “Review Salvage Ship Swim Exits” task initially stopped after
automatic approval review rejected its full conversation view. The user then
explicitly directed this active task to follow those original prompts. Here,
the task-scoped Discord navigation/Dylan view succeeded. The latest actual
swimming attachment, Dylan's14:10 message1559312377147686983, was downloaded
from its observed link as `../../Saves/Dylan/ViaPurifico-20261012/ffx_050`.
Its26,880 bytes/SHA256
`52103deb45b90e3b844ae2096149ad3a78f57bbde19c0e8400498919fe8d2069`
verify room209, dictionary244, saved room209/entrance1, story2220, and native
camera/map underwater flags1. `evidence.json` retains provenance without the
expiring signed URL. This is the exact stbv0000 swimming context already
covered by the priority repair, not another unknown map. Added its saved
context to the actual-script/floor fixture; fresh checks pass on both140-site profiles. The actual saved context is applied before boss/exit/save-sphere routing checks, while worker/actor runtime flags remain synthetic. See `Scripts/Offline/output/open_issue_repairs_20261012/save_review/verification.json` and its retained log.
Discord was closed with Alt+F4 afterward; no Discord process remained. Both
authorized saves have now been inspected offline. Own ffx_006 original/copy/hash
were reverified unchanged. No game was launched or save loaded into the game.
The strict threading task and other repairs remain active.

Continuation checkpoint: strict threading remains NONCONFORMING overall.
Preserve the completed owned filter/audio/settings/text helpers and all
existing edits. SDL3.4.4 exclusive controller ownership, input/interception
and speech/listener locks, remaining route/scan/text worker conversion, actual
Airship List/password qualification, final verification/build/deployment and
Tolk cleanup (now completed in the priority deployment) remain tracked in threading_audit_20261012.md and this
repair record. Save steering does not certify or replace that work.

The user clarified that this is one linked Exit to Salvage Ship - Underwater,
reached through a small underwater section of the shared Deck/Surface map.
Routing must note Dive first on Surface; listing it from Deck is acceptable.
The passage map contains the existing native exits to the ship and ruins.

The earlier neutral-horizontal Dive goal was incomplete: native83E5F0 calculates
the floor under the actor into +16C; 832E90 gravity2 caps actor Y against that
floor minus smoothed body clearance+4FC before applying vertical velocity+504.
83DE10 chooses the floor below the actor (least floor Y >= actor Y) when layers
overlap. Thus a Y>250 transition can require horizontal movement to deeper water.

Authorized offline graphics-asset reads reused the user's FFX asset permission
within this repair task. Exact cdsp00/cdsp01 mapout.vpa files were extracted, with
source names/hashes in output/open_issue_repairs_20261012/swim/manifest.json.
MAP1 offset18 points to the native ChMap header; version1203, group count+6,
scale float+C divided by10, relative vertices+18 and group descriptors+1C.
44BC0 relocates these pointers and83EA00 publishes group0 count+8/triangles+C.
Vertices are signed int16 X/Y/Z/padding,8 bytes; triangles6 int16 indices/adjacency
plus32-bit flags,16 bytes. The raw collision fixtures preserve exact bytes.

Surface has1739 triangles/959 vertices, scale47.246630859375, floor Y0..622.77.
At the retained Surface start X-192/Z79, triangle347 has floor Y171.38, which
cannot permit Y>250. Deeper triangles lie farther south (negative world Z).
Underwater has32 triangles with floor Y37.18..42.42, a distinct native mesh.
The new swimming_route clips each connected, natively passable Surface floor
triangle against252+actual body clearance, selects a reachable point in that
deep region, retains exact X/Z geometry, then appends the real Y251 objective.
The collision component/flags remain native; the projection only follows swim
height. Cached Surface geometry is reprojected at the player's current depth.
No extra barrier or guessed minimap coordinate is added.

The named exit is available on the shared Deck/Surface identity even before Dive
is ready. Instructions explain the prerequisite. Automatic movement requires
the real motion2/Dive-enable state and a valid route. It combines configured Dive
with ordinary horizontal steering; keyboard/controller manual input, cutscene,
dialogue, generation and progress cancellation retain priority. The beacon gives
horizontal guidance while approaching the deep region and stays silent for a
pure vertical endpoint. Actual readiness is separate from target availability.
Tests now exercise the real Surface mesh in both reader profiles, along with
before-Dive visibility, route rejection on a shallow floor and cached-height
handling. Focused route/reader tests pass; full verification and gameplay acceptance remain pending.

## Rendered Overkill producer

Registration39FC80 initializes one20-byte slot in the128-entry native number
pool. It records a new allocation lifetime, rather than speaking a queued word.
Renderer39F680 traverses the native active list after the reveal countdown and
screen-bound tests; for type2/glyph61 it calculates the displayed coordinates.
Accepted4999B0 sprite submission (nonnegative result and nonzero alpha), owned by
39F680, announces the first visible word for that allocation. Pool-full,
offscreen, delayed, unrelated and repeated animation submissions are silent.
New allocations, including identical words in a reused slot, remain new events.
There is no text-content deduplication. Existing Miss and committed KO producers
are unchanged. UI locale241290 and saved English-atlas bit385290 select exact
pixels reconstructed from icon atlases: OVERKILL/OVER KILL, VERNICHTEND,
Ultracidio, Exterminio. Unsupported locales do not guess wording.

## Address qualification and regression corrections

Native caller-qualified signatures must be emitted with owners before callees.
The generator now topologically orders them, including the native locale tail
jump. Runtime matching covers the entire bounded body within the permitted
start window and refuses missing owners transactionally. This caught an
implementation integration issue in offline startup checks before deployment.
The Macalania fixture also needed six worker slots for its now-distinct first
and revisit exits; independent state setup isolates its five story boundaries.
New fixture windows required a larger isolated memory range and array writes
through qualified bases. These corrections affect tests or signature integration,
and are not evidence of a reproduced game crash.

## Additional tester scope: 2026-10-12

Kilika **autosolve**, not autoskip, stops after the mod says “Destruction Sphere placed.” Trace the exact treasure continuation and scheduler completion. Via Purifico needs its required exit plus state-aware manual and automatic treasure-panel guidance. Macalania must offer a trial-completion versus fayth-access goal; the earlier story-boundary correction alone does not satisfy this repeat-visit request. These additions are implemented with offline tests; no gameplay claim.

## Kilika, Via Purifico and Macalania additional architecture

Kilika's old continuation 52179 follows sphere-fire fade and a message sequence; it does **not** open wall54 or make the treasure chamber reachable. Correct entry35/2 at53283 skips only a text wait whose message the substituted interaction never opened, then keeps original sound/timers, player/camera requests5/21 and5/22, map layers14/15, wall54 opening, sphere-control removal0/1 and RET55427. The generator and native contract agree. `kilika_continuation_proof.json` retains the traced path. Items and collection bits remain native.

Via land is **bvyt0900**, length37649/FNV97fd6083/SHA256 in retained flow. Its exit is worker30 **entry6**, an inside-region progression scene (not the no-op entry4). Original Common010C -> native8584B0 ->86FED0 transitions to room209/0; native flags/geometry remain authoritative. Water is stbv0000 length19861/FNVada03b86: worker26 entry4 -> room208/0. New supplemental catalog preserves both real scripts' exact identities, useful callbacks and progression. No script trigger is auto-activated by discovery.

Land chests11..17 call obtainTreasure(1,104..110), then native byte declaration6 records mask1..64. Manual guidance numbers chests without disclosing contents. Accessible chests1,2,6,7; Talk42 prepares the relocation glyph (byteA=1); crossing43 completes relocation (byteB=1). After relocation, byte9 node0..5 names northwest/north/west/south/east/sealed-west panel; byte8 cycles1north,2west,3south,4east,0none. Worker28/6 sets the direction before its corresponding visible light for30 frames. ByteD pauses cycling during warps. A guide cue is a mod-side timing instruction at the desired native direction edge only while near45 units, field controls available and the guide selected. It does not read undisplayed item names. Same-value writes are not new direction events; distinct cycles retain cues.

Manual chamber routing: node0 worker36 west->5; node1 worker38 west->0; node2 worker40 north->0; node3 worker37 north->1; node4 worker39 west->2. Node5 worker41 east->0 returns. Guide paths stop25 units beside the circle; entering too early selects the wrong native destination. Seven actual chest coordinates, current activation/panel/collection state and the actual live collision mesh supply routes. No minimap guess or invented obstacle is used.

The explicit **Collect all treasures automatically** action runs each remaining original chest entry2, waits for its native return and confirms its collection bit before requesting another. It bypasses manual travel, like the existing trial solver; it never writes treasure bits, grants items directly, activates the relocation glyphs, exits the dungeon or suppresses game dialogue. Collection can be turned off between chests; an already-running chest finishes naturally. Exact script identity, generation, native request reservation, field/focus/UI guards and interruption checks are shared with trial utilities. `via_chest_proof.json` records each original grant/commit. This automatic action is within the user's explicit request for a collect-all button; gameplay remains unconfirmed.

Macalania now asks for a goal **after** returning the substituted original interaction and before requesting any treasure script. The captured three-row menu chooses Complete trial and leave, Visit the fayth, or Cancel; central keyboard/controller menu gates, speech capture and neutral drain apply. Cancel returns without starting a script; map/battle changes invalidate it. The selected goal is per solver activation, not a persistent story override. Visit the fayth requests original south exit45/4 to room80/0 (antechamber, native fayth door still owns access). Complete uses original north exit44/4 to room106/1 from story1545 onward, including repeat visits; earlier first completion takes south45/4. Native scripts own transitions and progression; no story value is changed. Generic completion contracts retain their bounded Default behavior for existing callers. Goal-specific requests select explicitly rather than guessing intent from first/repeat visits.

Additional Luna tracking: `via_exit_catalog` completed two known records and matching expectations; primary reviewed and is validating actual EBP visibility. `via_treasure_data` did not write a file because its local filesystem access denied D:; its summary also used a wrong continuation field. Primary took over and created/reviewed the verified records with pc=entryPc. No repair teaching loop or Astra use. Both outcomes remain honestly recorded.

Real Via EBP fixtures found that `navigationExitName` replaced unknown supplemental labels with worker+1. The central helper now preserves a validated authored numeric Exit label when the destination record is absent; known destination records keep their established numbering, unknown direct scripts keep their fallback, and localized native destination names still append normally. Actual enabled/disabled Via callbacks are tested alongside all18 Airship passages.

Inside callback evidence: Common007F resolves85C270 and sets worker+34 bits6000; native866C60 dispatches rectangle channel5 on entry. 876360 maps default zone channel5 to entry6; 876470 requires live worker+AB bit20 before requesting it. Via land catalog therefore selects entry6 and respects bit20, whereas the water exit uses boundary channel4/bit10. The retained native function bodies confirm this independently of the synthetic fixture.

Deployment preflight initially stopped before any file replacement because its Windows PowerShell environment could not autoload Get-FileHash. The deployment guard now hashes actual files through a scoped .NET SHA256 helper with disposed streams/algorithm; no module installation or environment/security setting is changed. Final guard results follow.

## User follow-ups after the first guarded deployment

The first maintenance tester build was fully checked and guarded-deployed with
all13 old installed artifacts verified; the user then reproduced two failures.
The new repairs below are source/offline-verified and await the final Prism build
and deployment. Do not call these gameplay-confirmed.

### Shared field identity and Salvage Surface

The screenshot is the submerged region beside the salvage ship in cdsp0000.
Own-session log reproduced Exit0 and unavailable current map name. Previous
fixtures wrongly populated controller+1F8 as the field chunk. Actual native
86EDA0 assigns +48 to its primary field chunk and clears +1F8; 875490 uses that
loader for ordinary fields, while875650 uses the auxiliary pointer for separately
loaded event scripts. 470E90 obtains the primary field chunk via01325BA8 (+48).
nav_exits, activeField and swimming_goal now use +48. The real EBP fixture leaves
+1F8 zero and verifies active field, localized Surface name, the Underwater exit,
and the original1739-triangle southward deep-water route. Both135-site reader
profiles pass (work/live-followup-readers.log in current Codex task).

Luna gpt-6-luna medium prepared12 mechanical substitutions and the zero-auxiliary
fixture, without writing D source. Sol reviewed/applied and verified native bodies
and both reader profiles. No remaining implementation blocker in that handoff;
user-side gameplay check remains.

### Grid Status and current input device

Native654B40/6459E0 construct five information lists for the selected character:
attributes; Special(category41); Skills(category40); White Magic45; Black Magic44.
65A2E0 hides unlearned commands using that character's saved learned bits. These
are not separate locked party member slots. Slot6 is a Display Status button
hint, not a Status title. Its use as the heading was an earlier reader error.
The reader now describes the current list and explicitly identifies blank learned
ability pages. I/full review and AutoDetail Off/On retain their contracts.

controllerOwned indicates SDL device ownership, not the user's active device.
input_device.hpp records actual physical keyboard presses or controller edges/
analog engagement. A connected neutral/held controller cannot switch keyboard
control names. Glyphs, Dive/Auto walk readiness, pedestal and lightning control
instructions use that central choice; the DirectInput/XInput bridges retain
ownership semantics. Both reader profiles pass connected-pad keyboard naming,
remaps/alternates, every populated page, and a fully hidden ability list. Input,
controller/Auto walk/pedestal/lightning checks remain to run.

### Prism, intro prompt, equipment and performance — active work

The user requested Prism migration following the October12 draft, marking FFX
migrated, redeployment, and removal of installed Tolk after successful migration.
The central adapter and queue source are implemented, including UTF8/combined
speech+braille, intentional braille-only, capability gates, copied choices and
owner-thread selection. Launcher links the same adapter. A pinned x86 Prism
package has been built separately from the mod graph from upstream commit
b68e9c94240cefb1f08b2306c26376e3ab300a7a (0.18.3). Runtime DLL SHA256:
1aa76c8835963eccd210255b06b4db48551bd174dc965d205c9f68ae0c726885.
MSVC CMake C23 metadata failed; ClangCL was unavailable. A task-local copy uses
C17 for the bundled/generated C translation units; C++23 and all runtime sources
are unchanged. Successful x86 MSVC14.44 build and PE/export inspection confirm
undecorated C exports. Exact package lock/installer/receipt, dependency-separated
normal deploy/release installer, API failure/teardown checks and final builds are
still pending. Do NOT mark migration complete or remove Tolk yet.

Luna gpt-6-luna medium prepared a fake Prism library and bounded existing queue
fixture transformation. Sol reviewed, corrected its exported-header PRISM_BUILDING
annotation, integrated and ran --speech: passed existing400 distinct equal-text
concurrent events, copied text, ordering, braille-only, cancellation/backpressure
and producer policy checks. Further backend capability/failure/teardown checks
remain. No game or real screen-reader test was run.

User also requested a pre-title intro prompt. Offline movie investigation is in
progress; do not announce arbitrary story movie text or guess a native prompt.
Mod-side instructions may use verified skip controls. No prompt is implemented
until its native startup/playback context and controls are established.

Equipment follow-up: tester's last Armor inventory item speaks Longsword; user's
Rikku slot is blank and armor inventory speaks Tidus's Brotherhood. Native8D0140
skips a zero table entry at1197730+row*4, and native equipment commit8CEFF0 maps
zero to FF (unequip). Current listKey reads zero as equipment record0, yielding a
wrong weapon name. Fix/test this sentinel; inspect Rikku's equipped-slot producer
and native name/empty label. 4A97D0/4A9C20 native instructions MOVZX EAX from saved
armor/weapon bytes (the Ghidra narrow return type is NOT an upper-EAX garbage
bug). No equipment repair implemented yet. Evidence in task work/native_evidence.

Memory follow-up: installed FFX.exe is x86, COFF flags0122 and LAA already set;
SHA2560537b2a1047f3266e73495cd4e35f63f0777f4231d417699f979954686da686d.
Will and launcher flags0102 are not LAA, which does not limit FFX's separate
process. Microsoft documents4GB virtual address space for LAA x86 on64-bit Windows,
not4GB reserved RAM. User then explicitly requested launcher check/enable-if-missing:
implement a backed-up verified flag-only change before FFX launch for supported
executables, leaving already-enabled copies untouched. This is newly authorized;
no executable has been modified. Revisit proxy/hooks performance/thread integration;
own-session queue timings alone do not establish the tester lag cause. No game
probe/launch/Tester Logs access is authorized.

### Prism follow-up review and actual dependency installation

Pinned x86 Prism package `prism-0.18.3-b68e9c9-windows-x86-msvc-c17-1aa76c88`, native DLL SHA1aa76c8835963eccd210255b06b4db48551bd174dc965d205c9f68ae0c726885. Its imports are Windows system libraries only; Tolk support DLLs are not carried over. Immutable 15-file runtime/license/provenance bundle is under `D:/Games/Dev/codex/prism/packages/<package_id>/runtime`; source lock is prism.lock.json. Installed separately to game-root PrismRuntime; receipt identifies both FFX-native/FFX-launcher and canonical destination. A second explicit install preserved all16 files' hashes/timestamps (including receipt), zero writes. Normal deployment only checks it read-only and owns7 mod payloads; source lock is an embedded noncopied installer resource, total11 resources. Migration scanner excludes owned runtime/backup directories and preserves retired speech libraries for explicit separate cleanup.

Primary x86 speech checks pass with fake ABI: strict UTF8 Polish/supplementary characters, invalid surrogate rejection, initialized automatic/explicit engines, unavailable/capability/init failure handling, combined/fallback/braille-only channels, same-thread lifecycle/free-before-shutdown, stop, repeated valid events, existing queue/order/generation/nonblocking backend cancellation checks. Backend settings fixture passes including SAPI selection/details/Back and existing controller-routing checks. Actual C# installer source compiles warnings-as-errors;787 synthetic assertions pass, embedded7-file mod installs twice at ordinary/350-character paths preserve Prism runtime hashes/timestamps. Missing/modified receipts refuse without writes; corrupt catalog fixture now includes its lock and validates the intended refusal. No real screen reader, game or launcher has been executed.

Luna tracking: fixture updates accepted after Sol corrected parent-menu navigation and tested. ABI fixture accepted after Sol fixed corrupted non-ASCII literals, absolute DLL fixture path and allocator declarations, then ran actual x86 checks. Dependency-script draft initially passed its own PowerShell7 fixture but invented an extra Game subfolder and newer framework APIs; Sol took over, corrected destination/PowerShell5 API and identity checks, and verified both scratch cases and actual idempotent install. C# prerequisite draft had inconclusive agent compilation; Sol reviewed/integrated BOM/manifest/root checks and verified actual x86 .NET installer source. Release-fixture draft accepted after Sol added its missing namespace and repaired negative-test lock provenance; actual installer checks pass. No Astra used. Mod rebuild/launcher fixture/guarded deployment and requested Tolk removal remain pending until strict threading restructuring and remaining features are ready.

### Additional scope and strict audit

The user requested Monster Arena menu speech plus Airship password keyboard and coordinate search. Coordinate reading and an optional locked-destination picker need native render/control evidence; the user's proposed coordinate accessibility exception does not silently approve unrelated hidden names/passcodes. Prepare exact proposed hidden output/context for a decision where required. Discovery remains with Sol. Monster Arena native/asset/render evidence and the new strict thread-boundary audit are authoritative in [threading audit](threading_audit_20261012.md). Do not claim performance completion from zero-ms speech logs or existing LAA.

Armor terminal-row repair is implemented: native zero inventory row means Unequip armor; native FF equipped slot is explicitly named, and valid empty names log character/slot/index instead of remaining blank. Native getters are byte returns with MOVZX EAX (not garbage high bits). Last-row fixture exists; new focused Rikku/empty-slot coverage and final reader rerun remain pending.


## Accepted renderer capture and corrected airship identity

Monster Arena now captures at the native 36F960 renderer boundary after 376A50
expands conditional programs. Kind8 and11, room307/window11, current focus,
actual renderer parent, output buffer and non-mutating arguments qualify each
capture. The draw flag controls decoration, not visibility; 774B60==0 is the
renderer acceptance predicate. The buffer is 0x4000 bytes; at most three fixed
4096-byte prefixes plus copied binding/encoding values enter a 32-slot queue.
No native pointers enter the new copied-text worker. Copied title precedes
focus; copied controls form manual I/Autodetail help. Selection revision and
resource generation reject retired frames. outputAt preserves the capture's
generation across speech enqueue. Both136-hook reader profiles pass, including
post-return buffer reuse, equal labels on separate selection events, update/
unrendered guards, manual help, Autodetail and cancellation. New gameplay and
deployment remain pending; existing legacy reader/speech locks still need the
broader strict audit conversion.

Sol discovered a mistaken earlier Airship mapping: 40200000 is save/load at
save spheres. 4B1580 writes actual save data/records; 4B4DF0 returns 64-byte
save headers and 4B5510 reads the saved Tidus name at+20. The old fixture supplied
Airship/List/Input strings, so passing that fixture did not establish Airship
coverage. The useful reader is retained as save_load_menu.cpp with corrected
names and documentation; its fixture remains a synthetic getter contract.

The actual Airship map/list/search is hiku2100, bytecode SHA256
db7c1b83da25505b20873f81a8a8b35ebc02cab16c1db51fa01c84faab4408dc.
Worker9 owns list/main selection and invokes password naming through40080020.
Worker2 entry6 reads its own native position into float declarations49..51 at
05A8, then displays X/Y/Z digits as splash sprites011B, including fractional
digits and the Y sign. Coordinates are displayed graphics, so reading them
does not need a hidden-text exception. Search active declaration39 is015248;
six genuine search-zone workers34..39 perform normal native Confirm and set
discovery bits with displayed success messages. Hidden picker names/regions
await the user's exact scoped decision; no disclosure/bypass is enabled.

Native table addresses in the annotation TSV are RVAs, not absolute VAs.
Retained reference confirms dispatch pointer+0C in each16-byte record for these
calls: Map801D->91B400->91A930 (layer texture), Map800F->91B1A0->91A830
(show layer), Common00A3->856CC0->86A280 (worker position), Common01D2->85FFC0
(password comparison). Layer texture+96 and hidden bit0 are in C0-byte records
through193BF80. Continue tracing accepted drawing; mutation alone is not proof
that a texture has reached display.

Luna tracking: collision snapshot helper returned, Sol corrected header-only
integration and reviewed native aliases/actor gates; x86 fixtures pass. Pure
owned decoder returned with portable tests, Sol reviewed byte guards and ran
x86 W4/WX checks successfully. Nonblocking snapshot/queue helpers returned
without a compiler run from Luna; Sol reviewed memory ordering, then compiled
and ran x86 W4/WX concurrent coherence/FIFO/lifetime/equal-event checks. These
bounded helper scopes are accepted; they do not establish whole-mod threading
conformance. LAA header codec returned and passed its x86 mutation/idempotence/
invalid-format tests; launcher transaction integration remains Sol's work.

Updated SDL instruction read: canonical references/sdl-worker-ownership.md.
Apply its exclusive Windows SDL3.4.4 owner/readiness/copied-state design only
after this mod's packaged version and host ownership are verified. Existing
input-driven SDL initialization/pumping has not yet been converted.

Native boundary correction during integration: machine83E4F7/83E518 compare
(group-1) to3E and use unsigned JA/JBE, proving groups1..63 only. Group64
is always passable and does not read remap64, despite an unreachable decompiler
case3F label. Sol corrected the previously supplied 1..64 criteria/helper and
added exact-boundary/15-alias-chain fixtures before deployment. Native82D8E0
returns1300740, confirming controlled-actor comparison. readMesh now consumes
a 64-byte owned remap snapshot plus actor flags/control identity instead of
calling the native checker once per triangle. Geometry pointer checks now allow
readable high addresses and reject exact poison/overflow; moving geometry/route
work off native callbacks remains pending.

User decision received: explicitly enabled the proposed six-destination picker
(Baaj Temple, Omega Ruins, Besaid Falls, Mi'ihen Ruins, Battle Site, Sanubia Sands),
using native movement/search confirmation. This approval covers that optional
discovery bypass, not hidden passwords or unrelated game assets. The subsequent
user instruction requires a persistent default-Off Navigation toggle named
"Enable hidden destinations menu for airship map". The setting is appended to
preserve existing setting IDs and exposes an independent On/Off gate. Picker
shortcuts/menu entry must additionally require the current native search context;
implementation remains pending and must not become available merely because
permission was granted.
