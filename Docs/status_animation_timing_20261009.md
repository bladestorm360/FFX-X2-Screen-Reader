# Per-target buff animation timing — 2026-10-09

Sol investigated, implemented, reviewed and tested this work alone. The user
approved offline reading/copying the buff/debuff animation DLLs. No game launch,
runtime probe, tester-log access or status/gameplay mutation was performed.

## Native behavior and evidence

Cheer changes its counter during command calculation, before the action banner.
`78E680` calls `78D7F0`: the six counters at actor+65E through+663 use the native
command's buff mask/amount and cap at 5. `78DBA0` copies ordinary statuses into
result packets; these early counter changes require separately retained deltas.
`78F0B0` consumes each target's actual packet. `791230` calls `78DA40` to finish
unconsumed packets after an animation completes.

The animation has a separate, ordered target list. Cheer `magic_0003`'s EgoTask
creates each target's particle/sound effect, spacing targets using its own task
state. The per-target API+3EC call passes the character ID to native `795A30`
while preparing that effect's scale. It is inside the target-creation branch,
not a general per-frame renderer update. That call does not alter Cheer.
All six stack-buff modules have this independently confirmed creation cue:

| Native buff | Overlay | Return RVA of target creation query |
| --- | --- | --- |
| Cheer |3|6744|
| Aim |6|6857|
| Focus |4|675B|
| Reflex |5|675B|
| Luck |7|672A|
| Jinx |8|672A|

The main animation API is `C64CE8`, supplied by `9DA7D0`. API+3BC directly calls
`78DA40`; API+6F8 calls its `92E9A0` wrapper. Haste/Slow/Shell/Protect modules
have per-target API+3BC application calls during their animations. Existing
shared native status playback already handles these actual mutations, including
buffs/debuffs/removals. Cheer/Aim/Focus/Reflex/Luck/Jinx have visual creation and
API+358 finish calls without per-target API+3BC calls. Bio and delegated effects
must not be assumed to share either timing without their actual application path.

Retained evidence:

- `Docs/Scripts/Offline/output/status_animations_20261009_171247_874998/manifest.json`:
  approved 17 DLL copies, exact hashes and retained command-kernel overlay IDs.
- `Docs/Scripts/Ghidra/output/FFX/status_animations_20261009_171247_874998/`:
  complete headless decompilations, including six EgoTask bodies.
- `Docs/Scripts/Ghidra/output/FFX/status_animation_callbacks_20261009_174301_804529/`:
  additional seven modules' dynamically assigned callback bodies.
- `Docs/Scripts/Offline/output/status_cues_20261009_180944_919737/manifest.json`:
  exact call-site bytes, PE identities, relocation operands and owner/API globals.
- `Docs/Scripts/Offline/status_animation_cues.py` regenerates the six layouts
  from retained copies without reading the installation or executing DLLs.

## Production ownership

`status_animation.cpp` verifies loaded PE ownership without Windows loader APIs,
exact x86 header/size/timestamp, exact relocation-rebased code at the specific
caller, the API's verified native getter, and the actual effect owner. No file
hashing, disk IO, loader calls, new DLL callback writes or polling occurs there.
An unfamiliar cue leaves the existing native completion path available.

`battle_status.cpp` joins this visual event with the copied calculated delta,
current active target identity, source character, action serial obtained through
native `7B09C0`, and the current unconsumed result packet in one of the target's
two native buffers. Only that module's actual changed buff counter is announced.
Calculation copies the sequence from native counter word 3, passed as 78E680's
argument 9. Verified caller 7893A0 assigns the target buffer's source/sequence
header only after 789800/78E680 return. The fixture now reproduces this order:
reading the header at calculation fails, while the copied native sequence works.
Capped/unchanged effects remain silent. A packet records which buff was spoken;
completion retires that packet and still announces other unapplied deltas.
Repeated identical text on another target/action is retained. This is packet
ownership, not text deduplication. Battle reset clears pending records.

The native state and the animation's timing/order remain game-owned. Speech
queues through the existing damage/status producer; NVDA's spoken duration can
still lag the visual sequence if earlier queued speech takes longer to finish.

## Verification and acceptance

Both executable-profile reader fixtures pass. All six actual DLL images were
mapped/rebased as data only. Three party target events are separate; repeated
calls, changed owner/sequence/code/API, foreign thread, capped effects, encounter
reset, identical subsequent effects and unsupported-animation fallback pass.
New main-code signatures pass the central profile resolver fixture and retained
archive uniqueness check. Build/delivery hashes are recorded in
`install/status-wakka-followup-20261009/verification.json`.

Gameplay timing remains user-side: use Cheer and listen for each target's count
during its own animation. Aim/Focus/Reflex/Luck/Jinx use the same qualified
mechanism; other statuses retain their actual native application event.

Final delivery: both tester/developer native builds pass; a fresh final proxy
passes the offline forwarding fixture. Guarded deployment validates closed
processes, compiled FFX/Will profiles, launcher bootstrap and all 13 installed
hashes. The separately verified tester EXE is `install/windows-installer/InstallFFXScreenReader.exe`, SHA256 `e31263e2c30ab5a10df4d61a805951bbe1eb77d7b109e63dddb6eb030c08518f`.
The existing release ZIP is unchanged; gameplay acceptance remains user-side.
