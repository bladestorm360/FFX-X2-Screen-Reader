# Build, deployment, launcher, installer and developer utilities

October21: [the1.0/GitHub release procedure](../../release.md) supersedes the
historical release0.5.9/Dropbox descriptions below. The user explicitly resumed
deployment/packaging and requests public GitHub publication.1.0 is built,
guarded-deployed and its final single-installer ZIP verified. The current GitHub
repository still retains a removed game-code object; a safe clean-repository
publication decision is pending. Future prepare-release requests include the
reviewed source commit/push, annotated tag, verified draft ZIP and publication.
Native Windows version metadata/startup stamp and installer version are1.0.0.0;
`verify_release.ps1` independently verifies the final package in an x86 host.

## Build and native payload

`src/proxy/build_dinput8.bat` is the production x86 entry point; `src/build/build.bat`
forwards to it. The default developer build enables developer utilities;
`--release` excludes them, and `--no-deploy` stages without changing the game
installation. The prototype flag validates historical JS offline only. The
native build includes the current `src/native/*.cpp`, resources, x86 MinHook,
SDL and the original launcher adapters. It stages a proxy DLL, native launcher
DLL, C++/CLI launcher adapter and locked dependencies. Inspect entry-point side
effects rather than assuming that a successful compile did not deploy.

`--test-build` also excludes the development console. The default development
flavor enables the requested October20 Baaj console test. The user now requests
release0.5.9, which excludes that console; installer preparation is authorized.
[Console activation and exclusive F1 ownership](../runtime/console_f1_20261020.md).

`src/proxy/deploy.bat` is the guarded deployment path. `verify_deployment.ps1`
checks closed game/launcher processes, actual compiled profiles/signatures and
dependencies before any live replacement, then verifies all seven installed
payload hashes and launcher bootstrap afterward. Existing PRISM runtime/receipt
is checked read-only. No running module is replaced, no game entry point/hooks
are executed by preflight, and no real registry repair is part of verification.
`FFXAccessibility.ini` is preserved. Deployment evidence must say whether it
actually happened or was blocked, not merely report a successful staged build.

[Release build/payload/process evidence](../../release.md),
[PRISM dependency ownership](../../prism_migration_20261012.md) and
[October19 0.5.8 package receipt](../../../install/release-0.5.8-confirmation-20261019/release-verification.json)
and [October20 Baaj deployment receipt](../../../install/baaj-fix-20261020/verification.json)
are historical provenance. The user now requests0.5.9 with the October20 Baaj,
audio, keyboard, password, Cid and Sin changes. Sea of Sorrow is gameplay-confirmed;
later Sin confirmation remains tester-side. The new minigame solvers are deferred
to prospective0.6. [Final0.5.9 release evidence](../../../install/release-0.5.9-20261020/release-verification.json)
and [guarded deployment](../../../install/release-0.5.9-20261020/deployment-verification.json)
verify the actual single EXE,26 embedded resources,5 ZIP files and preserved
settings/saves/previous packages. No publication occurred.
The same-version Nucleus follow-up supersedes that original package:
[refreshed receipt](../../../install/nucleus-prearm-20261020/release-verification.json).
Saved Sin-only controls, both room aliases and entry/battle retention pass fresh
collection/menu/settings/both reader profiles, guarded deployment and actual
installer checks. Dropbox replacement is byte-verified; desktop Discord delivery
is explicitly authorized. See the current release record for delivery status.

## Launcher and installer

`src/launcher` retains the original launcher with a C++/CLI event adapter and
shared native PRISM adapter. UI events, not an unattended poller, announce
focused launcher controls. Launcher runtime/dependency ownership is separate from
the FFX game-hook profile. The separate Will profile only provides its verified
caption/audio path, not FFX-2 gameplay accessibility.

