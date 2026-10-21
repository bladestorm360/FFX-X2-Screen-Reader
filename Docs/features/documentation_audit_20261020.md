# Feature documentation audit — October 20, 2026

This focused review checks the assigned menus, battle and distribution pages
against current production sources and existing linked evidence. It is not a
claim that every state has fresh gameplay confirmation. Implementation status,
offline evidence and gameplay evidence remain separate in the [feature
register](status.md). The [source map](source-map.md) is the current143-unit
inventory; this table names the files directly reviewed for the three assigned
pages.

## Current0.6 minigame completion review

The existing explanatory inventory is reused for unchanged features. Sol reviewed
new `minigame_completion.cpp/.hpp`, reader installation/reset, settings/revision
semantics, contextual menu consumers and tests against the actual code and
retained originals. The [complete activity topic](puzzles/minigame_completion_20261020.md)
now explains all four activities, accepted controls/defaults, original mechanisms,
PC/declaration/ABI evidence, ordered processing, native pointer ownership,
capacity/refusal/cancellation, recovery, extension and exact verification limits.
The source map adds the new cohesive module; generated authority now covers143
native units and37 settings. Player controls, architecture, status, debug record,
README/changelog, prioritized gameplay checks and both memories are current.

All nine original resource variants,64 Remiem flag combinations, score/period/
choice/story boundaries, context/refusal/one-shot cases and original typed
setter/native queue/entry replay pass. Menu/default persistence and existing
minigame/butterfly/readers/profile/ABI/native actions/startup/utilities pass.
Both171-hook reader profiles pass. The existing airship test include retains one
C4389 signed/unsigned comparison warning; production and new component builds
have none. The release-flavor console-excluded build is guarded-deployed; all7
installed files and329 unchanged production input hashes match. Seven protected
INI/previous-package hashes match; no save writes or installer rebuilding.
[Exact receipt](../../install/minigame-verification-20261020/verification.json).
Real VM/story/animation/prize presentation and player-to-usable/audible latency
remain gameplay checks, not inferred from controlled entry-initialization timing.

| Model and bounded assignment | Outcome and Sol review/validation | Remaining issues |
|---|---|---|
| Luna, `gpt-6-luna`, medium: append four known-pattern settings/menu rows and expand default/persistence host | Completed in allowed files. Sol reviewed keys/defaults/contexts, added setting revisions and pending cancellation integration; defaults pass1024 combinations and absent keys. | No outstanding delegated implementation issue; existing English-only mod UI gap remains. |
| Luna, same model/effort: individual contextual menu fixture cases | Completed in allowed fixture. Sol corrected indexed-count assertions to enable Menu indexing, reviewed coverage and ran the passing actual menu host. | No outstanding delegated fixture issue. |
| Luna, same model/effort: supplied-fact controls, puzzle index, README and changelog | Completed four allowed documents. Sol corrected Remiem tier wording, console flavor and pending verification/deployment claims after final checks, reviewed links and updated authoritative mechanisms/status/memory. | Gameplay confirmation and installer/package hold remain explicit. |

Sol retained behavior discovery, native design/implementation, integration,
verification and guarded deployment. No Astra was used. No game launch/control/
probe, new asset investigation, Tester Logs, registry repair or publication.
The repository content audit has zero current/local-history pattern findings;
the known retained GitHub object remains a separate host-side cleanup issue.
Historical release/deferred statements below describe their earlier checkpoints.

## Coverage reviewed

