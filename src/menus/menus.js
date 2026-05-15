// Field menu, TkWindow popups, and the MsMenuGetText cache that backs them.
// NPC dialogue + cutscene subtitle reading lives in ./dialog.js.
//
// === HELPERS PORTED FROM universal_menu_battle_reader.js ===
// `decodeFFXSegments`, `cleanText`, `readTkWindowContext`, `getTkSegments`,
// `getTkFocusedItem` are local byte-for-byte copies of universal's
// equivalents (lines 105-222). Notably `decodeFFXSegments` differs from
// `core/charset.js`'s — production's adds many 2-byte opcodes (0x06-0x2A);
// universal treats only 0x09/0x10 as 2-byte. Keeping universal's local
// here for exact-port fidelity in the TkWindow path.
//
// === TkWindow LOOP — anchor needs investigation ===
// Universal hooked 0x471D10 (the per-frame input poll FUN_00871d10) —
// not because that function has any semantic relationship to the Items
// submenu, but because it's a pollable per-frame entry point. Universal
// was "intercepting by accident", same shape as the ESC menu situation
// before production found FUN_0068E6E0 as its proper hook.
//
// Production currently anchors at 0x4ABCE0 (TODrawMessageWindow). That
// catches yes/no popups, save/load, ESC menu, and the field menu's own
// TkWindow slot — but NOT Items. Items' actual rendering / cursor hook
// is unknown.
//
// TO FIND IT: run `field_submenu_probe.js` (v2). Its scene-activate hook
// reads scene_table[sceneId]+0x04 to log fn_ctrl RVAs for any submenu
// opened. Items' sceneId is 0x00; opening the menu logs
//   [ACTIVATE] sceneId=0x0 (Items) fn_ctrl_rva=0xXXXX *** DISCOVERY ***
// That RVA gets added as a second `attachRva` here, alongside the
// existing 0x4ABCE0 iterator.

import {
    fieldMenuCursorAddr, fieldMenuPlayerAddr, fieldMenuItemTable,
    tkWinArrayAddr, tkCurArrayAddr, eventIdAddr,
    CHARACTER_NAMES, TITLE_ITEMS, SCENE_NAMES, helpTextAddr
} from '../core/addresses.js';
import { attachRva } from '../core/hooks.js';
import { rva } from '../core/memory.js';
import { resolveCommandText } from '../core/charset.js';
import { logFile, speak } from '../core/speech.js';
import { registerHotkey, count as frameCount } from '../core/frame.js';
import { VK_N } from '../core/input.js';
import { state } from '../utils/state.js';

export const msMenuTextCache = {};
export const msMenuDescCache = {};
// Field-menu Items namespace (MsMenuGetText arg0=8). Game pre-fetches
// item names for visible slots when the Items menu opens, then again on
// page changes. Caching them here lets the Items hook resolve cursor-
// to-name without re-calling the resolver. arg1 is the inventory slot
// index (matches the +0x48 cursor for a dense inventory).
export const itemsNameCache = {};

let fieldMenuActive = false;

// === LOCAL HELPERS (verbatim port from universal_menu_battle_reader.js) ===

const CHARSET = '0123456789 !"#$%&\'()*+,-./:;<=>?ABCDEFGHIJKLMNOPQRSTUVWXYZ[\\]^_‘abcdefghijklmnopqrstuvwxyz{|}~';

