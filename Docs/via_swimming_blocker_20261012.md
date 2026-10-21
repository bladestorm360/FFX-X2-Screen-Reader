# Via Purifico swimming blocker — 2026-10-12

## Tester report and scope

A tester urgently reported a Via Purifico swimming run with Tidus, Rikku, and Wakka.
The map-name announcement is missing (`M`), boss approach speech fails intermittently,
the post-boss route has no exit, and two save points are confusing. These are the
tester-reported symptoms; this note records source and offline evidence, not gameplay
confirmation.

## Actual tester save follow-up

The user clarified that the prompts in “Review Salvage Ship Swim Exits” belong
to this active task. Both requested saves are now read offline: own Salvage
ffx_006 remains byte/hash-identical; Dylan's latest observed14:10 attachment
ffx_050 was downloaded into workspace Saves/Dylan/ViaPurifico-20261012.
SHA25652103deb45b90e3b844ae2096149ad3a78f57bbde19c0e8400498919fe8d2069,
26,880 bytes, room209/area244, saved room209/entrance1, story2220, both
underwater flags1. This confirms the current script/region qualification.
The updated fixture reads these actual context bytes before the existing
script/floor/boss/post-boss/save-sphere checks. It still constructs owned
synthetic live worker/actor flags; a save does not contain those runtime
pointers or reproduce movement. No game/save load or boss fight was run.
Discord was closed after download; process check found none remaining.

## Native script and map evidence

- Native script: `stbv0000`, room 209. EBP SHA-256:
  `cc7999606bfefc23c4aa0e73f8b46cedefab897b25549d0807abf38ea77d3f1c`; code 19861
  FNV: `ada03b86`.
- Existing supplemental worker 26 entry 4 is Exit 1 for room 208/entrance 0. Native path:
  `Common010C -> 8584B0 -> 86FED0`.
- Native `+0x48` primary chunk correction was implemented in pending source. Auxiliary
  `+0x1F8` is zero. Dictionary entries 244 and 245 both say “Bevelle - Via Purifico.”
- Offline `stbv00` mapout from the authorized installed VBF has 86 triangles and 85
  vertices at scale `17.3559326171875`, groups 0 and 48.
- Vertical connectors such as 38/39 and 50/51 project to duplicate vertices. The old
  `floorPortal` counted three matches and rejected them. It now counts unique matching
  points, requiring exactly two distinct points and a nonzero XZ edge. Native adjacency
  remains unchanged.
- Saves from actual chunk `5E00/5E0C`: worker 16 model `50AB`, position
  `(1623.0029296875, -84.99993896484375, -1707.5)`; worker 17 position
  `(1301, -84.99993896484375, -1365)`.

## Boss region and swimming navigation

- Boss approach is worker 25, entry 6. Native region is type 2, channel 5, bit `0x20`;
  radius comes from native geometry at `+0x48`, read by `868450`.
- The old mod checked radius only on channel 3, bit `0x08`, and missed the enter region.
  Current source checks radius on channel 3 OR the type-2 channel-5 flag, preserving
  rectangle and leave semantics.
- Exact hash-qualified Via workers 16/17/25/26 receive a horizontal swimming
  approach policy, copied through the owned filter packet. Other interactions
  retain native height restrictions, including ship-deck targets. Cached swimming
  waypoint Y is recalculated from the current origin.
- `inInteractionZone` and `navigationTargetReached` still honor native height and range.
  Cue waypoint advancement ignores stale Y only for horizontal swimming.
- Salvage depth goals retain their strict `Y > 250` condition.

## Offline validation and evidence

- The real-EBP, 86-triangle Via fixture PASSES against reference and installed profiles,
  with 140 hook sites each. Final scoped output is retained at `install/via-swimming-priority-20261012/via-readers-final.log`.
- `src/native/tests/via_swimming_followups.inc` checks three entrance/two area map names,
  event and exit, depth changes, no false arrival, post-boss exit, both saves, and the
  disabled callback using real EBP/map data.
- Dummy-SDL audio/cue checks, native geometry (including the ship-deck height
  guard), and the owned reachability filter pass. The first broad geometry run
  rejected a generic height exception; primary narrowed it to exact Via targets
  and reran the affected checks successfully.
- Retained evidence: `Docs/Scripts/Offline/output/open_issue_repairs_20261012/via_swim/`
  (`manifest.json`, `floor.json`, `triangles.bin`, `vertices.bin`,
  `stbv0000_flow.json`, the named `FUN_*.txt` records).
- Synthetic earlier pass is distinct from the first real-map failure, which exposed the
  connector bug. The real-map fixture now passes after that correction.
- Canonical x86 tester build, fresh proxy forwarding, launcher surrogate,
  guarded deployment and all seven installed SHA256 hashes pass. PRISM receipt
  verifies read-only; Tolk was backed up and removed after success.
  [Delivery verification](../../Analysis/FFX/RepositoryEvidence/historical-install/via-swimming-priority-20261012/verification.json).
  Gameplay confirmation remains pending.

## Remaining status and boundaries

- Overall thread ownership remains NONCONFORMING. No new native hook, wait, I/O, or SDK
  calls were added; geometry fixes use existing pure operations. Startup/filter/audio
  conversion exists, while route work, scans, locks, and SDL ownership remain pending.
- Native map-name handling uses the deployed `+0x48` primary chunk correction.
  No fallback or area-text change was invented.
- No game launch/control, Via save, tester-log access, or runtime performance claim.
- Runtime/gameplay confirmation and performance measurements remain outstanding. Track
  implementation, offline verification, and gameplay confirmation separately.
