# Launcher language and settings-return accessibility — October23

Implementation is **complete**; offline verification passes, actual launcher
gameplay confirmation is pending. The user reports Japanese-default
launchers have no arrow navigation or spoken choices, and asks for investigation
while gameplay testing continues. No running game/launcher modules will be
replaced. No game launch/control/probe or Tester Logs is authorized.

The later1.0.1 release request supersedes the initial deployment hold below.
The warning-free release payload is now guarded-deployed and included in the
verified1.0.1 installer/ZIP. Fresh final-adapter checks again pass all eight
startup languages and64 settings transitions. Release excludes the development
console; current [delivery evidence](../../release.md) is separate from the
initial staged-repair receipt and its preserved-console snapshot.

## Confirmed cause and design

`src/launcher/launcher_reader.cpp::UiReader::Attach` first adds the original
`GameLauncher.MainForm` to `attached`, then installs selector state/events only
for LanguagePathus/en. Japanese therefore receives neither `mainSelections`
nor activation/hover handlers. `PreFilterMessage` refuses its navigation keys;
the already-attached set also prevents a later retry. This is a direct source
cause matching the report, not a PRISM backend/language-voice failure.

Retained original `MainForm.OnConfigClicked` opens the native settings dialog,
and replaces `mActiveRects` through `InitializeActiveRect` when language changes.
`ConfigForm.SaveConfiguration` persists the language; `Program.SaveIniFile`
updates `mLanguage` immediately. The same main form survives. Labels must use
the current native LanguagePath, and all hover/clicks must resolve current
rectangles rather than retaining an obsolete array. Native settings controls
continue to own their input, accessible names and screen-reader speech.

The repair removes language as an attachment/input gate, keeping the original form-type,
enabled/modal, event lifetime and native action checks. A central localized
resolver in `src/launcher/launcher_labels.hpp` uses current LanguagePath for
each presentation of the eight actual rendered launcher variants. Unknown
language metadata does not disable navigation. Native hover/select sounds,
the repeated-key activation guard, LAA gate and original game/credits/settings
actions are preserved.

## Asset permission and provenance

The user explicitly approved read-only inspection of installed
`data/MetaMenu.vbf` for this issue. SHA256:
`6cce27133c05a1d20cc423a23411be8bd9b1ddb2d9b23f66d30d46430cde042c`.
Only130 referenced menu PNG/XML assets were extracted, outside Git, to
`Analysis/FFX/LauncherLanguages_20261023/assets_20261023_145140` with per-entry
hashes in `manifest.json`. The archive was not changed. The existing read-only
VBF decoder verifies header/filename hashes and decompressed lengths.

The renderer uses `MetaMenu/PS3Data/MenuMetaMenu/{LanguagePath}`. The actual
paths are us, jp, ch, kr, de, fr, it and sp; `Program.LanguagePath` maps en→us and
es→sp. All sixteen full-size/360p title images were visually inspected. The Japanese
Eternal Calm subtitle is 永遠のナギ節; Chinese and the other variants show
Eternal Calm. Japanese Settings is rendered in English as SETTING. Other
localized settings/credits labels follow their actual images. Existing game
title logos retain their displayed English spelling.

| Native path | Credits / Additional credits | Settings |
| --- | --- | --- |
| us | Credits & Bonus Audio / Additional Credits | Setting |
| jp | Credits & Bonus Audio / Additional Credits | Setting |
| ch | Credits & Bonus Audio / Additional Credits | 設置 |
| kr | Credits & Bonus Audio / Additional Credits | 설정 |
| de | Abspann & Extras / Weitere Mitwirkende | Einstellung |
| fr | Crédits & contenu bonus / Crédits supplémentaires | Paramètres |
| it | Riconoscimenti & Extra / Riconoscimenti aggiuntivi | Impostazioni |
| sp | Créditos y contenido extra / Créditos adicionales | Configuración |

Original launcher decompilation remains outside Git under
`Analysis/FFX/RepositoryEvidence/offline-output/FFX_Launcher/20261003_audit`.
Relevant functions are MainForm.InitializeActiveRect/OnMouseMove/OnConfigClicked,
Program.LanguagePath/SaveIniFile and ConfigForm.SaveConfiguration. No game source,
decompiled body or asset enters the source repository.

## Verification and remaining confirmation

The unchanged adapter failed the initial Japanese regression in the actual
production-adapter fixture: no arrow navigation after its first queued message.
The exact failure is retained in
`install/launcher-languages-20261023/before.log`. This confirms the input gate
independently of the source reading.

The expanded actual-adapter/PRISM WinForms fixture passes Japanese startup,
all eight starting locales/all seven actions, all64 language transitions on
the same form, rebuilt hotspot arrays with different bounds and both settings
reactivation orderings. It verifies native keyboard/mouse action order/sounds,
current labels, input/speech restoration, unchanged native-dialog accessibility,
repeated-key activation protection, repeated valid speech, alias/null/unknown/
throwing-language recovery and disposal of both form-state collections.
The installer checks pass preservation, idempotence, original backup, restore
and conflict refusal. Final log: `install/launcher-languages-20261023/launcher-tests.log`.

Canonical default development staging completed without warnings.419 of420
previously verified build inputs are unchanged: only launcher_reader.cpp differs,
with the new owned label header added. Native game/PRISM logic remains unchanged;
prior game-reader/trial verification is reused at those exact source hashes.
The development console remains compiled in. All seven installed payloads match
the previous console delivery; launcher config, settings, approved menu archive
and published1.0 ZIP are unchanged. Exact hashes and logs are indexed by
`install/launcher-languages-20261023/verification.json`. No installation was
written and no installer/package/publication step ran.