// Universal's decoder. NOT the same as core/charset.js — see header.
function decodeFFXSegments(addr, maxLen, maxOps) {
    var segments = [];
    var current = '';
    var opCount = 0;
    try {
        var bytes = new Uint8Array(addr.readByteArray(maxLen));
        var i = 0;
        while (i < maxLen) {
            if (maxOps > 0 && opCount >= maxOps) break;
            var b = bytes[i];
            if (b === 0x00) { if (current.length > 0) segments.push(current.trim()); current = ''; i++; opCount++; continue; }
            if (b === 0x03) { if (current.length > 0) segments.push(current.trim()); current = ''; i++; opCount++; continue; }
            if (b === 0x0A) { if (current.length > 0) segments.push(current.trim()); current = ''; i++; opCount++; continue; }
            if (b === 0x09 || b === 0x10) { i += 2; opCount++; continue; }
            if (b < 0x2B) { i++; opCount++; continue; }
            var idx = 0;
            if (b < 0x30) { idx = (b - 0x2B) * 0xD0; i++; if (i >= maxLen) break; b = bytes[i]; }
            idx += (b - 0x30);
            if (idx >= 0 && idx < CHARSET.length) current += CHARSET[idx];
            i++;
            opCount++;
        }
        if (current.length > 0) segments.push(current.trim());
    } catch(e) {}
    return segments.filter(function(s) { return s.length > 0 && s.replace(/\s/g, '').length > 0; });
}

function cleanText(s) {
    while (s.length > 1 && /^[^A-Za-z0-9 ]/.test(s[0])) s = s.substring(1);
    if (s.length > 1 && /^[0-9][A-Za-z]/.test(s)) s = s.substring(1);
    return s.trim();
}

function readTkWindowContext(w) {
    var winPtr = tkWinArrayAddr.add(w * 4).readPointer();
    var curPtr = tkCurArrayAddr.add(w * 4).readPointer();
    if (winPtr.isNull() || curPtr.isNull()) return null;
    var vis = curPtr.add(1).readU8();
    var idx = curPtr.add(2).readU8();
    var items = curPtr.add(5).readU8();
    var tp = winPtr.add(0x5C).readPointer();
    return {winPtr: winPtr, curPtr: curPtr, vis: vis, idx: idx, items: items, textPtr: tp};
}

function getTkSegments(ctx) {
    if (ctx.textPtr.isNull() || ctx.textPtr.compare(ptr(0x10000)) <= 0) return [];
    var tp8C = ptr(0);
    try { tp8C = ctx.winPtr.add(0x8C).readPointer(); } catch(e) {}
    var decodeLen = 0x300;
    if (!tp8C.isNull() && tp8C.compare(ctx.textPtr) > 0) {
        var diff = tp8C.sub(ctx.textPtr).toInt32();
        if (diff > 0 && diff < 0x1000) decodeLen = diff;
    }
    return decodeFFXSegments(ctx.textPtr, decodeLen, 0);
}

function getTkFocusedItem(ctx, segments) {
    var firstItem = segments.length - ctx.items;
    var target = firstItem + ctx.idx;
    if (target >= 0 && target < segments.length) return cleanText(segments[target]);
    return '?';
}

// === STATE (verbatim port — names match universal so the port is grep-able) ===

var prevTkVisible = {};
var prevTkTextPtrs = {};
var prevTkCursors = {};
var initialTkTextPtrs = {};
var isTkPopup = {};

// === Help hotkey (production-specific) ===

function handleHelpHotkey() {
    try {
        var helpSegs = decodeFFXSegments(helpTextAddr, 128, 0);
        if (helpSegs.length > 0 && helpSegs[0].length > 0) { speak(helpSegs.join(' '), true); return; }
        var cursor = fieldMenuCursorAddr.readS32();
        var logical = cursor;
        try { logical = fieldMenuItemTable.add(cursor * 8).readS32(); } catch (e) {}
        if (msMenuDescCache[logical]) speak(msMenuDescCache[logical], true);
    } catch (e) {}
}

