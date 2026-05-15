// Save/Load screen reader.
//
// Architecture (correcting an earlier misread): save/load is TK-driven.
// The state machine lives in FUN_006EFxxxx-FUN_006F0xxxx (FFX.exe); all
// menu logic, cursor state, and input dispatch are TK. Flash (saveload_*.swf)
// is just the visual renderer — the TK side pushes pre-resolved data to it
// for display, but Flash never owns state or input.
//
// What we hook:
//
// 1. Three TK helper functions that resolve localized strings via TK's
//    language manager (FUN_006480E0 → FUN_006DB320, sLanguageManager
//    singleton, plain 8-bit ASCII output) into fixed buffers, then mirror
//    to Flash for animation:
//
//      FUN_00702E00  setSaveLoadHelpText(slot_used, msg_loc_id)
//        loc(msg_loc_id) → DAT_00CEBCC8 (body),
//        sprintf "<used>/200" → DAT_00CEBEC8
//      FUN_00702F70  showPopUpStd(msg_loc_id)
//        loc(msg_loc_id) → DAT_00CEB450, processed → DAT_00CEB250
//      FUN_00702FE0  showPopUpWithBtn(msg_loc_id)
//        loc(msg_loc_id) → DAT_00CEB690 (popup body),
//        loc(0x204)      → DAT_00CEB890 (button 1, e.g. "Yes"),
//        loc(0x205)      → DAT_00CEBA90 (button 2, e.g. "No")
//
//    These hooks fire when the TK state machine emits a popup. The buffers
//    are populated by the TK loc resolver before the function returns, so
//    onLeave reads the correct text. (The Flash mirror call at the tail
//    of each function is incidental — pure visual handoff, no menu logic.)
//
// 2. Three TK input handlers that toggle DAT_00CE72F0 (Yes/No cursor)
//    on Up/Down input in the popup confirmation states:
//
//      FUN_006EFD90  popup-state input handler (state ~0x14)
//      FUN_006F00E0  popup-state input handler (state ~0xD)
//      FUN_006F06F0  popup-state input handler (state ~0x2/0x4)
//
//    All three call FUN_006EF460 to read input, branch on event type
//    (3=Up, 4=Down, 7=Confirm, 8=Cancel), and on Up/Down do
//    `DAT_00CE72F0 = 1 - DAT_00CE72F0`. We hook onLeave, read the new
//    cursor value, and announce the corresponding button label from
//    DAT_00CEB890 / DAT_00CEBA90.

import { rva } from '../core/memory.js';
import { attachRva } from '../core/hooks.js';
import { speak, logFile } from '../core/speech.js';
import { readCString } from '../utils/text.js';

// Save/load popup string buffers (RVAs = Ghidra absolute - 0x400000).
const POPUP_STD_BUF      = rva(0x8EB250); // showPopUpStd processed text
const POPUP_BTN_MSG_BUF  = rva(0x8EB690); // showPopUpWithBtn body
const POPUP_BTN_OK_BUF   = rva(0x8EB890); // showPopUpWithBtn button 1
const POPUP_BTN_NO_BUF   = rva(0x8EBA90); // showPopUpWithBtn button 2
const HELPTEXT_MSG_BUF   = rva(0x8EBCC8); // setSaveLoadHelpText body
const HELPTEXT_COUNT_BUF = rva(0x8EBEC8); // setSaveLoadHelpText "<n>/200"
const POPUP_YESNO_CURSOR = rva(0x8E72F0); // DAT_00CE72F0 — popup Yes/No cursor

const saveSlots = {};
let prevSaveSlot = -1;
// lastYesNoCursor is CHANGE DETECTION (did the cursor index change since
// the last time the input handler hook fired?), not text-content dedup.
// Without it the cursor handler — which fires every frame the popup is
// up — would re-announce the same focused button continuously. This is
// the kind of dedup that's needed; it's not suppressing user-meaningful
// re-announces.
let lastYesNoCursor = -1;
// Set by the popup-body announce so the immediately-following cursor-
// handler announce queues behind the body (interrupt=false) instead of
// preempting it. Cleared after the first cursor announce; subsequent
// cursor moves stay responsive (interrupt=true).
let queueNextCursor = false;

function readBuf(addr, max) {
    try {
        const s = readCString(addr, max);
        return (s && s.length > 0) ? s : '';
    } catch (e) { return ''; }
}

