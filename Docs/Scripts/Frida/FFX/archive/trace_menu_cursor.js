// ARCHIVED — BUG: reads from global DAT_ pointers instead of ECX. See track_cursor_native.js for fixed version.
// @desc Universal menu cursor tracker — hooks cursor update, sound, and position setter
//
// Hooks three layers of the menu system to catch cursor movement universally:
//   1. FUN_00a58ec0 — main cursor update (reads buttons, moves cursor, plays sound)
//   2. FUN_00886de0 — sound event trigger (0x80000004 = cursor move)
//   3. FUN_00a48ed0 — cursor position setter (alternative path, sets X/Y in menu struct)
//   4. FUN_008a9810 — get current menu item ID at cursor
//
// Key globals (all Ghidra VAs, rebased):
//   DAT_02305834 (ptr) +0x115bc = current cursor index
//                      +0x115bd = previous cursor index
//                      +0x11672 = button state (bits: 0x4=UP, 0x8=DOWN)
//                      +0x110cc + cursor*0x50 = menu items array
//   DAT_01841be8 = global cursor index
//   DAT_01841bd4[index] = menu item ID lookup
//   DAT_01a86108 = menu instance array (0x340 per menu, cursor at +0x20)
//
// Navigate any menu. Ctrl+C to stop.

var logPath = 'D:/Games/Dev/Custom/FFX/ffx-x2-screen-reader/Docs/Scripts/Frida/output/FFX/trace_menu_cursor.log';
var logHandle = new File(logPath, 'w');

function logFile(msg) {
    logHandle.write(msg + '\n');
    logHandle.flush();
}

function logBoth(msg) {
    console.log(msg);
    logFile(msg);
}

function tryReadCString(p) {
    try {
        if (p.isNull()) return null;
        var s = p.readCString();
        if (s && s.length > 0 && s.length < 4096) return s;
    } catch (e) {}
    return null;
}

function tryReadUtf16(p) {
    try {
        if (p.isNull()) return null;
        var s = p.readUtf16String();
        if (s && s.length > 0 && s.length < 4096) return s;
    } catch (e) {}
    return null;
}

function hexDumpBytes(addr, len) {
    try {
        var buf = addr.readByteArray(len);
        var bytes = new Uint8Array(buf);
        var hex = '';
        for (var i = 0; i < bytes.length; i++) {
            var h = bytes[i].toString(16);
            if (h.length < 2) h = '0' + h;
            hex += h + ' ';
        }
        return hex.trim();
    } catch (e) {
        return '(unreadable)';
    }
}

// ── Ghidra rebase (proven: offset 0x680000) ──
var ffxMod = Process.enumerateModules()[0];
var rebaseOffset = ffxMod.base.sub(0x00400000);

function g(ghidraAddr) {
    return ptr(ghidraAddr).add(rebaseOffset);
}

logBoth('[*] trace_menu_cursor (rebase 0x' + rebaseOffset.toString(16) + ')');

// ── Resolve global addresses ──
var pMainObj   = g(0x02305834);  // pointer to main menu/game object
var pCursorIdx = g(0x01841be8);  // global cursor index
var pItemTable = g(0x01841bd4);  // menu item ID lookup table
var pMenuArray = g(0x01a86108);  // menu instance array

// Read initial state
function readMainObj() {
    try { return pMainObj.readPointer(); } catch(e) { return ptr(0); }
}

function readCursorState() {
    var obj = readMainObj();
    if (obj.isNull()) return null;
    try {
        return {
            cursorNew: obj.add(0x115bc).readU8(),
            cursorOld: obj.add(0x115bd).readU8(),
            buttons:   obj.add(0x11672).readU16(),
            direction: obj.add(0x115be).readU8()
        };
    } catch(e) { return null; }
}

function readGlobalCursor() {
    try {
        var idx = pCursorIdx.readU32();
        var itemId = pItemTable.add(idx).readU8();
        return { index: idx, itemId: itemId };
    } catch(e) { return null; }
}

function readMenuItem(cursorIdx) {
    var obj = readMainObj();
    if (obj.isNull()) return null;
    try {
        var itemBase = obj.add(0x110cc + cursorIdx * 0x50);
        var hex = hexDumpBytes(itemBase, 0x50);
        // Try reading potential text pointers within the 0x50 byte item
        var texts = [];
        for (var off = 0; off < 0x50; off += 4) {
            var p = itemBase.add(off).readPointer();
            var s = tryReadCString(p);
            if (s && s.length > 2) texts.push('+0x' + off.toString(16) + ': "' + s + '"');
            var u = tryReadUtf16(p);
            if (u && u.length > 2 && u !== s) texts.push('+0x' + off.toString(16) + ': u"' + u + '"');
        }
        return { hex: hex, texts: texts };
    } catch(e) { return null; }
}

function readMenuStruct(menuIdx) {
    try {
        var base = pMenuArray.readPointer();
        if (base.isNull()) return null;
        var menu = base.add(menuIdx * 0x340);
        return {
            cursorIdx: menu.add(0x20).readU16(),
            cursorX:   menu.add(0x2a).readU16(),
            cursorY:   menu.add(0x2c).readU16(),
            primCount: menu.add(0x1e).readU16(),
            rows:      menu.add(0x16).readU16(),
            cols:      menu.add(0x22).readU8()
        };
    } catch(e) { return null; }
}

// ── Log initial state ──
var initState = readCursorState();
var initGlobal = readGlobalCursor();
logBoth('[*] Main obj ptr: 0x' + readMainObj().toString(16));
if (initState) {
    logBoth('[*] Initial cursor: ' + initState.cursorNew + ' (prev: ' + initState.cursorOld + ')');
}
if (initGlobal) {
    logBoth('[*] Global cursor idx: ' + initGlobal.index + ', item ID: ' + initGlobal.itemId);
}

