# Runtime, output, text and input

## Initialization and ownership

`src/proxy/dinput8_proxy.cpp` forwards the native DirectInput API and starts the
native bootstrap. `reader.cpp::initializeReader` checks speech readiness, the
actual executable name, `configureNativeProfile`, resolved extension signatures
and feature dependencies before enabling hooks. FFX gameplay and the separate
Will-caption profile have different registrations. This is not general FFX-2
gameplay support. `profile*.cpp`, `profile_extensions_data.hpp` and
`Docs/Scripts/Offline/native_extensions.py` keep exact supported identities and
masked original bodies; a similar function name or blanket address delta is
insufficient. Preserve complete body/caller/operand qualification when extending.

The native owner captures ready scalars/encoded data. `hooks.cpp`, `hook_calls.cpp`
and `native_action.hpp` preserve original argument stacks, registers, return and
callee cleanup; handlers do not reconstruct arbitrary guessed native prototypes.
`NativeReaderAccess` is a nonblocking admission gate. `resource_reset.cpp` publishes
generation invalidation at map/resource teardown, and `resetOwnedReaders` retires
feature state. `owned_retirement.cpp` handles owned-object destruction away from
native callbacks. `atel_resources.cpp` copies bounded physical resources and
prepares immutable identities/declarations for later native lookups; it avoids
rehashing whole scripts in an interaction callback. `native_worker_index.hpp`
qualifies loaded worker/header/chunk identities and caps collection at500 loaded
workers/1024 locals. A pointer retained only as an identity token is not worker
permission to dereference it.

See [current ATEL ownership](../../atel_field_owner_20261013.md),
[thread/reset repairs](../../threading_freezes_20261009.md) and
[SDL ownership](../../sdl_owner_20261012.md). Earlier polling/synchronous designs
in the historical architecture are superseded by current capture/owned domains.
The read-only prototype bridge exposes a small verified address whitelist; no
Frida process, Gadget producer or prototype JS ships in this path.

## Output and text

Feature producers build `TextPlan` recipes (`text_plan.hpp/cpp`,
`ready_game_text.cpp`, `utility_text_plans.cpp`). Native work copies bounded ready
operands and records formatting operations; preparation decodes text, joins rows,
formats values and creates owned buffers. A plan has128 nodes and114688 payload
bytes. Invalid/oversized data is rejected, not silently converted into an unsafe
unbounded read. Receiver/domain IDs own the downstream feature state; do not add
a second producer to compensate for a missing original event.

`speech_capture.cpp` provides the1024-event/512Ki-character bounded capture
transport. `speech_queue.cpp` and `speech_domains.cpp` preserve sequence,
generation, modal, tutorial and cancellation anchors. Maintenance retires state
even when canceled packets never reach their ordinary consumer. Equal text from
distinct native/user events remains distinct. Existing page/cast/phase guards
use native identity; text-content deduplication is not an allowed substitute.

`speech.cpp` and `prism_backend.cpp` serialize backend ownership, including
speech+braille and intentional braille-only output. Backend selection comes from
the current backend snapshot through F8. Voiced-dialogue policy belongs to the
dialogue producer, not a blanket speech filter. `log_capture.cpp`,
`log_history.cpp` and `speech_capture` retain producer-tagged evidence for
diagnosis. The Speech setting does not remove keyboard/menu control. See
[PRISM ownership, migration, dependencies and tests](../../prism_migration_20261012.md).

When adding output, capture on the real originating thread, carry owned data and
context into a plan/domain, and reject obsolete output at emission. Test cold,
busy, large and rapid actions plus reset/queue pressure. Asynchronous placement
alone does not prove a short audible response. Existing component measurements
and simulated SDK boundaries are labelled in their receipts; hardware/game/voice
timing requires user-side confirmation.

## Native text and localization

`game_text.cpp`, `text.cpp`, `message_text.cpp`, `owned_text_decoder.hpp` and the
ready-text adapters decode the actual native getters/order. `button_glyphs.cpp`,
`game_binding_input.cpp` and copied controls translate native logical action
tokens through configured keyboard/controller bindings. Do not replace a native
button with a guessed fixed key or expose invisible records as screen content.
The explicit pathfinding name exception is documented in Navigation.

