# Zanarkand displayed patterns and Help arrow clarification

Active user requirements: Tab > Help information pages are arrow-review buffers
and honor Autodetail; I remains optional. “Battle Mode” is the title in the user's
Help screenshot, not the game's battle state. All work is on out-of-battle Help.
New original-renderer regression fails the old zero-link arrow guard; corrected
help.cpp and Help Move packets pass both154-hook profiles. I retains the whole
page after arrow review; returning to the heading does not replay all details.
Logs: install/help-arrow-autodetail-before-20261015.log (expected failure),
...after... (fixture initially omitted its worker drain), ...reviewed... (PASS).
Luna gpt-6-luna medium completed only the established real-worker Help fixture.
Sol corrected its initial assumption that I should survive a later selection
token, retained the current-selection rule, and took over the fixture sequence.
The final held open/Down, current I, then held Up case passes with zero native
heap allocations/frees. Close/reset cancellation and real native menu links
remain covered. Log: install/help-buffer-worker-final-20261015.log. Native code
and all discovery/integration/verification remain Sol-owned; no Astra.

The user also requests reading Zanarkand's interactive glyph displays and
distinct floor labels matching their visuals. Installed Zanarkand trial graphics
and map reads were explicitly approved in this task, separately from Besaid.
No launch/control/probe or Tester Logs access. Installer/ZIP remains held.

## Original evidence

Retained43 authorized payloads under
`Scripts/Offline/output/help_arrows_zanarkand_20261015/zanarkand-assets`:
mapout.vpa, dome07.ahwin32 and41 original FP textures. The manifest includes exact
archive names, hashes and permission. Original texture decoder attribution and
format validation are in phyre_texture.py. The contact sheet was visually read.

Original loader9097C0 uses MAP1+38 for FP;72A8E0/712080/7121D0 establish108
0x50-byte records,108 linked PPP programs,120 model descriptors and relocation
layout. The two paired GT models for every glyph independently agree in cell
coverage; model header/FP counts and pointer offsets, face indices,100-unit grid
spacing, triangle area and filled-cell area are cross-checked. PPP80 DrawMdl3
selects the model; PPP11 Color/75D2E0 gives its RGB increments. Exact native
worker57..83 each call one original startGfxTimer ID, connecting floor trigger to
its matching graphic. Native interactions and walking geometry are unchanged.

27 verified glyphs: orange T/four squares, yellow line/four, cyan zigzag/four
(including its mirrored form), blue L/four, red corner/three and white square/four.
Existing panel numbers worker-55 remain distinct and are included in both display
and floor names. No directional remapping or invented solution order.

Original monitor workers53(first chamber) and54(main chamber) entry2 display the
patterns.53 shows five glyphs.54 branches on native stage1..4 and shows6/7/9/7;
other stages return without a sequence. Native camera/GFX waits belong to the
game; mod must not delay, pause or synthesize controls. Existing bounded ATEL
trace records the actual original call order and final RET; SDK calls remain
explicit stubs. Script SHA256a530da97346bc9d9bee90ab5239663d283d128d13203fe3e55bc4e308c1d29c6.

Audit/generator: `Scripts/Offline/prepare_zanarkand_patterns_20261015.py`.
Report: `Scripts/Offline/output/help_arrows_zanarkand_20261015/zanarkand-glyph-evidence.json`.
Human-readable source metadata: src/puzzles/zanarkand_glyphs.json; generated
native data: src/native/zanarkand_glyph_data.hpp. No raw assets ship in the mod.

## Implemented behavior and ownership

`zanarkand_patterns.cpp` observes the existing qualified469040 initialization and
469170 completion hooks; no new address or polling producer. Entry2 on exact
registered worker53/54 starts a candidate. Exact worker/signal/chunk/allocation/
epoch, original entry PC, native level and stage must agree. Original864180 RET
calls869170 before its ordinary PC increment, so the completion PC must point
at the verified final RET itself, not RET+1. Early cancellation, changed stage,
signal entry, allocation replacement or generation retirement cannot publish.
The new module does not write game state, request actions or arm a solver.

Final review also retained869170's next-queued-signal initialization semantics:
the new display can start inside the previous completion call. A native-owned
display serial captured in that listener's Event prevents the earlier leave
from publishing or clearing the newer candidate, including identical worker/
signal/entry/allocation reuse. The regression explicitly stages that nesting,
rejects the unfinished new display, then accepts its own later normal RET.
The first canonical build is retained in
`install/help-arrows-zanarkand-build-before-display-guard-20261015.log`;
it was never deployed. Final source is rebuilt after this guard.

