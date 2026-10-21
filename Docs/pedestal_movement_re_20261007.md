# Pedestal movement contracts, offline review, 2026-10-07

Astra (`gpt-6-astra`) read the existing hashed ATEL flows and native/community
archives for the primary's held-Confirm accessibility integration. No production
source edits, game access, launch, live probe, or native flag/script mutation.
Primary owns integration and verification. These are offline source facts;
gameplay acceptance remains pending.

Evidence root: `Docs/Scripts/Offline/output/FFX/20261003_audit/cloister_designs_20261004_131921_356300/`.
Each trial's `_flow.json` supplies original bindings, blocks and branch edges;
`_flow.txt` supplies readable original PCs. PCs below are hexadecimal ATEL code
positions, not executable RVAs. Coordinates are original X,Y,Z. Input codes below
are raw directional bitfields: the script divides `(0044() & 0xF000)` by 4096.
They are not universally world-oriented because the trials have different cameras.

## Critical architecture correction

Djose and Macalania do not use a mysterious actor collision callback to start
pushing. Their ordinary worker1 entry2 operates the sphere socket. Separate type4
workers (Djose42 entry1, Macalania29 entry1) execute native script loops, read live
player/pedestal position plus directional buttons/stick, and request movement after
15 matching frames. They call 019F(1) as an eligibility gate. Keep that original gate
and all original script behavior; input-only assistance need not reimplement it.

Do not require actor contact channel2 to be enabled for these type4 producers.
Both producers use their own strict rectangular approach zones. For assistance,
choose an interior point of the relevant verified rectangle, using current actor
position. Do not transplant the Besaid/Kilika circular collision contract here.
Suppress Confirm reaching sphere interaction during held-Confirm assisted pushing.

019F remains unnamed in both community metadata sources. Fahrenheit
`core/fh/Atel/call.cs` gives Common table RVA850050 and 16-byte rows
(init, exec, float-return, int-return). Thus row019F is RVA851A40, integer-return
pointer at851A4C. Native archive registration confirms VA877800 uses C50050.
No authorized offline table byte snapshot was available in this investigation;
no game executable was opened. Do not describe 019F as a collision getter.

## Djose lower pedestal

Worker42 entry1 CE95 reads positions (CEA1/CEB0) and input (CEB3/CEC7/CECD).
Worker1 starts at (0,-18,24), original reset entry8 returns it there.
Direction-side binding181226 has these values and strict player relative bounds:

| Raw input | World movement | Player approach relative to pedestal |
| --- | --- | --- |
| 2000 | +X | -9.6 < dx < -2; -2.4 < dz < 2.4 |
| 8000 | -X | 2 < dx < 9.6; -2.4 < dz < 2.4 |
| 1000 | +Z | -9.6 < dz < -2; -1.6 < dx < 1.6 |
| 4000 | -Z | 2 < dz < 9.6; -1.6 < dx < 1.6 |

CF80..D02E derives side. D03B..D086 converts direction to world destination
1000 units ahead. This far point is a motion ray, NOT the puzzle destination.
D111..D179 requires nonzero matching direction and side, 15-frame counter181228,
019F(1), no18122B, and no blocked-direction flag181227.
Blocked: -X when pedestalX<-30; -Z when Z<2; +Z when Z>48 unless0181EB==3.

At D17C player control is disabled. D188 waits for player0 entry18; D199 starts
pedestal1 entry9, D1A4 starts player0 entry19. Original pedestal movement speed8
and destination current position plus direction1000 run until181225 changes.
The native loop D1B3..D340 continues reading input during disabled control.
It stops if input direction changes, sampled pedestal movement<.39, or the above
limits apply. Assistance must retain only its owned direction through this span.
At D346 stop SFX, D34C/D352 stop pedestal/player, and D35E/D369 start ending
animations. Zero injected direction after this point; normal control returns D4DD.
A narrow control exception should be bound to the armed trial/action and this
specific producer, never any arbitrary disabled-player scene.

