# FFX PRISM migration status

Status: **migrated and guarded-deployed**, October 12, 2026. This is the
speech adapter/dependency migration; strict whole-mod threading is still
NONCONFORMING and gameplay/real-backend acceptance remains user-side.

Game and launcher use the central native PRISM adapter. Backend calls remain
serialized on the speech owner; copied UTF8, speech+braille, intentional
braille-only, capability/failure/selection/lifetime and repeated event behavior
pass isolated fake-ABI checks. Asynchronous bootstrap, fresh proxy forwarding
and the launcher surrogate pass. No real speech engine or game was executed.

The separately installed x86 prerequisite is locked at commit
`b68e9c94240cefb1f08b2306c26376e3ab300a7a`, version0.18.3. Native SDK SHA256:
`1aa76c8835963eccd210255b06b4db48551bd174dc965d205c9f68ae0c726885`. Normal deployment verifies the installed
PrismRuntime receipt/immutable files without replacing that SDK; seven mod
payload hashes and both executable profiles pass. Original Tolk.dll is backed
up in the priority delivery's `before` directory and removed from the live
game root only after successful verification. Existing unrelated legacy files
are retained. No executable flag or registry modification was performed.

[Delivery verification](../../Analysis/FFX/RepositoryEvidence/historical-install/via-swimming-priority-20261012/verification.json),
[repair history](open_issue_repairs_20261012.md#prism-follow-up-review-and-actual-dependency-installation),
[current threading audit](threading_audit_20261012.md).

The later speech-status candidate is **built/offline-verified/UNDEPLOYED**.
Owned backend metadata and atomic backend requests replace metadata/request
locks; the auto-reset event passes the exact pre-wait lost-wake check. Sol fixed
published-handle failure lifetime and verified the locked SDK's15 backend names.
Producer speech/log/cancellation locks still need conversion. This does not
change the migrated/deployed status of the earlier adapter/dependency delivery.
[Candidate checks](../../Analysis/FFX/RepositoryEvidence/historical-install/speech-status-candidate-20261012/verification.json),
[ownership and remaining reset gap](threading_audit_20261012.md#speech-status-and-review-completion-checkpoint).

Latest release packaging instruction supersedes the separate-prerequisite rule above: the single mod installer now contains/installs the locked PRISM runtime/licenses and SDL3, backs up/removes Tolk, and verifies matching existing SDK bytes without replacement. The 26-resource candidate compiles and passes 795 integrated plus 130 focused assertions; it is UNDEPLOYED pending the wider threading blockers. [Installer checkpoint](windows_installer_20261007.md#october-12-2026-one-installer-with-prism-and-sdl3).
