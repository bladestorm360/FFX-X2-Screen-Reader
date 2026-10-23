# Verification and remaining gameplay checks

## Launcher language/settings follow-up — October23

The [language repair](distribution/launcher_languages_20261023.md) is offline
verified and included in the guarded-deployed1.0.1 release build.
At the next convenient launcher test: with Japanese selected,
press Down/Up and expect each choice spoken. Open Setting, change language,
save/close, then use arrows/Enter; expect current-language labels and the correct
action. Native settings controls keep their own input/speech. All eight startup
locales/seven actions and64 settings transitions pass the actual-adapter fixture;
actual launcher focus, sounds and audible output still need user confirmation.

## Destruction-sphere flags — October22 priority

Besaid sphere collection is gameplay-confirmed by the user on October23.
The remaining priority is a remote tester reproducing Baaj Ruins unlock through
normal gameplay without warps. The supplied save already has Zanarkand's flag,
despite a reported missing sphere; a missing Zanarkand flag is not established.
For any further trial-collection regression, test its Baaj seal after saving/reloading.
Enable **F8 → Miscellaneous → Cloister of Trials → Solve current trial
automatically**, interact normally and confirm the game's treasure dialogue.
Expect the reward and retained sphere prerequisite; destruction-only solving
stays in the trial. A repeat reward, hang, interruption message or inactive
eligible seal is a failure. The supplied save is missing Besaid alone; no save
was edited. [Offline checks, deployment state and limits](puzzles/destruction_flags_20261022.md).

Current October18 repair evidence is in
[Dylan follow-up](../dylan_reports_20261018.md) and its
[deployment receipt](../../../Analysis/FFX/RepositoryEvidence/historical-install/dylan-followup-20261018/deployment-verification.json).
Offline hosts execute production adapters/state machines with retained original
script/layout data where available. Controlled actors, timers, SDKs and geometry
are labelled in those fixtures. They do not launch the game, establish an actual
DualSense response time, or prove an audible cue's practical reaction window.

## Optional minigame completion 0.6

These checks apply to the local0.6 build, not the preserved0.5.9 ZIP. Native
component checks cover all nine retained script variants, real typed setters
and original finish-entry initialization. They do not prove the complete game's
animations, story cutscenes, prizes or usable/audible response. Test these first
when progression permits; no save reset, warp, capture or special tooling is
required. Enable each saved default-Off option in **F8 → Miscellaneous** before
starting; the matching row appears on its preparation map/team screen. Close F8.
The same controls are available through the controller mod menu.

1. **Calm Lands:** enable **Chocobo training auto-success**, choose each available
   training course normally, and confirm its result dialogue. Expect completion
   after the actual start, with Catcher's perfect result and normal reward gate.
   Failure: no finish, training remains locked after success, or Catcher displays
   zero but lacks its native perfect reward when eligible.
2. **Remiem:** enable **Remiem race auto-success** and start normally. Confirm the
   first-win reward, then repeat for each unearned one-to-five-chest reward.
   Already-owned tiers should be skipped; all-owned repeats use five chests.
   Failure: poles penalize the result, lower separate prizes remain inaccessible,
   duplicate unique prizes appear, or mounted/control cleanup does not finish.
3. **Both butterfly courses:** enable **Butterfly hunt auto-success**, touch the
   ordinary starter and open the game's chest. Expect seven pickups, the normal
   timer/cleanup and that course's current story-phase reward. Repeat on the
   second course when available. Failure: count8, no chest, wrong phase prize,
   or an inaccessible/unopenable chest. Early and later phases are separate
   progression-dependent checks; do not advance solely to satisfy this test.
4. **Blitzball:** enable **Blitzball auto-win** at team setup. Setup, movement,
   formation and cancelled choices should remain normal. During the live match,
   confirm any actual **Action Commands** option. Expect the native end-match
   win and normal results/prizes/standings. Check story and later repeat matches
   as available. Failure: a setup/cancel choice triggers a win, halftime loops,
   the opponent wins, or normal story/results/prizes fail to advance.
5. **Manual control:** turn each option Off before another attempt. Expect the
   normal activity. Close/reopen F8 to confirm each choice persists independently;
   cue settings should remain independent. Changing maps, opening menus or
   entering battle must not apply an old queued completion to another activity.

Record which activity/course/story phase failed and the last native result or
spoken message. Existing own-session logs may diagnose that producer; runtime
probes and tester submissions retain their separate permission requirements.

## Prioritized checks for the current repairs

The user tested Sea of Sorrow on October20 and confirms the repair works.
Later Sin areas after Seymour were not tested; the user leaves those checks to
a tester. Native ledge animations still stop Auto walk; request it again after
control returns. The four optional0.6 minigame assists are implemented. See the
[implementation and native evidence](puzzles/minigame_completion_20261020.md);
prioritize the checks below as progression permits.

