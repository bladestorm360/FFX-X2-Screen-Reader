# Changelog

## 1.0 — 2026-10-21

- Includes four independent, saved, default-Off F8 > Miscellaneous preparation assists for Calm Lands Chocobo training, Remiem race, either Macalania butterfly course, and Blitzball. They preserve native results, rewards and progression.
- Mortiphasm targeting reports its energizing element and physical-left/magic-right mechanic; Seymour targeting reports wheel counts. Spare Change and Yojimbo report the amount and selected decimal step. Formation reports active party and reserves. Parameter prompts and Yes/No focus are spoken, and learned Overdrive messages include the character and mode names.
- Battle HP/MP totals use committed native values, preserve percentage output and use effective battle maximums, including capacity changes. Shift+H continues to report drawn HUD values.
- These additions are implemented and offline verified. Fresh gameplay confirmation is pending.

## 0.6 development — unreleased (historical)

- The four completion assists were implemented and offline verified during 0.6 development. At the time, they were excluded from 0.5.9 and packaging was held for gameplay testing. They are included in 1.0; see the [implementation record](Docs/features/puzzles/minigame_completion_20261020.md).

## 0.5.9 — 2026-10-20

- Sea of Sorrow routes detour around reverse one-way ledges, and spoken paths preserve bends. The user confirms these routes work in gameplay.
- Stale Sin gate routes retire and replan. City of Dying Dreams after Seymour lists four live moving-platform boarding stops; walk into a listed region for the native ride, then request another route after control returns. Offline verified; gameplay confirmation is left to the tester.
- Same-version follow-up: **Collect objects automatically** is visible on all Sin maps and hidden elsewhere. It defaults Off and saves your choice across maps, battles and restarts. Enable it before entering The Nucleus, then close F8. Help acts only in The Nucleus and resumes after battles. Confirm the game's reward dialogues; rewards, the ten-object count, next-area transition, spikes and battles retain their normal behavior. Off restores ordinary pickup. Offline verified; gameplay confirmation is pending.
- Stone Gaze and related debuff application speech are offline verified; gameplay confirmation remains pending.
- Free Search no longer blanket-mutes mod audio. The lifetime repair restores previews and normal cues; audible confirmation through both actual menu paths remains pending. Shift+M remains context gated and Off by default. M reports coordinates; Y remains static/deferred.
- Ordinary field F1–F5 and F9–F12 pass through to the game. F6–F8 remain mod controls; F9–F11 retain supported contextual guide behavior. Password entry reads **Password** on N, while ordinary naming reads **Name**.
- Baaj labels and availability match the six temple seals. The user confirms the missing Besaid treasure stays inactive and all eligible seals work. Opening the final door with all six remains untested. Cid identity/tracking and separate Wakka naming are offline verified; gameplay confirmation remains pending.
- At the 0.5.9 release, the development console (Shift+F1) and the later 0.6 minigame completion assists are excluded from the package. The reported silent Proceed? prompt remains unresolved; an offline selector result does not establish a fix.
- Mod UI localization and HRTF coverage remain incomplete.

## 0.5.8 — 2026-10-19

- The airship destination's Proceed? confirmation now reads the highlighted Yes/No choice, including the initial selection and later moves. Verified offline in every native image language; gameplay confirmation pending.
- Airship destination list reading and password entry are confirmed working by the user. **N** reads copied entered text; **I** retains its existing detail behavior. The hidden-destination assistance picker remains optional and defaults Off.
- All mod-generated audio is temporarily disabled while the Airship Search map is active because the Free Search cue loops. Native game music and sound effects are unaffected. This suppression has not yet received gameplay confirmation.
- Existing F8 Speech punctuation names remains independent, default Off, and reads raw symbols when Off. Image reading retains all 26 labels and nine native image languages.
- Wakka alignment uses the native beep with anticipatory instructions; mandatory calibration was removed. The revised manual timing remains awaiting gameplay confirmation.
- October 18 Grid controller paths and Yuna sphere/Dome staircase repairs remain documented in [the report](Docs/dylan_reports_20261018.md) and [feature index](Docs/features/index.md).
- Free Search's Y coordinate is reported static, and its movement control is unknown. X and Z update correctly with M. Investigation of whether Y is needed is deferred at the user's request; coordinate handling is unchanged.
- Mod UI localization and HRTF coverage remain incomplete.

