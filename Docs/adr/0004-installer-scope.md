---
status: accepted
date: 2026-10-07
decision-makers: project maintainer
---

# One Windows installer with a fixed scope

This record was written on 2026-11-14 from existing notes and code. The installer dates from 2026-10-07.

## Context and Problem Statement

The 0.5-Test Build shipped a ZIP with a batch setup script, `InstallLauncher.bat` (`CHANGELOG.md`). Players had to run scripts, and the scripts had to find the game, back up files and remove the old Frida mod.

What should setup do, and what must it never do?

## Decision Drivers

- Players run one program and do not need a command prompt.
- Setup must not damage or redistribute original game files.
- Setup must be able to undo its changes if it fails.
- Old mods that also hook the game can conflict with this reader.

## Considered Options

- Keep the batch and PowerShell setup scripts.
- A self-contained Windows Forms installer, `InstallFFXScreenReader.exe`.

## Decision Outcome

Chosen option: a self-contained Windows Forms installer, because it gives players one accessible program and lets setup check every file before it changes anything.

The scope, from `Docs/windows_installer_20261007.md`, `Docs/features/distribution/index.md` and `src/installer`:

- Setup checks every original game DLL and EXE against a pinned official Steam inventory (`src/installer/MigrationPlan.cs`, `src/installer/BaseGameCatalog.cs`). If one is missing or changed, setup stops and asks the player to verify the game files in Steam. It never replaces or ships original game files.
- Setup checks the SHA-256 hash of every embedded payload file before it installs anything (`src/installer/InstallerEngine.cs`, `PayloadNames`).
- Setup installs or verifies the locked PRISM runtime (`src/installer/PrismInstall.cs`). See [ADR 0003](0003-prism-speech-backend.md).
- Setup backs up every file it will change or remove to `FFX_SR_Install_Backups` before it changes anything. If installation fails, it restores the old files.
- Setup removes added DLL and ASI files that are not part of the stock game or this mod (`MigrationPlan.AddedModules`). It also removes known old files: `frida-gadget.dll`, `frida-gadget.config`, `frida-gadget.config.dll`, `ffx_sr.js` and `Tolk.dll` (`InstallerEngine.LegacyNames`).
- Setup keeps settings, saves, assets and unrelated launcher XML.
- Normal installation does not write to the registry. "Repair launch redirection" and "Restore launch backup" are separate actions that the player must confirm. They are limited to five documented Image File Execution Options names (`src/installer/LaunchRedirectRepair.cs`).
- Setup never starts the game.

Developers still use `src/proxy/build_dinput8.bat` and `src/proxy/deploy.bat`. `src/installer/build_installer.ps1` builds the installer and `src/installer/package_release.ps1` packages the release ZIP.

### Consequences

- Good, because players run one program, and every change has a backup.
- Good, because a changed or unknown game version stops setup before it changes files.
- Bad, because each new official game version needs an installer update with a new inventory.
- Bad, because setup removes other DLL and ASI mods. `Docs/windows_installer_20261007.md` says this follows the maintainer's request. A player who uses other mods loses them until they restore them from the backup.

### Confirmation

- `src/installer/tests/run_tests.ps1` runs the installer tests in `src/installer/tests`. `InstallerTests.cs` covers backup, rollback and legacy file removal. `LaunchRedirectRepairTests.cs` covers the registry actions.
- `src/installer/verify_release.ps1` checks the packaged release ZIP and `InstallFFXScreenReader.exe`.

## More Information

- `Docs/windows_installer_20261007.md`: installer behavior and the pinned inventories.
- `Docs/features/distribution/index.md`, section "Launcher and installer".
- Open question: whether setup should remove only known conflicting mods, instead of every added DLL and ASI file. This record describes the current behavior. A change to it needs a new ADR that supersedes this one.
