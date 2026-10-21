# Custom/FFX/ffx-x2-screen-reader — source entry

Read the shared [development agreements](../../../AGENTS.md) and the [project guidance](../AGENTS.md) once if they are not loaded, then [CODEX_MEMORY.md](CODEX_MEMORY.md). The project guide contains the converted Claude rules, current ownership, build command and deployment boundaries.


Preserve existing source changes and upstream attribution. Keep technical reference lookups scoped to the task. Update project memory and the established status/architecture document when their facts change; this entry file does not need a parallel journal.

## Strict thread ownership

The current user instruction requires hooks to detect/copy bounded ready data and
continue native processing immediately. No game-thread waits for workers, queues,
locks, I/O, RPC, sleeps or joins. Heavy text/scans/routes/audio/logging run on
workers using owned snapshots; thread-affine native pointers stay on their owner.
See [current audit and restructuring](Docs/threading_audit_20261012.md).
The audit is not a claim that existing source already conforms.

The current user instruction points to the maintained
[architecture/speech contract](../../../codex/templates/accessibility-mod/references/speech-and-architecture.md),
[integration](../../../codex/templates/accessibility-mod/references/integration.md),
[offline discovery](../../../codex/templates/accessibility-mod/references/offline-discovery.md)
and [feature/control contract](../../../codex/templates/accessibility-mod/references/features.md).
Read the bundled policy through the shared MODEL_POLICY entry. Preserve this
maintenance scope; no Frida-first ritual or speculative capture ladder. Sol owns
correctness/technical evidence; the user confirms gameplay, without code review.
Accumulate one prioritized nonvisual final test procedure with exact offline
limits, expected behavior, failures and progression dependencies.

October13 responsiveness requirement also covers mod-side actions. Measure the
whole request through collection, readiness, queue wait, computation and first
usable output, plus data age. Tiny native batches that take seconds to assemble
are a blocker. Define evidence-based feature/platform budgets and verify normal,
large/busy, cold/warm and rapid-action cases; keep native capture nonblocking,
preserve affinity/reveal/correctness and use compatible offline caches/live deltas.
Final gameplay timing remains user-side. See the audit's latency acceptance record.

## Restricted tester evidence

`../Tester Logs` is outside active source and contains logs submitted by testers.
Do not enter, enumerate, read, search, index, glob, copy or access it without
explicit issue-specific user permission. Exclude every `Tester Logs` directory
from broad workspace/source searches and default/latest-log discovery. Permission
for selected issues/files is limited to those issues/files and is not general or
permanent permission. Ordinary mod logs are generally read in the game directory
and must be the user's own session. See [project boundary](../AGENTS.md#working-rules).

## Engineering and model delegation

Follow the current [shared model policy](../../../codex/MODEL_POLICY.md).
Sol retains behavior discovery, unresolved correctness/design, unfamiliar
pointers/lifetimes/ownership/callbacks/state and architecture, plus integration,
review and verification. Delegate bounded routine work to Luna only when behavior,
correctness criteria and a verified pattern are already known. Use fresh concise
handoffs with fork_turns=none, model=gpt-6-luna and reasoning_effort=medium, allowed
files and acceptance checks. If a result reveals a conceptual misunderstanding,
Sol takes over. Minor local corrections are appropriate. Consider a sequential
one-writer handoff before retaining coupled work; avoid uneconomical handoffs.
Astra is emergency analysis only after two distinct documented evidence-producing
Sol approaches fail at a genuinely unfamiliar extreme RE/physics/geometry blocker.
It is never used for review, testing, routine debugging or working systems.
Summarize each completed delegation: model, task, outcome, review/validation and
remaining issues; retain durable findings in docs linked from memory. Delegation
does not expand runtime, asset-read or publication permission.

Latest release instruction (2026-10-12): one installer includes/installs locked PRISM and licenses plus SDL3, verifies matching existing PrismRuntime read-only and backs up/removes Tolk. No companion installer or prior-build restoration. See the October 12 installer checkpoint.

Release order correction, 2026-10-12: stop for the night until the user resumes. Complete deployment blocker repairs and verify the final deployable payload before compiling/packaging the installer. Existing premature 0.5.6 EXE/ZIP is unusable and must be recompiled after deployment is unblocked. Preserve source and history; no prior-build restoration.

2026-10-12 later instruction: monitoring tonight's full usage reset is authorized now. Resume necessary FFX blocker/deployment work only after the reset is confirmed; then recompile/repackage the single installer after verified deployment. This conditionally supersedes the night stop, not the release order or runtime/data boundaries. Record monitor state in source Docs/UsageResumeState.json; retain the 2% remaining stop threshold.

Latest user instruction, October13: fix all remaining deployment blockers and implement proper Stone Gaze/related ability speech, superseding the earlier documentation-only restriction. Then guarded-deploy the verified current-source build. HOLD installer rebuild/package for in-game testing; do not rebuild automatically after deployment. The reset heartbeat remains PAUSED at the user's request. Keep the2% allowance stop and existing runtime/data boundaries.

October21 release instruction supersedes the deployment/installer holds: deploy
the verified current build and prepare final1.0, make the source repository public
and publish its ZIP through GitHub Releases. Follow the FFPR Core convention:
future explicit prepare-release requests include reviewed commit/push, annotated
tag, draft upload/download-hash verification and publication. See Docs/release.md;
Dropbox is historical delivery only. Repository renaming/history rewrites remain
separately authorized; a known retained game-code object blocks making the
affected repository public. Runtime/probe restrictions remain.
