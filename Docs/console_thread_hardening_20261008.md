# Console, lightning and battle HP follow-up — 2026-10-08

The player reported a Shift+F1 console crash when a Thunder Plains bolt arrived,
followed by a battle that would not accept input. The exact crash instruction
is unproven. The user's own game-directory native log has no console/crash
marker; slow field-input spans during battle are not proof of this cause.
No Tester Logs were accessed for this follow-up. Their earlier authorization
was limited to exit/pathfinding and highlighted Change vitals.

## Confirmed hazards and repair

`utility_console.cpp` formerly posted an asynchronous show with only the
foreground HWND. A field check at the key handler did not survive an intervening
battle, dialogue, menu or resource transition. `utilities.cpp` queued only a
resource generation and accepted any foreground window belonging to the process.

More seriously, `pumpUtilities` and the trial solver called native46FED0 and
46E920 inside the471D10 reader callback. `hooks.cpp` still held readerMutex and
inReader was true. The first condition could block a native load waiting on a
loader-thread reader; the second bypassed native resource teardown and469040
continuation initialization. These are verified source hazards, not a claim that
either explains the precise reported crash.

`NativeAction` centralizes three phases. Prepare and completion serialize reader
state. Invocation uses copied scalar arguments, runs outside every reader lock,
and allows normal nested native hooks. The queue drains only after the outermost
adapter return on the PInput225930 owner, with no active resource scope. A
thread-local drain guard prevents recursive dispatch. Generation changes discard
stale work/completion; exceptions defer pending jobs until native stack-unwind
cleanup at the next owner entry. Saved registers, flags, x87/SSE and native return
overrides remain protected by the existing adapter.

Requests reserve an exact free native signal before queueing. `utilityBusy`
recognizes that pending reservation. Prepare revalidates its serial, worker,
chunk, free slot and context; a consumed slot cancels before calling46E920.
469040 now actually runs and applies the verified continuation. Solver phases
advance before queueing, and refusal cancels matching ownership/stops the solver.
Explicit force warps retain their dialogue/running-script recovery policy but
still reject battle, menus, minigames, foreign focus and an invalid owner/context.
Warp completion never restores an old controller after a resource-generation
change; same-generation cleanup clears only the flag added by this request.

`utility_context.cpp` publishes UI-safe revision/generation/actual-game-HWND
tokens from owner events. Resource, battle-window, native menu and dialogue
entry invalidate tokens before their originals change context. Input dispatch
also invalidates on focus/context changes. The console owns only Win32 controls,
copied tokens and command text. Show, submission and hide validate the session;
old show/hide messages cannot affect a reopened session. Activation checks the
exact game foreground before and after focus transfer. Cancellation restores
the captured game window only while the console itself owns foreground, so a
new battle can accept input. External application focus is preserved. Startup
failure/thread exit reset ownership; Win32 message/thread exceptions are contained.
The existing accessible edits/buttons, help and Enter-release drain remain.

No mod lock is held across native loading. These protections rely on the native
owner/lifecycle contract and generation events, rather than pretending to acquire
an undocumented native resource-lifetime lock. The exact player timing remains
gameplay acceptance work.

## Renderer and settings

Both supplied screenshots show numeric **current HP and MP**; neither shows a
numeric maximum HP. Change49F070 draws portraits/native names and492C20 current
HP/MP plus492880's **Overdrive** bar. No status-effect name/icon producer is present in
that Change row.39AE00 returns actor+594, the maximum used for HP color.
Current Change values remain actor+5D0/+5D4, through mapped394030, including
initialized inactive reserves and the three Magus actors.

The user's HP preference formats current HP plus its rounded, bounded percentage by default,
for example `HP 321 (64%)`. F8 > Miscellaneous > **Show maximum HP** defaults Off;
On uses `HP 321/500` instead. The setting applies to Change/Summon highlights
and the H active-turn reader. An unavailable maximum does not invent a percentage.
MP, status speech and existing saved field/shop statistics retain their sources.
Change vitals remain automatic with detail settings Off; H retains its turn-owner
meaning and I repeats highlighted detail.

2026-10-08 renderer correction:492880 calls395560/3955A0, returning
actor+5BC/+5BD (Fahrenheit ChrRam limit_charge/limit_charge_max). Neither HP nor
MP has a bar here; no MP maximum/bar renderer exists in492C20. Preserve the
user-requested HP fraction presentation; MP stays numeric. The conditional MP
percentage/max-toggle request therefore does not apply. Summon I now reads native
Help together with fresh displayed vitals; see [current repairs](player_reports_20261008.md).

F8's main menu has Speech, Navigation audio, Navigation, Miscellaneous and Close.
Other general settings, the maximum-HP toggle, lightning auto-dodge, controller
bindings and supported trial controls are under Miscellaneous. Lightning warning
is under Navigation audio, default On; auto-dodge defaults Off. See the separate
[lightning processor evidence](thunder_lightning_20261008.md) for exact two-map
qualification, native input/counter semantics and six-tick timing.

## Delegation and validation

- Sol6.1: read-only console investigation identified the stale asynchronous show,
  native mutation under reader lock and suppressed native hooks; exact crash
  remained inconclusive. Primary reviewed and integrated these findings.
- Sol6.1: implemented console UI tokens/focus/startup safeguards and a mock-Win32
  fixture. Primary reviewed, added cancellation focus restoration and exception
  containment, and passed the fixture without opening a real window.
- Sol6.1: first lightning counter search was inconclusive because of a wrong
  SaveData base assumption. Corrected archived native initialization proved the
  ordinary South/North processors, timing and raw pressed query. Implemented the
  qualified event hook, actual-archive fixture and documented evidence. Primary
  reviewed/integrated audio/settings/bindings and passed both-map tests.
- Astra6: documented the difficult x86 reentrancy/loader architecture and supplied
  a real-MinHook regression. Its fixture persisted through the PC/app restart;
  no final agent execution report survived. Primary reviewed it, corrected
  post-exception pending-call gating, and passed its real native tests.

Full native suite:37 PASS summaries. Its two fixture-only shadow-name warnings
were removed; a focused final console run is clean. Final qualified profile and
development/release utility checks also pass without warnings. Real-adapter tests
cover owner refusal, nested initialization exactly once, equal-stack native tails,
loader callbacks while invocation waits, generation cancellation, exceptions,
return override and SSE preservation; existing ABI/threading/tail tests also pass.
The actual SDL dummy-stream mixer checks a centered60ms warning independent of
speech/beacon/walls, immediate cancellation, no replay, focus and generation.
Menu persistence and both native reader profiles cover HP percentage/max toggling.

Canonical development build (`build_dinput8.bat --no-deploy`) is warning-free;
guarded deployment subsequently passed both compiled executable profiles,
all dependencies, launcher bootstrap and13 installed hashes. The first deploy
stopped before replacement because the bundled PowerShell environment prevented
Windows PowerShell's Get-FileHash module discovery. The established process-local
Windows module path resolved it; no source/system environment changes or checks
were bypassed. Build/test/deployment logs and hashes are in
`install/console-lightning-20261008/verification.json`.

No game launch/control/probe, real registry repair, release ZIP refresh or remote
publication. Exact console/bolt timing, remapped physical controls, live dodges
and sustained streak behavior remain user-side gameplay confirmation.
