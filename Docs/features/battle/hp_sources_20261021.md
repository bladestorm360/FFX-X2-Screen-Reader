# Battle HP/MP source repair — October21

Status: complete for implementation/offline verification; both173-hook profiles,
original capacity/renderer and real status-worker checks pass. Warning-free
release build and both installed-executable image preflights pass.
The user released the hold and requested final1.0. The current1.0 native build
is guarded-deployed and its verified installer/ZIP is published as Latest in the
fresh public repository; see Docs/release.md. This follow-up includes the previously verified
[speech repairs](seymour_amount_prompts_20261021.md).
Logs, source/test freeze and seven-file verification receipt are retained in
`install/battle-hp-20261021`. The previous receipt remains historical provenance.

## Report, design and source ownership

The user reports stale H totals after healing/Regen and an apparently low Wakka
maximum in the battle Change/Switch list. Preserve percentages and the saved
maximum-HP preference. Read native displayed/current values and effective battle
capacity; never reconstruct totals from damage/healing announcements or infer a
character's capacity. Percentage arithmetic represents the health proportion,
using the same native current/effective maximum pair. It does not calculate HP.

The current own-session log is beside FFX.exe, not under logs. The real host view
is necessary; the sandbox view was stale. At12:32:40 Wakka healed4584, but H at
12:32:42 and47 still said34. At12:45:35 the rendered HUD said1498/4618, a heal4618
followed, and H at12:45:44 and52 said6116. Another target said9236/4618. These
are contradictory producers, not evidence that a damage popup or H forecast is
an authoritative maximum. The user's separate Switch maximum concern remains
part of the gameplay check; do not dismiss it based on an earlier solved note.

The original getter/render/mutation bodies distinguish these native fields:

| Value | Actor offset | Native evidence / consumer |
|---|---|---|
| Committed current HP / MP |5D0 /5D4 |79ADE0 /79AE60; Change row892C20; normal playback78E2F0 /78E400 |
| Effective battle maximum HP / MP |594 /598 |79AE00 /79AE80; HP color/native HUD maximum; playback clamp |
| Base maximum HP / MP |59C /5A0 |78D330 retains these when changing effective capacity |
| Action-calculation scratch HP / MP |6E4 /6E8 |78E680 calculation; can be stale or exceed effective capacity |

All addresses in this table are reference-image VAs. Production RVAs remove the
400000 image base and resolve through the existing verified profile. The native
actor getter794030 uses the actor ID and F90 stride; the installed mapping is
394030→394020. Fahrenheit's historical current_hp/current_mp names for the
scratch pair were misleading; the original bodies determine semantics.

The Change row892C20 calls79ADE0 for its HP number and79AE60 for MP. It passes
the current HP and79AE00 maximum into78BFC0 for its health color. Effective
maximum594 includes native capacity changes: original78D330 changes it from the
independent base59C when HP capacity doubles, applies the native limit, and clamps
current HP. No mod-side doubling, cap, damage subtraction or heal addition is
permitted. The row's separate bar892880 is Overdrive; its fill is not an HP
capacity source. Numeric maximum presentation remains the existing user setting.

## Production flow and safe extension

`battle_status.cpp::queueBattleActiveStats` previously copied RAM+1A4/+1A8
(actor6E4/6E8) for H. It now copies RAM+90/+94 (actor5D0/5D4). Effective maxima
RAM+54/+58 remain native. `battle.cpp::targets` uses the same committed fields
for ordinary enemies and party HP/MP previews; Sensor already used5D0. The
forecast previously generated false current totals above capacity and stale
values after restoration. Damage/healing speech continues to use the accepted
native popup amount, independently of the totals reader.

`ready_game_text.cpp::planBattleSelectionVitals` and the synchronous manual-detail
counterpart in `game_details.cpp` already read5D0/594/5D4 for Change. Their source
comments now explicitly distinguish effective594 from base59C. They read each
highlight/request afresh and have no fixed Wakka capacity or saved-field maximum
fallback. The actual native maximum is used even above6000, including native
doubling and capacities above9999. Existing cursor-event guards remain; there is
no new text-content deduplication or timer. Selection I uses the same sources.

