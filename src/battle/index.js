// Battle feature: install() attaches damage/weapon/MsMenuGetText hooks and
// registers a per-frame tick that monitors battle windows, targeting, and
// status effects.

import {
    btlStateAddr, btlCurWinIdxAddr, btlWinArrayAddr,
    playerChrsAddr, CHR_SIZE, CHR_RAM_OFFSET, CHR_IN_BATTLE,
    BTL_WIN_SIZE, CHARACTER_NAMES
} from '../core/addresses.js';
import { attachRva } from '../core/hooks.js';
import { logFile, speak } from '../core/speech.js';
import { register as onFrame, registerHotkey } from '../core/frame.js';
import { VK_T, VK_H } from '../core/input.js';
import { isValidPtr } from '../core/safeRead.js';
import { state } from '../utils/state.js';

import {
    readBtlWindow, readBtlWindowFromPtr, readCommandName, getCommandPartyPreview,
    cacheEquipName, resetEquipNames
} from './windows.js';
import { readPlayerInfo, readMonsterInfo, readPlayerDisplayName } from './entities.js';
import { announceRewardsPhase1, announceRewardsPhase2 } from './rewards.js';
import { install as installStatusHooks, resetStatusCache } from './status.js';
import { install as installActionHooks } from './actions.js';
import { install as installOdSuccess } from './od_success.js';

// Per-battle tick state. (Status diffing moved to battle/status.js —
// event-driven, no per-frame loop needed. Cursor change moved to the
// FUN_0089C870 / FUN_0089CD50 / FUN_0089D580 event hooks in install().)
let prevBtlState = 0;
let prevBtlSelIdx = -1, prevBtlSelCmd = -1;
let inTargeting = false, prevTargetChrs = 0, prevTargetMons = 0;
let targetShowHp = true, targetShowMp = false, lastNonZeroCmdId = 0;

// Speak the highlighted command of `winBase`. Used by the window-open,
// window-pop, and cursor-writer hooks. Interrupt=true so rapid
// Main↔Left↔Right navigation always speaks the LATEST command, not a
// queue of stale ones.
function announceHighlightedCommand(winBase) {
    try {
        if (!btlStateAddr.readU8()) return;
        const win = readBtlWindowFromPtr(winBase);
        if (!win || win.state !== 4 || win.validSelAmt === 0) return;
        if (win.selIdx === prevBtlSelIdx && win.selCmdId === prevBtlSelCmd) return;
        const cmdName = readCommandName(win);
        if (cmdName) speak(cmdName, true);
        prevBtlSelIdx = win.selIdx;
        prevBtlSelCmd = win.selCmdId;
        if (win.selCmdId !== 0) lastNonZeroCmdId = win.selCmdId;
    } catch (e) {}
}

function handleWhoseTurnHotkey() {
    try {
        if (btlStateAddr.readU8() === 0) return;
        const curWinIdx = btlCurWinIdxAddr.readU8();
        const win = readBtlWindow(curWinIdx);
        if (win && win.state !== 0) {
            const chrName = (win.chrId < CHARACTER_NAMES.length)
                ? CHARACTER_NAMES[win.chrId] : ('chr' + win.chrId);
            speak(chrName, true);
        }
    } catch (e) {}
}

function handlePartyHpHotkey() {
    try {
        // Outside battle, playerChrsAddr can point at a stale/freeing
        // struct — iterating 8 slots would AV.
        if (btlStateAddr.readU8() === 0) return;
        const pArr = playerChrsAddr.readPointer();
        if (!isValidPtr(pArr)) return;
        const parts = [];
        for (let i = 0; i < 8; i++) {
            const chrBase = pArr.add(i * CHR_SIZE);
            if (chrBase.add(CHR_IN_BATTLE).readU8() === 0) continue;
            const ram = chrBase.add(CHR_RAM_OFFSET);
            const name = (i < CHARACTER_NAMES.length) ? CHARACTER_NAMES[i] : 'chr' + i;
            parts.push(name + ' HP ' + ram.add(0x1A4).readS32() + '/' + ram.add(0x54).readS32() +
                ' MP ' + ram.add(0x1A8).readS32() + '/' + ram.add(0x58).readS32());
        }
        if (parts.length > 0) speak(parts.join(', '), true);
    } catch (e) {}
}

