# Airship shortcut, function keys and password readback, October 20

Latest delivery: the user released the deployment hold when requesting the Baaj
fix, then deployment. The combined current-source build passed canonical checks
and guarded deployment. [Baaj/current installed receipt](../navigation/baaj_seals_20261020.md#delivery-bounds-and-review)
supersedes the historical held-build status below. Installer/package remains
held; actual gameplay/audio confirmation remains pending.

Implementation status: **complete** for the scoped keyboard/password changes.
Offline verification: input, accessibility menu and both 171-hook reader
profiles pass. Gameplay confirmation: pending. The changes are now guarded-
deployed with Baaj; installer/package work remains held. These accompany the separate
[Free Search audio repair](airship_audio_20261020.md).

## Controls and context ownership

The user's final instruction keeps **Shift+M** for the hidden-destination picker;
the earlier F3 request is superseded. `airship_search.cpp::installAirshipSearch`
already registers that chord through `hotkeyChordWhen`. Its existing
`hiddenAirshipMenuAvailable` predicate requires the default-Off Hidden airship
destinations setting, the current search session and the exact prepared
`hiku2100` identity. No production shortcut change or second producer is needed.
M still reads the copied displayed coordinates. Both entry paths depend on the
same native active-search flag, rather than one menu's renderer room number.
README and the player controls now give the shortcut and setting explicitly.

`mod_menu.cpp::installModMenu` formerly registered F9, F10 and F11 globally.
`guideKeysAvailable` now admits F9/F10 only in the supported Cloister, Via or
Gagazet guide context, outside battle and another native menu. Cloister
qualification uses `captureUtilityScript` and `captureCurrentTrial`: bounded
native-owner reads and an already prepared identity/checksum receipt. It does
not parse or checksum the script in a key predicate. F11 is admitted for entered
text, those guides, native menus, battle and Escape. Existing handlers retain
their naming-first, guide-next, speech-repeat-last order.

| Keys | Ordinary field ownership | Context exception |
|---|---|---|
| F1 | Native game | Existing qualified minigame controls; development-only shifted console binding |
| F2–F5 | Native game | Existing Sphere Grid or qualified minigame controls |
| F6 | Mod | Beacon toggle; existing Grid number entry/minigame override |
| F7 | Mod | Autodetail toggle |
| F8 | Mod | Accessibility settings |
| F9–F10 | Native game | Supported active guide only |
| F11 | Native game | Naming/password, guide, native menu, battle or Escape |
| F12 | Native game | No ordinary-field mod binding |

An open modal mod menu or binding/number-entry dialog retains its existing
capture rules. The ordinary-field statement excludes those deliberate modal
contexts. Native assignments beyond the user's F1–F5 report were not guessed;
passing unused F9–F12 through preserves any native use. `input.cpp` checks the
predicate both when deciding capture and again when dispatching queued input,
so a guide key queued before leaving cannot act in the next context. No new
keyboard mode, polling, input thread or native pointer transfer is introduced.

## Password subject and native evidence

`naming.cpp::after` already copies up to 32 four-byte native cells from the
current keyboard. It now copies a password-subject flag alongside those cells.
The native selector at RVA146A218 is -1 for the airship password descriptor;
ordinary character/Aeon naming uses 0–17. The read-failure sentinel is distinct.
The password subject additionally requires the airship renderer or the
qualified primary `hiku2100` header, preventing a change to unrelated naming.

Original initializer `FUN_008c4230` (RVA4C4230) normalizes selectors outside the
ordinary range to -1 and chooses native title ID200C. Ordinary naming uses
IDs200B/204E. Descriptor getter `FUN_008c4f60` selects the separate negative-index
descriptor. These facts come from retained original bodies and the existing
qualified profile operand, without a new hook/address dependency.
`airship_map.hpp::airshipSearchScript` shares the fixed header/name qualification
with the audio/search predicate; password entry does not require Free Search to
be active. The actual cell descriptor and focus remain owned by the original
naming hook, not a room-only reader.

The speech worker formats **Password: …** or **Password is empty** when this
copied flag is set; ordinary names retain their current subject. N and contextual
F11 use the same producer. `namingReadbackAvailable` exposes only the existing
active/current-generation test to input ownership. Workers receive no native
window, descriptor or script pointer. Existing session/generation retirement,
domain bounds, punctuation switch and help/details priority remain in force.
The mod's existing English-only UI-label gap also applies to these two subject
strings; native entered glyphs still use the current copied language context.
No setting, resource, dependency or default changes.

## Verification and safe extension

`run_tests.bat --input` exercises actual production Windows-message and
DirectInput paths: F1–F12 down/up/state ownership, inactive Grid/guide guards,
guide exit with queued keys, Peek/buffer behavior and existing COM lifetime.
`--accessibility` uses actual mod-menu registration/handlers and checks field
handback, guide stepping/readback, menu/battle/Escape repeat and naming priority.
`--readers` passes both supported 171-site profiles. Native naming snapshots
cover password/non-password/empty text, renderer382 and the original hiku2100
header without that renderer value. Shift+M is checked against the actual
registration, setting Off, closed search and both renderer values. The controlled
non382 case is an adversarial fixture, not actual second-path gameplay evidence.

The canonical release/launcher build is staged with `--no-deploy`. Logs and
current hashes are in `install/airship-keys-summon-20261020`. The reader host
retains the existing C4389 fixture warning; the production build is warning-free.
These hosts simulate native scheduling/input/text services; they do not measure
hardware latency or establish actual-game ownership at every field transition.

When extending keys, register an availability predicate using the real prepared
context and preserve the central input dispatch recheck. When extending naming,
copy a subject fact at native capture rather than reading native state on the
speech worker. Test ordinary-field handback and queued context exit alongside
the new screen. After the hold is released and the build deployed, first check
Shift+M from each real Free Search entry path, N on password entry versus ordinary
naming, and the game's F1–F5 field toggles. No progress reset is required.

Luna (`gpt-6-luna`, medium) handled the bounded Shift+M fixture/document update,
then the verified function-key/password player documentation. Sol retained
discovery and all production changes, reviewed the edits and validated the real
input/menu/readers. No unresolved issue remains in the delegated edits. Actual
gameplay and the separate reported Proceed?/Y investigations remain outside
that result; no Astra delegation occurred.
