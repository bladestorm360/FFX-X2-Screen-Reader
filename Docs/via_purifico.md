# Via Purifico accessibility guide

Implemented in the October12 maintenance build. Offline script/state/menu tests pass; gameplay confirmation is still required.

Press F8, open Miscellaneous, then Via Purifico while in the **land** dungeon. Controller settings use View/Select then Menu/Start; D-pad selects, South face confirms and East face returns. The dungeon submenu offers **Collect all treasures automatically**, manual step/path review, previous/next step and status. Close settings after enabling collection. Confirm each native treasure dialogue; the automatic action opens every remaining chest through its original game script and leaves you in the dungeon. Turning it off lets a current chest finish naturally. It does not reveal item names early or activate the exit.

For manual play, Read current puzzle step starts from verified progress. F9/F10 review previous/next; Shift+F10 repeats; F11 reads the selected step and Shift+F11 finds the expected step. Read path to current step uses the current collision map and navigation beacon; Auto walk, when enabled, follows that route. For panels, the walking route stops beside the circle. After arrival, release movement and listen for the desired direction cue before stepping onto it. Read the step again after each warp; the next panel depends on the native current location. If you take another direction, the guide replans from the resulting panel.

The eleven guide stages are:

1. Open chest1 in the southwest branch.
2. Open chest2 in the western corridor.
3. Open chest6 near the eastern glyph.
4. Open chest7 in the eastern corridor.
5. Touch the glyph near chest1.
6. Step on the eastern relocation glyph.
7. Follow the active warp panels into the sealed western chamber, waiting for each requested direction cue.
8. Open chest3.
9. Open chest4.
10. Open chest5.
11. Take the east arrow back out, then use Exit1 near the northern Save Sphere.

Treasure numbering is stable and uses actual chest coordinates; contents remain in the game's native treasure dialogue. Before relocation, the central panel connects the four corridors. After relocation, the active panel follows these connections toward the sealed chamber:

| Current panel | Wait for | Destination |
|---|---|---|
| Northwest corridor | West | Sealed western chamber |
| North corridor | West | Northwest corridor |
| West corridor | North | Northwest corridor |
| South corridor | North | North corridor |
| East corridor | West | West corridor |
| Sealed western chamber | East | Northwest corridor |

Both land and water progression exits now appear in the pathfinder while their original callbacks are enabled. The game owns every story scene and map transition. No agent game launch or gameplay test was performed. Architecture, original script identities and native collection evidence are in [the repair record](open_issue_repairs_20261012.md).
