// Status screen review buffer. The status fn_ctrl is a state machine at
// RVA 0x4D1DD0; it fires every frame the Status screen is displayed.
// We track visibility from that hook and serve a navigable buffer of
// {label, value} rows for the currently-displayed character via two
// hotkeys (`,` previous row, `.` next row). Each press re-reads PlySave
// so cycling characters via the game's L/R picks up immediately.
//
// PlySave slot offsets (slot start = saveDataAddr + 0x55CC + chrIdx*0x94)
// were lifted from the published Big CE Table for FFX HD Remaster
// (mod source: nexusmods.com/finalfantasyxx2hdremaster/mods/184) and
// cross-checked against the existing +0x3B Sphere LVL field in
// src/menus/config.js. Field map:
//   +0x04 u32  Base.HP
//   +0x08 u32  Base.MP
//   +0x14 u32  Total AP
//   +0x18 u32  AP till next level
//   +0x1C u32  Curr.HP
//   +0x20 u32  Curr.MP
//   +0x24 u32  Max.HP
//   +0x28 u32  Max.MP
//   +0x2D u8   Equipped weapon ID
//   +0x2E u8   Equipped armor ID
//   +0x2F..36  Effective stats (Str, Def, Mag, MDef, Agi, Lck, Eva, Acc)
//   +0x38 u8   Overdrive mode (index into OVERDRIVE_MODE_NAMES)
//   +0x39 u8   Overdrive current bar
//   +0x3A u8   Overdrive max bar
//   +0x3B u8   Sphere LVL available (also used by config.js)
//
// Equipment names are NOT resolved in v1. Each piece of equipped gear
// has a custom name ("Sonic Steel", "Yellow Shield") that's resolved
// through MsWeaponName (RVA 0x3A0C70), but calling that function safely
// from JS requires param-decoding work that isn't done yet — for now
// the buffer announces the byte ID. Tracking under a separate item.

import { saveDataAddr, CHARACTER_NAMES, fieldMenuPlayerAddr } from '../core/addresses.js';
import { speak, logFile } from '../core/speech.js';
import { attachRva } from '../core/hooks.js';
import { registerHotkey } from '../core/frame.js';

const VK_OEM_COMMA = 0xBC;   // ,
const VK_OEM_PERIOD = 0xBE;  // .

const PLYSAVE_BASE_OFF = 0x55CC;
const PLYSAVE_STRIDE = 0x94;

// Mode-byte → name. Order verified from the published Big CE Table
// (entry IDs 149..165, contiguous in the per-mode "uses left to learn"
// counter array immediately after the OD bar bytes).
const OVERDRIVE_MODE_NAMES = [
    'Warrior', 'Comrade', 'Stoic', 'Healer', 'Tactition', 'Victim',
    'Dancer', 'Avenger', 'Slayer', 'Hero', 'Rook', 'Victor',
    'Coward', 'Ally', 'Sufferer', 'Daredevil', 'Loner'
];

let statusVisible = false;
let bufferRows = [];
let bufferIdx = 0;
let bufferChrIdx = -1;

function chrSlot(chrIdx) {
    return saveDataAddr.add(PLYSAVE_BASE_OFF + chrIdx * PLYSAVE_STRIDE);
}

function getCurrentChrIdx() {
    try {
        const v = fieldMenuPlayerAddr.readS32();
        if (v >= 0 && v < CHARACTER_NAMES.length) return v;
    } catch (e) {}
    return 0;  // sane default when field menu hasn't seeded a player yet
}

