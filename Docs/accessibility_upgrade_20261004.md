# FFX accessibility upgrade — 2026-10-04

## Implemented

2026-10-06 NPC/animation follow-up installed: concise actor-local names, services and sprite
roles replace generated interaction phrases; events use script function/text
labels. Belgemine reward calls no longer make her a treasure chest. Existing
native UI events own dialogue/prompt speech. Field actor/movement callbacks skip
battle/scene navigation work and banner/original-span/reader timings are logged; animation
delay remains unconfirmed as fixed. Current verification/deploy: debug.md.

2026-10-05 orientation/navigation update installed: F2 selects a Standard section
or Expert stat focus; F4/F5 describe branches/endpoints; O reads the occupied
Standard section and nearest forward/backward branches on both layouts. F3
validates cumulative Key Sphere stock and movement levels from the actual pawn.
Grid keys are contextual and Overdrive auto-success is in F8. Shop descriptions
use visible panels; native active flags/current entries and interaction zones
drive navigation. Full native suite, final x86 build, both compiled profiles and
13 installed hashes pass. 397-script/646-exit audit retained; live acceptance
remains. Current deployment evidence/hashes: debug.md's orientation entry.

2026-10-05 interaction/log follow-up installed: book/shop choices follow explicit
native row anchors; full current-page dialogue and inserted native text are read.
Current logs remain beside DLLs, with twenty older reader logs under logs.
User confirms movement works; new preview-start "Preparing to move" covers the
quiet delay before confirmation. Native x86 build, full suite, both compiled executable checks and 13 installed
artifact hashes pass; fresh gameplay acceptance remains.


2026-10-05 movement confirmation follow-up installed: native prompt and Yes/No
selection, I prompt read and explicit "[character] moved to [node]" confirmation
with actual level changes. Both reader profiles, archived signature checks,
x86 build and compiled deployment checks/13 hashes pass; user confirms movement works.
Evidence: Docs/debug.md.

2026-10-05 Sphere Grid correction: native inventory disabled-byte polarity is
fixed; unactivated nodes announce the required sphere and quantity, including
lock keys. Both native reader profiles, planner regressions and x86 build pass;
deployed with both executable checks and 13 installed artifact hashes verified.
Fresh-start spoken acceptance remains pending. Current evidence: Docs/debug.md.

2026-10-05 menu follow-ups supersede earlier coverage notes: Status arrows read
only the current displayed page; naming speaks letters/controls and F11 reads the
entered name; item targets announce initial/all-party focus with only visible
HP or MP; equipment slots and grid exit choices have native speech. Backslash is
now connections/type-counted spheres/levels, Shift-Backslash only direction/node.
Controller refresh covers the save/load loop. Background Tolk/log workers avoid
blocking game callbacks; full load-time acceptance still needs a new run.
Current verification/deployment evidence: Docs/debug.md's current entry.

The active native FFX reader now has the FF12-style spoken accessibility menu,
SDL3 controller ownership and navigation audio. The menu exposes speech/braille,
combat descriptions, autodetail, beacon/wall/arrival sounds and
their volumes/range, existing Overdrive timing assistance, controller bindings,
audio previews and context-sensitive Cloister commands. Settings and swapped
controller assignments persist beside the executable in `FFXAccessibility.ini`.
F8 opens it without moving focus to another window. Arrows, Home/End, Enter,
I and Escape navigate it. F6 toggles beacon; F7 toggles the persisted Autodetail
setting; F11 repeats the last copied game speech. F9/Shift-F9 read/advance the
puzzle step, and F10 reads its path. N remains a manual-description alias.

### Description coverage requirement — 2026-10-05

The user requires **I** to read descriptions for highlighted menu entries,
items, shop items and equipment, and **Autodetail** to read those descriptions
automatically on highlight. Autodetail must be available in the **F8 settings
menu** and toggle with **F7**; beacon toggles with **F6**. These keys are wired
to the existing shared native description cache and saved `auto_detail` option.

The 2026-10-05 description/grid implementation below completes the native
description producers identified in this handoff. Check manual I and highlight
narration with Autodetail on/off in each actual menu before live acceptance.

SDL3 owns the first available gamepad. View/Select arms one mod command; View then
Menu/Start opens settings. East face cancels command mode. Mod actions drain until
neutral. Closing settings or regaining focus also drains held controls. The game
receives SDL snapshots through its own XInput decoder; named and ordinal imports
are supported. Its DirectInput controller reads are neutralized while SDL owns
the pad. Device creation must originate in the game executable, so shared COM
method hooks cannot suppress SDL's own DirectInput devices. Keyboard game controls remain available; captured mod/menu keys are
removed from both Windows messages and DirectInput state/buffered reads. Game
vibration goes through SDL. With no SDL pad, original controller reads resume.

