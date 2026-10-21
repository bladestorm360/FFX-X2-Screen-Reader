# Accessibility integration for side activities

The 2026-10-07 request activates Blitzball Auto/command-mode positioning and
the FFX Catcher Chocobo pickup cue. Frida spatial/event behavior was authored
first and offline-checked, then ported to one native producer using verified
archived native getters and extracted script declarations. The registered
`minigame_accessibility.js` switches to read-only diagnostics when the native
producer is present. The user runs launcher probes; none was executed here.
Implemented and offline-verified are separate from gameplay-confirmed.

## Blitzball

Implemented: context-owned F1 carrier/acting player, F2 own positions, F3 opposing
positions, F4 goalkeeper/goal directions, F5 score, native goal/turnover events.
All four extracted bltz0000 variants have checked declarations and checksums.
Manual A/B releases these keys. Native roster names and the additional menus/
events below remain planned; no full-match accessibility claim is made.

Recover its original team/formation, roster/stat/technique, match setup, action,
encounter and results menu input handlers. Read native selection labels and
stat deltas when transferring/equipping players. Map actual ball possession,
passes, shots, encounters, tackles, technique effects, score and half-end events.
Use copied field coordinates for optional player/goal bearings; control cues
follow native possession/movement commits. Clarify player control preferences
before adding assisted aiming or timing. Acceptance: roster preparation and an
entire match can be completed with speech/audio, including unsuccessful actions
and extra time. No frame polling or invented opponent positions.

## Chocobo training/races

Implemented for FFX Catcher Chocobo only: all twenty balloons' actual availability
bits, native touch radius/height, all four course legs, exact continuous stereo
centering, F1 pickup query, F5 counters, saved F6 cue toggle, native collection/
bird-hit events, running/finish guards and focus/menu/generation cancellation.
Both nagi0000 variants are checked. Embedded pickup.wav is replaceable at boot
through Sounds/pickup.wav. Gameplay collection at speed remains unconfirmed.
Other training stages, Remiem and FFX-2 are not implemented in this pass.

Identify start/countdown, lane/steering, balloon collection, bird collision,
finish and reward handlers. Derive course boundaries and moving hazards from
native collision actors. Offer distinct optional tones for hazards/collectibles
and preserve native steering through SDL. Announce actual timer changes and
penalties at events. Any automatic steering, hazard immunity or success shortcut
must be an explicit optional setting with documented behavior.

## Lightning dodging and butterfly catching

Thunder Plains warning and requested optional auto dodge are implemented and
offline-verified in the native input window; see [processor evidence](thunder_lightning_20261008.md).
Makalania Central/North butterfly cues, current red/blue targets, catch count,
native timer and manual hazard-aware routes are implemented/offline-verified;
see [butterfly evidence](butterfly_catching_20261008.md) and
[route geometry/limits](butterfly_paths_20261008.md). Current body/state deltas
drive availability and routes. Ordinary octant Auto walk cannot safely follow
these narrow detours, so butterfly movement remains manual. No automatic winning,
hazard immunity, synthetic timers or altered rewards. Actual course reachability,
timing and playability remain user-side acceptance.

## Optional field content

Cover Monster Arena capture/registration/battle selection/results, Celestial
Weapon upgrade/shrine confirmations, Cactuar searches, treasure puzzles,
Remiem races, Al Bhed collection/rewards and other optional menus as separate
native owners. Reuse entity identity and collision routes for fixed targets;
special transports require their original interaction and completion events.
Compare inventory/stat/ability changes through actual getters before/after
rewards. Confirm localization and native failed-action messages.

## FFX-2 and Last Mission

Create separate exact executable profiles and ownership diagrams before ports.
FFX-2 needs ATB, dressphere/garment-grid, chain, mission and battle-result readers;
Last Mission needs floor/tile/entity interactions, inventory and turn outcome
readers. After those foundations, prioritize Sphere Break, publicity/matchmaking,
Gunner's Gauntlet, chocobo expeditions and other requested optional activities.
The FFX offsets and event mappings provide architectural examples only.

## Delivery gates for each activity

Retain timestamped event traces and archived function/caller evidence. Verify
native text/getters, copied lifetime and stale-map cancellation; prototype first,
then port once. Add fixtures for meaningful event ordering/failure cases and a
blind gameplay acceptance checklist. Document unavailable timing/position data
instead of inferring it. Ship menu controls/descriptions and per-feature opt-ins
with persistent volume/binding settings. Gameplay validation stays under the
user's explicit authorization.
