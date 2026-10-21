# Guadosalam and battle follow-up — 2026-10-08

Implemented, offline-verified and deployed through the canonical guarded entry
point. Sol alone; no delegations. No game launch/control, runtime probe, Tester
Logs access, release ZIP update or publication. The user's own game-directory
log was explicitly requested and read. Current reports supersede earlier
optimistic conclusions in [player reports](player_reports_20261008.md).

## Context behind “Menu”

The old label meant that an ATEL path calls displayFieldChoice, not that the
target is a physical menu. Its actual arguments are important: text index is
argument1, not the final argument. The same correction is centralized for
displayFieldText/Choice extraction in `identity_labels.py`.

Guadosalam Inn `guad0100`, worker20, offers Rest/Leave and delegates the native
rest operation. It lacks the priced-commerce call combination that the former
shop inference required. An actual rest choice is now enough to identify
Innkeeper in Shops. Actual Buy/Sell or weapons/items choices plus native commerce
calls identify Shopkeeper. A conversational “Buy something for her” choice
cannot classify its speaker as a shop. Authored NPC names retain priority.

Other choice paths are conversations, Al Bhed compilation spheres, Jecht's
spheres and ordinary choice events. Native context supplies those labels where
proved. Unresolved choices receive the conservative low-priority Choice event
label, permitting better actual actor/model evidence to take precedence.
No target is called Menu solely because it opens a choice prompt.

The regenerated catalog preserves the previous 20261007_164736 audit's 3189
rows exactly in source/worker order, callback entries and model membership.
Only contextual labels/categories change (75 rows). Its old identity test
still expected the 20261006 catalog's 2442 rows and called a subsequently
identified Primer a Book; those stale assertions now use the retained baseline
and distinguish ordinary books from the Primer. The failed test did not prove
a new membership expansion. Actual `guad0100` length22852/checksum02e78409
lookup is also exercised in both native reader profiles.

## Inn route asymmetry

Own-session log reports on 11:33:59/11:34:02 show the Inn failing at a distance,
while closer approaches and the reverse Thunder Plains route succeed. Earlier
floor/portal fixes did not settle this report.

`nav_zones.cpp` prefiltered line goals using the side of the infinite trigger
line occupied by the original player position. A winding route can approach
the finite trigger from the other side and finish on the same side as that
original position. The prefilter incorrectly discards that valid endpoint.

Goals now consider both local sides and validate distance/height against the
finite trigger. The completed route must still prove an actual finite segment
crossing at the native interpolated height. Connected floors, physical portals,
collision gates and endpoint floor ownership remain authoritative; no distance
or traversal-count limit is introduced. Failed user-requested routes now log
player position/floor and live trigger coordinates for a subsequent report.

Native `00866680` dispatches type2 crossings to `00868450`. That routine tests
the current/previous movement segment against the finite edge and its height;
it does not require an endpoint opposite the player's initial global position.
Guadosalam Inn worker40 is an edge from
(-94.8541,31.2154,187.9628) to (-57.7394,30.0249,162.8112), height5,
leading to room140. Thunder Plains South is worker39, room97.

The regression connects an upper-right starting area to a lower-right exit
through a left corridor, forcing a finite crossing back onto the original
side. Distant and nearby approaches now pass. Existing finite-gap, separate
height, closed-gate, overlapping-floor and long-distance tests also pass.
An isolated counterfactual fixture reinstates only the old global-side
prefilter and reproduces the winding-exit rejection, proving that this check
alone causes that regression. The production file stays unchanged during this
check (`regress_inn_line_side_20261008.py`, `legacy-side.log`).
The actual save's live mesh is not retained in the log/offline field archives;
this is a reproduced likely cause, with exact Guadosalam acceptance pending.

## Status speech follows visible effect playback

Own log Cheer changes at11:35:39.515 precede the Tidus Cheer banner at.614;
later uses reproduce the order. `0078E680` precomputes effects and changes
stacked buffs before the banner. `00789800` passes the actual result packet
to it. `0078F0B0` later consumes that same packet during visible playback.

