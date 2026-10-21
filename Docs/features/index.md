# Current feature index

October20 maintenance inventory. The production root is `src/native`; the
`src/proxy` x86 DLL and original `src/launcher` adapters deliver it. Historical
JavaScript/Frida and the abandoned C# port are references, not alternate producers.
Start with the subsystem topic below, then its linked native evidence and tests.
Later corrections in these topics take precedence over historical session notes.

[Feature design/status register](status.md) separates implementation progress,
offline evidence and gameplay confirmation. The subsystem topics below form the
current [mod/game architecture map](status.md#architecture-and-design-entry-points).

| Feature | Current implementation and maintenance topic |
|---|---|
| Verified executable profiles, proxy loading, hook ABI, resource reset | [Runtime ownership](runtime/index.md) |
| PRISM speech/braille, backend choice, producer logs, cancellation/history | [Output and text](runtime/index.md#output-and-text) |
| Native text, remapped button glyphs, Polish fan translation, language limits | [Text and localization](runtime/index.md#native-text-and-localization) |
| Keyboard, DirectInput/XInput/SDL, controller mod mode, bindings, focus | [Input](runtime/index.md#input-and-settings) |
| F8 settings, defaults, persistence, independent keyboard commands | [Settings](runtime/index.md#input-and-settings) |
| Field menus, items/equipment/abilities, character targeting, Aeon attributes | [Menus](menus/index.md) |
| Save/load slots, confirmations, naming, shops, Customize sold-item filtering | [System screens](menus/index.md#system-screens) |
| Conditional Monster Arena menu, native title/focus/instructions | [Monster Arena](menus/index.md#system-screens) |
| Native dialogue, tutorial/Blitzball review, voice policy, repeat, Will captions | [Dialogue](menus/index.md#dialogue-and-review) |
| Help topics/pages, prompts/empty character slots, line buffers, footer controls | [Help](menus/index.md#help-pages) |
| Escape configuration, Special Features changes, title/quit prompts | [Escape and title](menus/index.md#escape-and-title) |
| Standard/Expert Sphere Grid, routes/stat/ability planning, directional rules | [Sphere Grid](menus/index.md#sphere-grid) |
| F6 node-number dialog and Grid controller guidance | [Sphere Grid](menus/index.md#sphere-grid) |
| Commands, targets, turn speaker, damage/healing/status and graphical words | [Battle](battle/index.md) |
| Visible CTB turns, Overdrive meters, HP/MP panels | [HUD](battle/index.md#visible-hud) |
| Sensor, statuses, stack buffs, Stone Gaze/native ability titles | [Battle details](battle/index.md#details-and-effects) |
| Results mandatory EXP/AP/gil/levels and review-only distribution/items | [Results](battle/index.md#results) |
| Tidus/Auron/Lulu/Wakka manual cues and optional assistance | [Overdrives](battle/index.md#overdrives) |
| Map/entrance arrivals, NPC/event/object/shop/save/chest/exit identification | [Navigation](navigation/index.md) |
| Collision routes, dynamic gates, unreachable filter, water/depth transitions | [Routes](navigation/index.md#routes-and-validity) |
| Beacon, walls, arrival, stereo/pitch, cue ownership and HRTF limits | [Navigation audio](navigation/index.md#audio) |
| Auto walk, cancellation, pedestal movement and native input handback | [Movement](navigation/index.md#movement) |
| All six Cloisters, native interaction interception, first/return/treasure gates | [Trials](puzzles/index.md#cloisters) |
| Zanarkand visual glyph sequence and floor labels | [Zanarkand](puzzles/index.md#cloisters) |
| Via Purifico treasure collection and swimming progression | [Via](puzzles/index.md#other-progression-puzzles) |
| Gagazet target/swimmer puzzles, manual targeting cues and optional solve | [Gagazet](puzzles/index.md#other-progression-puzzles) |
| Jecht Shot practice/memory assistance and manual cues | [Minigames](puzzles/index.md#minigames) |
| Thunder Plains lightning, Chocobo race/training, butterfly hunt | [Minigames](puzzles/index.md#minigames) |
| Airship coordinates/free search and approved six-region picker | [Airship](puzzles/index.md#airship) |
| Airship image destinations and Proceed choices, native focus, visibility and language | [Airship list/confirmation](puzzles/airship.md) |
| Free Search loop lifetime/toggle repair and restored airship audio previews | [Free Search audio](puzzles/airship_audio_20261020.md) |
| Shift+M picker ownership, field function-key handback, Password readback | [Airship/input follow-up](puzzles/airship_keys_20261020.md) |
| Sea of Sorrow dead end, scripted returning ledges and misleading merged bends | [Implemented, deployed and gameplay-confirmed](navigation/sin_sorrow_routes_20261020.md) |
| Later Sin rising/disappearing obstacles | [Implemented/deployed, tester gameplay pending](navigation/sin_late_obstacles_20261020.md) |
| Requested chocobo, butterfly and Blitzball completion | [Completion implementation, native evidence and limits](puzzles/minigame_completion_20261020.md) |
| Required Nucleus object collection and optional autosolve | [Implementation and original-native checks](puzzles/sin_collection_20261020.md) |
| Six Baaj seal destinations, player-entry geometry and named save prerequisites | [Baaj implementation and verified delivery](navigation/baaj_seals_20261020.md) |
| Cid/Wakka identity, story-independent actor presence and moving destination selection | [Airship Cid tracking](navigation/airship_cid_20261020.md) |
| Airship own-log repair, password N/punctuation and deferred Y | [Airship/password follow-up](puzzles/airship_password_20261019.md) |
| Developer console, ordinary/test warps, progression setup restrictions | [Developer utilities](distribution/index.md#developer-utilities) |
| Console-enabled Baaj build, exclusive Shift+F1 and test warp | [Console/F1 repair](runtime/console_f1_20261020.md) |
| Original launcher, LAA, install/check/repair, locked dependencies, packaging | [Distribution](distribution/index.md) |

## Evidence and coverage

The topics describe source entry points, data flow, ownership, extension rules
and verification limits. [Controls](../controls.md) is the player-facing key
reference; [current Dylan follow-up](../dylan_reports_20261018.md) records the
active report evidence and deployed repairs. Release0.5.8 includes these fixes
and the later airship/password repairs described in their current records.

The source inventory generated by `Docs/Scripts/Offline/audit_feature_docs_20261018.py`
records every native translation unit, its topic, hash, registration functions
and explanatory-comment count. It is a change/coverage inventory, not proof that
counting comments establishes documentation quality. Topic and source review
must continue for changed paths. Native profiles, lifecycle and gameplay gaps
remain in the linked evidence; old successful fixtures are not fresh gameplay
confirmation. Airship destination labels and password entry are user-confirmed;
the later Proceed? option reader remains unresolved for the actual reported
prompt. The October20 loop repair replaces blanket airship audio exclusion;
its own evidence and pending gameplay checks are in the linked audio topic.

[Source map](source-map.md) lists the actual production modules and their primary
topic. [Settings inventory](runtime/settings.md) retains the exact current defaults,
ranges and persistence keys. [Verification and remaining checks](verification.md)
distinguishes current offline evidence from user gameplay confirmation.

[Repository-content and external-input boundary](repository_content_20261020.md)
records the original evidence locations, source-free replay adapters, exporter
guards, authorized historical cleanup and the remaining GitHub retained-object blocker. The [documentation review](documentation_audit_20261020.md)
records source agreement, coverage and verification limits.

Current [Seymour/Spare Change/parameter prompt investigation](battle/seymour_amount_prompts_20261021.md) tracks the October21 report and its verification limits.