// Check first 8 menu structs
for (var m = 0; m < 8; m++) {
    var ms = readMenuStruct(m);
    if (ms && ms.primCount > 0) {
        logBoth('[*] Menu[' + m + ']: cursor=' + ms.cursorIdx + ' prims=' + ms.primCount +
            ' rows=' + ms.rows + ' cols=' + ms.cols + ' xy=(' + ms.cursorX + ',' + ms.cursorY + ')');
    }
}

// ── Hook 1: FUN_00a58ec0 — Universal cursor update ──
var cursorUpdateCount = 0;

try {
    Interceptor.attach(g(0x00a58ec0), {
        onEnter: function(args) {
            this.before = readCursorState();
            this.globalBefore = readGlobalCursor();
        },
        onLeave: function(retval) {
            cursorUpdateCount++;
            var after = readCursorState();
            var globalAfter = readGlobalCursor();

            if (!after) {
                logFile('[CursorUpdate #' + cursorUpdateCount + '] could not read state');
                return;
            }

            var changed = this.before && (this.before.cursorNew !== after.cursorNew);
            var globalChanged = this.globalBefore && globalAfter &&
                (this.globalBefore.index !== globalAfter.index);

            // Always log to file
            logFile('[CursorUpdate #' + cursorUpdateCount + '] cursor=' + after.cursorNew +
                ' prev=' + after.cursorOld + ' dir=' + after.direction +
                ' btns=0x' + after.buttons.toString(16) +
                ' globalIdx=' + (globalAfter ? globalAfter.index : '?') +
                ' itemId=' + (globalAfter ? globalAfter.itemId : '?') +
                ' changed=' + changed);

            // Console: only on actual change
            if (changed || globalChanged) {
                var menuItem = readMenuItem(after.cursorNew);
                logBoth('[CURSOR] ' + (this.before ? this.before.cursorNew : '?') +
                    ' -> ' + after.cursorNew +
                    ' (item=' + (globalAfter ? globalAfter.itemId : '?') + ')');

                if (menuItem && menuItem.texts.length > 0) {
                    for (var i = 0; i < menuItem.texts.length; i++) {
                        logBoth('[TEXT] ' + menuItem.texts[i]);
                    }
                }

                // Dump menu item hex to file
                if (menuItem) {
                    logFile('  item hex: ' + menuItem.hex);
                }

                // Also check menu structs for changes
                for (var m = 0; m < 8; m++) {
                    var ms = readMenuStruct(m);
                    if (ms && ms.primCount > 0) {
                        logFile('  menu[' + m + ']: cursor=' + ms.cursorIdx +
                            ' prims=' + ms.primCount);
                    }
                }
            }
        }
    });
    logBoth('[OK] FUN_00a58ec0 (cursor update)');
} catch (e) {
    logBoth('[!] FUN_00a58ec0: ' + e.message);
}

// ── Hook 2: FUN_00886de0 — Sound event ──
var soundCount = 0;

try {
    Interceptor.attach(g(0x00886de0), {
        onEnter: function(args) {
            soundCount++;
            var eventId = args[0].toInt32();
            logFile('[SOUND #' + soundCount + '] event=0x' + (eventId >>> 0).toString(16));
            // 0x80000004 = cursor move sound
            if (eventId === -2147483644 || (eventId >>> 0) === 0x80000004) {
                var state = readCursorState();
                var global = readGlobalCursor();
                logBoth('[SOUND] Cursor move! cursor=' +
                    (state ? state.cursorNew : '?') +
                    ' globalIdx=' + (global ? global.index : '?') +
                    ' itemId=' + (global ? global.itemId : '?'));
            }
        }
    });
    logBoth('[OK] FUN_00886de0 (sound event)');
} catch (e) {
    logBoth('[!] FUN_00886de0: ' + e.message);
}

// ── Hook 3: FUN_00a48ed0 — Cursor position setter ──
var posSetCount = 0;

try {
    Interceptor.attach(g(0x00a48ed0), {
        onEnter: function(args) {
            posSetCount++;
            var menuIdx = args[0].toInt32();
            logFile('[PosSet #' + posSetCount + '] menu=' + menuIdx +
                ' x=' + args[1].toInt32() + ' y=' + args[2].toInt32() +
                ' dx=' + args[3].toInt32() + ' dy=' + args[4].toInt32());
            if (posSetCount <= 5 || posSetCount % 10 === 0) {
                logBoth('[PosSet] menu=' + menuIdx + ' pos=(' +
                    args[1].toInt32() + ',' + args[2].toInt32() + ')');
            }
        }
    });
    logBoth('[OK] FUN_00a48ed0 (cursor position setter)');
} catch (e) {
    logBoth('[!] FUN_00a48ed0: ' + e.message);
}

// ── Hook 4: FUN_008a9810 — Get menu item ID at cursor ──
var getItemCount = 0;

try {
    Interceptor.attach(g(0x008a9810), {
        onLeave: function(retval) {
            getItemCount++;
            var itemId = retval.toInt32();
            logFile('[GetItem #' + getItemCount + '] returned itemId=' + itemId);
        }
    });
    logBoth('[OK] FUN_008a9810 (get menu item)');
} catch (e) {
    logBoth('[!] FUN_008a9810: ' + e.message);
}

logBoth('[*] All hooks set. Navigate menus (up/down/confirm). Ctrl+C to stop.');
