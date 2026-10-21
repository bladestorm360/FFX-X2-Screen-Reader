# Settings authority

Generated from `src/native/settings.cpp` by the documentation audit. Numeric
values are the persisted INI values; F8 presents their native mod value labels.
Boolean 0/1 means Off/On. Voiced dialogue 0/1/2 means Suppress/Braille only/
Speak anyway; Combat detail 0/1 means Normal/Detailed; Polish text 0/1 means
Auto/Polish. Backend 0 is Automatic; available backend choices are supplied
by PRISM. This is not a mod UI language catalog. See [ownership and extension](index.md#input-and-settings).

Setting descriptions below are the current shipped help strings. For battle
maximum MP, Shift+H/right stick down are the current all-panel controls; the
older setting string still says H. [Controls](../../controls.md) is authoritative.

| Setting / INI key | Default; range; step | Behavior |
|---|---|---|
| Speech / `speech` | 1; 0..1; 1 | Read game events with speech and braille. Menu controls remain available when disabled. |
| Combat detail / `combat_detail` | 0; 0..1; 1 | Normal reads actions, damage and status changes. Detailed adds command descriptions on selection. |
| Autodetail / `auto_detail` | 0; 0..1; 1 | Read native descriptions on highlight. F7 toggles this setting; I reads the current description manually. |
| Audio beacon / `beacon` | 0; 0..1; 1 | Follow the selected collision route. Stereo indicates left and right; lower pitch indicates behind; faster pings mean nearer. |
| Beacon volume / `beacon_volume` | 60; 20..100; 10 | Set beacon loudness from 20 to 100 percent. |
| Wall tones / `wall_tones` | 0; 0..1; 1 | Hear nearby collision boundaries relative to the camera. Left and right use stereo; ahead and behind use distinct pitches. |
| Wall volume / `wall_volume` | 40; 20..100; 10 | Set wall tone loudness from 20 to 100 percent. |
| Wall range / `wall_range` | 2; 1..5; 1 | Distance in walking steps at which collision boundaries begin sounding. |
| Arrival sound / `arrival` | 1; 0..1; 1 | Play a short sound when the final route destination is reached while the audio beacon is on. |
| Arrival volume / `arrival_volume` | 60; 20..100; 10 | Set arrival sound loudness from 20 to 100 percent. |
| Overdrive auto-success / `overdrive_assist` | 0; 0..1; 1 | Automatic success for Tidus, Auron and Lulu. Wakka's Attack Reels selects all double-hit attack icons; other reels pause for a matching result choice. |
| Voiced dialogue output / `voiced_dialogue` | 0; 0..2; 1 | Suppress, Braille only, or Speak anyway during native voice playback. Unvoiced, paused, muted or failed voice uses normal speech. Braille only stays silent if no braille display is available. T repeats the current dialogue line while dialogue is active. |
| Hide unreachable destinations / `hide_unreachable` | 0; 0..1; 1 | Check destinations in the background and omit those proved unreachable. Pending or unresolved collision data keeps destinations listed. U or field right stick click toggles this filter. |
| Race pickup cue / `race_pickup_cue` | 1; 0..1; 1 | Continuous stereo centers the next available Catcher Chocobo balloon. Equal left and right means centered; louder means nearer. F6 toggles this during the race. |
| Partial steps / `partial_steps` | 0; 0..1; 1 | Read path distances to one decimal place, such as 0.6 south. Off rounds distances to whole numbers, with a minimum of one. Route geometry and audio precision are unchanged. |
| Auto walk / `auto_walk` | 0; 0..1; 1 | Backslash walks the selected collision route. Your movement cancels immediately. Menus, dialogue, combat, map changes, lost focus and stalled routes stop walking. Interact manually on arrival. |
| Speak auto walk path / `auto_walk_path` | 0; 0..1; 1 | Read route directions before walking. Off announces only Auto walking to the selected destination. |
| Menu indexing / `menu_index` | 0; 0..1; 1 | Announce the selected entry number and total in mod menus and destination lists. |
| Lightning warning / `lightning_warning` | 1; 0..1; 1 | Play a short beep as the Thunder Plains warning flash begins, giving the full dodge window. Press dodge once when it sounds. Turn off Auto dodge to practise manually. |
| Lightning auto-dodge / `lightning_auto_dodge` | 0; 0..1; 1 | Automatically dodge ordinary Thunder Plains lightning. Disabled during dialogue, menus and battle. |
| Show maximum HP / `show_maximum_hp` | 0; 0..1; 1 | In battle and field targeting, read current HP followed by the maximum HP number. Off reads current HP and its percentage of maximum. |
| Butterfly cues / `butterfly_cues` | 1; 0..1; 1 | In the Makalania hunt, continuous stereo guides toward a blue butterfly. A low pulsing tone warns of a nearby red butterfly; steer away from that tone. Both grow louder as you approach. F6 toggles this during the hunt. |
| Overdrive cues / `overdrive_cues` | 1; 0..1; 1 | With auto-success off, speaks Auron's button sequence, beeps for Tidus's success zone and Wakka's symbols, and reads Lulu's cast count. |
| Speak maximum MP / `show_maximum_mp` | 0; 0..1; 1 | With H in battle, include maximum MP. Off reads current MP shown on the battle panel. |
| Polish text / `polish_text` | 0; 0..1; 1 | Automatically detect the installed Polish translation when its verified area names are ready. Polish forces Polish glyphs. |
| Speech backend / `speech_backend` | 0; 0..15; 1 | Select Automatic or an available Prism backend. |
| Enable hidden destinations menu for airship map / `hidden_airship_destinations` | 0; 0..1; 1 | Enable the optional six-destination search picker. Off keeps the menu and its shortcut disabled. |
| Jecht Shot auto-success / `jecht_assist` | 0; 0..1; 1 | Complete the original practice and timed memory prompts aboard the S.S. Winno. The game performs the shot and decides its result. Turn off to play manually. |
| Jecht Shot cues / `jecht_cues` | 1; 0..1; 1 | With Jecht Shot auto-success off, beep and speak the direction plus Confirm for each visible memory. Press the named controls together; center prompts require Confirm alone. |
| Gagazet puzzle auto-success / `gagazet_assist` | 0; 0..1; 1 | Complete Wakka's target throw and the three-swimmer puzzle through the game's original success paths. Turn off to solve manually. |
| Gagazet puzzle cues / `gagazet_cues` | 1; 0..1; 1 | Speak instructions, selected openings and throw timing for the Gagazet Submerged Passage puzzles. |
| Speak punctuation names / `speak_punctuation_names` | 0; 0..1; 1 | Speak names for highlighted punctuation during name or password entry. Off sends the original symbols to the screen reader. |
| Sin object collection / `sin_collection_assist` | 0; 0..1; 1 | Enable on any Sin map before entering The Nucleus. The game collects its normal objects and awards rewards. Stays enabled across battles; turn Off to collect manually. |
| Chocobo training auto-success / `chocobo_assist` | 0; 0..1; 1 | Enable on Calm Lands South before starting. Automatically finish the course you choose, including a perfect Catcher result. Confirm normal reward dialogue. |
| Remiem race auto-success / `remiem_assist` | 0; 0..1; 1 | Enable at Remiem before starting. Finish without poles: earn the first victory and each remaining chest reward in order, then use the normal five-chest repeat result. Confirm normal reward dialogue. |
| Butterfly hunt auto-success / `butterfly_assist` | 0; 0..1; 1 | Enable before starting. Collect seven butterflies in the current course and story phase. The normal timer and chest remain; open the chest and confirm its reward manually. |
| Blitzball auto-win / `blitz_assist` | 0; 0..1; 1 | Enable at team setup. Any confirmed live Action Commands option triggers a native win; setup, formation and forfeit choices do not. Native standings and prizes are preserved. |
