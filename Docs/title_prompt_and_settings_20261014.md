# Opening instruction and preserved assistance settings — October 14, 2026

Current opening corrections supersede the historical delivery below; see [the final October14 repair](opening_escape_help_20261014.md). Installer rebuilding remains held for gameplay testing.

The user reports that the B/Start instruction before the title menu is silent.
They asked to check native text and baked imagery first, then authorized a short
mod instruction if no native button prompt exists. Their additional clue was
copyright text resembling “Project Final Fantasy X, 1999–2002.” The user also
confirms the final Jecht tutorial/eleven-answer auto-success works perfectly,
after previously confirming manual cues and taunt suppression. The Jecht test
condition for rebuilding the installer is fulfilled; finish this new repair,
build and guarded-deploy/verify its final payload before packaging the one EXE.

## Source investigation and limits

No game was launched/controlled, no live probe was run and Tester Logs were not
accessed. Existing decompile archives and extracted game scripts were used first.
The user's image-check request also authorizes the narrow read-only extraction
of candidate title/logo/menu assets from the installed `data/FFX_Data.vbf`.
The VBF reader and self-describing Phyre DDS decoder retain their existing
Topher/VBFTool and ffgriever format attribution. Game assets stay in ignored
local research output, not in the installer or ZIP.

Retained evidence:

- `Docs/Scripts/Offline/output/title_prompt_20261014/`: decoded logo effect662,
  its component letter/HD Remaster textures, copyright_nagi/nea, main menu
  background/strtex/menu_new/texture, source bytes and inspection PNGs.
  No button prompt was found in the inspected title/logo/copyright imagery.
- `install/reset-resume-20261013/title-prompt-archive-index.json` and
  `title-prompt-menu-index.json`: scoped archive identities. `assets.json`
  retains initial hashes; the other retained source bytes are available for
  verification. Two small movie candidates were unrelated and discarded as
  control sources. They do not establish the reported frame's exact identity.
- `title-copyright-text-search.json`: no Project/year/Press Start match in the
  retained original English field strings. This and the inspected assets are
  evidence for using the authorized mod instruction, not a claim to have
  reconstructed a native button prompt or identified every displayed frame.
- Original `test20` is room23's New Game/Load/Grid menu. `loopdemo` is the
  room349 rendered introduction; test20's idle branch warps to348. These have
  no matching native Press B field message. Old architecture notes calling the
  initial OpeningScreen SWF the actual title menu are superseded by its AS3:
  `scripts/OpeningScreen_fla/MainTimeline.as` displays the autosave notice at
  frame156 and invokes the native Finish callback at frame263.

The user's own current `FFX_SR_Native.log` was read narrowly, separately from
tester submissions. It shows startup resource loads on a loading thread and
later title output on the PInput owner. It contains no working mod-intro cue.
No new capture or runtime measurement was performed.

## Correct producer and ownership

The former intro module waited for FMV playback, then excluded any field chunk,
actor or native menu. This cannot supply the requested pre-movie instruction.
It is replaced, preserving one production instruction producer.

Original native bodies are retained in `native_bodies_20261014_083605` and
`native_bodies_20261014_090238`: boot loop641870 calls loader657B60 for
OpeningScreen.swf, then observes the active byte through660580 while performing
the native PInput/Iggy loop. Loader657B60 sets the flag after a successful load;
the original Finish callback clears it. Getter660580 is six bytes and returns
**AL**, so the captured result must ignore unrelated upper EAX bits.

Two capture-only callbacks retain only a fixed atomic episode/phase and the
original scalar active result. They perform no text formatting, native getter,
engine-pointer transfer, waits or SDK work on the Flash/loading owner. A valid
episode must observe active1 then active0; failed/never-active loads stay silent.
The existing PInput225930 owner then captures configured Cancel/Start labels
and publishes the existing fixed copied-text packet. The worker formats:

“Press [Cancel] or [Start] for the title screen, or wait for the intro cinematic.”

There is no separate timer, UI polling thread, input consumption or simulated
button. This is user-authorized mod guidance, not undisplayed game dialogue.
Native focus/scene/battle/input-generation guards apply. A postponed episode
is discarded on leaving startup/attract rooms0/348/349, including an already
open room23 title menu. Copied worker output retains normal generation/reset
cancellation. A fresh actual opening episode can repeat equal wording; no
text-content deduplication was introduced.

The773-byte boot body and116-byte loader qualify independently; the six-byte
getter is anchored to its actual boot call, not a generic getter search.
The active flag's instruction operand is also qualified. Generated evidence:
`native_extensions_20261014_160500_833503.json`. New dependencies are explicit
in `profile_dependencies.cpp`; no blanket address delta is inferred.

## Assistance defaults and upgrade preservation

