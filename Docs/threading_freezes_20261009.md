# Shipped-build freezes and native thread audit — 2026-10-09

Sol performed the investigation, fixes, review and verification alone. Reports
refer to shipped 0.5.1-Test Build, not the current Wakka implementation. No tester
logs were accessed, no game/probe was run, and no security/registry policy changed.

## The exact Windows dialog

The quoted Swedish Bad Image dialog names stock `magicFiles/FFX/magic_0260.dll`
and status 0xC0E90002. Microsoft's official SDK defines that status as
`STATUS_SYSTEM_INTEGRITY_POLICY_VIOLATION`: an Application Control policy blocked
the file. It does not establish DLL corruption, insufficient RAM or a mod
deadlock. The exact policy is unconfirmed; Smart App Control is only one possible
source. [Official SDK header](https://github.com/microsoft/win32metadata/blob/main/generation/WinSDK/RecompiledIdlHeaders/shared/ntstatus.h)

If further evidence becomes available, CodeIntegrity Operational event 3077 names
the enforced block; related 3089 events supply signature details. This is the
appropriate evidence to identify the policy, rather than guessing or disabling
security settings. [Microsoft event documentation](https://learn.microsoft.com/en-us/windows/security/application-security/application-control/app-control-for-business/operations/event-id-explanations)

The installer already checks official stock inventories. Both pinned catalog
versions list this stock module at 344,064 bytes, with SHA1
`e66c7e3c2678e2ce326e5e88417759fc51e4f713` or
`9d61fbc079d9b5ff679e9a917680de3063ab8a44`. Correct disk bytes do not prove Windows
will permit loading. Threading repairs cannot remove that policy block.

The temporary field freeze without a dialog remains a separate unresolved
report. Neither that report nor the intermittent underwater boss failure proves
a particular command is responsible. No reproducible save was requested.

## Exact shipped payload and reproduced hazards

The shipped installer's native resource was extracted offline without executing
its entry point. SHA256
`c9d561efe2dfec1846ac803532a766e65e01a404197d6bf08267294e7b34d5c8`
matches the release checksum. Headless analysis of five actual functions confirms
`FUN1003AF20` acquires the shared reader mutex without first excluding foreign
User32/COM events. The newer Wakka callback/deferred-action markers are absent.
Evidence: `Docs/Scripts/Offline/output/shipped_threading_20261009_174213_7200758/`.
Extraction/script: `Docs/Scripts/Offline/extract_shipped_threading.ps1` and
`Docs/Scripts/Ghidra/FFX/shipped_threading_audit.java`.

A bounded synthetic native fixture reproduces the hazard: the game-owner reader
waits for a worker while holding its reader mutex; that worker enters a shared
message hook and waits for the same mutex. The pre-fix code fails the timed
regression. The repaired code bypasses foreign shared User32 events before
locking. Shared DirectInput events from a foreign caller image also bypass
before locking. The original API still executes, while owner/main-image events
retain accessibility callbacks. Ordinary resource lifecycle scopes remain
serialized as established by earlier verified loading/unwind behavior.

The hook adapter also leaked callback changes to Win32 last-error state. A
regression reproduced the failure. Entry/return guards now preserve the value
the original native code should see, including callback failure/deferred work.
The x86 register/stack, x87/SSE, exceptions and original return are preserved.

## Current-build loader precautions

`native_image.hpp` resolves MEM_IMAGE ownership with VirtualQuery, without
Windows loader-lock APIs. DirectInput qualification uses it; the main executable
base is cached before hooks are enabled. Wakka identifies its actual table/base
under serialization, then performs filename lookup/hash work as a deferred
native action outside reader/Wakka locks. Completion rechecks epoch, image,
hash, table and callbacks. Cancellation retires pending qualification. Original
native callbacks execute outside the Wakka mutex as before. These precautions
prevent new hazards; they are not blamed for the shipped testers' failures.

Wakka outcome highlights previously used a producer rejected by the real modal
speech gate. All four highlight paths now use `mod-overdrive-wakka-choice`.
Repeated arrows and controller choices remain speakable while the native reels
are paused; confirmation keeps its existing behavior. No outcome/timer mechanic
was changed by this speech fix.

## Checks and limits

The actual mixed x86 ABI fixture passes 11,000 calls; message/COM worker regression,
owner callbacks, concurrent readers, loader scopes and exception recovery pass.
All ten real Wakka DLL variants, deferred qualification/cancellation and 80
Aurochs selections pass. Fake Tolk verifies modal highlights with repeat text;
central keyboard/controller routing passes. The full native suite's remaining
fixture-only main-image stub and non-swimming dependency were corrected, with
the resumed input/utility groups passing. Logs and final hashes are in
`install/status-wakka-followup-20261009/` and `verification.json`.

Known mod hazards are repaired and offline-verified. The exact temporary field
freeze remains unproven, and the Windows Application Control block remains an
external unresolved issue. Gameplay acceptance remains user-side.

Final delivery: both tester/developer native builds pass; a fresh final proxy
passes the offline forwarding fixture. Guarded deployment validates closed
processes, compiled FFX/Will profiles, launcher bootstrap and all 13 installed
hashes. The separately verified tester EXE is `install/windows-installer/InstallFFXScreenReader.exe`, SHA256 `e31263e2c30ab5a10df4d61a805951bbe1eb77d7b109e63dddb6eb030c08518f`.
The existing release ZIP is unchanged; gameplay acceptance remains user-side.
