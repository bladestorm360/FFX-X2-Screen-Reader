# Airship own-log and password follow-up, October19

October20 supersession: the user reports the temporary mute did not stop the
Free Search loop and authorizes its repair followed by restored audio.
[Current source, reproduced mixer defect, context fix and checks](airship_audio_20261020.md)
replace the blanket suppression below. The destination/confirmation/password
and deferred Y records retain their separate status.

Latest user result: destination list and password entry are confirmed working.
The earlier field-cue exclusion did not stop the Free Search loop. Release0.5.8
temporarily disables all mod audio on the airship map instead; cause investigation
is explicitly deferred. See the current sections below; historical checks do
not constitute confirmation of the new suppression.

## Report and evidence

The user's own latest `FFX_SR_Native.log` (47,562 bytes, October19 07:40 local
file timestamp) contains Select destination at07:39:48 and07:40:41 with no
destination output. All three layer signatures matched. It also records repeated
Movement blocked during Free Search and literal password glyphs `*`, `/`, `%`,
`&`. These are punctuation, not Al Bhed glyphs. Raw symbols can be suppressed by
screen-reader punctuation settings. No Tester Logs were accessed.

The log proves the reported silence, not which of the former script checks
failed, and does not identify an audio category. Source permits ordinary field
navigation/cues on the movable search actor; that is a plausible cause of the
loop. Field cues have no valid role on this map and are now excluded explicitly.
Actual audible behavior still requires user confirmation.

## Destination reader

[Airship authority](airship.md) describes the complete image/language path.
The sole producer remains `airship_destinations.cpp::observed`: native accepted
cursor show/position -> current map382 and shown background/cursor/row -> scalar
row/texture/language packet -> bounded domain14 -> worker label/PRISM. W9,
script preparation and live declarations qualify optional absolute indexing;
they cannot silence an already shown image. If metadata is missing/inconsistent,
the actual image wins and no index is guessed. Blank/unknown texture, wrong map,
off-row cursor, hidden row, overlays and obsolete lifetime remain refused.

Native91AB30 explicitly compares map01330804 with382, uses C0-byte layer records,
and adjusts first/last-row Y by minus/plus1. `native_extensions.py` resolves the
new data operand from that already-qualified complete142-byte body; deployment
preflight verifies its compiled dependency. `audit_airship_render_20261019.py`
retains original29-byte91A3D0 and142-byte91AB30 with relocation fixups, hashes and
disassembly. Reader tests execute those original bodies for all15 rows before
production capture/consumer. Phyre component translation, native SDK getters,
layer allocation and transport scheduling remain modeled boundaries.

One native event guard covers selected index (or displayed row without metadata),
texture, locale and layer-table lifetime. Equal labels on different focus rows
remain separate. Close/reset changes revision so late queued packets cannot
speak after closure. Native callbacks copy fixed scalars; workers never follow
engine pointers. The existing32-byte packet/transport bounds remain. Logging
captures at most five scalar outcomes per epoch:1 context refused,2 visible-row
refused,3 queue pressure,4 queued with absolute index,5 queued rendered label
without index. This is diagnostic event gating, not speech-text deduplication.

## Free Search and field audio

`airship_map.hpp::airshipMapActive` reads the resolved map ID on the native owner
only. `navigation.cpp::capture` pauses field audio on map entry, invalidating
its copied context and canceling wall/positional/hazard cues. It does not publish
field navigation/steering intents there. Movement hooks no longer generate
Movement blocked for the UI search actor. `navigation_scene.cpp` marks copied
airship facts disallowed; a delayed worker cannot revive a field route. The
central `wall_cues.cpp` capture cancels before camera/mesh capture on this map,
including callers outside the normal navigation update. Returning to a real
field permits fresh owned observations. Native music/SFX and the separate
airship coordinate/search assistance producer are unchanged.

