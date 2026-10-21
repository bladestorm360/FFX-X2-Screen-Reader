# Field, MP and Wakka follow-up — 2026-10-09

Later own-session evidence identifies the missing scene-selected Wakka697-698/
691-696 variants and correct Aurochs success command. The Wakka discussion
below describes the preceding build; see [the definitive repair](wakka_detection_20261009.md).

Sol performed investigation, implementation, review and offline verification
alone. The user confirms Auron auto-success and manual sequence announcements
work. These confirmations supersede the earlier pending status.

## Title-screen arrival

The user's own mod log contains a false Entering Zanarkand Ruins at06:21:08.969,
generation9, followed by the actual Thunder Plains arrival at06:22:08.784/gen15.
No Tester Logs were accessed. Native scene initialization48DFE0 and a saved
room/native area name alone cannot prove that a field player has loaded.

Retained871A50/Common005D selects the active type1 worker's bound actor through
82DB50 and enables controller bit2. It can also enable bit2 while binding null.
871980/Common005E removes that binding and disables control. No SDK enum named
ActiveField was found; the shared activeField predicate expresses these verified
semantics and requires actual F00740 membership/alignment in the actor array,
enabled/initialized state, finite XYZ, workers and a valid primary chunk/entrance.
Navigation's fallback actor is intentionally insufficient for arrival speech.

A completed outer initialization stages generation, room, entrance and chunk.
Control-enable return or existing actor mutations finish it once readiness is
true. Native primary-chunk map getters supply the label; automatic arrival does
not use the saved-room fallback. Teardown or identity mismatch cancels the
candidate. Every later real transition can still announce identical text.
No timer, polling, text deduplication or new label source was introduced.

## MP and Overdrive bar

The screenshot displays current MP12/33/10 for Tidus/Auron/Wakka. Native battle
panel current MP uses39AE60. H defaults to current MP through the central
battleMpReadout helper. F8 > Miscellaneous > Speak maximum MP defaults Off and
persists independently of Show maximum HP. Other native current/maximum MP
menus retain their existing presentation.

H silent follow-up: activeStats now uses only the shared activeCharacter guard.
Missing/closed battle windows, enemy turns, inactive actors and outside battle
return without output. Both reader profiles verify these quiet cases and retain
active-character/summon readings and independent HP/MP preferences. Follow-up
delivery proof is stored in install/h-silent-20261009/verification.json.

O announces the active character or summon's Overdrive percentage. Renderer
492880 calls395560/current and3955A0/maximum, which read actor+5BC/+5BD, and
scales its fill by current/maximum. O uses these same native getters, rounds to
the nearest percentage and caps at100. No turn, enemy turn, inactive character,
missing maximum or outside battle produces no meter speech. A central battle
chord preserves the separate existing Sphere Grid orientation context.

## Wakka

The own-session log records Wakka: Element Reels at06:26:34.812 but no Wakka
callback-installed/refused message. The old fixture always called obtained on
the game owner, so it did not cover an asynchronous/foreign getter. The precise
reason attachment was skipped in that session is not established offline.

Retained788540/per-character and788E20/global start both call788EB0/MsMakeEffId
immediately before invoking table[0]. A return hook at388EB0 supplies the actual
character+F8C or globalD33360 table on the native owner thread. Existing PE-base,
exact file hash, callback, generation and context qualification still apply.
One event diagnostic records the packed handle and table for user-side retesting.
No module entry point or native game callback ran in agent testing.

Attack Reels are identified by all three actual patterns containing double-hit6
and only blank4/single-hit5/double-hit6. Auto-success applies triple6 immediately
through the central accepted-position helper also used by the match menu.
Native continuation calculates the result/hits. Element, Status and Aurochs
retain paused valid-match choices. Manual Attack Reels cues emit for5/6 and
cancel on4; other symbol pitches and native traversal guards are preserved.

## Evidence and verification

- Retained main decompile: layer2_game_logic.txt871A50/871980,788540/788E20/
  788EB0,795560/7955A0,892880; native module callback exports and actual copied
  magic_0565/magic_0644 PE data from the earlier authorized extraction.
- Full-body signatures and D33360 instruction operand:
  native_extensions_20261009_134256_346864.json. Actual supported executable
  identities qualify every mapping; missing dependencies refuse startup.
- Offline Frida prototype field_reels_followup.js and revised map_transition.js;
  check_field_reels_followup.cjs executes only pure rules, never a live probe.
- Both reader profiles verify deferred/silent title cases, real/repeated arrivals,
  invalid actors, teardown, current/optional MP, active summons and O no-op cases.
- Both actual reel layouts verify global/per-character starts, a foreign getter,
  native initialization transition, all match types, direct Attack match, quiet
  blanks, exact clock resume, ownership/cancellation and callback restoration.
- Focused menu persistence, profile refusal, resource threading, keyboard and
  controller routing, audio cancellation and fresh proxy checks pass. A fixture-
  only stale MP expectation, added menu row count and mixed-auto declaration
  were corrected before final validation.

The warning-free native x86 and launcher build is deployed through the canonical
closed-process guard. Both actual compiled profiles, launcher bootstrap and all
13 installed SHA256 hashes pass. Logs and identities are retained in
install/field-mp-reels-20261009/verification.json. Gameplay acceptance of these
changes remains pending. No game launch/control, live probe, Tester Logs,
registry repair or release ZIP change was authorized or performed.
