// @desc Config menu reader — follows game's own cursor and text resolution
// @python
//
// Config menu (scene 0x0A):
//   fn_ctrl = RVA 0x4CB0B0 — handles Up/Down/Left/Right input
//   Cursor index:    DAT_0186a404 (RVA 0x146A404)
//   Item count:      DAT_0186a408 (RVA 0x146A408)
//   Item array ptr:  DAT_0186a40c (RVA 0x146A40C) — array of pointers to item structs
//   Item struct: +0x04 = num options, +0x08 = current selection, +0x0C = visible, +0x18 = text ID
//   Text resolved via MsMenuGetText(7, textId, configLang)
//
// Approach: hook fn_ctrl, read cursor after input, announce setting name + current value.

var logPath = 'D:/Games/Dev/Custom/FFX/ffx-x2-screen-reader/Docs/Scripts/Frida/output/FFX/config_menu_probe.log';
var logHandle = new File(logPath, 'w');
function logFile(msg) { logHandle.write(msg + '\n'); logHandle.flush(); }
function logBoth(msg) { console.log(msg); logFile(msg); }

var base = Process.enumerateModules()[0].base;
function rva(offset) { return base.add(offset); }

logBoth('[*] Config menu probe');

// ── Addresses ──
var configCursorAddr   = rva(0x146A404);  // DAT_0186a404: current cursor index (byte)
var configCountAddr    = rva(0x146A408);  // DAT_0186a408: total item count (int)
var configArrayPtrAddr = rva(0x146A40C);  // DAT_0186a40C: pointer to item struct array
var configSpecialIdx   = rva(0x146A3F8);  // DAT_0186a3f8: Music Type item index (special rendering)

var msMenuGetTextRVA   = 0x38FD40;        // MsMenuGetText(arg0, arg1, arg2)
var configFnCtrlRVA    = 0x4CB0B0;        // Config scene fn_ctrl

// ── FFX charset decoder ──
var CHARSET = '0123456789 !"#$%&\'()*+,-./:;<=>?ABCDEFGHIJKLMNOPQRSTUVWXYZ[\\]^_\u2018abcdefghijklmnopqrstuvwxyz{|}~';

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

// ── Read config item at cursor index ──
function readConfigItem(cursorIdx) {
    try {
        var arrayPtr = configArrayPtrAddr.readPointer();
        if (arrayPtr.isNull()) return null;
        var itemPtr = arrayPtr.add(cursorIdx * 4).readPointer();
        if (itemPtr.isNull()) return null;

        var numOptions = itemPtr.add(0x04).readS32();
        var curSelection = itemPtr.add(0x08).readS32();
        var visible = itemPtr.add(0x0C).readS32();
        var textId = itemPtr.add(0x18).readU16();

        return {
            numOptions: numOptions,
            curSelection: curSelection,
            visible: visible,
            textId: textId,
            ptr: itemPtr
        };
    } catch(e) {
        logFile('[readConfigItem] error: ' + e.message);
        return null;
    }
}

// ── Call MsMenuGetText directly — same function the game uses ──
// FUN_0078fd40(arg0, arg1, arg2) -> pointer to FFX-charset text
// For config: arg0=7, arg1=textId, arg2=configLang (from FUN_007852b0)
var msMenuGetTextFn = new NativeFunction(rva(msMenuGetTextRVA), 'pointer', ['int', 'int', 'int']);
var getConfigLangFn = new NativeFunction(rva(0x3852B0), 'int', []);  // FUN_007852b0 = MsGetSaveConfigHiragana
// FUN_00641480(textId, outBuffer) — localized string table lookup (used for Music Type)
var locStringCopyFn = new NativeFunction(rva(0x241480), 'void', ['int', 'pointer']);

function resolveLocString(textId) {
    try {
        var buf = Memory.alloc(256);
        locStringCopyFn(textId, buf);
        var segs = decodeFFXSegments(buf, 256, 0);
        if (segs.length > 0) return segs[0];
    } catch(e) {
        logFile('[resolveLocString] error: ' + e.message);
    }
    return null;
}

