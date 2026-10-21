# Airship destination images, confirmation and free search

October19: user approved reading installed airship graphics offline. The later
[own-log/password follow-up](airship_password_20261019.md) supersedes the initial
script-dependent capture gate after the user reported the list still silent. This is
separate from game execution/control, which remains unapproved. The original
October17 save remains unmodified in the parent workspace's
`Saves/Dylan/Airship_20261017/ffx_071`. The original silent-list report and its
Discord provenance are in [Dylan follow-up](../../dylan_reports_20261018.md).

## Main destination list

`airship_destinations.cpp` is the single image-row producer. It is installed and
reset through `airship_search.cpp`, independently of that module's free-search
coordinates/optional steering. Native TK already supplies Select destination,
New location and confirmation text; this producer supplies only the focused
image label or highlighted confirmation choice. There is no password/unlock or full destination dump, extra list control,
OCR worker, new setting or game state mutation.

Original `hiku2100` W9 uses selected index at map15100, last index15124, scroll
151C0, and a31-element destination-ID array represented by binding5D000034.
The textual model's map offsets are not permission to treat the indirect array
as flat memory. Optional absolute-index metadata uses native86C500 after exact
prepared/live declaration verification. Metadata bindings: index7/5C000008 selected;
16/5C00002C last;25/5C0000C8 scroll;18/5D000034 high31 ID array;
29/5C0000E8 phase;10/5C000014 cursorX;11/5C000018 cursorY. Phase1 is selection;
phase2 is confirmation. Selected indices0..30 are bounded before the getter.

**Cursor layer26** is updated at original PC3701 and shown at3707. Layer10 is
the list background, not the focus cursor; the earlier informal interpretation
was wrong. Visible labels are layers11..25. The displayed row comes directly
from accepted cursor Y=84+18*row, constrained to0..14 with the original room382
top/bottom one-pixel adjustment. The shown row's texture ID1..26 supplies its label.
Blank0 and unknown IDs are refused. Unselected/future labels are never emitted.

The complete original native helpers establish accepted layer state:
91A3D0 returns the bounded native table count;91A830 clears hidden bit0 and calls
the Phyre component enable routes;91A7C0 sets hidden bit0 and disables those
components;91AB30 updates stored position after its native component translation.
The table is owned through193BF80, two object links and aC0-byte layer record;
texture variant is WORD+96, XY are integers+C/+10. These pointers stay on the
native owner. Hooks at RVAs51A830/51AB30 observe accepted show/position returns;
51A7C0 retires output when cursor/background/selected row is hidden. All three
full bodies and data operands are qualified by generated profile signatures.
Original91AB30 explicitly compares current map01330804 with382; the new
`airship_map.hpp` reads that resolved operand on the native owner. It binds the
catalog and field-cue exclusion to the actual airship map.
This traces native visibility/position, not a new GPU framebuffer/culling probe.

Capture requires map382, focus, no game/mod/console overlay, native layer
count48..128, shown background10/cursor26/selected row and a verified texture ID.
It does not require active W9 or a ready prepared script. Those facts qualify
only optional absolute indexing: primary `hiku2100`, exact length24785/
FNVde956a03/41 workers/codeE6C, current allocation/header/declarations, phase1
and matching selected ID/XY. If unavailable, speak the shown label without a
guessed index. No callback hashes/parses a script, allocates a label string or
waits for preparation. The original script hides10/rows/26 before confirmation
and uses layer29 there; closure therefore follows actual native visibility.

The32-byte POD packet goes through existing bounded speech domain14. Preparation
owns string construction, optional menu indexing and central PRISM output. Native
focus identity (verified absolute index or displayed row, ID, language and native
layer-table lifetime) is the local event
guard; equal labels on different rows remain separate. Opening appends after the
native prompt; later focus uses normal selection interruption. Queue refusal
leaves the focus uncommitted for a later genuine native cursor event. Close/reset
changes an atomic revision; queued old-lifetime output is rejected. Preparation
never follows the stored engine pointers or enters native getters.

## Proceed confirmation options

Latest October19 user correction: this may be the wrong prompt. The reported
silent confirmation options remain unresolved; the following evidence proves
only the implemented layer29 path, not that it addresses the user's prompt.
Further investigation/repair is deferred at the user's request at1% usage.
Identify the actual reported prompt first when work is explicitly resumed.

The October19 user report establishes that the destination list works but the
following Proceed? choices were silent. This is an image selector, separate from
`confirmations.cpp`'s ordinary native window/option-pointer route. Original
`hiku2100` W9 hides10/rows/26, displays field string6 (Proceed?), shows layer29
at PC38C4, waits one scheduler pass, then sets its texture at39E3/39EF. Variant0
highlights Yes and1 No;1 returns to the list,0 proceeds. PC3A7B hides29.

