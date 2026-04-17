# FFX / FFX-2 Screen Reader

## Purpose

Adds screen reader support, menu reading, battle narration, and navigation /
pathfinding to **Final Fantasy X & X-2 HD Remaster**, allowing blind and
visually impaired players to play through menus, battles, and field
exploration.

## Known Issues

Battle results (AP, gil, items, equipment) are announced all at once when the
results screen first appears instead of being split into the game's two
phases. Everything is still spoken — just faster than a sighted player would
advance through it.

Sphere Grid node descriptions ("Increases Strength by 4", etc.) are not yet
read. Node type (Strength Node, HP Node, etc.) and the currently focused
character and their Sphere Level *are* read.

Field sub-menu cursors (Items, Abilities, Equip, Status inside the in-game
menu) do not track selection yet. The main menu itself and battle menus do
track and announce selection normally.

The launcher window that Steam opens first ("FFX / FFX-2 / Last Mission"
chooser) is a separate .NET WinForms app that the mod cannot attach to, and
it is not keyboard-accessible. To get past it, use one of the three options
under "Skipping the launcher" below. The mod activates as soon as the actual
game executable starts.

## Install

Create an account at https://store.steampowered.com if you do not already have
one, then install the Steam desktop app.

Purchase **FINAL FANTASY X/X-2 HD Remaster** through the Steam store:
https://store.steampowered.com/app/359870/FINAL_FANTASY_XX2_HD_Remaster/

Once installed, find the directory for the game. This is usually:

```
C:\Program Files (x86)\Steam\steamapps\common\FINAL FANTASY FFX&FFX-2 HD Remaster
```

If you have a Steam library on another drive, the path will be:

```
<Drive Letter>\SteamLibrary\steamapps\common\FINAL FANTASY FFX&FFX-2 HD Remaster
```

Copy every file from the mod archive into that folder. The files you should
end up with in the game directory are:

* `dinput8.dll` (the mod loader)
* `frida-gadget.dll` (runtime injection — upstream dependency, bundled in the release)
* `frida-gadget.config` (tells the gadget to load the mod script)
* `ffx_sr.js` (the mod itself)
* `Tolk.dll` (screen reader bridge)
* `nvdaControllerClient32.dll` (required for NVDA support)

Make sure your screen reader (NVDA, JAWS, or Windows Narrator/SAPI) is running
**before** launching the game. The mod communicates through the Tolk bridge
and will not produce speech otherwise.

The Steam launcher window is not accessible, so you will need to bypass it —
see "Skipping the launcher" below. Once you are past the launcher, the mod
loads automatically.

## Skipping the launcher

The game installs four executables into the game directory:

* `FFX&X-2_LAUNCHER.exe` — the graphical launcher (not accessible)
* `FFX.exe` — Final Fantasy X
* `FFX-2.exe` — Final Fantasy X-2
* `FFX&X-2_Will.exe` — FFX-2: Last Mission

Pick **one** of the three methods below. Option A is the recommended path —
it keeps Steam in charge of the launch so achievements, playtime tracking,
the Steam overlay, and cloud saves all keep working. Option B launches the
game entirely outside Steam (none of those Steam features work). Option C is
a fallback if you need the real launcher for some reason.

### Option A — Retarget Steam to skip the launcher (recommended)

In the Steam library, right-click **FINAL FANTASY X/X-2 HD Remaster** →
**Properties** → **General** → **Launch Options**. Set:

```
%command:FFX&X-2_LAUNCHER.exe=FFX.exe%
```

(or replace `FFX.exe` with `FFX-2.exe` / `FFX&X-2_Will.exe` depending on which
game you want Steam's Play button to launch). This substitutes the launcher
path at launch time so Steam runs the game exe directly. Steam still wraps
the launch, so achievements, playtime, overlay, and cloud saves all work
normally.

### Option B — Launch the game .exe directly (no Steam features)

Open the game directory in File Explorer:

```
<Drive>\SteamLibrary\steamapps\common\FINAL FANTASY FFX&FFX-2 HD Remaster
```

Double-click `FFX.exe` (or `FFX-2.exe`, or `FFX&X-2_Will.exe`) to start that
game directly. The launcher never runs — but neither does Steam. The Steam
wrapper is not initialized when you launch the .exe this way, so **Steam
achievements, playtime tracking, the Steam overlay, and Steam Cloud saves
will not work** for this session.

Use this option if you don't care about any of those Steam features, or if
Option A's launch-options string is not working for you for some reason.

A desktop shortcut to the .exe makes this one click.

### Option C — Use the launcher with NVDA OCR

If you must go through the launcher (for example, Steam cloud-save prompts or
Steam overlay reasons), use NVDA's screen-recognition feature to read it:

1. Click Steam's Play button. The launcher window appears.
2. With NVDA focused on the launcher, press **NVDA + R** to run Windows 10
   OCR on the current screen. NVDA will read the text it finds, including
   the three game buttons.
3. Move NVDA's navigator to the text of the game you want. Use **NVDA +
   Numpad Divide** to move the mouse pointer to the navigator object, then
   **NVDA + Numpad Divide twice** (or the configured click command) to
   left-click there. The exact shortcuts depend on your NVDA input
   configuration — consult the NVDA user guide for "move mouse to navigator
   object" and "left mouse click" if the defaults have been changed.

Options A and B are strongly preferred. Option C depends on where the
launcher renders its buttons, which can shift between game updates.

## Keys

### Game Controls

These are the game's own controls. The mod does not change them.

Left stick or arrow keys: movement and menu navigation

A button or Enter/Space: confirm

B button or Escape: cancel

Triangle or the configured menu key: open the field menu

### Mod Controls

#### Battle

**T** — announce whose turn it is (the character whose menu is currently
active).

**H** — announce every party member's current HP and MP.

**N** — read the help text for the highlighted skill, item, or command.

#### Menus (field, config, title)

**N** — read the description for the currently highlighted menu entry.

Battle menu cursor, main menu cursor, config menu cursor, new-game settings,
and title screen selection are all read automatically as you move through
them. No hotkey needed.

#### Field Exploration

Nearby entities (NPCs, save spheres, treasure chests, exit triggers, events)
are automatically scanned whenever you enter a map. You can then cycle
through them and ask the mod to guide you to one.

**` (backtick)** — rescan nearby entities and announce the count.

**]** — cycle to the next entity in the current category and announce it with
distance and direction.

**[** — cycle to the previous entity.

**Shift + ]** — switch to the next category (All → Exit → Event →
Save Sphere → Treasure Chest → NPC → Object).

**Shift + [** — switch to the previous category.

**P** — re-announce the currently selected entity with distance and direction.

**\\ (backslash)** — pathfind to the selected entity. Speaks a turn-by-turn
walking route (for example, "3 north, 2 northeast, 1 east"). Routes use the
game's own navmesh, so they follow walkable paths and route around walls.

### Sphere Grid

Sphere Grid is announced automatically when you open it and as you move the
cursor:

* The currently focused character and their Sphere Level are announced when
  the grid opens or when you switch characters.
* Each node type (Strength Node, HP Node, Agility Node, Ability Node, Key
  Sphere Lock, Empty Node, etc.) is announced as you move.

## Credits

Tolk by Davy Kager (https://github.com/dkager/tolk) — speech / braille bridge
(LGPL).

Frida (https://frida.re) — runtime injection framework (wxWindows / dual
license).

NVDA Controller Client by NV Access — NVDA integration.
