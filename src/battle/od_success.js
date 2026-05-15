// Overdrive minigame auto-success — hotkey-toggled feature that forces the
// in-overdrive input minigame to succeed for the three characters whose
// state machines are character-and-mechanism-confirmed from decompile RE:
//
//   Tidus   Swordplay (meter-timing, bar in window) — RVA 0x492290
//   Auron   Bushido   (button-sequence matching)    — RVA 0x490EE0
//   Lulu    Fury      (joystick rotation count)     — RVA 0x491AF0
//
// Each handler is a state machine that ticks once per frame. Case 4 is the
// "input listening" state; case 5 is the "success animation" state in the
// binary-flag handlers (Tidus / Auron). Case 6 is the "time's up, finalize
// counter" state in Lulu's handler.
//
// Strategy: hook each function's onEnter. When the toggle is on AND the
// state register reads 4 (input is being collected), write the success
// values directly:
//   Tidus  — DAT_0133D6B3 = 1, DAT_0133D6F4 = 5  (skip ahead to success)
//   Auron  — DAT_0133D6B2 = 1, DAT_0133D6B4 = 5  (skip ahead to success)
//   Lulu   — DAT_0133D6DF = 0x10, DAT_0133D6C8 = 6  (max spell count, skip
//             to time-up branch which dispatches FUN_007b0480 with the
//             maxed counter)
//
// The state-register check gates the write to the input-listening phase
// only — once we've forced state=5/6 the next frame's onEnter sees a
// different state and skips the write, so we don't loop. State register
// returns to 0 between OD activations (case 10 / case 7 resets), so each
// new OD use re-arms the write.
//
// Wakka's Slots is deliberately NOT covered: it's multi-outcome (Element
// Reels match-3-fire ≠ match-3-ice; Status Reels different status per
// match) and requires a freeze + speak-list + user-pick + resume UX
// that's not in scope here. Wakka's handler RVA is also still unidentified.
//
// All RVAs verified by reading the function bodies in
// Docs/Scripts/Ghidra/output/FFX/decompile/layer2_game_logic.txt
// (lines 527797-527866 Auron, 528395-528550 Lulu, 528714-528795 Tidus).
// chrIdx mapping confirmed via FUN_007b0480 calls at end of each handler.

import { rva } from '../core/memory.js';
import { logFile, speak } from '../core/speech.js';
import { attachRva } from '../core/hooks.js';
import { registerHotkey } from '../core/frame.js';
import { VK_O } from '../core/input.js';

const TIDUS_FN_RVA  = 0x492290;
const AURON_FN_RVA  = 0x490EE0;
const LULU_FN_RVA   = 0x491AF0;

const TIDUS_SUCCESS_RVA = 0xF3D6B3;
const TIDUS_STATE_RVA   = 0xF3D6F4;
const AURON_SUCCESS_RVA = 0xF3D6B2;
const AURON_STATE_RVA   = 0xF3D6B4;
const LULU_COUNTER_RVA  = 0xF3D6DF;
const LULU_STATE_RVA    = 0xF3D6C8;

const LULU_COUNTER_MAX = 0x10;

// Default ON — a blind player has no way to play the OD minigames manually,
// so the feature is only useful when active. The O hotkey toggles it off if
// the player ever wants the native minigame back.
let forceEnabled = true;

// State-transition logging per character. The first session shipping this
// feature ran with default OFF and produced zero [OD-SUCCESS] log lines, so
// we have no evidence whether hooks fire, what states they pass through, or
// whether state 4 is actually reached. Log on transition (not per call) to
// capture the lifecycle without spamming — the fn_ctrls run per-frame for
// the duration of the minigame.
let lastTidusState = -1, lastAuronState = -1, lastLuluState = -1;

function forceTidus() {
    let stateVal;
    try { stateVal = rva(TIDUS_STATE_RVA).readU8(); } catch (e) { return; }
    if (stateVal !== lastTidusState) {
        logFile('[OD-DIAG] Tidus state ' + lastTidusState + ' -> ' + stateVal);
        lastTidusState = stateVal;
    }
    if (!forceEnabled) return;
    if (stateVal !== 4) return;
    try {
        rva(TIDUS_SUCCESS_RVA).writeU8(1);
        rva(TIDUS_STATE_RVA).writeU8(5);
        logFile('[OD-SUCCESS] Tidus Swordplay: forced success (state 4 -> 5)');
    } catch (e) {
        logFile('[OD-SUCCESS] Tidus write threw: ' + e.message);
    }
}

function forceAuron() {
    let stateVal;
    try { stateVal = rva(AURON_STATE_RVA).readU8(); } catch (e) { return; }
    if (stateVal !== lastAuronState) {
        logFile('[OD-DIAG] Auron state ' + lastAuronState + ' -> ' + stateVal);
        lastAuronState = stateVal;
    }
    if (!forceEnabled) return;
    if (stateVal !== 4) return;
    try {
        rva(AURON_SUCCESS_RVA).writeU8(1);
        rva(AURON_STATE_RVA).writeU8(5);
        logFile('[OD-SUCCESS] Auron Bushido: forced success (state 4 -> 5)');
    } catch (e) {
        logFile('[OD-SUCCESS] Auron write threw: ' + e.message);
    }
}

function forceLulu() {
    let stateVal;
    try { stateVal = rva(LULU_STATE_RVA).readU8(); } catch (e) { return; }
    if (stateVal !== lastLuluState) {
        logFile('[OD-DIAG] Lulu state ' + lastLuluState + ' -> ' + stateVal);
        lastLuluState = stateVal;
    }
    if (!forceEnabled) return;
    if (stateVal !== 4) return;
    try {
        rva(LULU_COUNTER_RVA).writeU8(LULU_COUNTER_MAX);
        rva(LULU_STATE_RVA).writeU8(6);
        logFile('[OD-SUCCESS] Lulu Fury: counter -> 0x10, state 4 -> 6');
    } catch (e) {
        logFile('[OD-SUCCESS] Lulu write threw: ' + e.message);
    }
}

function toggleHotkey() {
    forceEnabled = !forceEnabled;
    if (forceEnabled) {
        speak('Overdrive auto-success on, covering Tidus, Auron, Lulu', true);
    } else {
        speak('Overdrive auto-success off', true);
    }
    logFile('[OD-SUCCESS] Toggle -> ' + (forceEnabled ? 'ON' : 'OFF'));
}

export function install() {
    attachRva(TIDUS_FN_RVA, { onEnter: forceTidus }, 'Tidus Swordplay fn_ctrl');
    attachRva(AURON_FN_RVA, { onEnter: forceAuron }, 'Auron Bushido fn_ctrl');
    attachRva(LULU_FN_RVA,  { onEnter: forceLulu  }, 'Lulu Fury fn_ctrl');
    registerHotkey(VK_O, toggleHotkey, 'battle.od-success-toggle');
    logFile('Overdrive minigame auto-success installed (hotkey: O, default ON)');
}