| Topic | Production source reviewed | Current explanatory coverage and evidence |
|---|---|---|
| Field menu/detail | `src/native/field_menus.cpp`, `field_details.cpp`, `field_targets.cpp`, `field_controls.cpp`, `aeon_attributes.cpp`, `game_details.cpp`, `selection_details.cpp` | `menus/index.md` describes selection capture, native getter/text resolution, `TextPlan`, worker rendering, Autodetail/manual I, ownership invalidation, and field targeting/customization evidence. `installFieldMenus()` installs the subordinate field readers. Field detail and targeting stay qualified by scene/selected handles rather than remembered row indexes. |
| System screens | `src/native/system_menus.cpp`, `system_menu_capture.cpp`, `status_review.cpp`, `status_grid.cpp`, `save_load_menu.cpp`, `shops.cpp`, `confirmations.cpp`, `naming.cpp` | The menu page covers native slot/list/focus capture, empty states, confirmation choice changes, saved-item visibility after sales and contextual naming. Exact supported input controls/defaults are centralized in `Docs/controls.md` and `Docs/features/runtime/settings.md`. Existing menu, native row/buffer and Password records are linked from the page. |
| Conditional menu | `src/native/monster_arena.cpp`, `help.cpp` | The menu page records Monster Arena scene/menu/room qualification, draw-depth/epoch lifetime and copied-text handoff. Help's prompt, content and footer row roles, native draw phases, owned buffer preparation, navigation exceptions and context serial guards are explained with the retained Help identity/line/replay records. |
| Dialogue, title, Escape | `src/native/messages.cpp`, `message_text.cpp`, `copied_controls.cpp`, `dialogue_voice.cpp`, `movie_dialogue.cpp`, `blitz_review.cpp`, `esc_capture.cpp`, `esc_settings.cpp`, `esc_details.cpp`, `intro_cue.cpp` | The menu page explains native page/focus and token decoding, voice-output policy, caption ownership, review scope and native option/value changes. Speech defaults On; voiced-dialogue mode defaults Suppress, with the mode choices and repeat behavior documented in source settings and linked records. No text-content deduplication is used. |
| Sphere Grid | `src/native/sphere_grid.cpp`, `sphere_data.cpp`, `sphere_owned.cpp`, `sphere_paths.cpp`, `sphere_sections.cpp`, `sphere_expert.cpp`, `sphere_orientation.cpp`, `sphere_planner.cpp`, `sphere_jump.cpp`, `sphere_text.cpp` | The page documents native selection and owned bounded snapshot flow, separate Standard/Expert layouts, route rules, quarter-level costs, character/cursor distinctions, keyboard/controller mapping and F6 UI-owner handoff. Autodetail defaults Off. Linked Grid, controller, snapshot and dialog evidence covers offline behavior; it does not measure end-to-end audible latency. |
| Battle action/details | `src/native/battle.cpp`, `battle_announcements.cpp`, `battle_graphics.cpp`, `battle_status.cpp`, `battle_sensor.cpp`, `status_animation.cpp` | The battle page describes the single installer/producer path, native action/target/command ownership, copied operands, graphical-result qualification, Sensor/status readers, callback lifetime and per-target buff events. Stone Gaze command-title implementation is in the linked issue repair record; status register correctly leaves gameplay confirmation pending. |
| HUD/results/Overdrives | `src/native/battle_hud.cpp`, `battle_results.cpp`, `overdrive.cpp`, `wakka.cpp`, `wakka_identity.cpp` | The page records accepted/clipped visible-row filtering and caps, results baseline/completion phases, manual versus automatic cue defaults, real effect-image identity qualification and native-choice behavior. HUD controls and display defaults are linked to controls/settings and fixture/evidence records. |
| Developer utilities | `src/native/utilities.cpp`, `utility_console.cpp`, `utility_feedback.cpp`, `utility_warps.cpp`, `utility_context.cpp`, `utility_scripts.cpp`, `utility_trials.cpp`, `utility_memory.cpp`, `utility_text.cpp`, `utility_text_plans.cpp` | `distribution/index.md` documents release exclusion, separate Windows UI owner, native action revalidation, progression-preserving ordinary warps and bounded explicit trial setups. The utility capture queues have eight entries each; work is held until field preparation and revalidated against context/generation before execution. Detailed allowed writes/refusals are in linked trial records. |
| Build/deployment/installer | `src/proxy/build_dinput8.bat`, `src/proxy/deploy.bat`, `src/launcher/`, `src/installer/` (read-only page and project-manifest review) | The distribution page documents x86 build flavors and deployment side effects, seven-file payload verification, launcher/PRISM ownership, installer payload/dependency behavior and explicit registry UI boundaries. Current receipts are the October19 0.5.8 package and October20 installed Baaj repair. The 0.5.8 package was not rebuilt for October20 changes and remains held. |

