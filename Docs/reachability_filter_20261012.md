# Navigation reachability filter (2026-10-12)

## Existing navigation behavior

- The original navigation scan invoked `readMesh` and `destinationUnreachable` for every candidate except butterfly targets.
- If a route failed, fallback could copy an ungated mesh and retry for an exit, or sweep another graph/endpoint for objects.
- Navigation rescans are triggered by repeated navigation-key input, including while stationary; they do not run every frame.

## Background capture and worker

- `reachability_capture.cpp`, `reachability_worker.cpp`, `reachability_shared.hpp`, and `reachability.hpp` move decoding, DFS, route evaluation, ungated copies, and format-free verdicts to an exclusive background owner.
- The worker receives copied raw POD geometry and target data. It never dereferences geometry tokens.
- Native hooks capture at most 8192 geometry bytes and one target with at most 32 competitors per `PInput` update.
- Capture uses a 16-entry queue with one try; unknown results or pressure retain the visible legacy behavior.
- The worker handles up to 512 targets. Stale epochs, player pose/floor changes, gate flags, and actor flags invalidate results.
- Native callbacks do not wait for the worker or queue.
- The capture limits bound each update; deferred or unknown work remains visible to the legacy path.

## Geometry lifetime and source identity

- The native `43EA00` BEFORE hook retires a resource before its replacement.
- Offline analysis retained `FUN_0083ea00`, which binds `DAT01301a44`, `DAT01301a48`, `DAT01301a4c`, and `DAT01301a50`; retained references to geometry pointers otherwise read them.
- The raw geometry cache uses the native resource identity, epoch and loader revision.
- Route cache keys include actor, worker, ID, and push-control state; verdicts use the exact copied target and context.
- Unknown exits keep the legacy conservative counterfactual rules.

## Offline verification

- Source integration is current maintenance work; it is not deployed.
- Sol's x86 `/W4 /WX` reachability fixture passed with 50,000 triangles and deliberately blocked worker execution for `nav_geometry` and `nav_zones`.
- The fixture confirmed native Off/cancel/queue-pressure responsiveness while blocked, plus cache, floor, gate, resource, epoch, and unknown-exit behavior.
- The decoder passed x86 `/W4 /WX`. The primary corrected its handoff to preserve collision==0 swimming DFS behavior.
- Luna's decoder fixture was initially absent at the approved path. Luna restored it with Sol's corrected collision guard; Sol reviewed and independently passed it.

## Limits and remaining verification

- Existing whole-mod listener/input/speech locks, scans, path and collision work, and audio remain nonconforming. This filter does not certify the whole mod or establish a speed improvement.
- No game launch, runtime probe, Tester Logs access, or performance measurement was performed.
- Runtime comparison remains user-side: unmodded, filter Off, and filter On using the same save and room, with stationary browsing and gate/cutscene changes.
- Related records: [threading audit](threading_audit_20261012.md) and [source entry](../AGENTS.md).
- Capture snapshots and worker verdicts are copied data; the retained native path remains responsible for native resources.
