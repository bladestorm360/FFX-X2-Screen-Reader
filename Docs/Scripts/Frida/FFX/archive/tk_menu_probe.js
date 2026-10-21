// @desc Universal menu reader v5 — track text pointer shifts for popups
//
// Core approach (works for regular menus):
//   instruction = segments[0], focused item = segments[idx+1]
//
// Popup detection:
//   Track win+0x5C pointer. When it changes, a new popup/context opened.
//   For popups: items are the LAST N segments (N = item count from cursor data)
//   This handles confirmation dialogs where instruction text has many lines.
//
// Attach anywhere. Press arrows. Ctrl+C to stop.

var logPath = 'D:/Games/Dev/Custom/FFX/ffx-x2-screen-reader/Docs/Scripts/Frida/output/FFX/tk_menu_probe.log';
var logHandle = new File(logPath, 'w');
function logFile(msg) { logHandle.write(msg + '\n'); logHandle.flush(); }
function logBoth(msg) { console.log(msg); logFile(msg); }

var base = Process.enumerateModules()[0].base;
function rva(offset) { return base.add(offset); }

logBoth('[*] Universal menu reader v5');

var inputAddr = rva(0xF27080);
var windowArrayAddr = rva(0x14676B0);
var cursorArrayAddr = rva(0x1468A50);
var eventIdAddr = rva(0xEFBBF8);

var CHARSET = '0123456789 !"#$%&\'()*+,-./:;<=>?ABCDEFGHIJKLMNOPQRSTUVWXYZ[\\]^_\u2018abcdefghijklmnopqrstuvwxyz{|}~';

var INPUT_NAMES = {
    0x1000: 'Up', 0x4000: 'Down', 0x8000: 'Left', 0x2000: 'Right',
    0x0020: 'Confirm', 0x0040: 'Cancel'
};

// maxOps: if > 0, stop after this many operations (from win+0xA0)
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

var TITLE_ITEMS = ['New Game', 'Load'];

// ── getText hook ──
try {
    Interceptor.attach(rva(0x2DB320), {
        onEnter: function(args) { this.id = args[0].toInt32(); },
        onLeave: function(retval) {
            if (!retval.isNull()) {
                try {
                    var str = retval.readUtf8String();
                    if (str && str.length > 0) logBoth('[getText] "' + str + '"');
                } catch(e) {}
            }
        }
    });
    logBoth('[OK] getText hooked');
} catch(e) {}

// ── State tracking per window ──
var prevInput = 0;
var prevCursors = {};
var prevVisible = {};
var prevTextPtrs = {};  // track +0x5C pointer to detect popup shifts
var initialTextPtrs = {};  // store the first text pointer seen per window
var isPopup = {};  // true when text pointer has shifted from initial

function readWindowContext(w) {
    var winPtr = windowArrayAddr.add(w * 4).readPointer();
    var curPtr = cursorArrayAddr.add(w * 4).readPointer();
    if (winPtr.isNull() || curPtr.isNull()) return null;

    var vis = curPtr.add(1).readU8();
    var idx = curPtr.add(2).readU8();
    var items = curPtr.add(5).readU8();
    var tp = winPtr.add(0x5C).readPointer();

    return {winPtr: winPtr, curPtr: curPtr, vis: vis, idx: idx, items: items, textPtr: tp};
}

function getSegmentsForContext(ctx) {
    if (ctx.textPtr.isNull() || ctx.textPtr.compare(ptr(0x10000)) <= 0) return [];
    // Use distance between +0x5C (start) and +0x8C (render position) as decode length
    // This limits to the CURRENT screen only, not the full concatenated buffer
    var tp8C = ptr(0);
    try { tp8C = ctx.winPtr.add(0x8C).readPointer(); } catch(e) {}
    var decodeLen = 0x300; // fallback
    if (!tp8C.isNull() && tp8C.compare(ctx.textPtr) > 0) {
        var diff = tp8C.sub(ctx.textPtr).toInt32();
        if (diff > 0 && diff < 0x1000) {
            decodeLen = diff;
        }
    }
    logFile('[decode] 5C=0x' + ctx.textPtr.toString(16) + ' 8C=0x' + tp8C.toString(16) + ' len=' + decodeLen);
    return decodeFFXSegments(ctx.textPtr, decodeLen, 0);
}

function announceMenuOpen(w, ctx, segments) {
    // Instruction = everything before the last N items, joined
    var numInstruction = segments.length - ctx.items;
    if (numInstruction < 0) numInstruction = 0;
    var instruction = '';
    for (var i = 0; i < numInstruction; i++) {
        var cleaned = cleanText(segments[i]);
        if (cleaned.length > 1) { // skip single-char formatting fragments
            instruction += (instruction.length > 0 ? ' ' : '') + cleaned;
        }
    }
    if (instruction.length > 0) {
        logBoth('[MENU OPEN] ' + instruction);
    }

    // Initially focused item
    var focusedText = getFocusedItem(ctx, segments);
    logBoth('[FOCUS WIN' + w + '] ' + focusedText);
}

