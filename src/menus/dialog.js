// Dialog / field-menu / TkWindow popup reading. Owns the MsMenuGetText cache
// that the field menu reads from, plus the TkWindow per-frame watcher.

import {
    fieldMenuCursorAddr, fieldMenuPlayerAddr, fieldMenuItemTable,
    tkWinArrayAddr, tkCurArrayAddr, eventIdAddr,
    CHARACTER_NAMES, TITLE_ITEMS, SCENE_NAMES
} from '../core/addresses.js';
import { decodeFFXSegments } from '../core/charset.js';
import { attachRva } from '../core/hooks.js';
import { logFile, speak } from '../core/speech.js';
import { registerHotkey, count as frameCount } from '../core/frame.js';
import { VK_N } from '../core/input.js';
import { isValidPtr } from '../core/safeRead.js';
import { cleanText } from '../utils/text.js';
import { state } from '../utils/state.js';
import { helpTextAddr } from '../core/addresses.js';

export const msMenuTextCache = {};
export const msMenuDescCache = {};

// All TkWindow per-slot state (prev cursor, prev visibility, prev text
// pointer) used to live here for the polling-based diff. Now that every
// change fires via an event hook, the hook itself IS the change signal --
// no diff cache needed. Any remaining cache writes are defensive-only.

// Tracks whether the field menu is currently in its active/navigation
// state. Reset when the fn_ctrl state goes back to 0 (menu closed).
// Used to announce the current cursor on menu-open.
let fieldMenuActive = false;

function readTkWindowContext(w) {
    try {
        const winPtr = tkWinArrayAddr.add(w * 4).readPointer();
        const curPtr = tkCurArrayAddr.add(w * 4).readPointer();
        if (!isValidPtr(winPtr) || !isValidPtr(curPtr)) return null;
        const vis = curPtr.add(1).readU8();
        const idx = curPtr.add(2).readU8();
        const items = curPtr.add(5).readU8();
        const tp = winPtr.add(0x5C).readPointer();
        return { winPtr, curPtr, vis, idx, items, textPtr: tp };
    } catch (e) { return null; }
}

function getTkSegments(ctx) {
    if (!isValidPtr(ctx.textPtr)) return [];
    let tp8C = ptr(0);
    try { tp8C = ctx.winPtr.add(0x8C).readPointer(); } catch (e) {}
    let decodeLen = 0x300;
    if (!tp8C.isNull() && tp8C.compare(ctx.textPtr) > 0) {
        const diff = tp8C.sub(ctx.textPtr).toInt32();
        if (diff > 0 && diff < 0x1000) decodeLen = diff;
    }
    return decodeFFXSegments(ctx.textPtr, decodeLen, 0);
}

function handleHelpHotkey() {
    try {
        const helpSegs = decodeFFXSegments(helpTextAddr, 128, 0);
        if (helpSegs.length > 0 && helpSegs[0].length > 0) { speak(helpSegs.join(' '), true); return; }
        const cursor = fieldMenuCursorAddr.readS32();
        let logical = cursor;
        try { logical = fieldMenuItemTable.add(cursor * 8).readS32(); } catch (e) {}
        if (msMenuDescCache[logical]) speak(msMenuDescCache[logical], true);
    } catch (e) {}
}

// Speak the currently-selected item for slot `w` based on the cursor index.
// Used by both the cursor-change event hook (FUN_008B6B20) and the
// window-becoming-visible branch of the polling loop.
function announceTkCursorItem(w, ctx) {
    const eventId = eventIdAddr.readS32();
    if (eventId === 0x17 && w === 2) {
        speak(ctx.idx < TITLE_ITEMS.length ? TITLE_ITEMS[ctx.idx] : '?', true);
        return;
    }
    const segs = getTkSegments(ctx);
    const firstItem = segs.length - ctx.items;
    const target = firstItem + ctx.idx;
    if (target >= 0 && target < segs.length) speak(cleanText(segs[target]), true);
}

// Announce a TkWindow becoming visible. Used by the FUN_008B5F60 hook.
function announceTkWindowAppeared(w, ctx) {
    const eventId = eventIdAddr.readS32();
    if (eventId === 0x17 && w === 2) {
        speak(ctx.idx < TITLE_ITEMS.length ? TITLE_ITEMS[ctx.idx] : '?', true);
    } else {
        const segs = getTkSegments(ctx);
        if (segs.length > 0) speak(cleanText(segs[0]), true);
        if (ctx.idx + 1 < segs.length) speak(cleanText(segs[ctx.idx + 1]), false);
    }
}

// onTick removed entirely — all TkWindow events (cursor, visibility 0→1,
// textPtr update) are now event-driven via hooks registered in install().
// Field menu polling was removed earlier. Nothing left for this feature
// to poll per frame.

