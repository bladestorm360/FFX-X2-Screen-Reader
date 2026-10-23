# FFX Screen Reader — 1.0.1

The FFX Screen Reader makes Final Fantasy X HD Remaster on Steam accessible with spoken menus, dialogue, battles, field navigation, puzzle guidance and optional assistance.

FFX-2 and Last Mission support are not included.

The release ZIP contains one Windows installer, including the required x86 PRISM runtime and notices. Upgrades preserve accessibility settings.

See the [player guide](Docs/controls.md) for controls and features, and the [changelog](CHANGELOG.md) for release history.

## Getting started

After the opening notices, press the spoken keyboard advance key (B in the tested mapping) or controller Menu/Start to reach the title menu.

- **F8:** open accessibility settings during play.
- **F7:** toggle Autodetail.
- **I:** read available native descriptions.

Optional assistance settings default Off unless the guide says otherwise.

Features include:

- Battle review and speech, with HP/MP values and percentages.
- Field destinations and routes, with optional Auto walk.
- Cloister of Trials guides and solvers.
- Sphere Grid guidance.
- Accessible support for selected minigames.

Four independent preparation options assist Calm Lands Chocobo training, the Remiem race, either butterfly course, and Blitzball. They default Off and retain the game's normal result and reward flow.

## Installation

1. Close the game and launcher. Extract the **1.0.1** release ZIP.
2. Run **InstallFFXScreenReader.exe** and check the selected game folder. Use **Browse** if needed.
3. If setup detects an old launch redirect, choose **Repair launch redirection**, review and confirm its listed changes, and save the backup. Then choose **Install**.
4. Wait for **Installation complete**, close setup, start your screen reader and launch through Steam.

The installer checks supported original game files, backs up and removes added DLL/ASI mods and known old Frida files, and preserves settings, saves and unrelated launcher XML.

File backups are stored in `FFX_SR_Install_Backups`.

Normal installation does not edit registry values. Optional launch repair requires confirmation and saves a verified XML backup.

- **Restore launch backup:** undo a launch repair.
- **Check launch redirection:** check without making changes.

If setup reports a missing or modified original file, use Steam **Properties > Installed Files > Verify integrity of game files**.

If Steam reports a missing executable while no redirect is found, clear this game's custom Launch Options.

No manual DLL copying is needed.

The launcher now speaks arrow-selected choices in all eight native launcher languages. Its labels follow the artwork and current native language; native settings controls keep their own accessibility. To change the game language itself, use its settings file under `Documents\SQUARE ENIX` and restart.

## Known issues and coverage

Release 1.0.1 verifies each temple’s Baaj prerequisite flag after its destruction-sphere chest completes, and restores spoken launcher choices in all eight native languages. Besaid sphere collection is gameplay-confirmed. Remote confirmation of Baaj Ruins unlock through normal gameplay without warps, and real launcher/audio confirmation, remain pending.

- The airship **Proceed?** prompt remains unresolved or deferred.
- The Free Search Y coordinate remains unresolved or deferred.
- Free Search audio restoration still needs confirmation through both menu paths.
- Stone Gaze and related debuff speech, Cid tracking, the four new minigame assists, October 21 battle prompts and HP/MP repairs are implemented and offline verified. Fresh gameplay confirmation for these changes is pending.
- The intermittent field freeze remains unresolved.
- The reported Windows Application Control block for `magic_0260.dll` remains unresolved.
- Other minigames and side activities are incomplete.
- Full mod UI localization and HRTF coverage remain incomplete.
- FFX-2 and Last Mission support are not included.

## Credits

- PRISM.
- NV Access NVDA Controller Client.
- [MinHook](https://github.com/TsudaKageyu/minhook).
- [SDL](https://libsdl.org).
- [Grayfox96's Sphere Grid viewer](https://github.com/Grayfox96/FFX-Sphere-Grid-viewer), with layouts extracted using Karifean's FFXDataParser.
- Controller and navigation-sound design draw on the user's Bravely Default mod.
- Settings design draws on the user's FF12 mod.

See [dependency notices](src/native/DEPENDENCIES.md).

Source: [GitHub](https://github.com/bladestorm360/FFX-X2-Screen-Reader).

Download the latest release: [FFX Screen Reader 1.0.1](https://github.com/bladestorm360/FFX-X2-Screen-Reader/releases/latest).

For a problem report, include:

- Release version.
- Location in the game.
- Control used.
- Expected and actual behavior.
- `FFX_SR_Native.log` or `FFX_Launcher_SR.log` from the game folder.
