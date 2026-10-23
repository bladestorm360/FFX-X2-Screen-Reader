# Release procedure — GitHub Releases

A request to **prepare release X.X** authorizes the complete release: update the
player documents/version, build and verify, guarded-deploy, package the single
installer, commit/push the reviewed source, tag, verify an uploaded draft ZIP and
publish the GitHub release. This follows FFPR Core's release convention. GitHub
Releases replaces Dropbox delivery. Sending messages to other people remains a
separate action. Repository renames and history rewrites require their own
explicit authorization; they are not routine release steps.

Repository/downloads: https://github.com/bladestorm360/FFX-X2-Screen-Reader

## Standard procedure

1. Read guidance/memory and inspect the existing branch, remote owner/visibility,
   worktree, tags and releases. Preserve other work, settings, saves and previous
   packages. Never overwrite a published release or silently replace its asset.
   Check the source-content boundary, including staged files and all published
   history. Commercial-game bodies, raw inputs and analysis exports stay outside
   every Git tree. A known retained game-code object blocks making that repository
   public even after its reachable refs were sanitized.
2. Update README, CHANGELOG and controls from actual features/bindings, retaining
   credits and known issues. Use short paragraphs and semantic Markdown lists
   for separate issues/credits; single source newlines collapse on GitHub.
   Verify rendered HTML structure rather than relying on source wrapping.
   Set the requested public version in
   `src/native/build_version.hpp`, `src/installer/AssemblyInfo.cs` and the packaging
   default. The native Windows resource reads the shared version header; the
   startup log uses the public spelling. Preserve pinned runtime versions and the
   launcher's independent stable AssemblyVersion/bootstrap identity.
3. Run applicable native/worker/startup/utility/profile checks. Reuse prior native
   fixture evidence only with matching source/input hashes and explicitly record
   metadata-only differences. Build with
   `src/proxy/build_dinput8.bat --release --no-deploy`. Check warnings, x86 PE and
   numeric/string file/product version. Release excludes the developer console.
   Build/verification never launch or control a game.
4. Confirm closed game/launcher processes and use `src/proxy/deploy.bat`. Verify
   actual compiled executable profiles/dependencies, all seven installed/staged
   hashes, launcher XML and locked PRISM receipt. Keep settings/saves unchanged.
   If deployment is blocked, report the blocker; do not compile/package an
   installer from an unverified payload. Never replace running modules.
5. Run installer source tests, then package with
   `src/installer/package_release.ps1 -Version '1.0' -SevenZip 'C:/Program Files/7-Zip/7z.exe'`,
   substituting the requested version. The single x86 installer embeds26
   resources: seven mod/native files,15 locked PRISM runtime/provenance/license
   files, the source lock, checksum anchor and two pinned stock-game inventories.
   Matching existing PRISM is checked read-only; missing runtime installs with
   licenses. No separate dependency installer is shipped.
6. Test the actual final EXE's resource/dependency and short/long fixture
   install/reinstall paths. Registry fixtures use the in-memory backend only.
   Independently check ZIP CRCs, one named root, exactly five files (installer,
   README, changelog, controls and SHA256SUMS), source/document/embedded-byte
   equality, every checksum, versions and pinned runtime/license hashes.
   Preserve existing local packages under release-backups before regeneration.
7. Review the intended source/document diff and content audit. Commit/push the
   current branch to the existing user-owned remote. Build output, local receipts,
   packages, logs and commercial-game evidence remain uncommitted. Create/push an
   annotated `vX.X` tag at the verified source commit. Check local/remote tag and
   commit identity. A release asset is distributed through Releases, not Git blobs.
8. Write concise release notes with installation, known issues, README link and
   ZIP SHA256. Create a draft at the verified tag and upload the ZIP. Verify draft
   title/tag/source/asset name/size; download the draft asset into ignored local
   verification storage and compare its SHA256 with the local archive. Publish
   only the verified draft, mark it Latest and verify the public URL, non-draft/
   non-prerelease state, source/tag and asset digest/download. On failure preserve
   the local artifact and report the exact unfinished operation.
9. Record the final release URL, tag/source commit, hashes and evidence here and
   link them from concise memory. Commit/push the final publication record without
   altering tagged code or distributed bytes. Verify clean worktree and remote
   branch identity. Return a short public release/download link.

Ordinary developer deploy verifies PRISM read-only; it does not upgrade/repair it.
The installer retains the explicit launch-repair preview/confirmation/backup
workflow. No real registry repair or gameplay probe is part of a release.
Implementation/offline verification and user gameplay confirmation remain distinct.

## Release1.0.1 — October23

The requested release includes the [all-six destruction-sphere flag repair](features/puzzles/destruction_flags_20261022.md)
and [launcher language/settings repair](features/distribution/launcher_languages_20261023.md).
The user confirms Besaid sphere collection in gameplay. The remaining Baaj check
is remote reproduction of the Ruins unlock through normal gameplay without
warps. A tester reported Zanarkand missing, but its flag was already present in
the supplied save; a missing Zanarkand flag is not established. Actual launcher
focus/audio confirmation remains pending. Exit direction behavior is unchanged.

Local verification is complete. Warning-free x86 release build and native/setup
1.0.1.0 metadata pass; the development console is excluded. Startup forwarding,
both native utility modes/all-six collection cases, final launcher adapter with
eight startup locales/seven actions/64 settings transitions, both compiled-image
preflights, guarded deployment and all seven installed hashes pass. Previous
native/reader/original-bytecode evidence is reused with matching feature-input
hashes; native release identity is separately recorded. No game was launched
or controlled by the agent and no Tester Logs was accessed.

