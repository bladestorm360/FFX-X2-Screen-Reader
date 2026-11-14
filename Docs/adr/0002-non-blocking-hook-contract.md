---
status: accepted
date: 2026-10-12
decision-makers: project maintainer
---

# Hooks copy ready data and never block the game thread

This record was written on 2026-11-14 from existing notes and code. The rule dates from the threading audit of 2026-10-12.

## Context and Problem Statement

The reader hooks game functions that run on the game thread, and DirectInput and User32 functions that other threads can also call. Any wait in a hook delays the game. Notes from October 2026 record field freezes in shipped builds (`Docs/threading_freezes_20261009.md`). The threading audit found hooks and input callbacks that held a global reader mutex, ran route searches, or waited on speech (`Docs/threading_audit_20261012.md`).

What may code do while it runs inside a hook on the game thread?

## Decision Drivers

- The game must never wait for the mod.
- Game objects are thread-affine. A worker must not dereference a game pointer.
- Every distinct event must still be spoken, including repeated identical text.
- Results must belong to the current map and selection when they are spoken.

## Considered Options

- Let hooks do their work inline, guarded by locks.
- Let hooks capture bounded data and hand owned copies to workers.

## Decision Outcome

Chosen option: hooks capture bounded data and hand owned copies to workers, because a lock or wait on the game thread can stall or freeze the game.

The contract, from `Docs/threading_audit_20261012.md`:

- A hook detects and copies bounded data that is ready, then lets native execution continue at once.
- The game thread does not wait for mod workers, queues, locks, file I/O, RPC, sleeps or joins.
- Text formatting, scans, route searches, audio and logging run on workers that use owned snapshots.
- Thread-affine native objects stay on their owner thread.

How the code applies it:

- `src/native/hooks.cpp`: `enterBridge` returns at once for a foreign thread or a reader re-entry. `tryNativeReaderAccess` is a nonblocking admission check. `releaseNativeReaderAccess` has no lock to release. `deferNativeAction` queues at most eight actions and refuses more.
- `src/native/hooks.cpp`: the `registryMutex` guards hook registration only. `hookAddressKind` refuses registration from inside a native callback.
- `src/native/resource_reset.cpp`: map and resource teardown publish a new generation, so workers drop stale results.
- `src/native/owned_retirement.cpp`: owned objects are destroyed away from native callbacks.

### Consequences

- Good, because a slow screen reader, disk or worker cannot stall the game.
- Good, because the fixtures can test the rule offline (`src/native/tests/run_tests.bat --threading`).
- Bad, because each feature must copy what it needs at the hook. It cannot read more game state later from a worker.
- Bad, because work is split between capture and worker code, which is harder to follow.

### Confirmation

- `src/native/tests/run_tests.bat --abi` and `--threading` run the hook ABI and threading fixtures.
- Code review: reject a lock, wait, sleep or file write on a path that a game-thread hook can reach.

## More Information

- `Docs/threading_audit_20261012.md` lists the paths that did not conform on 2026-10-12, with the change each one needed. The audit says it "is not a claim that existing source already conforms".
- Uncertain: later commits converted some of those paths, for example the speech worker. This record does not re-audit them. Mutexes remain in `src/native/hooks.cpp` (registration only) and `src/native/speech_queue.cpp` (worker-only sink).
- `Docs/features/runtime/index.md`, section "Initialization and ownership".
