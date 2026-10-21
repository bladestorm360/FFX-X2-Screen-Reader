// Main-menu fallback, legacy title/popups and native text cache.
// Dedicated input readers in items.js/commandBars.js/fieldLists.js/partyLists.js
// replace the broken Items assumptions. The legacy draw iterator remains for
// existing title/dialog/popups until their native event migration is validated.

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
import { NATIVE_MAIN_MENU } from '../core/native.js';

export const msMenuTextCache = {};
export const msMenuDescCache = {};
// Namespace 8 supplies Items command-bar labels. Inventory text is read
// through the native record getters in items.js, independently of this cache.
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
    if (!NATIVE_MAIN_MENU) attachRva(0x4E0340, {
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

    registerHotkey(VK_N, handleHelpHotkey, 'menus.help');

    logFile('Menus feature installed (main-menu ownership coordinated with native)');
}