function getFocusedItem(ctx, segments) {
    // Items are the LAST N segments where N = item count
    var firstItem = segments.length - ctx.items;
    var target = firstItem + ctx.idx;
    if (target >= 0 && target < segments.length) {
        return cleanText(segments[target]);
    }
    return '?';
}

// ── Main loop ──
try {
    Interceptor.attach(rva(0x471D10), {
        onLeave: function(retval) {
            var input = inputAddr.readU16();
            var newPress = input & ~prevInput;
            if (newPress !== 0) {
                var buttons = [];
                for (var mask in INPUT_NAMES) {
                    if (newPress & parseInt(mask)) buttons.push(INPUT_NAMES[mask]);
                }
                logBoth('[PRESS] ' + buttons.join('+'));
            }
            prevInput = input;

            var eventId = eventIdAddr.readS32();

            for (var w = 0; w < 8; w++) {
                try {
                    var ctx = readWindowContext(w);
                    if (!ctx || ctx.vis === 0) {
                        prevVisible['w' + w] = 0;
                        continue;
                    }

                    var key = 'w' + w;
                    if (prevCursors[key] === undefined) prevCursors[key] = ctx.idx;

                    var wasVisible = prevVisible[key] || 0;
                    var oldTextPtr = prevTextPtrs[key] || ptr(0);
                    var textPtrChanged = !ctx.textPtr.equals(oldTextPtr) && !oldTextPtr.isNull();
                    prevTextPtrs[key] = ctx.textPtr;

                    // Track initial pointer and popup state
                    if (ctx.vis === 1 && wasVisible === 0) {
                        initialTextPtrs[key] = ctx.textPtr;
                        isPopup[key] = false;
                    }
                    if (textPtrChanged && initialTextPtrs[key]) {
                        isPopup[key] = !ctx.textPtr.equals(initialTextPtrs[key]);
                    }

                    // ── Menu open (vis 0→1): use idx+1 approach ──
                    if (ctx.vis === 1 && wasVisible === 0) {
                        if (eventId === 0x17 && w === 2) {
                            logBoth('[MENU OPEN] Title Screen');
                            logBoth('[FOCUS WIN' + w + '] ' +
                                (ctx.idx < TITLE_ITEMS.length ? TITLE_ITEMS[ctx.idx] : '?'));
                        } else {
                            var segments = getSegmentsForContext(ctx);
                            logFile('[WIN' + w + ' OPEN segments] ' + JSON.stringify(segments));
                            // First section: instruction = segments[0], items = segments[1..items]
                            if (segments.length > 0) {
                                logBoth('[MENU OPEN] ' + cleanText(segments[0]));
                            }
                            if (ctx.idx + 1 < segments.length) {
                                logBoth('[FOCUS WIN' + w + '] ' + cleanText(segments[ctx.idx + 1]));
                            }
                        }
                        prevCursors[key] = ctx.idx;
                    }
                    // ── Popup open (text pointer shifted): use last-N approach ──
                    else if (textPtrChanged) {
                        var segments = getSegmentsForContext(ctx);
                        logFile('[WIN' + w + ' POPUP segments] ' + JSON.stringify(segments));
                        if (segments.length > 0) {
                            announceMenuOpen(w, ctx, segments);
                        }
                        prevCursors[key] = ctx.idx;
                    }
                    prevVisible[key] = ctx.vis;

                    // ── Cursor change ──
                    if (ctx.idx !== prevCursors[key]) {
                        var focusedText = '';
                        if (eventId === 0x17 && w === 2) {
                            focusedText = ctx.idx < TITLE_ITEMS.length ? TITLE_ITEMS[ctx.idx] : '?';
                        } else {
                            var segments = getSegmentsForContext(ctx);
                            logFile('[WIN' + w + ' segments] ' + JSON.stringify(segments));
                            if (isPopup[key]) {
                                // Popup: items are last N segments
                                focusedText = getFocusedItem(ctx, segments);
                            } else {
                                // Normal menu: idx+1
                                if (ctx.idx + 1 < segments.length) {
                                    focusedText = cleanText(segments[ctx.idx + 1]);
                                } else {
                                    focusedText = '?';
                                }
                            }
                        }
                        logBoth('[FOCUS WIN' + w + '] ' + focusedText);
                        prevCursors[key] = ctx.idx;
                    }
                } catch(e) {}
            }
        }
    });
    logBoth('[OK] Monitoring all windows');
} catch(e) {}

logBoth('[*] Navigate menus. Ctrl+C to stop.');
