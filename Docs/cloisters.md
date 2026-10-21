# Cloisters of Trials — solver, guidance and development utilities

October15 correction: Zanarkand has separate first-visit and return-visit
assistance. First visit opens only the normal trial gate and stops; the player
takes the native exit and completes the encounter/story sequence. Return-visit
assistance collects the Destruction Sphere treasure and stays in the trial.
No automatic lift/encounter/story skip. [Phase evidence](help_navigation_quit_20261015.md).

The original pattern displays now have distinct navigation labels. After a
display's native animation completes, Autodetail reads its verified glyphs;
F8 > Miscellaneous > Cloister of Trials > Review displayed glyph sequence gives
Up/Down review and whole-sequence I. Its panel numbers, shapes and colours match
the floor targets and manual guide. [Source evidence and verification](zanarkand_patterns_20261015.md).

October12 source maintenance implements Airship internal exits/elevators and menu speech, Polish text detection/override, Grid Status detail/review, displayed Overkill, current Overdrive help, the linked Salvage Ship underwater exit, Kilika treasure continuation, and Macalania goal choice. Via Purifico adds both progression exits plus a manual panel guide and optional native-script collect-all action. Offline validation/build/deployment status: [repair record](open_issue_repairs_20261012.md). Gameplay confirmation remains user-side; the October9 release ZIP is unchanged.

Macalania asks **Complete trial and leave**, **Visit the fayth**, or **Cancel** after the first solver interaction. The fayth option takes the native antechamber exit; the game owns its door and events. F8 > Miscellaneous > Cloister of Trials arms the solver. [Via Purifico controls](via_purifico.md).


Known issue (2026-10-06 release): Kilika Fire barrier recess may route to the
barrier rather than the Glyph Sphere insertion point. This is unresolved.

## Current player-facing implementation — 2026-10-06

The six trials now have a native context-sensitive mod menu and an opt-in
interaction solver. No game launch, focus change, attachment, warp or save
modification was performed while implementing this update. New gameplay
behavior is partly live-validated: the player confirmed the Besaid autosolver
on 2026-10-06. Other solvers, manual guide order and the latest routing changes
still require gameplay validation.

After the updated DLL is installed, on the next user-launched game:

1. Press F8 in a trial, open **Miscellaneous**, then **Cloister of Trials**. The
   submenu focuses **Solve current trial automatically**. Space enables it;
   Escape returns to settings and another Escape closes the menu. Outside a
   supported trial, puzzle options are hidden. View/Select then Menu/Start opens
   the same settings with an SDL controller; D-pad/South/East navigate/back.
2. Interact normally with a sphere, glyph, pedestal or supported puzzle control.
   The solver intercepts that actual native interaction. A carried or stored
   Destruction Sphere runs the original reveal and chest scripts and stays in
   the chamber. Confirm the game's item dialogue normally. Solving remains on;
   another ordinary puzzle interaction completes and exits the trial.
3. An ordinary puzzle interaction prepares and collects the Destruction Sphere
   treasure first, then requests the original completion/exit script. This is
   a completion shortcut: it does not replay every normal sphere placement,
   pedestal push or rail animation. Zanarkand's first visit instead places the
   Besaid/Kilika socket state, requests its original gate sequence at progress
   2775, and stops for the native encounter exit. It does not force the later
   2785 story event or request the lift. Later visits preserve higher story
   progress and collect treasure without leaving. Bevelle's separate
   optional chest is not collected by the required-treasure shortcut.
4. F9 selects the previous manual step, F10 selects the next, Shift+F10 reads the
   selection, and Shift-F11 reads the step inferred from verified native state.
   Manual selection never moves automatically. When an exact expected step
   cannot be identified, the reader says so; many non-Besaid steps still lack
   verified completion guards. F8 provides Previous/Next/Read Step/Read Path;
   controller mod-mode A/B select previous/next.
   The six guides have 95 actions bound to original worker identities and
   unambiguous script coordinates, with live actor positions preferred.
   Every guide target name is copied from the audited pathfinder catalog.
   Kilika has 16 steps: retrieval/storage and pickup/placement are separate
   actions, and Entry door recess uses the same name in both features.
   Verified trial object controls remain listed while temporarily inactive.
   Walking paths use the native player collision getter, including mutable
   door groups and height; group-state changes refresh field routing.
   Disconnected platforms require their lift/pedestal/rail action; the reader
   does not describe a straight walk across a gap. Bevelle junction speech
   follows the native direction-window assignment, with no elapsed-time timer.
   Zanarkand tile targets follow the active original pattern branches and
   duplicate-shape flags. Manual action order, rail choices, tile completeness
   and movement distances still require gameplay validation. These are not
   claimed to be exact, validated walkthroughs.
