// Status-effect change announcer — event-driven replacement for the
// per-frame 20×3 status diff that used to live in battle/index.js onTick.
//
// Hooks:
//   FUN_0078F0B0 (RVA 0x38F0B0) — bulk status sync per character turn.
//     Writes the new 0x606 and 0x616 values onto the target character,
//     then the game itself computes bit-level transitions and calls
//     specialized handlers for KO/petrify/etc. Our hook fires after the
//     write and diffs against our cache to announce changes.
//   FUN_0079A1A0 (RVA 0x39A1A0) — clear-all statuses (battle end / KO
//     reset). Writes 0 to both 0x606 and 0x616. Our hook clears the cache.
//
// Both functions resolve the target char struct via FUN_00794030(charId)
// where charId is an 8-bit identifier. We mirror that resolution so we
// can read the updated status fields without re-entering a hooked fn.

import { attachRva } from '../core/hooks.js';
import { rva } from '../core/memory.js';
import { logFile, speak } from '../core/speech.js';
import { isValidPtr } from '../core/safeRead.js';
import { PERM_STATUS_NAMES, EXTRA_STATUS_NAMES } from '../core/addresses.js';
import { readPlayerDisplayName } from './entities.js';

const RVA_GET_CHAR_STRUCT = 0x394030; // FUN_00794030 — char-id to struct ptr
const fnGetChar = new NativeFunction(rva(RVA_GET_CHAR_STRUCT), 'pointer', ['uint32']);

// Per-character last-known status values. Keyed by charId. A character's
// entry is cleared on battle end (see FUN_0079A1A0 hook) and on KO-cure.
const lastPerm = {};
const lastExtra = {};
const lastDeath = {};

function getChrStruct(charId) {
    try {
        const p = fnGetChar(charId);
        return isValidPtr(p) ? p : null;
    } catch (e) { return null; }
}

function announceDiff(charId) {
    const chr = getChrStruct(charId);
    if (!chr) return;
    try {
        const permNow = chr.add(0x606).readU16();
        const extraNow = chr.add(0x616).readU16();
        const deathNow = chr.add(0xDCC).readU8();
        const permOld = (lastPerm[charId] !== undefined) ? lastPerm[charId] : permNow;
        const extraOld = (lastExtra[charId] !== undefined) ? lastExtra[charId] : extraNow;
        const deathOld = (lastDeath[charId] !== undefined) ? lastDeath[charId] : deathNow;

        if (deathNow !== 0 && deathOld === 0) {
            speak(readPlayerDisplayName(charId) + ': KO', false);
        }
        lastDeath[charId] = deathNow;

        const permDiff = permNow ^ permOld;
        for (let bit = 1; bit < PERM_STATUS_NAMES.length; bit++) {
            if (permDiff & (1 << bit)) {
                speak(readPlayerDisplayName(charId) + ': ' + PERM_STATUS_NAMES[bit] +
                    ((permNow & (1 << bit)) ? '' : ' cured'), false);
            }
        }
        lastPerm[charId] = permNow;

        const extraDiff = extraNow ^ extraOld;
        for (let bit = 0; bit < EXTRA_STATUS_NAMES.length; bit++) {
            if ((extraDiff & (1 << bit)) && EXTRA_STATUS_NAMES[bit] !== '') {
                speak(readPlayerDisplayName(charId) + ': ' + EXTRA_STATUS_NAMES[bit] +
                    ((extraNow & (1 << bit)) ? '' : ' cured'), false);
            }
        }
        lastExtra[charId] = extraNow;
    } catch (e) {}
}

// Reset state on battle end (called from battle/index.js).
export function resetStatusCache() {
    for (const k in lastPerm) delete lastPerm[k];
    for (const k in lastExtra) delete lastExtra[k];
    for (const k in lastDeath) delete lastDeath[k];
}

export function install() {
    // Bulk status-sync: target chrId is param_3 (args[2]).
    attachRva(0x38F0B0, {
        onEnter: function(args) { this.chrId = args[2].toInt32() & 0xFF; },
        onLeave: function() { announceDiff(this.chrId); }
    }, 'StatusApply');

    // Clear-all-statuses: target chrId is param_1 (args[0]).
    attachRva(0x39A1A0, {
        onEnter: function(args) { this.chrId = args[0].toInt32() & 0xFF; },
        onLeave: function() {
            // Game has just zeroed 0x606/0x616. Clear our cache for this
            // char so the next real change is announced from a clean slate.
            delete lastPerm[this.chrId];
            delete lastExtra[this.chrId];
            delete lastDeath[this.chrId];
        }
    }, 'StatusClear');

    logFile('Status-effect hooks installed');
}
