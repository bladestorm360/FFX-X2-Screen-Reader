# Windows installer — 2026-10-07

Self-contained x86 Windows Forms setup replaces distributed BAT/PowerShell setup.
Internal native build/deploy batches remain canonical. Setup never launches a game.

## Current migration behavior

`InstallerEngine` verifies13 embedded payload SHA256 values before replacement.
`MigrationPlan` selects a pinned official Steam inventory and validates every
original DLL/EXE before cleanup. Missing or patched stock files are named and
installation stops; players must Verify integrity in Steam. Setup never deletes,
redistributes or invents original game executables. Unknown versions require an
updated installer. `SteamAppManifest` validates app359870, installation location,
sole depot359871 and manifest identity. Direct-folder fallback uses exact known
FFX/launcher fingerprints. Discovery also recognizes damaged Steam installations.

All added nonstock DLL/ASI files in the selected game tree are cleanup targets,
except canonical root payload names and the exact top-level
`FFX_SR_Install_Backups` tree. Legitimate stock nested DLLs are preserved. Known
legacy Frida Gadget/config/script names also participate. Backups preserve relative
paths, include every affected preexisting file, and complete before any deletion.
Settings, saves, assets and unrelated XML are preserved; other added DLL mods are
intentionally removed, following the user's request. Rollback restores old files
and removes newly installed files; incomplete restoration names failures and backup.
Linked descendants, linked backup roots, traversal and directory/file conflicts
are refused. File targets are rechecked before mutation/restoration. Close the
game, launcher and mod manager. Final stock/payload/module checks detect remaining
changes, but no implementation can prevent every concurrent external file race.

Launcher XML disables DTD/external entities and preserves unrelated nodes and the
original configuration backup. Foreign AppDomainManager registration is migrated
only when its assembly DLL is a confirmed nonstock cleanup target. Unknown or
duplicate manager registrations otherwise stop installation before changes.

## Official inventories and evidence

Each complete inventory contains1435 stock DLL/EXE path, size and SHA1 records from
cached official Steam depot359871 metadata. No game binaries or VBF data are shipped.

| Manifest | Raw manifest SHA256 | Embedded inventory SHA256 |
| --- | --- | --- |
|1235377778411650212|10aaf1505a5b119b42ab750ee3ca1b1d6dc8bb4798824702dbe123d8670713fa|094195a8d58d667893d6fc05bfc71a5ced1067113e0cb72d16fb31cc9fa8820f|
|5168047039601173807|5be9ab5b5b5c2d76b756ccbd9668e7ac22ecec318fa2ec4d3bdc103d5719d83b|b27f00290612c241d6ce6325e330157a713381fc6a178ba0af8f8f26fe0f7a43|

