# Navigation coverage audit ? 2026-10-07

The tester reported an unspecified "highland". The user cannot identify its map. All extracted maps were audited; the 17 Mi?ihen/Calm Lands variants were checked as priorities. This is archive coverage and offline verification, not live-save acceptance.

Later player clarification: the reported location was Highroad Agency, correcting
the initial "highland" description above. The player confirms improved working
pathfinding and restored save-sphere entries at Highroad Agency and Djose Temple.
Other saves/maps are still unconfirmed in gameplay.

All 397 extracted source hashes match. Every eligible worker has either a useful outcome or an explicit exclusion record. All authored callbacks after initialization/idle are traversed, including callbacks above 31 and the complete finite REQ graph. There are zero omitted eligible workers and zero truncated traversals.

The final catalog contains 3,186 targets, 744 more than the prior 2,442. Categories reflect model/English identity evidence after raw outcome classification.

| Category | Targets |
| --- | ---: |
| Exit | 659 |
| Event | 427 |
| Treasure Chest | 269 |
| Object | 490 |
| Save Sphere | 106 |
| NPC | 1195 |
| Shop | 40 |

Authored workers have up to 289 callbacks; the latest useful callback is 133. The native catalog now has ten mask words, with generator overflow rejection. Native dispatch also validates the actual callback count. Save spheres use their actual model and live callbacks. Unknown chests with a proven no-op callback are removed; unresolved behavior is retained conservatively.

| Priority map | Source variant | Useful workers | Exits | Save spheres |
| --- | --- | ---: | ---: | ---: |
| mihn0000 | mihn0000.ebp | 22 | 2 | 1 |
| mihn0100 | mihn0100.ebp | 3 | 1 | 0 |
| mihn0200 | mihn0200.ebp | 15 | 7 | 1 |
| mihn0300 | mihn0300.ebp | 19 | 2 | 1 |
| mihn0400 | mihn0400.ebp | 14 | 3 | 1 |
| mihn0500 | mihn0500.ebp | 14 | 4 | 0 |
| mihn0600 | mihn0600.ebp | 22 | 3 | 1 |
| mihn0700 | mihn0700.ebp | 8 | 2 | 0 |
| mihn0800 | mihn0800.ebp | 15 | 2 | 0 |
| nagi0000 | dbg_nagi0000.ebp | 43 | 4 | 1 |
| nagi0000 | nagi0000.ebp | 43 | 4 | 1 |
| nagi0100 | nagi0100.ebp | 7 | 4 | 1 |
| nagi0400 | nagi0400.ebp | 10 | 3 | 1 |
| nagi0500 | nagi0500.ebp | 22 | 3 | 1 |
| nagi0600 | nagi0600.ebp | 1 | 1 | 0 |
| nagi0700 | full_nagi0700.ebp | 5 | 1 | 1 |
| nagi0700 | nagi0700.ebp | 5 | 1 | 1 |

Runtime reconciliation follows the user?s BDFFHD separation of static authored identity and live state: fresh native flags/channel remapping/actor membership/geometry on each key, stable selection/order when appropriate, and actual mutable collision for routes and walls. FFX uses its native collision mesh, not a baked walk grid. Copied scripts are bounded and reset per resource generation.

Interaction routes terminate strictly inside native geometry and validate the final projected endpoint. Backslash in range reports facing, including an exact camera-relative bearing when needed. Fractional final steps are spoken; beacon waypoints are not skipped early.

The complete x86 suite covers both profiles, enabled/disabled/remapped interactions, callbacks above 31, inside-region exits, save spheres, collected no-op chest callbacks, counter/rotated/vertical/crossing zones, moving geometry, waveform output and immediate current-map audio. No live game/probe/save was accessed.

Variable-dependent branches are conservatively included. Unknown computed request destinations cannot be given a definitive destination name without live evidence. Static completeness does not prove every game-state eligibility branch or every save.

Machine-readable evidence: [coverage](../../Analysis/FFX/RepositoryEvidence/offline-output/navigation_coverage_20261007_115012.json), [target identities](../../Analysis/FFX/RepositoryEvidence/offline-output/FFX/20261003_audit/navigation_audit_20261007_114432.json), [minigame control flow](../../Analysis/FFX/RepositoryEvidence/offline-output/minigames_20261007_104142/manifest.json). Native body and signature audits retain their timestamped output folders.


October16 scoped addition: native ChMap collision remains the route base.
Gagazet now also has a source/script/geometry-qualified shoreline transition
bake, preserving native land/swim mode changes and current collision gates.
It is not a replacement walk grid. [Evidence and coverage](gagazet_cave_20261016.md#october16-mixed-water-and-land-routing).