// Build the buffer for chrIdx by reading all of the character's PlySave
// slot. Order matches what's visible on the Status screen top-to-bottom:
// HP, MP, primary 4 stats, Overdrive mode, weapon, armor, secondary 4
// stats, sphere LVL / next AP.
function buildBuffer(chrIdx) {
    const rows = [];
    try {
        const slot = chrSlot(chrIdx);
        const name = (chrIdx < CHARACTER_NAMES.length)
            ? CHARACTER_NAMES[chrIdx] : ('chr' + chrIdx);
        rows.push({ label: 'Character', value: name });

        const curHp = slot.add(0x1C).readU32();
        const maxHp = slot.add(0x24).readU32();
        rows.push({ label: 'HP', value: curHp + ' of ' + maxHp });

        const curMp = slot.add(0x20).readU32();
        const maxMp = slot.add(0x28).readU32();
        rows.push({ label: 'MP', value: curMp + ' of ' + maxMp });

        // Effective stats (base + sphere-grid bonus). Status screen shows
        // these, not the +0x0C..0x13 base-only values.
        rows.push({ label: 'Strength',      value: slot.add(0x2F).readU8() });
        rows.push({ label: 'Defense',       value: slot.add(0x30).readU8() });
        rows.push({ label: 'Magic',         value: slot.add(0x31).readU8() });
        rows.push({ label: 'Magic Defense', value: slot.add(0x32).readU8() });

        const odMode = slot.add(0x38).readU8();
        const odModeName = (odMode < OVERDRIVE_MODE_NAMES.length)
            ? OVERDRIVE_MODE_NAMES[odMode] : ('mode ' + odMode);
        const odCur = slot.add(0x39).readU8();
        const odMax = slot.add(0x3A).readU8();
        rows.push({ label: 'Overdrive', value: odModeName + ', ' + odCur + ' of ' + odMax });

        // Equipment names not resolved yet — announce raw byte IDs so the
        // user at least gets distinct values for distinct equipped gear.
        rows.push({ label: 'Weapon ID', value: slot.add(0x2D).readU8() });
        rows.push({ label: 'Armor ID',  value: slot.add(0x2E).readU8() });

        rows.push({ label: 'Agility',  value: slot.add(0x33).readU8() });
        rows.push({ label: 'Luck',     value: slot.add(0x34).readU8() });
        rows.push({ label: 'Evasion',  value: slot.add(0x35).readU8() });
        rows.push({ label: 'Accuracy', value: slot.add(0x36).readU8() });

        const sphereLvl = slot.add(0x3B).readU8();
        const apNext = slot.add(0x18).readU32();
        rows.push({ label: 'Sphere Level', value: sphereLvl });
        rows.push({ label: 'AP to next level', value: apNext });
    } catch (e) {
        logFile('[STATUS] buildBuffer chrIdx=' + chrIdx + ' threw: ' + e.message);
    }
    return rows;
}

function ensureFreshBuffer() {
    const chrIdx = getCurrentChrIdx();
    if (chrIdx !== bufferChrIdx) {
        bufferRows = buildBuffer(chrIdx);
        bufferChrIdx = chrIdx;
        bufferIdx = 0;
    } else if (bufferRows.length === 0) {
        bufferRows = buildBuffer(chrIdx);
    }
}

function speakRow() {
    if (bufferRows.length === 0) return;
    const r = bufferRows[bufferIdx];
    speak(r.label + ': ' + r.value, true);
}

function hotkeyPrev() {
    if (!statusVisible) return;
    ensureFreshBuffer();
    if (bufferRows.length === 0) return;
    bufferIdx = (bufferIdx - 1 + bufferRows.length) % bufferRows.length;
    speakRow();
}

function hotkeyNext() {
    if (!statusVisible) return;
    ensureFreshBuffer();
    if (bufferRows.length === 0) return;
    bufferIdx = (bufferIdx + 1) % bufferRows.length;
    speakRow();
}

export function install() {
    // Status fn_ctrl (FUN_008D1DD0). Called every frame the Status screen
    // is displayed. Setting statusVisible=true on each call gates the
    // hotkeys so they don't speak when the user is elsewhere; setting it
    // false on case 0x13 (the close-state branch in the decompile) lets
    // the user dismiss the screen via the game's own input and have the
    // hotkeys fall silent without us having to poll.
    attachRva(0x4D1DD0, {
        onEnter: function(args) { this.param1 = args[0]; },
        onLeave: function() {
            try {
                const stateAfter = this.param1.add(0x1C).readS32();
                // 0x13 is the terminal state in the fn_ctrl switch — sets
                // the "finished" flag and tears down. Anything else means
                // the screen is still active.
                if (stateAfter === 0x13) {
                    if (statusVisible) {
                        statusVisible = false;
                        bufferRows = [];
                        bufferChrIdx = -1;
                        bufferIdx = 0;
                        logFile('[STATUS] Closed');
                    }
                    return;
                }
                if (!statusVisible) {
                    statusVisible = true;
                    logFile('[STATUS] Opened (chrIdx=' + getCurrentChrIdx() + ')');
                }
            } catch (e) {}
        }
    }, 'Status fn_ctrl');

    registerHotkey(VK_OEM_COMMA,  hotkeyPrev, 'status.prev-row');
    registerHotkey(VK_OEM_PERIOD, hotkeyNext, 'status.next-row');
    logFile('Status review buffer installed (hotkeys: , and .)');
}
