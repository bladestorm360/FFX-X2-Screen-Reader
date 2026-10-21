# Sin — The Nucleus object collection

The original implementation was guarded-deployed in0.5.9. The October20
follow-up is **complete**: implementation, component/menu/settings checks, both
reader profiles, warning-free release build, guarded deployment and actual
installer/ZIP verification pass.
The tester could not find the setting inside The Nucleus, and crystal battles
made enabling it after entry impractical. Menu availability and preference
lifetime are repaired below. Gameplay confirmation remains pending.
[Original delivery receipt](../../../install/sin-collection-20261020/verification.json).
Follow-up [release receipt](../../../install/nucleus-prearm-20261020/release-verification.json)
and [guarded deployment](../../../install/nucleus-prearm-20261020/deployment-verification.json).
No agent game
launch/control/probe has occurred.

## Mechanics and native evidence

The user's report separates the rising objects after Seymour from the collection
section beyond the final Save Sphere. Retained original scripts identify the
collection as `sins0500`, The Nucleus:60258 code bytes,44 workers,
FNV32 `05e485ed`, EBP SHA256
`c81148c2c9d6d8c7394d97b3d2285c3b7a77a839543b74d91dfc03ac764cb229`.
The existing verified room/area table maps both rooms324 and341 to Nucleus
area312. Checking only324 was too narrow. All Sin native areas are307–316;
the retained room mappings are203,204,296,322,324,325,326,327,341 and384.
Names307/308 are Near Airship,309 Sea of Sorrow,310 Garden of Pain,311 City
of Dying Dreams,312 The Nucleus,313–315 Dream's End and316 Tower of the Dead.
Current mappings do not assign an ambiguous room to308/315.

Ten type1 workers24–33 load collectible model20596. Their original entry6
shows the object, waits90 native ticks, then tests distance below3.6 **and** the
original Common019F(1) player-control predicate while a
150-iteration appearance window remains. Missing the window hides the object;
the native controllers retry. This is not evidence of one overall countdown.
Two controllers36/37 advance through the even/odd workers, respectively,
incrementing separate five-object counters only after each original pickup flag.
W40 waits for their sum to reach10, then owns control/encounter cleanup, story
3260 advancement and the room325 transition. Collection is required to proceed.
The mod does not reveal item contents before the native reward.

Original evidence and parsed models remain outside every Git tree at
`Analysis/FFX/SinDynamic_20261020/171143_927729/`. `collection-contract.json`
records factual worker/entry/PC/declaration locators. Original EBP parsing keeps
Karifean/FFXDataParser attribution; native call/layout references retain
Fahrenheit attribution. No original function bodies or byte arrays belong in
the mod repository or this document.

## Controls, defaults and behavior

On any Sin map, use **F8 → Miscellaneous → Sin object collection**. The submenu
has automatic collection, instructions, status and Back. Keyboard and existing
controller menu actions use the same handler. There are no new function keys.
Automatic collection defaults **Off** and is persisted as
`Accessibility/sin_collection_assist`,0/1, using the existing deferred settings
writer. Enable it before entering The Nucleus, for example in City of Dying
Dreams or Tower of the Dead. The menu is hidden outside Sin. Its availability
needs only native-owned room/area facts, so pending script preparation or a
battle cannot remove the option. Other global menu/input gates remain unchanged.
The preference survives room/resource/generation resets, battles and restarts.
Only borrowed interpreter scope is reset; actual assistance must requalify every
fresh resource and read. Off is immediate. Status reads native counters only in
the qualified Nucleus; other Sin maps report ready, and unprepared Nucleus
reports count not ready. Completed collection cannot be rearmed.

Close the mod menu after enabling. Each normal active pickup receives proximity
assistance. The original native script still owns appearance, waits, control,
reward dialogue, item grant, pickup flag, counter advancement, hiding and final
transition. Confirm any native reward dialogue. Rising spikes and native battles
retain their original behavior; assistance pauses in battle, menus, the console
or foreign focus. No position, inventory, completion counter, game variable or
bytecode is written by this helper.

The current mod menu/UI uses English. Full mod UI localization, language override
and HRTF remain recorded project gaps; this feature does not establish them.
It adds no sound or independent speech producer for native pickups.

