# Native test release procedure

Latest October14: user confirms the previous real Jecht solver, manual prompts
and taunt suppression. The centered tutorial Confirm-only loop is repaired.
Requested auto pacing now targets750ms and stops at the game's original score11;
24 original full rounds pass exactly11, with23-step gaps and final responses
at254/255 of300 steps (about8.5s). All-four tutorial/manual/176-query/lifecycle
checks, warning-free canonical eleven-build1, guarded actual FFX/Will deploy1
and installed7 hashes/PRISM receipt PASS. New tutorial/pacing gameplay remains
pending; installer HELD, marker retained, monitor PAUSED.
[Current evidence](jecht_shot_20261014.md#eleven-answer-pacing--october-14).

**Current October13 instruction:** finish all remaining current-source deployment blockers, implement Stone Gaze/related ability speech, then guarded-deploy and verify the final installed payload. **HOLD installer rebuild/package for in-game testing**, even after deployment; wait for a later user instruction before rebuilding. The reset heartbeat is paused. The already-built0.5.6 installer/ZIP remains premature and unusable; do not install or distribute it. Preserve source/history and evidence. This hold supersedes automatic installer compilation in the historical numbered procedure below.

Release **0.5.6 tester payload is deployed and verified**, October14. Current-source blockers, Stone Gaze/shared debuff feedback and queued Jecht support passed offline checks and the warning-free canonical x86 native/launcher build. Guarded preflight/installed checks pass both FFX and Will; all seven installed hashes and PRISM0.18.3 x86's15 locked files/receipt match. Tolk is absent and SDL3/license installed. [Final verification](../install/reset-resume-20261013/final-deployment-verification.json), [Jecht evidence](jecht_shot_20261014.md), [gameplay procedure](gameplay_acceptance_0.5.6.md). Installer recompilation/repackaging is still **HELD for user gameplay**. The previously compiled EXE/ZIP and its historical fixture counts below are not release-ready; retain the do-not-use marker.

October14 follow-up: the reported Jecht practice diagonal now speaks both
directions together. Both reader profiles, the clean canonical build, guarded
redeployment and fresh installed hashes/PRISM verification pass. The current
verification record covers this corrected payload; installer hold remains in force.

Later October14 follow-up: Jecht practice and timed cues now use control names
only (Up Right C), while explanatory text stays complete. Focused cue checks,
both reader profiles, clean canonical build, guarded redeployment and fresh
installed file/PRISM verification pass; the current record covers this payload.

Latest October14 repair observes the actual Jecht show/sound executors inside
the multi-opcode interpreter. Auto reacts450ms from visible show; real manual
cues have no intentional delay, pending cues speak in order and equal events
remain separate. SFX taunt subtitles follow the voiced dialogue setting.
Native6/dialogue3/both144 reader profiles/profile1, warning-free canonical
realtime build1, guarded actual FFX/Will deployment and installed7 hashes/PRISM
receipt verification PASS. Gameplay audibility/timing remain user-side; installer
rebuilding stays held. [Current evidence](jecht_shot_20261014.md#actual-show-producer-reaction-timing-and-sfx-subtitles--october-14).

Current production procedure, replacing the historical Gadget/JavaScript process. Installer compilation and packaging are gated on unblocked, verified deployment. Keep the exact requested version string: **0.5.6**. Do not commit, tag, push or publish unless separately requested. Preserve the working tree and record known issues.

1. Read project guidance and current memory; inspect the working tree and release directory. Update README.md, CHANGELOG.md and player controls for the requested release.
2. Follow the primary task's selected build path only after the build choice and known resource-reset blocker are resolved. The canonical production entry point is `src\proxy\build_dinput8.bat`; inspect its deployment side effects and use its documented no-deploy option when preparing a local package. Do not ship Frida/Gadget, JS, reference trees or debug tools.
3. Run the applicable native and installer checks after the selected build. Existing evidence is not new validation for 0.5.6; retain fresh build and verification logs.
4. After the remaining blockers are fixed, complete guarded deployment and verify the installed final native payload/PRISM receipt. If deployment is blocked, do not compile or package an installer. Then recompile the single installer, which embeds 26 resources: seven mod payloads (`dinput8.dll`, `MinHook.x86.dll`, `SDL3.dll`, `FFXLauncherReader.dll`, `FFXLauncherNative.dll`, `FFX_SR_SDL3_LICENSE.txt` and `FFX_SR_Native_Dependencies.md`), 15 locked x86 PRISM 0.18.3 runtime/provenance/license/notice files, `prism.lock.json`, the checksum inventory and two pinned game inventories. It installs PRISM into PrismRuntime. Matching receipted runtime contents are verified without writes; incompatible, nonempty or unreceipted contents are refused without replacement. It backs up/removes Tolk.dll. No companion installer is used.
5. Package with `src\installer\package_release.ps1 -Version '0.5.6' -SevenZip <exe>`. The script validates the embedded resource parser and 26 embedded resources and seven native payload hashes, copies README/CHANGELOG/controls, creates checksums, creates and tests a ZIP with one named top-level package folder, and preserves any previous local package under `install/release-backups`.
6. Review the resulting package and checksums against the source, confirm only the five distribution files are in the package folder, and record fresh verification. Never launch the game as part of packaging.
7. Report the ZIP path and known issues. Update concise release facts in memory and the established plan/debug documents. Packaging stages only; live deployment and remote publication are separate operations.

The packaging entry point covers staging, installer creation, outer checksums, and ZIP creation/integrity checking. Installer acceptance checks and native build/deployment remain separate. Runtime/gameplay testing remains user-side unless specifically authorized.
## 0.5.5 prepared — 2026-10-09

Prepared `releases/0.5.5/ffx-x2-screen-reader-0.5.5.zip` with the exact requested
version, a single named root and five files: InstallFFXScreenReader.exe,
README.txt, CHANGELOG.txt, CONTROLS.txt and SHA256SUMS.txt. All three documents
contain the open Grid Status and post-jump passage limitations. Existing
maintenance is included; no native/installer feature code changed in this pass.

The canonical x86 `--test-build --no-deploy` build passes without compiler
warnings. Development-console speech/code and its entry point are absent from
the actual proxy. Five fresh input/DirectInput/proxy/utility checks pass; the
unchanged utility fixture emits two existing C4459 parameter-shadow warnings,
which are recorded separately from the clean production build. Reused unchanged
native evidence includes both 131-site reader profiles and affected checks from
`install/exit-status-followup-20261009/verification.json`. This offline evidence
does not override the user's reports of incomplete gameplay behavior.

Installer source passes 558 synthetic assertions, warnings-as-errors UI
compilation, corrupt-inventory refusal and short/long embedded-resource tests.
The actual final 0.5.5 EXE validates both pinned stock inventories and installs
all 13 real embedded resources twice at both 129- and 347-character fixture
paths, preserving settings and backing up/removing added modules. No native
game code or installer UI runs. Registry tests use the in-memory backend only.

All 13 embedded hashes match production sources; installer and payload DLLs
are x86. 7-Zip and independent ZIP-byte/source-document/checksum verification
pass: exactly five files and 2,284,728 archive bytes. Prior 0.5.1-Test Build ZIP
retains SHA256 `2c4fc6c85b0a13289f155b77facfeda2698fadd4a243b91e798b9952f1f77199`.

0.5.5 ZIP SHA256:
`71dc87ff7ba556d0e06bae6511ad139ab20f813ed6f30e2eba1a46096df50db3`.
Installer SHA256:
`669ec02257815206d0f83ef46e011f590ac525b6f9b7f5d10c88e653df81558d`.
Release proxy SHA256:
`1c68bb42311a487714bedf81972b9f5edd6b736bf29f9c8a50a6cdac3573ec5b`.

Evidence: `install/release-0.5.5-20261009/verification.json`, native-build.log,
final-native-checks.log, installer-tests.log, package.log and
actual-payload-install.log; repeatable byte/known-issue verification is
`verify_release.py`. Sol worked alone. No game launch/control/probe, Tester Logs
access, live deployment, actual registry edits, commit/tag/push or publication.
Gameplay and interactive installer acceptance remain user-side.

## 0.5.1-Test Build prepared — 2026-10-07

Prepared `releases/0.5.1-Test Build/ffx-x2-screen-reader-0.5.1-Test Build.zip`.
Its single named package contains InstallFFXScreenReader.exe, README.txt,
CHANGELOG.txt, CONTROLS.txt and SHA256SUMS.txt. README now covers T dialogue
replay, Ctrl-arrow Blitzball/swimming review, Blitzball/race queries, configured
pedestal pushing, Auto walk and its independent path speech setting. Field
controller routes use right trigger directly; corrected the full guide.

Canonical tester build passed without warnings; reused unchanged native
34-check coverage and reran the five final input/proxy/utility checks successfully.
Installer checks passed 201 synthetic assertions and verified all 13 embedded
source hashes/x86 DLLs. The final EXE installed its real payload twice into a
workspace fixture, preserving settings, unrelated mods and launcher XML/backups.
7-Zip and independent ZIP-byte/document/checksum checks passed: five files,
2,080,195 archive bytes. Previous 0.5-Test Build package remains untouched.

Evidence: `install/release-0.5.1-20261007/verification.json`, `native-build.log`,
`final-native-checks.json`, `installer-tests.log`, `package.log`,
`archive-verification.json` and `actual-payload-install.log`. The first build
attempt could not discover Visual Studio under the sandbox account; the retry
used the documented per-process tool paths and the same canonical entry point.
No feature code was changed, game launched/probed, live deployment or remote
publication performed. New gameplay and interactive installer acceptance remain
user-side; FFX-2/Last Mission readers remain unavailable.

## Installer migration and launch repair refresh — 2026-10-07

Refreshed the same **0.5.1-Test Build** ZIP with original Steam DLL/EXE inventory
validation, full added DLL/ASI cleanup, true upgrade/rollback tests, and optional
explicitly confirmed FFX Debugger registry repair/backup/restore. Normal Install
remains registry-read-only. No actual registry edits, game launch/probe, deployment
or publication occurred. Prior0.5.1 package is preserved under
`install/release-backups/0.5.1-Test Build--20261007-155640-26dc2d23`.

508 integrated synthetic assertions pass, including34 Steam metadata and44 registry
memory checks. Actual final EXE validates two pinned inventories, refuses nonstock
originals, and installs its13 real embedded files twice into an injected synthetic
stock fixture with module cleanup/settings preservation. Corrupt-inventory build
refusal and offscreen explicit-consent checks pass. Native payload unchanged;
reuse the fresh canonical tester build/five final checks and prior34 coverage.

7-Zip plus independent ZIP-byte/document/checksum checks pass: exactly five files,
2,177,822 bytes. ZIP SHA256:
`2c4fc6c85b0a13289f155b77facfeda2698fadd4a243b91e798b9952f1f77199`.
Installer SHA256:
`dcb71ed000864d6010c68ebc437e033379558390b32755ade6b3c9ac6e138cbf`.
Evidence: `install/installer-migration-20261007/verification.json` and linked logs.
[Installer architecture, Sol handoffs and limits](windows_installer_20261007.md).
Live screen-reader/UAC/registry and gameplay acceptance remain user-side; the
private mod's exact redirect mechanism is not yet attributed.


## 0.5.6 premature candidate — unusable, October 12, 2026

The current-source candidate ZIP is `releases/0.5.6/ffx-x2-screen-reader-0.5.6.zip`. It contains exactly one `InstallFFXScreenReader.exe`, three player documents and outer checksums. The installer contains all 26 verified resources including PRISM/runtime licenses and SDL3; Tolk removal is backed up. Fresh 795 integrated assertions and 130 actual-resource dependency assertions pass, including long paths and read-only developer dependency identity/inventory checks. Source repairs, owned logger and the native/launcher x86 tester build are offline verified. See `install/release-blockers-20261012/verification.json` and linked logs.

This premature artifact is unusable as a release and must be recompiled/repackaged when deployment is unblocked. It must not be installed or distributed. Strict whole-mod threading remains NONCONFORMING; additional speech/listener/native-reset/scan/route/text ownership work remains. No earlier build/source was restored, no game or real SDK was run, and Stone Gaze is documentation-only. README and controls list new bindings since 0.5.5. Gameplay/interactive installation acceptance remains user-side.
