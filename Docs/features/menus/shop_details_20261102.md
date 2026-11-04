# Shop descriptions and extra information — November2

Implementation, offline checks and guarded deployment are complete / **Closed** in
[the single scope ledger](../../plan.md#november2-shop-descriptions-and-extra-information--scope-ledger).
The earlier Mushroom Rock work is preserved and its private smoother remains Closed.

## Player contract

The native highlight still reads the selected equipment name and price. Its
primary description reads the selected modifiers/empty slots first, then the
compared character actually shown in the shop header. Autodetail reads that
description automatically; I repeats it with Autodetail either On or Off.

U independently reads the visible comparison pane: compared character, shown
HP/MP and currently equipped gear/abilities. It does not calculate stat changes,
read Sphere Levels or add other character statistics. The separate default-Off
Automatically read extra info option sits immediately below Autodetail in
F8 → Miscellaneous. It automatically reads the U information independently of
Autodetail. When both options are On, primary modifiers/name precede the extra
body; the already-read character heading introduces that body. Manual U retains
its full character heading and never replaces I's description.

| Autodetail | Automatically read extra info | On selecting equipment |
|---|---|---|
| Off | Off | Native name/price; I and U available manually |
| On | Off | Name/price, selected modifiers, compared character |
| Off | On | Name/price, visible compared-character/current-equipment pane |
| On | On | Name/price, modifiers/character, then visible extra pane |

Both manual keys permit repeated equal information as separate player requests.
U's existing field reachability binding remains available outside the qualified
shop context. Ordinary item-shop descriptions are unchanged. No comparison-delta
controls were added because the investigated renderer displays current/selected
ability panels, not native numeric/stat-change deltas.

## Report and displayed-information evidence

The actual host own log is35166bytes, last written November2 at08:41:51.
At08:39:54 it selects Baroque Sword,825 Gil; at08:39:55 and08:40:03 the old I
description reads Tidus HP520/520, MP12/12, Sonic Steel/First Strike, then
Strength +5% and one empty slot. Ductile Rod similarly reads Yuna's HP/MP and
current Rod of Wisdom abilities. This is the reported producer, not a guess
based on a tester log. The sandbox's older October23 view was stale; shared-
handle host access supplied the current example.

The user's screenshot confirms the visible header character/HP/MP, current gear
and its abilities, and selected Strength +5%. Native slot metadata/log gives one
empty slot; panel spacing is not a second slot. Snapshot/log copies and hashes
are outside Git under Analysis/FFX/ShopDetails_20261102.

The retained native renderer is
Analysis/FFX/RepositoryEvidence/offline-output/native_bodies_20261006_052640/FUN_008d85f0.txt.
Its1138-byte reference body qualifies comparison owner0..6, selects buy versus
sell equipment through the actual native getters, chooses the current weapon/
armor record, draws the current panel or native empty-equipment placeholder17,
then draws the selected ability panel. Native8D8A70/8F4F10 supply ability names
and normalized empty slots. Prior renderer analysis and the screenshot show no
generated Gain/Lose or stat-delta display. Original game implementations remain
outside every Git tree; this record contains source locators and behavior only.

## Production flow, thread and lifetime

shops.cpp keeps the existing native buy/sell/quantity hooks and selection key:
phase/row/depth, actual table handle and displayed comparison owner. The existing
ready_game_text.cpp equipment helper now builds only selected modifiers. Its
new compared-character and extra-pane helpers copy the separately qualified
visible values. Legacy game_details.cpp formatter parity also excludes character
vitals/current equipment from primary detail.

On a changed valid equipment selection, a bounded TextPlan uses receiver9 for
shop-owned output ordering. TextPlan's existing receiver/maintenance arrays have
one additional slot; its packet ABI, node/byte limits and other receiver contracts
are unchanged. Main I text uses the existing selection-detail token/cache. Only
the extra character/body strings live in the shop preparation-owned state.

The Native owner publishes a small atomic view with epoch, selection token,
scene, pane readiness and an opaque window identity. U is registered through the
existing qualified chord API, so it precedes but does not replace ordinary U.
Its availability and worker reply recheck that view/current detail token/scene.
It never dereferences the stored window. Another selection, phase exit, scene
change (game_context.cpp resets the detail token), or resource epoch retires the
old pane. A missing replacement record retires the old pane before getter failure.
Owned strings are formatted, cached and retired only on preparation.

Native capture uses existing ready getters/fields, bounded four-slot ability
records and128-node TextPlans, with no new native hook, scan, timer, wait or
worker-held native pointer. I/U review owned text without reentering character
getters. Cache scope is the current completed shop selection/owner/phase; spontaneous
changes outside those native presentation events have not been measured in-game.
Capture/command admission is one bounded attempt. A refused new selection clears
its availability, a busy U command reports a retry, and a missing prepared pane
asks for reselection rather than reading stale information.

## Setting persistence and checks

auto_extra_info is appended at setting ID37, default0/Off. All previous numeric
IDs and INI keys remain stable; only menu presentation places it below Autodetail.
Missing-key loads preserve the exact legacy INI bytes and default Off. The mod's
existing English-owned UI labels are retained; native names/abilities continue
through game-language getters. This does not claim full mod UI localization.

Focused native-reader cases exercise both buy/sell paths and every automatic-
setting combination,32 manual I/U repeats, displayed comparison owner differing
from selected gear owner, hidden-owner refusal, phase close and queued stale-U
cancellation. Native getter affinity is checked; manual rereads use owned text.
Both supported executable profiles run this matrix. Menu tests verify position,
default, independent persistence and unchanged other submenu contents. Legacy
setting IDs, missing-key behavior and all four saved toggle combinations pass.

The fixture emits separate eight-episode Native and capture-to-owned-output cost
measurements. Its getters, original phase mutation and queue/sink are controlled
boundaries; these are capped component results, not actual game/frame, PRISM,
hardware or audible-response guarantees. Actual phase/input/backend integration
and responsiveness still require player confirmation. No agent game launch/probe,
personal/tester save access, Tester Logs or delegation is authorized by this work.

## Completed source-to-output checkpoint

The warning-free canonical x86 release-flavor native/launcher build, both complete
173-hook reader profiles, menu/settings and input-ownership checks pass. The
eight-event shop matrix totals0.144/0.216ms on Native across the two profiles,
with maximum episodes0.020/0.043ms; capture-to-owned-output totals4.570/5.023ms.
These bounded fixture measurements include the related collection/queue/output
path, with the controlled boundaries above; they do not establish real game or
audible timing. No new hook, polling or native wait was introduced.

All seven installed/staged/scanned artifact hashes match. Microsoft Defender
4.18.26100.4, engine1.1.26100.4, definitions1.463.84.0 reported no detection on
the exact payload. Executables remain unsigned because no trusted signing
identity is configured; universal scanner acceptance is not claimed. The guarded
deployment passed with closed game/launcher processes and unchanged INI bytes.
The locked installed PRISM runtime was verified without replacement. Every
expensive check/build/scan/deploy used the enforced two-slot2GB process-tree
budget, one worker and bounded CPU; receipts verify drained children.

Baseline e8c57490211ed3103fd98529a5aef03984470a8f plus preserved earlier changes;
523 current source/test inputs are hashed in
Analysis/FFX/ShopDetails_20261102/verification.json. Deployed dinput8.dll SHA256:
b69d5e192163432fcbb273356bca3c5da5d619d02fcbea1ec486729eb3664c24.
The final-verification receipt passes. The earlier Closed geometry hash remains
3927dc16a121975df300aec9697407131b6947904873ad1ee17c9e6f58fd1b64.

The accepted boundary is the selected shop description, qualified extra pane
and independent setting documented here. Further refactoring requires a scoped
human follow-up. Actual game/input/speech confirmation remains pending in the
[short gameplay procedure](../verification.md#shop-details--november2-priority-at-the-next-shop).
No installer, package, commit, push or publication is included.