## Source, state and ownership

`sin_collection.cpp/.hpp` is one cohesive feature module in the existing native
source layout. `reader.cpp` installs it and resets it with owned readers.
`mod_menu.cpp/.hpp` captures availability on the native owner and builds the
existing copied TextPlan/UtilityText menu output. The preparation/speech owner
does not enter game getters.

Two listeners reuse verified native APIs: interpreter464180 establishes and
restores nested TLS ownership; variable-pointer getter46C500 observes its native
return. The getter mapping46C500/installed46C570 already belongs to the verified
core profile. Both the exact ready allocation and current room/controller must
match. The existing native worker index checks actual loaded global membership
through its getter; a local worker number is not assumed to be global. The
controller's global count may include other resources, so equality with the
local44-worker header is no longer required. The established registry bounds
loaded globals to500; a cold/changed owner collects at most500 scalar identities
into fixed storage. On/status or ordinary native discovery can warm it. No new
data address is assumed. Menu visibility does not enter this resource path.

Only workers24–33, active signal entry6, matching thread/priority, and each
original distance-read PC qualify. The ten bodies are596 bytes apart; their
PUSHV read is at BEDA+596 times the worker offset and declaration207+3 times
that offset. The opcode, operand, prepared header/declaration and current live
declaration are checked. Its float declaration is66000000/high1. Adjacent
instructions, the earlier POPV write and cleanup entry7 do not qualify.

The original86E3D0 typed operand reader calls86C5A0 for a float;86C5A0 consumes
the synchronous46C500 pointer. The native logical AND/control read immediately
after the qualified distance PUSHV is preserved; assistance never forces that
separate predicate. `verify_sin_collection_20261020.py` checks its exact original
instruction order alongside separate writes, rewards and completion threshold.
The helper substitutes a process-lifetime
read-only float zero for that one read. The original computed distance remains
unchanged, and the original x87 float and typed stack paths remain intact.
Native864180 dispatch and its PC advancement were checked statically: the
current instruction remains at thread+18 until the PUSHV returns. No borrowed
game pointer or asynchronous mutation is introduced.

The native path has a constant number of ready scalar/header/declaration reads,
no parser, hashing, waits, allocation, new timer or background memory watcher.
Unavailable/mutated preparation and ownership refuse assistance. The existing
native hook adapter preserves integer, x87 and SSE state and returns immediately.
Only explicit menu actions build bounded UtilityText output.

## Offline evidence and limits

`tests/sin_collection_fixture.cpp` executes the production module and original
native variable-address, float-getter and PUSHV functions against the retained
original resource. Native bodies/tables/fixups are external under
`RepositoryEvidence/offline-output/sin_collection_20261020/`; body size/FNV64
and source provenance qualify the supplied original code. Controlled dependencies
are interpreter scope/PC, affine getters, focus/scene and the diagnostic endpoint.
The host does not execute the full VM, inventory backend or animations.

The follow-up canonical host passes all ten Sin room mappings/non-Sin hiding,
both Nucleus aliases, pending/changed scripts without hidden controls, actual
City/Tower resource-to-Nucleus transitions, preference retention through battle
and reset,45/500 loaded globals, all ten exact reads, unchanged underlying distances,
Off, adjacent/write/cleanup PCs, signal level, current global worker, changed live
declaration, room/owner/focus/menu/battle/console, all36 counter combinations,
completion, generation/reset and pending/changed-resource cases. Component
measurements are retained in `install/nucleus-prearm-20261020/sin-collection.log`, using
controlled PC/scope/getters; full-VM, inventory and animation behavior are not
covered. The isolated actual settings owner also checks fresh Off and all64
saved assistance/cue combinations, preserving exact INI bytes on load. Canonical
`--settings-defaults` keeps that INI beside its isolated fixture executable.
The retained original script contract rerun passes all ten pickup/control/reward/
expiry sites and native count10 progression. Cold first pickup16.700us, cold
maximum500-global-worker pickup17.400us and1000 warm production reads1.761ms
have heap0/0 in the controlled host. Both171-hook reader profiles and the
canonical menu/startup/development/release utility checks pass. The actual final
installer passes805 source assertions,130 actual dependency assertions and
122/340-character short/long install/reinstall. All26 resources,7 deployed/
embedded payloads and5 ZIP files match. Player docs and checksums match sources.
Existing settings, named saves and0.5.7/0.5.8 ZIPs are unchanged; the previous
same-version0.5.9 is preserved by packaging.

