# Player follow-ups — 2026-10-07

This follow-up was deployed at the user's explicit request after confirming
closed game/launcher processes. Both compiled profiles, launcher bootstrap and
all 13 installed hashes pass (`install/review-20261007/followup-deploy.log`).
The earlier running session still used the older build. That installed build has gameplay
confirmation for Primer labels and Auto walk. New behaviors need separate
user confirmation after a closed-game deployment.

## Configured push action and side guidance — 2026-10-07

User evidence: Space activates pushing; Confirm tries to operate the sphere.
This supersedes the held-Confirm instructions below for movable pedestals.
The corrected native build is deployed. Basic Auto walk and Primer labels have
prior player confirmation; these new push changes require gameplay acceptance.

Assistance now reads configured walk/push bindings (Space in the user's tested
setup), announces actual key/button names, current side and required side,
and preserves native single-action Confirm for upper Djose/Zanarkand controls.
Besaid/Kilika use actual DI/pad movement into contact, including outward staging
when already touching. Their first owned script read receives the correct
inverse-mapped direction immediately. Current-segment geometry stays stable
through alignment tolerances; directions/required side update at turns.
Release, manual action/movement and context guards retain priority; native
scripts own all state, movement and animations. The threading/title fixes remain.

Verification: 32 native checks passed before isolated utility link stubs needed
updating; both remaining development/release utility checks then passed. The
focused Space/contact/remap fixture, six label tests and 11 archived trial checks
pass. Canonical tester build is warning-free; guarded deployment verifies both
compiled profiles/all 28 dependencies, launcher and 13 installed hashes. The final
payload forwarding/input checks and installer evidence are recorded in debug.md.

Installer: refreshed 0.5-Test Build ZIP has one self-contained Windows EXE,
README/CHANGELOG/CONTROLS and checksums. No BAT/PS setup helpers or loose DLLs
are distributed. All 13 embedded resources match their source artifacts; the
actual EXE installs them into a synthetic directory preserving settings/XML.
Previous local package is preserved under install/release-backups. User-side
screen-reader/folder-picker and gameplay checks remain pending.

Delegation tracking:

- Sol (`gpt-6.1-sol`), native investigation/review: verified mapped held input,
  walk action and contact path; corrected its earlier mistaken config-ID offsets
  using producer/getter bodies and independent label tables. Primary cross-checked,
  integrated and tested. Default assignments cannot be proven from the archive;
  current names come from live configuration. [Evidence](pedestal_input_re_20261007.md).
- Sol (`gpt-6.1-sol`), installer implementation: passed 201 synthetic assertions,
  including rollback, process refusal, embedded resources and accessible controls.
  Primary caught a trailing-newline parser error; Sol fixed/retested it. Primary
  verified the final real payload/installer and archive. Live accessibility pending.
  [Evidence](windows_installer_20261007.md).
- Luna (`gpt-6-luna`), bounded release/control documentation: updated four files.
  Primary caught an incorrect F11 description; Luna corrected it. Primary reviewed
  the result, finalized setup/status wording and verified packaged docs match source.
  No code/build/gameplay validation was assigned to Luna.

## Earlier held-Confirm pedestal assistance — superseded by correction above