// Announce a targeting-bitmask change. Called from the FUN_0089D300
// event hook (RVA 0x49D300), which writes both curChrs (+0xD8) and
// curMons (+0xDA) as a single U32 at the current battle window.
function announceBtlTargetingChange(winBase) {
    try {
        if (!btlStateAddr.readU8()) return;
        const curChrs = winBase.add(0xD8).readU16();
        const curMons = winBase.add(0xDA).readU16();
        if (curChrs === 0 && curMons === 0) {
            // Targeting cleared/exited — reset entry state.
            if (inTargeting) { inTargeting = false; prevTargetChrs = 0; prevTargetMons = 0; }
            return;
        }
        if (curChrs === prevTargetChrs && curMons === prevTargetMons) return;

        // First fire of a new target-select session — pick hp/mp preview.
        if (!inTargeting) {
            inTargeting = true;
            targetShowHp = true; targetShowMp = false;
            if (lastNonZeroCmdId > 0) {
                const preview = getCommandPartyPreview(lastNonZeroCmdId);
                if (preview) {
                    targetShowHp = preview.showHp; targetShowMp = preview.showMp;
                    if (!targetShowHp && !targetShowMp) targetShowHp = true;
                }
            }
        }

        const names = [];
        for (let bit = 4; bit < 16; bit++) {
            if (curMons & (1 << bit)) {
                const info = readMonsterInfo(bit - 4);
                if (info) names.push(info.name + ': ' + info.hpPct + '%');
            }
        }
        for (let bit = 0; bit < 18; bit++) {
            if (curChrs & (1 << bit)) {
                const pInfo = readPlayerInfo(bit);
                if (pInfo) {
                    names.push(pInfo.name + ':' + (targetShowMp
                        ? ' MP ' + pInfo.currentMp + '/' + pInfo.maxMp
                        : ' HP ' + pInfo.currentHp + '/' + pInfo.maxHp));
                }
            }
        }
        if (names.length > 0) speak(names.join(', '), false);
        prevTargetChrs = curChrs; prevTargetMons = curMons;
    } catch (e) {}
}

function onTick() {
    // Battle feature has ZERO per-frame game-memory polling now.
    // Just battle-state-byte read for start/end edge detection.
    let btlState = 0;
    try { btlState = btlStateAddr.readU8(); } catch (e) { return; }
    state.inBattle = (btlState !== 0);

    if (btlState !== 0) {
        if (prevBtlState === 0) logFile('[BTL] Battle started');
        prevBtlState = btlState;
    } else {
        if (prevBtlState !== 0) {
            logFile('[BTL] Battle ended');
            state.rewardsAnnounced = false;
            prevBtlState = 0;
            prevBtlSelIdx = -1; prevBtlSelCmd = -1;
            inTargeting = false; lastNonZeroCmdId = 0;
            resetStatusCache();
            resetEquipNames();
        }
    }
}