Only a completed original display populates retained owned data. Autodetail On
reads the full sequence after that animation completes; Off announces the display
title and leaves the sequence available to review. F8 > Miscellaneous > Cloister
of Trials > Review displayed glyph sequence opens an Up/Down list; I reads all
entries. No completed display gives a concrete interaction instruction. Leaving
the map or retiring its allocation invalidates the review. Other trials retain
their original seven-row menu. Floor targets, automatic reading, review rows
and the manual guide share the same verified glyph metadata.

All pending native identities remain on the serialized native owner. Workers
receive ready copied UtilityText/TextPlan data; the menu receives at most nine
owned worker numbers. Metadata is fixed27 labels/five sequences; native work
has constant bounds, no waits, locks, timers, worker calls to game APIs or heap
formatting. Longest automatic sequence uses20/24 UtilityText parts. Reading
does not change the existing first/return solver or original navigation rules.

Catalog updater changed only dome0700's records (44 to46, adding the displays)
and shifted chunk/start indices. All other scripts' records are byte-identical.
Report: Scripts/Offline/output/help_arrows_zanarkand_20261015/catalog-change.json.

Help's zero directional offsets were being resolved to the SPS2 header and
misclassified as native menu links. The real page fixture now uses a nonzero
base and verifies all four content arrows. Help Move reads one row and retains
the whole-page I detail. A same-page accepted arrow can replace the detail token
before worker assembly; a page-owned detailSerial allows that transition while
still rejecting an external popup/reset/close. No text-content deduplication.

## Offline verification and remaining gameplay checks

Original ATEL/paired-model/color geometry audit passes all27 glyphs and five
display orders (5/6/7/9/7). Development/release utility fixtures cover those
orders, AutoDetail On/Off, nine-glyph capacity, no game effects, one completion
event, unseen/early/stage/allocation/entry/epoch rejection and catalog parity.
Actual F8-key/menu fixture covers absent data, arrow rows, whole-sequence I,
Back focus, retirement and unaffected other-trial menus. Real status worker
and original Help page renderer fixtures cover arrow buffers and Autodetail.
Final logs: install/help-arrows-readers-final-20261015.log (both154-hook profiles),
install/help-buffer-worker-final-20261015.log (held production worker),
install/help-zanarkand-accessibility-20261015.log (F8/key/menu), and
install/zanarkand-pattern-utilities-completion-final-20261015.log (development
and release utilities, including same-identity nested signal initialization).
The utilities fixture retains the earlier test-only C4459 shadow warning in
utility_choice_replay.inc; no new warning remains in the added glyph fixture.

Gameplay confirmation remains user-side. Priority: open Tab > Help with
Autodetail On without pressing I, then review with arrows and optionally I.
When Zanarkand progression permits, interact with each pattern display, let its
native animation finish, then compare automatic speech and the F8 review with
the named floor targets. Repeat a main display after advancing a pedestal; only
its latest completed pattern should remain. Cancel/leave during an animation:
no incomplete or old pattern should become readable. Actual game/backend timing
and speech are unmeasured; these offline checks do not establish runtime latency.

## Verified delivery

Final canonical development build `src/proxy/build_dinput8.bat --no-deploy`
passed with no compiler warnings/errors. All556 recorded source hashes agree
before/after the final build. Native proxy and original launcher rebuilt;
Shift+F1 console remains enabled. Guarded `src/proxy/deploy.bat` passed Preflight
and Installed checks with FFX and Will closed. No game entry point was run.

All seven installed artifacts match staged hashes; locked PRISM receipt/15 files,
launcher bootstrap, unchanged predeployment INI and unchanged held October14
installer/ZIP pass. Proxy SHA256
`65e175d97aaf9adfc9511a996074bf02b26f69b7d38123405757fad72de22e08`.
[Final receipt](../../Analysis/FFX/RepositoryEvidence/historical-install/help-arrows-zanarkand-deployment-verification-20261015.json)
links the build/deploy logs, source snapshot and final offline checks.
Previous help-navigation-trials receipt remains historical. Gameplay confirmation
is pending; installer/ZIP rebuild remains held. No launch/control/probes,
Tester Logs access, real registry repair, publication or prior-build restoration.
