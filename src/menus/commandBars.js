// Horizontal native command bars store {label, help, width} at +0x58, stride 12.
// This layout belongs to FUN_008E4790 / FUN_008E3CF0, not all TkWindows.
import { attachRva } from '../core/hooks.js';
import { speak, logFile } from '../core/speech.js';
import { menuWindow, enteredOrMoved, textAt } from './nativeText.js';

export function install() {
    attachRva(0x4E3880, {
        onEnter(args) { this.window = args[0]; this.before = menuWindow(this.window); },
        onLeave() {
            const after = menuWindow(this.window);
            if (!enteredOrMoved(this.before, after, 5)) return;
            const label = textAt(this.window.add(0x58 + after.row * 12).readPointer());
            if (label) {
                speak(label, true);
                logFile('[COMMAND BAR] row=' + after.row + ' ' + label);
            }
        }
    }, 'Native horizontal command-bar input');
}