export function install() {
    // Items-menu-just-opened flag, shared between scene-activate hook
    // and Items fn_ctrl hook below. Set true when sceneId 0x3 activates;
    // cleared by Items fn_ctrl after first announce.
    var itemsJustOpened = false;

    // ── MsMenuGetText cache (port from universal:746-808) ──
    // arg0=9 — main menu names + descriptions (cached for field menu cursor reads).
    attachRva(0x38FD40, {
        onEnter: function(args) {
            this.arg0 = args[0].toInt32();
            this.arg1 = args[1].toInt32();
            this.arg2 = args[2].toInt32();
        },
        onLeave: function(retval) {
            if (retval.isNull()) return;
            var segs = null;
            try { segs = decodeFFXSegments(retval, 256, 0); } catch (e) {}
            if (this.arg0 === 9) {
                state.mainMenuLastFrame = frameCount();
                if (segs && segs.length > 0) {
                    msMenuTextCache[this.arg1] = segs[0];
                    if (segs.length > 2 && segs[1] === '-') {
                        msMenuDescCache[this.arg1] = segs[2];
                    }
                }
            } else if (this.arg0 === 8) {
                // arg0=8 namespace: confirmed via test session as the
                // ITEMS COMMAND BAR (Use / Sort / Key Items / ...), NOT
                // the items list itself. Caching for diagnostic only.
                if (segs && segs.length > 0) itemsNameCache[this.arg1] = segs[0];
            }
            // While Items menu is active (sceneId 0x3), log return text
            // for ALL arg0 values not yet identified — helps find the
            // namespace for items list / key items list names.
            if (state.sceneId === 0x3 && this.arg0 !== 9) {
                var preview = '';
                if (segs && segs.length > 0) {
                    preview = JSON.stringify(segs.slice(0, 4));
                }
                logFile('[MsMG_TEXT arg0=' + this.arg0 +
                    ' arg1=' + this.arg1 +
                    ' arg2=' + this.arg2 +
                    ' segs=' + preview + ']');
            } else {
                logFile('[MsMenuGetText] args=(' + this.arg0 + ',' + this.arg1 + ',' + this.arg2 + ')');
            }
        }
    }, 'MsMenuGetText cache');

    // ── Scene activate (port from universal:813-826) ──
    // Universal speaks "[SUBMENU] sceneName" on every non-0x01 scene
    // activation. Production previously logged but did not speak —
    // missing the submenu announce. Now matches universal.
    attachRva(0x4AA0B0, {
        onEnter: function(args) {
            var sceneId = args[0].toInt32();
            state.sceneId = sceneId;
            var sceneName = SCENE_NAMES[sceneId] || ('scene 0x' + sceneId.toString(16));
            // Internal state only — never speak the scene name. The
            // per-menu fn_ctrl hooks (Items, Abilities, Aeons, ...) own
            // their own user-facing announcements. An earlier port of
            // this hook from universal's `logBoth('[SUBMENU] ...')`
            // (diagnostic dual-output) into production `speak()` was
            // wrong — universal logs that line to console for the dev,
            // not to a screen reader. The result was the player hearing
            // "scene 0x3" because addresses.js's SCENE_NAMES table is
            // missing the Items mapping and the fallback path produced
            // a literal scene-id string.
            if (sceneId === 0x01) fieldMenuActive = false;
            // Items has no character-select stage — sceneId 0x3 activates
            // directly into the items list. Set a flag so the Items fn_ctrl
            // hook announces the focused item on its first call after
            // activation.
            if (sceneId === 0x03) itemsJustOpened = true;
            logFile('[SCENE ACTIVATE] scene=0x' + sceneId.toString(16) + ' (' + sceneName + ')');
        }
    }, 'Scene activate');

    // ── Field menu fn_ctrl (production-specific, not in universal) ──
    // Universal detects field-menu cursor moves via MsMenuGetText activity
    // (PART 2, lines 1633-1673). Production uses the fn_ctrl hook
    // directly — strictly more reliable, and confirmed working by user.
    // Keeping this hook is a deliberate deviation from "exact port" for
    // engineering quality (fn_ctrl is event-driven, universal's timer
    // is heuristic).
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
                if (this.param1.isNull()) return;
                var stateAfter = this.param1.add(0x1C).readS32();
                if (stateAfter === 0) { fieldMenuActive = false; return; }

                var cursorAfter = fieldMenuCursorAddr.readS32();
                var playerAfter = fieldMenuPlayerAddr.readS32();

                if (!fieldMenuActive) {
                    fieldMenuActive = true;
                    var logicalIdx = cursorAfter;
                    try { logicalIdx = fieldMenuItemTable.add(cursorAfter * 8).readS32(); } catch (e) {}
                    var itemName = msMenuTextCache[logicalIdx] || ('item ' + logicalIdx);
                    speak(itemName, true);
                    return;
                }

                if (cursorAfter !== this.cursorBefore && this.cursorBefore !== -999) {
                    var logicalIdx2 = cursorAfter;
                    try { logicalIdx2 = fieldMenuItemTable.add(cursorAfter * 8).readS32(); } catch (e) {}
                    var itemName2 = msMenuTextCache[logicalIdx2] || ('item ' + logicalIdx2);
                    speak(itemName2, true);
                }
                if (playerAfter !== this.playerBefore && this.playerBefore !== -999 &&
                    this.playerBefore !== -1 && playerAfter !== -1) {
                    var playerName = (playerAfter >= 0 && playerAfter < CHARACTER_NAMES.length)
                        ? CHARACTER_NAMES[playerAfter] : ('player ' + playerAfter);
                    speak(playerName, true);
                }
            } catch (e) {}
        }
    }, 'Field menu fn_ctrl');

    // ── TkWindow per-frame iterator ──
    //
    // ANCHOR RVA IS PROVISIONAL. Universal hooked 0x471D10 (the input-poll
    // function FUN_00871d10) because it's a pollable per-frame entry point;
    // that's universal "intercepting by accident" — it had no specific
    // semantic relationship to the Items submenu. The right approach is
    // to find the SPECIFIC function that handles Items rendering / cursor
    // (analogous to ESC menu's FUN_0068E6E0 help-text resolver).
    //
    // For now we anchor on 0x4ABCE0 (TODrawMessageWindow) — this is what
    // production used previously; it does NOT catch Items but it DOES
    // catch yes/no popups, save/load, ESC menu, and the field-menu's
    // own TkWindow slot, so removing it would regress those features.
    //
    // TO FIND ITEMS' CORRECT HOOK: the v2 field_submenu_probe has a
    // scene-activate hook that reads scene_table[sceneId] -> +0x04 for
    // fn_ctrl_rva. Items' sceneId is 0x00; opening the Items menu while
    // the probe is running will log
    //   [ACTIVATE] sceneId=0x0 (Items) fn_ctrl_rva=0xXXXX (UNKNOWN) *** DISCOVERY ***
    // That XXXX is the RVA we should add as a separate attachRva.
    attachRva(0x4ABCE0, {
        onLeave: function() {
            try {
                var eventId = eventIdAddr.readS32();

                for (var w = 0; w < 8; w++) {
                    try {
                        var ctx = readTkWindowContext(w);
                        if (!ctx || ctx.vis === 0) {
                            prevTkVisible['w' + w] = 0;
                            continue;
                        }
                        var key = 'w' + w;
                        if (prevTkCursors[key] === undefined) prevTkCursors[key] = ctx.idx;

                        var wasVis = prevTkVisible[key] || 0;
                        var oldTp = prevTkTextPtrs[key] || ptr(0);
                        var tpChanged = !ctx.textPtr.equals(oldTp) && !oldTp.isNull();
                        prevTkTextPtrs[key] = ctx.textPtr;

                        if (ctx.vis === 1 && wasVis === 0) {
                            initialTkTextPtrs[key] = ctx.textPtr;
                            isTkPopup[key] = false;
                        }
                        if (tpChanged && initialTkTextPtrs[key]) {
                            isTkPopup[key] = !ctx.textPtr.equals(initialTkTextPtrs[key]);
                        }

                        // Menu open (vis 0→1)
                        if (ctx.vis === 1 && wasVis === 0) {
                            if (eventId === 0x17 && w === 2) {
                                speak('Title Screen', true);
                                speak(ctx.idx < TITLE_ITEMS.length ? TITLE_ITEMS[ctx.idx] : '?', true);
                            } else {
                                var segs = getTkSegments(ctx);
                                logFile('[WIN' + w + ' OPEN] ' + JSON.stringify(segs));
                                if (segs.length > 0) speak(cleanText(segs[0]), true);
                                if (ctx.idx + 1 < segs.length) speak(cleanText(segs[ctx.idx + 1]), true);
                            }
                            prevTkCursors[key] = ctx.idx;
                        }
                        // Popup open (text pointer shifted)
                        else if (tpChanged) {
                            var segs2 = getTkSegments(ctx);
                            logFile('[WIN' + w + ' POPUP] ' + JSON.stringify(segs2));
                            if (segs2.length > 0) {
                                var numInstr = segs2.length - ctx.items;
                                if (numInstr < 0) numInstr = 0;
                                var instr = '';
                                for (var si = 0; si < numInstr; si++) {
                                    var cl = cleanText(segs2[si]);
                                    if (cl.length > 1) instr += (instr.length > 0 ? ' ' : '') + cl;
                                }
                                if (instr.length > 0) speak(instr, true);
                                speak(getTkFocusedItem(ctx, segs2), true);
                            }
                            prevTkCursors[key] = ctx.idx;
                        }
                        prevTkVisible[key] = ctx.vis;

                        // Cursor change
                        if (ctx.idx !== prevTkCursors[key]) {
                            var focusedText = '';
                            if (eventId === 0x17 && w === 2) {
                                focusedText = ctx.idx < TITLE_ITEMS.length ? TITLE_ITEMS[ctx.idx] : '?';
                            } else {
                                var segs3 = getTkSegments(ctx);
                                logFile('[WIN' + w + ' cursor] ' + JSON.stringify(segs3));
                                if (isTkPopup[key]) {
                                    focusedText = getTkFocusedItem(ctx, segs3);
                                } else {
                                    focusedText = (ctx.idx + 1 < segs3.length) ? cleanText(segs3[ctx.idx + 1]) : '?';
                                }
                            }
                            speak(focusedText, true);
                            prevTkCursors[key] = ctx.idx;
                        }
                    } catch (e) {}
                }
            } catch (e) {}
        }
    }, 'TkWindow per-frame iterator');

    // ── Items submenu fn_ctrl (FUN_008c5420, RVA 0x4C5420) ──
    //
    // Items has no character-select stage — it activates directly as
    // sceneId 0x3 (confirmed via field_submenu_probe v3 session 60). The
    // fn_ctrl is a switch-on-state-at-`param_1+0x1C` machine in the same
    // shape as Abilities/Overdrive/Aeons but the arch doc had the wrong
    // RVA listed (the 0.0 confidence column). The probe identified it by
    // dumping all 6 scene-struct offsets and matching `+0x00` to known
    // submenu fn_ctrls.
    //
    // Items routes through TWO TkWindow globals. Both are standard
    // TkWindow structs with cursor at +0x48, num_items at +0x30, and
    // textPtr at +0x5C — same layout as the 8-slot pool universal
    // iterates, so the same getTkSegments + getTkFocusedItem helpers
    // resolve their text:
    //   *DAT_0186a354 (RVA 0x146A354) — items-list TkWindow (the actual
    //     inventory list once you've picked Use or Important Items).
    //     Same global as Abilities — they share an "active submenu
    //     TkWindow" pointer that gets repointed on submenu activation.
    //   *DAT_023cc124 (RVA 0x1FCC124) — command-bar TkWindow (Use /
    //     Important Items / etc., the menu shown FIRST when you press
    //     Items in the field menu).
    //
    // Both can have their cursor change in this fn_ctrl's lifetime. The
    // state machine determines which is focused at any given frame, but
    // we don't need to track that — we read both, diff against last-
    // seen, and speak whichever moved. They shouldn't both move in the
    // same frame.
    var prevItemsCmdCursor = -1;
    var prevItemsListCursor = -1;
    var prevItemsSlotIdx = -999;
    var prevItemsState = -999;
    var itemsHookCount = 0;
    // Diagnostic field-signature cache for the right-column-silence bug.
    var prevItemsFieldsSig = '';
    var prevItemsSlotSig = '';
    // Note: itemsJustOpened was declared at the top of install() so the
    // scene-activate hook (registered earlier) can write to it. The flag
    // signals "Items menu just activated — announce the focused item on
    // the next fn_ctrl call". Items has no char-select stage so we get
    // no separate submenu-name announce; we use this flag instead.
    function readTkWindowCursor(globalRva) {
        try {
            var winPtr = rva(globalRva).readPointer();
            if (winPtr.isNull()) return -1;
            return winPtr.add(0x48).readU16();
        } catch (e) { return -1; }
    }
    function readTkWindowText(globalRva, idx) {
        try {
            var winPtr = rva(globalRva).readPointer();
            if (winPtr.isNull()) return null;
            var ctx = {
                winPtr: winPtr,
                textPtr: winPtr.add(0x5C).readPointer(),
                idx: idx,
                items: winPtr.add(0x30).readU16()
            };
            return ctx;
        } catch (e) { return null; }
    }
    attachRva(0x4C5420, {
        onEnter: function(args) {
            this.param1 = args[0];
            // Frame transition: snapshot the previous frame's complete
            // query list, reset the accumulator. fn_ctrl runs before
            // fn_render in the per-frame dispatch order, so by this
            // moment the *previous* frame's queries are complete.
            itemsLastFrameQueries = itemsCurrentFrameQueries;
            itemsCurrentFrameQueries = [];
        },
        onLeave: function() {
            try {
                itemsHookCount++;
                var stateNow = -999;
                try { stateNow = this.param1.add(0x1C).readS32(); } catch (e) {}
                var cmdNow = readTkWindowCursor(0x1FCC124);
                var listNow = readTkWindowCursor(0x146A354);

                // DIAGNOSTIC: log first call + every 600 calls (~10 sec) + every state change
                if (itemsHookCount === 1 || itemsHookCount % 600 === 0 || stateNow !== prevItemsState) {
                    logFile('[ITEMS_DIAG count=' + itemsHookCount +
                        ' state=' + prevItemsState + '->' + stateNow +
                        ' cmd=' + cmdNow + ' list=' + listNow + ']');
                    prevItemsState = stateNow;
                }

                // DIAGNOSTIC: dump 32-bit values at each 4-byte offset of
                // DAT_0186a354 for the first call only. Looking for the
                // cmdListPtr equivalent — TkWindow's analog of
                // BtlWindow.cmdListPtr (+0x20). It's a pointer into the
                // items-list array; itemId at cmdListPtr+cursor*2.
                if (itemsHookCount === 1) {
                    try {
                        var lwInit = rva(0x146A354).readPointer();
                        if (!lwInit.isNull()) {
                            var dump = '';
                            for (var off32 = 0; off32 <= 0xA0; off32 += 4) {
                                var v = lwInit.add(off32).readU32();
                                dump += ' +0x' + off32.toString(16) + '=0x' + v.toString(16);
                            }
                            logFile('[ITEMS_STRUCT_DUMP at ' + lwInit + ':' + dump + ']');
                        }
                    } catch (e) { logFile('[ITEMS_STRUCT_DUMP error: ' + e.message + ']'); }
                }
                try {
                    var listWin = rva(0x146A354).readPointer();
                    var win358 = rva(0x146A358).readPointer();   // DAT_0186a358 secondary
                    var win33c = rva(0x146A33C).readPointer();   // DAT_0186a33c tertiary
                    var sig = '';
                    if (!listWin.isNull()) {
                        // 0x40-0x60: covers cursor area, num_items, plus surrounding
                        for (var off = 0x40; off < 0x60; off += 2) {
                            sig += ' L' + off.toString(16) + '=' + listWin.add(off).readS16();
                        }
                        // 0x2c-0x36: also covers nearby fields
                        for (var off2 = 0x2C; off2 < 0x38; off2 += 2) {
                            sig += ' L' + off2.toString(16) + '=' + listWin.add(off2).readS16();
                        }
                    }
                    if (!win358.isNull()) {
                        // Only sample 0x40-0x4A on secondary
                        for (var off3 = 0x40; off3 < 0x4C; off3 += 2) {
                            sig += ' B' + off3.toString(16) + '=' + win358.add(off3).readS16();
                        }
                    }
                    if (!win33c.isNull()) {
                        for (var off4 = 0x40; off4 < 0x4C; off4 += 2) {
                            sig += ' C' + off4.toString(16) + '=' + win33c.add(off4).readS16();
                        }
                    }
                    if (sig !== prevItemsFieldsSig) {
                        logFile('[ITEMS_FIELDS' + sig + ']');
                        prevItemsFieldsSig = sig;
                    }
                } catch (e) {}

                // 8-slot pool diagnostic — universal's TkWindow loop
                // catches the items list via this pool, so the items list
                // cursor is in one of these 8 slots. Log on any change.
                try {
                    var slotSig = '';
                    for (var s = 0; s < 8; s++) {
                        var sw = tkWinArrayAddr.add(s * 4).readPointer();
                        var sc = tkCurArrayAddr.add(s * 4).readPointer();
                        if (sw.isNull() || sc.isNull()) continue;
                        var svis = sc.add(1).readU8();
                        var sidx = sc.add(2).readU8();
                        var sitems = sc.add(5).readU8();
                        if (svis !== 0) {
                            slotSig += ' s' + s + '(vis=' + svis + ' idx=' + sidx + ' n=' + sitems + ')';
                        }
                    }
                    if (slotSig !== prevItemsSlotSig) {
                        logFile('[ITEMS_SLOTS' + slotSig + ']');
                        prevItemsSlotSig = slotSig;
                    }
                } catch (e) {}

                // Items list announce: pure decompile pattern.
                //   FUN_008c5420 case 0x8c5489 (line 565415):
                //     iVar9 = *(int*)(TkWindow+0x58) + *(short*)(TkWindow+0x48) * 2;
                //     itemId = (&DAT_01597730)[iVar9 * 2];   // ushort[]
                //              = *(ushort*)(0x1197730 + iVar9*4)
                //   And FUN_008c66e0 (line 566203) is the explicit getter:
                //     return (&DAT_01597730)[param_1 * 2];   // ushort
                // So slotIdx = base + cursor*2; itemId at +slotIdx*4
                // (each "slot" is 4 bytes: ushort itemId + ushort flags).
                var slotBase = 0;
                var slotIdx = -1;
                if (stateNow === 6) {
                    try {
                        var lwc = rva(0x146A354).readPointer();
                        if (!lwc.isNull()) slotBase = lwc.add(0x58).readS32();
                    } catch (e) {}
                    if (listNow >= 0) slotIdx = slotBase + listNow * 2;
                }
                var announceList = false;
                if (slotIdx >= 0) {
                    if (itemsJustOpened) {
                        announceList = true;
                        itemsJustOpened = false;
                    } else if (slotIdx !== prevItemsSlotIdx && prevItemsSlotIdx !== -999) {
                        announceList = true;
                    }
                }
                if (announceList) {
                    // State machine semantics for *DAT_0186a354+0x48:
                    //   state 6  → ITEMS LIST cursor (initial state on
                    //              Items entry; user sees items list).
                    //              Resolve name from saveData inventory
                    //              + item-bin excel lookup. arg0=8 cache
                    //              MUST NOT be used here — those are
                    //              pre-fetched command bar names.
                    //   state 53 → COMMAND BAR cursor (after Back from
                    //              items list). Use itemsNameCache
                    //              (arg0=8): 0=Use, 1=Sort, 2=Key Items,
                    //              9=Equipment.
                    //   other   → submenu / transition; silence until we
                    //              have name sources for those.
                    if (stateNow === 6) {
                        // Direct read: itemId at DAT_01597730 + slotIdx*4
                        // (RVA 0x1197730). Each slot is 4 bytes; the low
                        // ushort is the item ID, the next byte is the
                        // count (per the decompile clear loop and
                        // FUN_008c66e0 return type).
                        var itemId = 0;
                        try {
                            itemId = rva(0x1197730).add(slotIdx * 4).readU16();
                        } catch (e) {}
                        if (itemId !== 0) {
                            var itemName = null;
                            try {
                                itemName = resolveCommandText(itemId, rva(0xD2A940));
                            } catch (e) {}
                            if (itemName && itemName.length > 0) {
                                speak(itemName, true);
                                logFile('[ITEMS list] row=' + listNow +
                                    ' base=' + slotBase + ' slot=' + slotIdx +
                                    ' id=0x' + itemId.toString(16) +
                                    ' name="' + itemName + '"');
                            } else {
                                logFile('[ITEMS list NO_NAME row=' + listNow +
                                    ' slot=' + slotIdx +
                                    ' id=0x' + itemId.toString(16) + ']');
                            }
                        } else {
                            logFile('[ITEMS list EMPTY row=' + listNow +
                                ' base=' + slotBase + ' slot=' + slotIdx + ']');
                        }
                    } else if (stateNow === 53) {
                        var cmdName = itemsNameCache[listNow];
                        if (cmdName && cmdName.length > 0) {
                            speak(cmdName, true);
                            logFile('[ITEMS cmdbar] cursor=' + listNow +
                                ' name="' + cmdName + '"');
                        } else {
                            logFile('[ITEMS cmdbar NO_NAME cursor=' + listNow +
                                ' (cache keys: ' + Object.keys(itemsNameCache).join(',') + ')]');
                        }
                    } else {
                        logFile('[ITEMS announce_skip state=' + stateNow +
                            ' cursor=' + listNow + ' (silenced — non-list/non-cmdbar state)]');
                    }
                }
                if (listNow >= 0) prevItemsListCursor = listNow;
                if (slotIdx >= 0) prevItemsSlotIdx = slotIdx;

                // Command bar announce: same silence-over-wrong rule.
                var announceCmd = false;
                if (cmdNow >= 0) {
                    if (prevItemsCmdCursor === -1) {
                        announceCmd = true;
                    } else if (cmdNow !== prevItemsCmdCursor) {
                        announceCmd = true;
                    }
                }
                if (announceCmd) {
                    // Same SILENCE policy as list path above. We don't yet
                    // know what DAT_023cc124 is or whether it's a real
                    // command-bar/items source. Log only.
                    var ctx = readTkWindowText(0x1FCC124, cmdNow);
                    if (ctx) {
                        var segs = getTkSegments(ctx);
                        logFile('[ITEMS_CMD_DISABLED cursor=' + cmdNow +
                            ' segs.len=' + segs.length +
                            ' segs=' + JSON.stringify(segs.slice(0, 8)) +
                            ' (silenced)]');
                    } else {
                        logFile('[ITEMS_CMD_DISABLED cursor=' + cmdNow + ' no_ctx (silenced)]');
                    }
                }
                if (cmdNow >= 0) prevItemsCmdCursor = cmdNow;
            } catch (e) {}
        }
    }, 'Items fn_ctrl');

    // (queryList / fn_render bracketing approach was removed —
    // FUN_00790a40 is not called by the items menu renderer at all,
    // so capturing its calls produced an empty list. The renderer
    // reads DAT_01597730 directly. We now do the same.)

    registerHotkey(VK_N, handleHelpHotkey, 'menus.help');

    logFile('Menus feature installed (universal port + Items fn_ctrl)');
}
