# Free Search looping audio repair, October 20

Latest delivery: the user released the deployment hold when requesting the Baaj
fix, then deployment. The combined current-source build passed canonical checks
and guarded deployment. [Baaj/current installed receipt](../navigation/baaj_seals_20261020.md#delivery-bounds-and-review)
supersedes the historical held-build status below. Installer/package remains
held; actual gameplay/audio confirmation remains pending.

The user reports loud repeated mod audio only during Free Search, despite the
October 19 blanket mute, and requests a producer fix followed by restored audio
on every entry path. Treat this as an unresolved gameplay report. Do not infer
that the user is hearing native game audio from the absence of a dedicated
Free Search sound function. This work supersedes the temporary blanket mute in
[the October 19 record](airship_password_20261019.md#release058-temporary-audio-exclusion).

Implementation status for the reported defect: **in progress (~90%)**. The
reproduced mixer-toggle/generation defect and context cancellation are repaired
and offline verified; the exact audible producer in the reported session remains
unconfirmed. Gameplay confirmation remains separate; the combined build is now
guarded-deployed with Baaj, while installer/package rebuilding remains held.

## Findings and intended sound ownership

`airship_search.cpp` reads displayed coordinates on M and optionally steers to
an approved destination. It does not produce audio. The shared pickup sound in
`nav_audio.cpp` is a continuous 480 Hz sample for Catcher Chocobo balloons or blue
butterflies, with stereo position and proximity volume. Field beacons indicate
routes, walls indicate nearby collision, and red butterflies have their own low
warning. None of those producers has a valid target in the Free Search cursor UI.
Audio previews and actual timing events retain their separate controls.

The old pickup mixer compared two epoch values from the old sound record, so
that test could remain true after a resource-generation change.
It also never checked Race pickup cue or Butterfly cues while rendering the
accepted pickup. Turning a switch Off depended on a later producer refresh or
pause command. A production SDL dummy-stream regression fails on the old source
at `race toggle retires an already mixed continuous pickup without a producer
refresh`. This proves a mod-side lifetime/toggle defect. It does **not** identify
which exact sound/producer was audible in the user's session; that attribution
and the reported sub-10-ms recurrence are not runtime measurements by the agent.

## Native context, data flow and lifetime

`airship_map.hpp::airshipFreeSearchActive` uses the same primary `hiku2100` header
and search flag as M's coordinate reader. The bounded native reads qualify code
length24785, workers41, global offset150F8, code offsetE6C, name `hiku2100` and
the active flag at chunk+15248 (global+150). `current` in the coordinate reader
now reuses this predicate after checking its copied token/lifetime. Its existing
worker checksum qualification remains in force; the lightweight predicate does
not replace that checksum for steering or coordinate speech.

Original retained hiku2100 W9 sets map15248 to1 at PC4734 after showing Free
Search, requests W2 entry6, and resets it to0 at PC4778 on exit. W2 reads the
cursor through Common00A3 at PC05A8 after the native wait. Both UI entry routes
are handled through that shared active state, without enumerating menu choices
or guessing a second room ID. `airshipMapActive` still reflects original
91AB30's room382 renderer special case for image reading. The field predicate
`airshipNavigationUiActive` accepts either room382 or the actual Free Search
state; absence of that renderer room value cannot admit field loops during M's
active coordinate context. Native getters/pointers stay on the native owner.

At the existing PInput return hook RVA225930, `navigation.cpp::capture` publishes
setFieldAudioAllowed using not airshipNavigationUiActive. Movement-blocked checks,
copied navigation scenes and wall capture use the same predicate. The predicate
performs fixed header/name/flag reads; it does not scan a worker table, hash a
script, allocate, open a device or wait. The UI cursor cannot become a field
route/wall observation. Walls cancel before geometry capture.

`setFieldAudioAllowed` publishes an atomic permission and invalidates field
context, pickup/hazard serials and accepted voices on **both** entry and exit.
Only transitions enqueue a bounded copied `field-audio permission` log. On its
exclusive audio owner, `retireFieldVoices` retires route state, pickup/hazard,
arrival/beacon samples and wall gains; it preserves preview/timing voice state.
Rejected field publications do not occupy the audio FIFO. The mixer independently
checks permission, current generation and the source's category toggle. Blue
butterfly commands carry Butterfly cues; race/exported pickup commands carry
Race pickup cue. An Off observation retires the accepted loop, so On alone
cannot resurrect it. A fresh valid producer event is required.

There is no full-airship zero-output branch or suppression of all mod audio.
F8 beacon/wall previews work there again; event-specific timing sounds retain
their normal gates. This does not add a new automatic search tone or enable
navigation sounds that have no search target. Existing user settings/defaults,
native game sounds and the documented deferred Y-coordinate question are
unchanged. No new user-facing strings, settings, assets or dependency are added.

## Bounds, extension and verification limits

The existing command queue holds64 fixed records, drains at most64 per callback,
and emits at most48000 stereo frames per callback in512-frame chunks. The audio
callback reads copied atomics/settings and owned samples only. Native context
publication never waits for the mixer or a worker. New continuous producers
must supply their actual category and remain cancellable on native context and
resource changes; do not restore a blanket mute or compare two stale epochs.

`nav_audio_fixture.cpp` drives the actual production callback through SDL's dummy
device. It covers accepted-loop toggle Off/On, generation change without reset,
independent race/blue-butterfly switches, search entry after command drain,
repeated rejected publications, route/wall/arrival/pickup/hazard exclusion,
restored previews/timing, and no replay after exit. `airship_search_followups.inc`
loads the retained original hiku2100 EBP and checks the production native PInput
permission boundary with the real search flag under room382 and a controlled
non382 renderer value. The latter is an adversarial boundary fixture, **not** a
replay or runtime confirmation of the second user menu route. Existing original
native image/choice replay, M coordinate speech and keyboard/controller movement
tests remain in the full reader fixtures for both supported executable profiles.

The scoped changes introduce no game-thread loop, sleep, lock or worker join.
Dummy-stream tests establish generated-sample behavior, not hardware output
latency, actual game callback cost or end-to-end audible response. Native script
scheduling, SDK input/render behavior and both actual menu routes remain
user-side gameplay checks. No game launch/control/probe, installed game asset
read or Tester Logs access is part of this investigation. The user's own
October20 log and matching deployed DLL hash were read; that log does not name
an audible category. Installer/package rebuilding remains on hold.

## First gameplay check

Enter Free Search through each available airship menu, stay still, move the
cursor, press M, then leave and reopen. The loud repeating mod loop should stay
absent; M should read the current coordinates. F8 Audio beacon/wall previews
should still play when requested and stop normally. Ordinary field audio should
follow its saved switches after leaving. A returning loop, broken coordinate
readback, or replay after exit is a failure. No progression reset is needed.

## Local delivery status

The canonical `src/proxy/build_dinput8.bat --release --no-deploy` completed with
exit0 and no warnings. Both171-site reader profiles pass (the reader fixture
retains its pre-existing signedness warning in airship_runtime_followups.inc).
Production SDL/audio and route hosts pass, as does the updated real-Surface wall
host, including cancellation before geometry capture with the search flag and
no renderer-room special case. The documentation audit maps all141 production
units/32 settings/15 current topics with no broken local/source links.

The user explicitly said **hold deploy** after this local build. No deployment,
installer rebuild or packaging was performed. The installed reader remains the
prior build; the staged repair is not available in the game yet. The canonical
deploy must wait until the user releases that hold, then recheck closed game
and launcher processes. Local evidence and the verification receipt are retained
in `install/free-search-audio-20261020`. Both actual entry routes and audible
behavior remain gameplay-unconfirmed; no actual audible producer attribution
is claimed from these offline results.
