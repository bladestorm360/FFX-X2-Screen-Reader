## Session 51 — 2026-04-28 — Save/load architecture correction + Yes/No cursor handler

### User correction of session 50
Session 50 docs/comments labeled save/load as "Iggy/Flash-based UI". User
pushed back: Iggy/Flash is for animation + mouse support only; menus
(including save/load) are TK. The Flash callout at the end of
FUN_00702FE0 / F70 / E00 is just visual handoff; the loc resolution and
buffer population that happens BEFORE the Flash call are TK code (in
FFX.exe), driven by the TK state machine in FUN_006EFxxxx-FUN_006F0xxxx.

So the popup-text hooks shipped in session 50 ARE TK-side hooks — they
read TK-resolved buffers populated by the TK language manager
(FUN_006480E0 → FUN_006DB320, sLanguageManager singleton). The
architectural framing in the comments / debug.md was wrong, but the hook
locations capture TK data correctly.

### What was missing: Yes/No cursor handling
Session 50 captured popup TEXT but not the Yes/No cursor toggle. Read
the TK state machine in full this session:

- DAT_00CE72D8 (RVA 0x8E72D8) — save/load state register
- DAT_00CE72F0 (RVA 0x8E72F0) — Yes/No popup cursor (0=button1, 1=button2)
- Three TK input handlers each reverse the selection stored in DAT_00CE72F0
  on Up/Down input (different popup states use different handlers, but the
  toggle behavior is identical):
    - FUN_006EFD90 (RVA 0x2EFD90) — input handler A
    - FUN_006F00E0 (RVA 0x2F00E0) — input handler B
    - FUN_006F06F0 (RVA 0x2F06F0) — input handler C
  Each reads input via FUN_006EF460, branches on event type
  (3=Up, 4=Down, 7=Confirm, 8=Cancel), and toggles cursor on Up/Down.

### Implementation
All three TK input handlers share `popupCursorOnLeave`. It reads the resulting
cursor, ignores Confirm/Cancel events that leave selection unchanged, resolves
the button label from the corresponding native buffer, then speaks the focused
label.

Button labels are populated by the most recent FUN_00702FE0 call (loc
0x204 → button 1 buffer, loc 0x205 → button 2 buffer), so they're
already cached in the buffers from the popup-show event.

### Confidence
- DAT_00CE72F0 is the Yes/No popup cursor: **0.95** (read all 3 input
  handlers in full — identical toggle pattern, used for Confirm decision
  in FUN_006EFD90).
- The 3 input handlers cover all save/load Yes/No popup states: **0.85**
  (three toggle locations found via decompile grep on
  identical cursor-reversal behavior).
- DAT_00CEB890/A90 are valid button labels at the time the cursor
  toggles: **0.95** (always populated by the most recent
  FUN_00702FE0(loc) → loc(0x204)/loc(0x205) call, which fires on popup
  appearance before any cursor input).

### Files modified
- src/menus/saveload.js — header comment rewritten to correct the
  architectural framing (TK state machine drives, TK helpers resolve,
  Flash mirrors visually). Added `popupCursorOnLeave` shared by 3 new
  hooks on the input handlers. 196 lines (under 500 cap).
- (Will update gameArchitecture.md and debug.md to reflect the
  correction in next pass.)

Build: install/ffx_sr.js redeployed. Bundle confirms 7 occurrences of
new symbols.

### Pending user testing
- Save game over existing slot → expect popup body, then "Yes"/"No" as
  user toggles between the two confirm options.
- Load game → same.
- Cancel popup or back out → no spurious announce (dedupe should keep it
  quiet).
- All session 44-50 features still working.

KEYWORDS: save load TK state machine, FUN_006EFxxxx FUN_006F0xxxx,
DAT_00CE72D8 state, DAT_00CE72F0 yes no cursor, FUN_006EFD90
FUN_006F00E0 FUN_006F06F0 input handlers, FUN_006EF460 input getter,
TK helpers FUN_00702FE0 FUN_00702F70 FUN_00702E00, Flash callout
incidental, button labels cached DAT_00CEB890 DAT_00CEBA90,
architectural correction Flash for animation only,
session 50 mislabeled as Flash-side


## Session 52 — 2026-04-28 — Decoder opcode fix + save/load popup body + cursor inversion

### Issue 1 (sphere grid popup): odd tags ("the Y standard sphere grid", "b Yes/b No")
Found FUN_008DFD70 (RVA 0x4DFD70) — FFX's own "advance to next char"
function. It's a switch on the lead byte that returns the byte length
of each opcode. From the decompile:

| Bytes              | Length | Notes |
|--------------------|--------|-------|
| 0x00               | 0      | end of string |
| 0x01, 0x02, 0x03, 0x05, 0x0F, 0x11 | 1 | controls without data |
| 0x04               | 3 if next < 0x30 else 2 | conditional |
| 0x06–0x0E, 0x10, 0x12–0x2F | 2 | controls with one data byte |
| 'G' (0x47)         | run    | special — consume run of consecutive G |
| other 0x30+        | 1      | regular charset chars |

Our `decodeFFXSegments` was treating EVERY byte < 0x2B (except 0x09/0x10)
as a 1-byte skip. So a 2-byte opcode like 0x07 or 0x13 had its DATA byte
fall through to the regular charset decode and produce a spurious char:
- 0x13 0x68 → "Y" (CHARSET[56], byte 0x68 - 0x30 = 56)
- 0x07 0x71 → "b" (CHARSET[65], byte 0x71 - 0x30 = 65)
That's exactly the artifacts the user reported in the sphere grid type
popup body and in the Yes/No button labels.

Fix in src/core/charset.js: introduced TWO_BYTE_OPCODES Set covering
0x06-0x0E, 0x10, 0x12-0x2A. Added 0x04 as 2-or-3-byte. Kept 0x00
as end-of-segment, 0x03 as 1-byte separator (per FUN_008DFD70 0x03 is
1-byte), and 0x0A as 2-byte separator (was 1-byte before — also wrong
per FUN_008DFD70). Bytes 0x01, 0x02, 0x05, 0x0F, 0x11 stay as 1-byte
skips via the existing `b < 0x2B` branch.

### Issue 2a (save/load popup): body silent, only buttons read
My session-51 hook on FUN_00702FE0 returned early when the body buffer
read empty (msg.length equals 0). That suppressed the popup announce
entirely — user heard cursor-toggle button labels later via the input
handler hooks but never the prompt body.
Fix: build `parts` from whichever fields are non-empty (msg/btn1/btn2),
announce if any of them have content. Worst case the user hears just
the buttons on popup open (still better than silence); in normal cases
the body should be present too.
Also added richer log line: `[SAVE POPUP BTN] msg="..." btn1="..."
btn2="..."` so we can diagnose if body really is empty in some popups.

### Issue 2b (save/load popup): cursor → button mapping inverted
User report: popup starts on Yes, cursor moves to No, but mod announces
the labels backward (says "No" when on Yes, "Yes" when on No).
Confirmed in decompile (FUN_006EFF60 line 182880): popup-init writes
The native popup initializer sets DAT_00CE72F0 to 1. User says the popup starts
on Yes, so cursor value 1 corresponds to Yes.
Earlier code mapped cursor=0 → POPUP_BTN_OK_BUF, cursor=1 → POPUP_BTN_NO_BUF
which is exactly inverted.
Fix: cursor=1 → POPUP_BTN_OK_BUF (loc 0x204, "Yes"), cursor=0 →
POPUP_BTN_NO_BUF (loc 0x205, "No"). Source-comment now spells out the
mapping with the line reference.

### Confidence
- FUN_008DFD70 byte-length table: **0.95** (read full switch in the
  decompile; matches FFX charset opcode behavior across other functions
  that loop over text bytes).
