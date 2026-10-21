// @desc Field Items menu — row→itemId getter, TkWindow snapshots, DAT_01597730 dump
// @auto
//
// PURPOSE
// =======
// Diagnose why the field (out-of-battle) Items menu speaks items in the wrong
// order and sometimes names items not in the player's inventory. Production hook
// at src/menus/menus.js:439 reads the displayed-list global DAT_01597730 directly
// using slotIdx = base + cursor*2 (matching the decompile of FUN_008C5420 case
// 0x8c5489), but the speech is wrong. This probe captures ground truth:
//
//   1. FUN_008C66E0 (RVA 0x4C66E0) — the game's own row→itemId getter. One line:
//          FUN_008c66e0 resolves a row index through the native item-ID table.
//          It returns the 16-bit item identifier; no recovered body is retained.
//      Whatever this returns when called by the renderer for a given row IS the
//      itemId the game just rendered at that row. Hook it; capture (param_1, ret)
//      pairs into a per-second-window map.
//
//   2. Both candidate cursor TkWindows snapshotted once per ~30 frames:
//        - items-list TkWindow at *0x146A354
//        - command-bar TkWindow at *0x1FCC124
//      Logged with state from FUN_008C5420 (the items fn_ctrl, RVA 0x4C5420)
//      so we can correlate state value → which TkWindow's cursor moves.
//
//   3. First 32 entries of DAT_01597730 (RVA 0x1197730) snapshotted per second.
//      Per the writer at layer2_game_logic.txt:562230 each entry is at least
//      4 bytes (ushort itemId, byte type, byte index); the reader strides 8
//      bytes per cursor (cursor*2 ushorts indexed = cursor*4 ushort offset =
//      cursor*8 bytes). Either entries are 8 bytes or the reader is reading an
//      every-other-entry pattern. Dump 256 bytes (32 × 8-byte slots) per snapshot
//      so both interpretations are visible.
//
// FUN_00790a40 (the item-record lookup) is NOT hooked — its body is a 1-line
// wrapper using DAT_0112a940 (RVA 0xD2A940) as the bin pointer, which is the
// SAME address as the battle items bin (btlItemBinAddr in addresses.js). So
// name resolution is shared between battle and field; the bug is not there.
//
// RVA NOTES
// =========
// All RVAs below are Ghidra VA - 0x400000. Confirmed against frida_reference.md
// and addresses.js in the production source. Fahrenheit values not used here.
//
//   FUN_008C66E0   RVA 0x4C66E0   Items row→itemId getter
//   FUN_008C5420   RVA 0x4C5420   Items fn_ctrl (state at param_1+0x1C)
//   DAT_01597730   RVA 0x1197730  Displayed-list global (writer-rebuilt per submenu)
//   *0x146A354     RVA 0x146A354  Items-list TkWindow pointer (cursor at +0x48)
//   *0x1FCC124     RVA 0x1FCC124  Command-bar TkWindow pointer (cursor at +0x48)
//
// CONSOLE BUDGET (CLAUDE.md)
// ==========================
// Console: at most one summary line every ~600 frames (~10 sec). Everything
// else file-only. The user is blind and runs a screen reader; flooding console
// crashes it.

var base = Process.enumerateModules()[0].base;
function rva(off) { return base.add(off); }

var logPath = 'D:/Games/Dev/Custom/FFX/ffx-x2-screen-reader/Docs/Scripts/Frida/output/FFX/field_items_probe.log';
var logH = new File(logPath, 'w');
function logFile(m) { logH.write(m + '\n'); logH.flush(); }
function logBoth(m) { console.log(m); logFile(m); }

logBoth('[FIELD_ITEMS_PROBE] base=' + base + ' starting hooks');

// ── State ──────────────────────────────────────────────────────────────────

// Per-frame buffer of FUN_008C66E0 calls. Reset at the start of every items
// fn_ctrl invocation. Keys are the row index (param_1); values are the itemId
// the game returned (low 16 bits of eax).
var frameItemByRow = {};

// Most recent state value pulled from items fn_ctrl param_1+0x1C. Used to
// annotate cursor / TkWindow snapshots with the active state-machine value.
var lastFnCtrlState = -999;

// Frame-index counter incremented on every items fn_ctrl call. Lets us tag
// log lines so call-order across hooks is reconstructable from the log.
var frameCounter = 0;