## Findings and limits

- All reviewed topics have an explanatory entry point and useful links, but the
  initial design register is retrospective; its own status page says so. This
  audit updates factual implementation explanation and does not claim that
  features were designed before their original implementation.
- Controls, settings persistence/defaults and language restrictions are
  centralized references. The focused pages now explicitly point to them and
  state the core Speech/Autodetail defaults where readers otherwise could infer
  a per-feature switch. All mod-authored labels remain English; native game text
  follows the game getters. Full mod UI localization/manual override and HRTF
  are existing implementation gaps, not covered by these three pages.
- Offline fixtures and prior receipts establish specific code paths and replay
  cases. They do not establish current gameplay/device behavior. Current
  register gaps remain: Stone Gaze gameplay, Free Search audio's exact audible
  producer, airship Proceed prompt identity, new F-key behavior, and any cases
  marked pending in subsystem records. No runtime actions were performed for
  this documentation review.
- A source symbol or translation-unit inventory alone does not prove
  comprehensiveness. The descriptions above were checked against the named
  registration/producer/lifetime paths and linked records; unresolved native
  reverse-engineering questions remain with Sol. No source code or snippets are
  included here.

## Exact reviewed paths

Production files reviewed are the `src/native/*.cpp` names in the Coverage
table, plus the build/deploy batches and the launcher/installer entry paths named
there. Documentation reviewed: `Docs/features/index.md`, `status.md`,
`source-map.md`, `menus/index.md`, `battle/index.md`, `distribution/index.md`,
`runtime/settings.md`, `controls.md`, and the existing linked menu, battle,
installer and utility evidence referenced from those three subsystem pages.
The review did not inspect Tester Logs, the game installation, or files outside
the repository for this audit.

## Historical Markdown snippet cleanup

A later source-safe documentation pass inspected human-authored Markdown under
`Docs` (excluding `Docs/Scripts` and build/vendor/Git directories), plus root
`README.md`, `CODEX_MEMORY.md`, `CHANGELOG.md` and `AGENTS.md`. It removed 11
fenced implementation/pseudocode fragments from these three files:

| Sanitized path | Fragments removed | Replacement |
|---|---:|---|
| `Docs/gameArchitecture.md` | 7 | Prose describing the directory split, recovered input/update flow with native locators, class relationships, localized menu-value flow and script transition semantics. |
| `Docs/session1_50.md` | 3 | Historical architecture summarized in prose; two probe-output blocks retained as factual values in prose and labelled historical. |
| `Docs/session51_current.md` | 1 | Disassembly excerpt summarized as historical decoded behavior and offsets. |

The same pass removed inline implementation expressions, signatures, formulas
and pseudocode from these files. A conservative scan for operator-bearing inline
code spans (`=`, `;`, braces, arrows, pointer marks and increment operators)
showed this net change:

| Sanitized path | Net fewer matched inline spans |
|---|---:|
| `Docs/gameArchitecture.md` | 30 |
| `Docs/session1_50.md` | 23 |
| `Docs/session51_current.md` | 17 |
| `Docs/butterfly_paths_20261008.md` | 2 |
| `Docs/player_followups_20261007.md` | 3 |

The net inline count is an inventory measure, not a claim that every pattern
match was implementation code: the scan also matches factual captured output,
configuration, file patterns and identifiers that remain intentionally inline.
All edited formulas/signatures were restated in prose; native symbols, offsets,
tables, output and operational commands remain where they carry evidence.