`src/installer/build_installer.ps1` embeds the verified seven-file payload into
the self-contained Windows InstallFFXScreenReader.exe. `package_release.ps1`
stages that EXE, docs/checksums and ZIP while preserving the previous package.
The installer deploys or verifies the locked PrismRuntime/licenses, backs up and
removes Tolk as the documented migration requires, and supports the verified
long-path I/O route. Distributed batch/PowerShell setup helpers are retired;
internal build/deploy batches remain the developer workflow.

Normal Install/Check launch redirection never writes registry values. Optional
Repair launch redirection and Restore launch backup are separate explicitly
requested UI actions, limited to five documented IFEO Debugger names with
installation filters, preview/confirmation and verified XML backup. Agent tests
use an in-memory registry backend. LAA/profile handling preserves and verifies
the supported executable identity; it is not authorization to patch arbitrary
binaries or run real registry repairs during testing.

[Installer behavior and registry boundaries](../../windows_installer_20261007.md),
[long paths](../../installer_long_paths_20261009.md),
[launcher/LAA identity](../../launcher_laa_20261012.md) and
[release resource/install assertions](../../release.md) retain implementation,
native ownership, package manifests and test commands. Add a payload only by
updating build staging, embedded resource manifest, locks/licenses, installer,
deploy validation and package checks together. Do not infer a new valid package
from an old installer whose embedded DLLs predate the repair.

## Developer utilities

`utilities.cpp`, `utility_console.cpp`, `utility_feedback.cpp`, `utility_warps.cpp`
and `utility_trials.cpp` expose the development-only Shift+F1 console and utility
commands. Release builds do not ship its activation route. The Windows console
UI has its own thread/focus exclusion; text/replies are owned recipes, while
game-affine actions are revalidated and deferred to the native action owner.
`utility_context.cpp` rejects stale generation, battle/dialogue/menu and changed
script/allocation contexts. Command parsing does not grant permission to perform
an action on an arbitrary worker/address.

The console request and reply capture queues each hold eight entries. The UI
owner holds the editable command/history controls; queued native utility work
waits for field preparation and revalidates context/generation before execution.
Catalog parsing and reply formatting happen before the native action, while
game-affine changes are deferred to their owner. When extending a command, add
parser validation and refusal behavior beside it, preserve that native-owner
recheck, and keep progression-changing test setups opt-in.

Ordinary `warp` commands preserve progress and request the native transition;
they do not promise an unfinished trial, native party setup or a valid later
story state when loaded from an early save. Explicit Macalania/Gagazet test
commands have separate bounded original-story/context checks and documented
native setup writes. They do not edit an on-disk save. Gagazet test setup still
requires the native Prepare swimming party event before entering water.
Preserve these deliberate distinctions instead of silently changing ordinary
warps to reset flags. [Console threading/focus](../../console_thread_hardening_20261008.md),
[trial test transitions](../../trial_entry_followups_20261016.md) and
[Gagazet test setup](../../gagazet_cave_20261016.md) contain exact allowed state
changes, refusal cases and commands.

## Maintenance checks and evidence boundaries

Run the relevant native fixtures before the canonical build. Profile/preflight,
proxy forwarding, launcher/installer dependency, actual-resource and in-memory
registry tests cover different boundaries; an installer source-string assertion
does not replace an actual embedded-resource or install/reinstall check.
Current release evidence includes those separate stages and the short/long-path
cases. Native source comments/docs can be maintained without changing gameplay;
record which source revision a compiled payload actually represents. See the
[coverage audit](../documentation_audit_20261020.md) for the exact native units
reviewed and this page's evidence limits.

Offline retained commercial-game evidence is read-only and attributed. Installed
source asset reads require task-specific approval; game launch/control/live probes
require separate permission. The restricted Tester Logs tree is excluded from
default discovery. Reading a Discord report/download does not grant access to
unrelated tester submissions or permission to message the tester. No commit,
push, publication, package rebuild or real registry operation is implied by a
documentation update.
