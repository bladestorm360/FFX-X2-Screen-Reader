// ESC menu (FFEscMenu) cursor-change reader.
//
// escmenu.swf has no AS3 — UI logic and cursor state live in the C++
// FFEscMenu class. Page-builder FUN_0068E870 resolves all loc strings
// at game init and pushes them to IggyMenu (Flash storage); on cursor
// move, Flash dispatches "EscMenuHelpTextRefresh" back to C++, which
// matches a (sVar1, sVar2) tag pair against a 35-entry table at
// DAT_00C33DB8 and schedules FUN_0068E6E0 with a pointer to the
// matched help-text loc id. FUN_0068E6E0 calls FUN_006414A0 to resolve
// the help text and writes it to *DAT_00CCC868 + 0x121.
//
// We hook FUN_0068E6E0 directly: fires once per cursor move with the
// loc id. We re-resolve via FUN_006414A0 ourselves — same localized
// text the game writes, no hardcoded strings.
//
// This announces the cursor-focused item's HELP TEXT, which the game
// pairs with each option (e.g., on "Resume" the help text is
// "Return to the game"). Announcing the help text is sufficient for
// the user to know what's focused; the actual button label would
// require mapping the (sVar1, sVar2) tag pair to a page+index, which
// can't be derived from the decompile alone (the mapping lives in
// escmenu.swf's timeline data, not AS3).

import { rva } from '../core/memory.js';
import { attachRva } from '../core/hooks.js';
import { speak, logFile } from '../core/speech.js';
import { isValidPtr } from '../core/safeRead.js';
import { cleanText } from '../utils/text.js';

const RVA_LOC_RESOLVE = 0x2414A0;   // FUN_006414A0(loc_id, char* buf, int bufsz) __cdecl
const RVA_HELP_RESOLVER = 0x28E6E0; // FUN_0068E6E0(int* loc_id_ptr) __cdecl

let resolveLoc = null;
let scratchBuf = null;
// Pre-built zero buffer used to wipe scratchBuf before each resolveLoc call.
// FUN_006DB320 does NOT guarantee a null at the new shorter string length —
// without zeroing first, readCString walks past the new content into bytes
// left over from the previous (longer) item. e.g. after "Change System Settings"
// then "Resume Game", scratchBuf reads back as "Resume Gamem Settings".
const ZERO_256 = new Uint8Array(256);

function lazyInit() {
    if (resolveLoc) return true;
    try {
        resolveLoc = new NativeFunction(rva(RVA_LOC_RESOLVE),
            'void', ['int', 'pointer', 'int'], 'mscdecl');
        scratchBuf = Memory.alloc(256);
        return true;
    } catch (e) {
        logFile('[ESCMENU] lazyInit failed: ' + e.message);
        return false;
    }
}

export function install() {
    attachRva(RVA_HELP_RESOLVER, {
        onEnter: function(args) {
            try {
                if (!lazyInit()) return;
                const ptr = args[0];
                if (!isValidPtr(ptr)) return;
                const locId = ptr.readU32();
                if (locId === 0 || locId > 0xFFFF) return;
                scratchBuf.writeByteArray(ZERO_256.buffer);
                resolveLoc(locId, scratchBuf, 256);
                let raw = '';
                try { raw = scratchBuf.readCString(256) || ''; } catch (e) {}
                const text = cleanText(raw).trim();
                if (text.length === 0) return;
                speak(text, true);
                // Log (sVar1, sVar2) tags too — they're stable Flash IDs;
                // logging them now lets a future session correlate tag
                // pair → page/item index from real-play logs without
                // running a probe.
                try {
                    const sVar1 = ptr.add(4).readS16();
                    const sVar2 = ptr.add(6).readS16();
                    logFile('[ESCMENU] loc=0x' + locId.toString(16) +
                        ' (sVar1=0x' + (sVar1 & 0xFFFF).toString(16) +
                        ', sVar2=0x' + (sVar2 & 0xFFFF).toString(16) +
                        ') ' + text);
                } catch (e) {
                    logFile('[ESCMENU] loc=0x' + locId.toString(16) + ' ' + text);
                }
            } catch (e) {
                logFile('[ESCMENU] hook err: ' + e.message);
            }
        }
    }, 'FFEscMenu help-text resolver (cursor-move announce)');

    logFile('Esc-menu feature installed');
}
