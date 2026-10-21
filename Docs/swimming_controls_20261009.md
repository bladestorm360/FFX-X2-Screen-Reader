# Swimming control instructions — 2026-10-09

The relayed tester report is accepted as evidence: post-salvage Return to ship,
Dive and Examine spoke unrelated shoulder controls. Sol investigated, fixed,
reviewed and verified alone. No Tester Logs were accessed. No game was launched,
controlled or probed; all source evidence came from retained archives.

## Cause and native evidence

nativeGlyph used face-token rows11/10/9/8 and controller rows25/24/23/22.
Those are shoulder actions, explaining the reported LB/RB/RT instructions.
Its directional lookup also used face rows, and Start/Select were reversed.
The original fixture populated those same incorrect rows, so it did not detect
the error; the earlier swimming fixture omitted button tokens entirely.

Native8F8EE0 translates text opcode0B's atlas token and exchanges31/32 under
the native regional swap. Native6E6860 initializes the token-to-action table;
6E7180 uses that table, 6E67A0 reads the keyboard row and6E6590 adds14 for
the controller row. Native631770 independently registers the action masks.

| Token | Action row | Logical mask |
| --- | --- | --- |
| 30 | 6, Square | 10 |
| 31 | 4, Confirm | 20 |
| 32 | 5, Cancel | 40 |
| 33 | 7, Triangle/menu | 80 |
| 34 | 8, L1 | 4 |
| 35 | 10, R1 | 8 |
| 36 | 9, L2 | 1 |
| 37 | 11, R2 | 2 |
| 38 | 12, Start | 800 |
| 39 | 13, Select | 100 |
| 41/42/44/48 | 0/3/1/2, Up/Right/Down/Left | 1000/2000/4000/8000 |

Use this native table instead of interpreting tokens as familiar PS icon names.
The unchanged central nativeButtonMaskName reads configured keyboard,
alternate keyboard and controller names through native405AF0/405B10.
Fahrenheit's MIT `core/fh/input.cs` independently agrees on these masks.
No external reference source or loader was changed.

The original cdsp0000 string107 contains33 Return to ship and32 Dive;
cdsp0700 string0 contains31 Examine, and string1 contains33 Return to ship.
cdsp0000 worker16 PC42161 tests native button index7 for Return to ship.
Native82D200 consumes held mask40 to dive, with the existing Dive-enabled,
swimming-mode and vertical-velocity checks. The depth goal and Auto walk
already use action5/mask40; their mechanics remain the verified behavior.
With standard Xbox bindings this means Y Return to ship, B Dive and A Examine.
Actual speech always follows the current configured names, including remaps.

Fresh retained-source extraction:
`Docs/Scripts/Offline/output/swimming_controls_20261009_162013_936612.json`.
`inspect_swimming_controls.py` preserves original prompt bytes/hashes and
native function bodies, asserts the exact table and both input consumers.

## Repair and verification

The shared `button_glyphs.cpp` producer now translates tokens into verified
logical masks and calls the same central naming helper as Auron/Dive guidance.
This fixes ordinary dialogue/tutorial controls and field HUD controls together.
Known controls without a binding say `unassigned control`, without falling
back to an invented physical label. The existing regional swap, controller
ownership, native page order and per-event speech ownership remain.
No text deduplication, second speech producer or input automation was added.

The offline Frida text prototype is `button_glyph_controls.js`, validated by
`test_button_glyph_controls.cjs`; it performs no attachment or runtime call.
The native `button_glyph_followups.inc` fixture loads the actual EBP string
bytes and checks all ten face/shoulder/Start/Select tokens, movement tokens,
keyboard/controller names, remaps, keyboard alternates, missing bindings,
regional swap, field announcement/review and agreement with Dive guidance.
Separate installs of the same text each remain audible. Both supported reader
profiles pass, including the existing actual Auron arrays and sequence checks.

The canonical x86 native/launcher build completed without warnings or errors.
The fresh proxy forwarding check and existing swimming Auto walk/remap/manual
priority checks pass. Closed-process preflight, both compiled native profiles,
launcher bootstrap and all13 installed hashes passed; deployment completed.
Build/deployment proof is `install/swimming-controls-20261009/verification.json`.
Gameplay acceptance of the corrected spoken controls remains user-side.

## Aurochs matching effects, clarified in this turn

Retained command.bin's96-byte records confirm that other matched reels retain
their elemental or status effects. The command/status/element layouts are
independently described by Fahrenheit's MIT FFX structs.

| Match | Native command IDs | Effects |
| --- | --- | --- |
| Fire/Ice/Water/Thunder | 30EF–30F6 | Respective elemental damage |
| Havoc | 30F7/30F8 | Attempts Poison, Sleep, Silence and Darkness |
| Time | 30F9/30FA | Attempts Petrify and Slow |
| Break | 30FB/30FC | Attempts Power, Magic, Armor and Mental Break |
| Aurochs | 30FD | Non-elemental damage, no status effects |

Other shots have native power34; Aurochs Shot has72. Thus it is the strongest
raw damage match, while other matches can have tactical value. Status record
bytes254/100 are retained as evidence, not interpreted as universal success
probabilities or immunity bypasses. The subsequent player request changes
Aurochs to an eight-outcome paused choice list; see
[the current behavior](aurochs_choices_20261009.md).
Fresh records and hash:
`Docs/Scripts/Offline/output/aurochs_effects_20261009_162234_590088.json`.
