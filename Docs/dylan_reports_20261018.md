October19 current correction: airship graphics permission granted; focused-image
labels and no-calibration Wakka guidance are implemented, offline verified and
guarded-deployed. The dated October18 partial checkpoint below is historical.
[Current evidence and delivery](features/puzzles/airship_wakka_20261019.md).

# Dylan conversation follow-up, October18

User authorized opening DylanTheVisionless's Discord DM, reading the last two
days, fixing reported bugs and downloading yesterday's airship save. Read via
Discord's Windows accessibility tree. Relevant October16–18 reports are below;
unrelated conversation was not copied into project records. No messages sent.
Tester Logs remains outside this authorization and was not accessed.

## Reports and accepted behavior

All message references belong to DM1469013075771392120 (local displayed dates).

- October17 15:36, message1561145927345897513: Cid's menu and password entry
  speak, but the main airship destination list does not speak destination names.
  Search-map interaction is usable. October18 05:48,
  message1561360222474338487: "New location" is spoken for the story objective,
  but its location text is not. This is the progression blocker.
- October17 11:48, message1561088547753300110: controller Grid next-node commands
  repeat the already-read node on subsequent presses, while keyboard commands
  can preview onward through the route without moving the actual cursor.
- October17 09:07, message1561048005669425213: after the boss at Gagazet Summit
  Region, Yuna's dropped sphere is missing from navigation. It must be interacted
  with before the onward exit works. Tester eventually found it manually.
- October17 15:32, message1561144967764770889: in Dome — The Beyond after the
  three-phase boss, the staircase event that produces a chest is missing from
  navigation. Earlier description: October16 11:41,
  message1560724374481543343. Label the event without leaking chest contents;
  preserve native activation and collection conditions.
- October16 16:31, message1560797364627701893: manual Wakka ball cue is too
  brief/late to account for reaction and throw travel. Counting intervals can
  eventually succeed. Investigate the original timing rather than guessing a
  lead. Automatic solving and swimming colour selection worked.

The same October16 exchange explicitly accepts results items/equipment in manual
details/Autodetail: message1560804365671993435. Do not undo that behavior.
Controller passthrough delay is confirmed gone. October17 12:25,
message1561097908072218695 confirms the final Cloister solver, so all six are
confirmed by this tester. Earlier cave/event-category/return-route, Bevelle
pedestal/treasure and Macalania branch discussions refer to already repaired
behavior; a new failure report takes precedence if encountered.

## Downloaded save

October17 15:38 message1561146424547082240, attachment1561146424174059720:
`Saves/Dylan/Airship_20261017/ffx_071` relative to the parent FFX workspace.
26,880 bytes, SHA256
`62a5d7d4d6457f9dbb84479c03c723f45554d40fed98f758c2be65c8d5c111fd`.
Original downloaded bytes retained; no existing save overwritten or game loaded.
The initial Discord download opened a browser Save dialog; browser automation
approval timed out. The same observed attachment URL was downloaded directly
into the owned project folder. No executable was acquired or run.

## Scope and verification

Sol owns source diagnosis/integration. Use retained native/script archives and
this explicitly requested save offline. No game launch/control/runtime probes,
new installed-source asset reads, real registry changes or publication are
authorized. Build and guarded-deploy necessary repairs; keep0.5.7 release
artifacts unchanged unless separately asked to rebuild a release.

## Implementation and evidence checkpoint

Sphere Grid controller commands previously captured immediately in PInput::Update
entry; keyboard commands drain at its return. Grid controller commands now use
that same bounded64-command native return FIFO, with epoch/revision/focus and
Grid-only guards. There is no worker wait or analog passthrough change. LT/RT
retain cursor-based Backslash/Shift+Backslash; View+faces retain F2/F3/F4/F5,
including F4/F5's intentional pawn origin. The tester did not specify which
route; user requested both diagnosed. Native input/controller fixtures verify
fresh state after entry, exact bindings/modifiers, one press, held consumption,
both native decoders, closure without field fallthrough and full-queue refusal.
Sphere semantic fixtures advance the preview cursor through five nodes in both
directions while checking the pawn stays fixed. This corrects a phase discrepancy;
the remote symptom is not claimed independently reproduced in gameplay.

`navigation_passages_data.hpp` has exact-length/checksum-qualified overlays for
mtgz0800 W9 (Yuna's sphere, model0x5045, entry2) and dome0600 W25/W26 (distinct
Staircase 1/2 events, entry2). Original init/reveal/hide and interaction gates
remain authoritative. Original sphere entries6/8 reveal it and entry7 hides it;
the Dome crossing consumes native saved1D6 bit1 and requests W15:7 once. No
story, sphere, chest or exit script is changed. Catalog fixtures preserve the
original exits/type-only chest and reject altered code; native worker fixtures
check discovery and disable/re-enable. Fixture poses are controlled boundaries.
Original flows/hashes: `Scripts/Offline/output/dylan_reports_20261018_011846`.