export function install() {
    // MsMenuGetText cache + mainMenuLastFrame tracking (arg0===9 is field menu).
    // Decoding happens ONLY when arg0 === 9; other call paths (rewards arg0=4,
    // config arg0=7, etc.) never touched the decoder originally and routing
    // their retvals through it was a crash risk.
    attachRva(0x38FD40, {
        onEnter: function(args) {
            this.arg0 = args[0].toInt32();
            this.arg1 = args[1].toInt32();
        },
        onLeave: function(retval) {
            if (this.arg0 !== 9) return;
            state.mainMenuLastFrame = frameCount();
            if (!isValidPtr(retval)) return;
            const segs = decodeFFXSegments(retval, 256, 0);
            if (segs && segs.length > 0) msMenuTextCache[this.arg1] = segs[0];
            if (segs && segs.length > 2 && segs[1] === '-') msMenuDescCache[this.arg1] = segs[2];
        }
    }, 'MsMenuGetText (field menu cache)');

    // Scene activation -- internal mod state only, NOT a user-facing
    // announcement. The actual menu content (item names, character
    // names, field menu cursor) speaks from the TkWindow / fn_ctrl
    // hooks a moment later. Scene numbers ("Equip", "Aeons", "scene
    // 0x07") are developer-facing debug info, not useful to the player.
    //
    // sceneId 0x01 = field menu activating; we reset fieldMenuActive so
    // the next fn_ctrl fire takes the open-announce branch (the
    // fn_ctrl state doesn't return to 0 on close so we need a separate
    // open signal).
    attachRva(0x4AA0B0, {
        onEnter: function(args) {
            const sceneId = args[0].toInt32();
            state.sceneId = sceneId;
            const sceneName = SCENE_NAMES[sceneId] || ('scene 0x' + sceneId.toString(16));
            logFile('[SCENE] ' + sceneName);
            if (sceneId === 0x01) fieldMenuActive = false;
        }
    }, 'Scene activate');

    // Field menu fn_ctrl (FUN_008E0340). State machine for the in-field
    // pause menu. Reads input via FUN_008BE440/480, updates
    // fieldMenuCursorAddr (DAT_01871508) on Up/Down, and
    // fieldMenuPlayerAddr (DAT_0187150C) on player switch.
    //
    // State at param_1+0x1C drives the flow:
    //   0    = idle / closed / pre-init
    //   1-2  = opening (init frames)
    //   3    = navigation active (cursor up/down processed)
    //   4+   = sub-state (confirm, submenu enter, etc.)
    //
    // We use the 0 -> active transition to announce the initial cursor
    // item when the menu first opens, regardless of whether the cursor
    // value actually changed from its last session. Without this the
    // mod is silent when opening the menu (there's no in-game sound
    // cue for it either).
    attachRva(0x4E0340, {
        onEnter: function(args) {
            this.param1 = args[0];
            try {
                this.cursorBefore = fieldMenuCursorAddr.readS32();
                this.playerBefore = fieldMenuPlayerAddr.readS32();
            } catch (e) {
                this.cursorBefore = -999;
                this.playerBefore = -999;
            }
        },
        onLeave: function() {
            try {
                if (!isValidPtr(this.param1)) return;
                const stateAfter = this.param1.add(0x1C).readS32();

                // State 0 = menu closed/reset. Arm fieldMenuActive so the
                // next non-zero state transition counts as "just opened".
                if (stateAfter === 0) { fieldMenuActive = false; return; }

                const cursorAfter = fieldMenuCursorAddr.readS32();
                const playerAfter = fieldMenuPlayerAddr.readS32();

                // First non-zero state after a 0 -- menu just opened.
                // Announce the current item so the user knows the menu
                // is up and where the cursor sits.
                if (!fieldMenuActive) {
                    fieldMenuActive = true;
                    let logicalIdx = cursorAfter;
                    try { logicalIdx = fieldMenuItemTable.add(cursorAfter * 8).readS32(); } catch (e) {}
                    const itemName = msMenuTextCache[logicalIdx] || ('item ' + logicalIdx);
                    speak(itemName, true);
                    return;
                }

                // Cursor move announcement (normal nav).
                if (cursorAfter !== this.cursorBefore && this.cursorBefore !== -999) {
                    let logicalIdx = cursorAfter;
                    try { logicalIdx = fieldMenuItemTable.add(cursorAfter * 8).readS32(); } catch (e) {}
                    const itemName = msMenuTextCache[logicalIdx] || ('item ' + logicalIdx);
                    speak(itemName, true);
                }
                if (playerAfter !== this.playerBefore && this.playerBefore !== -999 &&
                    this.playerBefore !== -1 && playerAfter !== -1) {
                    const playerName = (playerAfter >= 0 && playerAfter < CHARACTER_NAMES.length)
                        ? CHARACTER_NAMES[playerAfter] : ('player ' + playerAfter);
                    speak(playerName, true);
                }
            } catch (e) {}
        }
    }, 'Field menu fn_ctrl');

    // TkWindow render/update (FUN_008B5F60). Runs every frame for active
    // windows and does layout work; line 552719 of the decompile writes
    // tkCurArrayAddr[slot]+1 = 1 — but ONLY inside an `if (vis == 0)`
    // guard, so the actual visibility transition fires exactly once. We
    // capture the vis byte on entry and only announce when it flipped.
    attachRva(0x4B5F60, {
        onEnter: function(args) {
            this.slot = args[0].toInt32();
            this.visBefore = 1; // default = assume already visible (no announce)
            try {
                const slotPtr = tkCurArrayAddr.add(this.slot * 4).readPointer();
                if (isValidPtr(slotPtr)) this.visBefore = slotPtr.add(1).readU8();
            } catch (e) {}
        },
        onLeave: function() {
            if (this.visBefore !== 0) return; // was already visible, no transition
            try {
                const ctx = readTkWindowContext(this.slot);
                if (ctx && ctx.vis === 1) announceTkWindowAppeared(this.slot, ctx);
            } catch (e) {}
        }
    }, 'TkWindow visibility setter');

    // TkWindow textPtr setter (FUN_008B8980). param_1 = slot, param_2 =
    // new text pointer. Called whenever the window's text buffer is
    // (re-)assigned. This fires on:
    //   - Items menu opening (TkWindow exists, content shifts to items)
    //   - Character-select mode activating (content shifts to character list)
    //   - Popup appearing inside an existing window (e.g. Use/Key
    //     Items/Sort command bar on Back from items)
    //   - Dialog text advancing to next page
    //
    // Announcement pattern matches the old universal_menu_battle_reader
    // behavior: decode all segments, speak the instruction/header segments
    // (everything before the item list -- segs.length - ctx.items items),
    // then speak the currently-focused list item via announceTkCursorItem.
    // Announcing the header alone left Items / character-select effectively
    // silent because the user needs to know what IS currently focused, not
    // just the menu's title.
    attachRva(0x4B8980, {
        onEnter: function(args) {
            this.slot = args[0].toInt32();
            this.tpBefore = ptr(0);
            try {
                const slotWinPtr = tkWinArrayAddr.add(this.slot * 4).readPointer();
                if (isValidPtr(slotWinPtr)) this.tpBefore = slotWinPtr.add(0x5C).readPointer();
            } catch (e) {}
        },
        onLeave: function() {
            try {
                const slotWinPtr = tkWinArrayAddr.add(this.slot * 4).readPointer();
                if (!isValidPtr(slotWinPtr)) return;
                const tpAfter = slotWinPtr.add(0x5C).readPointer();
                if (tpAfter.equals(this.tpBefore)) return;
                const ctx = readTkWindowContext(this.slot);
                if (!ctx || ctx.vis !== 1) return;
                const segs = getTkSegments(ctx);
                if (segs.length === 0) return;

                // Header / instruction segments come first; the list items
                // are the last `ctx.items` segments. Speak the header
                // pieces concatenated (e.g. "Use  Key Items  Sort").
                const numInstr = Math.max(0, segs.length - ctx.items);
                const instrParts = [];
                for (let i = 0; i < numInstr; i++) {
                    const s = cleanText(segs[i]);
                    if (s.length > 1) instrParts.push(s);
                }
                if (instrParts.length > 0) speak(instrParts.join(' '), true);

                // Then announce the currently-focused item in the list.
                announceTkCursorItem(this.slot, ctx);
            } catch (e) {}
        }
    }, 'TkWindow textPtr setter');

    // TkWindow per-slot dispatcher (FUN_008B6B20). Iterates active slots
    // each frame; calls each slot's +0xFC vtable method (the input handler).
    // Cursor-change probe (session 18 follow-up) confirmed this fires on
    // title-screen Down with slot=2 and the cursor byte updated.
    // Hooking here catches cursor moves regardless of which specific
    // input handler the slot has registered (FUN_008B6260, FUN_008B6490,
    // or others — the title slot uses something other than FUN_008B6260).
    attachRva(0x4B6B20, {
        onEnter: function(args) {
            this.slot = args[0].toInt32();
            this.cursorBefore = -1;
            try {
                const slotPtr = tkCurArrayAddr.add(this.slot * 4).readPointer();
                if (isValidPtr(slotPtr)) {
                    this.cursorBefore = slotPtr.add(2).readU8();
                }
            } catch (e) {}
        },
        onLeave: function() {
            if (this.cursorBefore < 0) return;
            try {
                const slotPtr = tkCurArrayAddr.add(this.slot * 4).readPointer();
                if (!isValidPtr(slotPtr)) return;
                const cursorAfter = slotPtr.add(2).readU8();
                if (cursorAfter === this.cursorBefore) return;
                // Cursor changed — announce the new selection.
                const ctx = readTkWindowContext(this.slot);
                if (ctx && ctx.vis === 1) {
                    announceTkCursorItem(this.slot, ctx);
                }
            } catch (e) {}
        }
    }, 'TkWindow cursor dispatcher');

    registerHotkey(VK_N, handleHelpHotkey, 'menus.help');

    // No onFrame callback — all dialog/TkWindow/field-menu state changes
    // are event-driven now.
    logFile('Menus/dialog feature installed');
}