Default mod bindings (after pressing View/Select):

| Control | Action |
|---|---|
| North face | Scan nearby entities |
| D-pad left/right | Previous/next entity |
| D-pad up/down | Previous/next category |
| West face | Distance and direction |
| South face | Request collision route |
| Right stick click | Toggle beacon |
| Left/right shoulder | Current turn / party HP and MP |
| Right stick up | Description |
| Left stick left/right | Previous/next status detail |
| Right trigger | Sphere Levels |
| Right stick down/right | Puzzle step / path |
| Left stick click | Repeat speech |
| Right stick left | Send the game's Select/Back button |

Controller bindings support face/shoulder/D-pad/paddle buttons, triggers and
stick gestures. View, Guide, Menu and East face stay reserved. Assigning an
occupied control swaps assignments; Escape/East cancels learning.

Entity routes and puzzle-control paths share one route planner and beacon.
The beacon follows copied collision-route waypoints; stereo shares the spoken
navigation camera basis, behind lowers pitch, and distance controls ping spacing.
Bravely Default's objective/arrival samples are embedded. Wall tones test exposed
native mesh edges ahead/left/right/behind. Arrival is announced once. Native
collision commits refresh cues and moving actor targets; audio callbacks only
mix copied state. Menu, battle, scene, focus loss and teardown pause cues. Audio
is opt-in: beacon and walls default off; arrival defaults on and only sounds
while beacon is on. F6 and settings-close events immediately refresh the current
route audio; disabling beacon cancels arrival audio without disabling wall tones.
2026-10-05 native audio/route/menu regressions and installation checks pass;
live audio acceptance remains pending. Evidence: Docs/debug.md, beacon repair.

Coverage repairs include:

- ESC audio/graphics focused labels and values, keyboard/controller assignment
  rows, and changes read through native localization and binding catalogs.
- Shared item/command/Sphere Grid descriptions; ordered equipment abilities,
  empty slots, and separately labelled visible current/selected shop panels.
- Sphere Grid signed cursor index, native node definitions/descriptions,
  character-specific activation bits, sphere availability and activation events.
- First status application and cures, duration-presence changes, KO/revival,
  Cheer/Aim/Focus/Reflex/Luck/Jinx levels, Doom, capture and other native effects.
  Nested clears belong to their outer effect; equal text from separate events
  remains speakable.
- Silent native action titles, battle failure messages and queued item/ability/
  gil notifications. Reward AP uses earned AP rather than the animation counter;
  key-item rewards and inline equipment abilities are retained.
- Explicit route failure reasons and movement-blocked events. Floor choice uses
  height and nearby edges; continuous segment coverage prevents smoothing across
  narrow gaps. Actor movement uses the actual controlled actor, including a
  nonzero actor slot. A* uses a priority queue.
- One speech queue centralizes speech settings, modal capture, repeat and copied
  text. Both enqueue and playback reject disabled/captured game speech. Tolk_Output
  remains the sole speech/braille backend.

## Evidence and verification

Discovery used the retained Ghidra decompile and reference binary archives, plus
the user's FF12/Bravely sources. No new game/Frida/Ghidra session was executed.
`Docs/Scripts/Frida/FFX/accessibility_events.js` is the registered diagnostic
prototype for the user-run launcher workflow; it does not speak or poll. It has
not been executed against gameplay in this session.

New addresses remain restricted to the two existing exact executable hashes.
Eleven independently matched signatures and instruction-derived globals resolve
transactionally before any hooks. The identical tiny binding catalog getters are
anchored to verified parent call operands. Ambiguity or an invalid operand refuses
startup. Archived reference uniqueness evidence is retained as
`Docs/Scripts/Offline/output/native_extensions_20261005_040727_168066.json`.
An isolated fixture verifies independent shifts, anchors, derived globals and
refusal without publishing partial mappings. Installed live signature matches
remain unobserved.

Final check logs are under `install`: `native-tests-session.log`,
`launcher-tests-session.log`, `menu-events-session.log`, `charset-session.log`,
`atel-session.log`, `puzzle-tests-session.log`, `prototype-build-session.log`,
`state-reader-session.log`, and the canonical native build logs. SDL controller tests use a real x86 SDL
virtual pad and the production XInput bridge. Audio uses the real production SDL
callback with a dummy device; this verifies initialization/lifetime, not audible
headphone quality. Reader fixtures exercise synthetic memory for both profiles,
not live menu layouts. Production C++/headers remain below 500 lines each.

