# 0.5.6 deployed tester build — gameplay checks

The user confirms Help menu speech is restored; Help information page buffers still fail in gameplay. This is a known issue, deferred at the user's request. Installer compilation is now authorized with that limitation documented. The intended page expectations below remain future repair acceptance criteria, not confirmed behavior.

Final opening/Escape/Help correction is deployed. Remaining checks:

1. In Help, move through Basic Terms and Reference. Names should speak again.
2. Open CTB Window. Expect its heading; I reads the complete page. Try F7 and
   comma/period review. Basic Controls also includes its English diagram labels.
3. In Escape, check Special Features names and I for the displayed help.
4. On the next fresh launch, hear the autosave warning and then the advance cue:
   B in the current keyboard mapping, Menu/Start on controller.

The installer hold has ended. The earlier broader procedure follows.

October14 menu/results follow-up: installer compilation authorized. Prior confirmations
remain valid. Prioritize these checks on the newly deployed follow-up payload:

1. After a battle, review AP/Sphere Level/Next AP. Press Confirm to finish the
   rolling counter, then I: values should match the completed screen. Repeat for
   gil/wallet and items. The buffer must change phase and keep its row on refresh.
2. In Status/Aeon Status, change every page and character. Empty pages should
   say No learned abilities. I must not invent an undisplayed skill description.
3. Browse blank Key Item/Overdrive View slots: expect Empty. Populated entries
   keep their names; owned Key Item help and working mode help remain available.
4. Browse Help menus and information pages, with F7 Off and On. Expect the
   actual heading rather than Dummy; I reads the complete page. Comma/period
   review visible paragraphs; native menu arrows must continue navigating.
5. Open Escape from both a map and the field menu. Expect option names on
   highlight, current Audio/Video values, and visible help with I or F7 On.
   Special Features must name Turbo/Supercharge and other actual features.
6. On a fresh launch, hear the autosave warning and then the configured
   Cancel/Start instruction while the opening/intro sequence proceeds.

All new gameplay and actual speech/backend responsiveness remain unconfirmed.
Offline/source verification and deployment provenance are recorded in
[menu follow-ups](menu_followups_20261014.md). The older general procedure follows.

The final current-source implementation, offline checks, canonical native/launcher
build and guarded deployment are complete. Installed files and the PRISM receipt
are verified. Use this procedure for user-side gameplay acceptance now; prioritize
the new opening instruction and Stone Gaze/related attacks (step10) when available.
Jecht manual prompts, final auto-success and taunt suppression are user-confirmed;
the Jecht installer-test condition is fulfilled.

Sol verifies source, ABI, dependencies, installed files and package integrity.
The user confirms gameplay and actual speech/braille delivery. No code or
architecture review is required. Controls are in [the player guide](controls.md).
Record an unreachable scenario as untested, then continue the reachable checks.

1. **Launcher, speech and input response.** Start through the ordinary launcher.
   After the opening notices, expect the configured Cancel/Start instruction:
   press either for the title menu, or wait for the intro cinematic. The cue
   should not wait for movie playback or recur during ordinary gameplay.
   Move through its choices, then open FFX's title/settings menus. Each accepted
   choice should speak promptly and ordinary controls should keep responding.
   Offline: x86 proxy/launcher builds, dependency/ABI and fake-backend checks.
   Remaining: actual launcher/game integration and audible backend delivery.
   Report silence, wrong backend, freezes or repeated unrelated messages.

2. **Settings, repeat and details.** Open F8 settings directly with the keyboard.
   Change Auto detail in Speech, return to the game, and use I on a current item
   description. Off keeps manual details available; On reads the new description
   after its focused label. Test the documented Repeat action twice. Each request
   should work, including equal text, and Escape should return native input.
   Offline: owned text/detail ordering, settings and context fixtures.
   Remaining: actual delivery, cancellation and saved preferences after restart.

3. **Status and all five Sphere Grid Status pages.** Use normal menu controls to
   open Status; review with the guide's arrows/comma/period and I. In Sphere Grid,
   open Status and cycle its five pages with V. With Auto detail Off, each page
   announces its character/page and keeps its displayed rows available manually.
   With On, the current full buffer follows the headline. Close it and verify
   normal Grid controls return. Report missing rows, a wrong character, stale
   details, full automatic reading when Off, or input remaining captured.
   Offline: private-window events, actual stat/command data and both profiles.
   Remaining: gameplay still needs
   native presentation/input ownership confirmation.

