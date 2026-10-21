# October16 Gagazet, battle HUD and grid controls

Implemented, offline verified and guarded-deployed. Kilika's preceding disabled
recess repair is now USER-CONFIRMED WORKING. New gameplay confirmation remains
user-side. Installer/ZIP remain held; reset heartbeat remains PAUSED.

Current proxy SHA256: `c0c0bcaa61ec22cb945069ae296dcf4fe15e53b3fa86fe3f0d124fbb39831101`.
[Deployment receipt](../../Analysis/FFX/RepositoryEvidence/historical-install/battle-panels-controls-deployment-verification-20261016.json),
[569-file stable source snapshot](../../Analysis/FFX/RepositoryEvidence/historical-install/battle-panels-controls-source-20261016.json).
Canonical development native/original-launcher build has zero warnings/errors;
compiled actual-executable Preflight/Installed checks, seven installed hashes,
15 locked PRISM files/receipt and unchanged user INI pass. Held0.5.6 package
hashes remain unchanged. No game launch/control/probe or Tester Logs access.

## Delivered controls

- Battle Shift+T/right stick right: only clipped, rendered CTB identities, ordered
  top to bottom. Repeated turns remain. Visible enemy letters become Enemy A-H;
  generic Aeon and hidden Unknown portraits retain those visual identities.
- O/mod mode + Y/Triangle: active-turn Overdrive; use the displayed interpolated fill
  when that meter is present, otherwise retain the requested native active-meter
  current/maximum normalization. Shift+O/right stick left reads ONLY HUD meters actually drawn;
  absent meters say No Overdrive meters displayed. All battle right-stick axes
  are consumed before native gamepad decoding, including subthreshold residuals.
- Shift+H/right stick down reads all visible HP/MP panels, with native current/maximum roles and clipping. [Latest native evidence](battle_panels_controls_20261016.md).
- Grid View mod mode + X/Square=F2 path planner; Y/Triangle=F3 ability planner;
  A/Cross=F4 forward guidance; B/Circle=F5 backward guidance. Direct LT is
  Shift+Backslash (previous route); RT is Backslash (next route). Grid triggers
  are consumed, with and without mod mode. Menus/planners retain their controls.
- Gagazet F8 > Miscellaneous > Gagazet cave puzzles offers separate auto-success
  default Off, cues default On, instructions, selected opening and status.
  Existing F11/Shift+F11 guidance routes apply. [Exact Gagazet evidence](gagazet_cave_20261016.md).

## Native battle HUD evidence and ownership

Retained originals are install/battle-hud-{original,status-hud-original,sprites,
frame-clip,quads}-20261016.txt (the status export is named
battle-status-hud-original-20261016.txt). Source functions and full signatures:
49B2D0 HUD frame530 bytes;4A0520 CTB window1546;4A1660 card1540;49E5D0 icon1576;
503BB0 textured quad806;4E5A20 textured clip683;4963C0 three-row status3482;
496050 meter879;4F4B20 plain quad280;4E58E0 plain clip310. RVAs above are
reference RVAs, dynamically qualified for both exact supported profiles.

CTB's native loop attempts32 internal cards, then clips them to its window.
Capturing the forecast or card call alone would reveal offscreen turns. The
producer now accepts a row only after its identifying texture quad survives the
ORIGINAL clip with positive screen area. Lettered enemies require the second,
letter quad; their anonymous backdrop cannot reveal the letter. Scroll arrows
and unrelated textures are excluded. Native same-thread frame enter clears the
pending POD; return publishes at most32 turns/3 meters, including an empty HUD.
Both CTB-window and plain-quad parent hooks are installed: Event.parentRva is
the nearest HOOKED parent, not necessarily the direct machine-code caller.

The meter copies the exact signed-word maximum at reference21D0AA4 (native
MOVSX WORD at896061) and the draw's interpolated fill operand. Only the three
native status rows atF3F76C that issue an accepted meter draw qualify. Empty FF
rows, clipped/sentinel geometry and earlier-frame rows are excluded. The plain
clip can return1 offscreen while replacing every coordinate with-100000; its
return alone is insufficient.

Ordinary native owner hooks only inspect/copy bounded ready values; no locks,
waits, scans of the whole forecast/roster, text processing, sorting, backend calls
or I/O. Primitive snapshots remain on that owner. Requested names are copied
with existing ready getters; OrderedRows sorts at most32 owned names/coordinates
on preparation. No borrowed engine pointer crosses to workers, no text-content
deduplication. Frame/epoch/reset guards retire stale data. Shared bounded text
capture rejects a whole over-capacity event rather than blocking Native.

## Verification and practical limits

- Both164-hook production reader profiles pass, including original683/310-byte
  x86 clip execution:32 attempted/five visible cards, repeated actors, vertical
  ordering, hidden portrait/generic Aeon, clipped letters, positive edge area,
  empty frames, interpolated meters, signed-word width and reset. Original GPU
  submission/call context is controlled; caller bodies/signatures are retained.
  Generator: Scripts/Offline/prepare_battle_hud_fixture_20261016.py; proof in
  Scripts/Offline/output/battle_hud_20261016/proof.json.