### Charger

From reset position, push **+X / raw2000**, one continuous segment towards
X70,Z24. Exact charging volume at D31F is **69<X<71 and22<Z<26**. It also
requires0181F1==1 and at least one pedestal sphere181ED/181EE==1.
For an EMPTY pedestal (guide step9), this automatic stop/charge does not fire;
assistance should stop ordinary input inside the same verified position volume.
The point70,24 is the center of an actual original objective region, not a guess.

Movement result181225 becomes2 at D325. After animation, D3A5 additionally checks
both charging-wall spheres181E9/181EA nonzero and not10; D407/D428 convert every
nonempty pedestal socket to3, and181F1 becomes2. Thus approaching the charger and
charging a sphere are distinct actions; empty-pedestal arrival is not charge success.

### Electrical gap

From native reset position, push **+Z / raw1000**. Door condition181EB==3 is
required to pass Z48. At D337 pedestalZ>85 sets181225=3; native worker1 entry10
then moves to Z92 and runs the original fall/bounce animation. Both pedestal
spheres181ED/181EE must be nonzero and not10 or it falls unsuccessfully.
Successful bounce sets **181229=1 at20C5**. There is no legitimate exact final
floating Z literal: original integration oscillates the position. Use the native
completion flag and one push, rather than an invented fixed landing distance.

Relevant T0 raw bindings (high1, element0):181225=0C0000D5,
181226=0C0000D6,181229=0C0000D9,18122B=0C0000DB.

## Djose five upper wall controls

Workers56..60 are type3 **ordinary Confirm entry2**, one scripted push each.
There is no sustained-direction input producer. Entry starts/flag writes:

| Worker | Entry2 | Done flag | Write PC | Original zone center |
| --- | --- | --- | --- | --- |
|56|14530|01822E=1|1453D|(30,-67,117)|
|57|146F0|01822F=1|146FD|(27,-67,151)|
|58|148B0|018230=1|148BD|(0,-67,162)|
|59|14A70|018231=1|14A7D|(-27,-67,151)|
|60|14C30|018232=1|14C3D|(-30,-67,117)|

Each checks its own flag0, sets1, disables control, sets18122D to its switch
number, and REQSW player0 entry31. All five flags set enables181233 and reveals
worker6. Use ordinary native Confirm once and let the full animation finish;
flags are set before animation, so do not start another action immediately on write.
Do not apply the lower pedestal's directional gating or suppress this Confirm.

## Macalania

Worker29 entry1 DF7B reads positions DF87/DF96 and directional state DF99 onward.
State=015A9B; side=015ABD; held counter=015A9A. Unlike Djose, enums map:

| Raw | Movement, states other than5 | Player relative approach |
| --- | --- | --- |
|2000|+Z|-9.6<dz<-2; -1.6<dx<2.4|
|1000|-X|2<dx<9.6; -2.4<dz<1.6|
|4000|+X|-9.6<dx<-2; -2.4<dz<1.6|
|8000|-Z|2<dz<9.6; -1.6<dx<2.4|

At state5, positive-X approach (normally side1) is changed to side8, and input8
moves **-X**, following the fixed walkway path. This is an explicit original
exception E120/E126/E1BB, not a camera-agnostic world-to-dpad assumption.
The producer requires abs(playerY-pedestalY)<=10 (E27A),019F(1),matching side and
15 frames. State2 blocks raw1000; state3 allows only raw2000; state4 blocks all.
State99 is hidden failed/fallen pedestal; do not arm assistance there.

Native entry2 sphere operation itself checks side2 and states other than4/6/99,
or side8 with state5. Holding Confirm unfiltered would operate/remove its sphere.

