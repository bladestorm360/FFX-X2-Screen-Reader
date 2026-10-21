# Battle controller meters and visible HP/MP panels — October16 follow-up

Implemented, offline verified and guarded-deployed. Canonical development native/
original-launcher build: zero warnings/errors,569 unchanged source hashes. Both
166-hook reader profiles, controller/SDL owner and profile checks pass. Actual
compiled Preflight/Installed checks pass without running game code. Seven installed
artifact hashes,15 locked PRISM files/receipt, unchanged user INI and held release
package hashes are verified. New gameplay confirmation remains user-side.

Installed proxy SHA256: `c0c0bcaa61ec22cb945069ae296dcf4fe15e53b3fa86fe3f0d124fbb39831101`.
[Deployment receipt](../../Analysis/FFX/RepositoryEvidence/historical-install/battle-panels-controls-deployment-verification-20261016.json),
[source snapshot](../../Analysis/FFX/RepositoryEvidence/historical-install/battle-panels-controls-source-20261016.json).

- Right stick left dispatches Shift+O: all actually displayed Overdrive meters.
- View/Select then Y/Triangle dispatches O: active character/aeon Overdrive.
  Saved battle face bindings take priority; the first free face in Y,A,B,X order
  is used. Existing default X HP/MP/status and field/Grid/menu contexts remain.
- Shift+H/right stick down reads visible HP/MP panels; plain H remains active-turn
  HP/MP/status. Right stick right/Shift+T turn order is retained.

## Native producer and visible-information proof

Source/ABI/call provenance is retained in
Scripts/Offline/output/battle_vitals_20261016/proof.json and its full disassemblies.
Read-only exports: install/battle-visible-vitals-{original,draw,glyph}-20261016.txt.
No installed source/asset or Tester Logs access was needed.

The existing4963C0 status renderer iterates ONLY its three native table slots at
F3F76C, skipping255. Each visible row asks39AF20 (original21-byte getter) for its
character's name, then505890 (original148-byte INTEGER digit renderer) receives
its displayed, potentially interpolated, numeric operands. The first operand
is MOV EDI,[EBP+8], not the decompiler's misleading float type. Both functions
are now signature-qualified native hooks. The name getter ties numbers to the
same native table row; no roster/actor scan supplies missing values.

Original mode byte F3D742:0 draws current/maximum HP;1 current/maximum MP;
2 status symbols only;3 current HP/current MP (and the separate Overdrive bar).
The reader keeps exactly those roles. Maxima already rendered remain numeric;
undisplayed maxima, statuses and reserve members are not appended.

The original foreground-glyph function4FB940 is405 bytes. It calls4E5A20 at
+14F and returns at+154. A value is accepted only while its qualified505890
call is live and this exact foreground caller survives original clipping with
positive area. Shadow/preparation calls cannot qualify it. The foreground
function immediately submits that accepted glyph through640810. Its full body,
call offset and SHA are recorded, not guessed from SDK function names.

Native owner state is bounded to3 panels,2 numeric draws per row and4 scalar
roles. The existing HUD frame clears/publishes copied POD; hidden frames,
reset and generation retire previous panels. Worker text plans format copied
names/integers; no pointers, text formatting, blocking operations or I/O cross
the native producer boundary. Controller free-face lookup reads only its fixed
21-command mod-owned binding table on a face edge; no SDL work or game UI walk.

## Offline verification and remaining integration limits

Both166-hook reader profiles pass original x86 clip replay: two visible subjects
including an aeon; animated values differing from actor memory; zero HP and
numeric maxima; HP-only/MP-only/status-only modes; missing current/MP digits;
foreground caller and stale-frame exclusion. Existing active H/O checks pass.
New virtual SDL controller checks pass Shift+O, O on the free mod face, saved
battle-binding priority/fallback, Shift+H, held input, native decoder consumption,
unchanged keyboard parent and all prior Grid controls.

Logs: install/battle-panels-{controller,readers-first,profile,owner}-20261016.log.
Original digit/GPU call context remains controlled in the fixture; native clip
arithmetic executes unchanged. No game launch/control or runtime capture.
Actual gameplay/audible timing remains user-side. Earlier exact controller
source/queue/sink timing is a component result, not a hardware guarantee.

The initial controller fixture compile exposed a missing internal function
forward declaration in its new custom-binding test; that declaration was fixed.
A direct invocation at that point was an OLD executable and is not acceptance
evidence. Fresh battle-panels-controller and both-reader runs are the acceptance
checks. The earlier build's source guard correctly rejected the intervening
fixture edit; it was never deployed. Final build uses a fresh unchanged snapshot.
No delegation was used for this narrow follow-up; Sol owns the implementation,
discovery, review and validation. Installer/ZIP remain held; monitor PAUSED.

## Short user check after deployment

During a character turn: right stick left should read shown Overdrive meters;
View then Y/Triangle should read only the active meter. Right stick down or
Shift+H should read only visible HP/MP rows in order; plain H keeps active stats.
Try each stick gesture twice with a neutral release: each should read once and
should not cause a game action. Saved custom face assignment priority is also
covered offline. No fresh gameplay confirmation is claimed.

Final controlled96 sample source-to-native-read result: p50=2.468ms,p95=2.582ms,
max=2.842ms; Native pump/read max48.4us versus direct fake-native0.6us. This ran
under concurrent canonical compilation with synthetic SDK source/focus/backend;
no hardware/GPU/full aggregate callback or audible response guarantee is inferred.
No new UI walks, blocking work or thread ownership changes were introduced.
