// Each list is keyed by its native renderer and ordered source. Input callbacks
// own the event; no draw-loop harvesting or universal TkWindow text assumption.
import { rva, base } from '../core/memory.js';
import { attachRva } from '../core/hooks.js';
import { speak, logFile } from '../core/speech.js';
import { menuWindow, recordName, gearName, nativeText } from './nativeText.js';
import { state } from '../utils/state.js';

function selection(window) {
    const focus = menuWindow(window);
    if (!focus || focus.depth < 0 || focus.row < 0 || focus.row >= focus.count) return null;
    const renderer = focus.renderer.sub(base).toUInt32();
    let slot = focus.row;
    // Overdrive stores the full linear index at +0x48 (bit 0 is column).
    if (slot < 0 || slot >= 512) return null;
    if (renderer === 0x4D6830) {
        const table = rva(0x146A9F8).readPointer();
        if (table.isNull()) return null;
        return { ...focus, renderer, slot, id: table.add(slot * 2).readU16(), kind: 'gear' };
    }
    if (![0x4CF9C0, 0x4D16C0, 0x4CDB70, 0x4D5F30].includes(renderer)) return null;
    const entry = rva(0x1197730).add(slot * 4);
    if (renderer === 0x4D16C0 && entry.add(3).readU8() === 0) return null;
    return { ...focus, renderer, slot, id: entry.readU16(), kind: entry.add(2).readU8(), flags: entry.add(3).readU8() };
}

function key(selected) {
    if (!selected) return '';
    // +0x28 is the input handler's activation state; preserve an initial
    // announcement when a window returns from a nested menu with the same row.
    const phase = selected.renderer === 0x4D16C0 && selected.phase === 3 ? 2 : selected.phase;
    return [selected.renderer, phase, selected.depth, selected.slot, selected.id, selected.kind].join(':');
}

function name(selected) {
    if (selected.kind === 'gear' || selected.renderer === 0x4CF9C0) return gearName(selected.id);
    if (selected.renderer === 0x4D16C0 && selected.kind === 2)
        return nativeText(0x473AF0, selected.id); // native row-name routing
    if (selected.renderer === 0x4D16C0 && selected.kind === 3)
        return nativeText(0x4DDFA0, selected.id + 0x1033); // native blank/fallback row
    return recordName(0x39A4C0, selected.id);
}

function attachInput(offset) {
    attachRva(offset, {
        onEnter(args) { this.window = args[0]; this.before = key(selection(this.window)); },
        onLeave() {
            // These sources are field-menu lists; shared battle callbacks have
            // their own producers and must not acquire a second speech owner.
            if (![4, 7, 9, 0x15].includes(state.sceneId)) return;
            const after = selection(this.window);
            if (!after || !((after.phase === 2) || (after.renderer === 0x4D16C0 && after.phase === 3)) || key(after) === this.before) return;
            const text = name(after);
            if (text) {
                speak(text, true);
                logFile('[FIELD LIST] renderer=' + after.renderer.toString(16) + ' slot=' + after.slot + ' ' + text);
            }
        }
    }, 'Ordered native field-list input ' + offset.toString(16));
}

export function install() {
    // The wrappers delegate to this cursor writer, so it is the single owner
    // for Equip, Aeon Learn, and Customize equipment/recipe list speech.
    attachInput(0x4B4460);
    // Overdrive wraps a distinct two-column input state machine.
    attachInput(0x4D0E80);
}