`Polish text` is persisted as Auto/forced Polish for a verified unofficial font
and area-name translation. It is not official language support or a Polish mod
UI. Native game text follows the game's getters; mod-owned settings/guidance
remain largely English. A complete mod UI translation catalog and persistent
manual mod-language override are not implemented here. Native resource-specific
image labels must be verified for their locale; an English transcription cannot
be presented as verified in every language. See
[glyph evidence](../../swimming_controls_20261009.md) and the locale cases in
`polish_text_followups.inc`/`help_native_replay.inc`.

## Input and settings

`input.cpp` routes context-owned commands through a64-command FIFO, draining at
qualified native PInput/Help/field returns. Focus, generation, revision and
feature context retire obsolete requests. Keyboard bindings remain independent
of controller mod mode. `direct_input.cpp` preserves exact COM ABI, device
ownership, Peek/buffered semantics and Release lifetime. Foreign devices/threads
are not used as permission to mutate shared decoder state.

The October20 local developer console reserves Shift+F1 through queue pressure
and focus/console handoff, preserving plain F1. Existing chords default to
ordinary handback. [Design, bounds and regression evidence](console_f1_20261020.md).

`physical_gamepad.cpp` exclusively owns SDL initialization/device enumeration,
event pumping and snapshots. `controller.cpp` consumes copied states at native
input boundaries, preserving ordinary input and consuming only reserved commands.
Snapshots include discrete edges and current analog values; stale/failed owner
publication returns control to native input. The private2ms owner cadence and
10ms freshness guard are component design values, not a measured DualSense
end-to-end guarantee. [Controller and focus evidence](../../tester_report_20261015.md)
and [DirectInput contract](../../direct_input_ownership_20261012.md) describe the
native decoder routes and the original latency failure.

View/Select arms controller-only mod mode; face commands and binding capture
retain context ownership. Battle reserves right-stick read commands; Grid faces
mirror F2–F5 and triggers mirror cursor route keys. October18 Grid commands now
join the keyboard FIFO at native return, preventing an earlier snapshot phase.
Do not move ordinary pass-through movement into that queue.

`settings.cpp::options` is the authority for defaults/ranges/INI keys. F8's
`mod_menu.cpp` groups Speech options/Audio/Navigation/Miscellaneous, bindings, backend, active
trial, Gagazet and optional airship choices. Defaults include Autodetail Off,
menu indexing Off, Auto walk Off, hidden destinations Off and automatic
assistance Off. Manual puzzle/timing cues have independent On defaults.
Settings use atomic values and deferred persistence to `FFXAccessibility.ini`;
do not perform file I/O in a native callback. Extend the enum, options, menu,
capture/value presentation, docs and fixtures together. Details and controls are
context-owned: N reads entered text on the naming/password keyboard and native
help elsewhere. Speech options contains default-Off Speak punctuation names.
I reads current detail, Shift+I reads displayed controls, F7
changes Autodetail, F8 enters settings. [Player controls](../../controls.md) gives
the complete bindings and context exceptions.

Run `run_tests.bat --input`, `--controller`, `--sdl-owner`, `--speech`, `--abi`,
`--threading`, `--tail`, `--native-actions`, `--accessibility` as relevant.
`input_dialog_focus_fixture` separately covers root/child UI exclusion and
foreign-process focus. Changing producer ownership requires downstream output,
retirement and source-to-usable tests, not only a helper benchmark.

October20: F9/F10 now require a supported guide, and F11 requires naming, guide,
native menu, battle or Escape. Ordinary field F1–F5/F9–F12 pass through; F6–F8
remain reserved. [Ownership, native evidence and verification](../puzzles/airship_keys_20261020.md)
also documents the retained Shift+M search chord and Password subject. These
changes passed checks and guarded deployment with the Baaj repair; installer/package
remains held. [Current delivery](../navigation/baaj_seals_20261020.md#delivery-bounds-and-review).