- Gagazet original120-byte integer/123-byte operand routines and both original
  script predicates pass development/release checks. Independent extra native
  ring-write fixture verifies both alignment bits, distinct alignments and
  assist/cue gates. Real SDL dummy audio verifies the cue works with Overdrive
  cues Off and cancels independently. Settings fixture covers all32 saved
  assist/cue combinations without changing INI bytes. Contextual F8 fixture passes.
- Actual x86 SDL virtual-pad bridge tests pass all new grid faces/direct triggers,
  battle sticks, held-input suppression, native DirectInput/XInput filtering and
  untouched keyboard decoder. Seven dedicated-owner/failure cases pass.
- Current controlled96 sample controller test: source-to-native-read p50=2.132ms,
  p95=2.571ms,max=2.936ms; Native pump+read max28us versus direct fake-native0.1us.
  SDK source/focus/backend are simulated. This is not DualSense/game latency proof.
- Actual bounded queue/preparation with32 copied HUD rows: max Native plan capture
  7.4us, request-to-first-usable event0.07ms; existing ordinary request baseline
  capture11.3us/output0.02ms in that run. Stalled preparation and zero Native heap
  checks pass. These components exclude GPU/full hook aggregate, physical input,
  PRISM/audible timing, cold game loads and real frame contention. No10ms hardware
  guarantee is inferred. HUD collection completes in one native draw, with no
  multi-frame batch assembly.
- Logs: install/puzzles-battle-grid-{readers-final,profile,menu,sdl-owner}-20261016.log;
  install/battle-grid-controller-final-20261016.log; install/gagazet-{utilities-final,
  native-cues-final,defaults,audio-first}-20261016.log;
  install/battle-hud-worker-second-20261016.log.

The sandbox could not start the fixed-base x86 verifier (Illegal System DLL
Relocation). The same read-only verifier passed outside the sandbox, followed by
normal guarded deployment; this was an environment start failure, not ignored
signature failure. Neither verifier executes game entry points/speech/hooks.

## Delegation tracking

All delegated tasks used gpt-6-luna, medium, fresh concise scopes. Sol retained
RE/design, core implementations, integration, review and acceptance testing.

| Task | Outcome and Sol review/validation | Remaining issue |
| --- | --- | --- |
| gagazet_settings | Appended independent settings/default fixture; reviewed enum/default/persistence;32 combinations pass | Gameplay preferences only |
| gagazet_menu | Context-only menu and F11 routes; reviewed; Sol removed proposed P shortcut because P already owns distance; menu fixture passes | Gameplay integration |
| gagazet_menu_fixture | Bounded context/toggle/action/stale menu cases; Sol inspected and ran accessibility checks | Gameplay integration |
| battle_grid_controller_fixture | Added real virtual-pad coverage; initial scratch-buffer/action-count bookkeeping faults corrected by Sol/follow-up; final checks pass | Physical controller timing |
| controller_fixture_finish | Bounded scratch/reset/count corrections and both trigger modes; Sol reviewed, corrected one later retained keyboard-parent setup and reran PASS | Physical controller timing |
| puzzle_controls_docs | Updated controls/Gagazet evidence from verified facts; Sol reviewed against code, asset proof and test results | New gameplay confirmation |

No Astra delegation. No outstanding delegated implementation blocker.

## Priority user checks and warps

Open Shift+F1 in the field. Existing native section entrances:

- `warp 310 0`: first trial's water section, Wakka ball target.
- `warp 310 1`: second trial's water section, three swimmers/openings.

These place the player at each section entrance; use normal entity navigation to
its trial trigger. Original entry0 requests swim camera W26:6; entry1 W26:8.
W32's trigger calls W22:6, W33 calls W22:7. Retained original entrances/proof:
Scripts/Offline/output/gagazet_fixture_20261016/warp_entrances.json. Warps preserve
story/solved flags; completed puzzles require an earlier save for a fresh test.
No new exact-position teleport or progression reset was implemented.

1. In battle press Shift+T or flick right stick right: visible upcoming turns only,
   in order; no game action from the stick. O/mod Y reads active Overdrive;
   Shift+O/stick left reads only meters currently shown (may report none). Check ordinary
   movement/menu response remains immediate, including during speech. Shift+H/
   stick down reads visible HP/MP rows. Earlier verification/timings below are
   retained; the linked follow-up records the latest166-hook checks.
2. On the Grid, View then each face button opens F2/F3 or reads F4/F5 as above.
   LT/RT review previous/next route; planner/F8 input stays contextual.
3. Gagazet auto-success Off: enter each trial. First trial announces controls and
   beeps on CURRENT opening alignment; manual projectile lead/usability remains
   unconfirmed. Second announces Rikku/Tidus selected openings. Native colours:
   Rikku green/right, Tidus blue/left, Wakka remaining lower red (tester's orange).
4. Before entering either puzzle, enable Gagazet auto-success. Native throw or
   selections should complete and the game should reveal its normal route. A
   partially wrong manual second choice needs Back/re-entry. Observe both native
   cave routes after success; report missing input/reveal with the own-session log.

Installer packaging remains explicitly held. No runtime capture is requested.