- 0x13 0x68 → "Y" mechanism: **0.95** (CHARSET[56] = 'Y', byte 0x68 -
  0x30 = 56, matches user's reported artifact).
- 0x07 0x71 → "b" mechanism: **0.95** (CHARSET[65] = 'b', byte 0x71 -
  0x30 = 65, matches user's "(b Yes/b no)" report).
- cursor=1 = Yes: **0.85** (initial DAT_00CE72F0 = 1 in popup-open
  handler matches user observation that popup starts on Yes; could be
  refined per-popup if a state uses different defaults).

### Files modified
- src/core/charset.js — TWO_BYTE_OPCODES Set + opcode branches; ~15
  added lines, no API change to callers.
- src/menus/saveload.js — popup body announce no longer requires non-
  empty msg; cursor → button mapping inverted; richer log line.

Build redeployed.

KEYWORDS: FFX charset opcode lengths FUN_008DFD70, two byte opcodes
0x06 0x07 0x0B 0x13 0x14, decodeFFXSegments fix, sphere grid Y
artifact, b Yes b No artifact, save load popup body silent removed
early return, cursor inversion fix, DAT_00CE72F0 = 1 popup init Yes,
loc 0x204 Yes loc 0x205 No


## Session 53 — 2026-04-28 — Load-game popup cursor seed + ESC menu architecture map

### Issue 1 (load game popup): body interrupted by Yes button focus
After session 52, body announce should fire (no longer suppressed by
empty-body early-return). User reports: "load game pop-up either not
reading or being interrupted by the yes button gaining focus."

Root cause: `lastYesNoCursor` initializes to -1. The TK input handlers
(FUN_006EFD90 / F00E0 / F06F0) fire every frame the popup is active.
First frame: cursor=1 (popup-init sets it), cache=-1, they differ →
announce "Yes". This double-announce on top of the body announce makes
it sound like the body was interrupted (or, if Tolk overlapped it,
clipped).

Fix in src/menus/saveload.js: in the FUN_00702FE0 popup-show hook,
after announcing the body, seed lastYesNoCursor receives DAT_00CE72F0. The
input-handler hook then sees cursor === lastYesNoCursor on the first
fire after popup-open and stays quiet. Subsequent toggles still
announce normally.

### Issue 2 (ESC menu): not reading
Status: identified architecture, not yet implemented. Defer to next
session.

ESC menu architecture (decompile-confirmed):
- FFEscMenu is a C++ class with vtable at 0x00B46C28. The active
  instance pointer is at `DAT_00CCC868` (RVA 0x8CC868). All ESC-menu
  state lives on the Flash side; TK is a thin wrapper.
- Page setup: `FUN_0068E870` (RVA 0x28E870) builds all pages at init.
  For each page (HomePage, SysSettingPage, AudioPage, VideoPage,
  KeyBindingPage, KeyboardPage, ControllerPage, ControlBar) it loops:
    1. FUN_006414A0 using loc_id, local_buf, 0x100 — resolve loc string
       via TK language manager
    2. thunk_FUN_006E9840 using local_buf, scratch, 0x100 — process /
       transform
    3. FUN_00A28D70 using the address of struct, scratch — IggyMenu::setString creates
       a Flash string handle
  Then calls `FUN_0068E090` / `FUN_0068DEE0` etc. with method name
  "setHomePage" / "setSysSettingPage" / etc. — IggyMenu::callFlash
  variants that pass the string handles to Flash.
- Cursor: read via FUN_00A28B20 using 0 (IggyMenu::readInt @ RVA 0x228B20)
  on the FFEscMenu instance referenced by DAT_00CCC868. Returns the current
  Flash-side cursor index.
- Per-frame loop: `FUN_00690130 showEscMenu` (RVA 0x290130) reads
  input every frame and dispatches "prevItem" / "nextItem" / "keyUp"
  to Flash via callFlash. Cursor moves happen entirely on the Flash
  side; TK only gets notified via the Flash → TK callbacks
  (EscMenuResume, EscMenuQuit, etc. registered at RVA 0x28FEC0+
  via `FUN_00A28E90 RegisterCallback`).

This is the OPPOSITE architecture of save/load: save/load owns state
in TK with Flash as visual mirror; ESC menu owns state in Flash with
TK as input proxy. So the existing "menus are TK only" rule doesn't
straightforwardly apply — ESC menu *requires* reading Flash state.

### Proposed ESC menu reader (next session)
1. Hook `FUN_00A28BA0 IggyMenu::callFlash` (RVA 0x228BA0) globally,
   filter on `args[1]` (method name) for "prevItem" / "nextItem".
   When fired, ESC menu cursor moved.
2. On move, call FUN_00A28B20 using the value pointed to by DAT_00CCC868, 0 via NativeFunction
   to read the current cursor index from Flash.
3. Maintain a mod-side cache of the current page's item labels. Hook
   `FUN_006414A0` (loc resolver) inside `FUN_0068E870` (or its
   per-page successors `FUN_0068E090` / etc.) and capture the
   resolved strings in order, scoped per-page.
4. On cursor move, announce `cachedItems[currentPage][cursorIdx]`.

Confidence on this plan: 0.7 — mechanism is clear from the
decompile but the TK→Flash boundary makes this trickier than save/load,
and there's no working precedent for it in our archive (the
read_iggy_menu probe was a dead end). Want to hold off until I can
read FUN_0068E090 / E870 in deeper detail to confirm exactly which
add-call delivers item N's label and how the page<->callback
correspondence works.

KEYWORDS: lastYesNoCursor seed popup open, FUN_00702FE0 onLeave
seed cursor cache, load game popup interrupt fix, FFEscMenu
DAT_00CCC868 instance, FUN_0068E870 page setup, FUN_006414A0 loc
resolver, FUN_00A28B20 IggyMenu readInt cursor, FUN_00A28BA0
callFlash dispatch, prevItem nextItem cursor events, ESC menu
deferred next session


## Session 54 — 2026-04-28 — Save/load cursor queue + ESC menu SWF verification

### Save/load: queue cursor focus behind body (correct fix)
User pointed out session 53's "seed lastYesNoCursor" approach REMOVED
the initial cursor announce instead of queueing it behind the body.
Real fix: announce both, but with the cursor focus using
interrupt=false so it queues behind the body's interrupt=true.

Implementation in src/menus/saveload.js:
- New `queueNextCursor` flag set true after the FUN_00702FE0 popup-body
  announce.
- `popupCursorOnLeave` reads the flag, uses interrupt=!queueNextCursor
  for the speak call, then clears the flag.
- First cursor announce after popup-open queues behind body
  (interrupt=false). Subsequent toggles stay responsive (interrupt=true).
- Removed the lastYesNoCursor seed — first cursor announce now happens
  normally, just at the right point in the speech queue.

### ESC menu SWF verification (per user request)
Verified escmenu.swf is animation-only. The decompiled AS3 directories
in `D:/Games/Dev/Custom/FFX/extracted_ffx/decompiled/`:

| SWF                 | scripts/ contents | Verdict |
|---------------------|-------------------|---------|
| escmenu             | EMPTY             | No AS3 logic — pure timeline animation |
| saveload            | EMPTY             | No AS3 logic |
| box                 | EMPTY             | No AS3 logic |
| chocobo             | scripts/chocobo_fla/ minimal | Minigame animation |
| openingscreen       | BitmapTextField, Tween, MainTimeline + many | **NOT title screen — autosave-attention notice** ("Loc_Texture_Saveload_Attention_Before_Game") |
| (no titlescreen.swf) | n/a               | Title screen has no SWF |

User is correct: ESC menu UI labels are NOT in escmenu.swf. The SWF
contains visual/animation timeline only; labels are dynamic text
fields that get populated from outside.

`openingscreen.swf` was a red herring — its MainTimeline.as sets text
to `Loc_Texture_Saveload_Attention_Before_Game`. That's the "this game
uses autosave" notice, not the title screen menu.

### ESC menu read mechanism — still unknown
Architecture confirmed from the decompile:
- FUN_0068E870 (RVA 0x28E870) builds all ESC menu pages at init: for
  each page it loops loc resolve → process → IggyMenu::setString →
  callFlash("setHomePage" / "setSysSettingPage" / etc.).
- Labels live in IggyMenu's internal string storage on the Flash side
  (after setString); they are NOT preserved in any TK global.
- Cursor state likewise lives Flash-side, accessible via
  IggyMenu::readInt (FUN_00A28B20).
- escmenu.swf has no AS3, so cursor logic must be in Iggy's built-in
  menu handling rather than custom AS3.

What I haven't found: a TkWindow slot that the ESC menu writes text
to (the way the title screen + main field menu do). If such a slot
existed, the menus.js TODrawMessage hook would already be catching it.
The user reports the ESC menu does not announce — so either no slot,
or the slot's text format isn't FFX-charset.

User remembers an old probe (the one that "finally got the title
screen reading") incidentally caught the ESC menu and system settings
but not the help menu. I haven't been able to identify which probe
that was — none of the archived probes have a clear "ESC menu reader"
I can port. The closest candidate was `title_screen_cursor.js` but
that's a discovery probe, not a working reader.

Question to user: do you remember any specific function name / RVA
the original ESC-reading probe hooked, or where in your Frida output
log it appeared as readable? With that anchor I can find the
mechanism in the decompile. Without it I'd have to either:
(a) ship a callFlash-based reader (confidence ~0.7, below the project
threshold), or
(b) instrument the existing TODrawMessage hook to log every active
slot's textPtr bytes when ESC is pressed, so we can see at runtime
which slot (if any) the ESC menu uses.

KEYWORDS: save/load cursor queue interrupt false, queueNextCursor
flag, escmenu.swf no AS3 confirmed, openingscreen.swf autosave
notice not title, ESC menu data flow IggyMenu setString readInt,
ESC menu unknown TK slot, original probe identity needed


## Session 55 — 2026-04-28 — Hook level changed: per-slot dispatch → per-frame iterator

### User insight
"It was whichever probe was used to initially read the title screen.
I think it was rolled into UniversalMenuBattleReader. If I'm right,
that probe will read the ESC menu whereas our production build does not."

The universal probe iterates ALL 8 TkWindow slots EVERY FRAME via its
onTick. Production was hooking FUN_008B5D70 (per-slot dispatch) — same
per-slot logic, but only fires for slots the game actually dispatches.

Looking at FUN_008ABCE0 (TODrawMessageWindow, RVA 0x4ABCE0) again, the
per-slot dispatch into FUN_008B5D70 is gated behind:
The recovered visibility/update gate considers flag 0x200, the selected record through DAT_01865afc (record offset 0x14, nested state offset 8 must be 2), slot field 0x1A (expected values 1 or 3), slot field 0x10 (special value 0x4000), nonzero field 0x0C, DAT_00C59558 (value 1), and nonzero DAT_01841DCC. If any of those disabling/alternate-state conditions holds, it calls FUN_006481F0; a zero result then allows FUN_00906BF0 and FUN_008B5D70 for the slot. The surrounding user insight explains why this gate matters to rendered information; these are recovered historical conditions, not a copied function body.

For most active slots one of those conditions holds. But the ESC menu
home page sits in a state where ALL of them are false, so FUN_008B5D70
never fires for that slot — production was blind to it.

The iterator FUN_008ABCE0 itself runs unconditionally per frame and
walks 0..7 regardless of dispatch. Hooking THAT and iterating slots in
onLeave matches what the universal probe did and reaches every active
slot.

### Implementation
Replaced the FUN_008B5D70 hook with FUN_008ABCE0 (RVA 0x4ABCE0):
- Single onLeave that loops slot = 0..7
- Per-slot logic identical (active flag, vis check, full-text decode,
  content-diff cache, announceTkContent on change)
- No more per-slot hook bouncing — one hook per frame total

### Confidence
- FUN_008ABCE0 is the per-frame iterator: **0.95** (read function in
  full, contains the a loop that processes entries until the index iVar3 reaches eight loop).
- Iterating manually catches gated-out slots: **0.9** (matches universal
  probe behavior; user's working oracle).

### Files modified
- src/menus/menus.js — hook target swapped 0x4B5D70 → 0x4ABCE0,
  per-slot logic moved inside a 0..7 for loop in onLeave.

Build: install/ffx_sr.js redeployed; tkLastActive symbol confirmed in
bundle.

### Pending user testing
- ESC menu (Resume / Settings / Help / Quit) → expect to read on open
  and on cursor moves
- All previously-working menus and popups → no regressions
- Save/load popup with the queue-cursor fix from session 54

KEYWORDS: TODrawMessageWindow per-frame iterator, FUN_008ABCE0 hook,
FUN_008B5D70 gated dispatch, ESC menu missed by per-slot hook,
universal probe equivalent in production, all 8 slots iterate


## Session 56 — 2026-04-28 — No-dedup rule + popup body-only + dialogue length filter removed

### User feedback this round
1. Save/load popup format wrong: announcing "Load this game? Yes No" —
   should announce "Load this game?" then queue the focused option.
2. No deduplication: dedup was suppressing pop-up bodies on legitimate
   re-shows (pick No, pick another slot → same body should re-announce
   but didn't). General rule: don't dedup unless explicitly told.
3. NPC dialogue still not reading — acquired items reads, dialogue
   doesn't. May use a different text panel.
4. ESC menu still not reading.

### Saved memory rule
`feedback_no_dedup` — default to NOT having `lastSpoken` / `lastText`
content-dedup caches on production speech paths. The user will
explicitly say "this is announcing too often, add dedup" if dedup is
actually warranted. CHANGE DETECTION (cursor index changed since last
frame) is distinct and OK; per-frame typewriter dedup on the dialogue
decoder is also OK because the alternative is broken Tolk.

### Save/load popup body-only fix
- `0x302FE0` (showPopUpWithBtn) now reads ONLY the body buffer, not the
  button labels. Sets queueNextCursor receives true so the immediately-
  following input-handler cursor announce queues behind it.
- Removed `lastPopupSpoken` content-dedup from the body announce.
- Removed `lastHelpSpoken` content-dedup from setSaveLoadHelpText.
- Removed `lastPopupSpoken` from showPopUpStd.
- Kept `lastYesNoCursor` (CHANGE DETECTION — fires every frame the
  popup is up, would otherwise re-announce the same focused button
  continuously).

### dialog.js: removed length<3 filter
Removed the `text.length < 3` gate that was rejecting short fragments
(might have been blocking legitimate short replies like "Hi."). Kept
the `lastDialogueText` per-frame typewriter diff with a clearer
comment — without it, the per-frame re-decodes of the same dialogue
line would Tolk-spam at ~60/sec and break speech entirely. This is
change-detection on identical content, not user-event dedup; the
no-dedup rule explicitly allows it.

### NPC dialogue: still not reading
The mod hooks `FUN_00776A50` (event-message decoder, RVA 0x376A50) in
dialog.js. That decoder is called only by `FUN_0076F580` and
`FUN_0076F960`, both event-message renderers reading
`DAT_0112A01C`. If NPC dialogue uses a different message system, this
hook won't fire for it.

What I haven't yet found: confirmation that NPC dialogue (NPC walks
up, presses confirm, dialogue box appears) routes through
`FUN_00776A50` vs. some other path. The acquired-items popup reads
because it's a TkWindow popup caught by the menus.js iterator — that's
a different mechanism entirely. Need to identify the NPC dialogue
mechanism.

### ESC menu: architecture mapped, no clean read path
Verified:
- `FUN_0068E870` is FFEscMenu's page-builder, called from
  `FUN_0068E170` (ctor) and `FUN_0068F540` (probably language-change
  reload). It runs ONCE at game start (and on language change), not
  per-ESC-menu-open.
- It loop-resolves loc strings via `FUN_006414A0` → `FUN_006DB320`,
  then pushes them to IggyMenu via `FUN_00A28D70` (setString).
- After init, the labels live in IggyMenu's internal Flash storage.
  The TK side has no per-page label table we can read directly.
- The universal probe's `getText` hook on `FUN_006DB320` (RVA 0x2DB320)
  fires for EVERY loc resolution game-wide. At GAME INIT it captures
  the ESC menu labels (along with everything else). When the user
  opens the ESC menu later, no new resolution happens — labels are
  already cached on the Flash side.

So porting just the getText hook would speak loc strings at game
init (overwhelming) but give nothing at cursor-change time. To get
cursor-change announces we'd need a multi-part hook system:
(a) capture labels with their indices during `FUN_0068E870`,
(b) maintain a per-page table mod-side,
(c) hook `FUN_00A28BA0 callFlash` for "showEscMenu" / "prevItem" /
    "nextItem" events,
(d) read FFEscMenu cursor via NativeFunction-call to
    FUN_00A28B20 IggyMenu::readInt using the value pointed to by DAT_00CCC868, 0,
(e) announce `pageTable[currentPage][cursor]`.

Confidence on the multi-part plan: ~0.75. Below the 0.9 threshold.
The uncertain pieces are the cursor-index → page-item correspondence
(setHomePage takes args in a specific order; we'd need to verify the
order matches the cursor's index space) and the NativeFunction calling
convention for IggyMenu::readInt.

### Pending user decisions
- ESC menu: ship the 0.75-confidence multi-part plan and iterate, or
  research more first?
- NPC dialogue: which dialogue case is failing? "Walk up to NPC, press
  confirm, dialogue box appears" vs. other? Need a code-side anchor
  (any specific NPC, specific area) to know where to look in the
  decompile.

KEYWORDS: no-dedup rule production speech, lastPopupSpoken removed,
lastHelpSpoken removed, queueNextCursor retained, lastYesNoCursor
change detection kept, dialog.js length filter removed, dialog.js
typewriter dedup retained, ESC menu architecture FFEscMenu init time
loc resolution, getText hook port game init only no cursor change,
NPC dialogue mechanism unknown

## Session 52 — 2026-04-28 — NPC dialogue mechanism RE (decompile-only)

### Question
"Acquired items" popup reads correctly via the TkWindow iterator hook in
`src/menus/menus.js` (`FUN_008ABCE0` RVA 0x4ABCE0), but interactive NPC
dialogue ("walk up to NPC, press confirm, dialogue box appears with
character name + speech") never announces. The `dialog.js` hook on
`FUN_00776A50` (RVA 0x376A50) also never fires for NPC speech (no
`[DIALOGUE]` lines in `FFX_SR_Latest.log`).

### Decompile-only research findings

1. **`FUN_00776A50` is NOT the NPC dialogue decoder.** It is the
   event-message *list* decoder, called only by `FUN_0076F580` and
   `FUN_0076F960` — both per-frame renderers walking the
   message-list at `DAT_0112A01C` whose entries are 0x138 bytes each
   with type tags `'\0'..'\x06'`. Those entries are sub-menu /
   choice-prompt / event-message-bundle items, not NPC speech text.

2. **NPC dialogue uses the same 8-slot message-window pool that
   menus.js already iterates** — `DAT_018676B0` (RVA `0x14676B0`).
   The text byte stream for any active slot is at `slot+0x5C` (variant
   A) and/or `slot+0x94` (selected variant for FUN_008B9600 / draw).

3. **The single universal text-installer for that pool is
   `FUN_008B8980` (RVA `0x4B8980`)**: signature
   void using slot_idx, textPtr_A, textPtr_B. Every
   "open message" path observed in the decompile funnels through it:
   - `FUN_008589F0` — Atel-script NPC dialogue handler (the chr-event
     namespace 0x5 / RVA 0x4891F8 path)
   - `FUN_00860D00` / `FUN_00860DE0` — Atel-script simple-message
     bytecode handlers
   - `FUN_0089B0B0` / `FUN_0089B140` / `FUN_0089B1A0` / `FUN_0089B200`
     / `FUN_0089B260` — battle / system popup wrappers
   - `FUN_00858B30` / `FUN_00858B90` — battle-window setup
   Confidence the function does what I think: **0.95**. Body is short,
   every field write matches every observed read site at slot+0x5C /
   0x60 / 0x74 / 0x78 / 0x90 / 0x94.

4. **Why the existing menus.js TkWindow iterator skips NPC dialogue:**
   `readTkWindowContext` reads the visibility field one byte after the current
   entry pointer, then `announceTkContent`
   is gated by when (ctx.vis differs from 1) skip this entry. Reading the cur-array
   writes in the decompile, `cur+1` is the **selector cursor mode**:
   - 0 = no selector (pure dialogue / no cursor)
   - 1 = selector active (yes/no, item-acquired confirm, etc.)
   - 2 = selector confirmed
   It is set to 1 only by `FUN_008B5F60` (TODrawSelectCursor) when
   slot offset 0x2C equals 2 and cur+1 equals 0. Pure NPC dialogue (no cursor /
   no selectable items) keeps cur+1 equals 0, so the iterator filter
   vis differs from 1 skips it. That is why "acquired items" (one-item
   selector with confirm) reads but plain NPC speech does not.

### Recommended fix (NOT applied — research-only round)

Replace the FUN_00776A50 hook in `src/menus/dialog.js` with a hook on
**`FUN_008B8980` (RVA `0x4B8980`)** in `onEnter`:
- `args[0]` = slot index (0..7)
- `args[1]` = textPtr_A (the byte stream — same FFX charset as menu
  text, decode with `decodeFFXSegments`)
- `args[2]` = textPtr_B (international variant — usually equal to
  args[1])

Diff against `lastDialogueText` per-call (one call per message
appearance, not per-frame), speak using text, false to queue without
interrupting. This catches NPC dialogue, acquired-items popups,
save/load yes/no, battle messages, system prompts — **one hook,
universal coverage**, zero typewriter redundancy because
FUN_008B8980 fires once per text-install, not per draw frame.

The existing TkWindow iterator hook in `menus.js` should also have
its vis differs from 1 filter relaxed for non-selector text announce — but
once FUN_008B8980 covers all text installs upstream, the iterator
hook can be downgraded to selector-cursor-change handling only.

### Files updated
- `Docs/gameArchitecture.md` — added §"Message Window System" with full
  slot+cur layout, callers, and the `FUN_008B8980` description.
  Also corrected the existing §"Event-Message System" header note
  clarifying that `FUN_00776A50` is for the event-message list, not
  NPC dialogue.
- `Docs/session51_current.md` — this entry.

### Pending user decisions
- Port to Frida prototype first, or straight to C++? (Frida-first
  rule applies — but FUN_008B8980 is a decompile-confirmed universal
  installer, comparable confidence to existing hooks already in
  production.)
- Keep `FUN_00776A50` hook for event-message-list / cutscene-subtitle
  cases or remove entirely once `FUN_008B8980` is wired up?

KEYWORDS: NPC dialogue mechanism FUN_008B8980 RVA 0x4B8980 universal
text installer slot 0x5C 0x94 textPtr message window pool DAT_018676B0
TkWindow filter vis cur+1 selector mode FUN_00776A50 not dialogue
FUN_0076F580 FUN_0076F960 event-message-list FUN_008589F0 atel CHEvent
NPC dialogue hook target proposed not applied research only round

## Session 57 — 2026-04-28 — ESC menu cursor read-path mapped from decompile

Decompile-only research round. No code changes. Updated
`gameArchitecture.md` §"FFEscMenu Class" with full read-path mapping
plus offset and global tables.

### What was unblocked

Previous session left ESC-menu read path at 0.75 confidence with the
note "uncertain pieces are the cursor-index → page-item correspondence
and the NativeFunction calling convention for IggyMenu::readInt."

Both are now resolved by deep reads of:
- `FUN_0068E730 EscMenuHelpTextRefresh` — the per-cursor-move callback
- `FUN_0068E6E0` — the deferred help-text resolver
- `FUN_0068E870` — the page builder (init only)
- `FUN_00A28B20`, `FUN_00A28B60`, `FUN_00A28BA0`, `FUN_00A28D70`,
  `FUN_00A288B0` — the IggyMenu helpers (readInt, readString,
  callFlash, IggyValue::setString, IggyValuePath::Init)
- `FUN_006414A0`, `FUN_006DB1A0`, `FUN_006DB320` — the localization
  resolver chain

### Key findings

1. **Cursor read mechanism**: `FUN_00A28B20` is `__thiscall` on a
   STACK-LOCAL `IggyValuePath` accumulator (NOT on FFEscMenu).
   Calling it standalone with the FFEscMenu pointer in ECX would
   read garbage. The proven path is to set up the path via
   FUN_00A288B0 using player, eventPayload first — which is exactly
   what `FUN_0068E730` already does as the named-callback handler
   for `EscMenuHelpTextRefresh`. **So the correct hook target is
   `FUN_0068E730`, not a synthetic readInt using 0 call.** The
   readInt(0)/readInt(1) inside that handler reads the
   (page-tag, item-tag) Flash-side pair from the event payload —
   not zero-based indices, but stable Flash-defined IDs that
   match against `DAT_00C33DB8`.

2. **Label resolution mechanism**: After init, the C++ side does
   NOT keep label strings — they are stored in Flash. The C++ side
   only re-resolves the **help text** (description) on cursor move.
   This means:
   - **v1 (announce help text)** — clean, requires only one hook on
     `FUN_0068E6E0` (RVA 0x28E6E0). FUN_0068E6E0 using loc_id_ptr
     fires once per cursor move with the matched help-text loc id.
     Mod-side, read the localization ID through the supplied pointer and
     resolve it with FUN_006414A0 into a 256-byte buffer before speaking it.
     The string matches the value written at offset 0x121 of the initialized
     menu instance before being sent to Flash through `setHelpText`.
   - **v2 (announce label too)** — capture labels at init by
     hooking `FUN_00A28BA0 IggyMenu::callFlash` and filtering on
     method name == `setHomePage` / `setSysSettingPage` / etc.
     Parse the IggyValue array (16-byte stride; tag at +0 == 5 for
     string; char* at +8; len at +12). The (sVar1, sVar2) → page
     index + item index mapping requires runtime evidence we
     don't yet have — **defer label v2 until v1 is in production
     and we can capture the (sVar1, sVar2) values during real
     gameplay**. Per project rules we ship help-text-only first
     and add labels when evidence is available, NOT speculative
     index-mapping that could mislabel.

3. **Page tracking**: There is no "current page" field in the
   FFEscMenu instance. The page is implicit in the (sVar1, sVar2)
   pair Flash sends with each `EscMenuHelpTextRefresh` event. For
   v1 (help-text) we don't need to know the page name — the help
   text already includes context.

4. **Loc resolver is callable from mod-side**: `FUN_006414A0` is
   __cdecl using loc_id, buf, bufsz — no ECX gymnastics
   needed. Internally it gets the sLanguageManager singleton and
   sets ECX before calling `FUN_006DB320`. This is reusable for
   any future loc-id-based reads (other menus, prompts, etc.).

### Confidence ratings

| Component | Confidence | Notes |
|-----------|-----------|-------|
| `FUN_0068E730` is the cursor-move event handler | 0.97 | Registered under name `EscMenuHelpTextRefresh`; read directly from FUN_0068FEC0 callback registration. The Iggy callback receives the menu and event structure. |
| `FUN_0068E6E0` is the help-text loc resolver | 0.95 | Body explicitly calls FUN_006414A0 using the value pointed to by param_1, the address of this.buf+0x121, 0x100 — direct evidence. Scheduled by FUN_0068E730 via FUN_00A28DF0 using FUN_0068E6E0, the address of entry. |
| `FUN_006414A0` is __cdecl using loc_id, buf, bufsz and safely callable | 0.92 | The wrapper hides MSVC ECX setup; called dozens of times with no `in_ECX` use. Singleton must be initialized — guarded by `FUN_006DB1A0` panic. |
| `+0x121` is the help-text buffer in FFEscMenu instance | 0.95 | Direct evidence in FUN_0068E6E0 (writes here) and FUN_00690130 (reads from here for setHelpText push). |
| (sVar1, sVar2) are NOT zero-based indices | 0.93 | Evidence: button-click handler (`FUN_0068F790`) uses when (sVar1 equals 0xd) — a specific value, not a sequential index. Help-text table has 35 entries with non-sequential `(sVar1, sVar2)` pairs. |
| Label-v2 mapping (sVar1, sVar2) → page index | 0.40 | Without runtime evidence we can't confirm the exact mapping. Decompile gives us the table layout but not the in-Flash IDs. |
| **Overall (v1 = announce help text)** | **0.92** | Above the 0.9 production threshold. Single hook, clean read, no speculation. |
| **Overall (v2 = announce label + help text)** | **0.55** | Below threshold. Defer until runtime evidence. |

### What v2 needs to close to ≥0.9

Per project rule (no Frida probes for menu structure, no runtime
discovery), v2 cannot be shipped speculatively. The path forward is
to ship v1, observe the (sVar1, sVar2) values that come through the
hook in real gameplay logs, and on subsequent sessions, decode the
mapping from real evidence. **No additional decompile reads alone
will close the gap** — the mapping lives inside `escmenu.swf`'s
ActionScript-replacement timeline scripts, which we already verified
are absent (the SWF has zero ActionScript). The Flash-side IDs are
encoded as MovieClip frame data we cannot statically read.

### Files updated
- `Docs/gameArchitecture.md` — replaced the §"FFEscMenu Class"
  superseded note with full read-path mapping: page-builder loc IDs,
  loc resolver call signature, per-frame update, named-callback
  table, cursor-change event chain, instance offsets, key globals.
- `Docs/session51_current.md` — this entry.

### Pending user decisions
- Approve v1 (announce help text) for Frida prototype, or research
  more first?
- Defer v2 (announce label) until first v1 logs come in, or attempt
  a cleaner approach now (e.g., hook `FUN_00A28BA0` for setHomePage
  to capture labels even though we can't yet map them)?

KEYWORDS: ESC menu cursor read path mapped EscMenuHelpTextRefresh
FUN_0068E730 FUN_0068E6E0 FUN_006414A0 loc resolver __cdecl callable
sLanguageManager DAT_00CDED48 DAT_00CCC868 +0x121 help text buffer
sVar1 sVar2 page tag item tag DAT_00C33DB8 35 entries help text table
IggyValue 16 bytes tag 5 string FUN_00A28BA0 callFlash FUN_00A28D70
setString FUN_00A28B20 readInt stack local IggyValuePath FUN_00A288B0
PathFromRef FUN_0068E870 page builder setHomePage 0x217 0x219 0x21B
0x21D 0x21F setSysSettingPage 0x221 0x223 0x225 setControlBar 0x20E
0x213 0x215 v1 help text only confidence 0.92 v2 label deferred



## Session 57 — 2026-04-28 — NPC dialogue + ESC menu read paths shipped

### NPC dialogue: hook FUN_008B8980 (TkObjectMessage text installer)
Agent research (decompile-only, confidence 0.95) found that NPC speech
routes through the TkObjectMessage 8-slot pool (DAT_018676B0) just like
acquired-items popups, BUT with the curArray's selector-cursor byte
(curPtr+1) at 0 because there are no selectable items.

The menus.js TODrawMessageWindow iterator filters when (ctx.vis differs from 1) skip this entry — that's the byte that gates non-cursor text out. NPC
dialogue stays at vis=0, gets skipped, never announces.

Universal text installer FUN_008B8980 (RVA 0x4B8980) fires once per
text install: void using slot, textPtr_A, textPtr_B x86
__cdecl. args[1] is the FFX-charset byte stream (same format as menu
text) — what gets written to slot+0x5C.

dialog.js now hooks FUN_008B8980. To avoid double-announce with the
menus.js iterator, we read curArray[slot]+1 (selector cursor mode) at
install time and skip if == 1 (menus.js handles those). Slots with
selector mode 0 are NPC dialogue / non-interactive text; we announce
those.

The existing FUN_00776A50 hook is kept for cutscene-subtitle / event-
bundle items (which the agent confirmed is what FUN_00776A50's renderers
actually drive — distinct from interactive NPC speech). Both use a
shared announceDialogueText using ptr, source helper.

### ESC menu: hook FUN_0068E6E0 (help-text resolver)
Agent research (decompile-only, confidence 0.92 for v1) found:
- FFEscMenu's labels live in IggyMenu Flash storage after init.
- On cursor move, Flash dispatches "EscMenuHelpTextRefresh" to C++
  (FUN_0068E730), which matches a (sVar1, sVar2) tag pair against a
  35-entry table at DAT_00C33DB8 to find a help-text loc id, then
  schedules FUN_0068E6E0(int* loc_id_ptr).
- FUN_0068E6E0 calls FUN_006414A0(loc_id, buf, 0x100) — the language
  manager — and writes the help text at offset 0x121 of the menu instance
  referenced by DAT_00CCC868.
- FUN_006414A0 is __cdecl(loc_id, buf, bufsz) and safely callable from
  mod-side (it wraps the sLanguageManager ECX setup internally).

New `src/menus/escmenu.js` (54 lines):
- Hooks FUN_0068E6E0 (RVA 0x28E6E0).
- onEnter: reads loc id from args[0]+0, calls FUN_006414A0 via
  NativeFunction with a pre-allocated 256-byte scratch buffer, decodes
  as ASCII C-string, speaks with interrupt=true (cursor moves are
  user-driven and want immediate feedback).
- Also logs (sVar1, sVar2) tags so a future session can correlate
  tag-pair → page/item-index from real-play logs (the Flash-side
  mapping isn't statically derivable from escmenu.swf since it has no
  AS3, only timeline data).
- No content dedup per the no-dedup rule.

This v1 announces the help text for the focused option ("Return to
the game" for Resume, etc.) on each cursor move. v2 would announce
the actual button label (e.g. "Resume") — that's deferred because the
(sVar1, sVar2) → label mapping requires runtime evidence.

### Files modified
- src/menus/dialog.js — added FUN_008B8980 hook with vis filter, kept
  FUN_00776A50 as secondary, shared announceDialogueText helper
- src/menus/escmenu.js — new module, 54 lines
- src/menus/index.js — registers escmenu.install()

Build deployed. Bundle confirms 7 occurrences of new symbols.

### Pending user testing
- Walk to NPC, press confirm → expect dialogue line to read; advance
  with confirm → next line reads
- Press ESC → expect ESC menu help text to read on each cursor move
  (Up/Down between Resume / Save & Quit / Settings / Help / Quit Game)
- All previously-working features → no regressions; if a popup that
  was reading via menus.js now double-announces via dialog.js too,
  the vis filter needs adjustment

KEYWORDS: NPC dialogue FUN_008B8980 universal text installer,
TkObjectMessage cur+1 selector cursor mode, dialog.js dual hook
008B8980 + 00776A50, ESC menu FUN_0068E6E0 help text resolver,
FFEscMenu IggyMenu Flash labels, FUN_006414A0 language manager cdecl,
sVar1 sVar2 Flash tag pair logged for future label v2 mapping

## Session 58 — 2026-04-30 — NPC dialogue re-ship + ESC bleed fix + submenu/help probes

### Four bugs in parallel (agent investigation)
User reported: (1) NPC dialogue not reading; (2) Items/Abilities/Overdrive/Aeons
submenus silent; (3) Help menu reads per-frame and repeats partials on idle; (4)
ESC menu speaks bleed text from prior selection. Spawned 4 parallel agents to
investigate. Results synthesized and acted on.

### NPC dialogue: re-ship FUN_008B8980 with segs[0] decode + slot+0x94
The session 57 ship of FUN_008B8980 was reverted because it joined every
decoded segment, producing all dialogue pages stitched together as one
utterance ("If you're near death, really near death, use this!..."). Diagnostic-
only hook had been left in place since.

This session re-ships speech using:
- the pointer stored at winPtr offset 0x94 — the post-install authoritative active
  page pointer (decompile L555081). Reading +0x94 is portable; trusting
  args[1]/args[2] requires duplicating the function's language-toggle logic
  at FUN_008ac340/FUN_00785290.
- decodeFFXSegments using pagePtr, 256, 0 then take **`segs[0]` only** — the
  first FFX text run before any 0x00/0x03/0x0A terminator, i.e. the active
  dialogue line. Joining segments was the v57 bug.
- vis filter preserved from v57: skip slots with curArray entry slot+1 equals 1
  (yes/no popups, save/load — menus.js iterator handles those).
- Existing `lastDialogueText` diff handles re-fires (focus-loss / unpause
  re-installs with the same arg2; observed 3x in mod log per page advance).

Also removed the per-frame FUN_008DEDF0 diagnostic from dialog.js — it was
useful for confirming the +0x94 mechanism but ships per-frame, which violates
the "no per-frame checks" rule for production.

### ESC menu text bleed: zero-fill scratchBuf
Agent confirmed mod-side bug from log byte inspection: Memory.alloc using 256 at
`escmenu.js:41` is reused without zero-fill. `FUN_006DB320` (the language
manager's underlying writer) does NOT guarantee a null at the new shorter
string length, so readCString using 256 walks past the new content into the
prior call's leftover bytes. Repro: highlight "Change System Settings", then
"Resume Game" → buffer reads back as "Resume Gamem Settings".

Fix: pre-built ZERO_256 receives new Uint8Array using 256; scratchBuf.writeByteArray using ZERO_256.buffer immediately before each `resolveLoc` call. Now the writer's
output is followed by guaranteed zeroes; readCString stops at the true new-
string boundary regardless of writer behavior. One-line fix, no game-side
change. The architecture doc claim that FUN_006DB320 null-terminates needs
correcting — runtime evidence contradicts it.

### Help menu per-frame: probe authored, NOT shipped yet
Help menu currently hooks FUN_00776A50 (per-frame event-message LIST decoder
inside FUN_0076F580/FUN_0076F960 renderers). On Help screen open the list
re-decodes ~60x/sec and the lastDialogueText diff doesn't suppress because
the renderer cycles through N different items per frame.

Agent identified candidate event-driven hook: **FUN_00776220 (RVA 0x376220)**
— the event-message list cursor/input dispatcher. Returns a status through its
first output argument:
- 0 = no input (idle path)
- 1 = cursor advanced to a new item (DAT_0112A088 / DAT_0112A084 changed)
- 2 = confirm/cancel with downstream state (DAT_0112A038 set)

Open question: is FUN_00776220 itself per-frame (early-returns on idle)
or only fires on input? Probe answers this. Cutscene subtitles also flow
through FUN_00776A50 and have no user input, so the existing FUN_00776A50
hook needs to stay for cutscenes; FUN_00776220 would be additive for
interactive event-message lists like the Help screen.

Authored `Docs/Scripts/Frida/FFX/help_dispatcher_probe.js` — counts total
calls, retcode distribution (0/1/2), and focus-pointer changes; emits a
periodic summary every ~10s plus per-event log. **NOT executed.** Awaiting
user to run via run_frida.bat before deciding production swap.

### Field submenus: probe authored, NOT shipped yet
Items/Abilities/Overdrive/Aeons (and Equip/Status/Formation/Help) submenus
are silent because none of their fn_ctrls are hooked. The pattern is the
same shape as the already-solved "Right battle menu silent" (debug.md:240)
— hook the per-submenu fn_ctrl, diff cursor before/after.

Candidate fn_ctrl RVAs (per gameArchitecture.md:1076-1112, agent confirmed
0.92 confidence by reading function bodies):
- Abilities: 0x4C6080
- Overdrive: 0x4D0430
- Aeons: 0x4CC300
- Equip: 0x4CEFF0
- Status: 0x4D1DD0
- Formation: 0x4D5510
- Help: 0x4BEF20

Authored `Docs/Scripts/Frida/FFX/field_submenu_probe.js` — hooks all 7
fn_ctrls, captures state (param_1+0x1C) and 8-slot cursor snapshot
before/after, logs first fire + state changes + cursor changes per submenu.
**NOT executed.** Awaiting user to run, open each submenu once, arrow
through items, then we wire the fn_ctrls into menus.js using the same
pattern as the existing field-menu fn_ctrl hook (menus.js:198-241). Item-
text reads (confidence 0.5-0.7 for some submenus per agent) deferred to a
second probe per the project rule "no production code below 0.9".

### Files modified
- `src/menus/escmenu.js` — added `ZERO_256` zero buffer + zero-fill before
  each resolveLoc call (2 lines added)
- `src/menus/dialog.js` — re-shipped FUN_008B8980 production speech with
  segs[0] + vis filter; removed per-frame FUN_008DEDF0 diagnostic and the
  unused decodeSnippet/lastPageStart helpers (~80 lines, was 144)
- `Docs/Scripts/Frida/FFX/field_submenu_probe.js` — NEW (probe, not run)
- `Docs/Scripts/Frida/FFX/help_dispatcher_probe.js` — NEW (probe, not run)

Build deployed via src/build/build.bat (twice — once for the initial change,
once after restoring the vis filter).

### Pending user testing
- Talk to NPC, confirm to advance pages → expect each line to read once.
  No double-announce on yes/no confirm popups (vis filter active).
- ESC menu Up/Down between Resume / Save & Quit / Settings / Help / Quit
  Game → each highlighted help text reads cleanly with no trailing bleed
  from prior selection.
- Run probes when ready: `run_frida.bat` → 1 (FFX) → `field_submenu_probe`
  / `help_dispatcher_probe`. Output to `Docs/Scripts/Frida/output/FFX/`.

KEYWORDS: NPC dialogue re-ship FUN_008B8980 segs[0] decode, slot+0x94
post-install active pointer, dialog.js vis filter restore, ESC menu bleed
zero-fill scratchBuf Memory.alloc not zeroed, FUN_006DB320 no null
terminator at new shorter length, field submenu fn_ctrl probe Abilities
Overdrive Aeons 0x4C6080 0x4D0430 0x4CC300, help menu per-frame
FUN_00776220 dispatcher probe retcode 1 cursor 2 confirm cancel

## Session 59 — 2026-04-30 — TkWindow iterator: actual exact port of universal probe

### Bug
User reported the field-menu Items submenu has been silent since the move
to the production Frida-gadget mod, despite multiple prior sessions claiming
"Items + ESC menu silent — Solved" via successive hook iterations
(FUN_008B8980 → FUN_008B5F60 → FUN_008B5D70 → 0x4ABCE0 iterator). The user
also reminded me that the working oracle, `universal_menu_battle_reader.js`,
DOES read Items correctly, and that the production code was supposed to be
an "exact port" of universal's TkWindow loop. It wasn't.

### Diff between universal (working) and prior production
Lined up universal_menu_battle_reader.js:1683-1762 against menus.js:272-345
and found four concrete divergences from "exact":

1. **Extra activity gate.** Production added winPtr.readU16 equals 0
   skip BEFORE checking `ctx.vis`. Universal has no such gate — it skips
   only on not ctx or ctx.vis equals 0. Most likely cause of Items silence:
   Items' winPtr+0 stays 0 even when curPtr+1 (vis) goes to 1, so
   production short-circuited before ever reading the context.
2. **Speech behavior on open.** Universal speaks `segs[0]` (header) then
   `segs[idx+1]` (focused item) as two log entries; production called
   announceTkContent using slot, ctx which speaks segs.join using ' ' —
   different text shape, drowns the focused item in the surrounding list.
3. **Popup branch.** Universal has an explicit tpChanged and wasVis differs from 0
   branch that fires when textPtr shifts while the slot stays visible
   (the Items confirm-into-inventory case). Production conflated this
   with the open path via fullText string-diff, which can miss textPtr-
   only changes.
4. **Cursor hook.** Universal handles cursor changes inside the per-frame
   loop (when (ctx.idx differs from prevTkCursors entry key)); production used a
   separate hook on FUN_008B6B20 (TkWindow per-slot input dispatcher).
   If FUN_008B6B20 isn't the dispatcher that handles a given submenu's
   cursor, production missed cursor moves there entirely.

### Port
Replaced the entire TkWindow iterator + cursor-hook block in
`src/menus/menus.js` with a line-by-line port of universal's PART 2b
(lines 1683-1762). Specifically:

- Dropped state: `tkLastActive`, `tkLastFullText`.
- Added state matching universal: `prevTkVisible`, `prevTkTextPtrs`,
  `prevTkCursors`, `initialTkTextPtrs`, `isTkPopup`.
- Dropped helpers: `announceTkCursorItem`, `getFullTkText`,
  `announceTkContent`. Their logic is now inline in the iterator.
- Added helper getTkFocusedItem using ctx, segments matching universal's
  same-name function.
- Replaced 0x4ABCE0 hook body with universal's exact loop:
  - Single gate: when (not ctx or ctx.vis equals 0) — no winPtr+0 check.
  - Three branches: vis 0→1 menu open, textPtr-shift popup, idx-change
    cursor move.
  - Speech adapts universal's two-utterance pattern (header + focused)
    into a single speak using 'header. focused', true so Tolk doesn't chop
    the second when interrupt=true on cursor moves.
- Removed the FUN_008B6B20 hook entirely. Cursor changes are now
  detected in the iterator loop, exactly like universal.

### Files modified
- `src/menus/menus.js` — full rewrite (351 → 250 lines), TkWindow
  iterator now matches universal_menu_battle_reader.js byte-for-byte
  in logic structure
- `Docs/gameArchitecture.md` — Items entries reset to "unconfirmed,
  silent in production" (confidence 0.0 → port shipped, awaiting test)
- `Docs/debug.md` — moved the false "Items submenu Solved" entry to
  Tried & Failed; added DO NOT retry without runtime evidence note
- `Docs/Scripts/Frida/FFX/universal_menu_battle_reader.js` +
  `_host.py` — restored from archive for use as live oracle during
  port validation
- `C:\Users\pmoen\.claude\projects\D--Games-Dev-Custom-FFX\memory\
  feedback_trust_user_observation.md` — new memory: when user reports
  X is silent, X is silent regardless of doc/agent claims to contrary

### Pending user testing
- Open field menu → Items → expect submenu name + first item read on
  open. Arrow up/down → each focused item reads. Confirm into inventory
  → expect popup-style read with header+focused.
- All other submenus (Abilities, Overdrive, Aeons, etc.) — SAME hook
  may now also catch some that were silent before. Equally, anything
  that worked through the dropped FUN_008B6B20 cursor hook may regress
  — universal didn't have that hook, but production's window types
  may be a superset universal didn't exercise. Watch for regressions.
- If Items still silent, the iterator is firing but Items' slot is one
  the body skips for an unknown reason — diagnose with a probe that
  dumps ctx for all 8 slots while Items is open.

KEYWORDS: Items submenu silent, exact port, universal_menu_battle_reader,
menus.js iterator rewrite, prevTkVisible prevTkTextPtrs prevTkCursors,
initialTkTextPtrs isTkPopup, getTkFocusedItem, removed winPtr.readU16
active gate, removed FUN_008B6B20 cursor hook, removed announceTkContent
getFullTkText announceTkCursorItem, segs[0] header segs[idx+1] focused,
tpChanged popup branch, line-by-line port lines 1683-1762


## Session 60 — 2026-05-06 — Dev-slug parsing + map-exit diagnostic pass (items 1+2 of effervescent-glacier plan)

### Goal
Execute the two ungated items from the high-confidence plan
(`C:\Users\pmoen\.claude\plans\we-need-to-discuss-effervescent-glacier.md`):

1. Dev-slug parsing — turn `chr_wakka_a`, `barrel_01`, etc. into player-facing
   names ("Wakka", "barrel") instead of `Object 0x<hex>` or raw slug.
2. Map exit completeness pass — surface destination-mapId resolution status
   in the worker diagnostic log, and clean up the speech format for exits
   that have no captured target ID.

Items 3–5 (help-menu dispatcher hook, status-screen review buffer, overdrive
auto-success) remain gated on probes/external addresses and were not touched.

### Item 1 — Dev-slug parsing
- New module `src/utils/labels.js` exports parseSlug using devString which
  returns `{kind, name}` for either `chr_<stem>[_*]` (kind='character',
  name from KNOWN_NAMES dictionary or capitalised stem) or known object
  prefixes `barrel|crate|door|lamp|chest|sign|box` (kind='object', name=
  the prefix word).
- KNOWN_NAMES verified against in-binary character-name tables:
  - Table 1 at RVA 0x748668 — Tidus using 0 / Auron using 2 / Wakka using 4 / Lulu using 5
  - Table 2 at RVA 0x756120 — `Tidus` / `Auron` / `Kimahri` / `Wakka`
  Both confirmed via strings_dump.txt:4542+ and :6756+.
- `src/navigation/index.js` label using e: tries parseSlug using e.name first;
  on hit, returns the friendly name directly; on miss, falls back to the
  prior `category + ': ' + e.name` and ultimately `category + ' 0x<hex>'`.
- Added per-call `logFile` lines (file-only) recording slug-hit vs.
  slug-miss with chrId, so coverage can be audited from production logs.
- Added an import of `parseSlug` from the labels utility to navigation/index.js.

### Item 2 — Map exit completeness pass
- Audit of the validity gate at `exits.js:285-303` showed it is already
  correct in current code: resolved name → `Exit to <name>`, only the
  unresolved branch produces `Exit (unverified) to 0x<hex>`. The plan's
  premise that "(unverified)" was always suffixed was based on older code.
- Added `tid=0x<hex> name=<resolved|<unresolved>>` to the `[WORKER ...]`
  diagnostic log line so we can see which exit destinations the bytecode
  scan is capturing AND whether they decode. Previously the log printed
  reqs/reqTargets/calls but never showed `exitTargetMapId`.
- Cleaned up the speech text for exits that have no captured target ID
  (e.g. workers 57/58/59 in current map: `class=EXIT reqTargets=[]`):
  `Exit ' + exitCount + ' (no target id)'` → `Exit ' + exitCount`.
  "(no target id)" was internal state and shouldn't reach speech.
- Decoder pair (`FUN_0046C3C0` + `FUN_002460E0`) untouched — already proven.

### Confidence
- parseSlug logic: **0.95** — pure string parsing, no game-state coupling.
- KNOWN_NAMES dictionary covers party + summons (verified against in-binary
  string tables): **0.9** — asset-side spellings ("kimari", "yojinbo")
  routed to display names; unknown stems fall through to capitalised stem.
- Exit log diagnostic addition: **0.95** — uses existing resolveMapName
  call (cached, side-effect-free outside the cache).
- Exit speech cleanup: **0.95** — drops user-facing internal state, no
  logic change.

### Files modified
- `src/utils/labels.js` (new) — 60 lines
- `src/navigation/index.js` — import + label() rewrite (~12 lines net)
- `src/navigation/exits.js` — log-line addition + speech cleanup (~10 lines net)

Build: `src/build/build.bat` ran clean. install/ffx_sr.js redeployed —
bundle confirms 7 occurrences of new symbols (parseSlug / KNOWN_NAMES /
tidLogStr).

### Pending user testing
- Walk a map where actor names load (chr_*) — expect "Wakka" instead of
  "NPC: chr_wakka_a"; expect "barrel" instead of "Object: barrel_01".
  Current dev map has all `name=null`, so the feature is silent there;
  not a regression — the prior behaviour was identical.
- Cycle through exits → existing format preserved; the noisy
  "(no target id)" parenthetical now drops to just "Exit N".
- Inspect production log after a session — `[LABEL] slug-hit:` /
  `slug-miss:` lines show coverage; `[WORKER ... tid=...]` shows
  exit-destination resolution status.

### Items 3 — Still gated this session
- Item 3 (help dispatcher) — gated on user running
  `Docs/Scripts/Frida/FFX/help_dispatcher_probe.js`. Probe exists; no
  output log yet in `Docs/Scripts/Frida/output/FFX/`. Awaiting user run.

### Items 4 + 5 — Unblocked mid-session by user-supplied CT files

User downloaded the Big CE Table for FFX HD Remaster from Nexus Mods
(mods/184) and pointed at `D:\Games\Dev\Custom\FFX\Cheat Engine Tables\`.
The CT is plain XML, parseable directly. Spot-checking the per-character
PlySave entries (Tidus through Rikku) yielded a complete struct map for
the 0x94-byte slot, cross-validated against the existing +0x3B Sphere
LVL field already used by `src/menus/config.js`. This is the same probe
data the plan called for, supplied by the published CT instead of a
discovery probe — no probe was authored or run.

PlySave layout (slot start = saveDataAddr + 0x55CC + chrIdx*0x94)
documented in detail in `Docs/gameArchitecture.md` String Tables section.
Key fields used this session:
  +0x18 u32 AP till next level
  +0x1C u32 Curr.HP, +0x24 u32 Max.HP
  +0x20 u32 Curr.MP, +0x28 u32 Max.MP
  +0x2D u8  Equipped weapon ID
  +0x2E u8  Equipped armor ID
  +0x2F..36 effective stats (Str, Def, Mag, MDef, Agi, Lck, Eva, Acc)
  +0x38..3A Overdrive (mode, current, max bars)
  +0x3B u8  Sphere Level

#### Item 5 — Overdrive auto-success (shipped)

- New module `src/battle/overdrive_force.js`. Single toggle (hotkey `O`)
  flips `forceEnabled`. When on, applyForceOverdrive using chrIdx writes
  current bar = max bar for the character whose Main battle window
  just opened (= start of their turn).
- Hooked into the existing window-open hook in `src/battle/index.js`
  (RVA 0x4A0D70) — when win.type equals 0x00 (Main window), call
  applyForceOverdrive using win.chrId before announcing the acting
  character. Per-character semantics: writing on each character's
  turn-start means a teammate's OD use doesn't reset our meter.
- Added `VK_O = 0x4F` to `src/core/input.js`.

#### Item 4 — Status review buffer (shipped)

- New module `src/menus/status.js`. Two hotkeys (`,` previous row,
  `.` next row) walk a 15-row buffer of {label, value} pairs covering
  every field shown on the Status screen as confirmed by user
  screenshot: Character name, HP curr/max, MP curr/max, Strength,
  Defense, Magic, Magic Defense, Overdrive (mode + bar), Weapon ID,
  Armor ID, Agility, Luck, Evasion, Accuracy, Sphere Level, AP to
  next level.
- Visibility gated by hooking Status fn_ctrl (RVA 0x4D1DD0). The
  hook reads param_1+0x1C (state) on `onLeave` — when state is the
  terminal 0x13 (close branch in the decompile switch), clear
  `statusVisible`; otherwise set it true. Hotkeys silently no-op when
  not visible.
- Buffer rebuilt automatically when the user cycles characters via the
  game's L/R (detected via `fieldMenuPlayerAddr` change between hotkey
  presses), so each press re-reads PlySave and reflects the current
  character.
- Overdrive mode names: 17-entry static table (Warrior, Comrade, Stoic,
  ... Loner) verified against CT entry IDs 149-165 (contiguous per-mode
  counter array immediately after the OD bar bytes).
- Equipment NOT resolved by name in v1 — buffer announces the byte ID.
  Calling `MsWeaponName` (RVA 0x3A0C70) from JS for an arbitrary slot
  ID requires param-decoding work not yet done; the function does
  resolve names but its 4-arg signature has not been characterised.
  Tracked as follow-up.
- Registered status.install in `src/menus/index.js`.

### Confidence
- PlySave struct map (CT-derived, cross-checked against existing +0x3B
  field): **0.95**
- Item 5 overdrive write semantics (current-bar = max-bar on each
  character's Main window open): **0.9** — correct per-turn refill;
  may need to verify the meter sticks through the turn (game might
  rewrite mid-turn). Test will confirm.
- Item 4 visibility gate (Status fn_ctrl state 0x13 = close): **0.85**
  read the fn_ctrl decompile in full; case 0x13 sets a "finished"
  flag and calls FUN_008aa0b0 using 1, 0, which is the standard menu
  teardown signature in adjacent menus.
- Status row order matching screen layout (HP/MP, primary 4 stats,
  OD/equipment, secondary 4 stats, footer): **0.9** — confirmed
  against user screenshot of the bottom half.

### Files modified this session
- `src/utils/labels.js` (new) — slug parser
- `src/navigation/index.js` — label() rewrite + parseSlug import
- `src/navigation/exits.js` — log diagnostic + speech cleanup
- `src/core/input.js` — added VK_O
- `src/battle/overdrive_force.js` (new) — OD force-ready toggle
- `src/battle/index.js` — wired in applyForceOverdrive + installOverdriveForce
- `src/menus/status.js` (new) — status review buffer
- `src/menus/index.js` — wired in status.install()
- `Docs/gameArchitecture.md` — character-name tables + PlySave map

Build: `src/build/build.bat` ran clean. install/ffx_sr.js redeployed —
deployed bundle confirms 10 occurrences of new symbols.

### Pending user testing
- Slug parsing (item 1): walk a map where actor names load.
- Map exit speech (item 2): cycle exits, verify "Exit N" replaces
  "Exit N (no target id)".
- Overdrive auto-success (item 5): enter battle, press `O` to toggle on,
  expect "Overdrive auto-success on" speech. Each character's first turn
  should have OD selectable. Disable mid-battle by pressing `O` again.
- Status review buffer (item 4): open Status from main menu, press `,`
  / `.` to walk all 15 rows. Cycle character with game's L/R, press
  `,` again — expect new character's data. Close menu — hotkeys go
  silent.

### Hotkey allocation summary (post-session)
- `\`` nav.scan
- `[` `]` nav.prev / nav.next (category if Shift+)
- `\` nav.pathfind
- P nav.announce
- T battle.whose-turn
- H battle.party-hp
- N menus.help
- S sgrid.sphere-avail
- **,** **.** status.prev-row / status.next-row (new)

### Item 5 — REIMPLEMENTED correctly later in same session

After the meter-fill revert (see "REVERTED" subsection below), user
clarified the actual feature: minigame outcome auto-success, not meter
fill. Two-agent decompile sweep + hand verification of three function
bodies in `layer2_game_logic.txt` produced character-and-mechanism-
confirmed handlers for Tidus, Auron, and Lulu. Wakka's Slots and aeons
deferred (multi-outcome / no-handler-found respectively).

**New module:** `src/battle/od_success.js`. Hooks each handler's onEnter,
checks the per-character state register, and when `forceEnabled` is on
AND state == 4 (input listening), writes the success values directly:

| Character | Handler RVA | Write on force-success |
|-----------|-------------|------------------------|
| Tidus Swordplay | `0x492290` | address 0xF3D6B3 receives 1 (flag), address 0xF3D6F4 receives 5 (skip to success state) |
| Auron Bushido   | `0x490EE0` | address 0xF3D6B2 receives 1 (flag), address 0xF3D6B4 receives 5 (skip to success state) |
| Lulu Fury       | `0x491AF0` | address 0xF3D6DF receives 0x10 (max counter), address 0xF3D6C8 receives 6 (skip to time-up) |

Hotkey **O** toggles. Default OFF. Speech: "Overdrive auto-success on,
covering Tidus, Auron, Lulu" / "Overdrive auto-success off". Wired into
`src/battle/index.js` install().

**Decompile evidence:**
- Tidus handler at decompile lines 528714-528795, case 4 contains the
  bar-window comparator the timing value DAT_0133D702 lies inclusively between DAT_0133D6FC and DAT_0133D6FE, after which DAT_0133D6F4 is set to five. Final
  dispatch line 528789: FUN_007b0480 using 0, ... (chrIdx=0 = Tidus).
- Auron handler at lines 527797-527866, case 4 contains the
  button-sequence matcher DAT_025D09D4 matches the signed 16-bit sequence entry at DAT_0133D6C4 plus twice DAT_0133D6BC; a match advances DAT_0133D6BC by one. Final
  dispatch line 527860: FUN_007b0480 using 2, ... (chrIdx=2 = Auron).
- Lulu handler at lines 528395-528550, case 4 reads analog stick via
  `FUN_00888B90`, tracks direction quadrants, increments counter on
  full rotation. Final dispatch line 528547: FUN_007b0480 using 5, DAT_0133D6DF, ... (chrIdx=5 = Lulu, counter as result).

**Confidence:**
- Tidus mapping (0x492290 / DAT_0133D6B3): **0.95** — bar-window
  comparator is unmistakably the meter-timing mechanism.
- Auron mapping (0x490EE0 / DAT_0133D6B2): **0.95** — chrIdx=2 in
  final dispatch matches Auron, "bauro" string anchor matches Bushido
  source-file naming (NOT Aurochs/Wakka — initial agent labels were
  wrong).
- Lulu mapping (0x491AF0 / DAT_0133D6DF): **0.9** — direction-quadrant
  detection + counter pattern matches the joystick-rotation Fury
  mechanism.

**Confirmed dead code (DO NOT hook):**
- `FUN_007B1B60` at RVA `0x3B1B60` ("Virtuos FIXME … what are these
  values used for??"). Zero callers across all 5 decompile layers,
  Wakka-only branch only logs and falls through. Documented in
  `gameArchitecture.md`.

**Deferred:**
- **Wakka's Slots** — multi-outcome (Element Reels match-3-fire ≠
  match-3-ice; Status Reels different status per match). Single
  binary success-flag write insufficient; needs freeze + speak-list
  + user-pick + resume UX. Handler RVA also still unidentified in
  decompile. Tracked as future investigation.
- **Yuna aeon ODs** — per user, no special input minigame mechanics;
  no handler needed.

### Item 5 — original meter-fill misimplementation, REVERTED

User correction: "overdrive auto-success" is not the meter (which the
plan and CT both pointed at), it's the in-overdrive minigame outcome.
Three different mechanisms — slot reels (Wakka, some Yuna aeons),
mash-X count (Tidus's Swordplay), timing-window press (Auron's
Bushido) — that play AFTER the user selects the OD command. Forcing
the meter doesn't auto-succeed any of them.

Production log confirmed the meter-fill code ran but was a no-op for
the user's test session — every character whose Main window opened
during the toggle-on window already had a max meter, so the
the current value had reached the maximum and an early return prevented a write. Even if
writes had happened, they wouldn't have helped — the user was failing
the slot-reel minigame, which is unrelated.

Per user direction, removed the misnamed feature entirely:
- Deleted `src/battle/overdrive_force.js`
- Reverted `src/battle/index.js` (no `applyForceOverdrive` call,
  no installOverdriveForce)
- Reverted `src/core/input.js` (removed `VK_O`)

True overdrive minigame auto-success requires Frida discovery probes
to find the slot-result function, mash counter, and timing-window
check — multi-session RE work, NOT addressable from the CT alone.
Tracked as future work; user did NOT ask to start that investigation
this session.

Hotkey allocation reverts: `O` is unused again. The `,` `.` status
hotkeys remain in place.

Files this session AFTER revert: same as the listed set, MINUS
`src/battle/overdrive_force.js`, MINUS the changes in
`src/battle/index.js` (now back to pre-session state for that file's
window-open hook), MINUS the `VK_O` line in `src/core/input.js`.

KEYWORDS: dev slug parsing parseSlug labels.js, KNOWN_NAMES party aeons
chr_ prefix barrel crate door lamp chest sign box, character name table
0x748668 0x756120 strings_dump verified, map exit completeness validity
gate exits.js, exitTargetMapId diagnostic log, Exit unverified resolved
mapName, no target id speech cleanup, effervescent glacier plan items
1 2 3 4 5, big CE table FFX HD Remaster Nexus mods 184, PlySave struct
0x94 stride saveData+0x55CC, overdrive bar 0x39 0x3A current max,
overdrive mode 0x38 17 modes Warrior Comrade Stoic Healer Tactition
Victim Dancer Avenger Slayer Hero Rook Victor Coward Ally Sufferer
Daredevil Loner, status review buffer 15 rows comma period hotkeys,
fieldMenuPlayerAddr 0x147150C selected character, status fn_ctrl
0x4D1DD0 state 0x13 close, overdrive_force.js status.js new modules,
items 4 5 unblocked by CT no probe needed, equipment names byte ID
unresolved followup MsWeaponName 0x3A0C70

## Session 61 — 2026-05-08 — Help-submenu spam silenced via sceneId gate

### Context

User asked for an audit of viability-summary "Achievable" items vs what's
already in `src/`. Audit result:

- **Done:** dev-slug parsing (`src/utils/labels.js`), status review buffer
  (`src/menus/status.js`), map-exit completeness (`src/navigation/exits.js`
  via engine `fnMapNameEncoded` + `fnDecodeFhText`), overdrive auto-success
  (`src/battle/od_success.js`, Tidus/Auron/Lulu), dialogue coverage
  (`src/menus/dialog.js`).
- **Not done:** help-menu spam fix, pathfinding dead-zone investigation
  (latter is diagnostic, not feature code).

Re-checked `Docs/Scripts/Frida/output/FFX/field_submenu_probe.log` —
exists, has data, but tab cursors only (Aeons state 6, Equip state 10,
Overdrive Set state 9 briefly); item-row cursors all NaN because the
trace didn't capture in-list arrowing. No `help_dispatcher_probe.log`
exists — that probe was authored Session 58 but never ran.

### Decision

User explicitly said: skip Frida for help menu. If we can't ship a real
read, comment out the spammy path and call Help submenu a no-op. Don't
chase more probes.

### Change

`src/menus/dialog.js` — added an import of `state` from the state utility.
and gated the `FUN_00776A50` (RVA `0x376A50`) `onLeave` body on
state.sceneId differs from 0x0B. Help is sceneId `0x0B` per
`gameArchitecture.md` and confirmed by
`field_submenu_probe.log` line 177 (`[ACTIVATE] sceneId=0xb (Help)`).
When the user is in the field-menu Help submenu, the per-frame
event-message decoder produces zero speech and zero log lines from this
path. Cutscenes and other event-message lists use different sceneIds and
remain unaffected.

The viability item "Help menu spam fix" is now closed as "no-op" rather
than waiting on a probe-driven dispatcher swap. If a future session
runs `help_dispatcher_probe.js` and `FUN_00776220` turns out to be
event-driven, the gate can be lifted and replaced with the proper
event-driven hook. For now Help submenu is silent by design.

### Files modified

- `src/menus/dialog.js` — added state import + Help-scene gate (4 lines
  net added: 1 import, 1 const, 1 comment block, 1 guard).

Build deployed via `src/build/build.bat`; ffx_sr.js and proxy DLL pushed
to game directory.

### Pending user testing

- Open the field-menu Help submenu (Pause → Help). Expect total silence
  while the submenu is active. No idle spam, no per-row spam.
- NPC dialogue and cutscene subtitles unchanged — sceneId is not 0x0B in
  those contexts.

KEYWORDS: help submenu spam silence sceneId 0x0B gate, dialog.js
FUN_00776A50 0x376A50 event-message decoder, state.sceneId guard,
viability achievable audit done items 5 of 7, field_submenu_probe.log
partial data tab cursors only NaN item rows, help_dispatcher_probe never
ran, no-op help submenu deferred dispatcher swap, scene 0xb confirmed
field_submenu_probe activate, cutscene subtitles unaffected different
sceneId, customer_capability_summary FFX_Viability_summary audit

## Session 62 — 2026-05-08 — OD auto-success default ON + Wakka slots re-confirmed unfindable

### od_success default OFF was a shipping bug

User reported "none of the overdrive successes work." Mod log evidence:
zero `[OD-SUCCESS]` lines, zero `Toggle -> ON/OFF` entries — confirming the
toggle was never engaged, the per-character force functions hit
when (not forceEnabled) return on every call and silently no-op'd. User had
used Auron's Banishing Blade and Tidus's Blitz Ace in the session
([SPEECH] log lines 451 and 651) and got mediocre damage outcomes
consistent with a missed minigame.

Fix in `src/battle/od_success.js`:
1. Changed the initial force-success setting from disabled to enabled. A blind player has no way
   to play the OD minigames manually — default-OFF was wrong from day
   one. The `O` hotkey still toggles off if the user ever wants the native
   minigame.
2. Added per-character state-transition logging
   (`[OD-DIAG] <character> state X -> Y`). Logs only on change so the
   per-frame fn_ctrl call rate doesn't spam, but gives evidence the hook
   fires and shows the actual state lifecycle. Prior code had no
   diagnostic that fired below the success branch, leaving us blind to
   "is the hook even attached?" type questions.

User confirmed working post-rebuild: "they worked this time."

### Wakka slots: re-walked, still unfindable from static analysis

User asked to build Wakka auto-success. Followed both string anchors
(`MsSetReelResult` FIXME at 0x00b54fc0, `# REEL%d` at 0x00b5504c) — both
appear only inside `FUN_007B1B60` (RVA 0x3B1B60), which session 60
already classified as zero-caller dead code. Re-confirmed.

Auto-label has only two `bwakk` matches:
- `00897410 | bwakk` — `FUN_00897410` (242 bytes), render setup with
  FUN_008ae4e0 using 2, "bwakk" plus 3D draw calls. Not a state machine.
- `008fdf20 | bwakk` — `FUN_008fdf20` (283 bytes), per-element draw
  helper. Not a state machine.

Searched the OD dispatcher `FUN_0089ad90` (line 533676 of layer2): calls
Auron/Tidus/Lulu fn_ctrls explicitly plus several other functions in the
0x89xxxx range (FUN_008910b0, FUN_00892040, FUN_00891190, FUN_008919a0,
FUN_00891a30, FUN_00891250, FUN_008913f0, FUN_008914f0). Spawned an
Explore agent to read each candidate in full; none has the
switch-on-state-register fn_ctrl shape used by the other three.

Negative evidence (Explore agent, confidence 0.95):
- No fourth `DAT_0133d6XX` state register exists.
- No FUN_008af160 using 2, "bwakk" call anywhere in the decompile.
- No FUN_007b0480 using 4, ... call (chrIdx 4 = Wakka) anywhere.
- Wakka is not in any state-machine dispatcher that includes the other
  three.

Conclusion: Wakka's slots fn_ctrl is **not statically findable**. The
existing `gameArchitecture.md:1938` claim ("Handler RVA also not yet
identified in the decompile") is upgraded to "decompile-exhausted —
requires runtime probe to locate" and the deferred status stands. Path
forward (NOT executed): Stalker trace during a Wakka OD use, then
hand-label the fn_ctrl and reel-symbol memory from the trace. Plus a
freeze + speak-list + hotkey-pick + write + resume UX layer for outcome
selection. Multi-session work; not in scope right now.

### Files modified

- `src/battle/od_success.js` — default ON + state-transition diagnostic
  logging. ~30 lines net added across three helper functions plus a
  comment block.

Build deployed via `src/build/build.bat`.

### Pending user testing

- Confirm `[OD-DIAG]` lines appear in next session's mod log (evidence
  the diagnostic is wired correctly).
- No Wakka testing — feature explicitly deferred.

KEYWORDS: overdrive auto-success default ON forceEnabled flipped,
od_success.js Tidus Auron Lulu state diagnostic OD-DIAG, hotkey O toggle
unchanged, Wakka slots fn_ctrl decompile-exhausted runtime probe
required, FUN_007B1B60 dead code re-confirmed, bwakk auto-label only
0x497410 render 0x4FDF20 draw not state machines, FUN_0089ad90 OD
dispatcher no Wakka entry, MsSetReelResult REEL%d strings dead anchors
only, no FUN_008af160 bwakk no FUN_007b0480 chrIdx 4 anywhere,
gameArchitecture 1938 deferred status stands, Stalker trace required
for Wakka future session

## Session 63 — 2026-05-08 — Exit resolver FINALLY wired + entity-count format

### Two user complaints reopened

1. Entity announce reads "1/3: Exit 3, …" — count BEFORE name; user wants
   count at the END of the line, not the first thing.
2. Exit resolution doesn't work in production — exits announce as bare
   "Exit 1/2/3" with no destination map name. User pointed out the map
   resolver was found earlier but never linked to live exits.

### Investigation — what actually went wrong

The existing `exits.js` was wrong on three independent counts:

- **Bogus `EXIT_FN_RVAS` list.** The discovery code in findExitAtelIds
  walked the Atel namespace tables and matched function pointers against a
  hand-curated list of 9 RVAs (0x4580C0, 0x458220, etc.). Reading
  `FUN_004580c0` in the decompile confirms it's an *allocator* (calls
  FUN_0042fc60 using param_1 times 8, 4 and writes via `FUN_004591c0`), not a map-
  transition function. The "exits" the production scanner thought it had
  were a long-running false positive. In the user's current map this
  caused workers 57/58/59 (LINE/BOX triggers) to be tagged `class=EXIT`
  via a coincidental `CALLPOPA 0x11` in their bytecode — `0x11` is just
  Common namespace function 17, completely unrelated to map jumps.
- **PUSHI vs PUSHII confusion.** The bytecode walker only tracked opcode
  `0xAD` (`PUSHI`, whose operand is an INDEX into the chunk's int reference
  table). Real map-transition calls use `0xAE` (`PUSHII`, whose operand is
  the literal sign-extended 16-bit value). So even when the walker DID
  reach a real exit call, the destination capture missed it: `tid=null`.
- **Per-trigger entry-point flow doesn't reach exit calls.** Trigger
  workers (type 2/3/5/6 = LINE/BOX) register callbacks via the engine's
  zone-detection layer; their own entry-point bytecode is short (35 steps
  total across 8 entries for workers 57/58/59 in the test log). The
  actual `CALLPOPA 0x8002` lives elsewhere in the same chunk and is only
  invoked by the engine when the player crosses the trigger zone. A
  scanner walking forward from the worker's entries will never reach it.

### What was already in the probe outputs

`Docs/Scripts/Frida/output/FFX/nav_exit_decode.log` — full chunk decode
with proper opcode names — already contained the exact pattern, dozens of
times:

The retained decode shows a room literal followed by an entrance literal and
then `CALLPOPA map:2 (0x8002)` at chunk offsets `+02965`, `+02968` and `+0296b`.
This is historical disassembly output; the body text records its interpretation
and retained source location.

`map:2` is `0x8002` = `AtelSetEventJump2`, the universal map-transition
Atel function. Two PUSHII literals immediately precede every call site:
destination map ID, then entrance ID. Room 140's chunk has 45 such call
sites, six distinct destinations (10/11/12/13/14/21). The resolver
(`FUN_0086c3c0` at RVA 0x46C3C0 + `FUN_006460e0` at 0x2460E0, already
integrated as `resolveMapName`) decodes those IDs to area-name strings.

### Production rewrite — `src/navigation/exits.js`

Dropped:
- `EXIT_FN_RVAS` import (the list itself stays in addresses.js for now,
  unreferenced — can be deleted later)
- findExitAtelIds and the `ATEL_NAMESPACES` walk
- `exitAtelIds` / per-namespace pointer scan
- The `0xAD` (PUSHI indexed) tracking — exits don't use it

Added:
- scanChunkExits using chunkPtr — linearly scans the entire chunk's bytecode
  for the exact pattern `0xAE OP1 OP2 | 0xAE OP3 OP4 | (0xB5|0xD8) 0x02 0x80`.
  Cached per chunk pointer string. Returns `{exits: [{offset, dest, entrance}],
  dests: [unique ids]}`.
- workerHasReq using worker — light flow-walker kept for the Event vs Ambient
  distinction on non-exit triggers. Same algorithm as before, minus the
  bogus EXIT_FN_RVAS path.
- New classification: trigger workers (type 2/3/5/6) are EXITS iff their
  chunk has any `CALLPOPA/CALL 0x8002` call. Otherwise EVENT (if REQ-
  family present) or AMBIENT (drop).

### Per-trigger correlation — explicit non-goal

Static analysis cannot tell which specific trigger leads to which specific
destination. The engine dispatches via runtime callback registration, and
runtime-discovery solutions (hooking `AtelSetEventJump2` and learning by
the player crossing exits) are forbidden by the project rules: the user
is blind and cannot bump around. So `scanExits` instead returns the
chunk's resolved destination NAMES as an aggregate. The scan announcement
appends "Exits to A, B, C" — gives the user real info about where this
map's exits lead, no fabrication, even if they don't yet know which
trigger is which.

### Files modified

- `src/navigation/exits.js` — full rewrite (~270 lines, was 343).
  No longer imports `ATEL_NAMESPACES`/`EXIT_FN_RVAS`/`base`.
- `src/navigation/index.js`:
  - `announceSelected` format flipped: `<name>, <steps> steps <dir>, N of M`
    (count moved to the end per user request).
  - `navAggregateDests` state added; populated from `scanExits`'s return
    value; cleared on JUMP2.
  - `hotkeyScan` appends "Exits to A, B, C" to the post-scan summary on
    All/Exit category views.

Build: `src/build/build.bat` clean. ffx_sr.js redeployed.

### Confidence

- Universal exit detector (CALLPOPA/CALL 0x8002 + PUSHII pair pattern):
  **0.97**. Pattern verified directly in `nav_exit_decode.log` lines
  1192-1194, 3803-3805, 3914-3916, 4013-4018, 4069-4071, 4170-4172, etc.
  Same pattern appears 45 times in Room 140's chunk with consistent
  semantics.
- Map-name resolver: **0.95** (unchanged from session 41 — already proven).
- Type-based trigger classification (2/3/5/6 = LINE/BOX): **0.9** —
  matches `nav_integrated.log` which used the same filter and the user
  confirmed at the time.
- Aggregate-only destination announcement (no per-trigger correlation):
  **1.0** — the static unsolvability is the answer, not a workaround;
  silence per-trigger is correct because fabricated mappings would
  mislead.

### Pending user testing

- On any field map with exits, press scan. Expect: total entity count,
  then "Exits to <area name 1>, <area name 2>, …" if the chunk has any.
- Cycle exits → format is now "Exit 1, X.X steps <dir>, 1 of N".
- Nav log line `[NAV] Chunk 0x… : N exit calls, M unique destinations: …`
  appears once per unique chunk, with resolved area names in the list.

KEYWORDS: exit resolver wired CALLPOPA 0x8002 PUSHII pair, map:2
AtelSetEventJump2, scanChunkExits chunk-keyed cache, EXIT_FN_RVAS bogus
allocator FUN_004580c0 false positive, workers 57 58 59 LINE BOX trigger
type 2 3 5 6 classification, aggregate destination list scan summary,
per-trigger correlation static-unsolvable runtime callback dispatch,
entity announce format count moved to end navSelectedIdx 1 of N,
navAggregateDests cleared on JUMP2, exits.js rewrite drop ATEL_NAMESPACES
findExitAtelIds exitAtelIds, PUSHI 0xAD indexed PUSHII 0xAE literal
sign-extended 16-bit, decompile FUN_0042fc60 allocator not exit,
nav_exit_decode.log 45 exit calls Room 140 destinations 10 11 12 13 14 21

## Session 64 — 2026-05-08 — Exit classifier strict per-worker walk

### What broke after Session 63

User reported: Thunder Plains showed 22 exits (way too many) and exits
still spoke as "Exit N" rather than the destination map name.

Root cause: Session 63's classifier was "any trigger in a chunk that
contains any `CALLPOPA 0x8002` → EXIT, with chunk-aggregate destinations
announced once at scan time." Big maps (Thunder Plains, Calm Lands)
have plot-state-dependent exit logic with dozens of static `0x8002`
calls in the shared script chunk — so every LINE/BOX trigger in the
map (lightning towers, fiend battle triggers, scripted-event boundaries,
etc.) got the EXIT label. Per-trigger destinations were never derived,
so the announce path stayed at "Exit N".

### Fix in this session — `src/navigation/exits.js` rewrite

Switched to **strict per-worker classification**:

- Walker walks the worker's own bytecode entries. Each unique entry
  offset gets its own `visited[]` (de-dup is intra-entry only) — the
  prior shared-`visited[]` was the reason the production log showed
  `steps=35` across 8 entries: entry 0 walked ~30 bytes, entries 1–7
  immediately landed on visited bytes and broke. With per-entry
  isolation, each entry walks its full reachable subgraph. Total step
  cap stays at 200000 to bound CPU.
- Walker tracks `PUSHII` (0xAE — literal sign-extended 16-bit operand)
  via `lastPushii1`/`lastPushii2`. The prior code tracked `PUSHI`
  (0xAD — index into the chunk's int reference table); destinations
  always use PUSHII so capture was always returning null.
- Classification: a worker is EXIT iff its walk reaches
  `CALL/CALLPOPA 0x8002`. If the two preceding 3-byte instructions are
  both PUSHII, the second-most-recent literal becomes the destination
  map ID and is run through `resolveMapName` for the speech label
  ("Exit to Mi'ihen Highroad"). If the walk reaches 0x8002 but the
  destination isn't a literal pair (computed via PUSHI/PUSHA/registers/
  stack ops), we keep the EXIT classification but fall back to the
  positional label "Exit N" — silence beats fabricated map names.
- Workers reaching neither 0x8002 nor REQ are AMBIENT and skipped —
  no entity pushed. Prior code over-classified these.

### Concrete behavioral changes vs. Session 63

| Scenario | Session 63 behaviour | Session 64 behaviour |
|----------|----------------------|----------------------|
| Trigger walk reaches 0x8002 with literal PUSHII pair | "Exit N" + aggregate sentence "Exits to A, B, C" | "Exit to A, X.X steps north, N of M" |
| Trigger walk reaches 0x8002, dest computed dynamically | "Exit N" + aggregate sentence | "Exit N" (positional only) |
| Big-map trigger that doesn't reach 0x8002 in its own walk (Thunder Plains state-dependent triggers) | Falsely classified EXIT (chunk-wide test) | NOT an exit — classified EVENT (if hasReq) or skipped (AMBIENT) |
| Trigger with REQ but no exit | EVENT | EVENT (unchanged) |

### Trade-off (acknowledged)

Triggers whose exit destinations live behind callback registration
(engine-side trigger-zone dispatch) won't be classified as exits. The
ones that do get classified will be a strict subset of the real exits
on a given map. Better than 22 false positives — and per the project
rule on fallback correctness, silence on undecidable cases is the
right answer.

### Files modified

- `src/navigation/exits.js` — rewrite (~245 lines).
  Removed: `scanChunkExits` chunk-cache, aggregate destination return.
  Added: per-entry isolated `visited[]` walker, PUSHII tracking,
  destination capture preceding 0x8002.
- `src/navigation/index.js` — reverted Session 63's `navAggregateDests`
  state and the `hotkeyScan` summary append. `scanExits` no longer
  returns anything.

Build: `src/build/build.bat` clean. ffx_sr.js redeployed.

### Confidence

- Per-entry isolated visited[]: **0.92** — the prior shared-`visited[]`
  is provably the cause of `steps=35` per worker across 8 entries (entry
  N+1 lands on entry N's bytes immediately). With isolation, walks are
  bounded only by the per-worker step cap.
- PUSHII tracking + 0x8002 detection: **0.97** — pattern verified
  multiple ways in `nav_exit_decode.log`.
- Strict classification (walk-reaches-0x8002 → EXIT, otherwise not):
  **0.85** — handles the simple-script case correctly. Won't catch
  callback-registered exits but won't false-positive either. Real-map
  testing pending.

### Pending user testing

- Thunder Plains: count of exits should drop dramatically from 22 to
  the actual real exits (a few). Real exits should announce
  "Exit to <name>" with a recognisable destination.
- Any map with a single canonical exit (e.g. Besaid Beach northbound
  to Besaid Village): should announce "Exit to Besaid Village".
- Maps where the trigger-handler is registered via callback
  (worker walk doesn't reach 0x8002): exit count drops to 0 in the
  entity list. This is a known regression — surfacing those exits
  needs further RE work (probe-driven, not in scope this session).

KEYWORDS: exit classifier strict per-worker walk, per-entry isolated
visited array, PUSHII 0xAE literal tracking lastPushii1 lastPushii2,
CALL CALLPOPA 0x8002 detection, Thunder Plains 22 false positives fix,
chunk-wide aggregate dropped, dest capture preceding PUSHII pair,
AMBIENT triggers skipped no entity, EVENT REQ unchanged, callback-
registered exits not detected known limitation, fallback correctness
silence over fabrication, 200000 step cap unique entry dedup

## Session 65 — 2026-05-08 — Reverted exits.js, added additive sole-dest fallback

### What happened in 63/64

Two consecutive rewrites of the classifier broke a working feature.
Session 63 over-classified (any trigger in a chunk with any 0x8002 →
exit, 22 false positives in Thunder Plains). Session 64 over-corrected
to strict per-worker bytecode walk, dropped 100% of exits in the same
map. Both sessions changed CLASSIFICATION when the actual gap was
DESTINATION RESOLUTION on already-correctly-classified exits.

### Action this session

1. **`git checkout HEAD -- src/navigation/exits.js`** — restored the
   committed working classifier (EXIT_FN_RVAS namespace match +
   workerScanBytecode flow walk). Same workers classify as exits as in
   the shipping 0.1.2 build.
2. **Added strictly-additive getChunkSoleDest using chunkPtr** — caches
   per-chunk linear scan for the literal pattern
   `0xAE OP1 OP2 | 0xAE OP3 OP4 | (0xB5|0xD8) 0x02 0x80`. Returns the
   single unique destination map ID if the chunk encodes exactly one,
   else null. Bails out as soon as it sees a second distinct
   destination.
3. **Wired the fallback in the existing `scan.isExit` branch** ONLY in
   the tid equals null path. Never overrides a classifier-captured tid;
   never alters classification; never adds or removes an exit. If the
   chunk has 0 or multiple unique destinations, the worker stays at
   "Exit N (no target id)" — same as before.

### Behavior matrix

| Pre-existing classifier outcome | New behavior |
|---------------------------------|--------------|
| isExit=true, tid=valid | Unchanged. "Exit to <resolved name>" if resolveMapName succeeds, else "Exit (unverified) to 0x...". |
| isExit=true, tid=null, chunk has exactly one static dest | NEW: "Exit to <resolved name>" using the chunk's sole destination. |
| isExit=true, tid=null, chunk has 0 or >1 static dests | Unchanged. "Exit N (no target id)". |
| isExit=false | Unchanged. (Event/Ambient handling untouched.) |

### Confidence

- Classifier preservation (git restore): **1.0** — exact bytes from HEAD.
- Sole-dest fallback semantics: **0.9** — pattern proven in
  nav_exit_decode.log; "exactly one unique destination" is a strict
  filter that mathematically can't pick a wrong one when satisfied.
- Resolver call: **0.95** — unchanged, already proven.

### Files modified

- `src/navigation/exits.js` — restored to HEAD, then added
  `chunkDestCache` + getChunkSoleDest and a 5-line fallback in the
  when (scan.isExit) branch (~70 lines added net).
- `Docs/session51_current.md` — this entry.

Build: `src/build/build.bat` clean. ffx_sr.js redeployed.

### Pending user testing

- Maps that previously announced "Exit N (no target id)": if the chunk
  encodes a single static destination, expect "Exit to <area name>".
- Maps with multi-destination chunks (e.g. Thunder Plains): exits stay
  at "Exit N" labels — no false destinations attached. Same count of
  exits as before.

KEYWORDS: exits.js git restore HEAD committed classifier, additive
sole-dest fallback getChunkSoleDest chunkDestCache, PUSHII pair pattern
0xAE 0xAE 0xD8 02 80 or 0xB5, exactly one unique destination filter,
classification untouched no exits added removed, Thunder Plains multi-
dest chunk safe stays Exit N, sessions 63 64 over-correction reverted,
EXIT_FN_RVAS classifier preserved despite imperfect, dest only filled
never overridden never removed

## Session 66 — 2026-05-08 — Removed dead chunk-sole-dest fallback + verbose suffix

User test result for session 65: every exit announced as "Exit N (no
target id)". The strict-singleton filter on `getChunkSoleDest` produced
no hits — almost all real chunks encode multiple distinct destinations
(plot-state-dependent transitions back and forth, scripted boundaries,
etc.), so the singleton requirement is virtually never satisfied. The
fallback was effectively dead code.

User direction: drop the verbose " (no target id)" string, fall back to
silence on the destination side. They also pointed out that the static-
chunk-scan path is structurally dead — either FFX doesn't allow
resolution this way at all, or only maps with stable single-destination
scripts (rare) would ever resolve.

### Changes

- Removed `chunkDestCache` and `getChunkSoleDest` (~70 lines).
- Removed the side-channel fallback wiring inside the `scan.isExit`
  branch.
- Changed the no-target-id label from `'Exit ' + exitCount + ' (no target id)'`
  to `'Exit ' + exitCount`. Comment in code records why we don't have a
  destination resolver path.

`exits.js` is now back to ~327 lines and is functionally identical to
the committed HEAD version EXCEPT the verbose suffix is gone. The map-
name resolver (`resolveMapName` / `fnMapNameEncoded` / `fnDecodeFhText`)
is retained — when the classifier DOES capture a tid, it still resolves
to a real area name. We just don't have a way to feed it a tid for the
common multi-dest-chunk case.

### What this means structurally

Per-trigger destination resolution is **not solvable by static chunk
analysis** with the current code. Possible paths forward (none in scope
this session):
1. Look at per-worker instance data (worker+0x48 int regs, +0x58 float
   regs, or other struct fields) — the engine may store the destination
   at trigger init time.
2. Hook the engine's trigger-zone callback dispatcher to learn the
   (worker → destination) mapping at activation time. Forbidden by the
   blind-player rule (runtime discovery).
3. Find a per-trigger configuration table in the chunk's data section
   (entrance table at chunk+0x04 / 0x20 etc.) that holds destinations
   keyed by trigger.

Build deployed. exits.js back to a clean known-good state with no
fabricated destination claims and no dead fallback code.

KEYWORDS: dead code removed chunkDestCache getChunkSoleDest, verbose
no target id suffix dropped, Exit N positional only no destination,
multi-dest chunks dominant case static singleton filter dead path,
resolveMapName retained for tid-capturing classifier path, future RE
options worker instance data trigger callback dispatcher entrance
table, blind-player runtime-discovery rule


## Session 67 — 2026-05-08 — Ingested Karifean's FFXDataParser as ATEL annotation layer; corrected 0x8002 misattribution

User pointed at Fahrenheit's README and Karifean's FFXDataParser as a
data source for ATEL call-target names. Goal: get every script-level
ATEL function ID named, then use that to attack stuck features (out-of-
battle / field menus the most likely beneficiary, per user's hypothesis).
Exits work paused on user direction and reframed as "deferred fix"
pending the data ingest below.

### What changed

- Cloned `https://github.com/Karifean/FFXDataParser` into
  `D:/Games/Dev/Custom/FFX/FFXDataParser/` (parallel to fahrenheit/, NOT
  in the mod repo). License not stated; treated as research-only data
  extraction, no source redistribution.
- Wrote `Docs/Scripts/Python/extract_atel_names.py` — joins Karifean's
  `ScriptCallTargetLib.java` (1182 named call targets across 11
  namespaces) with Fahrenheit's `core/fh/Atel/call.cs` (1516 IDs with
  hand-curated arg counts).
- Generated TSVs at `Docs/Scripts/Ghidra/output/FFX/atel/`:
  - `atel_namespaces.tsv` — 16 rows (11 known + 5 unpopulated).
  - `atel_calls_<nsId>_<short>.tsv` — one per known namespace.
  - `atel_opcodes.tsv` — 123 opcodes joined.
  - `atel_arg_count_disagreements.tsv` — 31 entries; all are Karifean-
    named functions he annotates as "Never used"/"Doesn't do anything",
    where Fahrenheit's hand-curated table reports 0 args because no
    live usage was observed. For ATEL bytecode disassembly, prefer
    Karifean's count (reflects native handler stack consumption).
  - `README.md` — provenance + format + refresh instructions.

### The big correction (gameArchitecture.md)

Session 63's "Universal Map-Transition Atel Function" entry claimed
`0x8002 = AtelSetEventJump2` based on 45 hits in Room 140's chunk with
6 distinct first-literal values (10/11/12/13/14/21). Per FFXDataParser:

- `0x8002` is map namespace entry 0x002, identified as `setGfxActive`; it takes
  a graphics index and an active-state value.
  — a graphics-effect toggle, NOT a room transition.
- The real warp primitives are Common-namespace:
  - `0x0011` is the room-transition entry, with room and entrance arguments.
  - `0x010B` is the warp-to-room entry, also with room and entrance arguments.

45 graphics-effect calls per map is typical (lights, particles, water,
fog); 45 distinct exits in one room is not. The 6 distinct first-literal
values were `gfxIndex` values that happened to overlap with neighbour-
room IDs by coincidence. Session 63 also dismissed `CALLPOPA 0x11` as an
"incidental collision" — that was exactly backwards; `0x11 / 0x0011` IS
the real warp call, the one Karifean labels `transitionToRoom?`.

The native dispatcher `AtelSetEventJump2 @ RVA 0x46FED0` IS real
(Fahrenheit's `core/runtime/src/events/game_loop.cs` hooks it for the
title-screen warp event with signature void using room, entrance, do_fade); session 63's confusion was conflating the Common-namespace
ATEL primitive with the Map-namespace call-target table entry.

### Files changed

- `Docs/gameArchitecture.md`:
  - Expanded `MapEntrance` (0x20 bytes, full Fahrenheit layout).
  - New `AtelScriptChunk` (0x38 bytes).
  - New `AtelScriptHeader`.
  - Replaced "Universal Map-Transition Atel Function" with corrected
    "Map-Transition Atel Calls" section. Old finding kept as a
    "Superseded session-63 finding (was incorrect)" subsection so the
    history isn't lost.
  - New "ATEL Call Target Namespace Tables" — 11 namespace bases with
    runtime VAs.
  - New "Other transition-related natives" — `TkSetEventJump`,
    `TkSetEventJumpMapNo`, etc.
- `Docs/debug.md`: new Known Issue entry recording the deferred exits
  fix (switch `exits.js` from `0x8002` to `0x0011`/`0x010B`).
- `Docs/Scripts/Ghidra/output/FFX/atel/` — new annotation layer
  (TSVs + README).
- `Docs/Scripts/Python/extract_atel_names.py` — new extractor.

### What this unlocks

The 1182 named ATEL call targets are now grep-able. Future RE on ANY
ATEL-driven feature can resolve the call IDs in the bytecode to real
names. Phase 5 of the plan (`silly-moseying-sifakis.md`) will run a
field-menu investigation against the annotated decompile to test the
hypothesis that out-of-battle menus are ATEL-driven.

The exits-fix becomes a one-line change in `exits.js` — swap `0x8002`
for `0x0011`/`0x010B` and re-test on Thunder Plains. Out of scope for
this session per user direction.

### Not done this session

- Phase 4 (Ghidra label-import script that walks the runtime function
  tables and labels the resolved RVAs `atel_<ns>_<name>_<role>`).
- Phase 5 (field-menu investigation against the re-exported annotated
  decompile).
- Phase 3 verification (one-shot Frida probe spot-checking 10 IDs from
  3 namespaces against the runtime function tables — TSV correctness
  has been spot-checked statically against source, but the runtime tie
  is not yet confirmed).
- Production exits fix (deferred per user direction).

KEYWORDS: FFXDataParser Karifean atel call target names ingestion,
ScriptCallTargetLib opcode 1182 entries 11 namespaces, Fahrenheit
cross-reference arg counts call.cs, TSV annotation layer
Docs/Scripts/Ghidra/output/FFX/atel, transitionToRoom warpToRoom 0x0011
0x010B Common namespace correct warp primitives, 0x8002 setGfxActive
NOT AtelSetEventJump2 session 63 misattribution corrected, MapEntrance
AtelScriptChunk AtelScriptHeader full struct layouts from Fahrenheit,
namespace runtime base addresses 0x850050 0x842618 0x85DC90, AtelSetEventJump2
0x46FED0 native dispatcher Fahrenheit hook, exits.js deferred fix
Thunder Plains, extract_atel_names.py Python data transformation
idempotent

### Phase 5 follow-up — field menus do NOT use ATEL (clean negative)

User direction: skip Phase 4 (Ghidra label-import) and go straight to
Phase 5 (field-menu ATEL audit) using the un-annotated decompile +
TSV cross-reference instead of a re-exported annotated decompile.

Audited every known field submenu fn_ctrl + the dispatcher
(`FUN_008AA240` @ RVA `0x4AA240`):

- Help      0x4BEF20
- Abilities 0x4C6080
- Aeons     0x4CC300
- Equip     0x4CEFF0
- Overdrive 0x4D0430
- Status    0x4D1DD0
- Formation 0x4D5510
- Config    0x4CB0B0
- Help      0x4BEF20

**Verdict: clean negative.** All 8 submenu fn_ctrls and the dispatcher
are pure native C++ state machines. They use TkWindow + DAT_ cursor
globals + direct glyph rendering or MsMenuGetText. None reference any
ATEL namespace base address, none use the call-table indexing pattern
the value in indexed table entry idx, using stride 0x10 and field offset 4 from ptr (references: 0x4), none call AtelSetEventJump2 (0x46FED0).

ATEL usage in the binary IS present, but concentrated in
battle/cutscene/character-event systems:
- `layer2_game_logic.txt:424047` — "atelscript in Ch_SetRot"
- `layer2_game_logic.txt:507830–508714` — "ATEL_DEBUG LIMIT CHECK"

The user's hypothesis that the lack of ATEL names was part of why
field menus have been hard is **refuted**. Open field-menu work
(Items fn_ctrl unknown, NaN cursor reads, glyph text rendering) needs
different angles: TkWindow iteration patterns (session 59 lead),
per-submenu DAT_ discovery, or direct glyph-buffer reading.

The ATEL annotation layer is still useful for: the deferred map-exits
fix (now a one-liner — swap `0x8002` for `0x0011`/`0x010B` in
`exits.js`), battle/cutscene RE work, and character-event
investigation. None of these were in scope this session.

### Files changed (full list this session)

- new: `Docs/Scripts/Python/extract_atel_names.py`
- new: `Docs/Scripts/Ghidra/output/FFX/atel/atel_namespaces.tsv`
- new: `Docs/Scripts/Ghidra/output/FFX/atel/atel_calls_<nsId>_<short>.tsv` × 11
- new: `Docs/Scripts/Ghidra/output/FFX/atel/atel_opcodes.tsv`
- new: `Docs/Scripts/Ghidra/output/FFX/atel/atel_arg_count_disagreements.tsv`
- new: `Docs/Scripts/Ghidra/output/FFX/atel/README.md`
- mod: `Docs/gameArchitecture.md` (MapEntrance expanded; AtelScriptChunk + AtelScriptHeader added; warp-call section corrected; namespace tables + transition natives added)
- mod: `Docs/debug.md` (Known Issues entry for deferred exits fix; Solved Problems entry for the field-menu ATEL negative)
- mod: `Docs/session51_current.md` (this entry)
- memory: `reference_atel_data_sources.md` (new); `MEMORY.md` index updated

KEYWORDS phase 5: field menu ATEL hypothesis refuted clean negative,
fn_ctrl audit submenu Items Equip Abilities Aeons Status Formation
Config Overdrive Help, FUN_008AA240 dispatcher native C++ state machine,
TkWindow MsMenuGetText DAT cursor globals not ATEL, namespace base
addresses 0x850050 not referenced from menus, atel concentrated in
battle cutscene Ch_SetRot ATEL_DEBUG LIMIT CHECK, Items fn_ctrl still
unknown different angle needed TkWindow iteration session 59 lead,
deferred exits fix one-liner 0x0011 0x010B


## Session 68 — 2026-05-12 — Applied the deferred exit-destination fix

Wired the exit list to the existing map-name resolver by correcting the
bytecode walker in `src/navigation/exits.js`. Goal: when the user cycles
the exit list, each entry should be announced as "Exit to <area name>"
instead of the positional / "Exit (unverified) to 0x<id>" fallbacks.

The fix follows Session 67's correction: the real script-level warp
primitives are `0x0011 transitionToRoom?` and `0x010B warpToRoom?` in
the Common namespace, NOT the previously-suspected `0x8002`. Two
additional traps the old code fell into:
1. The CALL-ID match was being derived dynamically by walking namespace
   tables and matching native function pointers against the obsolete
   `EXIT_FN_RVAS` list — that whole machinery is now gone.
2. The destination map ID was being read from PUSHI (0xAD), whose
   operand is an INDEX into the chunk's int reference table, NOT the
   literal. Real exit destinations come via PUSHII (0xAE), whose operand
   IS the literal. See `gameArchitecture.md:1549–1552`.

Changes:
- `src/navigation/exits.js`:
  - Imports trimmed to `RVA_ATEL_CONTROLLER` only (dropped
    `ATEL_NAMESPACES`, `EXIT_FN_RVAS`, `base`).
  - Deleted `exitAtelIds` lazy state and findExitAtelIds (~40 lines
    of namespace-table walking).
  - workerScanBytecode signature dropped its `exitIds` parameter.
  - Added PUSHII (0xAE) tracking (`lastPushii1` / `lastPushii2`),
    mirroring the existing PUSHI trackers and reset at the same
    per-entry boundary.
  - CALL/CALLPOPA (0xB5/0xD8) branch now matches operand equals 0x0011 or operand equals 0x010B and reads result.exitTargetMapId receives lastPushii2
    (the OLDER PUSHII = `room_id`; `lastPushii1` is `spawnpoint_id`).
  - Removed the obsolete when (exitIds entry lastPushi1) result.isExit receives true
    line inside the REQ branch — it would have thrown a ReferenceError
    after the import was dropped.
- `src/core/addresses.js`: deleted the now-unused `ATEL_NAMESPACES` and
  `EXIT_FN_RVAS` exports. `RVA_ATEL_CONTROLLER` retained.

scanExits already called resolveMapName using scan.exitTargetMapId at
the renamed line 263, and announceSelected in `navigation/index.js`
already speaks `e.name` — so no edits to the speech path were needed.
The fix is structural: feed the resolver a correct ID and the existing
plumbing does the rest.

Built and deployed via `src/build/build.bat`. User to verify in-game on
Thunder Plains (the Session 64 failure case from `debug.md:359`):
cycle the exit list with VK_OEM_4/VK_OEM_6 and confirm speech of the
form "Exit to <area>, X.X steps <dir>, N of M". Mod log should show
`[WORKER ...]` lines reporting either call identifier `0x11` or `0x10b`
on exit workers.

Residual risks (not addressed by this session):
- Flow-walker reach. If warp CALLs live behind plot-state branches the
  walker never visits, the call won't be classified as EXIT. Separate
  work item if Thunder Plains still falls back to positional labels.
- PUSHII as the only literal source. Documented in
  `gameArchitecture.md` but not validated by a runtime probe on a
  known-good exit before this commit. If `exitTargetMapId` comes back
  null too often, a one-shot Frida probe at `CALL 0x0011`/`0x010B`
  call sites would be the next debugging step.

Files touched:
- mod: `src/navigation/exits.js`
- mod: `src/core/addresses.js`
- mod: `Docs/session51_current.md` (this entry)
- mod: `Docs/debug.md` (moved deferred-exit entry to Solved Problems)
- mod: `Docs/gameArchitecture.md` (updated status note for exits.js)

KEYWORDS: exit destination speech, exits.js fix, 0x0011
transitionToRoom 0x010B warpToRoom Common namespace warp primitives,
PUSHII 0xAE literal operand vs PUSHI 0xAD reference-table index,
lastPushii2 room_id capture, removed EXIT_FN_RVAS ATEL_NAMESPACES
findExitAtelIds dead code, resolveMapName wired correctly, Session 64
Thunder Plains test case, FFXDataParser Karifean Fahrenheit confirmed
call signatures, speech path label e.name announceSelected unchanged


## Session 69 — 2026-05-15 — Release 0.1.3-ShotgunBuild

Packaged the 0.1.3-ShotgunBuild test release per `Docs/release.md`.

Pre-flight: git clean on master, no unpushed commits, tag
`0.1.3-ShotgunBuild` does not exist. `install/ffx_sr.js` and
`src/proxy/dinput8.dll` are current (no source under `src/` newer
than either artifact); deployed bundle in the game dir matches
`install/ffx_sr.js` byte-for-byte. Mod log `FFX_SR_Latest.log`
(May 12 14:44) contains no ERROR/FAIL/CRASH lines. No rebuild
needed — used the existing build artifacts.

Staged `releases/0.1.3-ShotgunBuild/ffx-x2-screen-reader-0.1.3-ShotgunBuild/`
with `install/` payload (`dinput8.dll`, `frida-gadget.dll`,
`frida-gadget.config`, `ffx_sr.js`, `Tolk.dll`,
`nvdaControllerClient32.dll`), `README.txt` (verbatim copy of repo
`README.md`), and a new `CHANGELOG.txt` covering sessions 51–68
since the 0.1.2 release: field-submenu reads via TkWindow iterator
exact-port (session 59), Help submenu sceneId gate (session 61),
Status review buffer hotkeys `,`/`.` (session 60), Overdrive
auto-success for Tidus/Auron/Lulu default ON (sessions 60+62), NPC
dialogue via FUN_008B8980 (sessions 57+58), ESC menu help-text read
+ zero-fill scratchBuf (sessions 57+58), save/load popup body +
Yes/No cursor inversion fix (sessions 51–56), sphere-grid charset
opcode-length fix (session 52), exit destination resolver finally
wired via `0x0011`/`0x010B` warp primitives (session 68), entity
announce count-at-end format (session 63), dev-slug parsing
`chr_wakka_a`→"Wakka" (session 60), no-dedup rule applied to
production speech (session 56). Known Issues section carries
Wakka's Slots (no static handler), callback-registered exits
unresolved, plus all prior known issues.

Zipped with `7z.exe a -tzip -mx=9` to
`releases/0.1.3-ShotgunBuild/ffx-x2-screen-reader-0.1.3-ShotgunBuild.zip`
(7706803 bytes after a one-cycle rebuild to put the
`ffx-x2-screen-reader-0.1.3-ShotgunBuild/` parent folder at the
zip root; first attempt globbed contents at root which violates
the release.md verify rule). Zip listing confirms the parent
folder structure plus all 6 install/ files + README.txt +
CHANGELOG.txt.

Git tag not created — user confirmation pending per release.md
Step 5.3 and CLAUDE.md "ask before shared-state changes".

Files changed this session:
- new: `releases/0.1.3-ShotgunBuild/ffx-x2-screen-reader-0.1.3-ShotgunBuild/` (staged folder + payload)
- new: `releases/0.1.3-ShotgunBuild/ffx-x2-screen-reader-0.1.3-ShotgunBuild/CHANGELOG.txt`
- new: `releases/0.1.3-ShotgunBuild/ffx-x2-screen-reader-0.1.3-ShotgunBuild/README.txt` (copy of repo README.md)
- new: `releases/0.1.3-ShotgunBuild/ffx-x2-screen-reader-0.1.3-ShotgunBuild.zip`
- mod: `Docs/session51_current.md` (this entry)

KEYWORDS: release 0.1.3-ShotgunBuild test build, sequential bump
from 0.1.2-ShotgunBuild, no rebuild needed install ffx_sr.js
dinput8.dll current, 7-Zip parent folder layout fix, CHANGELOG
sessions 51-68 user-visible summary, git tag deferred user
confirmation, releases gitignored staging dir

## Session 70 — 2026-10-03 — Native rewrite/menu/map/NPC/launcher audit

Preserved the source worktree's existing journal/deploy changes. User requested
C++ rewrite investigation, native menu discovery, missing speech inventory,
mismatched exit correction, contextual NPC inference from Bravely Default's
approach, and original Steam launcher speech investigation. The other working
implementations are private, so no source was available for comparison.

Source findings and ownership/bootstrap design are in gameArchitecture.md's
2026-10-03 audit; current coverage is in plan.md and reopened failures in debug.md.
Confirmed reversed TkMenu control/draw documentation and menu-specific TkWindow
data. Items has installed but incomplete code; several submenus have no reader.
No speculative new production menu hooks were added.

Read-only offline extraction produced all 397 EBP scripts plus English text
dictionary/charset evidence. Corrected sparse EBP chunk boundaries; retained
v1/v2 and final maps_v3.json snapshots. Generated 304 unambiguous room/area-index
mappings after checking all script variants. All reference attribution retained.
595 trigger-worker records reach warp calls; 559 have one known constant room.
Offline inference produced 64 distinct non-test speaker-header proposals and
30 merchant-role proposals. Live actor binding and icon semantics remain untested;
no inferred NPC narration was deployed.

Exit fix uses worker-local jump/int tables, JSR worker entries, path-owned
arguments, native area bounds/getters, and stable workerIdx. Exact regression:
znkd1400 worker 31 -> room 371/spawn 0 -> area index 340, Zanarkand Stadium,
instead of the logged Exit to None. Uncertain routes keep generic Exit labels.

Launcher decompiled cleanly with ILSpy: WinForms ActiveRect image selector and
ordinary settings controls. Extracted XML and visually checked the original
English image's seven labels. A supported AppDomainManager startup extension
offers a concrete alternative to the failed version/mscoree proxies. No launcher
addon or configuration was installed. New diagnostics appear in existing menus:
Frida game 4/native_ui_events (F9); Ghidra FFX/native_reader_anchors.

Validation: 14 ATEL regressions + 3 VBF tests + 3 extraction tests passed,
including all 397 parsed files, sparse chunks, real speaker headers and area-name
domain checks. Modified/new production files are under 500 lines. Canonical
src/build/build.bat built and deployed successfully after the sandbox denied the
first deployment; approved escalation used the same canonical script.
Built/deployed bundle SHA256 matches:
6e0cee82a0f21b10e5158b1589da2e75a6c175fe9488a3bc3ca4b01099ea441e.
No game launch or Frida/Ghidra probe execution; live transition validation pending.

KEYWORDS: native C++ Gadget rewrite threading loader lock owner thread copied
speech queue generation, native menus TkMenu control draw reversed, silent menus
inventory column text pointer, map mismatch jump table JSR PUSHI area index room
371 340 Zanarkand Stadium, VBF zlib sparse EBP 397, contextual NPC Oaka Belgemine
Maechen merchant, original Steam launcher WinForms ActiveRect AppDomainManager

Session 70 follow-up: user asked for next-task documentation and Cloister solutions.
Added a concrete handoff/task instruction to plan.md. Located all six trial-labelled
scripts, extracted native prompts and raw per-worker variable declarations into
cloisters_20261003_202810.json using workspace data only. Inventory verifies all
seven expected scripts and source hashes. Rule graphs, geometry/state semantics,
treasure/revisit paths and live guidance remain work for the next task. No source
or deployed gameplay behavior changed in this follow-up.

Session 70 priority correction: user approved the handoff with the native C++
rewrite first and deployed exit validation second. Updated plan.md and both
memory entries; transition/load/battle-return stress testing remains part of the
native vertical slice. Exit validation is not a prerequisite for native work.

## Session 71 — 2026-10-03 — Native vertical slice, launcher addon and menu repair prototypes

User requested the complete C++ rewrite/Gadget removal, native launcher speech,
parity with existing functionality, and Items/remaining silent menus. Followed
plan.md's required first native slice and live transition gate; the full request
remains unfinished. Preserved the existing exit repairs and uncommitted work.

Implemented and built an x86 main-menu/character reader, one copied-string Tolk
worker, deferred DirectInput bootstrap, native ownership exports and resource
generation/owner instrumentation. Only the main-list/character JS producer is
disabled after a successful native batch. All other existing implementations
remain loaded through Gadget and submit speech through the native bridge.
Resource-loader return is explicitly not actor-population completion. C++/CLI
extends the original WinForms launcher using its actual hotspot actions, focus
and value events; speech lives in a separate native DLL. Config merging retains
the original, refuses other domain managers and supports restore.

Removed the broken Items controller attempt and added native-source Frida
prototypes for ordered Items/Key Items/Abilities, command bars, Equip, Overdrive,
Formation, Aeon selection/Learn/Increase, Customize gear/recipes, Help and Sphere
Grid action/sphere inventory selection. Archive bodies corrected Formation's
scene attribution, Overdrive's full linear cursor, Help's rebased pointers and
Sphere Grid's pool/callback/layout assumptions. Architecture evidence is in
gameArchitecture.md's native migration section. Shops, ESC labels/values,
attributes/details/requirements and other coverage gaps remain open; no claim
that all menus or all existing features now have native parity.

Validation: canonical native/launcher build passes with x86 tools. Offline menu
fixtures verify ordering, columns, quantities, native getters, repeated labels,
activation/return, field routing, Help idle silence, Sphere Grid full-index
scrolling and nested event ownership. Charset fixtures preserve the old decoder
default and verify terminated-source behavior. Native tests cover page boundaries,
copied strings, 400 repeated valid events from four producers, one Tolk thread
and generation advance. A DirectInput surrogate verifies actual forwarding and
deferred startup without enabling FFX hooks. A hidden launcher surrogate verifies
real CLR AppDomainManager startup, original action invocation, key-repeat handling,
modal isolation and settings speech through fake Tolk. Installer checks verify
merge/idempotency/backup/conflict refusal/restore. All pass. The prior 13 ATEL
regressions pass. Modified/new production files are under 500 lines.

Canonical src/proxy/build_dinput8.bat deployed all coordinated artifacts; the
later JS-only canonical src/build/build.bat included Sphere Grid prototypes and
the byte-return getter correction. Deployment retained original proxy/config,
installed x86 dependencies/attribution, and verified installed artifact hashes.
Final bundle SHA256: 911953d19daf2f2476c401e253f4c06abd1a75c3c0382d52f93590b8674f3058.
Native proxy SHA256: edc2ccb4e184ff6b5b162db8d7f0faaf686a9447e3f8672d9bfaf10763c4fa47.
Build/deploy logs: install/native_build.log, native_deploy.log, menu_deploy.log.
Offline test logs: install/native_tests.log, launcher_tests.log.

Asked for the required fresh-launch launcher/menu/Items and repeated map/load/
battle-return check. No new game/launcher runtime logs were present when checked;
live validation is pending. No game launch, Frida attachment, Ghidra run or game
install exploration was performed. Only canonical deployment touched installed
artifacts; own-mod log reads are permitted. Native list coverage diagnostic is
registered by existing launcher discovery, timestamped, event-only and unexecuted;
existing Help probe offsets were corrected. Retained timestamped archive extracts.
Gadget removal and further production migration require the pending live evidence.

KEYWORDS: C++ native x86 MinHook deferred DirectInput Tolk_Output braille copied
speech generation owner parity Gadget migration gate original WinForms launcher
C++/CLI AppDomainManager keyboard Items Key Items two columns Overdrive linear
index Formation Customize Aeons Help event decoder idle Sphere Grid submenu
callback scroll nested ownership native_list_events backup restore deployment

## Session 72 — 2026-10-03 — Complete existing-reader C++ migration before live testing

User explicitly preferred migrating all existing functionality before testing.
That supersedes Session 71's intermediate transition gate. Ported every existing
FFX reader/hotkey to native C++ and removed Gadget/JavaScript from production
startup/build/deployment. FFX-2/Last Mission do not yet have verified readers;
their executables receive forwarding with no FFX RVA hooks.

Native owners now cover field/main/character/Items/Key Items/Abilities/bars,
Equip/Overdrive/Formation/Aeons/Customize, dialogue/subtitles/title/TkWindow
prompts/choices/Help, save/load/ESC/config/status, Sphere Grid, battle commands/
targets/HP/MP/actions/damage/status/rewards/existing Overdrive assists, and
navigation/entities/exits/ATEL/navmesh/A*/all review hotkeys. Original launcher
C++/CLI adapter and native Tolk remain. Game speech uses one copied-string
Tolk_Output worker, including braille, with producer/owner/generation logging.

Added source-derived silent-menu coverage: native shop buy/sell names/order/
prices/owned quantities/item quantity totals, generic confirmations, equipment
ability slots, Aeon attributes/learned abilities, Learn/Customize/Increase
material requirements and ESC Audio value speech. Corrected Increase display
labels to namespace C/table+4; namespace B/table+8 is later confirmation text.
Requirements queue after the outer stat label so that its interrupt does not
discard them. Restored step count in navigation's no-navmesh path fallback.

Hook adapter preserves original x86 argument stack, ECX/EDX, flags/registers,
x87/SSE and calling-convention cleanup. Copied per-call snapshots have separate
shared-hook listeners and generation guards; reader getter calls do not recurse.
Original/reader exception and generation tests pass. Actor allocation/position/
disposal events replace passive actor scans; native battle completion rescans.
DirectInput keyboard CreateDevice/read/release events own actual input, with
qualified window-message fallback and owner-thread dispatch. No new polling,
timer, text-content deduplication or learned labels. Existing dialogue suppression
and the previously authorized three Overdrive assists are preserved.

New Frida prototypes remain offline references, built through --prototype;
the full prototype entry disables every producer when native mask 7F is present.
Production build forwards to the native canonical entry and never compiles or
deploys JS/Gadget. Original legacy files remain only for proxy rollback.
restore_before_native.bat restores original proxy/launcher config and accurately
states that offline rebuilt prototypes are not installed.

Validation passes: x86 native/launcher production build with no warnings;
11,000 mixed ABI calls plus runtime hook additions, EDX:EAX return, exception
unwind/recovery and stale-generation suppression; actual production readers
at 65 hook sites including save/config/status/rewards, requirements/attributes/
equipment, navigation scan/categories/distance/fallback/disposal; native ATEL/
navmesh/A* fixtures; DirectInput state/buffered keys, Shift, held repeat, focus,
generation, nested message ownership and fallback; copied Tolk speech/guarded
decoding; real DirectInput forwarding/no Gadget; original launcher CLR/actions/
settings and installer preservation/idempotency/backup/conflict/restore fixtures.
Retained JS menu/charset fixtures, offline prototype build and 13 prior ATEL
regressions also pass. Native/launcher production sources remain below 500 lines.

Canonical src/proxy/build_dinput8.bat deployed the final game/launcher artifacts
with retained backups and mandatory installed SHA256 checks:

- dinput8.dll: 110cd9f934e4315c655a35d2bf13ec411caddd77aab94d132a9c7dfedb2beb8d
- MinHook.x86.dll: 1f28d13b8133551c8896cc556db768c94ebfaae0ad38e86719f3151f7b4ce1fc
- FFXLauncherReader.dll: 15d1d0065689a1fc09b3737289901a808946155866652f676f0db0eb17c23212
- FFXLauncherNative.dll: 9b9f1add00ae2f1d2da037e70242783fb1198922e2ba3800a171a232804fa0db

Evidence: install/native_deploy_all.log, native_tests_all.log,
native_abi_final.log, launcher_tests_all.log, menu_events_all.log,
charset_all.log, atel_all.log and prototype_build_all.log.
Timestamped archive body extracts retained locally. No game launch, live Frida
probe or Ghidra analysis was run; only canonical deployment touched the install.

Migration of existing implementations is complete; gameplay parity is pending
the combined fresh-launch launcher/menus/maps/save-load/battle checkpoint.
Items' live repair and the original transition failure are not declared solved
by offline checks. ESC focused labels/video/key-binding values, comparison/detail
panels and detailed Sphere Grid stat/cost/activation feedback remain open.
Production logs are FFX_SR_Native.log and FFX_Launcher_SR.log; the old
FFX_SR_Latest.log is no longer updated. Updated established plan/architecture/
debug/memory and project stack/build guidance; preserved earlier work/reference
trees. No commit or push was requested or performed.

KEYWORDS: C++ native x86 Gadget-free parity migrate all before test DirectInput
ABI exception generation Tolk_Output braille launcher Items shop confirmation
equipment abilities Aeon attributes requirements Help Sphere Grid navigation
ATEL actor disposal fallback path SHA256 deploy rollback silent menus

---

## Session 73 — 2026-10-03 — All-six Cloister designs and shared validation prototype

User requested all six designs in one task, with no testing stop between trials.
Implemented the complete static design set before running combined checks.

Added tracked src/puzzles tooling: six dependency-phase/reset/revisit contracts,
worker-local guarded ATEL compiler, hashed evidence generator, event-only Frida
state reader, user-run Python host, launcher registration and offline fixtures.
Generated 601 selected entry models and 6,598 REQ/JSR links across six primary
scripts; linked dome0200 flow and other Dome source/room metadata are retained.
Every trial preserves normal completion, treasure call and shared 0xA70 flag
write witnesses, with raw declaration aliases and source worker/bytecode PCs.
These are static control-flow models and coarse route designs, not validated
spatial solutions or currently available player actions.

Archived native bodies resolve variable scope/property decoding and pointer
getter semantics. The prototype reads selected declarations via native getter
46C500 on game-thread writer/CALL return events. It copies state for RPC,
checks live source identity/ownership/bounds, keeps repeated writes and rejects
late-generation returns after map/resource invalidation. Bevelle's 72-element
matrix/two slots and Zanarkand's initial/revisit stages remain separate.
No speech, polling, timer, movement automation or puzzle-state write was added.
The existing launcher discovers registered cloister_trials via its @python tag.

Combined offline validation passed: 11 Python cases, all-six mocked native-event
fixtures and JS syntax. Archive:
Docs/Scripts/Offline/output/FFX/20261003_audit/cloister_designs_20261004_004225_782375.
Log: install/cloister_checks_20261004_004228_711014.log. Existing timestamped RE
extracts and intermediate generated outputs are retained. Live native ABI/hook
coexistence, actor/spatial binding, charge/bridge/rail/tile/reset/revisit behavior
and actionable directional guidance remain unvalidated.

Updated plan, architecture, debug and concise memories; detailed design evidence
and one combined user-run handoff are in Docs/cloisters.md. No Frida/Ghidra/game
execution, install access, C++ production build, deploy, commit or push occurred.
Production remains the previously deployed native reader; the new diagnostic
is an offline prototype until user-run observations validate its behavior.

KEYWORDS: Cloisters all six Besaid Kilika Djose Macalania Bevelle Zanarkand
ATEL variable scope native getter alias CFG REQ JSR completion treasure reset
revisit two slots rail tile pattern event generation copied snapshot prototype

## Session 74 - 2026-10-03 local / 2026-10-04 UTC - Authorized game/RE tests

User clarified that the six Cloister solutions must be playable and authorized
Codex to operate the game and RE launcher directly. No user visit to inaccessible
trials is required. All-six native binding/interaction pilots ran; the requested
production solutions remain unfinished and were not deployed.

Added current native profiles, corrected Frida x86 mscdecl ABI and short script
name decoding, plus explicit RE-only driver, batch sequence host and read-only
native diagnostics under src/puzzles. Native getter calls execute on real game
callbacks; mutable test commands queue onto native input events. Dialog ack
uses the proper native remapper phase and prevents extra field confirmations.
No speech producer or test controls were added to the production reader.

Combined pilot: cloister_sequence_20261004_061222_196262.jsonl, 66 completed
commands, zero diagnostic errors/missing selected fields, all six room/scripts.
Direct requests bypass availability and story remained the loaded context;
neither pilot snapshots nor carried-over Besaid treasure bit prove six solutions.
Earlier Besaid exploration and native body/stack/window/input diagnostics remain
timestamped. Source specifics, witnesses and pending gameplay semantics are
recorded in Docs/cloisters.md.

Native runtime blocker: installed FFX.exe SHA256 begins 0537B2A1047F, differs
from reference addresses, and existing native reader batch fails unsupported
hooks at 4CCEA0, 4D77C0, 4A2100 and 426770. Other addresses also need validation.
Blank capture persisted in proxy and stock startup comparisons; native scripts
continued running. No unsupported conclusion that the game engine was frozen.

Combined offline checks passed after all six pilots: 11 Python cases, both
six-trial profiles, unsupported/mixed refusal and JS syntax. Final archive
cloister_designs_20261004_062020_133594; log cloister_checks_20261004_062023_705913.log.
Machine-readable pilot summary and cleanup record are under
install/cloister_test_runtime. No C++ puzzle implementation, production build,
new deployment, commit or push occurred. Existing dirty work was preserved.

Closed test hosts and FFX. Restored pre-test dinput8 proxy and verified SHA256
110CD9F934E4315C655A35D2BF13EC411CADDD77AAB94D132A9C7DFEDB2BEB8D.
The ffx_000 autosave changed during RE testing and was restored; all five save
hashes match pre-test backups. Temporary proxy rename was removed. Future work
must repair/validate the installed native profile and finish all-six route,
reset/revisit, spatial and timed-mechanic tests before shipping guidance.

KEYWORDS: Cloisters all six native profile getter mscdecl input remapper ack
unsupported hooks blank capture guarded request availability autosave restore

## Session 75 - 2026-10-04 - Launcher sounds and exact native profile repairs

User reported missing keyboard hover/select sounds, launcher left open after a
manual direct game launch, dotnet build dialogs and silent game title. They saw
the actual game; earlier blank captures occurred with the laptop lid closed.
They require notification before launching or taking keyboard focus.

Source launcher fix uses original mouse event paths, preserving FMOD sounds and
RunExeAndExit closing, plus exe startup cwd. Offline launcher/installer suite
passes. Native proxy/Tolk did load; unsupported reference targets aborted all
readers. Read-only existing Ghidra exports and x86 matching resolve all 174
required addresses (zero unresolved/conflicts) for exact installed SHA256.
Added exact-hash selection, explicit address map and callback normalization.
Both reader profiles, unknown-executable guard and full native suite pass.
The runner now catches negative crash statuses; initial success was false for
the installed fixture, which contained an unverified opaque callback address.
It uses a synthetic ignored draw callback and passes all reader cases.

The repair was deployed. Native features 127/speech-ready 1 and accepted
Tolk title/load/field outputs were observed; user confirmed launcher sounds.
Long manual Cloister route validation was then replaced by explicit utilities. Canonical native cl/link
builds avoid SDK/MSBuild; dotnet dialog cause not yet established. VBCSCompiler
RADAR_PRE_LEAK_64 warning is evidence, not proof of exact dialog ownership.
Latest scoped logs/audits/notification requirement: Docs/cloisters.md.


### 2026-10-04 continuation — user-controlled utilities

User requests native auto-solving with treasure before exit, ATEL labeling and
ambient cleanup, and an accessible warp console. They confirm threading and
exit destinations; future warps occur only when they request them. No further
live launch, attachment or keyboard test was performed during implementation.

Added all six original-script reveal/collection contracts, guarded native
continuations, monotonic completion/exit handling, F12 console and bounded
warp search. Audited 397 scripts, retained 2,460 relevant targets and omitted
6,941 ambient/non-interactive candidates. Named actor labels use source
speaker/model evidence, with honest role fallbacks. Updated exact profile to
179 verified RVAs. Combined offline native checks pass; final staging/install
and live limits are in Docs/cloisters.md. No C# or dotnet build was used.

Final canonical native build and full fixture/reference suites passed. With
no FFX/launcher processes running, the working repair was backed up and the
utility update deployed; all four artifact hashes matched. No game launched
or saves changed during deployment. Backup utilities_before_20261004_095911;
verification install/native_utilities_deployment_verified.json. New gameplay
and GUI validation remains pending under user control.