## Next session: remaining core work and live acceptance

This is an FFX implementation pass. FFX-2/Last Mission still lack independently
verified native profiles/readers; their accessibility is not implemented by this
update. Do not infer their offsets from FFX.

1. On a user-launched game, confirm all native extensions match and the reader
   enables. Check launcher/title, ESC audio/video/key/controller rows, field
   menus/shops/save confirmations and grid descriptions/activation. Investigate
   any new report even if earlier notes called that feature solved.
   Validate I descriptions and F7/menu Autodetail against the description coverage
   requirement above, especially item, shop and equipment selections.
2. Exercise special abilities (Steal/Mug/Bribe/Capture/Lancet/Mix/Blue Magic,
   swaps/summons and right battle menu), native failure text and temporary/stacked
   effects. Dynamic inserts in encoded battle text need the game's final
   formatter if the existing typed queue does not supply their values. Wakka's
   reels and Rikku's Mix feedback/assistance need separate native-event coverage;
   assistance currently covers only Tidus/Auron/Lulu.
3. Validate the new F3 ability planner, backslash path/material totals,
   Shift-backslash next connection, O orientation, ability-node I descriptions
   and actual stat deltas after activation. Test Standard and Expert grids,
   character switches, locks and previously travelled links.
4. Add native Scan/L sensor enemy weaknesses and complete CTB order review.
   Current target speech retains the prior name/HP percentage behavior; this
   session does not claim complete enemy analysis or full turn-order narration.
5. Split reward speech across the actual AP and loot result transitions. Earned
   values and loot are corrected, but the current reader still speaks the whole
   reward packet on opening. A draw hook or passive watch is not an acceptable
   transition trigger.
6. Walk routes on stacked floors, elevators/rails, tight walls, camera changes,
   moving NPCs and disconnected targets. Verify camera/pan convention and audio
   balance with headphones. Check SDL hotplug, reconnect and native game
   controller rebinding with the user's physical devices. Only one active pad
   is bridged; a device-selection setting is a useful follow-up.
7. Concise NPC/event labels are implemented using source-local dialogue,
   service/script functions and sprite families. Verify live conditional identity
   and labels. Actual icon display, scripted rides and camera-change writers
   still need event/call-context proof; no full prompt is inferred from proximity.
   Manual all-six Cloister guides remain pending gameplay acceptance in `cloisters.md`.

Do not launch, focus or warp the game without fresh explicit permission.
Minigames and optional side activities are intentionally deferred to
`side_content_plan.md`.

## Alignment-pass result (before the 2026-10-05 shortcut update)

Both canonical x86 development and release builds passed without C++ warnings.
The final combined native suite passed: copied speech/decoding and unsupported
hash guard, mixed x86 ABI/return override, transactional address discovery,
spoken settings/rebinding, real SDL virtual-controller/native import bridge,
dummy-device audio, route cues, 73 reader hook sites on both synthetic profiles,
geometry, Windows/DirectInput ownership, proxy forwarding and all-six utility
handoffs in development/release. Original launcher CLR/action/settings/installer,
JS menu/charset, 13 ATEL regressions, 11 trial tests, all-six state-reader fixtures
and offline prototype compilation passed. `git diff --check` passed.

The final default development build was deployed through `src/proxy/deploy.bat`
while all game/launcher processes were closed. Existing native backups and the
new `dinput8.dll.ffx_sr_before_sdl` backup were retained. Verification uses .NET
SHA-256 because this Windows PowerShell process did not expose Get-FileHash.
The five installed DLLs matched staging:

| DLL | SHA-256 |
|---|---|
| dinput8.dll | 2F970BA2FE75CA10845E4A66408E2E8D371BD56EBEA09D5F3D8348203920BF86 |
| SDL3.dll | 621BCC0A901D2A78251BB59823AA179E552BF9F865415C132051638B45233EA2 |
| MinHook.x86.dll | 1F28D13B8133551C8896CC556DB768C94EBFAAE0AD38E86719F3151F7B4CE1FC |
| FFXLauncherReader.dll | 6E7932F4E10273A6F46472B11904EF5B02826E5974479D2973B73013CFAD5A29 |
| FFXLauncherNative.dll | 1313D323C775DE19917087EB8B6B2F56947EBBA9FDD661D965962065BF3C283B |

