# Wakka choices, short beeps and highlight review — 2026-10-08

Sol investigated, implemented, reviewed and tested this follow-up alone. The
user explicitly authorized retrying the previously rejected offline extraction.
There is no access question pending. No game launch/control, runtime Frida
probe, Tester Logs access or release ZIP modification occurred.

## Native evidence

The correct installed archive is `data/FFX_Data.vbf`. The first authorized retry
used the archive name at the installation root and failed with FileNotFound;
correcting that path allowed extraction. Filename searches found no reel-named
assets. Packed battle UI scripts only contained `bwakk` animations. A broader
DLL string search produced pixel-data false positives and is not evidence.
The actual command records identified effect565 and its HD replacement644.

The extracted `magic_0644.dll` and `magic_0565.dll` provide the reel state,
patterns, accepted Confirm position and callback lifecycle. Files were copied
and hashed offline; their entry points were never executed by the agent.

| Module | SHA-256 | Table RVA | Begin | Initial control | Running control |
| --- | --- | --- | --- | --- | --- |
| magic_0644.dll | e72045bca7d99cc0e9f28038e66115afad38573c22db6c88cf1d7b5ae23b0ad4 | E190 | 5210 | 5480 | 5550 |
| magic_0565.dll | 4fba9fe86fd2821830f89263a301fabb95f056afc94459d0fa8809056c2d165e | EB40 | 1000 | 1270 | 1340 |

Native main-executable `9DA370` (reader RVA5DA370) returns the effect callback
table. Its full relocation-masked body and instruction operands match both
supported executable profiles independently. Module identity, callback entries,
game-owner thread and generation gate all changes. Unknown modules are refused.
Only begin/table[0] and control/table[4] are wrapped. The native initialization
transition to the running callback is retained; destruction, rendering and
the rest of the callback table remain native. Reset restores owned entries.

HD control5550 dispatches script objects in modes0/2, while render5500 dispatches
mode1. Reel scripts initialize object+18 to mode0. The opcode8 owner5850 calls
reel controller60C0. Legacy counterparts1340/12F0/16C0/1F30 have the same
division. Thus suspending the effect control callback freezes the actual reel
sequence while preserving rendering. The main countdown word at reader
RVAF3F73C pauses at1. The exact previous word is restored on selection or owned
cancellation, preserving its adjacent field. This pauses the pending overdrive
sequence and its countdown; it does not suspend the OS process.

The native reel slot is1C bytes. Pattern descriptor+0 is its count, +4 its byte
array; slot+4 is16.16 position, +8 speed and +18 phase. Confirm snaps down to an
integral icon, then the result and renderer select `(highword+1)%count`.
HD5B80/6150 and legacy equivalents establish that same accepted symbol. The
choice code intersects the three actual patterns, excluding blank4, and uses
the accepted symbol positions. It resumes the ordinary native success/result
path through `MsSetReelResult`7B1B60 and match processor7B1720. Normal action
calculation, targets, battle effects and rewards remain game-owned.

English command names were decoded from the native US-PC command table. IDs
30F0/30F2/30F4/30F6 name Fire Shot, Ice Shot, Water Shot and Thunder Shot;
30F8/30FA/30FC name Havoc Shot, Time Shot and Break Shot;30FE is Power Shot.
Triple5/6 maps to Attack Reels with6/12 hits. Production uses the localized
native name getter39A4C0, rather than the English evidence strings. The internal
command descriptions saying “Wakka personal skill” are not player help.

Evidence retained without replacing older archives:

- `Docs/Scripts/Offline/output/wakka_native_20261008_213012`: module copies and
  manifest with source/hash/size/export metadata.
- `Docs/Scripts/Ghidra/output/FFX/wakka_20261008_213150`: fresh native module
  analysis and complete function exports; no analysis timeout.
- `Docs/Scripts/Ghidra/output/FFX/wakka_callbacks_20261008_215025`: read-only
  callback exports, with unsaved function-entry corrections discarded.
- `Docs/Scripts/Offline/output/wakka_commands_20261008_212933` and
  `wakka_labels_20261008_215730`: actual command records and localized labels.
- `Docs/Scripts/Offline/output/native_extensions_20261008_214418_922027.json`:
  retained main-function body and independently matched profile operands.

## Controls and beeps

