# Airship Cid identity and tracking

Implementation and offline verification are **complete**; guarded deployment is **complete**. [Delivery receipt](../../../install/dev-console-20261020/cid-baaj-verification.json). This repair keeps Cid identifiable as his airship actor moves, is rebound, or is represented by stale static labels. It adds no interaction, cue or setting. Both 171-hook reader profiles pass, including actual command selection, movement and worker transfer. The game and launcher were observed closed before deployment preparation. No runtime probe or Tester Logs were used.

## Identity evidence and naming

Sol's retained source audit is outside Git under `D:/Games/Dev/Custom/FFX/Analysis/FFX/AirshipCid_20261020/150854_749759/`, with model records and `actor-summary.json`. It identifies Cid as model `0x2101` (decimal 8449), independently matching Fahrenheit's native `NpcId.chr_cid257` identifier.

| Hash-qualified airship script | Actual Cid worker | Cid's actual model | Finding |
|---|---|---:|---|
| `hiku0800` | 9 | `0x2101` | Actual Cid body verified independently of shared dialogue. |
| `hiku0801` | 8 | `0x2101` | Static worker 3 says Cid but loads Wakka model 5; actual Cid worker 8 was a generic Villager. |
| `hiku1600` | 7 | `0x2101` | Actual Cid had no static destination record. |
| `hiku1900` | 8 | `0x2101` | Talk entry 2 is a no-op below story value 3010; the old useful-interaction filter hid present Cid. |

Worker numbers are local script owners, not character IDs: W8 means worker 8. Shared dialogue can contain another speaker, so a dialogue speaker alone cannot reliably identify the actor. The repair changes model `0x2101` from the generic Villager fallback to the existing English Cid identity.

## Data flow, ownership and bounds

`navigation_labels.cpp` gives the dedicated Cid model precedence over an authored or static display-name mismatch, while retaining the native saved-name provider for ordinary NPCs. `identity_labels.py` and its NPC identity generator use the same model-identity priority; `navigation_models_1.inc` contains the Cid model identity. Both the maintained catalog and generated identity data therefore agree on model `0x2101`.

`navigationActorPresence` recognizes a bound dedicated Cid model on hash-qualified catalog scripts whose name starts with `hiku`. The existing body-enabled, readiness, finite-position and actor-binding checks still apply. Cid stays listed even when Talk is disabled or contains only a no-op; that record is marked inactive. Only actual Talk channel 0 determines his interaction availability, including native refresh in `nav_interactions.cpp`; ambient channels cannot manufacture Talk. Static useful-entry masks are not reused for a body that has changed owner. `navigation_scene.cpp` and `nav_exits.cpp` use the current actor binding and live pose.

Within a scene, `navigation.cpp` uses Cid's model identity for a stable entity key across worker or actor rebinding. Route and intention ownership still gets revalidated. If the owner changes, an older route may be cancelled; seamless Auto walk across a change of actor owner is not promised. Native route validity, interaction ownership and user input remain authoritative. The work remains within the existing bounded scene/actor pipeline: the transport caps scene snapshots at 500 worker records and 128 actor slots, which are maxima rather than expected room sizes. It does not introduce a new polling loop or wait on native processing.

## Controls, defaults and extension

There are no new controls or settings. Existing navigation controls remain in effect; Auto walk defaults Off and its separate Speak Auto walk path setting also defaults Off. This identification change does not move, interact with or activate Cid. It only improves the stable destination name and current position supplied to the existing navigation system.

To extend this pattern to another actor, first establish its exact model identity from retained native/developer evidence. Qualify script-specific presence by script identity and native header facts, then use current enabled/bound actor state and pose. Keep interaction activity separate from mere actor presence; require actual native interaction behavior before exposing an action. Preserve existing name providers for non-mismatched actors and validate identity stability across worker rebinding, scene changes and owner cancellation.

No new user-facing string or audio cue was added. The existing English Cid label is reused, and ordinary localized names continue through the native saved-name provider. Other-language display behavior was not newly audited as part of this fix.

## Offline evidence and remaining limits

The `airship_cid_followups.inc` fixture uses four retained original EBP cases with actual native capture and owned/native comparisons. Covered conditions include story states 0, 1290, 2440, 3010 and 4000, actor movement, rebinding, body disable and actual command selection across movement and worker transfer. The initial before-fix regression caught the Wakka-body case returning Cid. Both 171-hook reader profiles pass the final fixture. `test_identity_labels.py` also passes 3189 existing identity/membership cases plus the new dedicated-model assertions.

The canonical developer build passed without warnings, and guarded deployment verified all seven installed hashes. The developer console remains enabled; installer/package rebuilding stays held. There has been no gameplay confirmation, runtime probe or audio check. The English label reuses existing catalog text; this topic does not claim a new translation audit. The host uses controlled actors, native getters/callback boundaries and saved state; it does not prove every possible live story/event combination. Presence depends on the actual enabled Cid body rather than a hardcoded story list.

The separate Baaj report is now closed by user confirmation: Besaid correctly stays inactive without its treasure and all eligible seals work. Installed slot 0 matches `Saves/Dylan/Summon/ffx_000`; the earlier different-save diagnosis was incorrect. This confirmation does not establish Cid gameplay behavior or final door opening with all six treasures. See [Baaj seals](baaj_seals_20261020.md).
