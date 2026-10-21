# Trial routes, map labels, Blitzball review and Auto walk

Implemented and checked offline on 2026-10-07. Gameplay acceptance remains
pending for trials and Blitzball; the user confirms Primer labels and Auto walk.
The canonical x86 build was staged with `--no-deploy`, then deployed
at the user's request. Deployment verified both compiled profiles, launcher
bootstrap and all 13 installed hashes: `install/review-20261007/deploy.log`.
No game or launcher was launched and no runtime probe was run.

Later player follow-ups are staged separately while the game runs. Pedestal
selection, final facing, path narration, dialogue/tutorial and swimming evidence
is in [player_followups_20261007.md](player_followups_20261007.md).

## Trial and catalog corrections

All six guides use the same authored worker labels as navigation. Guide paths
now call the shared native interaction-zone routing helper rather than routing
to a nearby sphere model or barrier. Moving pedestals use their live position.
Point-only floor controls use a tighter arrival distance in Auto walk mode.

- Kilika: entry sphere 28, entry door 29, separate storage slots 30/32, wall
  glyph 31, fire-chamber recess 33, Destruction Sphere 34, lower treasure recess
  35, final door 36, reset tile 41. The glyph activates at 31, not the original
  sphere source. The treasure goes in 35, not the final door.
- Djose: reset 62 and lift 63 are distinct. Charging, door and circuit sockets
  have distinct labels. Sphere transfers, upper pedestals and treasure actions
  are separate guide steps.
- Macalania: the movable pedestal is 1, treasure socket 40, Glyph reveal 41,
  revealed sphere 42 and bridge-support socket 43. Reset tiles are 46/47;
  worker 20 is Auron. The guide keeps pedestal loading separate from pillar
  loading and includes bridge reconstruction after the treasure.
- Bevelle: the Glyph barrier is 35. The Destruction Sphere destination is 34,
  the former Glyph Sphere source. Rail travel and confirm timing remain game
  actions; a disconnected rail platform is never described as a walking path.
- Zanarkand: source spheres 48/49 and pedestal sockets 51/52 are separate
  actions. The revisit Destruction Sphere returns to socket 49. Native floor
  pattern guidance remains intact.
- Besaid: rechecked the existing source/socket/glyph/pedestal sequence against
  the original script; retained its player-confirmed solver behavior.

The Kilika opt-in solver's old reveal continuation used final-door worker 36.
It now uses worker 35, PC 52179: the literal pushes for original REQ 11/7,
followed by REQEW 18/2. Preparation places sphere 6 in binding 010c61 and
clears carried/source sphere state. The original chest script still grants
the item and temple bit. This continuation requires gameplay confirmation.

The complete hashed catalog has 3,189 targets across 397 script variants:
29 Primer targets, 13 Tutorial terminal targets and 131 named conversation
targets. Counts include alternate map variants. Primer labels require the
book model plus its own collection call/text. Ordinary readable books remain
Book. Primer labels are gameplay-confirmed; ordinary books await the user's
separate check. Conversation names use reachable speaker headers, never names mentioned
in dialogue. Shared sphere animation workers are labelled Sphere rather than
recesses. Unknown identity labels remain conservative.

Thunder Plains Agency `kami0100`, worker 11, is the f016 terminal model.
Existing extracted image: `model_assets_20261006_154301/f016_model.png`.
Its own script opens the tutorial menu; battle launch is in a reached helper.
Readable books 9/10 remain Book; collectible worker 16 is Primer.

## Blitzball ownership and review

The user's native log shows slot 1 selection followed by a separate slot 2
name-only window. In hashed `bltz0200` (113266 bytes, FNV 58a5c0a5), slot 2
is used only by worker 1 strings 41/134 (player name/No Mark). Only that
producer is quieted. Existing project-specific dialogue guards remain intact.
No text-content deduplication was added.