// Snapshot cadences — frameCounter modulus values.
var TKWIN_SNAPSHOT_EVERY = 30;       // ~0.5 s at 60 fps
var DAT_SNAPSHOT_EVERY = 60;          // ~1 s
var GETTER_HEARTBEAT_EVERY = 600;     // ~10 s — the one console line per cycle

// Last-spoken values for change detection (file-only, not screen-reader speech).
var prevListCursor = -1;
var prevCmdCursor = -1;
var prevState = -999;
var lastGetterRow = -1;
var lastGetterId = -1;
var getterCallsSinceHeartbeat = 0;

// ── Helpers ────────────────────────────────────────────────────────────────

function readU16Safe(addr) {
    try { return addr.readU16(); } catch (e) { return -1; }
}
function readS32Safe(addr) {
    try { return addr.readS32(); } catch (e) { return -999999; }
}
function readPtrSafe(addr) {
    try { return addr.readPointer(); } catch (e) { return ptr(0); }
}
function hexBytes(addr, len) {
    try {
        var bytes = new Uint8Array(addr.readByteArray(len));
        var s = '';
        for (var i = 0; i < bytes.length; i++) {
            s += (bytes[i] < 0x10 ? '0' : '') + bytes[i].toString(16);
            if ((i & 3) === 3 && i + 1 < bytes.length) s += ' ';
        }
        return s;
    } catch (e) { return '<read-fail:' + e.message + '>'; }
}

// ── Hook 1: FUN_008C66E0 (row→itemId getter) ──────────────────────────────
//
// Capture (param_1, ret) pairs into frameItemByRow. The renderer calls this
// once per visible row per frame; by the time items fn_ctrl onLeave fires,
// the map should be populated for every row currently on screen.

Interceptor.attach(rva(0x4C66E0), {
    onEnter: function(args) {
        this.row = args[0].toInt32();
    },
    onLeave: function(retval) {
        var id = retval.toInt32() & 0xFFFF;
        frameItemByRow[this.row] = id;
        lastGetterRow = this.row;
        lastGetterId = id;
        getterCallsSinceHeartbeat++;
    }
});

// ── Hook 2: FUN_008C5420 (items fn_ctrl) ──────────────────────────────────
//
// onEnter resets the per-frame map so each frame's getter calls are isolated.
// onLeave is where we read cursors, snapshot TkWindows, dump DAT_01597730,
// and emit the heartbeat. State is read from param_1+0x1C — same field the
// production hook uses (lines 453, 244 of menus.js).

