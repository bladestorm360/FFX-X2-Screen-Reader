// Save/Load screen: hook slot population + cursor focus change.
// Ghidra refs: FUN_00702e70 (slot populate), FUN_00702db0 (cursor focus).

import { attachRva } from '../core/hooks.js';
import { speak, logFile } from '../core/speech.js';
import { readCString } from '../utils/text.js';

const saveSlots = {};
let prevSaveSlot = -1;

export function install() {
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

    logFile('Menus/saveload feature installed');
}