export function install() {
    // Damage display
    attachRva(0x39FA20, {
        onEnter: function(args) {
            const chrId = args[0].toInt32() & 0xFF;
            const dmgType = args[1].toInt32();
            const value = args[2].toInt32();
            if (value === 0) return;
            const name = readPlayerDisplayName(chrId);
            const absVal = (value < 0) ? -value : value;
            const isHeal = (value < 0);
            const msg = (dmgType === 3)
                ? (isHeal ? name + ': restored ' + absVal + ' MP' : name + ': ' + absVal + ' MP damage')
                : (isHeal ? name + ': healed for ' + absVal : name + ': ' + absVal + ' damage');
            speak(msg, false);
        }
    }, 'Damage display');

    // Equipment name cache
    attachRva(0x3A0C70, {
        onLeave: function(retval) { cacheEquipName(retval); }
    }, 'MsWeaponName');

    // Rewards trigger: when MsMenuGetText is called with arg0===4 at battle end.
    attachRva(0x38FD40, {
        onEnter: function(args) { this.arg0 = args[0].toInt32(); },
        onLeave: function() {
            if (this.arg0 === 4 && !state.rewardsAnnounced) {
                state.rewardsAnnounced = true;
                try { announceRewardsPhase1(speak); announceRewardsPhase2(speak); } catch (e) {}
            }
        }
    }, 'MsMenuGetText (battle rewards)');

    installStatusHooks();
    installActionHooks();
    installOdSuccess();

    // Battle window pop (FUN_0089A5A0 at 0x49A5A0). Fires when a
    // sibling/submenu closes and a previously-open window becomes
    // active again (Cancel back from Black Magic to Main; Left back to
    // Main; Right back to Main; etc.). The cursor writers don't fire on
    // return because the game doesn't re-write the restored window's
    // selIdx. Read the newly-active window and announce its highlighted
    // command.
    attachRva(0x49A5A0, {
        onLeave: function() {
            try {
                if (!btlStateAddr.readU8()) return;
                const newIdx = btlCurWinIdxAddr.readU8();
                if (newIdx > 15) return;
                const newWin = btlWinArrayAddr.add(newIdx * BTL_WIN_SIZE);
                prevBtlSelIdx = -1;
                prevBtlSelCmd = -1;
                announceHighlightedCommand(newWin);
            } catch (e) {}
        }
    }, 'Battle window pop (menu-back)');

    // Battle window opened (FUN_008A0D70 at 0x4A0D70). Called once per
    // window creation by FUN_008997E0 (the window constructor). Main
    // opening = new turn starting — announce the acting character. Any
    // other window opening (Left/Right/Black Magic/etc.) = user just
    // navigated into a new menu — announce its highlighted command.
    attachRva(0x4A0D70, {
        onLeave: function() {
            try {
                if (!btlStateAddr.readU8()) return;
                const newIdx = btlCurWinIdxAddr.readU8();
                if (newIdx > 15) return;
                const winBase = btlWinArrayAddr.add(newIdx * BTL_WIN_SIZE);
                const win = readBtlWindowFromPtr(winBase);
                if (!win) return;
                prevBtlSelIdx = -1;
                prevBtlSelCmd = -1;
                if (win.type === 0x00) {
                    // Main opened = new turn. Announce the acting
                    // character only; the user will hear commands as
                    // they navigate.
                    const chrName = (win.chrId < CHARACTER_NAMES.length)
                        ? CHARACTER_NAMES[win.chrId] : ('chr' + win.chrId);
                    speak(chrName, false);
                } else {
                    announceHighlightedCommand(winBase);
                }
            } catch (e) {}
        }
    }, 'Battle window opened');

    // Battle window cursor-writer hooks. Three per-type input handlers in
    // the decompile all read DAT_025D09D4/D6 (input bitmasks) and update
    // selIdx (window+0x42). Hook each: pass the window pointer to the
    // shared announce helper which detects the actual cursor change.
    function makeCursorHandler() {
        return {
            onEnter: function(args) { this.winBase = args[0]; },
            onLeave: function() {
                // Leaving target-select mode (user pressed Cancel or moved
                // back to the command menu) — reset targeting state.
                if (inTargeting) { inTargeting = false; prevTargetChrs = 0; prevTargetMons = 0; }
                announceHighlightedCommand(this.winBase);
            }
        };
    }
    attachRva(0x49C870, makeCursorHandler(), 'BtlWindow cursor A');
    attachRva(0x49CD50, makeCursorHandler(), 'BtlWindow cursor B');
    attachRva(0x49D580, makeCursorHandler(), 'BtlWindow cursor C');
    // FUN_008a2100 — Change-character menu (type 0x0A). Installed at
    // window+0x9c via FUN_008a0d70 case 10 → FUN_008a8750 wrapper.
    attachRva(0x4A2100, makeCursorHandler(), 'BtlWindow cursor D');
    // FUN_008a2290 — Left menu (type 0x0C). Installed at window+0x9c via
    // FUN_008a0d70 case 0xc → FUN_008a89f0 wrapper. Cancel bit 0x2000.
    attachRva(0x4A2290, makeCursorHandler(), 'BtlWindow cursor E');
    // FUN_008a2630 — Right menu (type 0x0D): Weapon / Armor / Escape.
    // Installed at window+0x9c via FUN_008a0d70 case 0xd → FUN_008a8aa0
    // wrapper. Mirrors FUN_008a2290 (Left); cancel bit is 0x8000 instead
    // of 0x2000. Without this hook, Right-menu cursor moves are silent
    // (the window-open hook's announce is gated on state===4 and fires
    // before the window settles into active state).
    attachRva(0x4A2630, makeCursorHandler(), 'BtlWindow cursor F (Right)');

    // Battle targeting bitmask writer (FUN_0089D300 at 0x49D300). Reads
    // D-pad input, updates target-list cursor in DAT_0133D1BC, then
    // writes the new target bitmask as a U32 at winBase+0xD8 (covers
    // both curChrs +0xD8 and curMons +0xDA in one 4-byte store, line
    // 535241 of the decompile). Fires on every Left/Right target-change.
    attachRva(0x49D300, {
        onEnter: function(args) { this.winBase = args[0]; },
        onLeave: function() { announceBtlTargetingChange(this.winBase); }
    }, 'Battle targeting bitmask');

    registerHotkey(VK_T, handleWhoseTurnHotkey, 'battle.whose-turn');
    registerHotkey(VK_H, handlePartyHpHotkey, 'battle.party-hp');

    onFrame(onTick);
    logFile('Battle feature installed');
}
