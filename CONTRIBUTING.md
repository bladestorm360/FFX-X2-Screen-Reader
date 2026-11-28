# Contributing

This guide explains how to build and test the FFX Screen Reader, and which rules a change must follow. Players should read the [player guide](Docs/controls.md) instead.

## Repository layout

- `src/proxy`: the x86 `dinput8.dll` proxy, the build entry point and the guarded deployment scripts.
- `src/native`: the screen reader itself, in C++. Its fixtures are in `src/native/tests`.
- `src/launcher`: speech for the original game launcher, through a C++/CLI adapter.
- `src/installer`: the C# Windows installer that players run.
- `src/speech`: PRISM dependency scripts.
- `src/navigation`, `src/minigames`, `src/puzzles`: offline data tools. They do not ship.
- `Docs/Scripts`: reverse-engineering and offline analysis scripts. They do not ship.
- `Docs/adr`: architecture decision records.

## What you need

- Windows with Visual Studio 2022 and the x86 MSVC toolset, including C++/CLI support for the launcher adapter.
- The x86 MinHook 1.3.4 binary package.
- The SDL3 3.4.4 source tree. `src/native/build_sdl3.bat` builds it with CMake.
- Python 3, used by `Docs/Scripts/evidence_root.bat`.

## Set up the build environment

The build and test scripts call a `dev_env.bat` file three directories above the repository root. If that file does not exist, set the same variables in your command prompt before you run a script.

Set these variables:

- `DEV_ROOT`: any existing folder. The scripts stop if it is not set.
- `VCVARSALL`: the full path of `vcvarsall.bat` from Visual Studio 2022.
- `CMAKE_EXE`: the full path of `cmake.exe`.
- `MINHOOK_ROOT`: the MinHook folder that contains `include` and `bin\MinHook.x86.lib`.
- `SDL3_SOURCE`: the SDL3 3.4.4 source folder.
- `FFX_EVIDENCE_ROOT`: a folder outside every Git repository. Tests write their build output there.

`GAME_ROOT` is needed only for deployment. Deployment also needs `dev_env.bat`: `src/proxy/deploy.bat` calls its `find_game` helper to find the Steam game folder. Leave both out if you do not deploy.

## Build

Run this command from the repository root:

```bat
src\proxy\build_dinput8.bat --no-deploy
```

The build writes `dinput8.dll`, the launcher DLLs and their dependencies to `install\native`.

Always pass `--no-deploy` unless you intend to replace the mod in your game folder. Without it, the script runs `src\proxy\deploy.bat`, which writes to `GAME_ROOT`.

Other flags:

- `--release`: build without the developer utilities. Use it for player builds.
- `--test-build`: also builds without the developer utilities.

## Test

Run the native fixtures from the repository root:

```bat
src\native\tests\run_tests.bat
```

With no argument, the script runs the main chain of suites in order and stops at the first failure. Some suites run only when you name them. To run one suite, pass its name, for example `--readers`, `--abi` or `--threading`. The `if /I` lines at the top of `run_tests.bat` list every suite name.

The fixtures use a fake PRISM library and copied data. They do not start the game or a screen reader. A passing fixture is not proof that a change works in the game.

## Rules for changes

- Follow the [non-blocking hook contract](Docs/adr/0002-non-blocking-hook-contract.md). Code that runs on a game thread must not wait for locks, queues, file I/O, sleeps or other threads.
- Add production behavior to the native reader only. See [native-only runtime](Docs/adr/0001-native-only-runtime.md).
- Send speech and braille through the PRISM adapter. See [PRISM speech backend](Docs/adr/0003-prism-speech-backend.md).
- Do not commit game files, decompiled game code, tester logs, or PRISM or SDL binaries.

## Documentation

- `README.md`: player overview, installation and known issues.
- `Docs/controls.md`: the player guide.
- `CHANGELOG.md`: release history.
- `Docs/features/index.md`: the current feature map, with links to each subsystem.
- `Docs/gameArchitecture.md`: reverse-engineering reference for the game.
- `Docs/debug.md`: debugging reference.
- `Docs/release.md`: the release procedure.
- `Docs/adr`: why the main design choices were made.

To record a new design decision, copy the format of an existing ADR in `Docs/adr`. Give it the next number.

## Report a problem

The [README](README.md) lists what a problem report needs.
