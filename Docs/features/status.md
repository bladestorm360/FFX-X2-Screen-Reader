# Feature design and status register

October20 maintenance register, based on the reviewed source map and existing
subsystem topics. This records existing designs rather than claiming they were
planned retroactively before implementation. Future features need a design and
status before implementation. Complete means the named implementation scope is
present with documented offline checks; it never means all game states or
hardware behavior are confirmed. Percentages are rough engineering estimates.

| Feature / intended player task | Implementation status | Offline / gameplay distinction |
|---|---|---|
| Executable profiles, proxy/hook lifecycle, resource retirement | complete | Profile/ABI/owner hosts pass; actual field-freeze cause unresolved |
| PRISM speech/braille, backend choice, cancellation/history | complete | Production adapter/queue/domain hosts; hardware remains backend dependent |
| Native text, glyphs, Polish fan-translation detection | complete | Getter/decoder/profile fixtures; native translations distinct from mod UI |
| Keyboard/controller mode, bindings and focus handback | complete | Input/COM/controller hosts; new F-key correction pending gameplay |
| F8 settings, defaults and deferred persistence | complete | Actual registration/settings/menu hosts; saved settings preserved |
| Field equipment/item/ability menus and target details | complete | Native menu fixtures; historical gameplay evidence in menu topic |
| Save/load, naming, shops and Customize filtering | complete | Native getter/predicate replay; new Password subject pending gameplay |
| Monster Arena conditional menu | complete | Native capture/focus/render cases; not an automatic capture solver |
| Dialogue, voice policy, tutorial/Blitzball review and timed captions | complete | Speech/voice/caption fixtures; optional Blitzball auto-win has its own producer below |
| Help pages, line buffers and footer controls | complete | Native renderer/full-page fixtures; Help buffers user-confirmed |
| Escape/title/configuration and Special Features notifications | complete | Native reader fixtures; hardware/mapping limits documented |
| Standard/Expert Grid, planners, number entry and controller guidance | complete | Original selector/input replay; newer control routes retain gameplay limits |
| Battle commands, targets, actions, damage/healing/status | complete | Battle producer/readers; full enemy analysis remains separate |
| Fresh H/target HP/MP and effective Switch capacity | complete; deployed1.0 | Both173-hook profiles, original playback/capacity/Change renderer and real owned status worker pass; warning-free release build/guarded deployment; percentages retained; gameplay pending |
| Visible CTB, HP/MP and Overdrive panel review | complete | Owned HUD fixtures; native visibility/order retained |
| Sensor/details, buffs and Stone Gaze titles | complete | Native replay/reader checks; Stone Gaze gameplay pending |
| Seymour mechanics, amounts, confirmations, Formation and learned modes | complete; deployed1.0 | Both173-hook profiles, original256 wheel combinations and numeric/learned-message bodies, popup worker, profile/ABI and warning-free build/guarded deployment pass; gameplay pending |
| Battle results and review buffers | complete | Native order/reward fixtures; optional details separate |
| Tidus/Auron/Lulu/Wakka cues and optional assistance | complete | Native cue/input hosts; manual Wakka timing pending gameplay |
| NPC/event/object/shop/save/chest/exit identification | complete | Supported catalog/owned capture plus six Baaj regions verified and deployed; gameplay limits remain |
| Collision routes, dynamic gates, water/depth transitions | complete for existing scope | Mesh/reachability/arrival hosts; Sea of Sorrow ledges and live Sin gate repair verified/deployed below |
| Sea of Sorrow dead-end investigation | complete | Exact1220-triangle floor/production route and original callback facts reproduce return loop; current log corroborates landing |
| Directed Sea of Sorrow ledge routes and preserved bend instructions | complete | Original floor, both reader profiles, reachability/beacon/Auto walk/native-action checks pass; guarded-deployed; user gameplay-confirmed October20 |
| Later Sin rising/disappearing obstacles | complete for investigated scope | Approved exact floors pass192 City exit routes/all64 gate states and connected Nucleus/40401 candidate positions; four missing boarding targets pass both reader profiles, warning-free build and guarded deployment; full ride/gameplay pending |
| Sin final item-collection accessibility and optional autosolve | complete | Saved default-Off F8 toggle on Sin maps only; both Nucleus aliases and entry/battle resets pass component/menu/settings/both reader profiles; warning-free build, guarded deployment and refreshed0.5.9 installer/ZIP verified; gameplay pending |
| Beacon/walls/arrival/race/hunt sounds and stereo positioning | complete | Real SDL sample/ownership hosts; new search cancellation pending gameplay |
| Reported Free Search audio loop | in progress (~90%) | Toggle/generation defect repaired; exact audible producer unconfirmed |
| Auto walk, cancellation and pedestal/native input | complete | Route/action hosts; independent default-Off settings |
| All six Cloister guides and optional solvers | complete | Real script/geometry/state hosts; solvers and named returns user-confirmed |
| Zanarkand glyph sequence and floor labels | complete | Owned order/guide checks; first/return gates retained |
| Via Purifico treasure/swimming progression | complete | Original script/floor/arrival hosts; remaining state combinations documented |
| Gagazet targeting, swimmers and optional solve | complete | Native cue/geometry/state hosts; solving/returns/cross-water user-confirmed |
| Jecht practice cues and optional success | complete | Tutorial/input hosts; final behavior user-confirmed |
| Supported Lightning, Chocobo and butterfly assistance | complete | Scoped cue/input/route hosts; other side activities incomplete |
| Calm Lands/Remiem, butterfly and Blitzball optional completion | complete |0.6: all9 original variants,64 Remiem flags, score/choice/story matrices, menu/settings, both reader profiles and native regressions pass; warning-free release-flavor build guarded-deployed, gameplay pending, installer held |
| Airship coordinates, six-region picker and Shift+M | complete | Registration/search-state checks; both real menu paths pending new gameplay |
| Airship image destination list | complete | Native image/render cases; list reading user-confirmed |
| Password entered-text subject and punctuation controls | complete | Selector/copied-cell hosts; entry confirmed, new subject pending |
| Actual reported silent Proceed? choices | in progress (~40%) | Layer29 replay passes; user suspects wrong prompt; further work deferred |
| Free Search Y purpose/control | not started | Investigation deferred by user; M's X/Z confirmed |
| Six Baaj seal navigation targets | complete | Offline labels/64 flag combinations/native geometry pass; user confirms eligible seals work and missing-treasure Besaid stays inactive |
| Baaj native activation/closed Fayth door report | complete; user accepted | Latest gameplay agrees with missing Besaid treasure; all eligible seals work. Report closed; all-six final door opening not claimed |
| Cid airship identity, presence and moving destination | complete | Both171-hook profiles pass four original variants/native capture/selection movement and worker transfer; warning-free build guarded-deployed, gameplay pending |
| Developer console/warps with release exclusion | complete | Development/release utility hosts; progression restrictions retained |
| Console-enabled Baaj test build and exclusive Shift+F1 | complete | Input/COM/console/dev-release utilities, two reader profiles and warning-free build pass; guarded-deployed, gameplay pending |
| Original launcher, installer and locked dependencies | complete |0.5.9 release build/guarded deployment,805 source/130 final EXE assertions, short/long install/reinstall,26 resources and5 ZIP files verified |
| Full mod UI language catalog/manual override | not started | English mod labels/native translations documented separately |
| HRTF playback and independent global spatial controls | not started | Existing stereo/rear-pitch output does not implement this baseline |
| Full enemy analysis, other side activities, FFX-2/Last Mission | not started | Outside currently implemented FFX scope |
| External game-source inputs and source-free repository/docs | complete for reviewed local content/history and replacement remote | User deleted affected repository; clean history restored to new public ID1428963798; all32 known removed objects404; physical GitHub backup erasure is not claimed |

