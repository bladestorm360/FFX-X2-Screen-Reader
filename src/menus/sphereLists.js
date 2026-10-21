// Sphere Grid uses its own heap-allocated slot pool, not TkWindow.
// Callback identity distinguishes the action menu from other uses of slot 7.
import { rva, base } from '../core/memory.js';
import { attachRva } from '../core/hooks.js';
import { isValidPtr } from '../core/safeRead.js';
import { speak } from '../core/speech.js';
import { textAt, recordName } from './nativeText.js';

const quantity = new NativeFunction(rva(0x390500), 'uint', ['uint'], 'mscdecl');
let inputDepth = 0;
function selected() {
    const pool = rva(0x1686108).readPointer();
    if (!isValidPtr(pool)) return null;
    const slot = pool.add(0x2700).readS16();
    const callback = pool.add(0x2704).readPointer().sub(base).toUInt32();
    if (!((slot === 7 && callback === 0x656330) || (slot === 8 && callback === 0x6560D0))) return null;
    const window = pool.add(slot * 0x340);
    const row = window.add(0x18).readS16(); // full linear index; +1A is scroll only
    const count = window.add(0x1E).readS16();
    if (window.add(0x23).readU8() !== 1 || count <= 0 || count > 64 || row < 0 || row >= count) return null;
    const entry = window.add(0x40 + row * 12);
    return { pool, slot, row, entry, id: slot === 8 ? entry.add(8).readU16() : entry.readPointer().toUInt32() };
}
function identity(focus) { return focus ? [focus.pool, focus.slot, focus.row, focus.id].join(':') : ''; }
function announce(focus) {
    if (!focus) return;
    const name = focus.slot === 8 ? recordName(0x390A40, focus.id) : textAt(focus.entry.readPointer());
    if (name) speak(name + (focus.slot === 8 ? ', ' + quantity(focus.id) : ''), true);
}
export function install() {
    attachRva(0x6583E0, {
        onEnter() { this.before = identity(selected()); this.owner = true; ++inputDepth; },
        onLeave() {
            if (!this.owner) return;
            --inputDepth;
            const after = selected();
            if (identity(after) !== this.before) announce(after);
        }
    }, 'Sphere Grid native submenu input');
    // Constructor events can be nested in input dispatch. The outer event
    // owns that transition, so a constructor cannot produce duplicate speech.
    for (const offset of [0x648E40, 0x659950, 0x6598A0]) {
        attachRva(offset, {
            onEnter() { this.before = identity(selected()); },
            onLeave() {
                const after = selected();
                if (!inputDepth && identity(after) !== this.before) announce(after);
            }
        }, 'Sphere Grid submenu opening ' + offset.toString(16));
    }
}