Calculated deltas are copied into a per-effect queue, keyed by resource
generation, target/actor identity and opaque packet identity. No borrowed game
pointer is dereferenced later. Playback checks the native source/sequence,
current effect index and count in both target+774 buffers, stride2D8, with
2C-byte packets at buffer+18. Only a verified consumed index releases speech.
Preview/unconsumed/unrelated effects stay silent; battle reset clears pending
effects. Native buffers hold16 packets, which bounds buffer validation.

Each consumed packet retains its own copied delta even if later effects have
already precomputed a higher live buff count. Two packets consumed in one call
produce both applications. Same-effect nested clears are owned by their outer
calculation/playback or confirmed-death producer, avoiding duplicate producer
reports. There is no text-content deduplication or timer-based flush.

Other committed playback status changes still read in that callback. Zero
numeric HP/MP graphics now read their target and actual result. Negative numeric
values still read healing/MP restoration, including enemies. Non-numeric
guard/overkill sentinel values remain excluded. Zero restoration follows the
renderer's original-amount sign, retaining healing rather than calling it
damage. On the user's visual-output condition, final archive review confirms
`0079FC20` uses a do/while conversion so0 produces one digit, and `0079FC80`
registers that floating glyph with the damage/healing colour. `0079FA20`
returns0 after successful registration orFFFFFFFF if its128-entry glyph pool
cannot accept the popup. Outcome speech now follows that successful return;
rejected zero healing/damage stays silent. These are floating numbers, not a
text box. No zero damage is invented for
status-only abilities or absent native outcome graphics. Gold Element Thunder's
missing outcome is consistent with the removed numeric-zero early return;
its exact live result was not captured in the old log.

## Summon Help resolves the native aeon name

The battle Help buffer contains a13 actor-name macro. The literal decoder
skipped it, yielding “Summon the aeon .” The existing central rendered-text
parser now resolves it with native `008B6610`, whose13 case calls008AC800,
then combines full Help with the selected summon vitals. No fixed name or
English replacement sentence is appended. Encoded name-token, renamed aeon,
fresh Help and Magus Sisters fixtures pass for both profiles.

## Lightning: full manual window

The user confirms Auto dodge works. They withdrew the apparent manual-dodge
success because Auto dodge was still enabled; manual acceptance is pending.
The own log records warning requests for bolts344–346 and an auto-dodge
injection for347 at elapsed6. Those lines alone do not prove the native success
counter, but the later user report confirms Auto dodge gameplay.

On the user's explicit request the beep moves from elapsed6 to elapsed0: the
first native input query in the same VM tick as CALL4003 starts the visual
flash. Both archived South/North scripts expose twelve ordinary simulation
ticks, about0.4seconds at normal speed, before the hit. The single producer
uses that actual script event, with no wall-clock prediction. Normal mixer/
display latency and game speed can affect the perceived duration.

Auto dodge remains at elapsed6, preserving its working behavior. Help names
the configured primary keyboard action4 (C in the reported configuration) or
controller action18, without presenting the unconfirmed alternate Enter alias.
The separate ordinary Confirm action is not substituted. Help instructs one
press as the beep begins, and Auto dodge Off for manual practice. Native
count==1 remains the success condition; multiple press edges still fail.

## Validation and remaining checks

- Four offline identity summaries, including retained catalog membership and
  native choice context/Guadosalam service regression.
- Native navigation regression passes, including the winding-line case and
  all existing conservative crossing/floor/gate fixtures.
- Both reader profiles pass: ten summaries, including deferred effects, two
  consumed slots, actual zero/enemy-heal output and encoded summon names.
  Moving numeric speech to successful return initially exposed one old MP
  fixture still dispatching entry; that fixture now models the proven native
  return phase. Final zero-rejection and ordinary restoration cases pass.
- Both archived lightning maps pass opening-flash timing, unchanged Auto dodge,
  primary keyboard/controller help, identity and cancellation tests.
- Warning-free canonical development build; fresh proxy fixture passes.
- Closed game/launcher verified by guarded deployment; both compiled executable
  profiles, launcher bootstrap and all13 installed payload hashes verified.

Focused logs and hashes: `install/inn-menu-followup-20261008/verification.json`.
New live Inn route, buff order, summon text and manual dodge acceptance remain
user-side. No broader suite repetition was needed after affected checks passed.
