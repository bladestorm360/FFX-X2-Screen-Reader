---
status: accepted
date: 2026-10-03
decision-makers: project maintainer
---

# Run the screen reader as native C++ only

This record was written on 2026-11-14 from existing notes and code. The decision itself dates from the native migration on 2026-10-03.

## Context and Problem Statement

Releases from 0.1 to 0.1.3-ShotgunBuild ran the reader as JavaScript inside a Frida Gadget. The 0.5-Test Build was the first native release (`CHANGELOG.md`). The `dinput8.dll` proxy loaded the real DirectInput DLL and the Gadget from `DllMain` (`Docs/gameArchitecture.md`, section "Native C++ rewrite and transition threading"). The Gadget binary was 18,344,448 bytes. Navigation, battle and the TK pool reader polled from frame and draw callbacks, although earlier notes called them event-driven.

The same section discusses "the original transition crash" during map changes. It lists plausible causes: work under the loader lock, reads of game objects from a speech worker, and repeated hook installation. It does not confirm one diagnosis, and it does not say who reported the crash.

Which runtime should production use?

## Decision Drivers

- Hooks must run on the game thread with a known ABI and must not block it. See [ADR 0002](0002-non-blocking-hook-contract.md).
- DLL entry must stay minimal, so initialization runs outside the loader lock.
- Players install one set of files with no external runtime.
- Only one producer may speak for each feature, so output is not duplicated.

## Considered Options

- Keep the Frida Gadget and JavaScript reader.
- Run JavaScript and native producers side by side and move features one at a time.
- Move every reader to native C++ and remove the JavaScript runtime.
- A managed (C#) port. Notes call it abandoned and do not record why.

## Decision Outcome

Chosen option: move every reader to native C++, because native hooks give exact control over the x86 ABI and thread ownership, and they remove the Gadget from the game process.

The migration record is `Docs/gameArchitecture.md`, section "Native migration implementation — 2026-10-03". It says production is entirely native C++ and that there is no parallel JavaScript fallback. The side-by-side option was the earlier plan (Session 71). Session 72 replaced it when every reader was moved before the combined live test.

The current runtime:

- `src/proxy/dinput8_proxy.cpp` keeps DLL entry to module storage. The first `DirectInput8Create` call starts the native bootstrap and forwards to the System32 DLL.
- `src/native/reader.cpp` (`initializeReader`) checks speech readiness, the executable name, the profile and the hook signatures before it enables hooks.
- `src/native/hooks.cpp` installs MinHook hooks as one batch. Its entry and return adapters keep the original argument stack, registers and FPU state.

### Consequences

- Good, because the reader has no script engine or 18 MB Gadget in the game process.
- Good, because the build is one MSVC x86 toolchain (`src/proxy/build_dinput8.bat`).
- Good, because offline fixtures can test hook ABI and thread rules (`src/native/tests/run_tests.bat --abi`, `--threading`).
- Bad, because finding a new address now needs a native build or the separate tools in `Docs/Scripts`, instead of an edited script.
- Neutral: the installer still removes old Frida files from player installs. `src/installer/InstallerEngine.cs` lists them in `LegacyNames`.

### Confirmation

- `src/proxy/build_dinput8.bat` compiles `src/proxy/dinput8_proxy.cpp`, `src/proxy/profile_preflight.cpp` and `src/native/*.cpp`. It calls `src/launcher/build_launcher.bat`, which builds `FFXLauncherNative.dll` and `FFXLauncherReader.dll` from C++ sources.
- The JavaScript runtime was removed from the tree. It stays in Git history.
- Some JavaScript stays as offline tools that never ship: `src/navigation/atel.js` (used by `src/navigation/build_targets.mjs`), `src/minigames/spatial_model.js` and `src/puzzles/*.js`.

## More Information

- `Docs/gameArchitecture.md`, sections "Native C++ rewrite and transition threading" and "Native migration implementation — 2026-10-03".
- `Docs/features/runtime/index.md`, section "Initialization and ownership".
- `CHANGELOG.md`, release 0.1: "Used the original Frida/Gadget implementation, since replaced by native C++."
- Uncertain: the notes do not say why the managed port was abandoned.