The guard adds a bounded scalar native read, no scan, allocation, wait or I/O.
The wall host verifies cancel-before-copy and cache reuse after leaving382,
using real Surface geometry, maximum32767 triangles, source overwrite, held
worker pressure, stale-result cancellation and production decoder/rays. There
is no measured game action-to-audible latency or unmodded comparison in this task.

## Password/name keyboard

`field_menus.cpp::namingHelpHotkey` routes unshifted **N** first to
`readEnteredName`, then to existing help outside that context. **I** remains
details; **F11** retains its existing naming readback/repeat behavior. The user
explicitly chose N after initially suggesting I. Naming input return refreshes
owned four-byte cells even without a focus change; N reads that copied buffer,
preserving spaces, and never re-enters a native text getter. Existing session
revision/epoch checks retire closed naming screens. The existing output prefix
is Name, including password entry; there is no new password or unlock disclosure.

F8 -> **Speech options** -> **Speak punctuation names** is a persisted boolean
`speak_punctuation_names`, default **Off**, appended without renumbering prior
settings. Off sends the original highlighted symbol to speech exactly as before.
On maps a single copied ASCII punctuation glyph to a spoken name on the speech
worker; letters, numbers, non-Latin glyphs, Space and entered-name readback remain
unchanged. `naming.cpp::spokenPunctuation` is the mapping authority. The setting
description explains the limited name/password scope. Existing F8 Speech master
and other submenu positions/ownership remain, with one additional main submenu.
The new strings use the current English mod-UI path; full mod UI localization
remains the existing documented gap, distinct from native image-language support.

To extend punctuation, add a single-symbol mapping and an encoded native-glyph
fixture; do not rewrite arbitrary dialogue punctuation or introduce text
deduplication. To extend the map reader, retain actual map/layer/image evidence,
all language catalogs, profile qualification, close cancellation and native
owner/worker separation. Settings enum, option table, menu registration, docs
and persistence fixtures must stay synchronized.

## Validation and delivery

Results and installed hashes are recorded after actual checks in the delivery
receipt. Both reader profiles cover raw punctuation Off/named punctuation On,
N using copied cells and falling through after close, native cursor replay,
all26 labels across nine image languages, optional indexing and cancellation.
Menu and settings-worker hosts cover default/persistence/held disk work; audio
hosts cover actual SDL dummy-mixer cancellation. These are offline tests, not
fresh gameplay confirmation. The original save, INI and0.5.7 package are preserved.

Luna (`gpt-6-luna`, medium) implemented bounded naming/setting/menu fixtures after
Sol supplied the known pattern. Sol reviewed and corrected helper declaration
order, fixture native focus events, an unsupported test encoding, menu indexing
expectations and a missing test queue drain. User steering moved readback to N
and made punctuation names default-Off. Sol owns renderer/native discovery,
field-cue exclusion, integration, original-code replay and final verification.
No Astra, game launch/control/probe, installer rebuild or publication.

Prioritized nonvisual checks after deployment:

1. Open List and move through destinations. Hear the initial/current destination;
   close and reopen without hearing a stale queued row.
2. Open Free Search, pause, move its cursor and close it. Ordinary field tones
   and Movement blocked should stay silent there; native game sounds remain.
3. Enter several password characters and press N. Hear the entered text. With
   Speak punctuation names Off, the symbol row keeps the old output. Turn it On
   and highlight symbols to hear names; turn it Off to restore raw symbols.

These checks need only the airship map, do not require a progression reset, and
do not ask the user to review implementation. If list silence persists, the
bounded airship-focus outcomes in the next own log identify the failing stage.

## Allowance stop checkpoint

Reached98% used/2% remaining; standing stop applies. No deployment occurred.
Both reader profiles (170 hook sites), accessibility menu/persistence,
profile globals/signatures, held-settings worker, navigation audio and the
updated wall host pass. Logs are `install/native-tests/airship-password-*`
and `airship-wall-cues-20261019.log`. The new render replay audit passes;
original cursor bodies execute in both reader profiles. Canonical release-mode
native/launcher `--no-deploy` build was still running at the stop; check
`airship-password-build-20261019.log` before making any delivery claim.

