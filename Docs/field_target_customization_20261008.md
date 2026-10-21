# Field targets and customized equipment — 2026-10-08

## Native evidence before implementation

Reused completed analysis; no game launch, probe or analysis execution.
`Docs/Scripts/Offline/output/native_port_20261003/FUN_008c6080.txt`
case 7 constructs the shared 4C9210 target window for Abilities (scene 2).
`FUN_008c5420.txt` constructs the same window for Items (scene 3).
The reader currently admits only scene 3, explaining omitted spell targets.

`Docs/Scripts/Offline/output/ui_followups_20261005_193943_991507/field-target-status.txt`
contains 4C7400 input and 4C8080 renderer bodies. Phases 4/5 are initial/active
focus. +58 selects native party getter pair 4A9AE0/4A9A80 or 4A9BA0/4A9B70;
146A351 zero means all party, one means highlighted row. +5C equal to one
renders MP through 4A9920/4A9960; other modes render HP through 4A9870/4A9940.
No battle actor data is appropriate for these saved field values.

`Docs/Scripts/Offline/output/native_bodies_20261003_135557/FUN_008d5800.txt`
is Customize's equipment controller. State C accepts its confirmation; cancel
or the nonzero answer returns to state 6. Success executes state D in that call:
4D5650 adds the ability, 3993C0 updates the equipment, materials are consumed,
the current equipment name is fetched again, and the controller enters state A.
The selected equipment ID comes from the U16 array at 146A9F8, using the gear
window's +48 row. A fresh 3ABE10 ID getter after that completed transition is
required; the inventory ID and cursor can remain unchanged after the rename.

The referenced Docs/GhidraReference.md could not be found at project/source
roots; existing completed bodies were read only. Primary is checking shared
reference location. No new scripts were authored or executed.

## Intended changes and acceptance

Admit both native target scenes, preserve native order/resource semantics and
event identity, use the central HP formatter for the user's percentage/maximum
preference. Customized gear result reads the current getter after success,
independently of automatic detail. Cancel/unrelated windows remain silent.
Offline fixtures and gameplay acceptance are tracked separately below.

## Implemented source and review boundaries

`src/native/field_targets.cpp` now admits scenes 2/3 and copies the selected
native party IDs and displayed current/maximum resource values at callback
entry/return. An unchanged focus with a native heal or native party-order
change therefore refreshes; an idle callback remains silent. The phase 4-to-5
transition retains the existing single initial-focus announcement. This is
producer state tracking, not speech text deduplication or passive watching.
HP calls central `hpReadout`; MP preserves the panel's current/maximum display.

`src/native/field_menus.cpp` uses 4D5800 entry states C/D and completed result
state A, captures the selected native gear ID, then fetches `3ABE10(id)` fresh
on return. Fresh abilities are available to manual detail without adding
another automatic ability producer. No cached gear name is used.

`src/native/tests/field_menu_fixture.cpp` is an include fixture for the existing
production reader harness. It covers spell initial focus, phase transition,
native party IDs/order, highlight/all-party, HP preference, same-target heal,
MP-only items, reopening/invalid IDs, successful renamed equipment with an
unchanged ID/row, cancel/idle/unrelated scenes. Primary must integrate its
include/call and old HP expectations. Primary subsequently integrated these
and reports both production reader profiles passing. Primary proved 4D6D30
installs control callback 4D5800 and stores the identical gear window at
146A9F0, so 4D5800's argument row is the selected gear row. The completed
integration derives a unique 130-byte archived signature and global operands
at offsets 13/41/93; both profile fixtures pass. These are primary-verified
integration results, rather than claims based only on decompile names.

Gameplay confirmation remains user-side. No deployment/publication or runtime
operation was performed by this delegated task.

Primary final integration is now deployed in the combined development build.
The full suite reports 39 PASS summaries; the final affected reader checks
report 10 summaries across both profiles, with no compiler warnings/errors.
The final build, offline proxy guard, both compiled deployment profiles,
launcher bootstrap and all 13 installed hashes pass. No game was launched or
Tester Logs accessed; actual spell/item/customization acceptance is pending.
Final evidence: `install/field-butterfly-20261008/verification.json`.