## 0.5.7 — 2026-10-16

- Tab > Help preserves native lines in arrow-review buffers, reads page bodies with Autodetail, and reserves **Shift+I** for displayed footer controls. Native menu prompts precede focus; unknown characters and empty slots are handled, and native arrows continue menu navigation.
- Repaired controller passthrough responsiveness. Sphere Grid node-number entry uses F6; in Grid View mod mode, View/Select plus X/Y/A/B invokes F2/F3/F4/F5 functions, and LT/RT guides backward/forward from the cursor.
- Battle commands read visible CTB order, active or visible Overdrive meters, and visible HP/MP panels. Earned AP, gil and sphere-level gains speak with Autodetail Off; detailed statistics and drops remain in buffers.
- Repaired Customize navigation after sold weapons or armor are removed.
- Confirmed all six Cloister solvers. Kilika and Djose native prompt interception, Bevelle's two native chests, and Macalania's normal story and native return-fayth gate work as intended. Zanarkand distinguishes first visit and return, reads the displayed glyph sequence, and provides floor labels.
- Gagazet's two cave puzzles provide manual instructions and cues, with independent default-Off auto-success. Puzzle targets update from native events; native gates and mixed swimming/land routes work.
- Special Features activation instructions and field toggles are spoken. The generic title instruction says **B** or **Menu/Start**.
- Release package continues to use the self-contained installer with PRISM 0.18.3 and seven native payload files; saved settings are preserved.

## Historical release notes

The entries below describe earlier releases and retain their original history. They do not describe the current release status where later fixes supersede them.
Historical 0.5.6 package note, recorded before the 0.5.7 user confirmation: October15 changes were then outside the installer, with gameplay still pending. The 0.5.7 release notes above supersede that status. [Archived evidence](Docs/zanarkand_patterns_20261015.md).

Historical 0.5.6 packaging record: the single installer used PRISM/licenses and SDL3, verified native/launcher compatibility and the locked runtime, and preserved saved accessibility settings. The Help page issue noted at that time was resolved and confirmed for 0.5.7.


Major features and notable fixes for the FFX screen reader. Older entries
summarize the archived release notes; the current controls are in the
[player guide](Docs/controls.md).

## 0.5.6 — 2026-10-14

- Known issue: Help menu speech is confirmed restored, but Help information pages
  are still not assembled into readable buffers in gameplay. Their I/Autodetail
  and paragraph review support remains unresolved. Further repair is deferred
  at the user's request; offline page fixtures did not establish gameplay success.
- Added the actual localized opening autosave warning and corrected the opening
  cue's engine-mode/room guard.
- Fixed I dispatch in Escape/Help, corrected Special Features versus controller
  page IDs, and excluded off-screen Help placeholders using final text positions.
  The intended Help page buffer path remains unresolved in gameplay.

- Added phase-specific battle-results buffers. Confirm finishing AP/gil counters
  refreshes displayed values without replaying every animation step; review
  retains its position. Separate equipment-capacity management keeps native input.
- Added active-page Aeon/character Status buffers, explicit empty ability pages,
  and the Aeon Overdrive entries. Removed descriptions not displayed by the game.
- Key Items and Overdrive View now announce actual empty slots without resolving
  a hidden item or ability name. Owned Key Item help remains available.
- Help submenu speech is restored; page buffer support remains a known issue.
  Escape highlights speak their labels/current values, with actual
  help through I/Autodetail, including contextual submenus.
- Opening guidance handles initialization that starts before the mod attaches.
  Unconfirmed October14 follow-ups still need gameplay testing.

- Opening guidance now announces the keyboard advance or controller Start/Menu control for the
  title menu, with the option to wait for the intro cinematic. It observes
  opening completion instead of waiting for movie playback.
- All auto-success settings default Off for fresh/unset preferences. Fixed the
  Overdrive default; saved On/Off values and unrelated controls remain intact
  through installer upgrades and changed defaults.

