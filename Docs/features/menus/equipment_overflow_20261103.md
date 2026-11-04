# Equipment capacity management — November3

Implementation: **complete / Closed** for the documented native results-owned
reader. Combined offline checks, warning-free build, exact-payload Defender scan
and guarded deployment pass. Gameplay is unconfirmed.
The [single scope ledger](../../plan.md#november3-blitzball-setup-and-equipment-overflow--scope-ledger)
also covers the requested Blitzball corrections. The pasted tester report is the
only tester evidence used; no Tester Logs or tester/personal saves were accessed.

## Behavior and controls

When equipment exceeds capacity after a reward, announce the game's current
warning/instructions, the focused Keep or Discard list and selected equipment.
Native cursor movement and replacement-target selection remain the game controls.
The reader follows the effective opposite list during a replacement selection;
it does not assume the stored original-side cursor remains the current item.
Equipped markers, empty cells and current record changes are announced. Optional
Menu indexing uses the actual selected position and native list capacity.

I reads the selected equipment modifiers using the ordinary selection-detail
service. Autodetail controls automatic modifier reading independently; manual I
continues to work with it On or Off. Native capacity/discard warnings announce
on their producer transitions, including a later repetition of the same warning.
There is no automatic discard, swap, confirmation, input injection or save edit.
The native blocked/full-screen flow continues until the player resolves it.

The two native headings are image labels. Their mod-owned equivalents, Empty and
Equipped have Japanese, English, French, Spanish, German, Italian, Chinese and
Korean strings selected by the native UI locale getter. Prompts, names and
modifiers use existing native language getters/decoding, including the separate
Polish fan-translation decoder. This does not add official Polish support or
resolve the existing whole-mod manual language-override/localization gap. Shared
equipment-detail fallback wording retains its documented current behavior.

## Native contract and source locators

Retained original game implementations remain outside every Git tree in
Analysis/FFX/RepositoryEvidence/ghidra-output/FFX/decompile/layer2_game_logic.txt.
Raw x86 reference blocks are retained under
offline-output/reader_reference_20261003_235502. The focused external copies and
operand-analysis script are in Analysis/FFX/SetupOverflow_20261103. No new installed
asset investigation or live process/probe was needed.

| Native location, reference image VA | Verified behavior |
|---|---|
| 8BDB30, two cdecl arguments | Results initialization; second argument1 enters capacity management directly. Initializes both cursors, replacement state, inventory table and reward records. Existing results reader distinguishes this from a new AP/reward episode. |
| 8BA620, no arguments | Results input/advance owner. Exact jump table: states20/21 browse/scroll,25/26 replacement target/scroll,23/29 warnings;1C..1F introduction,2A/2B exit. AP/gil phases remain the existing producer. |
| 8BCFE0 | Detail renderer selects original side XOR replacement-active, then obtains the real record and its native modifier slots. |
| 8BD240 /8BD630 | Left inventory and right six-cell reward/discard list renderers. Left table uses current top plus left cursor; right uses its independent cursor. |
| 7ABD30 /7ABBF0 | Native table enumerates up to200 inventory slots. Native record getter maps inventory/battle/reward handles; out-of-range indices silently alias record0, while FF returns null. The reader rejects malformed aliases and treats FF as an empty cell. |
| 8BE070 /7ABA10 | Original record swap; native code alone changes inventory/reward contents. A row/handle can retain its identity while its22-byte record changes. |
| 8BDA20 /8BDFA0 | Native localized instruction lookup and current prompt transition. Reader copies the current prompt pointers' text while still on their native owner. |
| 8BC0C0 /8B6C60 /9016B0 | Lower warning measures/draws using native message window7. Equipment-name substitutions are resolved through the existing message_text.cpp capture, not omitted by the raw encoded-text decoder. |

RVA146A148 owns count/top/cursors/replacement state;21CC580 owns the left table;
21CC590 is the current lower instruction pointer and its preceding eight bytes
locate the top instruction pointer. RVA1F10F8E owns the six reward handles.
New profile operand rows derive these addresses from the already-qualified
4BA620/4BDB30 bodies. Independent duplicate operands must agree; no address-shift
guess or new native hook is introduced. Reference/installed resolution remains a
required verification step.

## Architecture, ownership and bounds

`battle_results.cpp::opening`, `advanced` and `resetBattleResults` call the three
entry points in `equipment_overflow.cpp/.hpp`. Its phase observation follows
closure of the AP/gil review so that old review-arrow ownership cannot erase the
new selected detail. Existing native arrows and controller input remain native;
the overflow screen does not install review-arrow capture.

The native owner qualifies a current results episode/epoch and supported input
phase, validates count1..200, both cursors0..5, side/replacement flags0..1 and the
selected table position, then resolves just one handle/record. There is no table
scan. A32-word selection key includes side, row, handle, replacement/warning
state, prompt identities, table/count and the copied22-byte record. An unchanged
key/current detail token exits without text capture; a new selection with the
same wording remains an event. This is a local native identity guard, not
text-content deduplication.

Changed events build one bounded ready TextPlan, including selected name,
equipped status and modifier detail. Decoding, joining, numeric formatting,
speech/braille and I's retained string belong to the existing preparation/output
services. Native pointers are never dereferenced by a worker; no wait, allocation
loop, timer, logging or file I/O is added to this callback. TextPlan uses its
existing128-node/114688-byte and shared queue bounds. Prompt capture reuses
planMessage's window7 native substitution getter, depth8,4KiB source and16KiB
total-token bounds per prompt; no native string/pointer is passed to preparation.

The reader retains only native scalars, its episode epoch and selection-detail
token. Exit, reset, changed epoch, invalid native state/count/handle or refused
capture retires only its own detail. A different overlay's detail token is not
cleared. Returning after another context invalidates the old token recaptures
the current selection even when its row is unchanged. No native write occurs.

## Checks and safe extension

`tests/equipment_overflow_followups.inc` drives the actual installed result hooks,
TextPlan/selection-detail services and I/Autodetail through the source-established
native fields. Controlled getters/state transitions cover normal result entry and
standalone capacity entry, both automatic modes, both lists, opposite-side target,
repeated warnings, same-handle record mutation, equipped/empty cells, FF/malformed
handles, maximum inventory position, menu index and eight labels. A window7
warning substitutes a different equipment name from the current selection and
must retain that native name. Idle callback
pressure must create no speech backlog. The complete reader host runs against
both supported profiles; profile checks and canonical preflights separately
validate instruction-derived addresses. Native controller/input/render/audio
dependencies remain simulated; this is not an in-game responsiveness claim.

Add a supported screen phase only from the original native switch/render contract.
Preserve effective side inversion, fixed native capacities and the getter's
malformed-handle distinction. Keep this reader on the existing results owner and
single description service. Recheck focused fixtures, both reader/profile paths,
build/preflight, cumulative native/owned-output cost and final payload security
before deployment. Actual discard/input flow, progression and audible/frame
response require the user's gameplay confirmation.

## Final checkpoint

Both complete173-hook reader profiles pass, including the window7 warning whose
inserted equipment name differs from the current selection. Across45 changed
episodes, Native callback aggregates are0.745/1.012ms, maximum0.028/0.042ms, and
capture-to-owned-output aggregates18.024/17.711ms.1000 idle Native callbacks total
0.923/2.164ms versus0.287/0.832ms with this reader disabled, with no speech backlog.
The host's complete fixture/drain loop totals201/253ms; that instrumentation cost
is separate from Native observation and does not establish game responsiveness.
These are job-throttled controlled component measurements, not actual game/frame,
hardware, PRISM or audible latency. The user's cumulative25ms application-lag
limit remains applicable to gameplay.

All24 retained Blitz resource availability cases/nine activity matrices, shared
utility developer/release regressions, native profile operands, menu/settings,
warning-free canonical release-flavor build and both actual compiled deployment
preflights pass. All seven scanned/staged/installed file hashes match; existing
INI bytes and locked PRISM runtime are preserved. Defender reports no detection
on the exact payload. Trusted signing remains unavailable; no universal scanner
acceptance or download-reputation result is inferred.

Final hashes, source-matched coverage, resource receipts, scan and deployment
records: Analysis/FFX/SetupOverflow_20261103/verification.json. This closes the
supported reader boundary; further redesign requires a human follow-up. No game
launch/control/probe, Tester Logs, saves, remote write, installer/publication or
delegation was performed. The [short gameplay checks](../verification.md#blitzball-and-capacity-menu--november3-priority-when-reachable)
are progression-dependent and do not require filling inventory for testing.
