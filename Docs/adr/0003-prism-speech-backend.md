---
status: accepted
date: 2026-10-12
decision-makers: project maintainer
---

# Use PRISM for speech and braille

This record was written on 2026-11-14 from existing notes and code. The migration dates from 2026-10-12.

## Context and Problem Statement

The reader first spoke through Tolk (`CHANGELOG.md`, release 0.1: "Tolk speech/braille"). By 2026-10-07 one speech worker made every Tolk call, and no producer called Tolk or waited for output ([Docs/debug.md at d632776, "Controller speech interruption and path verbosity"](https://github.com/bladestorm360/FFX-X2-Screen-Reader/blob/d63277653437682234ccfe1f569a4b13dd558ace/Docs/debug.md#controller-speech-interruption-and-path-verbosity--2026-10-07)). That rule continues under PRISM; see [ADR 0002](0002-non-blocking-hook-contract.md). The launcher needed speech too.

Which library should the game reader and the launcher use for speech and braille?

## Decision Drivers

- One adapter for the game and the launcher.
- UTF-8 text, speech with braille, and braille only.
- Players can pick a backend, or let the library pick one.
- A pinned, verifiable x86 binary with a known ABI.

## Considered Options

- Keep Tolk.
- Move to PRISM (https://github.com/ethindp/prism).

## Decision Outcome

Chosen option: PRISM, because one adapter gives the game and the launcher UTF-8 output, braille-only output, feature checks and backend choice.

Uncertain: the notes record that the maintainer requested the move from Tolk to PRISM (`Docs/open_issue_repairs_20261012.md`, section "Prism, intro prompt, equipment and performance"). They do not compare the two libraries. The reasons above come from what the adapter uses, not from a recorded comparison.

How the code applies it:

- `src/native/prism_backend.cpp` loads `PrismRuntime\prism.dll` from the game folder with `LoadLibraryExW` and a restricted search path. It refuses any PRISM version other than `0x1203`.
- The adapter offers Automatic and each backend that reports output, speech or braille support. Players choose in F8, Miscellaneous, Speech backend (`Docs/controls.md`).
- `src/launcher/build_launcher.bat` links the same `prism_backend.cpp` into `FFXLauncherNative.dll`.
- `src/native/speech.cpp` and `src/native/prism_backend.cpp` keep SDK initialization and backend calls on one speech owner (`src/native/DEPENDENCIES.md`).
- `prism.lock.json` pins version 0.18.3, source commit `b68e9c94240cefb1f08b2306c26376e3ab300a7a`, the x86 cdecl ABI and file hashes.
- PRISM is installed separately from the mod payload, in `PrismRuntime`. The installer installs or verifies it (`src/installer/PrismInstall.cs`). The repository does not contain PRISM binaries.

### Consequences

- Good, because players can choose NVDA, JAWS, SAPI, OneCore and other backends, or Automatic.
- Good, because the game and the launcher share one tested adapter.
- Bad, because PRISM is a separate dependency with its own install and version check. Version changes need a new lock file and installer.
- Neutral: the installer backs up and removes an old `Tolk.dll` (`src/installer/InstallerEngine.cs`, `LegacyNames`).

### Confirmation

- `src/native/tests/run_tests.bat --speech` builds a fake `prism.dll` and runs `runtime_tests.exe` and `prism_adapter_fixture.exe`.
- The fixtures do not prove output through a real screen reader.

## More Information

- `Docs/prism_migration_20261012.md`: migration status and the locked SDK hash.
- `src/native/DEPENDENCIES.md`: PRISM source, licence (MPL-2.0) and notices.
- `src/speech/install-prism.ps1` and `src/speech/prism_dependency.ps1`: developer scripts that install and check the locked runtime.