- Corrected diagonal button prompts to require both directions together, including
  Jecht practice's Up plus Right plus configured Confirm. Exact original prompt
  regressions pass and the corrected native payload is deployed.
- Jecht practice prompts and timed cues now speak control names only, such as
  Up Right C. The explanatory tutorial remains complete. Both-script/device
  regressions pass; the concise update is deployed.
- Removed tutorial cue suppression after it silenced real Jecht prompts.
  Original visible prompts and repeated identical cues speak separately;
  no deduplication was added. Corrected the second script's practice entry guard.
- Jecht auto-success covers all four tutorial practice prompts, including the
  centered Confirm-only prompt. Explanatory pages still advance manually.
- Jecht timed auto-success answers about every750ms and stops at eleven
  successes using the original game's score. Full-round checks finish all eleven
  around8.5 seconds into the ten-second round; the game retains its own timer,
  scoring and award behavior. Manual prompts and taunt suppression are unchanged.
  Manual cues observe the actual message-show executor, correcting the missed
  event inside the interpreter loop. Jecht SFX taunt subtitles now follow the
  voiced dialogue setting independently of control cues. Two pending control
  cues speak in order; equal events remain separate without deduplication.
- Fixed paced Jecht assistance stalling at a center prompt: all input checks
  for one prompt now receive a consistent response, including both Confirm
  checks and the original neutral-axis checks. Delay scales with round progress.
  Complete original-script offline rounds pass the eleven-success threshold
  and reach the game's award/learn branches. Control cues require auto-success
  Off; taunt suppression has user gameplay confirmation.
- Native listener/resource reset ownership, copied speech/log/text/menu plans,
  owned retirement, navigation scans/routes/walls and lifecycle/audio/input guards
  are repaired and offline checked. The final canonical x86 native/launcher build
  passed without warnings and guarded deployment passed both FFX/Will profiles.
- Jecht Shot auto-success (default Off) completes original practice and pending
  memory inputs. Manual cues (default On) beep and name simultaneous controls
  at the actual visible show event. Both original Winno script variants are
  covered; the preceding trigger now reads Exit 1 to Jecht Shot challenge.
- Verified source repairs are implemented and offline checked for Airship Search
  coordinates and hidden destinations, Sphere Grid Status detail/review,
  displayed Overkill, current Overdrive help, Salvage Ship navigation,
  Kilika treasure continuation, Macalania goal choice, and Via Purifico exits
  and guidance. Gameplay acceptance remains pending.
- Recompiled the single installer from the verified deployed payload after the
  user ended the gameplay hold. Previous packages are retained as backups.
