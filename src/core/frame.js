// Per-frame dispatcher. Two registration APIs:
//   - register(tickCb)       — legacy tick callback, still used by features
//                              that haven't yet migrated off polling.
//   - registerHotkey(vk,...) — preferred. Central keyboard dispatch; the
//                              only form of polling that will survive
//                              Phase 2 migration.
// As each feature migrates off per-frame state polling, it removes its
// register(onTick) call and uses registerHotkey() instead.

import { attachRva } from './hooks.js';
import { keyPressed } from './input.js';
import { logFile } from './speech.js';

const FRAME_RVA = 0x471D10;
const tickCallbacks = [];
const hotkeys = [];
let _count = 0;

export function register(cb) {
    tickCallbacks.push(cb);
}

// vk: virtual-key code (e.g. VK_T). handler: () => void. label: short
// string for error logging (e.g. 'battle.whose-turn').
export function registerHotkey(vk, handler, label) {
    hotkeys.push({ vk, handler, label: label || ('vk0x' + vk.toString(16)) });
}

export function count() {
    return _count;
}

export function start() {
    attachRva(FRAME_RVA, {
        onLeave: function(retval) {
            _count++;
            // Hotkey dispatch — only fires on down-edge (keyPressed semantics).
            for (let i = 0; i < hotkeys.length; i++) {
                const hk = hotkeys[i];
                if (keyPressed(hk.vk)) {
                    try { hk.handler(); }
                    catch (e) { logFile('hotkey[' + hk.label + '] threw: ' + e.message); }
                }
            }
            // Legacy tick callbacks — remaining feature polling. Drained as
            // features migrate to event-driven hooks.
            for (let i = 0; i < tickCallbacks.length; i++) {
                try { tickCallbacks[i](_count); }
                catch (e) { if (_count % 1000 === 0) logFile('tick[' + i + '] error: ' + e.message); }
            }
        }
    }, 'per-frame');
}
