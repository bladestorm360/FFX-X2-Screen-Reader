# Makalania butterfly catching — 2026-10-08

## Integrated behavior and review

The native reader supports the Central and North courses using the exact
archived scripts and fresh game state. Objects includes `Butterfly hunt start`
outside a hunt and named blue/red targets during it. Ordinary approach starts
the hunt; no automatic catching, movement, timer or reward changes are added.
F1 reads the nearest blue bearing, F5/G reads count/time, and F6 toggles the
independently saved, default-On Butterfly cues under Navigation audio.
Existing controller navigation/route controls work; View then X reads status
and L3 toggles cues. Blue guidance and the nearby red pulse use the native
camera and copied SDL mixer state, and cancel on invalid context or geometry.

Backslash requests a route over the current native floor, excluding fresh red
body contact zones. Targets, layouts and hazard bounds are revalidated by the
existing movement/actor hooks. Unknown red bounds stop guidance. Butterfly
routes always speak and require manual steering, including hunt starters:
ordinary Auto walk's octant input and hysteresis cannot prove narrow detours
safe. Arrival uses actual physical contact bounds. Entity distances remain
native units throughout discovery/sorting; only speech converts to steps.

Sol (gpt-6.1-sol) investigated and implemented the hunt state, timer, renderer
roles, physical zones, event hooks and starters. Its first execution attempt
was blocked by the restarted desktop shell helper; the explicit authorized
PowerShell invocation recovered it. Primary reviewed the source and integrated
navigation, controller context, settings, audio, dependency coverage and tests.
Review fixed an unrelated script poisoning whole-generation identity recognition
and the native/step distance mismatch. Real archived-script fixtures cover both
courses, unavailable/decorative starters, generation/global-worker guards,
catch/exit distinction and changing actor state. Physical actor memory is mocked;
these tests do not run the game's VM.

Astra (gpt-6-astra) implemented and documented the floor/hazard route algorithm
and independent geometry regressions. Primary reviewed its conservative finite
segment/height checks, native reciprocal portals and blue arrival predicate.
Sol's separate integration review found two bugs: missing red bounds could be
dropped, and ordinary Auto walk could cut a narrow detour. Primary retained
unknown red hazards and made all butterfly movement manual, with regression
coverage. Astra's synthetic connected/disconnected, tangent, height, large
distance and seven-hazard cases pass. Actual course floor/body snapshots were
not available in the existing extraction, so real course reachability,
performance, audio and gameplay acceptance remain unconfirmed.

Sol (gpt-6.1-sol) also implemented out-of-battle spell/item target vitals and
post-customization equipment names. Primary integrated both production profiles,
verified the archived controller/row context and signature, and reviewed the
fixtures. [Field reader evidence](field_target_customization_20261008.md) and
[route algorithm/limits](butterfly_paths_20261008.md) retain the findings.

The full native suite reports 39 PASS summaries. After final starter integration,
the affected checks report 2 butterfly, 10 reader (both profiles) and 2 audio/
route summaries, all without compiler warnings/errors. Final build/deployment
evidence is retained in `install/field-butterfly-20261008/verification.json`.
The warning-free combined development build is deployed through the canonical
closed-process guard. Both compiled native profiles, launcher bootstrap and
all 13 installed payload hashes pass; the final offline DirectInput proxy fixture
also passes. A process-local Windows PowerShell module path was needed for the
guard's hash command after the desktop restart; no system environment or guards
were changed. The previous console/lightning/HP build remains included. Release
ZIPs were not refreshed. No game launch, runtime probe or Tester Logs access was
performed for this work.

## Offline evidence before implementation