Original whole files were hash-checked before editing and stored outside Git
under `D:/Games/Dev/Custom/FFX/Analysis/FFX/RepositoryCleanup_20261020/doc-originals/`
at their repository-relative paths. The audit itself was snapshotted in the
`second-pass` subfolder before this section was added. The `Docs/cloisters.md`
fenced PowerShell block was retained because it contains only short operational
test commands. Factual tables, native function/RVA locators, captured values,
configuration and output were retained. This pass found no other multiline
fenced blocks in the permitted scan set; it did not establish that every short
inline code span is non-implementation, and no code or runtime file was changed.

## Historical evidence link migration

A separate link migration updated **68 Markdown file hyperlinks** in 23 permitted documents after resolving each destination to an existing retained file. Targets moved from `install/` now point to `Analysis/FFX/RepositoryEvidence/historical-install/`; moved offline evidence links point to `offline-output/`. Each edited document's complete pre-edit bytes were hash-verified in an external backup under `D:/Games/Dev/Custom/FFX/Analysis/FFX/RepositoryCleanup_20261020/doc-originals/link-migration/`. The retained manifest linked from [the airship follow-up](puzzles/airship_wakka_20261019.md) explains the former offline-output location and points to its external archive.

| Edited document | Links updated |
|---|---:|
| `Docs/features/puzzles/airship_password_20261019.md` | 2 |
| `Docs/features/puzzles/airship_wakka_20261019.md` | 1 |
| `Docs/features/verification.md` | 1 |
| `Docs/active_puzzles_battle_controls_20261016.md` | 2 |
| `Docs/battle_panels_controls_20261016.md` | 2 |
| `Docs/bevelle_treasures_fayth_gates_20261016.md` | 6 |
| `Docs/debug.md` | 2 |
| `Docs/dylan_reports_20261018.md` | 1 |
| `Docs/help_lines_kilika_20261016.md` | 5 |
| `Docs/help_navigation_quit_20261015.md` | 1 |
| `Docs/jecht_shot_20261014.md` | 11 |
| `Docs/menu_followups_20261014.md` | 2 |
| `Docs/navigation_asset_audit_20261007.md` | 3 |
| `Docs/opening_escape_help_20261014.md` | 1 |
| `Docs/plan.md` | 1 |
| `Docs/prism_migration_20261012.md` | 2 |
| `Docs/release.md` | 3 |
| `Docs/tester_report_20261015.md` | 3 |
| `Docs/threading_audit_20261012.md` | 4 |
| `Docs/trial_entry_followups_20261016.md` | 9 |
| `Docs/trial_intercepts_20261016.md` | 4 |
| `Docs/via_swimming_blocker_20261012.md` | 1 |
| `Docs/zanarkand_patterns_20261015.md` | 1 |

The exact-target pass left unknown and non-link historical locators intact. It did not rewrite build commands, links to retained current install folders/releases, or prose/source locators. The migration covered Markdown hyperlink destinations matching the moved roots; it was not a comprehensive check of every link in the repository, all historical file references, test status, or release readiness. Gameplay status and packaging hold remain as previously documented.
## Final source agreement and migration review

The subsequent Cid repair changes naming/category priority, actual actor presence,
Talk-only availability and selection identity across worker/actor transfer. Both
171-hook readers pass four original maps and the actual selection/transfer case.
Baaj capture adds two qualified read-only getters and three owned diagnostic
scalars; explicit commands alone log the bounded state. Relevant source, ownership,
bounds and unresolved gameplay are in the linked Cid and Baaj topics. The final
warning-free developer build is guarded-deployed with all seven installed hashes
matching [current delivery](../../install/dev-console-20261020/cid-baaj-verification.json).
This supersedes delivery status below, while earlier receipts remain historical.

