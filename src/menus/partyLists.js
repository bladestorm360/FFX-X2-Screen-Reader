// Formation lives in the main controller, not scene 9 (Customize).
// These new readers are Frida prototypes until their native migration gate.
import { rva } from '../core/memory.js';
import { attachRva } from '../core/hooks.js';
import { speak } from '../core/speech.js';
import { nativeText, menuText, menuWindow, enteredOrMoved } from './nativeText.js';
import { state } from '../utils/state.js';
import { increaseRequirement } from './repairs.js';

const partyCharacter = new NativeFunction(rva(0x4A9AB0), 'uint8', ['int'], 'mscdecl');
const aeonCharacter = new NativeFunction(rva(0x4A9830), 'uint8', [], 'mscdecl');

function formation(menu) {
    const phase = menu.add(0x1C).readS32();
    if (phase !== 0x0E && phase !== 0x14) return null;
    const slot = rva(phase === 0x0E ? 0x147151C : 0x1471520).readS32();
    if (slot < 0 || slot >= 8) return null;
    return { phase, slot, character: partyCharacter(slot) };
}
function identity(selected) {
    return selected ? [selected.phase, selected.slot, selected.character].join(':') : '';
}

export function install() {
    // Separate event states from the main-list native producer; no duplicate
    // reader owns the same selection when the native vertical slice is active.
    attachRva(0x4E0340, {
        onEnter(args) { this.menu = args[0]; this.before = identity(formation(this.menu)); },
        onLeave() {
            const after = formation(this.menu);
            if (after && identity(after) !== this.before) {
                const name = nativeText(0x385070, after.character);
                if (name) speak(name, true);
            }
        }
    }, 'Formation ordered-party input');

    attachRva(0x4CCEA0, {
        onEnter(args) {
            this.window = args[0];
            this.phase = this.window.add(0x28).readS32();
            this.character = aeonCharacter();
        },
        onLeave() {
            if (state.sceneId !== 7 || this.window.add(0x28).readS32() !== 3) return;
            const character = aeonCharacter();
            if (this.phase !== 3 || this.character !== character) {
                const name = nativeText(0x385070, character);
                if (name) speak(name, true);
            }
        }
    }, 'Aeon native-character selection');

    attachRva(0x4CD6A0, {
        onEnter(args) { this.window = args[0]; this.before = menuWindow(this.window); },
        onLeave() {
            if (state.sceneId !== 7) return;
            const after = menuWindow(this.window);
            if (!enteredOrMoved(this.before, after) || after.row >= 10) return;
            // Native stat renderer 4CE590 uses namespace C and table+4.
            // Namespace B/table+8 is the later confirmation, not this row.
            const id = rva(0x85A60C).add(after.row * 0x1C).readS32();
            const label = menuText(12, id);
            if (label) speak(label, true);
            increaseRequirement(after.row);
        }
    }, 'Aeon Increase native stat selection');
}