Nucleus collection's Sin-only saved pre-arm repair is implemented, offline-
verified, guarded-deployed and packaged in refreshed0.5.9. When progression
permits, use **F8 → Miscellaneous → Sin object collection → Collect objects automatically:
On** on any Sin map before The Nucleus (City or Tower are suitable), close F8
and enter. It should remain On after a crystal battle and both Nucleus room
variants should offer the menu. Outside Sin the entry should be absent.
Confirm the game's normal reward
dialogues; **Collection status** should reach10 and the game's normal next-area transition
should follow. Set it Off to restore ordinary pickup behavior. No reset or
capture is needed. See the [collection implementation and limits](puzzles/sin_collection_20261020.md).

These checks are user-side, when progression permits. Do not reset progress or
load the downloaded save merely to satisfy them. The verified0.5.9 release ZIP
contains these repairs; its [receipt](../../install/nucleus-prearm-20261020/release-verification.json)
separates native/offline checks from pending gameplay.

For the local console-enabled Baaj build, close dialogue in the field, press
Shift+F1, enter `warp 196 0`, then Enter. Check the six Antechamber seals in the
Object list. The chord must never toggle turbo, including held/released keys;
plain F1 keeps the game toggle. [Offline evidence and bounds](runtime/console_f1_20261020.md).

First for the October20 audio repair: enter Free Search from each available
airship menu, pause, move, press M, exit and reopen. The loud repeating mod loop
should remain absent and M should read coordinates. F8 Audio previews should
work again. [Evidence, exact scope and limits](puzzles/airship_audio_20261020.md).

The October20 Baaj/audio/key/password repairs are now **guarded-deployed**.
These fixes are packaged in0.5.9. First when progression permits,
Baaj's Object list should include all six named temple seals. Walk into an
active seal without Confirm; native activation should mark it inactive in the
list. Inactive seals must remain identifiable, and stale routes must cancel.
The installed Baaj slot0 exactly matches the supplied save. The user now
confirms Besaid correctly starts/remains inactive because its destruction-sphere
treasure is missing, and every eligible seal works. The user accepts the result;
the Baaj issue is closed. No additional diagnostic list request is required.
Final door opening after all six treasures remains untested in this save and
is not required to close this report. All64 flag combinations pass both offline
availability proofs; labels match original temple writes.
[Baaj source/native checks and confirmation](navigation/baaj_seals_20261020.md).

For Cid, select him in the airship NPC list, move around, and refresh the list.
His name should remain Cid and his distance should follow his current position.
Wakka must remain a separate actor. A present Cid with unavailable conversation
may be labelled inactive; he should remain listed. Offline tests cover the four
retained Cid variants and actual selection transfer, not gameplay confirmation.

In each actual Free Search entry path, the default-Off hidden destinations
setting enables **Shift+M**; Off/closed search must release it. On the password
keyboard, N should say Password; ordinary naming should say Name. On the field,
the native F1–F5 toggles must still work and F9–F12 should be available to the
game; F6–F8 remain mod commands. Guide F9–F11 must work in supported puzzles and
release on exit. No progress reset is needed. [Key/password implementation](puzzles/airship_keys_20261020.md).
The [status register](status.md) distinguishes offline and gameplay evidence.

1. Sphere Grid: choose a path or ability and move the preview cursor several
   nodes. LT/RT should describe backward/forward from each newly focused node.
   View/Select then A/B (Cross/Circle) should match F4/F5 from the character's
   position. Repeating the same character-origin instruction while only the
   preview cursor moves is intentional. Returning a previous cursor node on
   LT/RT, or executing a field command after closing the Grid, is a failure.
2. Gagazet Summit after the boss: active navigation should include Yuna's sphere.
   Interact normally, then check the onward exit. It must disappear or become
   unavailable when the game hides it. The label repair changes no story flags.
3. Dome — The Beyond after Yunalesca: navigate to Staircase 1 or Staircase 2 and
   perform the native interaction. The original chest/exit remain available as
   their native gates allow; chest contents are not disclosed by navigation.
4. Manual Wakka target: switch automatic solving Off and puzzle cues On. Listen
   through a couple of passes of the targeting beep, then begin throwing before
   the beep so the ball arrives when the gaps align. No calibration throw or
   countdown is required. The beep marks alignment, not the launch moment.
   Off, leaving the trial and focus loss must suppress old cues.
