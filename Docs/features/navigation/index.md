# Navigation, route validity, audio and movement

Airship Cid identification and offline verification are **complete**; guarded deployment is complete; gameplay remains unconfirmed. Both reader profiles pass actual capture, selection, movement and worker transfer. [Airship Cid identity and tracking](airship_cid_20261020.md) records the native/model identity, live actor handling and test limits.

The Baaj report is closed by user gameplay confirmation: Besaid correctly remains inactive without its treasure, and every eligible seal works. Final door opening with all six prerequisites was not tested in this save. See [Baaj evidence and confirmation](baaj_seals_20261020.md).
## Identification and observations

`navigation.cpp::installNavigation` connects query input, native movement/actor/
interaction events, owned observations, reachability, pedestal and Auto walk.
`navigation_observations.cpp` captures the native scene; `navigation_scene.cpp`
prepares entities from owned parts/actors/worker interaction facts. Scene bounds
are500 workers,128 actors,64 physical parts and32767 vertices/triangles each.
These are transport limits, not claims that every room reaches them. Allocation,
worker/header, physical-part extent, epoch, scene and player/control identity
must match before ready facts are admitted.

`atel.cpp`, `atel_memory.cpp`, `nav_exits.cpp`, `exit_labels.cpp`,
`navigation_targets.cpp`, `navigation_labels.cpp` and generated/hash-qualified
catalogs identify exits, NPCs, events, objects, save spheres, shops and chests.
Native active channels/useful entries, actor binding/hidden/readiness and actual
interaction geometry determine whether a candidate exists. A nested warp in an
event is not automatically the event's category. Save spheres were audited across
all retained maps; same-room lift/door interactions are not assumed absent.
Stable worker/entry identities preserve numbering as other targets disappear.

The user's October18 exception permits reliable map/code/developer/HUD names
for **pathfinding identification** before a visible name reveal. It does not
permit invalid routes, invented entities or treasure contents. Treasure labels
remain type-only. New Yuna sphere and Dome staircase labels use exact retained
script length/checksum and entry2; native activation/collection remains in charge.

See [catalog and save-sphere audit](../../save_sphere_audit_20261009.md),
[exit/arrival identities](../../exit_status_swimming_20261009.md),
[map asset audits](../../navigation_asset_audit_20261007.md),
[native field activation](../../open_issue_repairs_20261012.md), and
[October18 missing events](../../dylan_reports_20261018.md).
`publishFieldArrival`/`announceMapArrival` require completed native load plus a
valid initialized controlled actor. Saved room numbers alone do not trigger an
arrival. Repeated genuine arrivals are preserved.

## Routes and validity

The [Sea of Sorrow repair record](sin_sorrow_routes_20261020.md) documents the
implemented directed-event route constraints and bend-preserving speech change.
Source-free checks and the warning-free final developer build pass; guarded
deployment is complete; the user confirms Sea of Sorrow works on October20.
The later Sin gate repair and optional Nucleus collection module pass their
offline route/input/native-reader checks and warning-free build, and are
guarded-deployed. Approved exact City/Nucleus floors now pass original native
collision replay, including192 onward/return routes across all64 gate states.
Four missing platform boarding stops pass both reader profiles and are deployed;
full ride/gameplay confirmation remains pending.
See [later Sin obstacle audit](sin_late_obstacles_20261020.md) and
[Nucleus collection](../puzzles/sin_collection_20261020.md) for their separate status.

`nav_memory.cpp`/`nav_geometry.cpp`/`nav_zones.cpp` decode copied collision and
target shapes. `navigation_intents.cpp` and owned computation connect player
position to the actual usable interaction zone. The same committed route feeds
spoken path/distance, beacon, reachability and Auto walk. Avoid an independent
straight-line reachability shortcut in one consumer.

Native collision83E4E0 plus both switch tables establish dynamic group rules,
including aliases48..63 and the strict64 boundary. Captured remaps and actor
flags are evaluated against the real geometry; unsupported/unready data stays
unresolved instead of becoming passable. `reachability_capture.cpp` and
`reachability_worker.cpp` keep context/target identities with each result. The
default-Off Hide unreachable filter removes only a proved unreachable target;
pending/unknown data remains listed. Matching committed results survive a
compatible refreshed scene, while changed geometry, pose, gate or epoch retires
them. An empty/no-path result must propagate to every route consumer.

Water/depth routing is a three-dimensional transition problem. `swimming_route.cpp`,
`swimming_navigation.cpp` and generated Gagazet cross-water links join verified
water sections to their dry cave crossings; the path is not a direct swim
between disconnected pools. Native party preparation and story/puzzle gate
initializers still open the physical passage. Spoken direction/distance is a
presentation transform; Partial steps only changes rounding, never route
geometry or cue precision. Preserve native world/camera axes and interaction
endpoint height rather than flattening every floor onto X/Z.