5. Reopen F8's Cloister submenu and press Space to disable solving. Map teardown clears the solver
   and guide. A running original animation/dialogue is allowed to finish.

F12 and the warp/trial command console remain in the local development build.
`src/proxy/build_dinput8.bat --release` and `--test-build` exclude the console
window, F12 registration and console command entry points; F8/F9/F10/F11 remain.
Both modes also accept `--no-deploy`. Default builds include development
utilities. The distributed-test flavor has the same console exclusion as a
release. Production uses only native x86 C++ and Tolk_Output; no JS/Gadget
producer or dotnet/MSBuild command was added.

Source: `src/native/mod_menu.*`, `puzzle_solver.*`, `puzzle_completion.*`,
`puzzle_guide.*`, `puzzle_tiles.*`, shared `utility_*` code and input ownership.
Generated contracts retain script length/FNV, declaration, entry-table,
original-RET, owner/signal and generation checks. The return hook is reference
469170 / installed 4691D0, verified through the archived VM call context;
469310 is not the interpreter's normal return and is not used here.
The exact executable profile contains 180 individually verified RVAs.

Offline validation: the combined native suite passes both executable profiles,
ABI, input ownership, collision geometry (stacked floors/gaps), all six natural
interaction handoffs, carried/stored Destruction Sphere detection, treasure
before completion, first/later completion selection, duplicate collection,
opt-out and stale-map cancellation. UI accessibility/focus and actual ATEL
execution/item grants are not simulated by those tests. Source checks also
pass for both six-trial profiles and unsupported/mixed-profile refusal.
Detailed logs: `install/puzzle_mod_menu_tests.log`,
`puzzle_mod_menu_source_checks.log`, `puzzle_test_build.log`, and
`puzzle_mod_menu_build.log`. Current source check archive:
`cloister_designs_20261004_131921_356300`; check log:
`install/cloister_checks_20261004_131930_864435.log`.
Contracts: `native_utilities_20261004_132400.json`,
`puzzle_controls_20261004_132405.json`, `puzzle_completions_20261004_131008.json`.
Profile evidence: `reader_reference_20261003_235502/reader_audit_20261004_122556.json`.

Navigation additionally replaces decorative Djose/Macalania sphere model
targets with their actual player collision controls, labels all five Djose
upper switches, and preserves real pedestals, resets, chests and exits. The
catalog now contains 2,442 targets across 397 scripts; audit:
`navigation_audit_20261004_122952.json`. Kilika's Destruction Sphere identity
is 6; Djose's upper pedestal uses binding 0181F0, and Macalania's pedestal
sphere uses 015A8F rather than its position state.

The local development build was installed at 2026-10-04 13:33 UTC after
rechecking that FFX and its launcher were closed. The canonical deploy script
verified all four installed artifact hashes. No game was launched or saves
changed. Installation: `install/puzzle_menu_deployment_verified.json`;
rollback backup: `install/cloister_test_runtime/puzzle_menu_before_20261004_133303`.
It takes effect on the next user-launched game. The distribution binary's
console-class absence and retained mod-menu class were separately verified.

## Earlier development-console implementation — 2026-10-04

The user replaced prolonged live route testing with native utilities and ATEL
navigation cleanup. All six trial utilities are implemented in the source.
The update was installed with the game and launcher closed. All four native/
launcher artifact hashes match the staged build. It takes effect on the next
user-launched game; no launch was performed.
No new game launch, warp, attachment, or keyboard test was performed during
this utility implementation. Do not launch or take focus without the user's
explicit permission. Test the new commands when the user chooses; remaining
trials can be exercised by testers over time.

After installation and a game restart, use these from a loaded field:

1. Press F12 to open the accessible utility window. Type a command and press
   Enter. A game command returns focus after Enter is released; replies use
   Tolk_Output, including braille. Escape returns to the game. Tab to Results
   to review previous replies. Typing here does not invoke reader hotkeys.
