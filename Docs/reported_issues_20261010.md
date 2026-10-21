# Deferred reported issues — 2026-10-10

October12 source maintenance implements Airship internal exits/elevators and menu speech, Polish text detection/override, Grid Status detail/review, displayed Overkill, current Overdrive help, the linked Salvage Ship underwater exit, Kilika treasure continuation, and Macalania goal choice. Via Purifico adds both progression exits plus a manual panel guide and optional native-script collect-all action. Offline validation/build/deployment status: [repair record](open_issue_repairs_20261012.md). Gameplay confirmation remains user-side; the October9 release ZIP is unchanged.


Source: user reports in the FFX project chat on 2026-10-10, including Dylan's tester report relayed by the user. These issues are recorded for later work; implementation and validation are pending.

## 1. Polish diacritics with the tester's translation mod

- Report: the Polish diacritics issue matches the issue encountered in our FFXII mod. The user says we already have the Polish mod the tester uses and should be able to apply the same approach.
- Required behavior: correctly decode and vocalize Polish diacritics when that translation mod is in use. Reuse the verified FFXII solution after confirming how it applies to FFX.
- Prefer automatic detection of the translation mod. Also provide a persistent, explicit manual override in the grouped mod settings menu that forces the Polish decoding mode when automatic detection fails. The override must take precedence over automatic detection.
- Later investigation: locate the already-held translation mod through authorized project references; review the FFXII fix and verify the FFX text/glyph mapping and a reliable detection signal offline. Do not assume the same implementation applies without confirmation.
- Acceptance: Polish diacritics are correct with automatic detection; forcing the override works when detection is unavailable or fails; normal decoding still works when the translation mod is absent.
- Status: deferred; reported similarity, translation-mod availability and proposed reuse have not been independently verified in this documentation-only task.

## 2. Displayed enemy defeat text, including Overkill