Manual Wakka cues no longer beep at impact alignment. With auto-success Off,
one ordinary manual throw measures native ring-clock elapsed time through the
original hit predicate. Subsequent cues give two lower countdown tones and a
repeating high launch tone, with the measured complete throw delay projected
onto both original ring schedules. All three native speeds are qualified by
worker/entry/wait-PC/timer duration. The original mtgz0700 MGRP already retained
in the workspace contains36/19-frame animations332005/332006; no new installed
Wakka reads were needed. Nominal animation duration55 is evidence only; production
does not guess it as the delay. No game flags/input are changed for manual cues.
Native ring observations and constant-bounded interval comparisons allocate no
owned data or wait for work. Separate audio cancellation and a100ms obsolete-queue
cutoff prevent old launch tones after a throw/exit. Audible device/reaction timing
is still a gameplay check, not proven by asynchronous placement.
See `Scripts/Offline/output/gagazet_timing_20261018/proof.json` and the original
30-stage production timing/hit-operand fixture in both utility build flavors.

Airship labels remain under investigation. Original W9 applies destination IDs
to image layers11..25, with the selected row at map15100, last index15124,
ID array1512C and scroll151C0. TK draws the prompt and New location separately;
the existing coordinate/free-search reader cannot read these images. Installed
airship graphics verification awaits the task-specific permission prompt. No
airship fix is claimed. The other three repair areas were guarded-deployed;
the receipt below records their separate verification boundaries.

Luna (`gpt-6-luna`, medium, fresh bounded handoffs) added known catalog records,
native event fixtures and controller routing/semantic fixture extensions. Sol
reviewed production records, preservation/gate assertions, corrected a fixture
integer-width warning and a shadowed local name, and owns integration/build/checks.
No delegated functional issue remains; actual gameplay/hardware timing and the
separate airship blocker remain unverified. No Astra was used.

Two bounded Luna comment tasks cover six integration modules and ten smaller
adapter/reset/guide modules. Sol read back the contracts, removed duplicate
headers, corrected a missing topic anchor and narrowed dependency wording to the
actually listed RVAs. Sol also reviewed and added purpose/non-obvious contracts
in eighteen additional modules. These are comment-only edits; they do not expand
gameplay behavior. The source/settings inventory is a coverage/change aid, not
a comment-count quality claim.

## Verification and delivery

Passed offline: native-return input queue, controller routing/consumption, Sphere
cursor/pawn semantics, both native reader profiles, both utility build flavors,
SDL owner and production SDL dummy-audio callback/queue-age fixtures. The new
native ring wait signature is unique in the supported retained binary; the timing
audit qualifies all 30 original ring stages and retained MGRP animations. Original
ring/hit operands are executed by the production utility fixture with controlled
timers. No native-game entry point was run.

The controlled SDL owner case measured 96 source-to-native samples: p50=2.134ms,
p95=2.595ms, maximum 2.649ms; native pump/read maximum 16.5 microseconds. These
fake-SDK/component measurements do not prove real DualSense/game or audible
response. The controller repair fixes a proven entry/return phase mismatch;
the tester's stale-node symptom has not been independently reproduced live.
Manual Wakka device/reaction/turbo integration remains a user-side check.

Canonical `src/proxy/build_dinput8.bat --release --no-deploy` passed without
warnings, followed by `src/proxy/deploy.bat` with fresh closed-process checks.
Compiled-profile/signature preflight, all seven installed payload hashes, locked
PRISM runtime/bootstrap and unchanged INI pass. No game/launcher was launched,
and no running modules were replaced. [Deployment receipt](../../Analysis/FFX/RepositoryEvidence/historical-install/dylan-followup-20261018/deployment-verification.json)
and neighboring logs retain exact hashes/commands. Final build/deployment after
comment catch-up passed; installed proxy SHA256 is
`dc5cc85adb386d4957fee7d7795ad5f22e28e505b71ae23f2ad221284704a36b`.
Both reader profiles passed again after the fixture cleanup. An existing fixture
C4389 at `airship_runtime_followups.inc:43` remains; the production build has no
warnings. Two comment-anchor spelling corrections during compilation changed
no noncomment tokens or line counts; the receipt records that provenance limit.

The existing 0.5.7 installer/ZIP is unchanged and does not include these new fixes.
Original downloaded save bytes remain unmodified. [Complete feature index](features/index.md),
[source map](features/source-map.md), [settings](features/runtime/settings.md) and
[prioritized gameplay checks](features/verification.md) now link current
implementation/ownership/controls and the retained detailed native evidence.
The audit maps 140 production modules and 31 settings and validates current local
links. Main airship destination image labels remain the sole unfixed reported
feature, awaiting the already requested task-specific offline installed-asset
permission. This is a partial repair checkpoint, not completion of all reports.