2. Use `warp besaid`, `warp kilika`, `warp djose`, `warp macalania`,
   `warp bevelle`, or `warp zanarkand`. `warps besaid` searches the broader
   catalog; `warp 122 0` uses a room and entrance. The catalog has 328 rooms
   with extracted entrance bounds. `where` reports the active script.
3. `trial treasure` completes the Destruction Sphere objective and prepares
   its treasure while remaining in the chamber. `trial collect` runs the
   original chest script, including its item grant, dialogue, and permanent
   collection flag. Confirm any game dialogue normally.
4. `trial status` reports the current objective/collection state.
   `trial finish` completes and leaves for the antechamber after collection.
   It refuses to leave treasure behind, preserves later story progress, and
   applies the original Besaid/Zanarkand completion progress when needed.
5. `warp force besaid` is an explicit recovery command for leaving a running
   field sequence or dialogue. Ordinary warps respect those guards. Both
   commands refuse battles and invalid room/entrance pairs. Warps themselves
   do not change story progress; trial commands intentionally change gameplay.

Commands cross from the standard Windows controls to the existing native
input event as copied text. Native getters, setters, REQ and map transitions
run only on that owner event. There are no UI timers, passive state watchers,
Gadget loads, or concurrent Frida production producers. Reveal continuations
resume original bytecode at recorded instruction boundaries after the native
request initializer; code and entry tables are not patched. Generation,
code-hash, entry, declaration, bounds and busy guards reject stale operations.
The RE-only continuation reference is `src/puzzles/utility_reference.js`;
its new continuation behavior has not yet been exercised in the live game.

Navigation now binds verified script/local-worker labels to worker +9C's live
actor, deduplicates actor/worker targets, and excludes the actual player.
The offline audit covers 397 scripts, 2,460 useful targets, and 6,941 workers
without a useful player-entry outcome. Trial controls, exits, NPC dialogue,
services, save spheres and treasure remain. Names such as Talk with Auron
come from consistent speaker/model evidence; unidentified people/events retain
role labels. This is inferred labeling authorized by the user, not a claim
that every identity is known. Navigation/catalog behavior still needs live
confirmation. Exit destination names continue through native getters.

Source: src/native/utility_*.cpp, utilities.cpp, navigation_targets.cpp,
nav_exits.cpp; generators src/puzzles/build_utilities.py and
src/navigation/build_targets.mjs / finish_targets.py. Generated catalogs are
included with source; regenerating requires the preserved offline extraction.
The exact-hash native profile now has 179 individually verified addresses,
including the request initializer and utility APIs. The canonical x86 cl/link
build stages with `src/proxy/build_dinput8.bat --no-deploy`; install only after
FFX and its launcher are closed using `src/proxy/deploy.bat`.

Offline validation covers both reader profiles, native ABI, input ownership,
all six utility paths, repeat collection, original exit pairs, story preservation,
changed-code/entry rejection before writes, queue generations, and warp bounds.
It does not prove live animations, console speech/focus, or six actual item
collections. Current detailed build/test logs are install/native_utilities_*.log. Final combined reference checks:
install/cloister_checks_20261004_094742_396477.log. Deployment verification:
install/native_utilities_deployment_verified.json; working-reader rollback
backup: install/cloister_test_runtime/utilities_before_20261004_095911.
Timestamped contracts and label audits are preserved under
Docs/Scripts/Offline/output/FFX/20261003_audit/native_utilities_20261004_085539.json
and navigation_audit_20261004_094323.json. Profile evidence:
reader_reference_20261003_235502/reader_audit_20261004_090520.json.

The earlier repair is already installed: native features 127 and speech-ready
1 were observed, with accepted title/load/field-menu Tolk output. The user
confirmed launcher UI sounds and that threading/exit destinations worked.
The launcher closed after its normal launch. No Defender detection was found
in the queried two-hour interval; that does not establish every past JS error's
cause. The prior game-event trace was moved to its correct timestamped output
folder. The loaded-world save backup remains world_before_20261004_080536;
no restore was attempted while the user continued playing.

## Historical design checkpoint — Session 74


Session 74, 2026-10-03 local / 2026-10-04 UTC. The user clarified that the
goal is six playable in-game solutions and authorized Codex to operate FFX
and the RE tools directly. This supersedes the earlier user-run-only handoff.
The static designs and shared validation harness exist. All six live binding
and interaction pilots and the combined offline checks passed, but the
requested production puzzle guidance is still unfinished and not deployed.

