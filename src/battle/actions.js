// Enemy (and ally) action banner announcement.
//
// Hooks FUN_00898bd0 at RVA 0x498BD0 — the universal action-banner draw
// function. Session 38 probe data (action_text_diff_probe.log) confirmed:
//
//   - FUN_00898bd0 is a 59-byte function that takes a 16-bit cmdId.
//   - It calls MsGetComData (0x39A4C0) to resolve the command's data
//     entry, checks the "has banner" flag (entry[0xe] & 4), and when set,
//     pushes the name text (at entry[0] + base) to display slot 1.
//   - Fires exactly once per action that displays a banner, for ANY
//     actor class (party member, summoned aeon, monster) and ANY command
//     prefix (item 0x2xxx, player ability 0x3xxx, monster ability 0x6xxx).
//   - Probe-confirmed hits: Rikku + Bomb Fragment (slot 6, 0x201a),
//     Lulu + Fire (slot 5, 0x3042), Valefor + Sonic Wings (slot 8,
//     0x30cc). Each case: one call, caller RA 0x498be2, decoded k0 = the
//     exact ability name.
//   - Unrelated to menu rendering. The menu-render callers (0x38f7b9,
//     0x49ac45, 0x4a66cf, 0x4998f3, 0x492075) never route through this
//     function — confirmed by the false-arm noise capture in session 38.
//
// Announce shape:   "<actor name>: <command name>"
// Interrupt mode:   false (queued) — lands before the damage announce.

import { attachRva } from '../core/hooks.js';
import { rva } from '../core/memory.js';
import { logFile, speak } from '../core/speech.js';
import { isValidPtr } from '../core/safeRead.js';
import {
    btlStateAddr, playerChrsAddr, CHR_SIZE
} from '../core/addresses.js';
import { decodeFFXSegments } from '../core/charset.js';
import { readPlayerDisplayName } from './entities.js';

const RVA_BANNER_DRAW    = 0x498BD0;  // FUN_00898bd0
const RVA_MS_GET_COM_DATA = 0x39A4C0; // MsGetComData

const CHR_DF0 = 0xDF0;  // action-state byte. 0=idle, 3=exec, 5=finalize.

// NativeFunction wrapper for MsGetComData so we can resolve the cmdId's
// text the same way FUN_00898bd0 does internally, without having to
// duplicate its excel-bin walk logic.
// FUN_0079a4c0 takes a command identifier and output-base pointer, and returns
// the native command-entry pointer. Keep the binding ABI and these owners aligned.
const msGetComDataFn = new NativeFunction(
    rva(RVA_MS_GET_COM_DATA), 'pointer', ['uint32', 'pointer']
);
const outBaseBuf = Memory.alloc(4);

function resolveBannerText(cmdId) {
    try {
        const entry = msGetComDataFn(cmdId, outBaseBuf);
        if (!isValidPtr(entry)) return null;
        const base = outBaseBuf.readPointer();
        if (!isValidPtr(base)) return null;
        // FUN_00898bd0 reads entry[0] (ushort, the "k0" name offset) as
        // the text offset. See probe data: k0="Fire"/"Bomb Fragment"/
        // "Sonic Wings", k8="Deals fire damage..."/description.
        const nameOffset = entry.readU16();
        const textPtr = base.add(nameOffset);
        const segs = decodeFFXSegments(textPtr, 64, 0);
        return (segs.length > 0) ? segs[0] : null;
    } catch (e) { return null; }
}

// Scan the 31-slot unified actor array for whichever slot is currently
// in action-exec phase (df0=3). Used to associate the cmdId from the
// banner-draw hook with the acting character. Event-driven (only runs
// inside the banner-draw hook, not per frame).
function findExecutingActor() {
    try {
        const arr = playerChrsAddr.readPointer();
        if (!isValidPtr(arr)) return -1;
        for (let i = 0; i < 0x1f; i++) {
            const chrBase = arr.add(i * CHR_SIZE);
            if (chrBase.add(CHR_DF0).readU8() === 3) return i;
        }
    } catch (e) {}
    return -1;
}

export function install() {
    // Hook onLeave so we only announce when the game itself is going to
    // display the banner. FUN_00898bd0 returns 1 when the "has banner"
    // flag is set and FUN_0089db10 was called to push text to display
    // slot 1; returns 0 when the flag is clear and nothing was rendered.
    // Matching the game's own gating keeps us from speaking for
    // commands that don't visibly banner.
    attachRva(RVA_BANNER_DRAW, {
        onEnter: function(args) {
            if (!btlStateAddr.readU8()) { this.skip = true; return; }
            this.cmdId = args[0].toInt32() & 0xFFFF;
        },
        onLeave: function(retval) {
            if (this.skip) return;
            if (this.cmdId === undefined) return;
            if (retval.toInt32() === 0) return;  // banner flag clear
            const slot = findExecutingActor();
            if (slot < 0) return;
            const actor = readPlayerDisplayName(slot);
            const ability = resolveBannerText(this.cmdId);
            if (!actor || !ability) return;
            speak(actor + ': ' + ability, false);
        }
    }, 'Action banner (FUN_00898bd0)');

    logFile('Action banner hook installed');
}