Resources: `src/installer/baselines/359871_<manifest>.txt`. Full parsed metadata:
`install/installer-migration-20261007/vanilla-<manifest>.json`. Extraction followed
[SteamKit depot manifest code](https://github.com/SteamRE/SteamKit/blob/master/SteamKit2/SteamKit2/Types/DepotManifest.cs)
and the [Valve manifest protobuf](https://github.com/SteamDatabase/Protobufs/blob/master/steam/content_manifest.proto).
Only Steam ACF/depot metadata was inspected; no new game code or runtime access.

## Registry permissions and repair

The original native mod, launcher adapter and retired BAT setup have no registry
write path. Steam discovery reads locations. We add speech to the original Square
Enix launcher; there is no replacement launcher. The user later explicitly asked
for repair of other mods' persistent launch redirections.

Normal Install and Check launch redirection remain registry-read-only. Repair
launch redirection is separate: optional UAC reopens setup to a preview, then an
explicit labelled confirmation and SaveFileDialog select a new verified XML
backup before deletion. Enter in preview cannot approve; Escape cancels. Restore
launch backup separately previews/confirms and refuses overwriting existing data.

Only `Debugger` values under HKLM
`SOFTWARE\Microsoft\Windows NT\CurrentVersion\Image File Execution Options`
are eligible, for exactly `FFX&X-2_LAUNCHER.exe`, `FFX.exe`, `FFX-2.exe`,
`FFX-2_LM.exe` and `FFX&X-2_Will.exe`. Child entries require enabled root UseFilter
and exact FilterFullPath for the selected game executable. Whole keys, other
values, other images, other-install filters and Steam registry paths are preserved.
See Microsoft's [Debugger launch settings](https://learn.microsoft.com/en-us/windows-hardware/drivers/debugger/gflags-details)
and [shared IFEO registry keys](https://learn.microsoft.com/en-us/windows/win32/winprog64/shared-registry-keys).
Modern Windows shares IFEO across bitness; backend enumerates native view once.

Backups preserve exact raw UTF16 String/ExpandString data, type, view and filter
association with a SHA256 checksum. Bounded parser prohibits DTD/external entities,
invalid/unrelated keys and duplicates. The confirmed list must equal the current
eligible list before deletion; the whole backup identity is verified. Repair
failure restores deleted values or reports partial restoration and backup. Restore
rechecks confirmed backup content and rejects retargeted/disabled filters or any
preexisting Debugger. No actual registry edit was performed in this engineering
session: registry tests use memory only. Live UAC/repair/restore remain user-side.

The private modder's exact key is unconfirmed. Detecting a Debugger entry establishes
a supported redirect, not attribution. Unknown proprietary Steam path overrides
cannot be guessed. README covers clearing this game's custom Steam Launch Options
and reporting the exact missing path if no supported redirect is found.

## Missing executable investigation

Archived `Docs/Scripts/Offline/output/FFX_Launcher/20261003_audit/GameLauncher/MainForm.cs`
387–395 invokes ShellExecuteW with relative commands and null cwd; archived
`Program.cs`436–452 loads commands from embedded Resources.PathConfig. No saved
user path override was identified. Current adapter sets cwd to Application.StartupPath
in `src/launcher/launcher_reader.cpp`190. Verified stock hashes establish files exist;
they do not prove adapter initialization/cwd or the tester's launch registration.
Reinstall alone cannot establish absence of retained mod files or launch overrides.

## Build, packaging and offline checks

`build_installer.ps1` validates13 checksums and nine x86 DLL headers, embeds13 files,
SHA256SUMS and two pinned inventories (16resources), compiles warnings-as-errors,
and validates the actual compiled parser before replacing any output. Packaging
reuses the verified canonical native tester stage, verifies all resources, copies
three player docs, generates outer checksums and tests the five-file ZIP. Previous
release is retained. Build/package do not deploy or launch.

Run `src/installer/tests/run_tests.ps1`:508 integrated assertions pass, including
34 bounded Steam metadata and44 memory-registry assertions. Coverage includes
true A-to-B upgrades and rollback; nested DLL/ASI cleanup; stock preservation;
missing/patched executable/DLL refusal; real synthetic locks, complete/partial
rollback; XML/settings; linked folders; labelled keyboard controls and explicit
registry consent. Actual compiled EXE validates both inventories and refuses
nonstock placeholder originals. With an explicitly injected synthetic catalog,
it installs all13 embedded files twice and removes a foreign module. Standalone
build refuses corrupt catalog resources and preserves previous output. No fake
native module/game binary is loaded or executed. The short process-refusal helper
is the synthetic test assembly copied as FFX.exe, not the game.

Logs/evidence: `install/installer-migration-20261007/installer-tests.log`,
`package.log`, `actual-payload-install.log`, `archive-verification.json` and
`verification.json`. Native payload remains unchanged: reuse the fresh canonical
0.5.1 tester build/five final checks and prior34 native coverage. Interactive
screen-reader/folder-picker/UAC/registry and gameplay acceptance remain user-side.

## Sol delegation tracking

All delegated tasks used gpt-6.1-sol. Initial read-only audit found absent broader
cleanup, missing original-executable checks and repeat-only tests. Primary reviewed
and implemented stock preflight, cleanup, upgrades and packaging. Sol implemented
SteamAppManifest and34 tests; primary reviewed/integrated malformed/location/depot
refusal. Sol's integrated review found no blocking defect and two build diagnostics;
primary fixed resource-count wording and standalone inventory validation.

Sol implemented scoped registry backend/backup/restore and44 memory assertions.
Primary reviewed exact preview, type/data/filter preservation and rollback, added
complete backup identity comparison, integrated consent/UAC, and verified508
combined checks. No real registry/game/interactive testing was delegated. Exact
private-mod attribution and live acceptance remain unresolved, not claimed complete.

## October 12, 2026: one installer with PRISM and SDL3

The latest user instruction explicitly supersedes the separate PRISM prerequisite packaging rule: one `InstallFFXScreenReader.exe` must contain and install its necessary dependencies. `PrismInstall.cs` is an internal dependency component of the existing installer; there is no companion installer or second GUI. The newly drafted standalone companion sources and output were retired. No earlier source/build was restored.

The single x86 EXE embeds 26 resources: seven native payloads (including SDL3 and its license), 15 exact locked PRISM DLL/provenance/license/notice files, the source lock, native checksum inventory, and two stock-game inventories. `build_installer.ps1` validates all locked bytes and x86 images, compiles warnings as errors, and runs the actual compiled resource/parser validation before replacing its output. Builder, packager and test runner dispatch to the matching x86 Windows .NET Framework host; PowerShell 7/x64 could not load the x86 installer for reflection validation.

Main installation validates closed processes, embedded payloads, stock files, ordinary affected paths, configuration and backup location before installing a missing PRISM runtime. A named per-game SDK mutex guards a GUID stage within the exact game folder. Each of 15 files is verified before and after writing; the stage includes the matching ownership receipt and becomes `PrismRuntime` by a same-parent move. Ancestor/reparse and closed-process checks are repeated. An existing matching receipted runtime is verified with zero SDK writes; nonempty incompatible, unreceipted or modified SDKs are refused without replacing them. Failed newly created stages are retained for diagnostics and excluded from module cleanup. If a subsequent mod installation fails, its existing backup transaction handles affected mod files; the newly installed dependency remains and no prior SDK is replaced.

Tolk.dll is explicitly included in the affected backup/removal set; nested added Tolk DLLs follow the existing module migration. The old helper libraries remain separately owned. SDL3.dll and FFX_SR_SDL3_LICENSE.txt are always in the seven-file native installation. PRISM's MPL license, notice, bundled-component license files and provenance are installed inside PrismRuntime. MinHook's existing complete notice remains in the installed native dependencies document; the Sphere Grid data license remains embedded in the native DLL.

Fresh `DependencyInstallerTests.cs` passes 130 assertions against the actual current-source installer resources: missing SDK installation, all seven exact native bytes including SDL3, 15 SDK files plus receipt/licenses, Tolk root/nested backups and removal, preserved settings, repeat unchanged SDK hashes/timestamps, corrupt embedded SDK refusal before directory creation, unreceipted/corrupt installed SDK refusal without writes, modified-stock preflight, >260-character paths, and production process/stock refusal. Its synthetic catalog and guard are injected only into the fixture constructor; production constructors always use the real closed-process guard. No PRISM/native DLL or game placeholder is loaded/executed, and no real registry writes occur.

Evidence: `install/release-blockers-20261012/dependencies.log`, `InstallFFXScreenReader.exe` and the retained synthetic fixture directories. The older full integrated suite could not finish: its junction fixture required sandbox escalation (granted), then an open game/launcher session triggered the production guard. That earlier refusal was not a passing whole-suite result. The later full run, after no live game/launcher processes were present, passes 795 assertions, repeated actual-resource installs and long-path checks; see `install/release-blockers-20261012/installer-full.log`. Existing suite resource mappings reflect 26 resources. The retained `run_dependency_tests.ps1` runner also passes the 130 focused assertions and verifies the developer receipt check accepts Windows path capitalization without writes and rejects extra inventory/missing ownership; see `dependencies-final.log`. Deployment/release remain held by the strict threading audit, independently of the successful installer compile. No game was closed/controlled or launch performed.

Delegation: Luna gpt-6-luna medium supplied the bounded standalone build-script draft; Sol corrected its x86 host and entry point and compiled it, then retired it when the user requested one installer. Sol implemented/integrated/reviewed/validated the unified component and focused tests. Luna gpt-6-luna medium updated README/changelog/controls/release documentation from specified verified behavior; Sol reviewed resource count, version/hold status, keys and exact Stone Gaze link. No remaining delegated source defect; overall threading and user-side gameplay acceptance remain open.

## Premature artifact and night stop — October 12, 2026

The user rejected building the installer before game deployment was unblocked. Existing 0.5.6 EXE/ZIP artifacts are unusable as a release and must not be installed or distributed. Preserve installer source and the historical 795/130-assertion evidence, but recompile against the final verified deployable native payload and regenerate the package/checksums after deployment is unblocked. Work is stopped for the night; no further build or test was performed in this documentation pass. [Remaining blockers and required resume order](threading_audit_20261012.md#night-stop-and-required-resume-order).