The installed native reader fails its startup hook batch. Game captures also
remained blank during both proxy and stock startup comparisons. Native field
scripts continued running, so blank capture is not proof that the game engine
was frozen. Current evidence and remaining work are recorded below.

## Implemented assets

[trial_designs.py](../src/puzzles/trial_designs.py) defines each trial's normal
and treasure dependency phases, interaction workers, selected state bindings,
completion target and reset/revisit policy. The route prose is design material;
it is not player-facing game text or a verified sequence of movement commands.

[compile_trials.py](../src/puzzles/compile_trials.py) compiles the existing,
hashed event-script extraction. Each trial gets a design JSON, complete
worker-local control-flow JSON and readable flow dump. These preserve branch
guards, native call IDs/arguments, raw declarations, alias groups, writes,
REQ/JSR dependencies and source bytecode PCs. Unknown stack/register values
remain symbolic. Native calls are retained as effects requiring verification;
the compiler does not simulate the game engine.

| Trial | Script | Selected entry models | Dependency links | Design distinctions |
|---|---|---:|---:|---|
| Besaid | bsyt0100 | 57 | 1,074 | Glyph/symbol flags, sphere recesses and pedestal reset are separate. |
| Kilika | klyt1000 | 40 | 2,361 | Distinct sphere instances, flame guards and pedestal alignment remain separate. |
| Djose | djyt0600 | 152 | 514 | Socket transfers, charging, gap traversal, lift and upper switches require separate validation. |
| Macalania | mcyt0500 | 165 | 262 | Treasure dependencies can dismantle the bridge; its three supports must be restored. |
| Bevelle | bvyt1100 | 45 | 1,026 | Two pedestal slots, a 72-element state matrix and native rail direction windows; required and second chests are distinct. |
| Zanarkand | dome0700 | 142 | 1,361 | Pattern stages and revisit stages 5/6 remain separate; dome0200 has a linked flow model. |

The 601 entry models include setup/helper/story branches. They are not 601
player actions. The 6,598 links are request/subroutine dependencies, not a
serialized solution. Constant request targets resolve within these models;
conditional spawning, native side effects and complete spatial reachability
remain open. The other Dome maps retain hashed source/room metadata.

## Completion and treasure witnesses

These are source witnesses inside the compiled control flow, not proof that a
goal is currently reachable. PCs below are hexadecimal **bytecode offsets**,
not native function addresses. The design JSON retains the surrounding guards
and all alternate branches, including generic Djose/Macalania chest branches.

| Trial | Exit worker / PC → destination room | Treasure worker / call PC / ID | Flag write PC / mask |
|---|---|---|---|
| Besaid | 30 / 5B78, 5C35 → 103, spawn 0 | 16 / 2987 / 15 | 29B3 / 1 |
| Kilika | 37 / EEB1 → 45, spawn 0 | 7 / 3087 / 19 | 30C1 / 2 |
| Djose | 66 / 15300 → 90, spawn 1 | 24 / BBE1 / 484 | BB3B / 4 |
| Macalania | 45 / 12FC9 → 80, spawn 0 | 19 / D255 / 485 | D201 / 8 |
| Bevelle | 56 / 1CEC3 → 226, spawn 0 | 16 / 13753 / 217 | 13785 / 16 |
| Zanarkand | 33 / 4DBD → 318, spawn 0 | 32 / 48A5 / 209 | 48D1 / 32 |

The first five destinations map to their antechambers; Zanarkand maps to
dome0300, the Chamber of the Fayth. Bevelle's second chest is ID 102 in worker
17; it does not replace the temple-treasure witness. All six treasure masks
share declaration scope 0, offset 0xA70. Resolve that scope through the game's
getter rather than treating it as an absolute save-data address.

## Shared validation prototype

[state_reader.js](../src/puzzles/state_reader.js) observes native integer and
float writer returns and selected native CALL completion events. It reads
selected declarations through the game's getter inside game-thread callbacks
and returns only copied snapshots to the host. Map requests and resource-load
starts invalidate state, and late-generation returns are rejected. Repeated
identical writes remain distinct events. The default reader has no speech,
polling, timers or game-state writes.

