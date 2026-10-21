# Final Fantasy X LAA maintenance record — 2026-10-12

## Scope and status

This record covers the launcher change that ensures the installed Final Fantasy X executable has the Large Address Aware (LAA) PE flag set. The launcher patch is implemented and offline-verified. Runtime gameplay and performance confirmation remain pending with the user. No actual game executable was changed or launched during this work.

LAA raises the virtual address-space ceiling available to a 32-bit process on supported 64-bit Windows. It does not increase physical memory allocation and this change does not establish that it fixes lag.

## Verified executable identities

- Installed `FFX.exe`: SHA-256 `0537b2a1047f3266e73495cd4e35f63f0777f4231d417699f979954686da686d`, PE characteristics `0x0122`; LAA was already enabled.
- Reference `FFX.exe`: SHA-256 `3d13e5ed53821df8db204cd3c27d470c5c220b18e16849fc4440996c15f277e1`, PE characteristics `0x0102`.
- These are distinct supported executable profiles. Their characteristic values differ in the LAA bit; this does not claim the whole images differ only by that bit. The installed image already has LAA, so its integrated header check returns without writing.

## Implementation

- `src/launcher/launcher_large_address.hpp` runs the work through the thread pool using copied UI status. It begins after app-domain initialization. Pending checks gate only FFX activation temporarily; on failure, the native `OnMouseClick` launch path remains available.
- The helper accepts only the supported FFX executable whitelist, patches only when the LAA bit is absent, verifies the complete replacement hash, and requires every FFX process to be closed. It clones with `OR 0x20` so adjacent PE flags are preserved.
- Staging uses a unique `FFX_SR_LAA_Backups` location, flushes the staged file, and replaces the target with `File.Replace`. It keeps a complete original backup, verifies hashes, and rolls back without overwriting a concurrent edit. Already-enabled images are read-only to this helper.
- `src/native/large_address_image.hpp` is a pure x86 PE32 codec.
- `src/native/profile.cpp` first checks the exact SHA-256, then allows only an LAA-bit-normalized SHA-256 for the verified reference/installed FFX pair. Will retains its separate exact hash; no LAA normalization is allowed for it.
- `src/installer/LargeAddressIdentity.cs` uses flag-only SHA-1 identity. `BaseGameCatalog.StockFile` falls back to the root `FFX.exe` only, with matching size. `MigrationPlan` excludes backups.

## Offline verification

- Sol built the actual launcher cleanly and compiled a scratch C++/CLI header fixture with `/W4 /WX`.
- `work/check_laa_transactions.py` operated on an isolated copy of the installed image with its LAA bit cleared. The actual helper restored the original bytes exactly; the `.original` backup matched the complete clear-flag input. A repeat run preserved hashes, modification times, and file count. Unrelated data, adjacent PE flags, and malformed input were rejected or preserved as expected.
- `work/prism-launcher-laa-reviewed.log` records PASS for the real adapter against fake Prism, including CLR initialization, hotkeys, actions, sounds, dialog isolation, config preservation, idempotency, and rollback.
- Luna's `laa_stock_identity` helper had 37 checks PASS and Sol reviewed it. Luna added eight `StockFile` assertions; the isolated build passed with warnings treated as errors.
- An initial agent fixture using a stale Steam manifest failed. Sol's fresh official suite in `work/prism-installer-laa-reviewed.log` passed, including the full installer and two packaged short/long runs. It covered 11 resources and seven payloads; Prism remained unchanged.
- The latest DLL source has not been deployed.

## Boundaries and follow-up

- No game launch, runtime probe, or actual game-file modification was performed. Gameplay behavior, load time, frame responsiveness, and any perceived lag change are unconfirmed and remain user-side checks.
- Launcher/runtime code and installer behavior have offline evidence as described above; neither offline success nor the LAA flag alone proves a performance improvement.
- Prior primary references: [Windows memory limits](https://learn.microsoft.com/en-us/windows/win32/memory/memory-limits-for-windows-releases) and [MSVC `/LARGEADDRESSAWARE`](https://learn.microsoft.com/en-us/cpp/build/reference/largeaddressaware-handle-large-addresses?view=msvc-170).