Persisted auto-success settings are Overdrive, Lightning auto-dodge and Jecht.
Overdrive's default was1 and is now0; the other two already default0. All
Overdrive characters/reels share the same setting. Trial/Via solvers are
explicitly activated session actions and already start/reset unarmed; Auto walk
also already defaults0. Cues remain independent from automatic success.

Production `loadSettings` already gives valid saved INI values precedence over
defaults and does not write during loading. Installer payload/migration do not
replace `FFXAccessibility.ini`; no migration or preference reset was added.
Defaults apply to fresh/unset values. Existing explicit On and Off remain as
saved. The current user's INI contains all three values1; its pre-deployment
SHA256 and keys are in `title-settings-before-deployment.json`. Verify the exact
same file bytes after deployment and packaging; do not edit these values.

Focused evidence under `install/reset-resume-20261013`:

- `title-defaults-settings-2.log`: real production settings loader `/W4 /WX /O2`
  passes fresh-file Off and all8 saved On/Off combinations, preserving exact INI
  bytes, comments and unrelated control settings. Fixture1's compile-only wide
  to narrow warning was corrected with explicit ASCII-key conversion.
- `title-defaults-installer-tests-2.log`:805 synthetic assertions, including
  changed-payload A-to-B upgrade and byte-preserved mixed assistance values,
  actual synthetic embedded EXE installs twice at short/long fixture paths,
  runtime/receipt preservation, failure/refusal, accessible controls and
  in-memory registry tests. Test1 hit sandbox denial creating the test junction;
  the authorized workspace-only elevated rerun passes. No real registry repair
  or actual game was run. These fixture EXEs are not extra release installers.
- `title-prompt-profile-tests-1.log`: Reference/Installed/Will schema,
  instruction globals, anchored catalogs, ambiguity and transactional refusal.
- `title-prompt-readers-2.log`: both146-site reader profiles, real AL-only return,
  failed/active/completed opening, configured keyboard/controller controls,
  source reuse, native-owner separation, stale input, background/overlay,
  source replacement, equal distinct events and final gameplay/title cancellation.

Actual startup audibility remains user-side. No audible-latency or real-backend
claim follows from these controlled fixtures. Sol handled discovery, design,
implementation, review and verification directly; no delegation was used.

## Current delivery checkpoint

Completed in the required order: warning-free canonical x86 native/launcher
`title-defaults-build-1.log`, then closed-process guarded deployment
`title-defaults-guarded-deploy-1.log`. Actual FFX/Will preflight and installed
profiles, all seven file hashes and PRISM0.18.3 x86 receipt/15 locked hashes
pass. SDL3/license is installed; Tolk is absent. Final proxy SHA256:
`050cfe8556fa18e5ba92ab2e160daddce320c0d70fcb5d95496c1467f4a234d2`.
The installed verification is timestamped `2026-10-14T16:12:58.1287560Z`.

The user's INI remains byte-identical after deployment and packaging, SHA256
`81750fa26901d3ba6199f4b0d87e620243fb9f0f19dc5cd511b277ccafd08528`.
Existing Overdrive, Lightning and Jecht assistance all remain On; fresh/unset
settings default Off. No user preference was edited.

Only after that verification, `title-defaults-release-package-1.log` rebuilt
the single0.5.6 installer and ZIP. The actual final EXE contains26 verified
resources: seven native payloads,15 PRISM/runtime/license files, lock/checksum
anchors and two pinned stock inventories. Actual-resource dependency tests
pass130 assertions; actual final payload installs/reinstalls pass at short and
long synthetic paths, preserving mixed saved assistance choices. These checks
supplement the805 source assertions above; no native DLL or game was executed.
Logs: `title-defaults-actual-installer-dependencies-1.log` and
`title-defaults-actual-installer-transactions-1.log`.

The ZIP has exactly five distribution files,2,508,153 bytes; source documents,
package bytes, checksums and archive integrity match. Installer SHA256:
`38e99cea3c3192a5f2df11def39d476d16630f4b83d82fa5b3652485bd8340ff`.
ZIP SHA256:
`b1903474582225014b149a0e561ecb2890be8fcd85d46f807ef6803ddcf9c9ef`.
See `title-defaults-release-verification.json`. Previous artifacts and the old
default-stage EXE are preserved under
`install/release-backups/0.5.6--20261014-091606-bf6624e9`; the canonical default
stage now copies the same validated final EXE, without a second compilation.
The premature do-not-use marker was removed only after final checks passed.

No engineering, deployment or packaging blocker remains in this scope. The new
opening instruction's actual audibility and other unconfirmed gameplay remain
user-side in [the acceptance procedure](gameplay_acceptance_0.5.6.md); Jecht's
final auto-success/manual cues/taunt suppression are user-confirmed. No runtime
response measurement, real backend or registry repair, Tester Logs access,
publication or prior-build restoration occurred. Reset monitor stays paused.
Latest available core88% used/12% remaining, secondary unavailable; retain the
2% remaining stop threshold for future authorized work.