The same `airship_destinations.cpp` producer now arms a separate confirmation
lifetime on accepted51A830 show29. It deliberately emits nothing at show: the
stored texture may still highlight No from the previous opening. Accepted
51A930 texture return supplies the actual WORD+96 variant0/1 after native write.
Complete60-byte original91A930 calls91A3D0 and stores into the sharedC0-byte layer
record. Its full signature, dependencies and original-body replay are retained.

Capture requires the armed epoch and same native layer-table owner, map382,
foreground/no overlay, count48..128, shown29 and hidden list background10/cursor26.
It requires no prepared script or active W9. The first accepted choice appends
after the native prompt; a later different option/locale interrupts. The local
guard compares native option and language, so Yes/No/Yes speaks all three.
Show/hide/reset retire separate atomic confirmation revisions; list closure
cannot cancel the new option. Unknown variants or changed ownership retire the
lifetime. A refused queue push leaves selection uncommitted for a later native
texture event. Capture performs fixed bounded reads and copies the same32-byte
scalar packet; worker label construction/output uses no engine pointers.

Exact layer29 variants0/1 for maps21..30 (20 files,2,675,040 bytes) are retained
under `Scripts/Offline/output/airship_confirmation_20261019`, with archive paths,
source and pixel hashes and a contact sheet. Sol visually checked both highlight
states after native UV vertical orientation. Native map27's English choices are
independently verified. The catalog's `confirmation_by_map` contains Japanese,
English, German, French, Italian, Spanish, Korean and both Chinese options;
native locale mapping/default follows6403C0. Spanish periods are retained.

The reader fixture executes the original60-byte setter before real capture and
consumer. It verifies both options in all native locales/default, first append,
movement interruption, repeated selection events, reopen stale-text avoidance,
no worker getters, unprepared/different worker, hidden/reset/focus/map/owner guards
and independent list/confirmation retirement. Both supported reader profiles are
required. No game was launched; live prompt timing and highlight integration
remain user-side: select a destination, hear Proceed? then Yes, move to No/back
to Yes, cancel and reopen. No must not speak briefly on reopening before Yes.

## Language graphics and transcription

Complete native346-byte6403C0 substitutes the graphics map by UI language:

| Native language value | Actual image map |
|---|---|
| default/0 | hiku21, Japanese |
| 1 | hiku22, English |
| 2 | hiku24, French |
| 3 | hiku26, Spanish |
| 4 | hiku23, German |
| 5 | hiku25, Italian |
| 9 | hiku29, Korean |
| 10 | hiku28, traditional Chinese |
| 11 | hiku30, simplified Chinese |
| 18 | hiku27, English images |

Original authorized extraction retains874 narrowly selected assets/84,751,171
bytes with per-file hashes in `Scripts/Offline/output/airship_assets_20261019`
relative to Docs. Only hiku21/hiku2100 plus localized layer11 variants/headers
were read. Native Phyre UV orientation requires a vertical flip for inspection.
All26 nonblank decoded map27 buffers match map22 byte for byte; blank dimensions
differ. The compiled catalog includes all nine distinct label sets. Examples:
English IDs2/18/26 are Besaid Island/Highbridge/Penance; French18/26 are
Bevelle/Der Richter. These differences must not be replaced by guessed area names.

`airship_destinations_labels.json` is the reviewed transcription authority;
`build_airship_labels_20261019.py` writes ASCII-escaped wide-string data and a hash
proof. It validates27 entries per map, blank0, complete1..26 and bounded lengths.
Catalog mapping follows the native graphics loader, not an assumed language enum.
The native UI language getter is captured on the owner. Fan Polish decoding is
separate; no unverified translated image catalog or general mod UI translation
support is claimed.

## Extension and verification

Change a label only after inspecting its actual locale/variant. Keep the native
map substitution, script/variable identities, layer visibility/coordinates,
catalog generator, profile dependencies and tests synchronized. A new map/script
variant requires its own original evidence; do not relax the checksum or publish
the full list to work around an unknown focus. Keep free-search steering separate.

`airship_destinations_followups.inc` replays the original29-byte count getter
and142-byte cursor function before production capture/consumer, including all15
rows and the native edge-pixel correction. The retained EBP and real immutable
resource/declaration service supply optional indexing. Phyre translation, SDK
getters, layer records and the reader-host queue remain modeled boundaries.
It checks initial/change/scroll/equal-label events, each catalog ID/locale,
invisible/invalid rows, actual-image versus inconsistent metadata, absent current
W9, map/focus/confirmation visibility and queued close/reset.
Run `run_tests.bat --readers` for both supported profiles, the label generator,
signature audit, canonical native build and guarded deployment. Actual in-game
show/scroll integration, screen-reader prompt timing and audible response remain
user-side confirmation. The evidence receipt records actual passed results.

The free-search M coordinate reader and default-Off six-region chooser retain
their implementation and tests described in [Puzzles](index.md#airship).