H captures the current turn owner; Change captures the highlighted native row,
including inactive initialized reserves. These identities must not be conflated.
Both producers read bounded native values on the validated native/input owner,
copy names/control substitutions and numbers into existing owned packets/plans,
then hand off without waiting. Workers format percentages or maximum preference,
statuses and speech using copies. No actor pointer is walked off-thread, no new
hook/thread/queue is introduced, and reset/generation cancellation is retained.
Shift+H continues to report accepted drawn HUD operands, including native number
animation; do not replace it with reconstructed actor totals.

The source change replaces reads, with the same four-value H packet and existing
bounded target loops. No new producer allocation or background scanning is
needed. Existing worker/queue evidence applies to the unchanged transport;
reader-host measurements and real game/audio limits must remain separate.

## Offline evidence and remaining checks

Original game bodies, relocation records, native jump table and renderer constants remain outside
every Git tree at `Analysis/FFX/RepositoryEvidence/offline-output/battle_hp_20261021`.
The source-free export tool is `Docs/Scripts/Offline/prepare_battle_hp_20261021.py`.
Its source is the retained reference text/rdata blocks; no new installed battle
asset read, launch, game control or runtime probe is involved.

`src/native/tests/battle_hp_followups.inc` executes original263/94-byte HP/MP
playback,26-byte clamp,291-byte capacity update,22-byte display getters and
1071-byte Change renderer. Encounter context, eligibility, actor lookup, geometry
scaling, graphics submission and color submission are controlled seams; original
numeric getter calls/order and effective-capacity mutation are preserved. This
does not execute the Regen scheduler, full renderer or device/backend.

`src/native/tests/run_battle_status_worker_tests.bat` builds the production
status/name/ready-text/plan/selection/speech transport with W4/WX in a separate
external build directory. A held real worker receives three H captures: injured
329/4618 with7% presentation, restored4618/4618 despite unchanged9236 forecast,
and genuine6116/9236 with4618 base. Native names, actor memory and preferences are
changed/reused before release; the copied outputs remain correct and ordered.
All native capture allocation/free counters remain zero. Backend/actor getters
are controlled; this is whole owned-pipeline correctness and bounds evidence,
not an audible latency or game-frame performance measurement.

Checks cover stale9236 forecasts, damage/healing/overheal, zero/current/maximum,
MP restoration, preserved percentage output and28 native Switch capacity/state
pairs. A native doubled4618 base produces9236 effective capacity, then actual
6116 current HP: H, Change numbers, health-color inputs and manual details must
all use6116/9236, while the base remains4618. Removing the native capacity effect
must restore4618 and the game's own clamp. Test-only arithmetic/native mutations
do not introduce HP calculation or writes in production.

After the staged build is deployed when the user releases the hold, verify H
after regeneration/restoration and revisit Wakka in Change. If a native capacity
effect is active, the spoken maximum/percentage must follow its effective value.
Compare H with Shift+H once native number animation has settled. Missing updates,
an unchanged base maximum under a real capacity effect, or current HP above the
native displayed maximum are failures. Gameplay and frame-to-audio timing remain
unconfirmed. The new1.0 payload is installed; no game was launched by the agent.

Sol owns this HP investigation/implementation and original-native verification.
No HP delegation or Astra invocation. Earlier Luna popup/Formation work and Sol
corrections remain tracked in the linked speech-repair record.

Fixture corrections are retained honestly: the first new compile lacked the
ready-vitals declaration; the native renderer replay initially omitted its
indexed jump table; the installed-profile test initially addressed a window
member as an independent RVA. The fixture now includes the declared adapter,
relocates the original four-entry table and uses the qualified window base plus
member offset. These were test-host defects, not alternative production HP
formulas. Failed diagnostic logs are not counted as passing validation.