| Guide movement | Native start guard | Inject until launch | Original result |
| --- | --- | --- | --- |
|First ice mound, including repeated restored-mound step|state0 and015ABE!=0|raw2000 (+Z)|worker1 state1 at20F0 after Z>=52|
|Down ramp, including repeats|state1 and015A8E==4|raw1000 (-X)|worker1 state3 at2780, destination(.5,68,12.2)|
|Lower support, including restoration|state3; loaded sphere015A8F==4 for useful bridge|raw2000 (+Z)|worker1 state4 at242E after Z>=66|
|Walkway sphere conversion, including conversion back|state5|raw8000 (-X), positive-X approach|worker1 entry12, state6 at2D88, native sphere transfer/conversion follows|

Initial pedestal is(34,39,12), state0. First desired push keepsX34 and reaches
Z~52 (float loop threshold, not exact destination). Choosing -X at state0 instead
moves only toX~5 and state2, the wrong guide branch. Do not auto-treat state2 as
completed first mound; original reset or supported alternate plan is needed.

Ramp original motion first passes X<-82 then literal intermediate destinations
(-87.5245,42.1867,48.2578),(-75.2058,60.9334,20.3596),
(-48.4012,63.8026,14.1140),(-38.0181,66.6565,11.1250),(.5,68,12.2).
No lateral reposition of the pedestal is required between the intended branches;
reposition the PLAYER to the correct approach face using ordinary navigation.
Missing ramp sphere causes the original failure/fall branch; check before arming.

Normal launch: worker29 disables control E2AD; waits player0e10 E2B9;
REQ pedestal1e10 atE2D5 and player0e11 E2E0. Pedestal movement thereafter is fully
scripted and does not need held direction. End sequence E2F2..E41A (enable control).
State5 disables E44A, REQEW pedestal1e12 E456, re-enables E458.
All owned disabled contexts should release injected direction once native movement
has launched. No need to steer through its complex sliding/jumping path.

State5 reset position(-18,-1,-116); conversion path reaches
(-26.2789,4.25968,-114.0476),(-39.6550,8.58268,-106.3503),
(-55.4186,20.2171,-81.2279),(-57.673,24,-57), then native positive-X launch.
It stops atX24, state6, with sphere transfer logic after2D88. Complete only when
original worker1 entry12 returns (2FBA), since state6 is written before conversion.

Raw T0 bindings, high1:015A9B=0C0000A3;015A9A=0C0000A2;
015ABE=0C0000C6;015A8E=0C000096;015A8F=0C000097.

## Bevelle rail pedestal

Worker13 is the visual/motion actor. Its entries2..5 are immediate returns;
do not require actor13 ordinary contact interaction and do not guess free pushes.

Entrance: original type2 edge worker39 entry6 **1C69D..1C701** starts the push
and initial ride. Edge endpoints(-30.25306,-459.99835,-44.29230) and
(-9.74233,-459.99835,-44.31187), authored edge lengths10/10, height2.
Player0e7 aligns to(-20.26033,-459.99881,-43.45033), then original push animation.
Pedestal starts(-20.26033,-460.01617,-36.25044); its first original endpoint is
(-20.21984,-460.02228,-13.32647) at4AC1/4AEE, then native rail boarding.
Navigate/cross the actual entrance trigger toward+Z; zero assisted direction when
worker39 disables control1C69D. One native push action; no repeated manual counts.

The final optional-platform action uses worker38 **entry15,1C35C..1C46A**.
Actual starting pedestal/zone(-18.7,-453.2,263.1); approach its +Z side, playerZ>=267.
Native condition1C3D0 additionally requires (1F49F==1 OR1F4A0==1),1F4A4==0,
1F4A9==1 and held controllerButtonPressed1(15), or stick angle>=2.3/<=-2.3.
It literally tests playerX<271 alongside playerZ>=267; do not silently rewrite
that source condition as a Z upper bound. User-facing motion is one **-Z** push;
raw native directional button15 corresponds left/8000 in standard bit ordering.

