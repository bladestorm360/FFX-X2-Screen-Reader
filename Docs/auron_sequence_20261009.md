# Auron manual sequence speech — 2026-10-09

Gameplay update: the user confirms Auron auto-success and sequence announcements
work. Earlier pending statements below are historical.

The user requests spoken Auron sequences with Overdrive cues On and
Overdrive auto-success Off. This extends the earlier
[map/overdrive follow-up](map_overdrive_followup_20261009.md). Sol investigated,
implemented and verified alone; no delegation, game launch/control/live probe,
game-install source investigation or Tester Logs access.

## Event and native input evidence

Retained `FUN_00898A40` (RVA498A40) selects the actual button array and timer,
counts the terminated short-word array, then sets Auron state to1. On return,
the sequence is available before `FUN_00890EE0` starts its animation/setup and
timed phase4. Speech attaches to this initializer return, once for each cast.
The per-frame phase handler does not produce sequence speech; repeated casts
with identical sequences remain audible. There is no text dedup or polling.

The native instructions store a 32-bit count at133D6C0, a 32-bit array pointer
at133D6C4, and a 16-bit phase at133D6B4. The array contains 16-bit action masks
terminated byFFFF. Exact bounded signature154 bytes and these operands were
qualified from the retained native archive, without accessing the installation.
Fresh evidence:
`Docs/Scripts/Offline/output/native_extensions_20261009_114945_611215.json`.
Missing initializer/count/pointer dependencies refuse startup/preflight.

The four actual arrays are retained in
`Docs/Scripts/Offline/output/auron_sequences_20261009_112337_188983.json`,
extracted from the archived C0A000 block by `inspect_auron_sequences.py`.
Native `FUN_00631770` registers configuration rows with masks in this order:

| Row | Logical mask | Action |
| --- | --- | --- |
| 0 | 1000 | Up |
| 1 | 4000 | Down |
| 2 | 8000 | Left |
| 3 | 2000 | Right |
| 4 | 20 | Confirm |
| 5 | 40 | Cancel |
| 6 | 10 | Square |
| 7 | 80 | Triangle |
| 8 | 4 | L1 |
| 9 | 1 | L2 |
| 10 | 8 | R1 |
| 11 | 2 | R2 |
| 12 | 800 | Start |
| 13 | 100 | Select |

Controller rows14–27 have the same lower-word masks (shoulders also retain
native upper-word flags); keyboard alternates are rows49–62. The R1 assignment
appears as `&DAT_02000008` in Ghidra: it is the immediate mask02000008, not a
borrowed input pointer. The local Fahrenheit `core/fh/input.cs` independently
confirms these action masks; no reference source was edited or loader adopted.
Existing `pedestal_keys.cpp` uses this same native registration order.
All28 keyboard/controller registrations were independently extracted and
checked against their lower-word masks in
`Docs/Scripts/Offline/output/auron_bindings_20261009_120127_655143.json`.

These logical input masks must not be interpreted as font tokens or assumed
PlayStation/Xbox button labels. nativeButtonMaskName is a central helper
in `button_glyphs.cpp`: it reads the configured device's native action name
through nativeBindingName/5AF0/5B10. Keyboard uses its configured alternate
when the primary is unassigned. Connected controller ownership selects physical
controller labels through the existing SDL/native bridge and game remaps.
No unrelated font-token behavior or pedestal guard was changed.

## Behavior and validation

auronSequence reads the native selected array on initialization return.
It emits `Sequence: ` followed by every configured button name in order, using
the producer `overdrive-auron-sequence` and interrupting speech for the timed
prompt. Cues must be On and auto-success Off, with active focused battle,
no scene/mod-menu/utility console, and initialization phase1. It copies at most
15 buttons plus the terminator; missing/unreadable data, invalid lengths,
unknown/composite masks and missing bindings remain silent rather than inventing
a partial sequence. It does not alter countdown, progress, phase or input.
The existing auto-success paths and Tidus/Wakka/Lulu behavior remain intact.

The event-only Frida prototype `Docs/Scripts/Frida/FFX/auron_sequence.js` was
checked offline against all four retained arrays before native implementation.
It is never deployed or enabled beside production. Native reader fixtures test
those actual archive bytes, both device bindings, all four ordered sequences,
new casts with identical text, fresh remaps/alternate keys, unchanged native
timer/phase, toggle and context/data rejection, and quiet per-frame callbacks.
Both executable profiles are tested along with signature/dependency refusal.

An initial test compilation used a temporary string where the fixture's expect
helper requires a pointer; its call now uses c_str. The first clean reference
run passed, but the installed fixture's synthetic data setup incorrectly kept
an already mapped native state operand at its reference address. The fixture
now honors the existing verified mapping for such operands. These were fixture
issues, not deployed behavior; clean final results are recorded below.

The offline prototype, both-profile native readers and signature/dependency
checks pass. The setting's spoken Help also describes Auron's sequence speech.
The final canonical native x86/launcher build is warning-free; the fresh proxy
forwarding check and guarded closed-process deployment pass. Both actual compiled
executable profiles, feature dependencies, launcher bootstrap and all13 installed
artifact SHA256 hashes pass. Windows PowerShell module paths were scoped to the
deployment child process as in the preceding follow-up; no persistent environment
change. Verification and artifact identities:
`install/auron-sequence-20261009/verification.json`; final build/proxy/deployment
logs are in that folder, and the clean native reader/profile logs are
`install/auron-readers-verified-20261009.log` and
`install/auron-profile-20261009.log`.

Implementation, offline verification and development deployment are complete.
Exact speech timing and successful manual input remain user-side gameplay
checks. Release ZIPs and registry settings are unchanged.

User test: F8 > Miscellaneous, Overdrive cues On and Overdrive auto-success Off;
select Banishing Blade or another Auron overdrive. Expect `Sequence:` and your
configured buttons in order as the move starts. Press them individually in that
order during the native input window. Turning cues Off or auto-success On must
suppress the sequence announcement.
