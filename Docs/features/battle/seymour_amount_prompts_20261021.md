# Battle mechanics, amounts, confirmations and Formation — October21

Status: implemented, offline verified and guarded-deployed in1.0 after the user released the hold. The verified single-installer ZIP is published as Latest in the fresh public GitHub repository; see Docs/release.md. Gameplay confirmation is pending. The earlier staged receipt and initial hold below remain historical evidence. The four completed minigame assists remain included.

The actual current log is beside FFX.exe. The sandbox exposed a stale empty copy and no process; the real read-only host view found the active session and454KB log. Logging works. The log confirms Mortiphasm100%, Spare Change without an amount, and at12:25:37.952 a learned-mode message missing both names. Use the real host view for own-session evidence and process closure checks. No Tester Logs, gameplay control, launch or runtime probe was used.

## Mortiphasms and Seymour Omnis

`battle.cpp::targets` remains the sole target producer. Each Mortiphasm target adds its current energizing element and the mechanic: physical attacks turn left, magic turns right. Seymour's target adds the count of wheels feeding each element. Counts describe current wheel positions; they do not promise a next cast or claim his affinities have already been recalculated. The AI counts wheels before choosing its action.

The user approved read-only installed Seymour/Mortiphasm battle assets. The bounded extraction retained five monster records and155 wheel visual records,4,362,693bytes, outside Git. Same-model alternatives118/150/154 have different scripts/HP. Model identity alone cannot qualify this encounter. `monsterScript` requires active actor20/model1083/script131 and four active actors21..24/model106A/script106. It reads only four script-ID bytes at a bounded header offset. `seymourWheels` copies four signed positions0..3 and maps them to the elements used by original m131. Invalid/missing state omits the mechanic. Only the selected target gets added text, bounding multi-target plans.

| Native reference location | Verified behavior |
|---|---|
| m106 worker0 entry0113 | Resolves actor21..24 to four hit paths; physical/magic types set direction and wheel mask |
| m106 worker1 | Reads/updates shared positions4/8/C/10 for wheel animation |
| m131 worker1 entry0193 | Counts each wheel's element, sets elemental properties and chooses actions |
| VA7983A0,298bytes;875650 | Supplies battle shared storage1134568 to controllers2/3/4 at controller+30 |
| VA86C270 | Location1 declarations resolve through that shared base |
| VA783BB0,132bytes | Binds original monster ATEL at actor+F78 |

Production uses qualified RVA D34568 rather than assuming the current interpreter is the battle controller. Native pointers stay on their owner. A packed four-position key extends the existing window/owner/target-mask guard, refreshing the same target after rotation. No-target/battle reset retires it. The callback checks native ownership/generation. There is no polling, worker pointer dereference, text deduplication, game write, setting or new control.

`seymour_inventory_20261021.py` reuses FFXDataParser chunk/declaration layouts and the existing ATEL compiler. Java's dynamic accessor argument counts are explicitly qualified for this report; generic TSV counts were zero. A bounded interpreter executes the original m131 entry for all256 position combinations, recording1024 wheel contributions and aggregate counts. Native property setters are recorded boundaries. The3072-byte matrix and original106/131 headers feed `tests/seymour_followups.inc` through production target speech. Native VM/frame/animation scheduling and audible gameplay remain unmeasured.

## Spare Change and Yojimbo

`battle.cpp::amountDrawn` observes the return of actual renderer RVA4A3250. It qualifies current battle window, phase4, type15/16, owner0..17 and the non-automatic branch. It copies the localized label at window+B4, amount F40424 and selected digit F40418. Amount is bounded0..999999999; digit0..8. Speech includes amount and decimal step, so left/right feedback changes even when the amount stays equal. Window lifecycle/battle reset clears its bounded identity guard; failed queue admission remains retryable. No timer or numeric-input mutation is added.

Original VA8A8780,618bytes, handles decimal movement, wallet cap, zero clamp and confirmation. `tests/battle_amount_followups.inc` replays it before feeding the production renderer observation. Sound, validation, window-pop and security-cookie calls are controlled boundaries. All nine digits, cap/clamp, both pay types, idle/reentry and automatic guards are covered. A1000-event unchanged-render measurement records callback time and zero heap activity; graphics/input scheduling/audio remain unmeasured.

## Escape parameter confirmations

`esc_details.cpp::parameterBefore/After` observes native three-Iggy-value wrapper RVA28DF70 only for the real Escape movie and exact `showPopUpWithBtn` method. It copies the localized prompt/Yes/No. Three16-byte values are passed by value: type tags are argument words2/6/10, UTF8 pointers4/8/12, lengths5/9/13. The tail is read from the original return-slot stack pointer beyond Event's twelve-word prefix. Tags must be5; lengths1..512; native owner/generation must match on return. The wrapper owns/frees none of these caller strings. Only copied bytes survive.