4. **ESC, inventory, equipment and shops.** Change a native audio/video setting,
   visit both keyboard binding columns and a controller binding, and cancel a
   confirmation. Browse equal item/ability names and buy/sell quantity choices.
   Labels, columns, quantities/prices and current descriptions should stay in
   order. No cancelled or undisplayed confirmation should speak. Equipment
   details should match the current and selected panels. Offline: both reader
   profiles plus real held-worker/source-reuse/ordering fixtures. Remaining:
   actual renderer/getter integration and rapid input under backend load.

5. **Airship menus and naming/password.** When the Airship is available, browse
   List/Search and their destination choices, then inspect current help with I.
   Test ordinary naming/password input using native controls. Announcements must
   follow actual focus/readiness; do not expose masked or unrevealed text.
   Report missing options/help, stale text or a freeze. Offline: retained renderer
   evidence, owned naming/help and profile fixtures. Actual Airship presentation
   and the earlier intermittent failure remain unconfirmed.

6. **Navigation, moving targets and cancellation.** In a familiar field, browse
   targets and request directions. Repeat rapidly, cancel, move manually, and
   change menus/map. A useful response should arrive at normal play speed; old
   targets/routes/audio must retire. Test a moving NPC and a changed door/gate
   when available. Enable Auto walk only for a familiar safe route; manual input
   must take priority. Spoken path counts stay concise and follow the selected
   target. Offline: real script/geometry/state fixtures; the real preparation-thread pipeline, valid maximum geometry, source reuse,
   cancellation and aged-input refusal pass controlled-host latency checks. Actual
   game/backend latency remains unconfirmed. Report seconds-long collection,
   stale directions, a wrong floor, continued movement or delayed cancellation.

7. **Known swimming and progression scenarios.** When saves/progression permit,
   check the Salvage Ship post-jump south passage and its destination exits; in
   Via Purifico check both underwater progression exits, depth changes and save
   points. For the applicable trials, verify Kilika's continuation and the chosen
   Macalania completion/fayth destination. Offline: retained real maps/scripts,
   actual Via86-triangle geometry and native context/arrival guards. Remaining:
   actual progression, collision and route/interaction handback. Report the
   starting location, selected target/action and the wrong destination/omission.

8. **Controller and navigation audio.** Use the guide's controller mod mode,
   settings, details and direct pathfinder actions. Try focus loss/recovery and
   disconnect/reconnect. Wall/beacon controls remain independent and cues should
   follow the correct direction/current target, then stop on cancellation/menu
   entry. Keyboard F8 must not activate controller mode. Offline: qualified SDL3
   owner, virtual-device/input and audio fixtures. Remaining: actual hardware,
   driver/backend timing and native input handback.

9. **Jecht Shot confirmed.** The user confirms final auto-success works perfectly,
   including the centered tutorial prompt and slower eleven-answer pacing.
   Manual control prompts and voiced-taunt suppression are also confirmed.
   Assistance remains optional and defaults Off for fresh installations;
   current saved choices are retained. Repeated prompts are separate events.
   The preceding trigger label remains a separate navigation gameplay check.

10. **Stone Gaze and similar attacks when available.** Confirm the attack name,
    applied petrification/status feedback, and the native Miss or Guard/Immune
    output on unsuccessful attempts. Equal separate attacks should still speak.
    Report the ability and actual outcome if an announcement is absent or wrong.
    Offline shared producers/status packets/displayed popup labels pass both
    profiles; individual ability gameplay remains unconfirmed.

For responsiveness checks, note the action, starting location/page, cold or warm
state, approximate delay to usable/audible feedback, cancellation delay and any
native input/frame/load hitch. Compare only comparable saves/settings/cache
conditions with an available known baseline; do not restore an older build.
An unavailable unmodded or feature baseline remains an explicit measurement gap.
Sol will correlate the user's own-session diagnostics; Tester Logs require
separate issue-specific permission. Do not run a broad capture or technical
probe for this procedure. Stone Gaze and related ability speech are authorized
for implementation; confirm them in gameplay here after deployment.