Remaining: confirm build outcome, refresh documentation inventory/settings table
(now32 settings) and check links, preserve source/payload provenance, then fresh
closed-process guarded deployment and seven-hash/INI/ZIP verification. The existing
source topic updates are written, but the final generated documentation audit
has not been rerun. Keep installer/package unchanged. Do not restart the paused
heartbeat or launch/control/probe the game.

## Final delivery under user allowance exception

The user explicitly authorized finishing this build and deploying now despite
the2% stop. The staged native/launcher build completed with exit0 and no compiler
warnings. All focused checks above pass; documentation inventory now maps all141
production units and32 settings with no broken topic/source links. Current source
hashes match the retained checkpoint and predeploy snapshot.

Canonical guarded deployment passed fresh closed-process, PRISM/bootstrap,
compiled FFX/Will profile/signature/operand/dependency and installed SHA256 checks.
All seven artifacts match staged bytes; INI and0.5.7 release ZIP are unchanged.
[Delivery receipt](../../../../Analysis/FFX/RepositoryEvidence/historical-install/airship-password-20261019/deployment-verification.json)
retains hashes and logs; prior installed payload is preserved alongside it.

One preflight attempt used PowerShell7, which cannot load the full-framework
dependency. Canonical Windows PowerShell under the sandbox then could not start
the fixed-base x86 verifier (Illegal System DLL Relocation). The identical
canonical preflight and deployment succeed outside the sandbox, with no code
change or operating-system security changes. Earlier failed logs are retained.
No game entry point, launch/control/probe, Tester Logs access, installer rebuild
or publication. The three concise gameplay checks above remain unconfirmed.
The allowance exception covers this delivery; the heartbeat stays PAUSED.

## Release058 temporary audio exclusion

The earlier field-only pause did not stop the loop. The user explicitly requests
complete temporary suppression and0.5.8 without cause analysis. Destination speech
and password entry from the prior build are now user-confirmed.

`navigation.cpp::capture` publishes the native map382 fact to
`nav_audio.cpp::suppressAirshipModAudio` at the existing input-return boundary.
The native owner changes only atomic exclusion/revision/cancellation state;
it never waits for or queries the mixer. Entry and exit retire queued route,
arrival, positional, hazard, preview, lightning, Overdrive and Gagazet events.

The exclusive SDL callback clears its voices on revision changes, drains and
discards the existing maximum64-command queue while excluded, and writes literal
zero samples for every category, including continuous loops and wall fade tails.
Output retains the48000-frame request cap and512-frame chunks; a final atomic
check also silences a chunk if entry occurs during synthesis. Workers read only
copied atomics. Muted commands cannot replay on return; fresh field audio is
required. Game music/SFX, speech, labels, password handling and coordinate data
are unchanged. This is a temporary blanket exclusion on the entire airship map,
with no new setting or guessed trigger repair. Loop-producer diagnosis is deferred.
Already-buffered real-device latency is unmeasured by this offline fixture.

`nav_audio_fixture.cpp` verifies entry after command drain, all command categories,
full-queue discard, exact zero samples for48000 frames, no replay on return and
fresh non-airship output, using the actual production callback/SDL dummy stream.
Existing mixing, focus and cancellation checks remain. Reader profiles and the
canonical release-mode build verify integration. New suppression gameplay
confirmation remains pending; no game launch/control/probe was performed.

Luna (`gpt-6-luna`, medium) updated only README, changelog and controls from known
release facts. Sol reviewed them, clarified the optional hidden-destination picker,
corrected Wakka confirmation wording and added the deferred coordinate report.
Sol owns mixer changes, integration, tests/build/deploy/package and verification.
No Astra. Delivery results are recorded in the release receipt after actual checks.

## Deferred Y-coordinate question