Sol compared the retained prior documentation source hashes with current
production:138 of141 translation units are unchanged. The changed units are
`input.cpp`, `utilities.cpp` and `nav_interactions.cpp`; their current ownership,
controls, bounds and native evidence are documented in the console and Baaj
follow-ups. New shared guard facts and owned proof are documented beside Baaj.
The six subsystem topics and their linked feature records cover all implemented
features; source and setting inventories supplement that explanatory review.
Unimplemented baseline features, the unknown Free Search audible producer,
actual Proceed prompt and gameplay limits remain explicit in the status register.

The exact Baaj Macalania no-activation epilogue is qualified by event name,
length7530,FNV-32d13cfb8c,worker34,entry2,PC7072 and call93. Native refresh also
requires prepared allocation/header identity. Both171-hook reader profiles pass
all64 treasure combinations/native-versus-owned guards,540 native endpoints each
and original airship/Help/Grid/Customize replays. Direct comparison subsequently
confirmed that installed Baaj slot0 exactly matches the named file. Earlier
different-save wording was wrong. The user subsequently confirms Besaid stays
inactive because its treasure is absent and all eligible seals work, accepting
the fixes and closing the report. Final all-six door opening remains untested.
Bounded native flags/counter diagnostics are independently documented.

All32 external replay declarations now name owning byte storage before binding
C-array references. The initial temporary-subobject adapter failed after static
initialization; the explicit storage and generator correction pass both readers,
utilities, native worker getter, reachability and pedestal hosts. The standalone
adapter checks retained content, type/size/fingerprint and rejected path/size/hash
cases. All15 guarded Java exporters compile against retained JDK21/Ghidra12 jars;
Sol repaired three stale output-path print variables found by actual compilation.
No Ghidra script, Frida probe or game process was executed.

Original archives, inputs, copied historical fragments and the pre-rewrite Git
archive are outside every Git tree. The source-content audit scans working
content and all local Git blobs; the externally retained provenance review and
move/array manifests supplement pattern matching. All seven commits and the
explicitly approved master/two tags were sanitized and remote refs verified.
Current mod changes remain unpublished. GitHub still retains a known old
decompiled-body blob; host-side deletion is the remaining cleanup blocker. See the [content boundary](repository_content_20261020.md).

The warning-free developer build is guarded-deployed with all7 matching hashes;
INI, named Summon save and0.5.7/0.5.8 ZIPs are unchanged. Current receipt:
[0.5.9 release delivery](../../install/release-0.5.9-20261020/release-verification.json).
The user explicitly resumed packaging after confirming Sea of Sorrow and leaving
later Sin to a tester. New minigame solvers are deferred. Offline fixtures measure controlled
components, not actual player-to-audible/gameplay latency. The large cold
reachability run's complete-set cost is retained in the content record rather
than being presented as a universal response guarantee.

After the approved City/Nucleus follow-up, the feature audit passes142 production
units,32 settings and26 current topic documents with no broken local/source
links. The content audit scans2174 working files and83 local Git blobs with zero
pattern findings. These checks complement the provenance review; they do not
resolve the separately recorded retained GitHub object. The two new offline
Python adapters parse, both retained assets match their approved hashes, and
the installed archive's size/mtime are unchanged. Current receipt above retains
the exact build, reader, route, deployment and audit log hashes.

## Delegation outcome