Live validation corrected the x86 Frida ABI to `mscdecl` and established two
whole native profiles using their direct-call relationships. The reference
getter is RVA 0x46C500; this installed FFX build uses 0x46C570. Its integer and
float writers are 0x471400 and 0x4712E0, CALL return is 0x4777F0, map request
is 0x46FF40 and native input dispatch is 0x471D70. The resource loader remains
0x5093B0. These are individually verified entries, not a global RVA delta.
Unsupported or mixed profiles are refused. Short, NUL-padded script names
such as test20 are handled without treating the padding as UTF-8 text.

The source checks x86 FFX, live script-name/bytecode identity, chunk ownership,
declaration bounds and getter relationships. All selected fields were resolved
in every final pilot snapshot. This confirms the tested ABI and bindings;
it does not establish every source field's gameplay meaning or route validity.

Native decoder evidence remains archived FUN_0086C270/0086C500. Location is
`(raw_low >> 25) & 7`, type is `raw_low >> 28`, and offset is the low 24 bits.
Scopes 0/1 use controller bases, 2/3/4 worker data, 5 worker registers and 6
event data. Resolve declarations through native getters and retain alias owners.

[live_driver.js](../src/puzzles/live_driver.js) is appended only when explicit
RE test controls are enabled. Host requests queue one operation onto a verified
native input callback. Diagnostic event-count delays are confined to this tool.
It supports native room jumps, position changes, entry requests, copied worker
inspection, prompt acknowledgement and an explicit story-fixture seed. These
controls can affect autosaves; they are not a production puzzle implementation.
No story seed was executed in this session. Direct entry requests bypass outer
interaction availability and cannot prove legitimate player reachability.

Input confirmation must enter the native logical remapper at the correct
controller dispatch phase. Writing the ATEL held/pressed globals or repeating
unrestricted confirmation can stall prompts or remove/reinsert a sphere.
The diagnostic `ack` command is restricted to active message/choice windows
and drops confirmation when no dialog remains. Player position alone does
not establish facing or interaction availability.

## Run and inspect

From the repository root, regenerate all six designs and run the combined
offline checks:

```powershell
python src/puzzles/check_all.py
python src/puzzles/register_prototype.py
```

The existing RE launcher is `Docs/Scripts/Frida/run_frida.bat`; it is a
developer diagnostic menu, separate from the playable mod and original game
launcher. Its FFX `cloister_trials` entry launches the default read-only host.
`state`, `sources` and `quit` inspect copied data; they do not speak hints.
User authorization now permits Codex to run the tools directly. No requirement
to obtain access to each Cloister is being handed back to the user.

The explicit RE sequence host is [live_test.py](../src/puzzles/live_test.py).
[diagnose_live.py](../src/puzzles/diagnose_live.py) retains read-only native
disassembly, call-table, worker, window and input diagnostics. Back up saves
and the active proxy before mutable RE tests; preserve and verify them after
closing the game. Testing can trigger autosaves even without selecting Save.

## Current live evidence and limits

The combined pilot is
`Docs/Scripts/Frida/output/FFX/cloister_sequence_20261004_061222_196262.jsonl`.
All 66 commands completed; there were zero diagnostic errors and zero missing
selected fields. It visited all six native room/script pairs:

| Trial | Room | Script | Copied snapshots |
|---|---:|---|---:|
| Besaid | 122 | bsyt0100 | 41 |
| Kilika | 108 | klyt1000 | 14 |
| Djose | 214 | djyt0600 | 94 |
| Macalania | 239 | mcyt0500 | 76 |
| Bevelle | 306 | bvyt1100 | 13 |
| Zanarkand | 320 | dome0700 | 46 |

These pilots test getters, writes and native prompt paths with direct RE entry
requests. They do not validate normal completion, treasure routes or physical
availability. The story value remained the loaded context rather than each
trial's first visit. A shared temple_treasure value of 1 was carried from an
earlier Besaid test; it is not evidence that all six chests were obtained.

Earlier Besaid exploration is retained in sequences
`cloister_sequence_20261004_060106_951141.jsonl` and
`cloister_sequence_20261004_060642_636051.jsonl`. The former observed transfers
and treasure bit 1 but bypassed availability and used repeated confirmations;
it is not a solved-route proof. The latter naturally activated the first wall
glyph, but moving to the next wall without verified facing did not activate it.
Actor approaches, pedestal paths, charge/lift, bridge supports, timed rail
windows, complete tile patterns, resets and revisits still require validation.

