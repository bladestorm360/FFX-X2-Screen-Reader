# Wakka activation and success mechanics — 2026-10-09

Sol investigated, implemented, reviewed and verified this repair alone. The
user's repeated report supersedes earlier optimistic attachment conclusions.
Gameplay acceptance remains pending. Tidus and Auron retain their user-confirmed
status. No game launch/control, live probe, Tester Logs, registry change or
release ZIP change occurred.

## Definite cause

The permitted own-session log records Attack Reels/command3075 at07:26:19.710,
then native effect handle204 and table5BA5EB38 at07:26:21.213, with no callback
installation. The two previously supported DLLs return EB40/565 and E190/644.
Their actual installed files still match the retained hashes.

The native selector79D310 recognizes Wakka command categories24/34/44/54 and
overrides the kernel effect IDs according to the native environment value:

| Native environment | Two selected effect IDs |
| --- | --- |
| 0 or2 | 697,698 |
| 3 | 693,694 |
| 4 | 691,692 |
| 5 | 695,696 |
| Other | Command's original565/644 |

Selector iVar2 chooses the first/second native animation variant. This is why
reading only the command record's effect565/644 missed the live module.
Read-only exported-getter inspection found697 is the exact DLL returning EB38.
Its table/base and hash match the observed address. Variant698 returns144020;
the other variants use EB40/E190 but have distinct hashes. The old hash loop
would also refuse an otherwise compatible variant at its first different hash.

## Activation and lifecycle

Native7AFD40 initializes built-in controllers for Tidus, Auron and Lulu only;
Wakka's minigame runs inside the selected effect DLL. Native7B1A00 selects
Element/Attack/Status/Aurochs from actor+F5C commands3074/3075/3076/3077.
Its full98-byte relocation-masked body matches once at installed3B19F0,
independently verifying this field in both actual executable profiles.

Both per-character788540 and global788E20 create an effect ID through788EB0
immediately before table[0]. The existing owner-thread return listener now
requires those exact reel commands, avoiding ordinary Wakka effects, and obtains
character+F8C or globalD33360. Ten exact file hashes qualify the supported
layouts. The exact loaded-image base, table offset and begin/control entries
remain required. No module-ownership check was weakened.

Begin stores the packed handle; its low byte identifies Wakka. Initialization
changes table+10 from1270 to1340 in legacy variants, or5480 to5550 in HD
variants. The wrapper preserves that native transition. Rendering/table+0C
stays native. Legacy window14CF28, set14D118, count14D11C, sequence14D120 and
actor14D184 are verified in every legacy copy. HD counterparts14C460/14C650/
14C654/14C658/331A44 are verified in every HD copy. No blanket data delta.

## Native accepted symbols and results

Each1C-byte slot contains a descriptor/count/byte array,16.16 position and phase.
Legacy697 controller19F0 snaps Confirm to an integer; result1FC0 selects
`(highword+1)%count`. Other variants have the same verified rule. The native
interactive sequence is1 with window+68 equal1. Assistance places three accepted
symbols, sets each reel stopped and advances column+64 to3. The ordinary native
match animation/result path then continues.

Native7B1720 counts matching symbols; native7B1B60 maps the result through
C4394E and calculates the ordinary targets/action data:

- Element0/1/2/3: two matches produce result1/3/5/7, a single-target shot.
  Three produce2/4/6/8, commands30F0/30F2/30F4/30F6 for all enemies.
- Status7/8/9: two produce9/B/D, a single-target shot; three produceA/C/E,
  commands30F8/30FA/30FC for all enemies: Havoc, Time and Break Shot.
- Attack5/6: three single-hit5 symbols give6 hits; three double-hit6 symbols
  give12. Mixed symbols give count5 plus twice count6. Result11 uses312E.
  Auto-success selects triple6. Manual Attack cues omit blank4.
- Aurochs10: three produce resultF, command30FD, **Aurochs Shot**. It targets
  all enemies and has native power72 versus34 for the other available matched
  shots. Every Aurochs pattern includes10; the player's latest request makes
  this one of eight paused choices instead of selecting it automatically.
  Command30FE/Power Shot belongs to unmatched result10, not the Aurochs match.
  The old symbol10 label was therefore wrong and is corrected.

Element, Status and Aurochs use the paused valid-match list with localized native
command names. The countdown wordF3F73C pauses at1 and resumes its exact saved
value; adjacent data is preserved. Input/controller capture and existing
owner/context/generation cancellation remain. Manual symbol traversal guards
remain; no text deduplication was introduced.

The Aurochs choice follow-up is documented in
[Aurochs paused choices](aurochs_choices_20261009.md). Only Attack Reels applies
an automatic match immediately; the other three Wakka reels pause for input.

## Retained evidence and verification

- Main native bodies: existing decompile/layer2_game_logic.txt79D310,7AFD40,
  7B1A00,788540,788E20,788EB0,7B1720 and7B1B60.
- Actual variants/callbacks/hashes/patterns:
  `Docs/Scripts/Offline/output/wakka_variants_20261009_144514`, including
  qualified_layouts.json. Fresh untimed headless analyses are
  `Docs/Scripts/Ghidra/output/FFX/wakka_697_20261009_144000` and
  `wakka_variants_20261009_144600`; original565/644 archives are preserved.
- Installed command-field body:
  `Docs/Scripts/Offline/output/wakka_command_layout_20261009_150437.json`.
  An initial comparison omitted the absolute jump-table operand; the complete
  relocation mask resolves it uniquely without masking the F5C field.
- Native result table and retained English command records:
  `Docs/Scripts/Offline/output/wakka_table_inventory_20261009_143722/result_commands.json`.
  Record strings begin at header-relative7814, not record-end or7800. Production
  uses the native localized getter; these English strings are offline evidence.
- Offline Frida prototype/pure checks: field_reels_followup.js and
  check_field_reels_followup.cjs. Neither was attached/deployed.
- Regression first reproduces missing attachment using actual697:
  install/wakka-detection-20261009/before.log. Revised fixtures map all ten
  actual DLLs into non-executable memory; entry points/exports/native callbacks
  are never executed. All four actual commands attach; Element/Status choices,
  Attack/Aurochs best matches, native accepted bytes, initialization transitions,
  ordinary-action/null-actor/foreign/outside-battle refusal, exact hashes/table/
  callback refusal, clock/cancellation and restoration pass. Both reader
  profiles pass, including prior H/O, Auron, map and swimming behavior.

Automatic review rejected one proposal that broadened ownership resolution and
another with a null-actor guard ordered after a read. Both were replaced before
application: exact-base qualification remains, and the actor guard precedes its
field read. No rejected proposal was applied or bypassed.

Profile, threading, keyboard/controller, navigation audio and settings/menu
regressions pass. The native x86/launcher build has no compiler warnings/errors.
Fresh proxy forwarding passes with the new DLL. Canonical closed-process guarded
deployment verifies both compiled profiles, launcher bootstrap and all13 installed
SHA256 hashes. Source, stage, fresh fixture and installed proxy identities match.
Proof: `install/wakka-detection-20261009/verification.json`. Gameplay timing,
menu behavior and actual chosen damage/results await the user's test.