| Model and bounded assignment | Outcome and primary review | Remaining limits |
|---|---|---|
| Luna, `gpt-6-luna`, medium: menus/battle/distribution explanation, Markdown snippet cleanup, evidence links and content-record refresh | Completed within allowed documentation files. Sol checked changed facts against actual source/records and corrected one relative controls link and the resource checksum width; current feature-link audit is required after these corrections. | Gameplay, HRTF/localization gaps and deferred prompt investigation remain documented; no unresolved delegated edit. |
| Luna, `gpt-6-luna`, medium: known replay-array/path transformations and exporter guards | Completed mechanical scope. Sol reviewed/integrated, corrected replay storage ownership, three Java print variables and portable paths, then ran native replay/utility/worker/reachability/pedestal and Java compilation checks. | Exporter runtime/live capture untested and not authorized; no unresolved migration defect after covered checks. |
| Luna, `gpt-6-luna`, medium: supplied-fact Cid topic and navigation index | Completed bounded documentation. Sol corrected worker-versus-character terminology, the precise stale worker 3/Cid worker 8 defect, story threshold wording and current verification status against implementation and both reader profiles. | Gameplay and localized Cid fallback remain unconfirmed; no remaining delegated edit. |
| Luna, `gpt-6-luna`, medium: remaining legacy exporter-path guard | Wrapped the parenthesized Dylan report output path with the established external evidence guard. Sol reviewed the exact destination before mkdir; all79 Offline Python files parsed and bounded legacy-writer review found no other unguarded output path. | Compile/static review only; no exporter or runtime probe executed. |

| Luna, `gpt-6-luna`, medium: supplied-fact Sea of Sorrow topic and navigation index | Two bounded documentation passes completed. Sol reviewed implementation/native facts, test results, ownership and timing limits, corrected remaining stale deployment/verification wording, and retained final delivery responsibility. | Corrected gameplay pending; later Sin barriers/Nucleus autosolve separate in-progress scope. |

| Luna, `gpt-6-luna`, medium: supplied-pattern Nucleus F8 submenu and controlled menu tests | Completed in mod_menu.cpp/.hpp and accessibility_fixture.cpp. Sol reviewed context/key/default/session ownership, ran the clean canonical accessibility host and integrated the original-native collection module separately. | No remaining menu defect; native behavior/ABI, build/deploy and gameplay limits remain primary-owned. |
| Luna, `gpt-6-luna`, medium: supplied-fact collection guide, README controls and user checks | Completed three allowed documents. Sol reviewed exact current metrics/limits and corrected the control label to Collect objects automatically and the count action to Collection status; current source/doc audit covers142 units and32 settings. | Sol verified build/seven installed hashes/preserved files. Subsequent approved City floor replay is complete; gameplay remains pending. No Astra used. |
| Luna, `gpt-6-luna`, medium: supplied-fact approved City/Nucleus map results and platform documentation | Completed four allowed documents. Sol checked map hashes, route/native counts, boarding semantics, code/data flow and test limits; corrected stale pending status, a stale floor claim, neutral stop labels and documentation formatting. Both reader profiles, actual floor host, warning-free build and seven installed hashes pass. | No remaining delegated edit. Full ride/spawn VM, gameplay, frame/audible response remain untested; original data stays outside Git. |
| Luna, `gpt-6-luna`, medium: initial0.5.9 player-document update | Interrupted at the user's hold after partial README/changelog edits. No result was accepted as final; Sol preserved the work and recorded the interruption. | Superseded by the fresh resumed task below; no release was built during the hold. |
| Luna, `gpt-6-luna`, medium: resumed0.5.9 README/changelog/controls from supplied facts | Completed all three allowed documents. Sol reviewed controls, feature/reward scope, current versus historical version, confirmed versus pending gameplay and packaged document bytes. Sol removed a misplaced heading and trailing whitespace. | No delegated edit remains. Sol verified warning-free build, startup/release gates, seven installed payloads,805/130 installer assertions and actual short/long transactions; later gameplay gaps remain explicit. |

Sol retained all discovery, geometry, callback correctness, implementation design,
integration, history operation, verification and deployment. No Astra delegation
was used. Full pre-edit documents and source-bearing evidence remain in the
external cleanup archive; no removed fragments are copied into this audit.

The [remote-object handoff](remote_cleanup_handoff_20261020.md) records the
confirmed server object, clean refs/private/no-fork/no-PR facts and unsent
support request. Overall source cleanup remains in progress at~95%; current
local content, local objects and remote reachable history are separately complete.