Interceptor.attach(rva(0x4C5420), {
    onEnter: function(args) {
        this.param1 = args[0];
        // Snapshot the previous frame's getter map so we can log it before
        // resetting. (frameItemByRow at this moment contains the PREVIOUS
        // frame's row→id pairs, since the getter fires during fn_render
        // which runs after fn_ctrl.)
        this.prevFrameMap = frameItemByRow;
        frameItemByRow = {};
    },
    onLeave: function() {
        frameCounter++;
        var stateNow = -999;
        try { stateNow = this.param1.add(0x1C).readS32(); } catch (e) {}
        lastFnCtrlState = stateNow;

        // ── State change ────────────────────────────────────────────────
        if (stateNow !== prevState) {
            logFile('[STATE] frame=' + frameCounter +
                ' state=' + prevState + '->' + stateNow);
            prevState = stateNow;
        }

        // ── Items-list TkWindow ─────────────────────────────────────────
        var listW = readPtrSafe(rva(0x146A354));
        var listCursor = -1, listItems = -1, listBase = -1;
        var listTextPtr = ptr(0);
        if (!listW.isNull()) {
            listCursor = readU16Safe(listW.add(0x48));
            listItems = readU16Safe(listW.add(0x30));
            listBase = readS32Safe(listW.add(0x58));
            listTextPtr = readPtrSafe(listW.add(0x5C));
        }

        // ── Command-bar TkWindow ────────────────────────────────────────
        var cmdW = readPtrSafe(rva(0x1FCC124));
        var cmdCursor = -1, cmdItems = -1;
        var cmdTextPtr = ptr(0);
        if (!cmdW.isNull()) {
            cmdCursor = readU16Safe(cmdW.add(0x48));
            cmdItems = readU16Safe(cmdW.add(0x30));
            cmdTextPtr = readPtrSafe(cmdW.add(0x5C));
        }

        // ── Cursor change events (file-only) ────────────────────────────
        if (listCursor !== prevListCursor) {
            // Prev frame's getter map — what the renderer ASKED for
            var ids = Object.keys(this.prevFrameMap || {})
                .map(function(k) {
                    return k + '=>0x' + (this.prevFrameMap[k] & 0xFFFF).toString(16);
                }.bind(this))
                .sort()
                .join(',');
            logFile('[LIST_CURSOR] frame=' + frameCounter +
                ' state=' + stateNow +
                ' prev=' + prevListCursor + ' now=' + listCursor +
                ' base=' + listBase + ' items=' + listItems +
                ' getter_map_prev_frame={' + ids + '}');
            prevListCursor = listCursor;
        }
        if (cmdCursor !== prevCmdCursor) {
            logFile('[CMD_CURSOR] frame=' + frameCounter +
                ' state=' + stateNow +
                ' prev=' + prevCmdCursor + ' now=' + cmdCursor +
                ' items=' + cmdItems);
            prevCmdCursor = cmdCursor;
        }

        // ── Periodic TkWindow snapshot ──────────────────────────────────
        if ((frameCounter % TKWIN_SNAPSHOT_EVERY) === 0) {
            var msg = '[TKWIN] frame=' + frameCounter + ' state=' + stateNow;
            if (!listW.isNull()) {
                msg += ' list{ptr=' + listW +
                    ' cursor=' + listCursor +
                    ' items=' + listItems +
                    ' base=' + listBase +
                    ' textPtr=' + listTextPtr + '}';
            } else {
                msg += ' list{NULL}';
            }
            if (!cmdW.isNull()) {
                msg += ' cmd{ptr=' + cmdW +
                    ' cursor=' + cmdCursor +
                    ' items=' + cmdItems +
                    ' textPtr=' + cmdTextPtr + '}';
            } else {
                msg += ' cmd{NULL}';
            }
            logFile(msg);

            // Optional textPtr peeks (file-only). 0x80 bytes is enough to see
            // whether item names are embedded as decoded text or as glyph
            // indices we'd have to translate.
            if (!listTextPtr.isNull() && listTextPtr.compare(ptr(0x10000)) > 0) {
                logFile('[TKWIN_LIST_TEXT] ' + hexBytes(listTextPtr, 0x80));
            }
            if (!cmdTextPtr.isNull() && cmdTextPtr.compare(ptr(0x10000)) > 0) {
                logFile('[TKWIN_CMD_TEXT] ' + hexBytes(cmdTextPtr, 0x80));
            }
        }

        // ── Periodic DAT_01597730 dump ──────────────────────────────────
        if ((frameCounter % DAT_SNAPSHOT_EVERY) === 0) {
            // 32 × 8-byte slots = 256 bytes. Covers both 4-byte and 8-byte
            // entry interpretations so the log shows the actual layout.
            logFile('[DAT_01597730] frame=' + frameCounter +
                ' state=' + stateNow + ' bytes=' + hexBytes(rva(0x1197730), 256));
        }

        // ── Console heartbeat ──────────────────────────────────────────
        if ((frameCounter % GETTER_HEARTBEAT_EVERY) === 0) {
            console.log('[ItemByRow] frame=' + frameCounter +
                ' state=' + stateNow +
                ' list_cursor=' + listCursor +
                ' cmd_cursor=' + cmdCursor +
                ' last_getter row=' + lastGetterRow +
                ' id=0x' + (lastGetterId & 0xFFFF).toString(16) +
                ' getter_calls=' + getterCallsSinceHeartbeat);
            getterCallsSinceHeartbeat = 0;
        }
    }
});

logBoth('[FIELD_ITEMS_PROBE] hooks installed.');
logBoth('  - FUN_008C66E0 @ RVA 0x4C66E0  (row->itemId getter)');
logBoth('  - FUN_008C5420 @ RVA 0x4C5420  (items fn_ctrl, frame coord)');
logBoth('Open the field menu, press Items, navigate Use list, Back, navigate command bar,');
logBoth('confirm on Important Items, navigate that list, Back twice to exit.');
logBoth('Log: ' + logPath);
