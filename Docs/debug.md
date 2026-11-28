# Debugging reference

This page lists debugging facts that still hold for the native reader. For build and test commands, see [CONTRIBUTING.md](../CONTRIBUTING.md). For game structures and addresses, see [gameArchitecture.md](gameArchitecture.md).

Until 2026-11-14 this file also held a dated debugging log from October and November 2026. Most of that log described the Frida JavaScript runtime or one build. The full log is in Git history: [Docs/debug.md at d632776](https://github.com/bladestorm360/FFX-X2-Screen-Reader/blob/d63277653437682234ccfe1f569a4b13dd558ace/Docs/debug.md).

## Logs

- The game reader writes `FFX_SR_Native.log` in the game folder (`src/native/runtime.hpp`).
- The launcher reader writes `FFX_Launcher_SR.log` in the game folder (`src/launcher/launcher_native.cpp`).
- At startup, the reader moves the previous log to the `logs` folder with a time stamp. It keeps the 20 newest archived logs. If it cannot move the log, it appends to it and does not truncate it (`src/native/log_history.cpp`).
- Ask players for these two files when they report a problem. `README.md` tells players where to find them.
- `recordLog` copies its arguments into a bounded queue and does not wait. A worker formats the line and writes the file (`src/native/speech.cpp`, `src/native/log_capture.cpp`). Do not open or write files directly from a hook. See [ADR 0002](adr/0002-non-blocking-hook-contract.md).

## Addresses

- Native code stores game addresses as RVAs: offsets from the start of the executable. `address(rva)` in `src/native/text.cpp` adds the base address of the loaded executable.
- Ghidra shows virtual addresses that use the preferred image base of the PE file. To get the RVA, subtract that base. The old notes give `0x00400000` for `FFX.exe`. Check the base in your own Ghidra project.
- Do not read the image base from the PE header in memory to compute a rebase. Windows updates that field to the load address.
- Each supported executable has a profile of RVAs. Profile validation refuses an executable that it does not know (`src/native/profile_validation.cpp`).

## Lessons that still apply

- Game UI text comes from the native TkMenu and TkWindow system in the executable. Iggy (Flash) only handles movies and mouse input. Look for UI functions in the TkMenu code, not in Iggy exports.
- Many game functions use the `__thiscall` convention. Read the object from `ECX` at entry. A global `DAT_` pointer near the same code often points to a different object.
- Some game string functions do not write a terminating zero when the new string is shorter. Clear a reused buffer before each call, or use the length the function returns.
- Do not poll game state each frame and compare it. Pointers become invalid for a short time when a window closes, a map loads or a battle ends. Old polling code read them and crashed. Hook the function that writes the state, and copy the data at that point.
- When a battle ends straight into a cutscene, actor slots and the navigation mesh are partly rebuilt. Old code crashed when it scanned them at that time. `src/native/resource_reset.cpp` drops work from the previous map.
- In a batch file, escape `&` in file names as `^&` when you pass them to `call :label`. Use delayed expansion (`!NAME!`) inside the label.