## Shortcut update verification — 2026-10-05

The canonical x86 development build and the complete native fixture suite
passed after the F8/I/F6/F7 remap. Fixtures check independent beacon/autodetail/
repeat shortcuts, F10 path without opening settings, F6/F7 within the modal menu,
I setting/native descriptions, persisted Autodetail, F8 close/binding cancel,
and the real SDL controller's settings chord. Development/release puzzle utility
checks passed; no live game or launcher was launched or focused. Production
C++/headers remain within 500 lines; `git diff --check` passed.

The update was deployed through `src/proxy/deploy.bat` with game/launcher closed,
retaining existing backups. Build, checks and deployment evidence:
`install/shortcut-build-20261005.log`, `install/shortcut-tests-20261005.log`,
`install/shortcut-deploy-20261005.log`. All five installed DLL hashes matched
staging; these supersede the alignment-pass hashes above:

| DLL | SHA-256 |
|---|---|
| dinput8.dll | E1D44D846994269112FC302836DB8066A5D03080FF02320DE9F9AC566FCBAF81 |
| SDL3.dll | 621BCC0A901D2A78251BB59823AA179E552BF9F865415C132051638B45233EA2 |
| MinHook.x86.dll | 1F28D13B8133551C8896CC556DB768C94EBFAAE0AD38E86719F3151F7B4CE1FC |
| FFXLauncherReader.dll | 8EC84B7830F5153C671CB3DDEF2A4C7E6D01BE0CC300B68AF0A3B6D36A105C52 |
| FFXLauncherNative.dll | 76E70DDCF4CDB0D1601C98EFA9EC01E11269DCB4AC6B0CFB3575C72B2AE0D1A4 |

Deployment log: `install/native-deploy-session.log`. No game was launched or
focused for acceptance testing. Source/build verification does not establish
complete live accessibility coverage; the remaining work above stays open.

## Description coverage and Sphere Grid planner ? 2026-10-05

The user's renewed ability-node failure report reopens description acceptance.
The reader now resolves an ability node's native command ID (definition +12)
and reads its command description, rather than relying exclusively on the grid
node template. Both node and ability descriptions retain all native text lines;
manual I resolves the current grid selection again on the game owner thread.

| Surface | Description source/behavior |
|---|---|
| Main menu | Native namespace 9 name/description record |
| Horizontal command bars | Native per-row help pointer (+5C, stride C) |
| Items, Key Items, field/battle abilities | Native record description |
| Gear, Customize, gear shops, battle equipment | Rendered ability names and empty slots; shop current/selected panels labelled separately, with no generated stat comparisons or expanded catalog effects |
| Overdrive modes | Native namespace 1 description record; character rows use current saved HP/MP/S.Lv |
| Formation, character/Aeon selection | Current saved HP/MP/S.Lv |
| Aeon Learn/Increase | Native ability/stat description plus copied material requirements |
| Aeon attributes and status review | Native attributes/learned abilities; status gear rows include visible names/slots |
| Shop items and quantity selection | Native item description; live names, price, quantity remain primary narration |
| Config and ESC audio/video/bindings | Native description/localization; focused values/binding names retained |
| Save/load choices and other confirmations | The current native prompt, replayable with I; moved choices obey Autodetail |
| Help pages and native choice windows | Copied current page/prompt; opening narration has one owner |
| Grid nodes/action list/sphere inventory | Native node/command description, action help, and native sphere-specific usage text |

Gear descriptions have one producer: the highlight reader updates the shared
manual cache and optionally speaks it according to Autodetail. The old independent
equipment-abilities producer was removed. Battle descriptions use the same cache;
Combat detail and Autodetail enable one automatic utterance. An explicitly empty
selection reports no available native description on I, instead of falling through
to a stale battle buffer. Scene transitions invalidate the cache. No new
text-content deduplication was introduced.

F3 opens a spoken, captured modal planner directly for the native grid's active
character; no character-selection step is required. The list contains the actual
loaded grid's ability nodes, including any native
learned-ability ID, and excludes stat/empty/lock nodes. Names/descriptions come
from native getters. Repeated ability names at different nodes remain separate
rows, distinguished by stable node numbers. Enter saves a destination for the
active character; it does not change the game's selected character or node.
Escape closes, I reads the ability, and Home/End/Page Up/Page Down browse.
Destinations last for this loaded session and clear on resource/map invalidation.