- The installer embeds 26 resources: seven mod files, the 15-file locked x86 PRISM 0.18.3 runtime/provenance/license/notice set, prism.lock.json, the checksum inventory and two game inventories. It installs PRISM and SDL3.dll with its license; matching receipted PrismRuntime is verified without writes, incompatible/nonempty/unreceipted contents are refused without replacement, and Tolk.dll is backed up and removed. Saved accessibility settings are preserved.
- Stone Gaze/shared debuff action, application and native Miss/Guard/Immune feedback is implemented, offline checked and deployed; individual gameplay acceptance remains pending. See
  [reported issue 6](Docs/reported_issues_20261010.md#6-stone-gaze-has-no-text-on-application-or-miss--2026-10-12).
- The exact intermittent field-freeze cause remains unresolved. The reported
  magic_0260.dll / 0xC0E90002 error is a Windows Application Control block;
  the responsible policy remains unidentified.
## 0.5.5 — 2026-10-09

- Includes the October 8–9 native reader and installer maintenance listed below.
- Added completed field-map transition announcements, with loaded-field guards.
  Battle H is silent without an active character turn; O reads that character's
  Overdrive percentage. MP reads its current value by default, with a separate
  optional maximum setting.
- Added Auron's manual button-sequence announcement and native Wakka detection.
  Attack Reels chooses three double-hit icons; Element, Status and Aurochs Reels
  pause for a browsable spoken outcome list. Tidus and Auron have player confirmation;
  Wakka's assistance has begun working in player tests.
- Added native exit destination labels and stable passage numbers after auditing
  all 397 retained map variants, including same-name dungeon passages.
- Added a Sphere Grid Status reader and a depth-based Salvage Ship dive target.
  Player testing confirms the limitations below; these features are incomplete.

Known issues carried into 0.5.5:

- V cycles five Sphere Grid Status pages but speaks the full buffer even with
  Auto detail Off. Only one page speaks its details; the others say the character's name.
- After jumping from the Salvage Ship, the southward approach to the passage
  containing the Salvage Ship and Underwater Ruins exits has no navigation target.
  It still needs a marker qualified by diving state and reachability. Players
  currently must swim south manually to enter the passage.
- The exact intermittent field-freeze cause and the Windows policy responsible
  for the reported magic_0260.dll / 0xC0E90002 block remain unresolved.
- Other pending gameplay checks and FFX-2/Last Mission limitations remain as
  described in README.txt.

## Maintenance included in 0.5.5 — 2026-10-09

- Cheer, Aim, Focus, Reflex, Luck and Jinx announce actual changed counts at
  each character's qualified effect creation, with native completion fallback.
- Wakka's paused outcome list speaks each keyboard/controller highlight.
- Save Spheres include the underwater variant; corrected Underwater Ruins and
  both Via Purifico entries after checking all397 retained map variants.
- Windows installer handles extended file paths for discovery, stock checks,
  installation, backups and rollback, preserving ordinary readable paths.
- Shared native message/COM hooks bypass foreign events before reader locks;
  callbacks preserve native Win32 error state and avoid loader-lock work.
- The reported magic0260 errorC0E90002 is a Windows Application Control block;
  this external policy issue and exact intermittent field freeze remain open.

## Maintenance included in 0.5.5 — 2026-10-08

- Added Makalania Central/North butterfly guidance, red warning tones, native
  count/time readouts and manually followed routes around current red hazards.
- Field spell/item target vitals now cover both menus, follow native party order
  and refresh after healing. Successful equipment customization reads its fresh name.
- Hardened the development console against stale focus/context and native calls
  during loading; queued game actions run on the game owner outside reader locks.
- Added a short Thunder Plains warning about 0.2 seconds before impact at normal
  speed and optional native-window auto dodge, default Off.
- Grouped general F8 settings, controller bindings and trial controls under
  Miscellaneous; Speech stays in the main menu.
- HP reads current plus its bar percentage; Show maximum HP selects current/max.
- Battle Change highlights automatically read the selected character's current
  HP/MP, including with detail settings Off. H retains active-turn statistics.
  Summon details also use displayed combat values, including the native group row.
- Exit movement refresh retains the useful callbacks selected during discovery,
  preventing unrelated talk callbacks from replacing crossing geometry. Earlier
  conservative path validation remains; arbitrary finite contact-size ceiling removed.
- Tester Logs requires explicit issue-specific permission and is excluded from
  routine agent searches. All three authorized Dylan logs were reviewed for these bugs.
- Offline native checks/build and guarded local deployment pass; gameplay checks pending.

## 0.5.1-Test Build — 2026-10-07

- Strengthened installer updates: verified official base-game DLL/EXE inventories,
  backup/removal of other added DLL/ASI mods, and named missing/modified-file errors.
  Optional, explicitly confirmed FFX launch-registry repair saves a reversible
  backup; normal installation and launch diagnostics do not edit the registry.

- Player-confirmed: basic Auto walk operates, and collectible Primer targets use
  the **Primer** label. These confirmations do not validate the new trial push
  assistance or Primer-aware glyph-conversion changes.

- Organized F8 into Navigation audio and Navigation submenus. Speech stays
  first and voiced dialogue output stays in the main menu. Back restores the parent row.
- In-range facing guidance now ends with the compass direction, such as Face west.
- Added saved Auto walk: enable in F8's Navigation submenu, select a destination and press Backslash.
  Uses the FFXII steering/cancellation approach with FFX movement bindings.
- Corrected all six trial guides, distinct sphere destinations, pedestal resets,
  shared interaction-zone routes, and Kilika's solver treasure continuation.
- Fixed Blitzball's separate player-name mirror producer. Added Ctrl-arrow
  review of player stats and displayed move details, plus Backslash position assignment.
- Re-audited map labels: collectible Primer, Tutorial terminal, script-derived
  conversations and distinct trial controls. Unknown identities remain conservative.
- Added T to replay the active dialogue page; Ctrl-arrow review for swimming
  prompts reads the configured native controls.
- Added F1–F5 position/score queries in Blitzball Auto mode and continuous
  Catcher Chocobo balloon cues, with F1 query, F5 counters and F6 cue toggle.
- Added default-Off Speak auto walk path, Partial steps and Menu indexing.
  Whole-number routes use a minimum distance of 1 and concise directions.
- Repaired native title-transition tail returns, concurrent loading/readers
  and missing pedestal input dependencies that caused startup failures.
- Native fixtures and the x86 build pass offline. Gameplay confirmation is pending.
- Replaced release BAT setup with a self-contained Windows installer. Standard
  labelled controls discover Steam libraries, allow folder browsing and preserve
  settings/configuration, verified payloads, backups and failure rollback.
- Pedestal assistance now follows the configured walk/push control (Space in the
  tested setup), with actual contact movement, remapped directions and current/
  required-side guidance. Confirm remains available for sphere interaction;
  upper Djose and Zanarkand retain their native one-shot Confirm animations.
  New push assistance and live installer accessibility need user acceptance.

## 0.5-Test Build — 2026-10-06

Major changes since **0.1.3-ShotgunBuild**. These describe the current native
implementation. Native tests pass. Launcher speech, Sphere Grid planning, KO
announcements and Besaid autosolving have player confirmation; other new behavior
still needs gameplay acceptance. Selected trial steps read with Shift+F10.
At the time of this archived release note, the Kilika Fire barrier recess route
was reported as a possible issue; source repairs are implemented, with gameplay
confirmation pending as of October 7.
This test build excludes the development console and includes launcher setup.

### Native reader and accessible launcher

- Simplified setup: run InstallLauncher.bat after extracting the ZIP. It finds
  Steam libraries, installs DLLs and launcher speech, checks hashes, and backs
  up/removes the old Frida mod while keeping player settings.
- Migrated the existing FFX readers and hotkeys to a 32-bit native C++
  `dinput8.dll` mod. Production no longer loads Gadget or JavaScript.
- Added keyboard selection and speech to the original shared launcher,
  preserving its game actions and sounds. Native Windows settings use the
  screen reader directly; spoken selection resumes after closing them.
- Added executable compatibility checks to installation, including compiled
  reader validation, dependency verification and installed file hashes.
  Installation preserves the original proxy and launcher configuration.

### Settings, descriptions and dialogue

- Added the spoken **F8 accessibility menu**, with automatic saving to
  `FFXAccessibility.ini`. Settings cover speech/braille, combat detail,
  descriptions, navigation sounds, Overdrive assistance and controller bindings.
- Added **I** for native descriptions throughout supported menus, shops,
  equipment, battle selections and the Sphere Grid. **N** remains an alias.
  **F7 Autodetail** reads descriptions automatically on highlight and is also
  available in F8.
- Added **F11 repeat**, including explicit reading of dialogue suppressed during
  voice playback. On the naming screen it reads the entered name instead.
- Added saved voiced-dialogue modes: **Suppress** (default), **Braille only**
  and **Speak anyway**. Unvoiced, paused, muted or failed voice playback uses
  normal speech; choices and menus remain readable. Braille only stays silent
  when a display is unavailable.
- Extended this policy to movie dialogue and added localized timed captions for
  the separate post-game audio drama, using the mode saved in FFX's F8 menu.
  Movie policy follows the combined soundtrack, including background music.
  Caption speech/braille still needs live acceptance.

### Controller support and navigation sounds

- Pathfinder NPC labels use concise names or roles from actor-local dialogue,
  service behavior and sprite families; events use their script functions.
  Removed generated Talk/Shop phrasing and corrected reward NPCs classified as
  treasure chests.
- Field navigation callbacks skip battle and scene transitions, reducing
  unnecessary work during attacks. Added action entry/return timing to investigate
  the reported animation delay; gameplay confirmation remains pending.
- Added SDL3 controller input through the game's native bindings, with vibration
  forwarding and keyboard controls retained. One connected controller is used.
- Added contextual controller defaults: field D-pad destinations/categories,
  RT route, L3 beacon and R3 unreachable filter. View/Select arms X for gil or
  active battle stats, Y for map, A/B for trial step/path and Start for settings.
  Right stick Up reads native menu and shop descriptions without command mode.
- Added keyboard **G** current gil, **M** map name and **U** unreachable filter.
  Unknown collision data remains listed; reachable counter zones are retained.
- Added saved controller rebinding for buttons, triggers and stick gestures.
  Assigning an occupied control swaps assignments.
- Added a route-following stereo beacon, camera-relative wall tones and an
  arrival sound, with individual volumes, wall range and previews in F8.
  **F6** toggles the beacon; beacon and wall tones default off.
- Guidance follows route turns and moving targets and pauses during menus,
  battles, scene changes and focus loss. Arrival sounds require the beacon;
  spoken arrival remains independent.

### Menus and battle information

- Expanded field reading for Items/Key Items, both item columns and counts,
  Abilities, equipment abilities/empty slots, Overdrive, Formation, Aeons and
  Customize, including displayed attributes and material requirements.
- Added shop buy/sell prices, quantities and confirmations, with separately
  identified current and selected equipment panels.
- Added visible-page Status review with **Up/Left** and **Down/Right**; comma
  and period remain aliases. Pages include their displayed headers, equipment,
  statistics or Auto-Abilities.
- Added naming-screen letter/control speech and entered-name reading on F11.
- Added initial and all-party field-item target reading in native party order,
  using only the HP or MP displayed for that item. Equipment selection identifies
  Weapon/Armor and the equipped item.
- Added focused ESC audio/video values and keyboard/controller binding details,
  plus an input-driven Help reader.
- Expanded battle feedback for applications/cures, KO/revival, temporary and
  stacked effects, Doom, capture, silent actions and failure messages.
- Corrected **H** to read only the active character or aeon's HP, MP and status.
- Separated result speech into native AP/Sphere Level and gil/reward phases.
- Corrected numeric battle graphics: KO, petrify, delay and gauge events cannot
  become false one-point healing announcements.
- Shop descriptions include the displayed comparison character's HP/MP and both
  equipment panels' visible ability names and empty slot icons, without stat deltas.
- Preserved timing assistance for **Tidus, Auron and Lulu**, now controlled in
  F8. Wakka's reels and detailed Rikku Mix feedback remain unfinished.

### Sphere Grid planning and guidance

- Added native node/ability descriptions, character-specific activation state,
  required activation spheres/key spheres and actual before/after stat changes.
- Added **F2** Standard character-path selection and routing to its nearest
  reachable entry. On Expert, F2 selects a stat focus and routes to an
  unactivated boost.
- Added **F3** ability planning for the currently active character, with all
  ability nodes on the loaded grid, description review and a saved destination
  for each character. Routes check cumulative Key Sphere stock and movement
  Sphere Levels from the character's actual position.
- Added **F4/F5** forward/backward guidance through sections, branches and
  endpoints, plus **O** orientation with the current section, nearby branches,
  highlighted node, character position and neighbouring nodes.
- **F4/Backslash** guides forward and **F5/Shift+Backslash** backward from the
  cursor, retaining the selected path even for another character's section.
  **Shift+F2** clears planner selections. Defaults follow the active character's
  occupied path; planning leaves movement and sphere spending to the game.
- Added stable node numbers and directions that distinguish simultaneous
  diagonals from alternative single-arrow inputs.
- Added movement preparation, native confirmation prompts/Yes/No choices,
  committed destination feedback and changed Sphere Levels. Cancelling does
  not announce a completed move; grid exit choices are also read.
- Restricted F2/F3/F4/F5/O/S to the Sphere Grid so ordinary game controls resume
  elsewhere.

### Field exploration

- Expanded map-script navigation targets and contextual labels for people,
  shops/innkeepers, puzzle controls, save spheres, treasure and exits.
- Corrected exit destination resolution and actor/worker identities. Navigation
  filters inactive targets and decorative duplicates and uses reachable
  interaction zones and exit crossing triggers.
- Improved route handling on stacked floors and around narrow gaps, using the
  actual controlled actor. Failed paths and blocked movement give explicit
  feedback. Live story-dependent transitions still need verification.

### Cloister of Trials

- Added guides for **Besaid, Kilika, Djose, Macalania, Bevelle and Zanarkand**,
  with 93 actions and collision paths to their controls. **F9** reads the step,
  **Shift+F9** advances and **F10** reads the path; F8 also offers guide controls.
- Added an optional solver in F8's contextual Cloister submenu. It acts on the
  next supported sphere/glyph/pedestal interaction, collects the Destruction
  Sphere treasure through the original scripts and can complete the trial.
- Destruction Sphere interactions collect treasure while staying in the chamber;
  ordinary solving prepares treasure and exits. First/later Zanarkand visits
  preserve their distinct progression. Bevelle's separate optional chest is
  outside the required-treasure shortcut.
- Solving shortcuts intermediate puzzle actions and animations. Guide order,
  rail/tile choices and actual item grants still need gameplay verification;
  these are implemented guides, not validated complete walkthroughs.

### Reliability and diagnostics

- Fixed supported-profile startup refusal that could leave all game readers
  silent despite working launcher speech.
- Fixed game input leaking through F8 settings and the F3 planner; held controls
  drain when closing menus or returning focus. Controller refresh now also
  covers the save/load input path.
- Fixed false Sphere Grid inventory unavailability, missing sphere requirements,
  interaction-choice offsets and truncated current-page dialogue/substitutions.
- Fixed beacon toggling on an existing route and arrival audio replay after
  disabling it.
- Moved speech/log output off game callbacks and preserved distinct speech
  events even when their text matches.
- Added log history: current `FFX_SR_Native.log` and `FFX_Launcher_SR.log` stay
  beside the DLLs; `logs` keeps the twenty most recent archived reader logs.
- Added an F12 development console for room warps and separate trial
  treasure/collect/finish commands. Release and test builds exclude the console.

### Remaining work

- FFX-2 and Last Mission do not yet have native readers.
- Full enemy weaknesses/analysis, CTB order review, some Overdrive feedback and
  separate AP/loot result phases remain unfinished.
- Minigames and optional side activities remain deferred.
- Physical-controller/audio checks, movie/caption output, manual trial routes
  and other recent behavior still need gameplay acceptance.

Implementation and validation details:
[current coverage](Docs/accessibility_upgrade_20261004.md),
[trial notes](Docs/cloisters.md) and [debug reference](Docs/debug.md).

## 0.1.3-ShotgunBuild — 2026-05-15

- Added initial field-submenu reading for Items, Abilities, Equip, Overdrive,
  Aeons, Status and Formation. Later native coverage repairs are listed above.
- Added comma/period Status review and Overdrive timing assistance for Tidus,
  Auron and Lulu.
- Added interactive NPC dialogue reading and corrected save/load confirmation
  bodies and Yes/No labels.
- Added ESC menu selection/help reading and silenced repeated idle Help text.
- Corrected text-control decoding, exit-name resolution and entity name/count
  presentation; removed speech-content suppression of distinct events.

## 0.1.2-ShotgunBuild — 2026-04-27

- Added automatic entity-list refresh after loads, map transitions and scripted
  actor changes; filtered phantom actors and unreadable labels.
- Added native area names for exit destinations.
- Added instructions for selecting English when the game starts in Japanese.

## 0.1.1-ShotgunBuild — 2026-04-23

- Added named battle ability announcements for characters, aeons and enemies.
- Added **S** to read every party character's available Sphere Levels on demand.
- Added right battle-menu speech and improved menu-switch announcements.
- Reduced unsafe text/memory reads and repeated title-screen speech; moved the
  implementation toward event-driven hooks.

## 0.1 — 2026-04-17

- First test release: Tolk speech/braille, title/main/config menu reading,
  battle commands and targets, damage/healing/status/rewards and party HP/MP.
- Added Sphere Grid character/node reading and field entity scanning with
  navmesh routes, destination/category selection and exit detection.
- Used the original Frida/Gadget implementation, since replaced by native C++.