## Architecture and design entry points

The [runtime topic](runtime/index.md) maps central mod architecture: x86 proxy
and profile validation initialize bounded native hooks; native-owner capture
copies context/generation-owned facts; prepared owned workers build text/routes;
central PRISM/audio services deliver output; reset/close retires resources
without waiting on the game thread. Input capture and native actions remain
separate, with dispatch-time availability recheck and native handback. Settings
use atomic values and deferred persistence. [Distribution](distribution/index.md)
maps launcher/install/deployment ownership and locked dependencies.

Relevant game architecture is beside its consumers. [Menus](menus/index.md)
explains native row/text and screen phases, [battle](battle/index.md) identifies
action/HUD/status producers, [navigation](navigation/index.md) maps ATEL workers,
collision and live gates, and [puzzles](puzzles/index.md) maps script/state
mechanics. Each links native locations, lifecycle, evidence and extension rules.
The [Baaj region contract](navigation/baaj_seals_20261020.md#native-behavior-and-limits)
adds the newly investigated mechanic; the
[keyboard/password contract](puzzles/airship_keys_20261020.md) adds this task's
selector/key ownership design. [Source map](source-map.md) covers all143 native
production units and [settings](runtime/settings.md) all37 persisted options.

The [verification procedure](verification.md) separates offline evidence,
historical gameplay confirmation and remaining checks. Baaj and the earlier audio/key/password repairs passed offline checks and
user-requested guarded deployment. Release0.5.9 is built and packaged with those
repairs. The four0.6 completion assists are implemented, offline-verified and
guarded-deployed; their installer/package remains held for gameplay testing. No game launch/control/probe is authorized by this register. The
[repository-content record](repository_content_20261020.md) documents external inputs, exporter guards and the completed source/history migration.