The two Butterfly Hunt fields are mcfr0100 (Makalania Woods Central, area189)
and mcfr0200 (North, area190). Existing extraction archive files were decoded
with butterfly_inventory_20261008.py after matching their archived SHA256.
New output: Docs/Scripts/Offline/output/butterfly_20261008/*.json. No original
archive was overwritten; no game install, runtime probe or tester evidence used.

Exact bytecode identities: mcfr0100 length78486,55 workers,FNV1a908ca3c3,
SHA25663466cb214dfb0bc4bc44d719cd42b49a007f201078986ddbb72d5157cd12fa3;
mcfr0200 length14255,40 workers,FNV1ac248925f,
SHA25625109dd2e69cef050b389d1ff86afce7b7f282cf88f077a3c732825ca5166dd7.

Both scripts use event-shared active declaration0x0C000041 (south26/north20)
and collection count0x0C000042 (south27/north21). Timer belongs to southworker37
vars50,51,52 or northworker26 vars35,36,37, with exact local declarations
0x56000000/04/08 (signedint32). These are native tenths,seconds,minutes;
the original displayed time calls setStringVariable. Initial countdown may be
39 or29 seconds (the UI initially displays40/30). No synthetic timer is needed.

Collectible/hazard actor workers are south20..26 and north10..16. Same workers
change red/blue and positions between story stages, including retries/collected
flags. They use model0x5008. South18/19,north8/9 are decorative/start butterflies
and must be excluded from active catches/hazards; only Central19/North9 become
separate hunt-start targets. Southworker20 collisionentry4 atPC15807 disables collision,
fades away, incrementscountat15858, setscollectionflag28at15864, and copies
actual caughtcoordinates22..24; later alternate branch incrementsat16046 and
setsflag29. Thus one actor can represent different authored pickups over time.
Native field interaction disabling0/1 does not eliminate the separate touch
callbackentry4. Never treat fixed worker IDs as fixed colors.

Color comes from the game's actual renderer state. Archived FUN_0082B650 calls
FUN_0082BA10 at actor+330/+33C/+348; BA10 stores target/latch at+4/+8 and current
immediately when latch0. Script RGB setter0x5015 uses exactly blue(.1,.3,1) and
red(.9,.1,.1),latch0. FUN_0082B7A0 passes the currentfields to renderer. Native
actor debug prints shadeRGBA at330,33C,348,354 and bodyhit frominverse194bit8.
Fahrenheit Actor.cs/vec.cs independently names RGB/alpha LVec3f current/target/latch.
New preserved bodies: native_bodies_20261008_091101,091241,091502,091522.
The known navigation interactionEntries helper provides actual callback4
availability; actual body-collision disable flag and both current/targetalpha
provide independent collection/availability guards.

Implementation is event driven at existing verified native setter4713A0,
movement43D3B0, actor allocation425010/position42B500/enabled426770. These
addresses already belong to production minigames; no new native signatures are
introduced. ReadScript plus exact declaration/getter checks preserve profile
translation and reject unknown script revisions. Setter before/after uses
individual value transitions, not text-content deduplication.

## Implemented contract and validation

butterfly.hpp/.cpp own exact script identity/declaration checks, native getter
reads, actual renderer color/alpha and callback/body-state availability, native
collection events, count/time queries and generation/focus/menu/battle guards.
Each real native collectible-worker count increment is spoken, including repeated
identical text belonging to different events. Result-controller counter writes
are not called collection events. Ending state reads actual native seven/timeout
conditions; no result or reward is synthesized. Counters/timer come through
native46C500, preserving local ownership and exact32-bit declaration widths.
Before/after state uses Event slots24/25 to avoid sharing existing reader state.
Targets retain categoryObject, explicitbutterfly/hazardous metadata and global
worker identity proved by actual46A7C0 index lookup; no local/globalalias.
refreshButterflyTarget repulls current target role/actor/zone or marks it inactive.

The physical touch zone is a combined native body-radius cylinder in XZ, NOT
navigation's generic negative-height sphere. Shared actorContactRadius computes
live scales/bounds. FUN_0083D3B0 recomputes4E8, optionally tests vertical bounds
when both actor194 flags have40000000, checks inclusive radial contact, and
dispatches876420(actor,2). Callback2 resolves entry4 for these actor workers.
New archived body: native_bodies_20261008_092001/FUN_0083d3b0.txt. Targets require
actor+2 enabled, body collision194bit100 clear, and !(180!=0 &&181==0).
Zone range is combined radius, contactRadius0, height-1, centeractualactorposition,
contactMinY/MaxY exact optional vertical overlap. Butterfly-specific path code
must interpret this cylinder; generic inInteractionZone must not replace it.

Southworkers45..49 are enabled crossing edges, not additional red actors. They
request different actor20..26 entries that change colors/positions as the player
crosses trail boundaries. For example45 changes entry8/11 atPC77063/77153;49
changes entry9/10 at77873/77919. Thus route validity must compare fresh layout
at each movement and never treat a static current layout as all future states.
Read-only movement callbacks refresh actual colors/positions; no polling or
synthetic timer is added. Unknown/intermediate shades, hidden/collected fade
states, disabled body/callbacks and wrong script/declarations fail closed.

The primary owns install/reset/context/key/controller/build/deploy integration,
independent savedButterflyCues(defaultOn), blue/red audio and safe navigation.
Module exports freshblue/red targets, status/bearing queries, native touch-zone
helper and audio refresh. Hazard cue takes priority over the nearest blue when
near native contact range, and cancels the other cue channel. Teardown cancels
bothchannels. It does not change movement, catching, battles, timer or rewards.
Initial F1 module query is a bearing, not a safe route claim; verified safe-route
work belongs to butterfly_paths and primary integration.

Isolated x86/W4 compilation of butterfly_fixture.cpp +butterfly.cpp +actual
atel.cpp/atel_memory.cpp passed without warnings. Both actual archived fields
passed exact identity, allseven countincrements, count unchanged, actuallocal
29.9 timer, successfulend/expiry, rendererrolechanges, intermediatecolorrejection,
fade-target removal, collision/callbackdisablement, independentcueoff,
focus/menu/battle/generation/teardown and bytecode/declaration-rejection checks.
Fixture uses real archived bytecode/declarations; runtime processes and native
physicalactor bounds are mocked. It does not run the game's VM or prove live
story timing. Actual gameplay/controller/audio acceptance remains user-side.
REreference guide names were checked in sourceDocs,rootDocs andDevroot but were
absent; no Ghidra/Frida execution or newprobe authoring was performed.


Counter7 is also an exit-cancellation sentinel, not unconditional success.
Southexitworkers39/40 write7 atPC76525/76640; northexits27/28 do likewise.
Only a collection-counter transition from actual collectible workers proves
seven caught. Exit-sentinel writes cancel context/audio until resourcegeneration
changes and sayhunt ended; the fixture proves they cannot announce a win.
Ordinary native result/reward text retains its own speech producers.


Final fail-closed review: recognized active red actors with invalid bounds/position
are retained as hazardous targets withzone.valid=false; safe routes must reject
unknown geometry rather than silently dropping a red. Blue invalid zones are
omitted. Cue publication cancels if any recognized red zone is invalid. Each
actor/player bodyradius must be finite and positive independently; combinedsum
alone cannot validate a bad radius. Targetpositions use actual actorbodycoords,
so no extra geometry getter is required. Focused regression passes botharchives.


## Native hunt-start target

Central19 andNorth9 are actual model5008 rainbow hunt-start actors. Entries4/5
share the native start processor atPC14304/1993 and set actualactiveflag26/20
at14587/2276. Central18/North8 are decorative companions (touchentriesreturn;
bodycollisiondisabled); they are not hunt-start targets. Startpositions are
(-149.6115,-36.3700,-200.8887) and(-605.6915,15.7151,125.9332).
Existing hashed navigationcatalog has bothstarterrecords but genericObjectlabel:
navigation_targets_6.inc line27(Central19),line45(North9),entrymask30,identity1.
Generic label precedence discards identity1 labels for noncharacter models.
configureButterflyStarter supplies the verified role label directly in central
navigation discovery rather than a duplicate target producer or generator hack.

Central API configureButterflyStarter(Entity&,verifiedScript,local,worker)
requires exact hashedcourse identity and localheader binding; it verifies the
actual globalworker index, actorarray bounds/model/enabled/body flags, current
and targetalpha, nativecallback4/bodychannel2, exactinactivehuntflag and fresh
physicalbodyradius/verticalzone. No fixed storythreshold or colorpurity is used:
realresource/callback/actorstate is authoritative and rainbow tint cycles.
It tags butterfly/butterflyStart,hazardousfalse,categoryObject,labelButterflyhuntstart
and usefulEntries30. refreshButterflyStarter validates storedresourcegeneration,
current globalworker/currentactor identity, freshnativeentry/bodyzone and preserves
originalmodel or syntheticstableID. Huntactivation/hidden/disabled/fading/dead
callback/resourcechanges make the starterinactive. No gamecall tostart/catch is
added; ordinary manual approach/touch retains native start behavior. Existing
walktowardstarter remains manual, per primary minigame route policy.