function resolveConfigText(textId) {
    try {
        var lang = getConfigLangFn();
        var textPtr = msMenuGetTextFn(7, textId, lang);
        if (!textPtr.isNull()) {
            var segs = decodeFFXSegments(textPtr, 256, 0);
            if (segs.length > 0) return segs[0];
        }
    } catch(e) {}
    // Fallback: try localized string table (FUN_00641480)
    try {
        var buf = Memory.alloc(256);
        locStringCopyFn(textId, buf);
        var segs = decodeFFXSegments(buf, 256, 0);
        if (segs.length > 0) return segs[0];
    } catch(e) {
        logFile('[resolveConfigText] fallback error: ' + e.message);
    }
    return null;
}

// ── Hook fn_ctrl — fires every frame the config menu processes input ──
var prevCursor = -1;
var prevSelection = -1;
var configActive = false;

try {
    Interceptor.attach(rva(configFnCtrlRVA), {
        onEnter: function(args) {
            this.param1 = args[0]; // TkMenu* (param_1 in decompile)
        },
        onLeave: function() {
            // Read state AFTER fn_ctrl processes input
            var state = this.param1.add(0x1C).readS32();

            // Only announce when in navigation state (state 3)
            if (state < 3) {
                if (!configActive) {
                    configActive = true;
                    logBoth('[CONFIG] Opened');
                }
                return;
            }
            if (state > 3) return;
            configActive = true;

            var cursor = configCursorAddr.readU8();
            var item = readConfigItem(cursor);
            if (!item) return;

            var cursorChanged = (cursor !== prevCursor);
            var selectionChanged = (item.curSelection !== prevSelection);

            if (cursorChanged || selectionChanged) {
                // Check if this is the special Music Type item
                var specialIdx = configSpecialIdx.readS32();
                var isSpecial = (cursor === specialIdx);

                var settingName = '';
                var optionName = '';

                if (isSpecial) {
                    // Music Type: game uses localized string table, not MsMenuGetText
                    // Name=0x1CD(Music), options: 0x1CE(Original), 0x1D0(Arranged)
                    // Descriptions: 0x1CF(original desc), 0x1D1(arranged desc)
                    settingName = resolveLocString(0x1CD) || 'Music';
                    var musicOptIds = [0x1CE, 0x1D0];  // Original, Arranged
                    if (item.curSelection >= 0 && item.curSelection < musicOptIds.length) {
                        optionName = resolveLocString(musicOptIds[item.curSelection]) || ('option ' + item.curSelection);
                    } else {
                        optionName = 'option ' + item.curSelection;
                    }
                } else {
                    // Normal items: resolve via MsMenuGetText(7, textId, lang)
                    settingName = resolveConfigText(item.textId) || ('setting ' + item.textId);
                    try {
                        var optTextId = item.ptr.add(0x1C + item.curSelection * 4).readU32();
                        optionName = resolveConfigText(optTextId) || ('option ' + item.curSelection);
                    } catch(e) {
                        optionName = 'option ' + item.curSelection;
                    }
                }

                if (cursorChanged) {
                    logBoth('[CONFIG] ' + settingName + ': ' + optionName);
                } else {
                    logBoth('[CONFIG] ' + optionName);
                }

                prevCursor = cursor;
                prevSelection = item.curSelection;
            }
        }
    });
    logBoth('[OK] Config fn_ctrl hooked (0x4CB0B0)');
} catch(e) { logBoth('[FAIL] Config fn_ctrl: ' + e.message); }

// ── Hotkey handler ──
// ── Resolve description text for current option ──
function resolveConfigDesc(textId) {
    try {
        var lang = getConfigLangFn();
        var textPtr = msMenuGetTextFn(7, textId, lang);
        if (!textPtr.isNull()) {
            var segs = decodeFFXSegments(textPtr, 256, 0);
            // Format: name | - | description
            if (segs.length > 2 && segs[1] === '-') return segs[2];
        }
    } catch(e) {}
    return null;
}