Backslash recalculates from the actual pawn position for the grid's current
character. Field backslash retains its existing entity-route behavior. Reports
include each connection, Sphere Levels to reach a connected activation neighbour (and separate standing-on-node cost), required
lock keys and destination activation sphere, owned quantities and deficits. Stat
activation along the route is optional. F3 validates cumulative owned Key Spheres
and available Sphere Levels before claiming accessibility. Paths minimize
movement costs among key-valid routes. Locked routes include
separate movement confirmations before clearing locks. Native 659760 confirms that activation can target the current node or an immediate connected neighbour, so the final ability link does not require movement. Teleport/special-sphere
shortcuts and optional stat purchases are not part of this ordinary-link planner.
Shift-backslash reads only the next connection. O gives stable node numbers,
character position, fixed native grid coordinates and neighbouring node bearings.
Those bearings describe data-space directions, independent of camera rotation;
they do not promise that the same screen arrow remains correct after rotation.
Activation feedback compares actual native stat values before/after the mutation.

F2 chooses a character section on Standard, or one of ten native stat categories
on Expert. F4/F5 describe forward/backward native connections, branches and
endpoints; Standard follows the occupied section after an F2 entry is reached.
Expert stat guidance advances only after the character actually activates the
boost. F3 preserves that focus. O additionally names the occupied Standard
section and nearest forward/backward junctions on both layouts. Layout metadata
is accepted only after all node coordinates/presence and undirected links match.
F2/F3/F4/F5/O/S use an owner-event cached grid context; O no longer changes
Overdrive assistance, whose saved setting is in F8.

Discovery used retained decompile/binary archives only. The read-only Frida
prototype `Docs/Scripts/Frida/FFX/sphere_planner.js` is automatically registered
by the existing launcher directory discovery; its route logic passed offline
fixtures before the native implementation. It is not a production speech owner
and was not executed against a game. New sphere-definition global 16860E4 is
instruction-derived from the full, independently unique 649310 body. The shorter
signature was ambiguous with 6493B0 and was rejected; all 151 bytes resolve it.
Evidence: `Docs/Scripts/Offline/output/native_extensions_20261005_121400_853346.json`.

The full native suite passes, including read-only planner/filtering/modal fixtures,
character-specific travel history, four travelled links per level, locked route
rounding, disconnected graphs, missing spheres, fresh-highlight recalculation and
both native profiles. Reader fixtures cover manual/automatic items/gear/bars/main
menu/confirmations, multi-line stat-node descriptions, ability-node lookup,
native sphere/key catalog resolution and stat deltas. Final reader/build evidence:
`install/descriptions-grid-readers-20261005.log`,
`install/descriptions-grid-planner-20261005.log`,
`install/descriptions-grid-tests-20261005.log`, and
`install/descriptions-grid-final-build-20261005.log`.
No game was launched, focused or probed. Live acceptance is still pending.

### Installed verification

The final canonical x86 development build/deploy completed with game and launcher
closed. Build output has no C++ warnings/errors. The deployment script verified
all five installed DLL hashes against staging and retained existing backups.
Latest evidence: `install/descriptions-grid-deploy-20261005.log`; offline Frida
logic: `install/descriptions-grid-prototype-20261005.log`. These hashes supersede
the earlier shortcut-build hashes, without claiming live gameplay acceptance.

| DLL | SHA-256 |
|---|---|
| dinput8.dll | 4CECF8227A98E38366552666AC1795DABBBAB23402A89CFE0667028E386F0011 |
| SDL3.dll | 621BCC0A901D2A78251BB59823AA179E552BF9F865415C132051638B45233EA2 |
| MinHook.x86.dll | 1F28D13B8133551C8896CC556DB768C94EBFAAE0AD38E86719F3151F7B4CE1FC |
| FFXLauncherReader.dll | 44284ACA81B8748172ED9EE94234D2DDF406C19CD78CBD1E96F5F08BCB5D7FB4 |
| FFXLauncherNative.dll | 8AB2ED9829242B61CAEDE9E685F99FF4E1F15052D487096ECB0D7B906C89F1B4 |

User acceptance: with Autodetail off/on, select items, gear, shops, command bars,
Overdrive/Aeon/config/confirmations and press I. In the grid, compare a stat
node and ability node with I. F3 opens the active character's abilities; Enter saves a destination,
backslash reads the path/materials, Shift-backslash reads the next connection,
and O reads position/connections. Compare a new/travelled link and locked path;
verify the character highlight and actual position distinction. Standard and
Expert grids and real controller/audio behavior still need gameplay acceptance.