export function install() {
    // Slot population — fires when a save slot row is built (per-row):
    // arg0=slot index, arg2=area name C string, arg5=playtime C string.
    // We cache the resolved labels so the cursor-focus hook can speak them
    // without re-reading the buffer (which gets overwritten as more rows
    // populate).
    attachRva(0x302E70, {
        onEnter: function(args) {
            try {
                const slotIdx = args[0].toInt32();
                const area = readCString(args[2]);
                const playtime = readCString(args[5]);
                if (area || playtime) {
                    saveSlots[slotIdx] = { area: area || '', playtime: playtime || '' };
                    logFile('[SAVE] slot ' + slotIdx + ' populated: area=' + area +
                        ' playtime=' + playtime);
                }
            } catch (e) {}
        }
    }, 'Save slot populate');

    // Cursor focus change in the save/load slot list. arg0 = newly-focused
    // slot index. Speak the slot label so the user knows where the cursor is.
    attachRva(0x302DB0, {
        onEnter: function(args) {
            try {
                const slotIdx = args[0].toInt32();
                if (slotIdx === prevSaveSlot) return;
                prevSaveSlot = slotIdx;
                const info = saveSlots[slotIdx];
                if (info) {
                    let slotLabel = 'Slot ' + slotIdx;
                    if (info.area) slotLabel += ': ' + info.area;
                    if (info.playtime) slotLabel += ', ' + info.playtime;
                    speak(slotLabel, true);
                } else {
                    speak('Slot ' + slotIdx + ' empty', true);
                }
            } catch (e) {}
        }
    }, 'Save cursor focus');

    // Save/load confirmation popup with Yes/No buttons (showPopUpWithBtn).
    // Fires for "Overwrite this save?" / "Load this save?" etc. We speak
    // ONLY the prompt body here — both button labels go unread. The
    // immediately-following input-handler cursor hook announces the
    // currently-focused option (queued behind this body via the
    // queueNextCursor flag), and subsequent Up/Down toggles announce the
    // newly-focused option. Reading both options up front is redundant
    // when the cursor handler covers them.
    //
    // No content-dedup — every popup-show should announce, even if the
    // text is identical to a previous popup (different slot/save context).
    attachRva(0x302FE0, {
        onLeave: function() {
            try {
                const msg = readBuf(POPUP_BTN_MSG_BUF, 256);
                if (msg.length === 0) return;
                speak(msg, true);
                logFile('[SAVE POPUP BTN] ' + msg);
                queueNextCursor = true;
            } catch (e) {}
        }
    }, 'Save/load popup with buttons');

    // Save/load notification popup, no buttons (showPopUpStd). Fires for
    // "Saving..." / "Save complete" / system-notice style messages.
    // No dedup — re-announces on every popup-show.
    attachRva(0x302F70, {
        onLeave: function() {
            try {
                const msg = readBuf(POPUP_STD_BUF, 256);
                if (msg.length === 0) return;
                speak(msg, true);
                logFile('[SAVE POPUP STD] ' + msg);
            } catch (e) {}
        }
    }, 'Save/load popup standard');

    // Help-text bar at the bottom of the save/load screen. Updates as the
    // user moves the cursor and as the screen state changes (browse vs.
    // confirm-overwrite vs. confirm-load). Includes a "<used>/200" slot
    // count alongside the contextual prompt. No dedup — every state
    // transition re-announces.
    attachRva(0x302E00, {
        onLeave: function() {
            try {
                const msg = readBuf(HELPTEXT_MSG_BUF, 256);
                const count = readBuf(HELPTEXT_COUNT_BUF, 32);
                if (msg.length === 0) return;
                const out = (count.length > 0) ? (msg + ', ' + count) : msg;
                speak(out, true);
                logFile('[SAVE HELP] ' + out);
            } catch (e) {}
        }
    }, 'Save/load help text');

    // TK Yes/No popup cursor handlers. All three TK input handlers toggle
    // DAT_00CE72F0 (the Yes/No cursor) on Up/Down. We share a single
    // onLeave that reads the post-toggle cursor and the cached button
    // labels, announces the focused option. Dedupe via lastYesNoCursor
    // so we only speak when the cursor actually moved (handlers are also
    // called for Confirm and Cancel events, where the cursor doesn't
    // toggle and we shouldn't re-announce).
    //
    // Cursor → button mapping: cursor=1 maps to button 1 (loc 0x204,
    // typically "Yes"), cursor=0 maps to button 2 (loc 0x205, typically
    // "No"). The save state machine sets DAT_00CE72F0 = 1 on popup open
    // (FUN_006EFF60 line 182880) so the popup starts on Yes — which
    // matches user-observed behavior. Earlier code had this mapping
    // reversed and read every cursor-move announce inverted.
    function popupCursorOnLeave() {
        try {
            const cursor = POPUP_YESNO_CURSOR.readU32();
            if (cursor === lastYesNoCursor) return;
            lastYesNoCursor = cursor;
            const label = (cursor === 1)
                ? readBuf(POPUP_BTN_OK_BUF, 64)
                : readBuf(POPUP_BTN_NO_BUF, 64);
            if (label.length === 0) return;
            // Queue the initial focus announce behind the popup body so it
            // doesn't preempt mid-sentence; subsequent toggles interrupt
            // for responsive feedback when the user is navigating.
            const interrupt = !queueNextCursor;
            queueNextCursor = false;
            speak(label, interrupt);
            logFile('[SAVE POPUP CURSOR] ' + cursor + ' = ' + label);
        } catch (e) {}
    }
    attachRva(0x2EFD90, { onLeave: popupCursorOnLeave }, 'Save popup input handler A');
    attachRva(0x2F00E0, { onLeave: popupCursorOnLeave }, 'Save popup input handler B');
    attachRva(0x2F06F0, { onLeave: popupCursorOnLeave }, 'Save popup input handler C');

    logFile('Menus/saveload feature installed');
}