Installer source805 assertions, actual final-EXE dependency130 assertions,
actual final-EXE short98/long316-character install/reinstall,26 embedded
resources, pinned runtime/licenses, every checksum/source-document equality and
the exact five-file ZIP pass. Saved settings and the published1.0 package are
unchanged. Evidence and source fingerprints are in
`install/release-1.0.1-20261023`; `verify_release.ps1` independently verifies the
final package. Full current-content/local-history audit is clean, including1092
Git blobs. Source commit/tag and draft upload/publication verification follow.

- ZIP: `releases/1.0.1/ffx-x2-screen-reader-1.0.1.zip`,2,595,055 bytes.
- SHA256: `d38fdcd355f52f0de1891f765c66f24a51353aedc3b5e69d023ba54201218f52`.
- Installer SHA256: `325918052b964647b5278c340117be58ee0833ae4d1e1fd4d23b7c13b2a9939b`.
- Local receipt: `install/release-1.0.1-20261023/verification.json`.

Luna (`gpt-6-luna`, medium) supplied the bounded README/changelog/controls update.
Sol reviewed against the verified behavior, removed extra spacing and applied
the user's later Besaid/Baaj/Zanarkand clarification before packaging. Exact
packaged player-document equality passes. No delegated issue remains; all
implementation, source review, build/deploy, verification and publication work
stays with Sol. No Astra was used.

## Release1.0 — October21

The user explicitly resumed deployment/installer packaging and requested public
GitHub publication.1.0 includes the four completion assists previously called
local0.6 and all October21 battle/menu/HP repairs. See the changelog and
[feature register](features/status.md). Newly reported fixes retain their listed
pending gameplay checks; a final version number does not close known issues.

The pre-release source/HP native/worker evidence is in
[the HP record](features/battle/hp_sources_20261021.md). The only subsequent native
behavior change is the release startup stamp; Windows version resources and
installer metadata are1.0.0.0. Release build/deployment/installer/package/GitHub
receipts are retained under `install/release-1.0-20261021`.

Local verification is complete: warning-free release build, guarded deployment/
all7 installed hashes, source805/actual dependency130 installer assertions,
actual final-EXE short/long install/reinstall,26 embedded resources and the exact
five-file archive pass. Settings and0.5.7/0.5.8/0.5.9 packages are preserved.
The independent verifier is `src/installer/verify_release.ps1`.

- Source: `625af61d685b335d86314b0aefde9c04d815940e`.
- Annotated local tag: `v1.0`, tag object `f7a3e6f02d016b7660bcf6ca3045cfc10d4f6c56`.
- ZIP: `releases/1.0/ffx-x2-screen-reader-1.0.zip`,2,593,240 bytes.
- SHA256: `768c49042b3c73966a97656ed0f22973402860c908614b0b90d17ddd49a2963d`.
- Receipt: `install/release-1.0-20261021/verification.json`.

Luna (`gpt-6-luna`, medium) supplied the bounded README/changelog/controls update.
Sol reviewed/corrected wording/format and verified their exact packaged bytes;
all native/version/build/deployment/installer/repository work was retained by Sol.
No Astra or unresolved delegated edit. Known gameplay limits remain in the player
documents. Failed manual reflection in a64-bit host and a fixture call missing
its explicit PRISM test environment were corrected in the x86 independent
verifier/configured test host; those failed invocations are not passing evidence.

Publication is complete. The user deleted the old private repository after the
authorized CLI DELETE returned HTTP403. Authenticated owner/name404 was verified
before creating a fresh private repository, restoring clean master/three tags,
and making it public. New repository ID1428963798 differs from old ID1191559099.
All32 known removed objects return404 in the replacement repository; the known
source blob also returns404 anonymously after publication. These checks establish
API inaccessibility, not physical erasure of GitHub backups.
[Replacement/access evidence](features/remote_cleanup_handoff_20261020.md).

- Public release: [FFX Screen Reader1.0](https://github.com/bladestorm360/FFX-X2-Screen-Reader/releases/tag/v1.0).
- Release ID420089987; published October21 at21:51:23UTC; Latest, non-draft and
  non-prerelease. Asset ID663887107 is the ZIP above.
- Uploaded draft and anonymous public downloads match the recorded ZIP SHA256.
- The user chose to leave collaborator permissions unchanged on the replacement;
  no collaborator invitations were sent.

After publication, the user requested accessible formatting in GitHub's
`README.md`. Commit `71ac394` splits long paragraphs into short paragraphs and
semantic lists. GitHub's renderer and the published README HTML contain nine
separate known-issue items and seven credit items. This verifies HTML structure;
an actual screen-reader session was not run by the agent. The mistaken initial
packaged-text edits were discarded before commit or packaging. The release ZIP,
installer and tag remain byte-for-byte unchanged. Packaged-document equality is
against tagged source625af61, before this website-only formatting correction.

Final publication and README receipts are in
`install/release-1.0-20261021/publication-verification.json` and
`readme-public-verification.json`; external replacement recovery/checks are in
`Analysis/FFX/RepositoryCleanup_20261021`.

Earlier delivery records, Dropbox history and superseded release holds remain
in [the historical release record](release_history_before_1.0_20261021.md).