`esc_capture.cpp` emits prompt then actual default No in order. Accepted native AS3 `change` sounds transfer Yes/No focus; click/cancel, visibility closure and reset retire it. Each highlight speaks its option. Quit retains its existing initializer; the generic wrapper skips its parent to avoid duplicates. Obtain items/unlock skills/max-gil use the three-value route. Five-value result notifications retain their separate producer. Manual I retains the prompt independent of Autodetail. Multilingual copied strings and owner/method/tag/epoch/thread guards are covered in `results_esc_followups.inc`.

## Formation

`field_menus.cpp::mainKey/mainAfter` copies the exact three-byte front-party roster through cdecl RVA385330 (optional count pointer null). It compares the highlighted native character ID, not row number. Phases0E/14 append active party/reserve;255 is an empty slot, other IDs outside0..17 omit classification. Same-cursor membership changes refresh. Original description/I/Autodetail are retained. The getter's full23-byte body and roster operand are profile-qualified. `formation_active_followups.inc` covers both phases, actual swaps during native callback execution, non-first-three active rows, empties and invalid IDs.

## Learned Overdrive modes

`battle_announcements.cpp::battleOpeningMessage` now uses `planMessage` in renderer namespace7, copying both substitutions before later messages reuse them. `battle.cpp::message` uses that namespace for direct banner1. Banner index0/1 and substitution namespace7 differ. Original VA893560,104bytes, registers native character text in slot0 and mode text in slot1 (namespace1/id1033+mode), obtains template300F and submits banner0 via89DB10. The old raw encoded copy dropped both operands. No English matching, invented names, deduplication or duplicate learned-mode hook is used.

`overdrive_learn_followups.inc` replays that104-byte initializer, its namespace registration/order and production expansion/worker path. Text getter/registration/renderer boundaries are controlled. It checks two characters/modes, rapid namespace reuse and equal independent events. Existing opening formation protection/order and direct action banners remain covered by the reader suite.

## Evidence, extension and limits

Originals/game-bearing reports stay outside every Git tree under `../Analysis/FFX/RepositoryEvidence/offline-output/seymour_20261021` (manifest, original monster/visual files, reports, matrix proof/headers), and `../Analysis/FFX/RepositoryEvidence/offline-output/battle_amount_prompts_20261021` (bodies/fixups/native locators). Parser/opcode attribution is Karifean/FFXDataParser; VBF reader attribution remains in its module. Production retains mod code, factual IDs/locators/mappings and masked signatures only. Qualify exact scripts/visible state before extending mechanics, and actual ABI/owner/focus lifecycle before extending popups. Regenerate `native_extensions.py`; run both reader profiles, profile/dependency, ABI and relevant owned-worker checks.

Native strings retain game localization. Added mechanic/party/step labels remain English under the existing mod-wide catalog/manual-override gap; complete mod UI localization/Polish UI is not claimed. Existing broader HRTF/platform gaps remain in the feature register. Native speech planning is bounded and nonblocking, with worker formatting; this alone is not end-to-end responsiveness proof. Runtime data age/frame-to-audio latency and rapid real gameplay remain unmeasured.

Luna gpt-6-luna/medium supplied bounded popup and Formation edits. Sol found the popup by-value ABI error and Formation fixture array/event-timing errors, took over corrections and owns integration/verification. No unreviewed delegated work is accepted; no Astra was used. Exact final check results/build/delivery status belong in `install/battle-prompts-20261021/verification.json`.

Final offline verification: both173-hook reference/installed reader profiles pass all256 original m131 position cases/1024 colors, numeric618-byte and learned-message104-byte replays, Formation membership, popup ABI/focus and regression cases. The real held-worker popup transport passes W4/WX source reuse, zero native heap/destruction, queue pressure, equal events, prompt/focus order and cancellation. Signature/dependency and11000-call native ABI hosts pass. The release-flavor production build is warning-free; both installed FFX/Will executable-image preflights pass without running their entry points, speech or hooks. All prior masked function signatures remain unchanged; four are added. Existing unrelated reader fixture C4389 persists. See the receipt for comparable native callback measurements; real frame-to-audio latency remains unmeasured.

At the initial October21 checkpoint the user chose to keep playing and held deployment; no running modules were replaced. The later final1.0 request superseded that hold. The current-source release build is guarded-deployed with all7 installed/staged/embedded hashes matching, and the actual installer/ZIP passes offline checks. Settings and prior0.5.7/0.5.8/0.5.9 packages are preserved. No game launch/probe or registry repair occurred. Current evidence is in `install/release-1.0-20261021`; future deployment still requires closure/source/payload checks through the guarded route.