recv('hotkey', function onHotkey(msg) {
    if (msg.key === 'n') {
        // Read description for current config option
        var cursor = configCursorAddr.readU8();
        var item = readConfigItem(cursor);
        if (item) {
            var specialIdx = configSpecialIdx.readS32();
            var desc = null;
            try {
                if (cursor === specialIdx) {
                    // Music Type: descriptions at 0x1CF (Original), 0x1D1 (Arranged)
                    var musicDescIds = [0x1CF, 0x1D1];
                    if (item.curSelection >= 0 && item.curSelection < musicDescIds.length) {
                        desc = resolveLocString(musicDescIds[item.curSelection]);
                    }
                } else {
                    var optTextId = item.ptr.add(0x1C + item.curSelection * 4).readU32();
                    desc = resolveConfigDesc(optTextId);
                    if (!desc) desc = resolveConfigDesc(item.textId);
                }
            } catch(e) {}
            logBoth('[DESC] ' + (desc || 'No description available'));
        }
    }
    else if (msg.key === 'f') {
        // Find Music Type text: scan localized string table for nearby IDs
        logBoth('[SCAN] Scanning localized strings 0x1C0-0x1E0:');
        for (var sid = 0x1C0; sid <= 0x1E0; sid++) {
            var text = resolveLocString(sid);
            if (text && text.length > 0 && text.length < 40) {
                logBoth('  0x' + sid.toString(16) + ' = ' + text);
            }
        }
        // Also scan MsMenuGetText(7,...) for IDs 0-30
        logBoth('[SCAN] MsMenuGetText(7, 0-30):');
        for (var mid = 0; mid <= 30; mid++) {
            var mt = resolveConfigText(mid);
            if (mt) logBoth('  id=' + mid + ' = ' + mt);
        }
    }
    else if (msg.key === 'd') {
        // Dump raw hex of each config item struct to understand the layout
        var count = configCountAddr.readS32();
        logBoth('[DUMP] ' + count + ' config items (details in log):');
        var arrayPtr = configArrayPtrAddr.readPointer();
        for (var i = 0; i < count; i++) {
            try {
                var itemPtr = arrayPtr.add(i * 4).readPointer();
                // Dump first 0x30 bytes as hex + decoded fields
                var hex = '';
                for (var b = 0; b < 0x30; b += 4) {
                    hex += ' ' + ('00000000' + itemPtr.add(b).readU32().toString(16)).slice(-8);
                }
                var numOpt = itemPtr.add(0x04).readS32();
                var curSel = itemPtr.add(0x08).readS32();
                var vis = itemPtr.add(0x0C).readS32();
                var nameId = itemPtr.add(0x18).readU16();
                var resolvedName = resolveConfigText(nameId) || '???';

                // Try reading option text IDs at various offsets
                var optIds = '';
                for (var oi = 0; oi < numOpt && oi < 4; oi++) {
                    var otid = itemPtr.add(0x1C + oi * 2).readU16();
                    var otName = resolveConfigText(otid) || '?';
                    optIds += ' opt' + oi + '=' + otid + '(' + otName + ')';
                }

                logFile('[ITEM ' + i + '] hex:' + hex);
                logFile('  name=' + resolvedName + '(id=' + nameId + ') sel=' + curSel + '/' + numOpt +
                    ' vis=' + vis + optIds);
                logBoth('  [' + i + '] ' + resolvedName + ': sel=' + curSel + '/' + numOpt);
            } catch(e) {
                logFile('[ITEM ' + i + '] error: ' + e.message);
            }
        }
    }
    recv('hotkey', onHotkey);
});

logBoth('[*] Open Config menu. Hotkeys: d=dump items, q=quit');
logBoth('[*] Log: ' + logPath);