5. Airship destination list: check initial destination, native moves and scrolling,
   New location, closing while output is queued and reopening. Only the focused
   displayed label should speak in the actual UI language. The October20 fix
   restores previews and independent events while rejecting invalid field and
   minigame loops in Free Search. Destination list and password entry are now
   user-confirmed. Y necessity/control is documented for later investigation. N reads entered password
   text; F8 Speech options -> Speak punctuation names is Off by default. On
   names highlighted symbols; Off keeps raw output. Native TK owns prompts and
   confirmation prompts. Latest user report suspects the Proceed? repair targeted
   the wrong prompt; the original silent options remain unresolved. Work is
   deferred at1% usage. On explicit resumption, establish the actual prompt and
   producer before another fix; layer29's offline tests do not establish this.
   [Image option flow/offline evidence](puzzles/airship.md#proceed-confirmation-options).
   [Current repair/evidence](puzzles/airship_password_20261019.md).

## Coverage reused from current records

The user/tester confirmed all six Cloister solvers, Bevelle bonus treasure,
Macalania normal story exit, Help page buffers, Gagazet automatic solving/returns
and cross-water routes, and restored controller passthrough responsiveness. New
reports override those confirmations. Their exact controls/native dependencies
and remaining save/state combinations remain in the subsystem evidence.

[Existing gameplay procedure](../gameplay_acceptance_0.5.6.md) accumulates the
broader unconfirmed race/hunt, speech, navigation, battle, Help and installer
checks. Its versioned historical results do not establish new gameplay coverage.
Use current [controls](../controls.md), [feature topics](index.md) and later
corrections when a historical step differs. UI translations beyond native getter
text and HRTF output are documented implementation gaps, not already verified
features. The current task does not add them by implication.

## Offline test entry points

From the source root, run `src/native/tests/run_tests.bat` with the affected
switch. Input/controller/SDL owner/Sphere/native readers, utilities, navigation
audio, route and reachability hosts cover separate boundaries. The subsystem
topics list exact switches and fixture files; additional standalone worker hosts
have their own `run_*_tests.bat` entry points in the same directory. Do not run
two modes that overwrite the same external `FFX_EVIDENCE_ROOT/test-build` output
concurrently. Inputs and generated original bodies must remain outside Git.

For the current fixes, input, controller, Sphere, SDL owner, both reader profiles,
both utility flavors and navigation audio pass. The Gagazet audit replays the
retained 30-stage ring schedules and MGRP animation evidence. Native proxy build
and guarded deployment verify compiled profiles, all seven installed hashes and
locked PRISM runtime. The INI and 0.5.7 package are preserved. Exact logs, component
measurements and simulated dependencies are linked from the repair record.

Run `python Docs/Scripts/Offline/audit_feature_docs_20261018.py` after source/docs
changes to regenerate source/settings maps and verify current local links. It
checks143 module assignments and37 settings, retaining hashes for future reuse.
Its comment counts are inspection aids; actual intent, ownership, safety and
source agreement require review.


## Later Sin obstacles: current source follow-up

Read-only City/Nucleus map extraction was explicitly approved and the real floors pass offline native collision replay. Four City moving-platform boarding stops pass exact native rectangle routes and both reader profiles, and the warning-free current-source build is guarded-deployed. When progression permits, check the City Object list for active platform stops, request one route and walk into its boarding region. Let the native game perform the ride, then request the next route after control returns. Failure symptoms: an active stop is absent, the route does not reach its boarding rectangle, or stale steering continues after native movement takes control. The Nucleus floor replay confirms all native floor components and that the count10 warp remains separate progression; full ride/spawn-controller, gameplay and whole-game timing remain untested. The canonical real-floor host is `--sin-later-maps`; native/owned target discovery is in `--readers`. See the [evidence and limits](navigation/sin_late_obstacles_20261020.md).

## October21 speech repairs

The staged [HP follow-up](battle/hp_sources_20261021.md) also needs these checks
after deployment is released: after a character regenerates or receives healing,
press H again and compare with Shift+H after native number animation settles.
Reopen Change and highlight Wakka; the HP proportion/optional maximum must follow
the game's effective battle capacity, including any active capacity increase.
Stale pre-heal HP, totals above the native maximum, or a base maximum spoken
instead of a real raised maximum are failures. Percentages remain enabled under
the existing setting. These checks do not require repeating Seymour or learning
a new mode first; test HP when the next normal battle permits.

After the verified repair build is deployed, test these when progression permits:

1. Open Formation. Each member should include active party or reserve. Swap one and revisit the same member; membership should update. I should retain the character's native description.
2. Open Spare Change, change an amount and move its digit. Hear the gil amount and decimal step on entry/change. Cancel safely. Yojimbo uses the same input reader.
3. Open an obtain-items, unlock-skills or max-gil confirmation. Hear its prompt then No. Move between choices and hear Yes/No; cancel without applying it. The prompt should remain available with I.
4. In Seymour Omnis, select each Mortiphasm. Hear its current energizing element and physical-left/magic-right mechanic. After turning a wheel, revisit it and Seymour; hear the new element and aggregate wheel mix. Missing/reversed/stale colors are failures. Native animation timing still needs gameplay confirmation.
5. When a character naturally learns an Overdrive mode, hear that character's actual name and mode in the complete native sentence. Rapid learned messages must keep their own names; repeated events must remain audible.

[Native design, bounds, original-data fixtures and exact evidence](battle/seymour_amount_prompts_20261021.md).
These fixes are separate from the preserved0.5.9 installer and remain subject to the existing package hold. Callback/queue fixture checks do not establish game-frame-to-audio latency.