The installed FFX.exe SHA256 is
`0537B2A1047F3266E73495CD4E35F63F0777F4231D417699F979954686DA686D`.
The mod's own native log reports `MH_ERROR_UNSUPPORTED_FUNCTION` at reader
RVAs 0x4CCEA0, 0x4D77C0, 0x4A2100 and 0x426770, then
`native reader batch failed`. Its entire feature batch is disabled. Other
accepted hook addresses must also be validated: acceptance does not prove a
function boundary, and this build differs at both functions and globals.
Repair and verify the installed native address profile before shipping spoken
puzzle guidance; changing only the four failing entries is insufficient.

Final offline archive:
`Docs/Scripts/Offline/output/FFX/20261003_audit/cloister_designs_20261004_062020_133594/`.
Log: `install/cloister_checks_20261004_062023_705913.log`. Eleven Python checks,
both six-trial native-profile fixtures, unsupported/mixed-profile refusal and
JavaScript syntax passed. This is offline validation, separate from live pilots.
The machine-readable live summary is
`install/cloister_test_runtime/live_validation_20261004_062020.json`.

The game and attached diagnostic hosts were closed. The pre-test proxy was
restored and SHA256 verified. All five save hashes match the pre-test backup;
the changed ffx_000 autosave was restored. Cleanup evidence is
`install/cloister_test_runtime/cleanup_verified.json`. No C++ puzzle guidance,
native production build or new deployment occurred during this session.

Tracked implementation is under `src/puzzles`. Extracted evidence, generated
designs, registered launcher wrappers and logs remain local and Git-ignored
under existing project policy. A fresh checkout needs that evidence copied or
regenerated. References retain attribution to Karifean/FFXDataParser,
Fahrenheit and the existing native decompile archive. Authorized game/Frida
tests were performed; no new Ghidra analysis, native production build or
deployment was performed. The requested six playable solutions remain open.

## Launcher and native profile repair - 2026-10-04

Latest user reports supersede the capture/launch assumptions above. The lid was
closed during prior blank captures; the user saw the actual game at title with
the lid open. The manual direct game launch left the original launcher/music
running. Title had no speech. Notify the user before launching or taking UI
focus; they are working on other tasks. Do source/offline work without focus.

The game proxy and Tolk initialized; four unsupported reference hook targets
aborted all readers. This establishes an address-profile failure after injection,
not a missing launcher-to-game DLL handoff. The read-only Ghidra export contains
reference SHA256 3d13e5ed53821df8db204cd3c27d470c5c220b18e16849fc4440996c15f277e1;
the installed build has the 0537b2a1 hash above. Individually matched x86 bodies
and reference contexts resolve all 174 required addresses, with zero unresolved
addresses/conflicts. Audit: reader_reference_20261003_235502/
reader_audit_20261004_071020.json. No new Ghidra analysis/import was performed.

src/native/profile.hpp/.cpp now select the exact executable SHA256, translate
verified code/data RVAs and normalize callback identities. Unknown executable
hashes and requested unverified RVAs are refused. No blanket delta or polling.
Both reference and installed-profile production fixtures pass all 65 hook-site
flows; the executable-hash guard rejects an unsupported fixture. Full native
suite: install/native_profile_tests_20261004_checked.log. A negative crash exit
was initially missed by the batch runner; checks now require exact zero status.
The installed-profile fixture used an unverified opaque draw address; it now
uses a synthetic callback in the fixture range because Increase ignores it.

The C++/CLI launcher adapter invokes the original OnMouseMove/OnMouseClick paths
for keyboard hover and selection. Original FMOD sounds, command arguments and
RunExeAndExit close-after-confirmation behavior remain owned by the launcher.
Startup establishes Application.StartupPath as cwd for original relative game
commands. Isolated launcher/installer checks pass, including original hover and
select counters and cwd. Log: install/launcher_repair_fixture_20261004.log.

The reader/launcher repairs were deployed and their live evidence is recorded
in the current implementation section. Long manual route validation was
subsequently replaced by the user-authorized utility task above.
The canonical build uses x86 cl/link and C++/CLI; never build the abandoned C#
port or decompiled SDK reference project. The user-reported dotnet dialog's
cause remains unresolved; Windows recorded a compiler-server leak warning.