The fixture uses the production C++/CLI adapter and native PRISM worker with a
controlled original-launcher API and fake speech backend. Native game launch,
FMOD, actual launcher rendering/focus and audible screen-reader delivery remain
distinct gameplay checks. Initial staging held deployment while the user tested;
the later1.0.1 request authorizes delivery as recorded above. At the next launcher test
convenient: start with Japanese selected, press Down/Up and expect each menu
choice spoken; open Setting, change language, save/close, then use arrows and
Enter. Expect the current-language labels and correct native action. Silent or
unresponsive rows, a wrong action, or addon speech over native settings controls
are failure symptoms. No story progress or trial save is needed.

### Controlled response evidence

Both comparison runs use the same final fixture/native speech DLL/fake PRISM,
replacing only the managed adapter. Each warms with one cold Home presentation,
then records128 serial English Down presentations. Logs are `english-before.log`
and `english-after.log` in the receipt folder.

| Controlled measurement, milliseconds | Old adapter | Repaired adapter |
| --- | ---: | ---: |
| Speech ready after requested startup | 14.812 | 9.554 |
| Cold key dispatch / sampled fake-backend receipt | 15.572 / 16.396 | 15.978 / 16.863 |
| Warm dispatch mean / maximum | 0.168 / 0.670 | 0.190 / 0.461 |
| Warm sampled fake-backend receipt mean / maximum | 15.472 / 16.536 | 15.492 / 16.462 |

The full353-key matrix includes cold forms, language-property exceptions and
settings rebuilds: dispatch mean/max1.341/17.391ms and sampled fake-backend receipt
15.258/31.904ms; cold speech readiness91.566ms. Receipt includes collection,
managed/native publication, queue/preparation/backend and the fixture's polling
delay, whose Thread.Sleep sampling is scheduler dependent. It is an observed
upper bound at the fake backend, not an audible timing measurement or universal
target. Serial cases wait for each output and assert exact text/count, so no
stale row accumulates there. Modal controls produce no addon output. Rapid key
bursts, a real launcher render/message pump, background CPU contention, FMOD,
actual screen-reader latency and aggregate host impact remain unmeasured. This
repair adds only current ready metadata lookup and short artwork label selection;
it does not claim responsiveness merely from worker placement.

## Data flow, ownership and controls

`ReaderDomainManager.InitializeNewDomain` installs one `UiReader` message filter.
Only an exact original MainForm receives selector state/events. The UI thread
owns Form, MethodInfo and ActiveRect objects, the attached set and selected index;
none enters a worker. `Attach` records the selected row independently of language.
`Select` and `MouseMoved` resolve the current array, then `LanguagePath` reads the
current original static property on that same UI thread. `MenuLabels.Name` maps
the native action identity to ready localized artwork text. Labels also update
the main form's AccessibleName. Missing/throwing language metadata uses common
English artwork labels and remains navigable; later valid metadata is read on
the next presentation. No permanent startup fallback or text deduplication exists.

The original seven actions determine ordering and activation. Arrows/Tab cycle,
Home/End select endpoints, Enter/Space raise the existing native click at the
current rectangle center, and activation autorepeat remains ignored. Original
hover/click handlers retain FMOD sounds and saved mute/volume behavior. FFX keeps
the existing LAA launch guard. Settings/credits dialogs own their native keys,
accessible controls and speech; disabled main forms hand input back. Deactivation
pauses/clears addon speech and reactivation resumes the retained row. Disposal
removes the form from both collections. The original modal continuation may
rebuild rectangles after activation: the next input/hover uses the new array and
language rather than cached objects.

`Speech.Say` passes a ready UTF16 label through the existing native
`FfxNative_Output` bounded capture; PRISM startup, formatting and backend calls
remain on the established speech workers. Capture owns the copied text, so the
managed pin/temporary and UI objects need not outlive that call. Current labels
are at most40 UTF16 units and the original selector has seven rows. This change
adds no timer, polling, I/O, worker wait or new queue. Existing speech cancellation
and bounded-capacity failure handling remain in `speech_queue.hpp`,
`speech_capture.hpp`, `speech_capture.cpp` and `speech_queue.cpp`; this task does
not change their budgets or backend ownership.
The shared capture uses2048 records and4096 blocks of512 UTF16 units; admission
budgets are1024 events and512Ki UTF16 units. Exhaustion refuses publication
without making the UI owner wait. Modal epochs retire pending addon speech.

There is no new mod setting or persistence migration. Language follows the
launcher's saved text-language choice, separate from in-game mod UI language.
These are rendered native menu labels, so the resolver preserves the displayed
wording even when a language's artwork deliberately uses English. Normal native
settings still save through Program.SaveIniFile. The read-only investigation and
offline fixture do not change the user's configuration.

## Extension and integration

Add a verified native image-action identity in `MenuLabels.Name`, with actual
artwork wording for every supported language and independent fixture expectations.
Do not make attachment depend on catalog detection. Continue reading current
rectangles at use time and keep native input/modal/LAA action ownership. Unknown
actions return empty text while retaining native behavior; a new screen needs
its own verified producer rather than extending a type-name gate speculatively.
The header is included by the existing C++/CLI translation unit and needs no new
payload or package resource. Canonical production staging remains
`src/proxy/build_dinput8.bat --no-deploy`, retaining the user's development console.
`src/native/tests/launcher_installer.ps1` compiles/runs the controlled surrogate,
loads both actual launcher DLLs and verifies installer merge/restore/conflict rules.
Original evidence, assets and source-bearing fixtures remain outside Git. All
work for this repair is retained by Sol; no delegation or Astra escalation.
