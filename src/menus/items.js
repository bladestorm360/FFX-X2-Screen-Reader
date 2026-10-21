// Items / Key Items / field Abilities share an actual two-column input handler.
// FUN_008C7620 and each renderer establish their distinct native list sources.
import { attachRva } from '../core/hooks.js';
import { rva } from '../core/memory.js';
import { speak, logFile } from '../core/speech.js';
import { state } from '../utils/state.js';
import { menuWindow, recordName } from './nativeText.js';

function focused(window) {
    const focus = menuWindow(window);
    if (!focus || focus.depth < 0 || focus.phase !== 2 || focus.row < 0 || focus.row >= focus.count) return null;
    const depth = Math.min(focus.depth, window.add(0x42).readU8() - 1);
    if (depth < 0 || depth > 1) return null;
    const column = window.add(0x58 + depth * 4).readS32();
    if (column < 0 || column > 1) return null;
    const slot = focus.row * 2 + column;
    if (slot < 0 || slot >= 512 || slot >= window.add(0x94).readS32()) return null;
    const type = window.add(0x43).readU8();
    if (type === 1) {
        // Key Items: a dense U16 array, unlike the four-byte item records.
        const id = rva(0x1196F30).add(slot * 2).readU16();
        return { id, slot, type, depth, count: null };
    }
    if ((type !== 0 && type !== 2) || slot >= window.add(0x94).readS32()) return null;
    const entry = rva(0x1197730).add(slot * 4);
    if (entry.add(2).readU8() === 0) return null; // blank native row
    return { id: entry.readU16(), slot, type, depth, count: type === 0 ? entry.add(2).readU8() : null };
}

function identity(focus) {
    return focus ? [focus.type, focus.depth, focus.slot, focus.id, focus.count].join(':') : '';
}

export function install() {
    attachRva(0x4C6DA0, {
        onEnter(args) {
            this.window = args[0];
            this.before = identity(focused(this.window));
        },
        onLeave() {
            if (state.sceneId !== 2 && state.sceneId !== 3) return;
            const after = focused(this.window);
            if (!after || identity(after) === this.before) return;
            const getter = after.type === 1 ? 0x3908F0 : (after.type === 2 ? 0x39A4C0 : 0x390A40);
            const name = recordName(getter, after.id);
            if (!name) { logFile('[ITEMS] native name unavailable slot=' + after.slot + ' type=' + after.type); return; }
            speak(name + (after.count === null ? '' : ', ' + after.count), true);
            logFile('[ITEMS] slot=' + after.slot + ' type=' + after.type + ' id=' + after.id + ' ' + name);
        }
    }, 'Items / Abilities native ordered-list input');
    logFile('Items reader installed: native row/column/count and separate Key Items source');
}