User report, October19: M updates X and Z correctly. Y is not updating at all,
and the user does not know how to move it on the airship map. Document only;
do not investigate or change coordinate behavior in0.5.8. Future work must first
establish whether Y is needed at all for Free Search; only if needed establish
its native movement control and correct spoken/displayed representation. Do not
invent a control or assume one is missing. Cause, necessity and control remain
unresolved. The coordinate producer and formatting are unchanged.

## Verified release058 delivery

[Final receipt](../../../../Analysis/FFX/RepositoryEvidence/historical-install/release-0.5.8-20261019/release-verification.json)
records the warning-free canonical release-mode native/launcher build; both
170-site reader profiles, production SDL/audio exclusion, documentation141-unit/
32-setting checks,805 installer source assertions and130 actual dependency
assertions. The final EXE installs/reinstalls its real payload with synthetic
stock and in-memory registry validation at121/339-character fixture paths.
Guarded deployment verifies all seven hashes, compiled FFX/Will profiles,
PRISM/bootstrap/dependencies and closed processes before/after file replacement.
INI and prior0.5.7 ZIP are unchanged. The new ZIP has five distribution files,
26 embedded resources and2,569,488 bytes. Source documents, checksums and ZIP
bytes independently match. No publication or game launch/control/runtime probe.

The temporary audio exclusion remains gameplay-unconfirmed. Destination list and
password behavior remain user-confirmed. Future testing needs only opening Free
Search: mod tones should be silent, then ordinary field audio should recover
from fresh events after leaving. The user explicitly defers loop-cause and Y
investigation; no additional capture/prototype or coordinate changes were made.

## Proceed confirmation follow-up

**Latest user correction, October19:** the user suspects the wrong prompt was
fixed. Treat the originally reported airship destination confirmation's silent
highlighted options as unresolved; do not infer success from the layer29 fixtures
or build/package checks below. At1% remaining usage, the user explicitly defers
all further investigation and repair and will explain the limitation in the
changelog. Documentation only now; no code, build, deployment, package or
changelog changes. When the user resumes, first establish which actual prompt
and option producer the report concerns before changing behavior. Whether this
is layer29 or a different confirmation remains unverified. Retain the attempted
implementation and evidence; do not restore older code or payloads automatically.

The user confirms the main list reads, but its following Proceed? prompt's
highlighted options are silent. [Image confirmation flow and evidence](airship.md#proceed-confirmation-options)
records the separate layer29 texture producer, verified original Yes/No images
and native setter replay. The native prompt remains its existing TK producer.
First accepted choice appends; changes interrupt; showing a stale texture before
the next-frame native initial selection emits nothing. Confirmation and list
have independent bounded scalar snapshots and retirement revisions.

This same-version0.5.8 rebuild retains blanket airship mod-audio exclusion and
the deferred Y/loop-cause restrictions. No new gameplay confirmation is claimed.
Final build/deploy/package checks are recorded in the
[confirmation receipt](../../../install/release-0.5.8-confirmation-20261019/release-verification.json)
after completion. Sol owns this scoped fix and validation; no new delegation.

Verified delivery: both171-hook reader profiles pass, including original60-byte
native texture-setter replay and all localized options. The canonical release-mode
native/launcher build is warning-free. Guarded deployment verifies closed processes,
compiled FFX/Will signatures/operands/dependencies, PRISM/bootstrap and all seven
installed hashes. Frozen source, live INI and0.5.7 are unchanged. Final EXE passes
805 source/130 actual dependency assertions and actual install/reinstall at134/
352-character fixture paths, without game/native payload execution or real registry
repair. The five-file ZIP is2,570,771 bytes, SHA256
`897d5160ca1e0161e67c9900d8d027b0fa50f4141355453bc9a10625e8a262e3`.
Prior0.5.8 is preserved byte-for-byte in
`install/release-backups/0.5.8--20261019-092922-286ade3c`. Pending gameplay:
Proceed? then initial Yes, moves to No/back, cancel/reopen without stale No;
the existing complete airship audio exclusion remains unconfirmed. No Y/loop-cause
investigation, publication, game launch/control/probe or Tester Logs access.