- Report: an enemy defeated by excessive damage can produce displayed "Overkill" text, which needs to be spoken.
- Required behavior: vocalize the actual enemy defeat text shown on screen, including "Overkill" when rendered and "KO" if KO is rendered when an enemy is defeated. Preserve the game's displayed wording and language. If KO is not rendered as text, leave the mod's current KO vocalization unchanged (explicit user clarification, 2026-10-10).
- Base newly supported defeat-text speech on the actual rendered text and its display event. Do not infer or invent "Overkill" from damage amounts, enemy HP or a defeat flag. The absence of displayed KO text must not disable, remove or otherwise alter the existing KO announcement path.
- Later investigation: reuse existing offline renderer/text-getter analysis and trace the central defeat-text producer, rendering/reveal conditions and native event hook. Preserve existing project-specific event guards; no text-content deduplication is authorized.
- Existing evidence lead: [2026-10-06 battle outcome notes](debug.md#testing-corrections-model-proof-planner-origins-and-native-battle-outcomes---2026-10-06) identify MsNumberRegist kinds 3/4/5 as Miss/Guard/Overkill and the existing KO path as a committed MsDamageCheckDeath transition. Reuse that archive and verify the current display/producer path; historical coverage does not close this new report.
- Acceptance: rendered Overkill is read when it appears; rendered KO or other defeat text is read when it appears; when no KO text is displayed, current KO vocalization remains unchanged. New defeat-text support must not invent undisplayed labels or add a second announcement for the same defeat event. Verify normal and excessive-damage defeats and preserve nearby battle announcements through producer/event ownership, not text-content deduplication.
- Status: deferred; fresh renderer/hook confirmation, implementation, offline verification and gameplay confirmation are pending. The requested addition reads displayed game text; existing KO vocalization is explicitly retained when KO text is absent.

## 3. Macalania Cloister autosolver sends the player to the wrong room

- Source: Dylan, a tester; report relayed by the user on 2026-10-10.
- Report: when advancing the story sequence, the Macalania Cloister of Trials autosolver moves the player back to the trial entrance instead of the room containing the event trigger.
- Required behavior: move the player to the correct room and event trigger location so the intended story sequence can advance.
- Later investigation: confirm the relevant progression state, destination room, entrance/spawn and trigger location from the retained Cloister/ATEL analysis. The exact correct destination and cause have not been established by this report. Reuse [existing Cloister evidence](cloisters.md); prior implementation/offline checks do not close this new failure report.
- Acceptance: at the reported progression point, the autosolver reaches the correct event trigger and advances the story sequence instead of returning the player to the trial entrance. Confirm the destination offline and the resulting progression through user/tester gameplay testing.
- Status: open; reported by Dylan, not independently reproduced. Investigation, fix, offline verification and gameplay confirmation are pending. The tester's build and exact progression state were not supplied.

## 4. Airship menu option speech

- Source: user request on 2026-10-10 to check this menu later.
- Required check: make sure the displayed Airship menu options are vocalized on menu entry and as the selection changes.
- Later investigation: verify the native option text, displayed ordering and selection events using retained menu/text-getter analysis.
- Acceptance: each displayed option is read correctly when selected, including the initial selection.
- Status: deferred; menu speech has not been independently checked in this documentation-only task.

## 5. Overdrive mode select menu help text and I key

- Source: user report on 2026-10-10: options read, but help text does not.
- Required check: confirm exactly which help text is rendered for the selected Overdrive mode and make sure the I key reads that displayed help text.
- Later investigation: verify the renderer/reveal conditions and native help-text getter, then trace the central description action and this menu's context handling. Preserve the existing option speech and read the current selection's displayed help.
- Acceptance: I reads the help text actually rendered for the selected mode; changing the selection makes I read the newly displayed help. Option speech continues to work. Do not introduce speech for undisplayed game text without the required explicit approval.
- Status: deferred; the option/help behavior is user-reported. Rendered-text confirmation, investigation, any fix, offline verification and gameplay confirmation are pending.

## 6. Stone Gaze has no text on application or miss — 2026-10-12

- Source: user report in the FFX project chat on 2026-10-12.
- Report: Stone Gaze has no text when used, whether the debuff applies or the attack misses. Record both outcomes as a known issue; the report does not establish whether native text is absent or rendered text is missing from mod output.
- October13 status: the user explicitly authorized implementation. Shared action/message/status repairs are implemented and offline-verified on both140-site executable profiles; actual Stone Gaze and individual related ability gameplay remain unconfirmed. Ability names no longer depend on finding the optional actor action-state flag; direct native slot1 messages are read with one producer for nested banners. Accepted native Miss and Guard/Immune popups use their displayed locale labels. Consumed petrify packet data survives native KO/deactivation cleanup; skipped or unchanged packets do not invent an outcome. The final October14 native/launcher tester payload is guarded-deployed; all seven hashes and PRISM receipt/runtime are verified. Both144-profile reader checks preserve this repair; actual individual ability gameplay remains pending. See the [current repair checkpoint](threading_audit_20261012.md#stone-gaze-and-owned-battle-status--october-13-2026).
- Related-debuff check: reviewed the existing shared producers in `src/native/battle_status.cpp` and `src/native/battle.cpp`, plus [retained status-animation evidence](status_animation_timing_20261009.md#native-behavior-and-evidence) and [battle-outcome evidence](gameArchitecture.md#planner-origins-contextual-controllers-and-battle-phases---2026-10-06). The status producer announces copied, changed native status values, including petrify, poison, sleep, silence, darkness, Slow and the four Break statuses. Miss speech depends on an accepted native kind-3 MISS popup; an unchanged status or zero damage does not establish a miss.
- Scope of that check: these shared paths identify other debuff attacks to review, but do not confirm their individual action text, application timing, rendering or missed/resisted outcomes. Other petrifying attacks and attacks applying the listed debuffs remain unconfirmed, not additional established failures. Prior general status coverage does not close the Stone Gaze report.
- Later investigation, when requested: check Stone Gaze and related debuff attacks for the actual displayed action/result text in successful and unsuccessful cases, keeping missed, resisted/immune and unchanged/already-present outcomes distinct where the game does. Reuse retained offline command, renderer and application-path evidence before requesting gameplay evidence.
- Historical October12 documentation pass made no code/build/deploy/runtime changes. The October13 implementation request supersedes its deferral. Keep native command/result meanings, accepted-popup and packet-consumption guards; no invented success/miss/resistance text. Runtime testing remains user-side.

## Scope

This request documents future work only. It does not authorize accessing restricted Tester Logs, inspecting the game installation, launching/controlling the game, live probes or publication. Follow the project guidance, shared model policy and existing runtime permissions when any issue is taken up.
