# Save-sphere-only map audit — 2026-10-09

Sol investigated, implemented, reviewed and tested alone. The user reported a
tester finding an Underwater Ruins save sphere named Inscription in Objects,
then requested the same scoped check across other maps. The quoted report was
used as evidence; no Tester Logs or installation assets were accessed.

The normal save sphere is model 0x5001. The underwater variant is model 0x50AB
(retained native archive object f171). The existing model classifier recognized
only 0x5001. The underwater worker therefore became Object, and its save tutorial
dialogue's reference to an inscription supplied the incorrect functional label.
Actor-local native text explicitly identifies Traveller's Save Spheres; the
model was initialized by native ATEL model call 1. This is not inferred from a
generic occurrence of the word sphere.

The retained 397-map catalog has 108 unambiguous save-sphere targets. Exactly
three had the alternate model and incorrect category/label:

| Map script | Worker | Previous label/category | Correct label/category |
| --- | --- | --- | --- |
| cdsp0700 — Underwater Ruins |12|Inscription / Object|Save Sphere / Save Sphere|
| stbv0000 — Bevelle, Via Purifico |16|Inscription / Object|Save Sphere / Save Sphere|
| stbv0000 — Bevelle, Via Purifico |17|Inscription / Object|Save Sphere / Save Sphere|

`navigationSaveSphereModel` is the central native classifier. Both category and
name use it before stale catalog labels; scanning uses it for native object
priority and unrestricted current interaction callback selection. Actual bound
actor identity, enablement, remapping and native callback availability still
govern inclusion; an unbound inscription remains an inscription. A genuinely
disabled save sphere remains omitted.

The offline generator uses shared `SAVE_SPHERE_MODELS` in identity_labels.py and
finish_targets.py. Regeneration changes exactly these three target labels and
the alternate model's generic label. All 3,189 target memberships, callback lists,
models and every other target label/category/identity are preserved. A mixed
developer model-selection worker in test25 is recorded separately: its actual
runtime model supplies classification, rather than assigning all its selectable
objects to Save Sphere.

Evidence: `Docs/Scripts/Offline/output/save_spheres_20261009_182647_725707.json`
and `Docs/Scripts/Offline/output/FFX/20261003_audit/navigation_audit_20261009_182618.json`.
`audit_save_spheres.py` verifies all map variants, save source hashes, exact
changed rows, authored native text and C++/Python classifier agreement. Existing
identity tests pass. Both native reader profiles pass the alternate-model scan,
remapped callback, disabled save and stale name/category tests. Final build and
delivery details are in `install/status-wakka-followup-20261009/verification.json`.
Gameplay acceptance remains user-side.

Final delivery: both tester/developer native builds pass; a fresh final proxy
passes the offline forwarding fixture. Guarded deployment validates closed
processes, compiled FFX/Will profiles, launcher bootstrap and all 13 installed
hashes. The separately verified tester EXE is `install/windows-installer/InstallFFXScreenReader.exe`, SHA256 `e31263e2c30ab5a10df4d61a805951bbe1eb77d7b109e63dddb6eb030c08518f`.
The existing release ZIP is unchanged; gameplay acceptance remains user-side.