Overdrive auto-success opens a valid-match menu for all four Wakka reel types.
Up/Down browse; Left/Home and Right/End choose the first/last row. Enter/Space
or controller A applies it. Escape/Back/F8 or controller B returns to manual
reels for that cast. I reads controls. Keyboard and physical controller use
the central menu dispatcher; held input drains before native controls resume.
Focus loss, console/battle/scene changes and generation reset cancel safely.
Foreign-thread resets defer native writes to the game owner.

Manual Overdrive cues use separate40ms centered beeps, the same brief style
requested for lightning. Tidus beeps on entry to its native success interval.
Wakka beeps for the symbol the native Confirm handler would currently accept.
Each actual icon traversal emits its own beep, including adjacent equal icons;
staying in the same icon does not retrigger each frame. This is a native phase
guard, not text deduplication. Auto-success and manual-cue toggles remain
independent. Lulu reads native increases in its cast meter.

| Selectable result | Frequency |
| --- | --- |
| Blank in Attack Reels | 220 Hz |
| Fire Shot / 6 hits in Attack Reels | 440 Hz |
| Ice Shot | 660 Hz |
| Water Shot / 12 hits in Attack Reels | 880 Hz |
| Thunder Shot | 1100 Hz |
| Havoc Shot | 1320 Hz |
| Time Shot | 1540 Hz |
| Break Shot | 1760 Hz |
| Power Shot | 1980 Hz |

Aurochs elemental and status symbols use distinct pitches; the Attack-only
hit symbols never occur alongside their shared-pitch elemental symbols.

## Sensor buffer ownership

Enemy highlight builds a copied affinity/status/monster-help buffer before
the existing target speech guard. The native visibility callback confirms or
clears it. Ctrl+Up/Down browse by line, Ctrl+Left selects the first and
Ctrl+Right selects the last; controller review uses the same dispatcher.
Reading keys do not reconstruct it or call the native description getter.
Native help line breaks are preserved. Current/max Sensor HP remains numeric
and independent of the player HP percentage option. The native active-party
Sensor/Scan visibility, immunity, hide-UI, target/window/actor/mask/generation
and action-reset guards remain authoritative.

## Player follow-up — 2026-10-09

The user confirms Tidus works, but Wakka produces neither manual cues nor the
auto-success choice. The earlier offline fixtures used a bare character ID and
missed the native packed effect handle. Both reel modules retain the effect
handle, while `MsSetReelResult` extracts its low-byte character ID. Production
now uses that same mask. Loaded-image ownership is resolved from the returned
table's address, preserving exact module hashes and callback-entry checks.
The revised fixtures use real-format handles and reject other characters.
This correction supersedes any implication of live Wakka acceptance above;
the repaired build still requires a gameplay retest.
[Evidence and current verification](map_overdrive_followup_20261009.md).

## Verification and gameplay limits

The focused Wakka fixture maps actual copied PE sections into non-executable
memory, relocates data and invokes production mod wrappers with mocked native
callbacks. It never loads or executes the game DLL. Both layouts verify native
patterns for Element/Attack/Status/Aurochs, begin/control transitions, repeated
casts, exact countdown resume, adjacent-word preservation, symbol beeps,
distinct Aurochs pitches, repeated identical icons, toggles, focus/foreign-thread/
generation cancellation and callback restoration.

Sensor reader fixtures count native Help calls to prove assembly on highlight
and zero reconstruction by reading keys. They cover first/last lines, boundaries,
native multiline help, updated owner snapshots, visibility/Scan/immunity and
stale ownership. The production mixer fixture verifies silence after40ms.
Affected input, SDL virtual controller, two-profile reader, extension and menu
checks cover integration. Results and deployment identities are recorded in
`install/sensor-beeps-20261008/verification.json`.

All18 affected offline PASS summaries and the fresh proxy fixture pass. The
warning-free native x86 and original-launcher build is deployed through the
canonical closed-process guard. Both actual compiled executable profiles,
feature dependencies, launcher bootstrap and13 installed SHA256 hashes pass.
No game executable entry point or gameplay hook ran during preflight.

Implementation, offline verification and development deployment are complete.
Live Wakka choice/result
acceptance, manual beep timing and Sensor browsing still require the user's
gameplay test. The existing swimming repair remains in this build; swimming
gameplay acceptance also remains pending. The development build is separate
from the existing release ZIP.
