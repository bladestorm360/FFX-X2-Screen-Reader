// Dialogue / event-message reader.
//
// Two distinct hooks for two distinct paths:
//
// 1) FUN_00776A50 (RVA 0x376A50) — event-message LIST decoder, called by
//    per-frame renderers FUN_0076F580 / FUN_0076F960 walking the message
//    list at DAT_0112A01C. Catches cutscene subtitles and scripted event
//    bundles. Args[0] is the destination decode buffer.
//
// 2) FUN_008B8980 (RVA 0x4B8980) — TkObjectMessage text installer.
//    Reached from the Atel "show dialogue" opcode FUN_008589F0 at
//    decompile L466365 and from MsMessageCue dispatchers. Picks the
//    active-language pointer (param_2 vs param_3) and writes it to
//    slot+0x94, the page-start pointer the renderer FUN_008DEDF0
//    consumes each frame (decompile L555052..555096, L585260). We read
//    slot+0x94 in onLeave (post-install authoritative active pointer)
//    and speak only segs[0] — the first FFX text run before any
//    0x00/0x03/0x0A terminator, i.e. the active page line, NOT all
//    pages concatenated. The earlier reverted attempt joined every
//    segment, which is why the decoded text looked like all four
//    dialogue pages stitched together.
//
// Both feed lastDialogueText for content-change detection. The cutscene
// renderer re-decodes the same line every frame during typewriter
// rendering, so without a content diff we'd send Tolk 60 speak() calls
// per second for the same line. The NPC installer fires multiple times
// per page (focus-loss / unpause re-installs the same active pointer);
// the same diff covers it.

import { decodeFFXSegments } from '../core/charset.js';
import { attachRva } from '../core/hooks.js';
import { speak, logFile } from '../core/speech.js';
import { isValidPtr } from '../core/safeRead.js';
import { cleanText } from '../utils/text.js';
import { tkWinArrayAddr, tkCurArrayAddr } from '../core/addresses.js';
import { state } from '../utils/state.js';

// Help submenu (sceneId 0x0B) drives FUN_00776A50 per-frame for each entry in
// its event-message list, producing speech spam on idle. No event-driven hook
// has been confirmed for this list, so for now the Help submenu is a no-op:
// gate the hook out when sceneId is 0x0B and the user gets silence instead of
// a loop. Cutscenes use different sceneIds and remain unaffected.
const HELP_SCENE_ID = 0x0B;

let lastDialogueText = '';

export function install() {
    attachRva(0x376A50, {
        onEnter: function(args) {
            this.outBuf = args[0];
        },
        onLeave: function() {
            try {
                if (state.sceneId === HELP_SCENE_ID) return;
                if (!isValidPtr(this.outBuf)) return;
                const segs = decodeFFXSegments(this.outBuf, 256, 0);
                if (segs.length === 0) return;
                const text = cleanText(segs.join(' ')).trim();
                if (text.length === 0) return;
                if (text === lastDialogueText) return;
                lastDialogueText = text;
                speak(text, false);
                logFile('[EVENT] ' + text);
            } catch (e) {}
        }
    }, 'Event-message decoder (cutscene subtitle / event bundle)');

    // Vis filter: skip slots with curArray[slot]+1 == 1 (selector-cursor
    // popups — yes/no confirms, save/load, sphere-grid). The menus.js
    // TODrawMessageWindow iterator (RVA 0x4ABCE0) already announces those
    // from slot+0x5C; if we also announce here we'd double-speak.
    // Slots with selector mode 0 are NPC dialogue / non-interactive text
    // that the iterator's `if (vis !== 1) continue` skips — those are
    // ours to read.
    attachRva(0x4B8980, {
        onEnter: function(args) {
            this.slot = args[0].toInt32();
        },
        onLeave: function() {
            try {
                const slot = this.slot;
                const curPtr = tkCurArrayAddr.add(slot * 4).readPointer();
                if (isValidPtr(curPtr)) {
                    const vis = curPtr.add(1).readU8();
                    if (vis === 1) return;
                }
                const winPtr = tkWinArrayAddr.add(slot * 4).readPointer();
                if (!isValidPtr(winPtr)) return;
                const pagePtr = winPtr.add(0x94).readPointer();
                if (!isValidPtr(pagePtr)) return;
                const segs = decodeFFXSegments(pagePtr, 256, 0);
                if (segs.length === 0) return;
                const text = cleanText(segs[0]).trim();
                if (text.length === 0) return;
                if (text === lastDialogueText) return;
                lastDialogueText = text;
                speak(text, false);
                logFile('[NPC] ' + text);
            } catch (e) {}
        }
    }, 'TkObjectMessage text installer (NPC dialogue page advance)');

    logFile('Dialog feature installed');
}
