# Map arrivals and overdrive follow-up — 2026-10-09

Later gameplay reports found title arrival speech and continued missing Wakka
attachment. The active-field gate and native effect-start discovery supersede
those earlier conclusions; Auron is user-confirmed. See the
[current follow-up](field_reels_mp_followup_20261009.md).

The user requests completed map-transition announcements, confirms Tidus's
overdrive works, reports missing Wakka cues/auto-success selection, and asks
how Auron's Banishing Blade input works. Sol performed the investigation,
implementation and verification alone; no delegation occurred.

## Completed map arrival producer

The existing request hook `46FED0` and resource loader `5093B0` do not establish
field readiness. Retained native `FUN_0088DFE0` (RVA `48DFE0`) initializes the
destination through `FUN_00872E90`, completes nested field resources, resets
transition flags `0133080C`/`01330818` and performs the final room setup calls
before returning. `FUN_0088D340` supplies the actual destination room. Evidence:
`Docs/Scripts/Ghidra/output/FFX/decompile/layer2_game_logic.txt`, functions
`0088DFE0` and `0088D340`, and retained
`Docs/Scripts/Offline/output/reader_reference_20261003_235502`.

`map_transition::Initialize` is a native resource scope. Its entry invalidates
old reader snapshots and navigation state; ordinary callbacks stay suspended
through nested loads. Only its completed outermost return takes the current
generation and reads fresh destination state. Unwinds produce no arrival and
release the loading barrier. This does not revive ordinary stale snapshots.

The shared currentMapName serves both M and announceMapArrival. It uses
the primary field chunk at controller `F26AE8 + 1F8`, current entrance
`D2CA90 + C`, native `46BD20` area index, `4B7F10` catalog, `46C3C0` name and
`2460E0` decoding. The existing exact room fallback remains. Unknown labels
remain silent. The completed room must match the initializer's argument;
title/save-load room23 and battles are excluded. Speech queues with the producer
`map-transition`, for example `Entering Guadosalam - Shop`. Repeated completed
arrivals with the same text still announce; no text deduplication or polling.

The event-only offline Frida prototype is
`Docs/Scripts/Frida/FFX/map_transition.js`; it is never deployed or enabled.
The native hook's masked signature is independently unique in both supported
profiles, with dependency refusal if absent. Fresh qualification:
`Docs/Scripts/Offline/output/native_extensions_20261009_110506_333594.json`.

## Wakka correction and limits

The permitted current own-session `FFX_SR_Native.log` contains Element Reels at
04:04:40, but no Wakka callback-installed/refused message. This establishes
missing live integration evidence; it does not prove which binding guard failed.
No Tester Logs were accessed. No game launch, control or live probe occurred.

Both retained native modules store the packed effect handle in their actor field.
Main `FUN_00788E20` calls the module begin callback with
FUN_00788EB0 using serial, type, character; its low byte is the character ID.
`FUN_007B1B60` (`MsSetReelResult`) masks the handle with `FF` before checking
Wakka's ID4. The previous nativeWindow comparison with bare4 rejected
ordinary casts such as handle `00128004`, preventing cues and auto-selection.
The central helper now extracts the same low byte. This is a definite bug
independent of the missing callback-install log.

Loaded module ownership now uses GetModuleHandleExW using FROM_ADDRESS, UNCHANGED_REFCOUNT and requires the returned exact PE base, instead of a
basename lookup. Main `009DA420 -> 00679590 -> 006F9CA0/006F9DA0` loads native
PEs through `LoadLibraryA`; `009DB0F0` resolves `GetEffectOverlayTable`, and
`009DA370` returns its table. Exact file hashes, supported layout/table offsets,
original callback entries, owner-thread and generation checks remain required.
Unsupported callback entries now have an explicit refusal message. Native
rendering/result processing, cancellation and the existing traversal guards
remain unchanged.

Retained module evidence and layout hashes are linked in
[the preceding native investigation](wakka_sensor_beeps_20261008.md).
The revised fixture maps these copied modules into non-executable memory and
does not call their entry points. It uses packed handles across all four reel
types, rejects another character, verifies address-based loaded-image ownership,
and exercises paused choices, clock preservation/resume, manual pitches,
repeated casts, callback transitions, focus/generation/foreign-thread
cancellation and restoration. Runtime callback installation, beep timing and
actual chosen results still require the user's gameplay retest.

## Auron behavior

Native `FUN_00898A40` chooses one of four terminated short-word button arrays
through `FUN_00A5CA60`; it starts the corresponding native timer. Retained data
was exported without reading the installation by
`Docs/Scripts/Offline/inspect_auron_sequences.py` to
`Docs/Scripts/Offline/output/auron_sequences_20261009_112337_188983.json`.
These are native action masks, not assumed keyboard/controller labels.

In `FUN_00890EE0`, phase4 compares each pressed action with the next required
array entry. Correct input advances; wrong input resets sequence progress;
timer expiration fails. Completing the sequence selects phase5, the ordinary
success flag/animation and result path. Banishing Blade uses this timed input
mechanism. Auto-success advances that shared phase to normal success for all
four combinations. It has no selection menu. With Auto-success off, the mod
now has an initialization-event sequence reader when Overdrive cues is On;
manual inputs remain required. This was added by the subsequent user request:
[implementation, evidence and verification](auron_sequence_20261009.md).
The user's Tidus confirmation is recorded separately from this offline proof.

## Verification and deployment

Focused offline checks pass: map prototype; reader fixtures in reference and
installed profiles; signature/dependency qualification; real-adapter threading,
tail jumps, ABI preservation and deferred native jobs; both Wakka module layouts.
An initial reader fixture failed because the new test failed to restore its
mock controller; restoration fixed the test isolation and the clean rerun
passes. An ABI run overlapped another test's shared MinHook copy and hit a file
lock; the sequential rerun passes. Neither failure affected production code.

The warning-free canonical native x86 and launcher build, fresh proxy forwarding
check and guarded closed-process deployment pass. Both actual compiled executable
profiles, launcher bootstrap and all13 installed artifact SHA256 hashes pass.
An initial deployment preflight failed before replacing files because Windows
PowerShell inherited PowerShell7 module directories and could not find
Get-FileHash. Restricting PSModulePath to Windows PowerShell modules for that
child process fixed it; no persistent environment or project workflow change
was made. The full guard was rerun successfully.

Logs and final artifact identities:
`install/map-overdrive-20261009/verification.json` and
`install/map-overdrive-20261009/deployment-final.log`. The permitted own-session
log was preserved as `install/map-overdrive-20261009/own-session-before.log`.
Implementation, offline verification and development deployment are complete.
Map arrival timing, live Wakka binding/cues/selection and Auron's in-game result
remain user-side confirmations. No game executable entry point, live speech
or gameplay hook was executed by the agent. Release ZIPs and registry settings
remain unchanged.

User retest: enter a map, then return to it; expect one native destination
announcement per completed initialization. For Wakka, use F8 > Miscellaneous:
Auto-success Off and Overdrive cues On should give symbol beeps; Auto-success
On should pause the reel sequence and read `Match reels` choices. Up/Down
browse; Enter/controller A applies; Escape/controller B returns to manual reels.