Evidence: [reachability pipeline](../../reachability_filter_20261012.md),
[Besaid/no-path correction](../../help_navigation_quit_20261015.md),
[swimming progression](../../swimming_progression_20261009.md),
[Via swimming](../../via_swimming_blocker_20261012.md),
[Gagazet return/cross-water bake](../../gagazet_cave_20261016.md).
`navigation_asset_audit_20261006.md` and retained Offline scripts explain baked
data provenance. Regenerate/check the affected real map, including dry/wet and
gate states, rather than adding an unverified global passage.

## Audio

`nav_cues.cpp` owns a bounded POD route leg and camera-relative cue facts.
`wall_cues.cpp` computes walls from owned geometry. `nav_audio.cpp` receives
bounded commands and alone synthesizes/mixes the SDL stream after the physical
SDL owner establishes audio readiness. Native actor/movement callbacks do not
open devices or wait for computation/audio. Generation/context and independent
preview, warning, arrival, timing and position cancellation retire stale sounds.
Settings, focus, pause and route loss gate each relevant sound.

The current implementation uses stereo left/right, distinct rear pitch and
proximity/rate. Beacon and wall volume/range are separate; Arrival requires an
enabled beacon and actual final destination. Blue butterfly/race pickup cues,
red hazards and manual timing tones have independent gates. October18 Gagazet
launch tones add their own cancellation and reject commands older than100ms;
this is an obsolete-cue cutoff, not a hardware latency guarantee.

Full HRTF3D playback and a separate global sound/HRTF UI are not present in this
existing implementation; do not claim the maintained new-mod baseline is already
implemented. `audio_sample.hpp`, embedded resources and SDL license/runtime are
deployment dependencies. [Audio/route ownership](../../navigation_blitz_autowalk_20261007.md)
and [SDL owner evidence](../../sdl_owner_20261012.md) cover the current pipeline.
The real dummy stream fixture tests production mixed samples/gates; fake SDK
ownership tests and component timings remain separate from audible gameplay.

## Movement

`auto_walk.cpp`, `movement_keys.cpp`, `game_binding_input.cpp` and input assist
adapters use approved native input routes. Auto walk defaults Off; speaking its
path is a separate default-Off setting. Ordinary movement cancels immediately;
menus, dialogue, focus loss, combat, stale context, route loss and stall cancel.
Arrival does not automatically interact. Native input handback must survive
worker/SDK failure without waiting for a mod result.

`pedestal_input.cpp`, `pedestal_keys.cpp`, `pedestal_state.cpp` and
`pedestal_rules.cpp` track the native held-pedestal mode/interaction geometry.
Push/crossing/recess targets differ; a held pedestal cannot be treated as
ordinary free-walk input. [Pedestal input](../../pedestal_input_re_20261007.md),
[movement evidence](../../pedestal_movement_re_20261007.md) and
[current trial rules](../../trial_entry_followups_20261016.md) retain the exact
native bodies, transformations and validation. Do not bypass the Bevelle rail
pedestal requirement when adding a solver trigger.

## Extension and checks

Add identification through the qualified catalog/native name path, actual useful
entry and live zone. Preserve native active/hidden/collision and treasure rules.
Add route types through shared geometry/owned scene facts so filter, speech,
audio and walking agree. Worker jobs carry copied data, cancellation/version and
bounded computation; native callbacks perform bounded observation only.

Run `run_tests.bat --readers`, `--reachability`, `--navigation-geometry`,
`--navigation-audio`, `--pedestal`, `--native-actions` as affected. Tests include
original collision and script predicates, stale ownership, disabled/re-enabled
events, strict endpoints, disconnected floors and water portals. Controlled
poses, native API adapters and fake SDK callbacks are documented dependencies.
User confirmation after a known game build is still required for actual movement,
audio reaction and unusual maps. Installer/archive preservation is independent.

The October20 [Free Search audio repair](../puzzles/airship_audio_20261020.md)
supersedes release0.5.8's temporary blanket airship mute. The native permission
uses room382 or M's active search flag, with transition cancellation; the mixer
also checks each pickup's current generation and original category toggle.
Invalid field/race/butterfly loops are refused in search, while explicit
previews and separately gated events work again. This is offline verified;
the user's actual audible source and both menu routes await gameplay confirmation.
The [deferred Y question](../puzzles/airship_password_20261019.md#deferred-y-coordinate-question)
is unchanged.
