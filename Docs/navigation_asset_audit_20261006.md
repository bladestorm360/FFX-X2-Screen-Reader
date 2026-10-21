# Navigation model classification — 2026-10-06

The player's Thunder Plains travel-agency screenshot exposed two separate
errors: `ALBDBOOK011` matched the `ALBD` person-label pattern, and ATEL actor
type 1 was treated as proof of an NPC. An actor may instead be a book or prop.
The player explicitly authorized extracting character assets to verify this.

`Docs/Scripts/Offline/extract_navigation_models.py` is a repeatable read-only
VBF extractor. It validates archive header/hash/bounds, extracts original
DX11 meshes/textures into a dedicated workspace directory, records asset
SHA256 values, and generates `src/navigation/model_assets.json`. Texture
dimensions, format, payload location and mip counts come from the Phyre
namespace, instance and fixup descriptors. Nothing is searched in a game process
or written into the archive. Supported meshes also export OBJ; unsupported
layouts retain their originals and an explicit failure record.

Format attribution: the existing independent VBF reader references
[Topher's VBFTool](https://github.com/topher-au/VBFTool).
The independent texture decoder cross-checks descriptor layouts against
[ffgriever's dds-phyre-tool](https://gitlab.com/ffgriever/dds-phyre-tool).
The independent mesh reader cross-checks face/vertex descriptors against the
original Xentax reader preserved in
[tinkerbeast's ffx-ai](https://github.com/tinkerbeast/ffx-ai/tree/master/thirdparty_xentax).
FFXDataParser's existing attributed model enumeration supplies asset-family
names; those names alone never establish that a sprite is a character.

Retained evidence is under `Docs/Scripts/Offline/output/`:

- `model_assets_20261006_154301`: 2,342 original assets, SHA256 manifest,
  1,328 decoded textures, texture review sheets, and selected rendered meshes.
  The initial 27 ARGB8 textures were subsequently decoded from original files;
  no texture failures remain in the manifest.
- `model_assets_20261006_154654`: original `m208` Cactuar mesh/texture and
  three-view review image.
- `FFX/20261003_audit/navigation_audit_20261006_160545.json`: all 397 hashed
  script variants and 2,442 targets, including model IDs and identity evidence.

`render_model_review.py` uses actual vertices and UVs, sampling original texture
colors per face. Its three views establish shape and costume; they do not
recreate game lighting or animation. Character model coordinates use downward
Y, which is inverted for the review projection.

| Model | Actual asset | Verified interpretation |
|---|---|---|
| `500B` | `obj/f011` | Book; never Al Bhed or NPC |
| `5002` | `obj/f002` | Treasure chest; never Villager/Crate |
| `5010` | `obj/f016` | Mechanical sphere device; object, identity left general |
| `2050` | `npc/n080` | Al Bhed character |
| `207A` | `npc/n122` | Soldier character |
| `207E` | `npc/n126` | Priest character |
| `20EA` | `npc/n234` | Rin; own actor-local speaker text supplies his name |
| `10FA` | `mon/m250` | Belgemine's human body, confirmed with her local dialogue |
| `10D0` | `mon/m208` | Cactuar character; never Villager |
| `5033` / `5034` | `obj/f051` / `obj/f052` | Chocobo / armored Chocobo characters |

A character need not be human or stored in the NPC folder. The named exceptions
above are visually confirmed whole character models. Eye-only overlays,
white dummy proxies, missing models and untextured placeholders do not prove
an NPC. Unverified interactive actors use Object, retaining navigation access.
Actual live model class vetoes stale NPC catalog identities. Strong native
script functions and commerce roles remain usable for objects and events.

Current catalog: 907 NPC, 648 Exit, 296 Event, 246 Treasure Chest, 200 Object,
105 Save Sphere and 40 Shop targets. Every NPC has character-model evidence.
`kami0100` workers 9, 10 and 16 are Objects named Book. The first worker's own
text is the screenshot's copy of "All About Lightning." This is concise
destination identity, not a generated interaction prompt.

Validation logs: `install/model-identity-tests-20261006.log`, offline identity
prototype checks, and final native readers in both executable profiles. Native
fixtures cover actual book scanning, Objects membership, absence from NPCs,
unknown-model behavior, character/creature exceptions, and stale NPC labels.
Live acceptance is pending player testing.