The first held-Confirm build caused the user's startup VC++ termination:
the new interpreter RVA lacked an Installed mapping and threw outside startup
guards. That regression is repaired and the corrected build is deployed.
Exact signatures/operand-derived globals, complete feature dependency validation,
exception containment and16-bit input accesses now have focused regressions.
Both compiled profiles/dependencies, launcher and 13 hashes pass; game startup
recovery and pedestal gameplay remain unconfirmed. Current evidence:
[runtime repair](debug.md#vc-runtime-termination-after-pedestal-build--2026-10-07).

The user authorizes ordinary in-mod assistance while holding configured Confirm.
This follow-up supersedes the earlier Besaid/Kilika-only push limitation below.
It does not authorize agent gameplay or change the optional native trial solver.

The central `pedestal_*` modules describe all six trials from the original hashed
scripts. Besaid, Kilika and lower Djose announce camera-relative directions and
remaining distance (11.13 native units per navigation step). Macalania, Bevelle,
upper Djose and Zanarkand announce one native action and its direction. Each
autonomous animation is one push; rail junctions and floor patterns remain
separate original actions. F11 reads the selected guide's current push guidance;
Backslash and route arrival read it for the selected pedestal.

With Auto walk enabled, Backslash arms the current pedestal action and walks to
its proper side. Holding native Confirm supplies movement consent. A shared
coordinator suppresses sphere Confirm and supplies the original directional
input while the original script moves the pedestal. Scripted and single-Confirm
actions stop after one action and drain held Confirm until release. Continuous
multi-leg pushes use ordinary route steering to change sides, with actor-clear
detours. Release, physical manual input, dialogue, menus, scene/battle, map or
focus changes cancel. A committed original animation can finish after release;
the mod never restores player control, writes puzzle flags, warps actors or calls
the shortcut solver.

Kilika, lower Djose and Macalania now retain separate **Pedestal sphere recess**
rows. Recess and push identities remain distinct through selection, rescans,
moving-target updates and route refresh. Besaid's existing separate socket is
retained. Bevelle stops at safe staging before its entrance trigger; only held
Confirm changes to the actual crossing, including subsequent live route refresh.
Its optional upper push survives the original 21 one-frame probes, stops held
direction on native commit, and completes on the committed callback return.

Confirm is derived by inverting the native button mapper, then reading primary
and alternate keyboard bindings and physical controller bindings. No hardcoded
Return/South assumption is used. Physical controller input is observed before
mod routing filters it; movement priority survives exclusive and consumed inputs.
The native input owner updates the coordinator after aggregate collection. Only
exact verified interpreter PCs, signal/entry/level, chunk and generation may own
player-control disable. When inactive, completion callbacks read no trial state.
Existing callback serialization/loading scopes and the title tail-return repair
remain intact.

Astra (`gpt-6-astra`) investigated the distinct trial rules in
[movement evidence](pedestal_movement_re_20261007.md). Sol (`gpt-6.1-sol`)
investigated input and reviewed integration in
[input evidence](pedestal_input_re_20261007.md). Primary cross-checked archives,
implemented and reviewed the code, and owns verification/deployment. Sol found
the Bevelle probe/commit and stable-crossing cases, exact single-action ownership,
anchor guidance and pre-filter physical cancellation; those fixes have dedicated
offline regressions. The original 019F eligibility body and some common-call
dispatch identities remain unresolved; original game checks are preserved.
No runtime validation or game control was performed. Final build/deployment
evidence is recorded in debug.md and `install/review-20261007/`.

Final status: implemented, offline-verified and deployed. Full native suite has
32 PASS summaries and zero warnings/errors/failures; final focused input checks
and six label checks pass. Canonical game/launcher build is clean; both compiled
profiles, launcher bootstrap and all 13 installed hashes pass. Staged/proxy/game
DLL hashes match. Exact record: `pedestal-push-verification.json`; matching final
DLL/map and `pedestal-push-*` logs are retained alongside it. Gameplay confirmation
remains pending. Test with Auto walk On: select Pedestal, Backslash, hold configured
Confirm; release or manual movement must stop assistance. Each animation ends its
claim and requires a new route request for the next action.

## Earlier contact-only correction

The user still could not push Besaid after the deployed talk-priority repair.
The own log records Besaid Sphere insertion at 12:03:15 and the final guide step
at 12:04:17, so the initial failure was not a skipped sphere prerequisite.
Later angle/Confirm attempts removed the sphere, explaining subsequent inactive
push state. The final guide now checks that the Besaid Sphere is still inserted.

Verified original script: Besaid worker 7 entry 4 (PC 4749) is a physical-contact
callback. It reads directional buttons/stick and requires sustained direction
for 15 script frames before moving. Entry 2 is the ordinary Confirm handler;
its narrow sphere-handling branch is not the held-movement push. Sphere insertion
in worker 44 sets fields 0x11E50 and 0x11E64 to 1 and enables worker 7
channels 0, 1 and 2.
No further activation step was found in this verified sequence. Kilika worker 6
entry 4 uses the same sustained-movement mechanism. Its authored navigation mask
contains only entry 2; the shared push helper therefore reads the actual native
contact channel independently of that ordinary sphere-operation mask.

Native evidence: `layer2_game_logic.txt` function 83D3B0 (RVA 43D3B0), lines
441174?441251, computes collision radius as the average of the two horizontal
actor scale components multiplied by its base-radius field, sets contact state
on overlapping actor pairs and dispatches channel 2 through
476420/476470. Cached actor+4E8 can be stale after initial spawn/scale changes;
we recompute its exact live formula. Conditional height overlap applies only
when both actor+194 flags have 40000000: top +698; bottom +6A8 if +51C is zero,
otherwise actorY minus +51C. The helper derives the allowed player-Y interval
from those native bounds. 43ECD0 resolves the pair in X/Z. Archive 43E3E0 loses
its square-root argument details; exact independent distance recovery is limited.

Shared implementation: `configurePedestalTarget` marks only verified Besaid/Kilika
controls; `refreshTargetZone` reads live contact enablement, actor position and
collision bounds. Ordinary navigation, moving-target refresh, puzzle guidance,
arrival and final facing share this contact zone. Besaid approaches from +Z,
matching final -Z pedestal travel. Kilika chooses a reachable cardinal face.
The endpoint is .25 units outside contact; .8 radial and .65 lateral arrival
margins are mod tolerances, not native constants. Sphere recesses retain their
own talk geometry. Arrival/Backslash announces the ordinary movement direction
to hold, and the two guides distinguish movement pushing from Confirm.

The floor mesh omits live actor collisions. `contactRoute` validates each route
leg against the target cylinder; if necessary it tries both connected-floor arcs
around a 16-point clearance ring. Opposite-face approaches cannot take a direct
path through the pedestal. The ring's 1.5-unit clearance covers ordinary waypoint
advance tolerance. It never changes actor position, script flags or game input
semantics; normal manual cancellation remains intact.

Sol (`gpt-6.1-sol`) performed read-only native/script investigation and reviewed
the central change. Primary cross-checked the log/script, integrated fixes and
owns verification. Sol caught stale radii, an unsupported initial vertical limit,
the opposite-face actor-crossing risk and availability-only stale route state;
all were corrected. A final follow-up review confirmed radius/height algebra and
mesh detour segment clearance. Open-plane detours and inactive-target invalidation
have dedicated regression checks. Call 019F(1) remains
an unresolved eligibility gate and is not presented as a collision getter.
Djose/Macalania use distinct callbacks/side-state guards; their exact custom
channel mapping is not established, so their controls remain unchanged.
Bevelle/Zanarkand likewise retain their rail/tile handlers. No game launch or
runtime probes were used.

Implemented, offline-verified and deployed after the closed-process guard passed.
The complete native suite passes (31 PASS summaries plus forwarding/runtime
success reports), with zero compiler warnings/errors/failures. Six navigation
label tests, focused contact geometry and both archived-script utility fixtures
pass. The canonical x86 game/launcher build is clean. Guarded deployment verifies
both compiled executable profiles, launcher bootstrap and all 13 installed hashes;
staged/proxy/installed DLL hashes match. Evidence: `install/review-20261007/pedestal-verification.json`,
`pedestal-full-tests-complete.log`, `pedestal-geometry.log`, `pedestal-utilities.log`,
`pedestal-build.log`, `pedestal-deploy.log`. Gameplay confirmation remains pending.

Early fixture compilation exposed a string-literal concatenation mistake and a
missing test `<algorithm>` include; both were corrected. Editing the test batch
while it was executing caused a late harness path failure. The stable runner was
then rerun completely and passed. Do not edit running batch scripts. These were
offline test/build failures; no game was launched or runtime crash produced.

## Pedestal selection and trials

Archived RVA 466920 scores circles by distance squared; 466C60/4665B0 score
rectangles/lines by total interaction reach squared. The facing check at
48B6F0 also determines eligibility. Being inside a pedestal's circle is
insufficient if an overlapping recess wins the native selection.

Besaid worker 7 is the pedestal actor and worker 44 its rectangular sphere
recess. Labels and guide targets were distinct; the old route stopped in the
larger circle's outer range where the recess won. The central helper now copies
live competing talk controls and accepts approach points only when the selected
worker wins while the player faces its center. Exact ties are rejected
conservatively. Automatic arrival uses the same predicate and native facing
setter 42B230. It never injects confirm input.

Ordinary destinations and guide paths share the helper across Besaid, Kilika,
Djose, Macalania, Bevelle and Zanarkand. Authored all-six labels and guide targets
were rechecked. Native flags/movement events refresh competitors; competitor-only
changes can replan actor pedestals and non-actor recesses. Tests cover distinct
actionable endpoints, facing cones, ties, live changes and final facing.

## Speech and settings

F8 uses Navigation audio and Navigation submenus, explicit Back and parent-row
restoration. Speech is first; voiced output stays in Main. Navigation includes
Auto walk, Speak auto walk path, Hide unreachable and Partial steps. Path narration
saves as `auto_walk_path`, default Off: only “Auto walking to [destination]” starts
the walk. On reads concise directions first. Ordinary directions still speak when
Auto walk is disabled. In-range facing is compass-only. Menu indexing defaults Off.

Native TK Al Bhed windows call Primer-aware converter 506780 under the exact
rendering flag. Untranslated glyphs retain its cipher output; collected glyphs
use readable output. Ordinary dialogue bypasses conversion. Separate event
subtitle expansion retains its existing producer; no unverified cipher flag
was invented for it. Gameplay checks should include windowed and event dialogue.

T owns a separate current-dialogue page/event cache, advances on native page
changes and expires on closure/control restoration. Selectors and swimming HUD
text cannot become dialogue replay. F11 reads the selected guide step in trials;
Shift+F11 reads inferred state. Naming and outside-trial behavior retain their
own contexts.

The user's `FFX_SR_Native-20261007-091609-864-000.log` shows the Cheer tutorial
caption immediately followed by interrupting sphere focus. Tutorial ownership
now defers forced sphere focus while its caption is active. Focus changes replace
older pending focus; the latest focus and description queue non-interrupting on
release. Deliberate input retains normal cancellation. No speaking timer or new
text-content deduplication was introduced.

Button tokens are decoded centrally using native configured keyboard/controller
names and regional confirm/cancel configuration, with physical glyph fallbacks.
The Sphere Grid tutorial Status uses its native renderer and existing arrow
buffer, including opening HP/MP/stats. Quit's native popup builder announces its
localized prompt/buttons. Confirmed party costume stems label Rikku's salvage-ship
0029/RIKK_A model and other party variants. Collectible evidence distinguishes
Primer from readable Book; there is no global book-model rename.

## Swimming and console

Native mode setter 42B260 and actor gravity identify fixed-height swimming.
43D3B0/43E5F0 advance X/Z through native collision adjacency independently of Y.
Routes project only the actual current collision component onto the swimmer's
height; disconnected decks remain excluded. Verified free swimming without a
mesh supports direct X/Z routes. Other missing collision data stays unavailable.
Interaction heights still govern reachability; the saved unreachable filter can
hide inaccessible ship-deck targets. Mode changes retire old routes.

Map names now use the actual current chunk/saved entrance with native 46BD20,
handling multi-entrance swimming rooms before the unique-room fallback.
Extracted cdsp0000 string 107/cdsp0700 string 1 are nonblocking native swimming
controls with prefix 09:30. Return to ship/Dive are spoken with actual button
tokens and copied into Ctrl-arrow review: Left first, Up/Down browse, Right all.
View/Select then D-pad supplies controller review. Action buttons keep native
bindings; these controls are not fabricated physical route destinations.

Menu open/close/delete callbacks derive context from the active-window mask.
The old open-only state could retain scene 1 after grid closure and prevent
console chord capture. Development Shift+F1 is now reserved even in unavailable
contexts, preventing native F1 fallthrough. Opening the Win32 console performs
no field-state writes. Verified context/chord defects are addressed; the precise
cause of the reported swimming softlock remains unproven offline.

## Verification

Checks use synthetic x86 boundaries and existing extraction/decompilation only.
Deployment was verified without game launch, runtime probe or player movement.
Production C++ modules remain at or below 500 lines. Final logs/hashes are in
`install/review-20261007/`; [debug.md](debug.md) records final build status.