The native text parser preserves line boundaries and native substitution and
choice-anchor behavior. Numeric macro boundaries get readable spacing without
adding spaces before punctuation. Each selector event clears prior-player
details before its corresponding native stats/description window arrives.

Worker 1's displayed arrays are read through the game's ATEL getter. Stat
identity is checked against worker 4's Team Data text variables and growth
tables: HP/SP 5/6, EN/AT 7/8, PA/BL 9/10, SH/CA 11/12. Descriptions and costs
are copied from displayed native pages. The buffer exposes available rendered
details; it does not invent effects for an undisplayed move.

Ctrl+Left: first line; Ctrl+Up/Down: previous/next; Ctrl+Right: whole buffer.
Assign Positions keeps native prompt speech; Backslash reads the first empty
native position slot (value 100). It checks that the assignment selector is
still open. Controller View then D-pad provides review; View then right trigger
provides the assignment announcement.

## Auto walk

Reference: the user's `Custom/FFXII/FFXII-Screen-Reader/src/navigation/auto_walk.cpp`.
The saved F8 setting defaults Off. With it On, selecting a destination and
pressing Backslash starts the same live collision route used by speech/audio.
The controller route command uses the same entry point.

Camera-relative octants, 32.5-degree/250-ms hysteresis, line rejoin, 250-ms
input expiry, 600-ms owner-event gap cancellation, bounded alternating side
steps/replanning and a 15-second no-progress cap follow the FFXII approach.
Input is added at the actual game keyboard read/controller publication;
physical movement wins in the same poll. Raw key edge tracking never sees
injected keys. FFX arrows and configured primary/alternate bindings use the
native C30E90 scan table, shared with spoken binding metadata.

Silent-scene evidence: Fahrenheit's Common table offset 850050 is an RVA,
not an absolute VA. Archived table at VA C50050 resolves Common 005D/005E to
856540/856650; their calls reach 871A50/871980. Those bodies set/clear controller
bit 2 and the controlled actor. The disable hook cancels immediately; input
also verifies native control ownership. Dialogue, menus, battle, focus loss,
generation changes, relocation and unreadable camera stop input. Walking never
confirms interaction and never resumes automatically after cancellation.

## Verification and next gameplay checks

Full native suite: 26 PASS checks, no warnings/errors;
`install/review-20261007/final-tests.log`.
F8 Auto walk On/Off persistence and SDL View/D-pad/trigger review pass.
Artifact metadata: `install/review-20261007/verification.json`.
Canonical staged build: `install/review-20261007/final-build.log`.
Focused labels: `src/navigation/tests/test_review_labels.py`.
All new production modules remain below 500 lines.

New fixtures cover player-name mirror ownership, native macro spacing/nested
selectors, Ctrl-arrow ownership, all eight labelled player stats, move lines,
all six positions, Auto walk manual priority/remaps/controller, silent scenes,
arrival, stale input, context changes and bounded stalled-route recovery.

For user gameplay testing:

1. F8: turn Auto walk On; choose a field destination; press Backslash. Check
   arrival, then start again and interrupt with movement or Escape.
2. Select Blitzball players. Check one name announcement, Ctrl+Left/Down/Right
   for stats and technique details, and Backslash during Assign Positions.
3. Check Kilika's separate glyph/fire/treasure sockets and reset tile. Test
   the remaining guides as saves become available; solver use stays optional.

Guide wording was checked primarily against the archived ATEL controls and
also corroborated with [Kilika](https://guidestrats.com/ffx-kilika-cloister-of-trials/),
[Djose](https://guidestrats.com/ffx-djose-cloister-of-trials/),
[Macalania](https://guidestrats.com/ffx-macalania-cloister-of-trials/),
[Bevelle](https://guidestrats.com/ffx-bevelle-cloister-of-trials/) and
[Zanarkand](https://guidestrats.com/ffx-zanarkand-cloister-of-trials/) walkthroughs.