At1C3D3 disable,1C3D9 sets1F4B0=8;1C3E5 REQEW player0e8, whose3D77..3DB7
continues checking same direction/flags and increments1F4A1 until>20.
Retain owned direction through this narrow disabled-control period. At1C407 the
completed hold disables again;1C40D/1C413 set1F486=83,1F487=87, and1C42A invokes
player0e6. Release injected movement; remaining push/rail motion is scripted.
Player0 PC2BE0 sets1F489=16 and2BEC REQs pedestal13e6. Actor's54AF branch drives
(-18.7,-453.56369,203.59248), rises toY-441.56369 and sets1F488=65 at557B.
Player ends ground point(-18.7,-453.56369,203.59248),1F4AC resets0 at2CBB.

Rail-turn Confirm prompts and socket transfers are separate ordinary actions.
Do not automate all rail choices by claiming this pedestal contract covers them.
The entrance edge's exact native dispatch to entry6 was not independently traced
in this subtask; script and authored edge action establish the target, and the
shared navigation trigger resolver must retain its existing verified semantics.

## Zanarkand

Workers44..47 are ordinary type3 entry2 actions. One Confirm activates each
pedestal's display; the actual puzzle completion still requires matching floor
tiles. Entries:747E,75F6,776E,78E6. They disable control, perform native animation,
and finally set00F6A8(T5) and00F6C5(T0) to1,2,3,4 respectively. No sustained
pedestal-direction producer exists in these four callbacks. Those flags mean
selected pattern, not the associated tile puzzle solved. Keep original tile rules.

## Integration and validation requirements

- Generate bindings/PC contracts from hashed original flows, with explicit guards
  for each guide phase. Distinguish current guide action from merely selected actor.
- Use a single input coordinator, original native direction bits, physical held
  Confirm as consent, immediate release/manual/focus/map cancellation, no solver
  shortcuts, teleports, or writes to puzzle flags. Preserve original019F gate.
- Emit one-push instructions for autonomous Macalania/Bevelle/Djose-upper actions.
  For continuous lower Djose, use remaining distance from live actor position to
  actual verified objective region, and stop when inside it. No arbitrary meters
  or guessed push-count divisor.
- Test input cancellation and owned disabled-control spans using original source
  guards. Never make a universal exception for disabled-player context.
- Newly found contracts remain gameplay-unconfirmed. 019F exact native body is
  unresolved but preserved by leaving the original producer in control.

## Root-worker binding table for implementation

All high words are1, element0. Prefix0C000000 plus the hex suffix below denotes
T0 binding low; Zanarkand pattern is separately T5 low5C000004.

| Trial | State/role | Original map field | Low suffix |
| --- | --- | --- | --- |
|Djose|Charging-wall sockets|0181E9,0181EA|99,9A|
|Djose|Gap door charged sphere|0181EB|9B|
|Djose|Pedestal sockets|0181ED,0181EE|9D,9E|
|Djose|Charger state|0181F1|A1|
|Djose|Movement result, side, held frames|018225,018226,018228|D5,D6,D8|
|Djose|Gap ready, movement blocker|018229,01822B|D9,DB|
|Djose|Five upper controls|01822E..018232|DE..E2|
|Djose|All upper controls|018233|E3|
|Macalania|Ramp sphere,pedestal sphere|015A8E,015A8F|96,97|
|Macalania|Other bridge spheres|015A90,015A94|98,9C|
|Macalania|Held frames,state|015A9A,015A9B|A2,A3|
|Macalania|Side,first mound,bridge complete|015ABD,015ABE,015AC2|C5,C6,CA|
|Bevelle|Scene,player action,platform,pedestal motion|01F486..01F489|06..09|
|Bevelle|Socket ready flags|01F49F,01F4A0|1F,20|
|Bevelle|Held frames,blocker,optional prerequisite|01F4A1,01F4A4,01F4A9|21,24,29|
|Bevelle|Ride flag,push kind|01F4AC,01F4B0|2C,30|
|Zanarkand|Selected pattern byte|00F6C5|21|

Validation: extracted from each flow JSON's `workers[0].bindings`; matched the
readable flow statements/branch targets for each action. No runtime validation.