Luna `gpt-6-luna`,medium, updated only README/changelog/controls from supplied
facts for this follow-up. Sol reviewed and simplified user instructions, updated
technical documentation and owns discovery, implementation and all verification.
No delegated defect remains unresolved. Gameplay remains pending.

The actual production hooks and MinHook were also measured in an isolated host.
The original figures below describe the first implementation; the follow-up
log retains fresh baseline/Off measurements, cold first-pickup and maximum500-
global-worker measurements. No whole-game timing is inferred from either run.
With two interpreter listeners and10,000 getter calls, the warm five-run baseline
mean/max is18.380/18.600us. With collection Off,10,000 calls measured4513.740/4574.900us mean/max
(first run4536.500us);1000 warm calls measured434.260/450.600us mean/max, and100
measured51.380/73.700us. This quantifies the default-Off global getter hook in
that host. Actual game call rate, frame impact and audible response remain
unmeasured; these host figures do not establish in-game responsiveness.

Luna `gpt-6-luna`, medium, implemented only the supplied-pattern F8 wiring and
controlled menu tests. Sol reviewed the UI wiring and canonical validation passed;
the initial Python evidence-root warning was superseded by the clean primary
`--accessibility` run. Both171-hook reader profiles pass `--readers` after the shared collision-serial
repair. Native behavior,
RE, pointer/ABI choices, integration and final verification remain Sol-owned.
No menu defects remain from the delegation. No Astra delegation occurred.

The final current-source `--no-deploy` build passes without warnings, including
the shared gate repair. Guarded deployment completed with all7 installed/staged
hashes matching and existing PRISM/launcher bootstrap verification passing.
This paragraph records the original delivery. The requested same-version0.5.9
installer refresh supersedes that installer hold. Full mod UI
localization and HRTF remain explicit project gaps.
User-side confirmation: enable collection on a Sin map before The Nucleus,
close F8 and enter. After a crystal battle, assistance should resume without
reopening F8. Confirm any native reward dialogue; status should advance to10
and the game's own transition should follow. Both Nucleus variants should still
offer the submenu. Off restores ordinary pickup; outside Sin the entry is absent.
No progress reset or live capture is required for the offline evidence.

Retained preparation-map evidence is outside Git at
`Analysis/FFX/NucleusPrearm_20261020/212350_082933`. Tower `sins0900` has7551
code bytes,17 workers,FNV32 `5e11a138`,EBP SHA256
`5836fcc718a400b6402874f8d108142bc1e7cc25bdab0139282ccc70d2fe457c`.
Its two literal warps return to City room204; they do not alone prove a direct
Nucleus transition. Room/area names come from the retained native English macro
and existing verified map manifest. No new installed asset access was used.


## Approved Nucleus floor replay

The user explicitly approved read-only extraction of the Nucleus map asset `ffx_ps2/ffx/master/jppc/map/sins/sins05/bin/mapout.vpa`. It is retained outside Git at `Analysis/FFX/SinMaps_20261020/184012_314750` (1579808 bytes, SHA256 `5dd7c0c7e17f294fb35c51c31ac9837382f2da9a9a8618a1c031f30acf89cdd1`). Its original 83EA00 ChMap floor has 146 triangles/91 vertices, one connected group 0 component. Offline replay against original 83E4E0 and actual jump tables reaches every triangle center from the native entrance using 15814 route samples; all 40401 integer X/Z points in [-100,100] are on the floor, including both endpoint candidates around native `GetRandomInRange(200)-100`. The full RNG/spawn controller was not executed. The off-floor exit lines do not have walking routes; original count 10 W40 warp 325 remains progression. No live gameplay, full VM or whole-game performance claim follows from this replay. The map extraction and preparation adapters and external reports are recorded in [the later Sin obstacle investigation](../navigation/sin_late_obstacles_20261020.md).
