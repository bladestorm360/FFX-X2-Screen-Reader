# FFX player guide — 1.0

This is the full control reference for the 1.0 release of the native FFX reader. See the [README](../README.md) for installation and a concise release overview, and the [changelog](../CHANGELOG.md) for release history.

## Launcher setup

The release installer includes the locked x86 PRISM 0.18.3 runtime, required licenses/notices and SDL3. It verifies a matching existing runtime read-only, backs up/removes Tolk and preserves saved settings.

Close the game and launcher, extract the release ZIP and run **InstallFFXScreenReader.exe**. Select the detected installation or use **Browse**. If setup reports a previous launch redirect, choose **Repair launch redirection**, review and confirm the listed changes, and save the backup. Then choose **Install** and wait for **Installation complete**.

Setup verifies original game files, backs up/removes added DLL/ASI mods and old Frida files, and preserves settings, saves and unrelated launcher XML. File backups are in `FFX_SR_Install_Backups`. Normal installation makes no registry changes. Optional repair removes only the listed FFX `Debugger` launch values; **Restore launch backup** reverses it. **Check launch redirection** is read-only. Use the same installer to update. The [README](../README.md#installation) includes missing-file repair and Steam Launch Options instructions.
### Development builds

In a local development build, **Shift+F1 in the field** opens the utility
console. Type a command and press Enter to run it and return to the game;
Escape returns without running a command. Plain F1 keeps its existing controls
and F12 is available to NVDA. Shift+F1 stays consumed if the console cannot open,
its queue is full, or focus changes before F1 is released. Release and
`--test-build` exclude this console.
Battle, menu and map transitions cancel a stale console session and return
focus to the game if the console still owns it.

The six trial commands are `warp besaid`, `warp kilika`, `warp djose`,
`warp macalania`, `warp bevelle` and `warp zanarkand`. Open the console with
Shift+F1 in the field, enter one command and press Enter. These use the trial
chambers, rather than the temple exteriors, and preserve story progress.
Close game dialogue before issuing a warp command.

For pathfinding tests, `warp 82 0` goes to the Djose Temple exterior, entrance 0.
`warp djose` goes directly to its Cloister of Trials, room 214.
`warps djose` lists Djose rooms and their valid entrances.
For the six Baaj seals, `warp 196 0` goes directly to Ruins — Antechamber and preserves progression. The development console is excluded from the packaged release. [Implementation and checks](features/runtime/console_f1_20261020.md).

## Language

If the game starts in Japanese, find its settings file under
`Documents\SQUARE ENIX` in the FFX / FFX-2 HD Remaster subfolder. Change the
language value from lowercase `jp` to lowercase `en`, save, and restart the game.

## Keys

### Game Controls

Keyboard controls use the game's bindings. SDL controller controls pass through
the game's native bindings; View/Select is reserved to arm a mod command.

Left stick or arrow keys: movement and menu navigation

The configured Confirm action (Enter in the tested setup): interact or confirm.
Trial pedestal pushing uses the configured walk/push action (Space in the tested setup).

B button or Escape: cancel

Triangle or the configured menu key: open the field menu

### Mod Controls

**F8** opens the spoken accessibility menu. Speech is the first row, followed by
Navigation audio, Navigation, Miscellaneous and Close.
Up/Down select, Left/Right change settings, Right or Enter opens a submenu,
Enter/Space activates, and I describes. Escape or Back returns to the same
parent row; Escape in the main menu closes it. Home/End select the first/last
row of the current menu. Settings save automatically in `FFXAccessibility.ini`.

Overdrive auto-success, Lightning auto-dodge and Jecht Shot auto-success default
**Off** for fresh/unset preferences. Installer updates preserve your settings
file, and valid saved On or Off values override new defaults. Cues are independent.

After the opening notices, a short mod instruction names the keyboard advance
control (B in the tested mapping), or Start/Menu when using a controller. Press
it for the title menu, or let the intro play.
The initial autosave warning is also spoken using the game's localized text.
In Escape, I reads the displayed help for the highlighted option. Help menu names
speak. In Tab > Help, information pages are text buffers: arrow keys review
each native text line, and Autodetail reads the body when the page opens. Genuine
Help menu/page links keep their native arrows. I still reads the whole page;
comma/period also review lines. **Shift+I** reads actively displayed footer
controls, or "No controls displayed." Footer controls are excluded from I,
arrow review and Autodetail. Menu prompts precede initial focus; unavailable
characters say Unknown character. F7 toggles Autodetail.

- **Navigation audio:** beacon, wall and arrival sounds, their volumes, wall
  range, race pickup cues, Jecht Shot cues, Lightning warning and sound previews.
- **Navigation:** Auto walk, Speak auto walk path, Hide unreachable destinations and Partial steps.
- **Miscellaneous:** Autodetail, voiced dialogue output, combat detail, Menu indexing, Overdrive assistance, Overdrive cues, Jecht Shot auto-success, Lightning auto dodge, Show maximum HP, Chocobo training auto-success, Remiem race auto-success, Butterfly hunt auto-success and Blitzball auto-win. These four minigame assists default Off. Controller bindings and supported Cloister of Trials controls are submenus here.

### Optional minigame completion assistance

Four independent saved options are in **F8 > Miscellaneous**, using the keyboard or controller menu. Each defaults **Off**, appears only in its preparation context, and uses the existing menu controls. No new field F-key is assigned. Off leaves the activity manual; existing cues and controls remain independent.

- **Chocobo training auto-success:** On the Calm Lands South preparation map, enable before starting. Completes whichever of the four courses you start, including Catcher with its perfect-time result. Confirm the normal results/reward dialogue.
- **Remiem race auto-success:** Enable before starting. First completion uses native victory; later completions take the next unearned chest prize tier from one through five. After all tiers are earned, repeat completion uses the native five-chest result.
- **Butterfly hunt auto-success:** Enable on either Makalania course before starting. Supplies seven pickups for the current course and story phase; the normal timer and cleanup continue. Open the resulting chest yourself.
- **Blitzball auto-win:** Enable at team setup. During a live match, confirming an actual Action Commands choice triggers the native player win. Setup, movement, formation, forfeit and cancelled choices do not trigger it. The game retains its normal results and prizes.

All four retain native prompts, progression flags and rewards. Implementation and offline verification are complete; fresh gameplay confirmation is pending. See the [implementation and evidence record](features/puzzles/minigame_completion_20261020.md).
The **Voiced dialogue output** row cycles **Suppress** (default), **Braille only**,
and **Speak anyway**, and saves immediately. It applies to dialogue during audible
native voice or movie-dialogue playback; unvoiced, paused, muted or failed audio uses normal speech.
Movie suppression follows audible soundtrack playback, including background music.
Braille only sends no synthesized speech, including when no display is available.
Ordinary field F1–F5 and F9–F12 pass to the game; F6–F8 remain mod controls.

F11 reads puzzle guidance in supported Cloister, Via, and Gagazet guide contexts.
In native menus, battle, or the Escape menu, it repeats the current speech.
Ordinary field F11 is passed through to the game. **F9/F10** select the
previous/next step only while a supported guide is active; Shift+F11 reads the
step inferred from game state when identifiable.

In Zanarkand, interact with a pattern display and let its animation finish.
Autodetail reads the displayed glyphs. To review them, use **F8 > Miscellaneous >
Cloister of Trials > Review displayed glyph sequence**. Up/Down select a glyph;
I reads the sequence. Each name includes its floor-panel number, shape and
colour, matching the navigation target and manual guide.

Gagazet's Submerged Passage offers **F8 > Miscellaneous >
Gagazet cave puzzles** while the puzzle context is active. **Gagazet puzzle
auto-success** defaults Off; **Gagazet puzzle cues** defaults On. The submenu
reads instructions, the selected opening and native completion status. During
the target throw, listen through a couple of passes of the targeting beep. It
marks when both gaps align. Begin throwing before the beep so the ball arrives
during alignment; do not press exactly on the beep. No calibration throw is
required. In the swimmer puzzle, Rikku uses the green right
opening, Tidus the blue left opening, and Wakka the remaining lower red opening.
A partly chosen wrong Rikku opening may
require Back and re-entry. F11 reads the current puzzle step; Shift+F11 uses
the existing inferred-step guidance. Both puzzles and their controls are confirmed in the 0.5.7 release.

**T** repeats the current dialogue page while its window or event is active.
It uses dialogue text independently of other mod announcements and expires
when dialogue closes. Native TK Al Bhed glyphs follow collected Primers.
**Menu indexing** defaults Off in F8 > Miscellaneous.

The post-game audio drama also reads its localized timed captions. Choose its
output mode in FFX's F8 menu before opening the audio drama; the saved setting
applies to both executables. Live movie/caption speech and braille acceptance
remains pending.

**F6** toggles the audio beacon on the field and opens node-number entry in the Sphere Grid; **F7** toggles Autodetail, which also appears in
the settings menu and reads available native descriptions on highlight. F8 opens
accessibility settings. F1–F5 run their documented mod functions only in supported
Sphere Grid or minigame contexts. F12 remains available to NVDA outside modal mod
UI. Beacon and wall tones default off. Enable them in
F8. Enabling the beacon starts guidance for the selected reachable destination;
Backslash also reads the route. Wall tones indicate nearby floor boundaries relative to
the camera. Audio previews are available in F8.

Controller defaults use Xbox button names:

| Context | Control | Action |
|---|---|---|
| Field | D-pad Left / Right | Previous / next destination |
| Field | D-pad Up / Down | Previous / next category |
| Field | Right trigger | Read the turn-by-turn route |
| Field | L3 | Toggle beacon |
| Field | R3 | Toggle unreachable filter |
| After View/Select, field | X | Current gil |
| After View/Select, field | Y | Current map name |
| After View/Select, field | A / B | Previous / next trial step |
| After View/Select, battle | X | Active character or aeon's HP, MP and status |
| After View/Select, battle | Y / Triangle | Active character or aeon's Overdrive meter |
| After View/Select | Start | Accessibility settings |
| Native menus, shops or battle menu | Right stick Up | Highlighted description |

View/Select arms one mod command. Release controls after a command, closing
settings or returning focus. In accessibility settings, D-pad navigates,
A confirms, B returns, X or right stick Up describes, and LB/RB select first/last.
F8 > Miscellaneous > Controller bindings supports buttons, triggers and stick gestures;
occupied bindings swap. This layout replaces the earlier controller defaults.
The left stick continues native field movement; D-pad retains native menu and
battle navigation. Shoulders retain the game's controls, including character
switching in the Sphere Grid.

**F8 > Miscellaneous > Speech backend** selects **Automatic** or an available
PRISM backend. Polish text is recognized on Automatic; a Polish override is
also available.

Controller button presses and deliberate stick/trigger movement interrupt
speech in the same way as keyboard actions. This includes native menu movement
and character-status queries during battle. Holding a control does not keep
silencing speech. New selection and detail lines retain their queue order.

In a supported Cloister, Via, or Gagazet guide context, **F9** selects the
previous step, **F10** selects the next, and **Shift+F10** reads the selected step.
**Shift+F11** reads the step
inferred from verified game state; if that state cannot identify an exact step,
it says so. Manual selection stays independent of native completion. The F8
Cloister submenu provides Read Path and the opt-in trial solver. Guide target
names match the pathfinder, which marks temporarily disabled controls as inactive.

Zanarkand selects separate solver and guide paths from the game's current visit.
On the first visit, solving opens the normal trial gate and stops. Take the
native exit to continue the boss sequence. On the later return, solving collects
the Destruction Sphere treasure and keeps you in the trial. The return objective
stays locked until the game unlocks it; guide steps cannot cross between visits.

#### Airship Search map

Y is reported static; its purpose and movement control are deferred for investigation.
X and Z update correctly with M.

During native Airship Search, **M** reads displayed X/Y/Z. Elsewhere, M reads the
field map name. **View/Select then Y** reads the same Search coordinates.

Turn **F8 > Navigation > Enable hidden destinations for airship map** On (it
defaults Off), then press **Shift+M** during Free Search to open the six-destination
picker. The shortcut is accepted only in the current native Search context. On a
controller, open the same picker with
**View/Select then Start > Navigation > Hidden destinations for airship map**.
Up/Down selects; Home/End selects the first/last item. Enter/Space applies,
Escape cancels. Controller D-pad and LB/RB browse; A applies and B cancels.
Native movement or Cancel stops assisted cursor motion. The picker provides no
password list. User testing confirms the destination list reads and password
entry works. The reported silent **Proceed?** options remain unresolved; offline
selector checks did not prove that report fixed.
The October20 source repairs Free Search audio lifetime and toggle behavior,
removes the blanket mute, and restores previews and independent events. Audible
behavior through both actual menu routes remains unconfirmed.

#### Battle

**T** — announce whose turn it is (the character whose menu is currently
active).

**H** — announce the current turn owner's HP, MP and status, including an active aeon.
H is silent outside battle and when no character turn is active.

H reads current MP by default, matching the battle panel. **Speak maximum MP**
in **F8 > Miscellaneous** includes the maximum, independently of the HP setting.

**O** — announce the current turn owner's Overdrive bar percentage, including an
active aeon. No active player turn means silence. The existing Sphere Grid O
orientation command retains its separate context.

**Shift+T** or **right stick right** — read only accepted, clipped CTB icons
that are currently visible, in top-to-bottom order. Repeated turns are kept.
Enemy icons use their displayed letter equivalent (for example, **Enemy A**);
generic Aeon icons say **Aeon**, and a hidden portrait says **Unknown**.

**O** or **View/Select then Y/Triangle** reads the active character's Overdrive
percentage. **Shift+O** or **right stick left** reads only Overdrive meters
actually drawn on the HUD and may report that none are visible. Battle consumes
all right-stick axes. A saved battle command keeps its assigned face: if Y is
occupied, the active meter uses the first free face in Y, A, B, X order.
Field map, Grid planners and open mod-menu controls retain their contexts.

**Shift+H** or **right stick down** reads all currently visible character or aeon
HP/MP panels in HUD order. It reads the displayed values, including maxima only
where drawn, and excludes empty slots, reserve characters, clipped numbers and
status-only pages. **H** continues to read the active turn owner's HP/MP/status.

In the battle **Change** list, highlighting a character automatically reads their
name, current HP and current MP. This works with Autodetail and combat detail Off.
I repeats those highlighted vitals; H continues to read the current turn owner.
HP reads as current HP followed by its bar percentage, such as **HP 321 (64%)**.
Turn **Show maximum HP** On in **F8 > Miscellaneous** for **HP 321/500** instead.
This applies to Change, Summon and H. The Change rows display current HP/MP and
an HP bar; they do not display numeric maximum HP or status-effect labels.

When the game reveals **Sensor** information for a highlighted enemy, its
exact current and maximum HP are read directly. **Ctrl+Left** selects the first
information line, **Ctrl+Up/Down** browse elemental weaknesses, absorption,
nullification, resistance, status icons and the monster description;
**Ctrl+Right** selects and reads the last line. **View/Select then D-pad** provides the same review
on a controller. Enemy highlight assembles the buffer; reading keys browse that
snapshot. The buffer expires when that target or its pane closes.

In **F8 > Miscellaneous**, **Overdrive auto-success** defaults **Off** and supports Tidus, all four
Auron button combinations, Lulu and Wakka. Attack Reels automatically selects
three double-hit attack icons for **12 hits**. For Element, Status and Aurochs
Reels, the sequence
and countdown pause while you choose a valid match. **Up/Down** browse, **Home/End**
or **Left/Right** select the first/last choice, and **Enter/Space** applies it.
**Escape** returns to manual reels for that cast. On a controller, use the
**D-pad**, **A** to apply, or **B** to cancel. **I** reads these controls. Choices
use the game's names: Element Reels offers Fire Shot, Ice Shot, Water Shot and
Thunder Shot; Status Reels offers Havoc Shot,
Time Shot and Break Shot. Aurochs Reels offers all eight: Fire Shot, Ice Shot,
Water Shot, Thunder Shot, Havoc Shot, Time Shot, Break Shot and Aurochs Shot.
Attack Reels is the only Wakka reel that applies its result immediately.
These choices match all three icons and use the game's
all-enemy result. Only matches present on all three native reels are offered.

### FFX Jecht Shot practice

During either of the two S.S. Winno Jecht Shot practice sequences, **Jecht Shot
auto-success** in **F8 > Miscellaneous** defaults **Off**. Turn it **On** to
complete all four original practice button loops, including the centered
Confirm-only prompt, and timed memory prompts
automatically. The game retains its own scoring and learning behavior. Explanation
pages still require you to press Confirm manually.
Timed auto-success waits about 750 ms before answering and between answers.
It stops when the game's own score reaches eleven, then lets the original
round finish. Timing scales to the original round's progress so accelerated
play can still finish. All eleven answers fit within about8.5 seconds in the
offline full-round checks, leaving roughly1.5 seconds of the normal round.
Each admitted prompt supplies one consistent response to all of its input
checks; centered prompts cannot cancel their own second Confirm check.
The original round lasts about ten seconds at normal speed and requires at
least eleven successful inputs. Each memory remains pending until answered or
the round ends; there is no separate half-second deadline per memory.

For manual practice, a diagonal needs both directions together. For example,
**Up Right C** means hold Up and Right, then press C while holding both.
Practice prompts and timed cues speak just the required control names.
Follow the spoken Confirm key if yours is configured differently.

With auto-success **Off**, **Jecht Shot cues** in **F8 > Navigation audio**
defaults **On**. During real timed play, a beep accompanies speech of each visible memory prompt's
configured direction and Confirm control. Continue using the simultaneous controls
shown by the game; when the memory prompt is centered, press Confirm alone. With
cues **Off**, no Jecht Shot cue beep or prompt speech is provided. No separate
hotkey is needed. Manual prompts, auto-success, the centered tutorial repair and eleven-answer pacing are confirmed in gameplay.
With auto-success **On**, manual control-cue speech and beeps are silent.
Turn auto-success **Off** to test the spoken control prompts.

Manual cues follow the original visible prompts without tutorial-phase filtering.
Repeated identical prompts still speak each time.
Manual cues are submitted at the actual message-show event with no intentional
delay. Two pending memories speak in order so the second does not cut off the
first control cue. Jecht's voiced taunt subtitles follow **Voiced dialogue
output**; their suppression does not suppress the control cues. Taunt suppression and audible manual control prompts are confirmed in gameplay.

For Auron, including Banishing Blade, manual play requires pressing the displayed
buttons one at a time, in order, before the countdown expires. A wrong input
resets progress through the sequence. With auto-success **On**, the mod completes
the sequence automatically and the game continues its normal success animation.
Auron has no match-selection menu. With auto-success **Off** and Overdrive cues
**On**, the mod speaks **Sequence:** followed by the configured keyboard or
controller inputs in order when the move initializes, before its timed input
phase. Each cast reads its own complete sequence. Cues **Off** suppresses it.

Turn auto-success **Off** to play manually. With **Overdrive cues On**, Tidus
gets a short centered beep as his cursor enters the native success zone; press
the game's Confirm action then. Wakka beeps as each selectable icon reaches
the position accepted by Confirm. All of these beeps last **40 milliseconds**.
For Element and Aurochs Reels, the ascending pitches are **Fire, Ice, Water,
Thunder, Havoc, Time, Break, Aurochs**. For Attack Reels, a middle beep is a
single-hit attack icon and a higher beep is a double-hit attack icon. Blanks
are silent. Status Reels uses the
Havoc, Time and Break pitches. Lulu's current cast count is read as it increases
using ordinary stick/button input.

Battle start announces the visible enemy formation by native name and count
when the initial enemies appear. Native opening banners follow in the speech
queue. Battle speech clears when the battle ends or its resource context changes.
Confirmed misses and KOs are spoken alongside damage. AP/Sphere Levels are
read in the first results phase; gil and rewards in the second.
Buffs and debuffs follow their actual native application events. Cheer, Aim,
Focus, Reflex, Luck and Jinx announce each changed count when that character's
qualified visual effect starts. Capped counts stay silent. Unrecognized
animations retain completion announcements; speech queues alongside damage.

Battle results use two buffers: AP/Sphere Level/Next AP, then gil/current wallet
and items/equipment. Arrows or comma/period review the active phase; I reads the
assembled page on entry. Confirm finishing a rolling counter refreshes its values
without replaying the animation steps. Review keeps its position during a refresh.
The separate equipment-capacity screen retains native navigation.
Earned AP and gil speak regardless of Autodetail. Each character's sphere level
gain speaks once after native counting completes, including a Confirm skip.
Additional per-character statistics and item drops remain in the buffer;
Autodetail On reads the additional details. Earned totals survive the countdown.

**I** — read the help text for the highlighted skill, item, or command.

#### Menus (field, config, title)

**I** — read the available native description for the currently highlighted
menu entry, item, shop item or equipment. N remains a compatibility shortcut.
Autodetail is toggled with F7 or the Autodetail row in F8 settings. Remaining
description coverage is tracked in `Docs/accessibility_upgrade_20261004.md`.

Battle menu cursor, main menu cursor, config menu cursor, new-game settings,
and title screen selection are all read automatically as you move through
them. No hotkey needed.

**Status:** Up/Left and Down/Right review the displayed page's buffer. Comma
and period remain aliases. The statistics page contains its visible header,
Overdrive mode, equipment and eight stats. Auto-Abilities contains its visible
header and displayed abilities. Closing Status restores ordinary game arrows.
For Status opened from the Sphere Grid, **V** cycles five pages. Up/Left or
comma reviews the previous copied line; Down/Right or period reviews the next.
**I** reads available details: on page entry, the full copied page; after
selecting a row, only details actually displayed for it. Empty ability pages say
No learned abilities. Character and Aeon Status use the same active-page buffer.
**F7 Autodetail** Off/On controls
automatic details. Controller mod-mode previous/next detail actions remain
available. Closing the page restores Grid controls.

**Naming/password entry:** highlighted letters and controls are read automatically.
**N** or contextual **F11** reads the same copied entered text. In the native
airship password selector, the readback says **“Password: …”** or **“Password is
empty”**; character and Aeon naming continues to say **“Name: …”**. N resumes
native help; F11 follows its current menu, battle, Escape, guide, or naming
context, and ordinary field F11 passes through. I remains details. F8 -> Speech options ->
Speak punctuation names defaults Off (raw symbols as before). On speaks names
such as asterisk and slash for highlighted punctuation.

Out-of-battle spell and item targeting reads the initially focused character
and the HP or MP shown by the game. All-party targets follow native party order.
HP uses the percentage/maximum-number preference in Miscellaneous; MP keeps its
displayed current/maximum numbers. Healing refreshes the same highlighted target.
Equipment selection reads Weapon/Armor and the equipped item.
After successful customization, the updated equipment name is read even when
the cursor stays on that equipment. I reads its updated abilities.

#### Field Exploration

Pathfinder NPCs use concise names from their interaction/script when available,
otherwise a sprite or service role such as Al Bhed, Soldier, Villager or Priest.
Only verified character models enter NPCs. Collectible Al Bhed books are labelled
**Primer**; ordinary readable books remain **Book**. Books and other props enter Objects;
unverified models use Object rather than a guessed person. Events use their
script function or trial role. Selecting a destination does
not generate a full Talk/Interact phrase; native dialogue and prompts are read
through their existing UI events.

Every navigation key refreshes nearby NPCs, shops, save spheres, treasure,
exits and events from the current native state. A collected or disabled
interaction can disappear without a map reload. While you stand still, moving
NPCs keep their browsing order. Selection follows the same native worker.

**` (backtick)** — rescan nearby entities and announce the count.

**]** — cycle to the next entity in the current category and announce it with
distance and direction.

**[** — cycle to the previous entity.

**Shift + ]** — switch to the next category (All → Exit → Event →
Save Sphere → Treasure Chest → Shop → NPC → Object).

**Shift + [** — switch to the previous category.

**P** — re-announce the currently selected entity with distance and direction.

**Backslash** — route into the selected entity's actual interaction zone,
including the reachable side of a counter. Routes use live native collision
and retain short final movements. Once in range,
Backslash says which way to face, such as **Face west**.
North means ahead relative to the camera. With Auto walk off, turn and confirm
manually. Inactive trial controls are identified as inactive.

**Auto walk** in **F8 → Navigation** defaults **Off**. Turn it **On**, select a
destination, then press **Backslash** to walk its current collision route.
The field controller route command (right trigger) also starts walking. Movement input or Escape cancels immediately. Dialogue, scripted
scenes, menus, battle, lost focus and a map change stop it; press Backslash
again to start a new walk. On arrival, Auto walk faces an interactible; confirm
the interaction yourself.
Blocked routes get bounded recovery attempts and stop if progress stalls.
The nonblocking swimming controls overlay leaves Auto walk active; actual
dialogue and cutscenes still cancel it.

Passage exits retain stable numbers, including dungeon-floor passages; sorting and temporarily hidden exits do not renumber them.

Release 1.0 includes the six Baaj Antechamber seals as navigation destinations.
They activate by walking into the native regions; selecting or arriving at one
does not press Talk or Confirm. The user confirms the missing Besaid treasure stays
inactive and every eligible seal works. Disabled seals stay listed as currently
inactive. The supplied save's Besaid prerequisite bit is clear, but that file does
not establish current unsaved or later game progress. See the [Baaj seal status and first gameplay checks](features/navigation/baaj_seals_20261020.md).

#### Sin routes and object collection

In Sea of Sorrow, routes account for the returning one-way ledges and speak each
bend. The user confirms these routes work in gameplay.

In City of Dying Dreams after Seymour, the Object list includes four active
moving-platform boarding stops. Request a route, then walk into the listed
boarding region so the game performs the native ride. Request another route
after control returns. The route replan and four boarding stops are offline
verified; gameplay confirmation is pending.

On any **Sin map**, use **F8 > Miscellaneous > Sin object collection > Collect
objects automatically**. It defaults **Off** and saves your choice across maps,
battles and game restarts. Turn it On before entering The Nucleus, for example
in City of Dying Dreams or Tower of the Dead, then close F8. Collection help
works only in The Nucleus and resumes after battles. Confirm the game's reward
dialogues; **Collection status** reads progress toward ten objects. Spikes,
battles, rewards and the next-area transition keep their normal behavior.
Off restores ordinary pickup. The option is hidden outside Sin. Offline
verified; gameplay confirmation is pending.

**Speak auto walk path**, also in Navigation, defaults **Off**. Off announces
only “Auto walking to [destination]” when starting. On reads the route first.

For a trial pedestal, Backslash gives the push direction and remaining distance,
or says to push once for an animation. Selection and P announce your current
side; the guide and arrival announce the required side. Hold **Space** in the
tested setup to activate pushing; **Confirm** handles spheres. Instructions
announce the actual configured key or controller button. With **Auto walk On**,
select **Pedestal**, press **Backslash**, then **hold the announced push control**.
Release it or use manual movement to cancel assistance. Each scripted animation
stops assistance after that action; request the next push separately.
Sphere placement and removal use the separate
**Pedestal sphere recess** destination. Upper Djose and Zanarkand one-shot
animations remain distinct from held push actions. Bevelle rail turns and
Zanarkand floor patterns retain their ordinary guide actions.

Swimming **Return to ship** and **Dive** prompts announce their native button
bindings; **Examine** announces the configured Confirm control. With standard
Xbox bindings: **Y** returns to ship, **B** dives and **A** examines. Remapped
controls use their actual configured names. **Ctrl+Left** selects the first prompt line, **Ctrl+Up/Down** browse,
and **Ctrl+Right** reads all lines. View/Select then D-pad provides controller
review. Use the announced game button to perform the action.

Paths use concise numbers and directions: `1 south, 2 west, 2 north`.
**Partial steps** in **F8 → Navigation** defaults **Off**, rounding path
distances to whole numbers, with a minimum of **1**. Turn it **On** for
`0.6 south, 1.4 west, 2.3 north`.
The setting changes spoken distances; routes and audio keep their exact geometry.

Beacon guidance follows the selected reachable route immediately when enabled.
Wall tones also begin on the current map when enabled; no map reload is needed.

**G** — read current gil.

**M** — read the current map's native name.

Completed map transitions also announce the destination automatically, for
example **Entering Guadosalam - Shop**, using the game's current subarea name.
The announcement queues behind existing speech.

**U** — toggle the saved unreachable-destination filter. It hides destinations
with no connected collision route or reachable interaction zone. Destinations
with unknown collision data remain listed. This option is also in F8.

### FFX Makalania butterfly hunt

Both the Central and North woods courses have butterfly support. Find
**Butterfly hunt start** in **Objects**, request its route with **Backslash**,
and approach it manually to begin the game's hunt. During a hunt,
active blue and red butterflies appear by name in **Objects**. Select a blue
butterfly with the normal navigation controls and press **Backslash** for a
route. The route checks the current floor and red collision zones, and changes
when butterflies move or change color. Unknown red bounds stop the route.
Follow these routes manually; Auto walk does not steer to butterfly targets.

**Butterfly cues** in **F8 > Navigation audio** defaults **On**. Continuous stereo
guides toward the nearest blue butterfly. A low pulsing tone takes priority when
a red butterfly is close; steer away from that tone. Louder means nearer.
These cues use Beacon volume and remain independent of the ordinary beacon.

- **F1:** distance and bearing to the nearest blue butterfly.
- **F5** or **G:** catch count and the game's current time remaining.
- **F6:** toggle butterfly cues.
- **Controller:** normal field destination/category and route commands work;
  **View then X** reads count/time, and **L3** toggles butterfly cues.

Catches and actual start/end events are spoken. Collected or disabled targets
disappear; leaving the course cannot be announced as a win. Menus, battles,
lost focus and map changes cancel the cues. 

### FFX Thunder Plains lightning

**Lightning warning** in **F8 > Navigation audio** defaults **On**. A short beep
warns about 0.2 seconds before impact at normal game speed. Press the game's
configured lightning-dodge control once. I on this setting reads your configured
keyboard or controller control. Game speed changes can alter the wall-clock timing.

**Lightning auto dodge** in **F8 > Miscellaneous** defaults **Off**. On supplies
one dodge press during the native window; the game handles its ordinary animation,
counters and rewards. Physical presses still count, including a multiple-press
failure. Menus, lost focus, battles and map changes cancel pending assistance.

### FFX Blitzball

In player and technique menus, **Ctrl+Left** reads the first review line,
**Ctrl+Up/Down** browse previous/next lines, and **Ctrl+Right** reads the whole
buffer. Player selection includes level, HP, speed, endurance, attack, pass,
block, shot and catch. Displayed technique descriptions, costs and effects
keep their native line order. Plain arrows still select in the game.

During **Assign Positions**, **Backslash** announces the position currently
being assigned, from left forward through goalkeeper. The game's prompt is
also read normally. On a controller, press and release **View** before each
review command: D-pad directions match the Ctrl arrows; right trigger reads
the position being assigned.

These keys apply during a rendered match in Auto movement/command mode. They
leave the Sphere Grid and other mod contexts to their existing controls.
Directions and distances use the acting player, or the ball carrier when no
acting player is available. Field players have stable numbered slots; native
roster-name narration is not bound in this pass.

| Key | Information |
| --- | --- |
| F1 | Ball carrier position and acting player |
| F2 | Your five field players and goalkeeper |
| F3 | Opponent positions |
| F4 | Goalkeepers and goal directions |
| F5 | Current score |

Actual score increases announce goals. A change of possession between teams
is announced, including a turnover after the ball was in flight. These keys
are released in Manual A/B. Manual swimming assistance remains outside this
pass. 

### FFX Catcher Chocobo race

In the Calm Lands race, a continuous stereo cue targets the nearest available
balloon ahead automatically while you run. No scan or query key is required;
native movement and pickup events continuously refresh the sound. F1 is an
optional spoken check. Steer toward the louder ear. Equal left/right volume means the
balloon is centered; its volume rises as you approach. The cue uses the native
pickup radius and course direction, including each turn, and stops targeting
a collected balloon immediately. You retain all steering control.

| Key | Action |
| --- | --- |
| F1 | Next balloon's distance, left/right error or centered status |
| F5 | Collected balloons and bird hits |
| F6 | Toggle the saved race pickup cue |
| F8 | Accessibility settings, including Race pickup cue |

Collection and bird-hit counter changes are announced. Finish, menus, foreign
focus and map teardown cancel the cue. F6 resumes its field-beacon function
after the race. Race cues use Beacon volume. This covers the FFX balloon/time bonus race, not FFX-2.

To replace the embedded cues, put `beacon.wav`, `arrival.wav` and/or
`pickup.wav` in a `Sounds` folder beside `dinput8.dll`, then restart the game.
Use mono 16-bit PCM WAV, no longer than two seconds or 256 KiB. Make pickup
audio loop cleanly. Missing or invalid files retain the embedded sounds.

### Sphere Grid

Sphere Grid is announced automatically when you open it and as you move the
cursor:

In Grid View mod mode, press View/Select then **X/Y/A/B** for F2 path planning,
F3 ability planning, F4 forward guidance or F5 backward guidance. **LT** gives
backward cursor guidance and **RT** gives forward cursor guidance. These commands
are consumed only on the Sphere Grid.

* The currently focused character's name is announced when the grid opens
  and when you switch characters with L1 / R1.
* Native node names are announced as you move, including activation for the
  focused character. Unactivated nodes announce the required sphere and quantity,
  including the matching key sphere for a locked node. **I** reads the native description; autodetail can be
  enabled with F7 or in F8. Activation feedback announces the actual character/node.
  Ability nodes resolve the ability's own description, including all text lines.
  Activation feedback includes actual before/after stat values when they change.
* Movement reads the native confirmation prompt and Yes/No choices. Confirming
  a move announces the character and destination node, plus changed Sphere Levels.
  Native preview start says "Preparing to move" before the delayed confirmation.

**F6** — at a stationary grid cursor in Browse, Move or Use, open **Node number**.
Type the spoken node number and press Enter to select that cursor position;
Escape cancels. Close any grid action menu before using it. Move and Use retain
the game's available-node restrictions and normal confirmations; this changes
the cursor only. Some crowded branches have no direct arrow combination, so
number entry provides another way to select them. Outside the grid F6 retains
its existing context action. 

**F3** — open the spoken ability planner for the character currently active on
the Sphere Grid. Choose an ability with arrows and Enter. The list includes all ability nodes on the
loaded grid, including any node with a native learned-ability ID; stat nodes
are excluded. I reads the selected ability description. Home/End and Page
Up/Down move through the list; Escape closes it. While the planner or F8 mod
menu is open, its keys are consumed so they do not also control the game.
Each ability reports whether its route is accessible with your current Key
Spheres and Sphere Levels. Every locked node consumes one matching key in the
calculation; a route requiring more keys than you own is reported as invalid.

**F2** — on Standard, select a character's path and get a route to its nearest
reachable entry. On Expert, select a stat focus (Strength, Defense, Magic,
Magic Defense, Agility, Luck, Evasion, Accuracy, HP or MP) and route to the
nearest unactivated boost. The latest F2 or F3 selection replaces the active
planner selection for that character.

**Shift + F2** — clear all ability/path/stat selections, close any planner and
restore default guidance.

**F4 / F5** — forward / backward guidance from the character’s position.

**Backslash / Shift + Backslash** — forward / backward guidance from the cursor.

Guidance follows the selected path, ability or Expert stat focus. Without a
planner selection, Standard follows whichever character path the active
character currently occupies; Expert uses its default branches. Directions
use the origin of the requested key, even when the cursor is away from the
character. F2/F3 selection
reports still validate Key Spheres and movement Sphere Levels from the actual
character position. Preview directions leave movement and sphere spending to
the game.

**O** - announce the current character path on Standard, or Expert layout,
the nearest forward/backward branches, and the highlight, actual character
position and neighbouring nodes.
Directions distinguish a required combination ("diagonally up and left") from
single-arrow alternatives ("up or right"). If no single input selects the next
node, guidance describes its relative location instead. Node narration includes
stable node numbers. The grid exit confirmation reads its prompt and focused
Yes/No choice.
F2/F3/F4/F5/O/S and node entry on F6 are active only in the Sphere Grid. Overdrive auto-success is
changed in the F8 mod menu.

**S** — read out how many S.Lv (Sphere Levels) each party character
currently has available to spend. Speaks every party member in turn, for
example *"Tidus 3, Yuna 5, Auron 0, Kimahri 0, Wakka 1, Lulu 2, Rikku 4"*.
Only active while the Sphere Grid is open. Use this to decide who to
switch to with L1 / R1 before planning a move.

Battle speech additions: Mortiphasm targeting now reports its energizing element and physical-left/magic-right mechanic; Seymour targeting adds wheel counts. Spare Change/Yojimbo speaks amount and selected decimal step. Formation identifies active party/reserve. Parameter confirmations read prompt/No on entry and Yes/No on focus; I repeats the prompt. Learned Overdrive mode messages include the native character and mode names. No new controls/settings. These changes are implemented and offline verified; fresh gameplay confirmation is pending. [Technical evidence and remaining checks](features/battle/seymour_amount_prompts_20261021.md).

H and target totals use the game's committed HP/MP. Percentages remain and use the effective battle maximum. Change uses that effective maximum, including capacity effects;
Shift+H keeps actual drawn HUD values. Existing maximum preferences and controls
are unchanged. [Native source proof and remaining gameplay check](features/battle/hp_sources_20261021.md).
