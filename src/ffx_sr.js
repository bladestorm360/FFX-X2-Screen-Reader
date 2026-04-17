/*
 * ffx_sr.js — FFX/FFX-2 HD Remaster Screen Reader Mod
 *
 * Loaded by Frida Gadget (frida-gadget.dll) at game startup.
 * Merged from universal_menu_battle_reader.js + nav_integrated.js
 * with Tolk speech output and GetAsyncKeyState keyboard input.
 */

// ══════════════════════════════════════════
// SECTION 0: Logging + Tolk Speech
// ══════════════════════════════════════════

var LOG_PATH = '';
var logHandle = null;
var base = Process.enumerateModules()[0].base;
function rva(offset) { return base.add(offset); }

(function initLog() {
    try {
        var exePath = Process.enumerateModules()[0].path;
        var gameDir = exePath.substring(0, exePath.lastIndexOf('\\') + 1);
        LOG_PATH = gameDir + 'FFX_SR_Latest.log';
        logHandle = new File(LOG_PATH, 'w');
        logFile('FFX Screen Reader Mod — Frida Gadget mode');
        logFile('Game: ' + exePath);
        logFile('Base: 0x' + base.toString(16));
    } catch (e) {}
})();

function logFile(msg) {
    if (!logHandle) return;
    try {
        var d = new Date();
        var ts = ('0'+d.getHours()).slice(-2) + ':' + ('0'+d.getMinutes()).slice(-2) + ':' +
                 ('0'+d.getSeconds()).slice(-2) + '.' + ('00'+d.getMilliseconds()).slice(-3);
        logHandle.write('[' + ts + '] ' + msg + '\n');
        logHandle.flush();
    } catch(e) {}
}

// Tolk speech
var tolkLoaded = false;
var fnTolkOutput = null;

(function initTolk() {
    try {
        Module.load('Tolk.dll');
        var tolkModule = null;
        Process.enumerateModules().forEach(function(m) {
            if (m.name.toLowerCase() === 'tolk.dll') tolkModule = m;
        });
        if (!tolkModule) { logFile('Tolk.dll not available'); return; }

        var addrs = {};
        tolkModule.enumerateExports().forEach(function(exp) { addrs[exp.name] = exp.address; });

        var fnLoad = new NativeFunction(addrs['Tolk_Load'], 'void', []);
        var fnTrySAPI = new NativeFunction(addrs['Tolk_TrySAPI'], 'void', ['int']);
        var fnIsLoaded = new NativeFunction(addrs['Tolk_IsLoaded'], 'int', []);
        fnTolkOutput = new NativeFunction(addrs['Tolk_Output'], 'int', ['pointer', 'int']);
        var fnDetect = new NativeFunction(addrs['Tolk_DetectScreenReader'], 'pointer', []);

        fnTrySAPI(1);
        fnLoad();
        if (fnIsLoaded()) {
            tolkLoaded = true;
            var srPtr = fnDetect();
            logFile('Tolk OK, screen reader: ' + (srPtr.isNull() ? 'none' : srPtr.readUtf16String()));
        } else {
            logFile('Tolk_IsLoaded returned false');
        }
    } catch(e) { logFile('Tolk init error: ' + e.message); }
})();

function speak(text, interrupt) {
    logFile('[SPEECH' + (interrupt ? ' INT' : '') + '] ' + text);
    if (!tolkLoaded) return;
    try {
        var buf = Memory.allocUtf16String(text);
        fnTolkOutput(buf, interrupt ? 1 : 0);
    } catch(e) {}
}

// logBoth = speak (user-facing output)
function logBoth(msg) { speak(msg, true); }

logFile('Speech layer ready');

// ══════════════════════════════════════════
// SECTION 1: Keyboard Input (GetAsyncKeyState)
// ══════════════════════════════════════════

var fnGetAsyncKeyState = null;
var fnGetForegroundWindow = null;
var fnGetWindowThreadProcessId = null;
var myPid = Process.id;

try {
    var user32 = null;
    Process.enumerateModules().forEach(function(m) {
        if (m.name.toLowerCase() === 'user32.dll') user32 = m;
    });
    if (user32) {
        user32.enumerateExports().forEach(function(exp) {
            if (exp.name === 'GetAsyncKeyState')
                fnGetAsyncKeyState = new NativeFunction(exp.address, 'int16', ['int']);
            if (exp.name === 'GetForegroundWindow')
                fnGetForegroundWindow = new NativeFunction(exp.address, 'pointer', []);
            if (exp.name === 'GetWindowThreadProcessId')
                fnGetWindowThreadProcessId = new NativeFunction(exp.address, 'uint32', ['pointer', 'pointer']);
        });
    }
    logFile('Keyboard: ' + (fnGetAsyncKeyState ? 'OK' : 'FAIL') +
        ', ForegroundCheck: ' + (fnGetForegroundWindow ? 'OK' : 'FAIL'));
} catch(e) { logFile('Keyboard init error: ' + e.message); }

var VK_OEM_4 = 0xDB; // [
var VK_OEM_6 = 0xDD; // ]
var VK_OEM_3 = 0xC0; // `
var VK_OEM_5 = 0xDC; // backslash
var VK_P = 0x50;
var VK_T = 0x54;
var VK_H = 0x48;
var VK_N = 0x4E;
var VK_S = 0x53;
var VK_SHIFT = 0x10;

var keyWasDown = {};

var pidBuf = Memory.alloc(4);

function isGameFocused() {
    if (!fnGetForegroundWindow || !fnGetWindowThreadProcessId) return true;
    var hwnd = fnGetForegroundWindow();
    if (hwnd.isNull()) return false;
    fnGetWindowThreadProcessId(hwnd, pidBuf);
    return pidBuf.readU32() === myPid;
}

function keyPressed(vk) {
    if (!fnGetAsyncKeyState) return false;
    var down = (fnGetAsyncKeyState(vk) & 0x8000) !== 0;
    var was = keyWasDown[vk] || false;
    keyWasDown[vk] = down;
    if (!isGameFocused()) return false;
    return down && !was;
}

// ══════════════════════════════════════════
// SECTION 2: FFX Charset + Constants
// ══════════════════════════════════════════

var CHARSET = '0123456789 !"#$%&\'()*+,-./:;<=>?ABCDEFGHIJKLMNOPQRSTUVWXYZ[\\]^_\u2018abcdefghijklmnopqrstuvwxyz{|}~';

var CHARACTER_NAMES = ['Tidus','Yuna','Auron','Kimahri','Wakka','Lulu','Rikku','Seymour',
    'Valefor','Ifrit','Ixion','Shiva','Bahamut','Anima','Yojimbo',
    'Cindy','Sandy','Mindy','Dummy','Dummy2'];

var BTL_WIN_SIZE = 0xF0;
var BTL_WIN_TYPES = {};
BTL_WIN_TYPES[0x00] = 'Main'; BTL_WIN_TYPES[0x01] = 'Black Magic';
BTL_WIN_TYPES[0x02] = 'White Magic'; BTL_WIN_TYPES[0x03] = 'Skill';
BTL_WIN_TYPES[0x04] = 'Overdrive'; BTL_WIN_TYPES[0x05] = 'Summon';
BTL_WIN_TYPES[0x06] = 'Item'; BTL_WIN_TYPES[0x07] = 'Weapon';
BTL_WIN_TYPES[0x0A] = 'Change'; BTL_WIN_TYPES[0x0C] = 'Left';
BTL_WIN_TYPES[0x0D] = 'Right'; BTL_WIN_TYPES[0x0E] = 'Special';
BTL_WIN_TYPES[0x0F] = 'Armor'; BTL_WIN_TYPES[0x11] = 'Use';
BTL_WIN_TYPES[0x14] = 'Mix'; BTL_WIN_TYPES[0x15] = 'Spare Change';
BTL_WIN_TYPES[0x16] = 'Yojimbo Pay';

var NODE_TYPE_NAMES = {
    0x00:'Lock Lv.3', 0x01:'Empty',
    0x02:'Strength +1', 0x03:'Strength +2', 0x04:'Strength +3', 0x05:'Strength +4',
    0x06:'Defense +1', 0x07:'Defense +2', 0x08:'Defense +3', 0x09:'Defense +4',
    0x0A:'Magic +1', 0x0B:'Magic +2', 0x0C:'Magic +3', 0x0D:'Magic +4',
    0x0E:'Magic Def +1', 0x0F:'Magic Def +2', 0x10:'Magic Def +3', 0x11:'Magic Def +4',
    0x12:'Agility +1', 0x13:'Agility +2', 0x14:'Agility +3', 0x15:'Agility +4',
    0x16:'Luck +1', 0x17:'Luck +2', 0x18:'Luck +3', 0x19:'Luck +4',
    0x1A:'Evasion +1', 0x1B:'Evasion +2', 0x1C:'Evasion +3', 0x1D:'Evasion +4',
    0x1E:'Accuracy +1', 0x1F:'Accuracy +2', 0x20:'Accuracy +3', 0x21:'Accuracy +4',
    0x22:'HP +200', 0x23:'HP +300', 0x24:'MP +40', 0x25:'MP +20', 0x26:'MP +10',
    0x27:'Lock Lv.1', 0x28:'Lock Lv.2', 0x29:'Lock Lv.4',
    0xFF:'NULL'
};

var SCENE_NAMES = {};
SCENE_NAMES[0x01] = 'Main Menu'; SCENE_NAMES[0x02] = 'Abilities';
SCENE_NAMES[0x15] = 'Overdrive'; SCENE_NAMES[0x04] = 'Equip';
SCENE_NAMES[0x06] = 'Status'; SCENE_NAMES[0x07] = 'Aeons';
SCENE_NAMES[0x09] = 'Formation'; SCENE_NAMES[0x0A] = 'Config';
SCENE_NAMES[0x0B] = 'Help';

// ══════════════════════════════════════════
// SECTION 3: Game Addresses
// ══════════════════════════════════════════

// Battle system
var btlBaseAddr       = rva(0xD2A8D0);
var btlStateAddr      = rva(0xD2A8D0 + 0x10);
var btlWinArrayAddr   = rva(0xF3C910);
var btlCurWinIdxAddr  = rva(0x1FCC092);
var btlCommandBinAddr = rva(0xD2A8D0 + 0x5C);
var btlItemBinAddr    = rva(0xD2A8D0 + 0x70);
var saveDataAddr      = rva(0xD2CA90);
var INVENTORY_IDS_OFF = 0x3ECC;
var INVENTORY_CNT_OFF = 0x40CC;
var monsterChrsAddr   = rva(0xD34460);
var playerChrsAddr    = rva(0xD334CC);
var CHR_SIZE = 0xF90;
var CHR_RAM_OFFSET = 0x540;
var CHR_IN_BATTLE = 0xDC8;
var btlRewardAddr = rva(0x1F10EA0);
var helpTextAddr  = rva(0xF3F4F8);

// Field menu
var fieldMenuCursorAddr = rva(0x1471508);
var fieldMenuPlayerAddr = rva(0x147150C);
var fieldMenuItemTable  = rva(0x1471528);
var eventIdAddr         = rva(0xEFBBF8);

// TkWindow
var tkWinArrayAddr = rva(0x14676B0);
var tkCurArrayAddr = rva(0x1468A50);

// Sphere Grid
var sgridIsOpenAddr    = rva(0x1685f70);
var sgridEnginePtrAddr = rva(0x1F05834);

// Config menu
var configCursorAddr   = rva(0x146A404);
var configCountAddr    = rva(0x146A408);
var configArrayPtrAddr = rva(0x146A40C);
var configSpecialIdx   = rva(0x146A3F8);

// Navigation
var NAV_ACTOR_SIZE = 0x880;
var NAV_MAX_ACTORS = 128;
var NAV_UNITS_PER_STEP = 11.13;
var NAV_MAX_DISTANCE = 500;
var RVA_ACTOR_ARRAY  = 0x1FC44E4;
var RVA_TRIS     = 0xF01A44;
var RVA_TRICOUNT = 0xF01A48;
var RVA_VERTICES = 0xF01A4C;
var RVA_SCALE    = 0xF01A50;
var RVA_CURRENT_WORKER = 0xF270A4;
var RVA_ATEL_CONTROLLER = 0xF26AE8;

// Atel namespace tables: each entry is 0x10 bytes (init, exec, ret_float, ret_int).
// Function IDs are encoded as (namespace << 12) | index.
// NOTE: these RVAs were previously wrong (off by 0x400000). Fahrenheit's values
// are already runtime RVAs used directly with ptr_at(base + rva). The game's
// data section lives much higher than its code section — 0x800000+.
var ATEL_NAMESPACES = [
    { id: 0x0, rva: 0x850050, count: 616 }, // Common
    { id: 0x1, rva: 0x852BE0, count: 30 },  // Math
    { id: 0x4, rva: 0x888D88, count: 71 },  // SgEvent
    { id: 0x5, rva: 0x8891F8, count: 145 }, // ChEvent
    { id: 0x6, rva: 0x843988, count: 138 }, // Camera
    { id: 0x7, rva: 0x842618, count: 296 }, // Battle
    { id: 0x8, rva: 0x85DC90, count: 108 }, // Map
    { id: 0x9, rva: 0x85D8C0, count: 1 },   // Mount
    { id: 0xB, rva: 0x840E20, count: 16 },  // Movie
    { id: 0xC, rva: 0x852DD8, count: 94 },  // Debug
    { id: 0xD, rva: 0x885EB0, count: 1 }    // AbiMap
];

// Native RVAs of the Atel-callable wrappers that script CALLs use to do a map
// transition. All four of these internally invoke FUN_0086fed0 (AtelSetEventJump2).
// Found by grepping decompile for callers of 0x46FED0.
var EXIT_FN_RVAS = [
    0x4580C0, // FUN_008580c0 — fade=1 wrapper
    0x458220, // FUN_00858220 — fade=1 wrapper
    0x458370, // FUN_00858370 — fade=0 wrapper
    0x4584B0, // FUN_008584b0 — fade=0 wrapper
    0x48E9D0, // TkSetEventJump
    0x48EA60, // TkSetEventJumpMapNo
    // Higher-level map-transition hub and its wrappers (from Ghidra decompile search)
    0x45BA20, // FUN_0065ba20 — central map transition hub
    0x4403B0, // FUN_006403b0 — wrapper calling FUN_0065ba20(1)
    0x458C70  // FUN_00658c70 — wrapper calling FUN_0065ba20(0)
];

logFile('Addresses resolved');

// ══════════════════════════════════════════
// SECTION 4: Text Decoders + Helpers
// (Copied verbatim from universal_menu_battle_reader.js)
// ══════════════════════════════════════════

function decodeFFXSegments(addr, maxLen, maxOps) {
    var segments = [];
    var current = '';
    try {
        var bytes = new Uint8Array(addr.readByteArray(maxLen));
        var i = 0;
        var opCount = 0;
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

function decodeFFXString(addr, maxLen) {
    var result = '';
    try {
        var bytes = new Uint8Array(addr.readByteArray(maxLen));
        var i = 0;
        while (i < maxLen) {
            var b = bytes[i];
            if (b === 0x00) break;
            if (b === 0x03 || b === 0x0A) { result += ' '; i++; continue; }
            if (b === 0x13 && i + 1 < maxLen) {
                var chrIdx = bytes[i + 1] - 0x30;
                try {
                    var pArr = playerChrsAddr.readPointer();
                    if (!pArr.isNull() && chrIdx >= 0 && chrIdx < 0x1f) {
                        var ns = decodeFFXSegments(pArr.add(chrIdx * CHR_SIZE + CHR_RAM_OFFSET), 40, 0);
                        if (ns.length > 0) result += ns[0];
                    }
                } catch(e2) {
                    if (chrIdx < CHARACTER_NAMES.length) result += CHARACTER_NAMES[chrIdx];
                }
                i += 2; continue;
            }
            if (b === 0x09 || b === 0x10) { i += 2; continue; }
            if (b < 0x2B) { i++; continue; }
            var idx = 0;
            if (b < 0x30) { idx = (b - 0x2B) * 0xD0; i++; if (i >= maxLen) break; b = bytes[i]; }
            idx += (b - 0x30);
            if (idx >= 0 && idx < CHARSET.length) result += CHARSET[idx];
            i++;
        }
    } catch(e) {}
    return result.trim();
}

function cleanText(s) {
    while (s.length > 1 && /^[^A-Za-z0-9 ]/.test(s[0])) s = s.substring(1);
    if (s.length > 1 && /^[0-9][A-Za-z]/.test(s)) s = s.substring(1);
    return s.trim();
}

// NativeFunction: call game's text functions directly
var msMenuGetTextFn = new NativeFunction(rva(0x38FD40), 'pointer', ['int', 'int', 'int']);
var getConfigLangFn = new NativeFunction(rva(0x3852B0), 'int', []);
var locStringCopyFn = new NativeFunction(rva(0x241480), 'void', ['int', 'pointer']);

function resolveCommandText(commandId, excelBinAddr) {
    try {
        var excelPtr = excelBinAddr.readPointer();
        if (excelPtr.isNull()) return null;
        var hdr = excelPtr.add(0x8);
        var firstIdx = hdr.readU16(), lastIdx = hdr.add(2).readU16();
        var elementSize = hdr.add(4).readU16(), dataLength = hdr.add(6).readU16();
        var dataStart = hdr.add(8).readU32();
        var excelIdx = commandId & 0xFFF;
        if (excelIdx < firstIdx || excelIdx > lastIdx) return null;
        var elementAddr = excelPtr.add(dataStart).add((excelIdx - firstIdx) * elementSize);
        var nameOffset = elementAddr.readU16();
        var textAddr = excelPtr.add(dataStart + dataLength + nameOffset);
        var segments = decodeFFXSegments(textAddr, 64, 0);
        return (segments.length > 0) ? segments[0] : null;
    } catch(e) { return null; }
}

function resolveLocString(textId) {
    try {
        var buf = Memory.alloc(256);
        locStringCopyFn(textId, buf);
        var segs = decodeFFXSegments(buf, 256, 0);
        return (segs.length > 0) ? segs[0] : null;
    } catch(e) { return null; }
}

function resolveConfigText(textId) {
    try {
        var lang = getConfigLangFn();
        var textPtr = msMenuGetTextFn(7, textId, lang);
        if (!textPtr.isNull()) {
            var segs = decodeFFXSegments(textPtr, 256, 0);
            return (segs.length > 0) ? segs[0] : null;
        }
    } catch(e) {} return null;
}

function readConfigItem(cursorIdx) {
    try {
        var arrayPtr = configArrayPtrAddr.readPointer();
        if (arrayPtr.isNull()) return null;
        var itemPtr = arrayPtr.add(cursorIdx * 4).readPointer();
        if (itemPtr.isNull()) return null;
        return { numOptions: itemPtr.add(0x04).readS32(), curSelection: itemPtr.add(0x08).readS32(),
            visible: itemPtr.add(0x0C).readS32(), textId: itemPtr.add(0x18).readU16(), ptr: itemPtr };
    } catch(e) { return null; }
}

// ══════════════════════════════════════════
// SECTION 5: Battle Helpers
// ══════════════════════════════════════════

function readBtlWindow(idx) {
    var winBase = btlWinArrayAddr.add(idx * BTL_WIN_SIZE);
    try {
        return { state: winBase.add(0x01).readU8(), type: winBase.add(0x06).readU16(),
            chrId: winBase.add(0x08).readU8(), submenuDepth: winBase.add(0x0B).readU8(),
            winCmdId: winBase.add(0x0C).readU16(), selCmdIdRender: winBase.add(0x16).readU16(),
            selCmdId: winBase.add(0x1E).readU16(), cmdListPtr: winBase.add(0x20).readPointer(),
            cmdListLen: winBase.add(0x24).readU32(), scrollPos: winBase.add(0x40).readU8(),
            selIdx: winBase.add(0x42).readU8(), validSelAmt: winBase.add(0x44).readU32() };
    } catch(e) { return null; }
}

function readMonsterInfo(monIdx) {
    try {
        var arrayPtr = monsterChrsAddr.readPointer();
        if (arrayPtr.isNull()) return null;
        var chrBase = arrayPtr.add(monIdx * CHR_SIZE);
        if (chrBase.add(CHR_IN_BATTLE).readU8() === 0) return null;
        var ramBase = chrBase.add(CHR_RAM_OFFSET);
        var nameSegs = decodeFFXSegments(ramBase, 40, 0);
        var name = (nameSegs.length > 0) ? nameSegs[0] : 'Monster ' + monIdx;
        var currentHp = ramBase.add(0x1A4).readS32(), maxHp = ramBase.add(0x54).readS32();
        return { name: name, hpPct: (maxHp > 0) ? Math.round(currentHp / maxHp * 100) : 0, currentHp: currentHp, maxHp: maxHp };
    } catch(e) { return null; }
}

function readPlayerInfo(chrIdx) {
    try {
        var arrayPtr = playerChrsAddr.readPointer();
        if (arrayPtr.isNull()) return null;
        var chrBase = arrayPtr.add(chrIdx * CHR_SIZE);
        if (chrBase.add(CHR_IN_BATTLE).readU8() === 0) return null;
        var ramBase = chrBase.add(CHR_RAM_OFFSET);
        return { name: (chrIdx < CHARACTER_NAMES.length) ? CHARACTER_NAMES[chrIdx] : ('Player ' + chrIdx),
            currentHp: ramBase.add(0x1A4).readS32(), maxHp: ramBase.add(0x54).readS32(),
            currentMp: ramBase.add(0x1A8).readS32(), maxMp: ramBase.add(0x58).readS32() };
    } catch(e) { return null; }
}

function getItemQuantity(itemId) {
    try {
        var invIds = saveDataAddr.add(INVENTORY_IDS_OFF);
        for (var i = 0; i < 256; i++) {
            var id = invIds.add(i * 2).readU16();
            if (id === itemId) return saveDataAddr.add(INVENTORY_CNT_OFF + i).readU8();
            if (id === 0) break;
        }
    } catch(e) {} return 0;
}

var inOurWeaponCall = false;
var msWeaponNameFn = new NativeFunction(rva(0x3A0C70), 'pointer', ['uint', 'uint', 'int', 'pointer']);
var equipListNames = [];

function readCommandName(win) {
    if (win.cmdListPtr.isNull() || win.cmdListLen === 0) return null;
    if (win.selIdx >= win.cmdListLen) return null;
    try {
        var cmdId = win.cmdListPtr.add(win.selIdx * 2).readU16();
        var prefix = cmdId & 0xF000;
        if (win.type === 0x07 || win.type === 0x0F) {
            return (win.selIdx < equipListNames.length) ? equipListNames[win.selIdx] : 'equipment ' + win.selIdx;
        }
        if (win.type === 0x0A || win.type === 0x05) {
            var chrIdx = cmdId & 0xFF;
            var pArr = playerChrsAddr.readPointer();
            if (!pArr.isNull() && chrIdx < 0x1f) {
                var ns = decodeFFXSegments(pArr.add(chrIdx * CHR_SIZE + CHR_RAM_OFFSET), 40, 0);
                if (ns.length > 0) return ns[0];
            }
            return (chrIdx < CHARACTER_NAMES.length) ? CHARACTER_NAMES[chrIdx] : 'chr' + chrIdx;
        }
        if (prefix === 0x2000) {
            var name = resolveCommandText(cmdId, btlItemBinAddr);
            if (name) return name + ' (' + getItemQuantity(cmdId) + ')';
        } else if (prefix === 0x3000) {
            return resolveCommandText(cmdId, btlCommandBinAddr);
        }
        var name2 = resolveCommandText(cmdId, btlCommandBinAddr);
        if (!name2) name2 = resolveCommandText(cmdId, btlItemBinAddr);
        return name2 || ('cmd:0x' + cmdId.toString(16));
    } catch(e) { return null; }
}

function getCommandPartyPreview(cmdId) {
    try {
        var prefix = cmdId & 0xF000;
        var binAddr = (prefix === 0x2000) ? btlItemBinAddr : btlCommandBinAddr;
        var excelPtr = binAddr.readPointer();
        if (excelPtr.isNull()) return null;
        var hdr = excelPtr.add(0x8);
        var firstIdx = hdr.readU16(), lastIdx = hdr.add(2).readU16();
        var elementSize = hdr.add(4).readU16(), dataStart = hdr.add(8).readU32();
        var excelIdx = cmdId & 0xFFF;
        if (excelIdx < firstIdx || excelIdx > lastIdx) return null;
        var elementAddr = excelPtr.add(dataStart).add((excelIdx - firstIdx) * elementSize);
        var pp = elementAddr.add(0x22).readU8();
        return { showHp: (pp & (1 << 6)) !== 0, showMp: (pp & (1 << 1)) !== 0 };
    } catch(e) { return null; }
}

function readTkWindowContext(w) {
    var winPtr = tkWinArrayAddr.add(w * 4).readPointer();
    var curPtr = tkCurArrayAddr.add(w * 4).readPointer();
    if (winPtr.isNull() || curPtr.isNull()) return null;
    var vis = curPtr.add(1).readU8(), idx = curPtr.add(2).readU8(), items = curPtr.add(5).readU8();
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

function readSgridNode(engineBase, nodeIdx) {
    try {
        var nodeBase = engineBase.add(0x808 + nodeIdx * 0x28);
        var rawType = nodeBase.add(0x6).readS16();
        var nodeType = (rawType === -1) ? 0xFF : (rawType & 0xFF);
        return { type: nodeType, typeName: NODE_TYPE_NAMES[nodeType] || null,
            activatedBy: nodeBase.add(0x21).readU8(), props: nodeBase.add(0x22).readU8(),
            x: nodeBase.add(0x0).readS16(), y: nodeBase.add(0x2).readS16() };
    } catch(e) { return null; }
}

function readSgridSlv(chrIdx) {
    try {
        var plySave = saveDataAddr.add(0x55CC + chrIdx * 0x94);
        return { available: plySave.add(0x3B).readU8(), spent: plySave.add(0x3C).readU8() };
    } catch(e) { return null; }
}

// Battle rewards
function announceRewardsPhase1() {
    var apParts = [];
    for (var i = 0; i < 18; i++) {
        if (!btlRewardAddr.add(i).readU8()) continue;
        var ap = btlRewardAddr.add(0x80 + i * 4).readU32();
        apParts.push((i < CHARACTER_NAMES.length ? CHARACTER_NAMES[i] : 'chr'+i) + ' ' + ap + ' AP');
    }
    if (apParts.length > 0) speak(apParts.join(', '), false);
}

function announceRewardsPhase2() {
    var itemCnt = btlRewardAddr.add(0xD0).readU8();
    if (itemCnt > 0) {
        var parts = [];
        for (var ii = 0; ii < itemCnt && ii < 8; ii++) {
            var itemId = btlRewardAddr.add(0xD4 + ii * 2).readU16();
            var itemAmt = btlRewardAddr.add(0xE4 + ii).readU8();
            var itemName = resolveCommandText(itemId, btlItemBinAddr);
            parts.push((itemName || 'item') + ' x' + itemAmt);
        }
        speak(parts.join(', '), false);
    }
    var gearCnt = btlRewardAddr.add(0xD2).readU8();
    for (var gi = 0; gi < gearCnt && gi < 8; gi++) {
        speak('Equipment obtained', false);
    }
    var gil = btlRewardAddr.add(0xCC).readU32();
    if (gil > 0) speak(gil + ' Gil', false);
}

logFile('Helpers loaded');

// ══════════════════════════════════════════
// SECTION 6: Hooks
// ══════════════════════════════════════════

// State tracking
var prevBtlState = 0, prevBtlWinIdx = -1, prevBtlWinState = 0, prevBtlWinType = -1;
var prevBtlSelIdx = -1, prevBtlSelCmd = -1, btlWinAnnounced = false;
var inTargeting = false, prevTargetChrs = 0, prevTargetMons = 0;
var targetShowHp = true, targetShowMp = false, lastNonZeroCmdId = 0;
var inMainMenu = false, mainMenuLastFrame = 0, frameCount = 0;
var msMenuTextCache = {}, msMenuDescCache = {};
var prevFieldCursor = -1, prevFieldPlayer = -1;
var activeSubScene = -1, prevSubScene = -1;
var prevSgridOpen = false, prevSgridSelectedNode = -1, prevSgridChrId = -1;
var prevSgridNodeChangeText = '', prevSgridActivatedText = '';
var prevTkCursors = {}, prevTkVisible = {}, prevTkTextPtrs = {};
var initialTkTextPtrs = {}, isTkPopup = {};
var TITLE_ITEMS = ['New Game', 'Load'];
var prevConfigCursor = -1, prevConfigSelection = -1, configActive = false;
var rewardsAnnounced = false;
var PERM_STATUS_NAMES = ['KO','zombie','petrify','poison','power break','magic break',
    'armor break','mental break','confuse','berserk','provoke','threaten'];
var TEMP_STATUS_NAMES = ['sleep','silence','darkness','shell','protect','reflect',
    'nul-water','nul-fire','nul-thunder','nul-ice','regen','haste','slow'];
var EXTRA_STATUS_NAMES = ['scan','','','','','','shield','boost','eject','auto-life',
    'curse','defend','guard','sentinel','doom'];
var prevStatusPerm = {}, prevStatusTemp = {}, prevStatusExtra = {}, prevStatDeath = {};

// MsMenuGetText hook
try {
    Interceptor.attach(rva(0x38FD40), {
        onEnter: function(args) {
            this.arg0 = args[0].toInt32();
            this.arg1 = args[1].toInt32();
            this.arg2 = args[2].toInt32();
        },
        onLeave: function(retval) {
            if (retval.isNull()) return;
            var segs = null;
            try { segs = decodeFFXSegments(retval, 256, 0); } catch(e) {}
            if (this.arg0 === 9) {
                mainMenuLastFrame = frameCount;
                if (segs && segs.length > 0) msMenuTextCache[this.arg1] = segs[0];
                if (segs && segs.length > 2 && segs[1] === '-') msMenuDescCache[this.arg1] = segs[2];
            }
            if (this.arg0 === 4 && !rewardsAnnounced) {
                rewardsAnnounced = true;
                try { announceRewardsPhase1(); announceRewardsPhase2(); } catch(e) {}
            }
        }
    });
    logFile('MsMenuGetText hooked');
} catch(e) { logFile('FAIL MsMenuGetText: ' + e.message); }

// Scene activation hook
try {
    Interceptor.attach(rva(0x4AA0B0), {
        onEnter: function(args) {
            var sceneId = args[0].toInt32();
            activeSubScene = sceneId;
            if (sceneId !== 0x01) {
                var sceneName = SCENE_NAMES[sceneId] || ('scene 0x' + sceneId.toString(16));
                speak(sceneName, true);
            }
        }
    });
    logFile('Scene activate hooked');
} catch(e) { logFile('FAIL Scene activate: ' + e.message); }

// Config fn_ctrl hook
try {
    Interceptor.attach(rva(0x4CB0B0), {
        onEnter: function(args) { this.param1 = args[0]; },
        onLeave: function() {
            var state = this.param1.add(0x1C).readS32();
            if (state < 3) { if (!configActive) { configActive = true; } return; }
            if (state > 3) return;
            configActive = true;
            var cursor = configCursorAddr.readU8();
            var item = readConfigItem(cursor);
            if (!item) return;
            var cursorChanged = (cursor !== prevConfigCursor);
            var selectionChanged = (item.curSelection !== prevConfigSelection);
            if (!cursorChanged && !selectionChanged) return;
            var specialIdx = configSpecialIdx.readS32();
            var isSpecial = (cursor === specialIdx);
            var settingName = '', optionName = '';
            if (isSpecial) {
                settingName = resolveLocString(0x1CD) || 'Music';
                var musicOptIds = [0x1CE, 0x1D0];
                optionName = (item.curSelection >= 0 && item.curSelection < musicOptIds.length)
                    ? (resolveLocString(musicOptIds[item.curSelection]) || 'option ' + item.curSelection) : 'option ' + item.curSelection;
            } else {
                settingName = resolveConfigText(item.textId) || ('setting ' + item.textId);
                try { var optTextId = item.ptr.add(0x1C + item.curSelection * 4).readU32();
                    optionName = resolveConfigText(optTextId) || ('option ' + item.curSelection);
                } catch(e) { optionName = 'option ' + item.curSelection; }
            }
            speak(cursorChanged ? (settingName + ': ' + optionName) : optionName, true);
            prevConfigCursor = cursor; prevConfigSelection = item.curSelection;
        }
    });
    logFile('Config fn_ctrl hooked');
} catch(e) { logFile('FAIL Config fn_ctrl: ' + e.message); }

// Damage display hook
try {
    Interceptor.attach(rva(0x39FA20), {
        onEnter: function(args) {
            var chrId = args[0].toInt32() & 0xFF;
            var dmgType = args[1].toInt32();
            var value = args[2].toInt32();
            if (value === 0) return;
            var name = '';
            try {
                if (chrId < 0x1f) {
                    var pArr = playerChrsAddr.readPointer();
                    var chrPtr = pArr.add(chrId * CHR_SIZE);
                    var nameSegs = decodeFFXSegments(chrPtr.add(CHR_RAM_OFFSET), 40, 0);
                    name = (nameSegs.length > 0) ? nameSegs[0] : '';
                    if (name === '' && chrId < CHARACTER_NAMES.length) name = CHARACTER_NAMES[chrId];
                }
                if (name === '') name = 'chr' + chrId;
            } catch(e) { name = 'chr' + chrId; }
            var absVal = (value < 0) ? -value : value;
            var isHeal = (value < 0);
            var msg = (dmgType === 3)
                ? (isHeal ? name + ': restored ' + absVal + ' MP' : name + ': ' + absVal + ' MP damage')
                : (isHeal ? name + ': healed for ' + absVal : name + ': ' + absVal + ' damage');
            speak(msg, false);
        }
    });
    logFile('Damage display hooked');
} catch(e) { logFile('FAIL Damage display: ' + e.message); }

// MsWeaponName hook — cache equipment names
try {
    Interceptor.attach(rva(0x3A0C70), {
        onLeave: function(retval) {
            if (inOurWeaponCall) return;
            if (!retval.isNull()) {
                try {
                    var name = decodeFFXString(retval, 64);
                    if (name.length > 0) equipListNames.push(name);
                } catch(e) {}
            }
        }
    });
    logFile('MsWeaponName hooked');
} catch(e) { logFile('FAIL MsWeaponName: ' + e.message); }

logFile('All hooks installed');

// ══════════════════════════════════════════
// SECTION 7: Navigation
// ══════════════════════════════════════════
//
// SCAN TRIGGERING:
//   - PRIMARY: automatic scan on map transitions. Detected via navmesh tri_count
//     going 0 → positive (first field map after loading) AND via navmesh pointer
//     changes (map-to-map transitions where tri_count stays positive throughout).
//     Scan retries for up to 150 frames until the actor array is populated.
//   - FALLBACK: manual rescan via backtick hotkey. Only used if automatic scan
//     somehow fails or the user wants to force refresh.
//
// EXIT DETECTION:
//   Real exits are identified by scanning each worker's Atel bytecode for CALL
//   opcodes targeting map-transition functions (AtelSetEventJump2, TkSetEventJump,
//   TkSetEventJumpMapNo). This is static analysis done on map load, not runtime
//   discovery — exits are known immediately on first visit to any map.

var navEntities = [];
var navSelectedIdx = 0;
var navFieldActive = false;
var navPrevTriCount = 0;
var navPrevTriPtr = 0;
var navScanPending = false;
var navScanRetries = 0;

// Category filter
var NAV_CATEGORIES = ['All', 'Exit', 'Event', 'Save Sphere', 'Treasure Chest', 'NPC', 'Object'];
var navCategoryIdx = 0;  // 0 = All

function navClassify(id) {
    if (id === 0x5001) return 'Save Sphere';
    if (id === 0x5002) return 'Treasure Chest';
    if (id >= 0x2000 && id < 0x5000) return 'NPC';
    if (id <= 0x0012) return 'Player';
    return 'Object';
}

function navLabel(e) {
    return (e.name !== '?' && e.name !== e.category) ? e.category + ' "' + e.name + '"' : e.category;
}

function navGetActorsBase() {
    var p = rva(RVA_ACTOR_ARRAY).readPointer();
    return p.isNull() ? null : p;
}

function navGetPlayerPos() {
    var ab = navGetActorsBase();
    if (!ab) return null;
    return { x: ab.add(0x0C).readFloat(), y: ab.add(0x10).readFloat(), z: ab.add(0x14).readFloat() };
}

function navDistXZ(a, b) {
    var dx = a.x - b.x, dz = a.z - b.z;
    return Math.sqrt(dx * dx + dz * dz);
}

// Camera-relative compass. ms_camera_now at RVA 0x1F11430 points to the current
// camera struct (0xda0 bytes). Within the struct:
//   +0x40  = eye position (3 floats: x, y, z)
//   +0x65c = target position (3 floats: x, y, z)
// Forward direction = target - eye. Projected to XZ plane gives screen-up ("north").
// Confirmed from decompile at layer2_game_logic.txt line 347242.
var RVA_MS_CAMERA_NOW = 0x1F11430;
var CAM_EYE_OFFSET    = 0x40;
var CAM_TARGET_OFFSET = 0x65C;

function navGetScreenNorthAngle() {
    try {
        var camPtr = rva(RVA_MS_CAMERA_NOW).readPointer();
        if (camPtr.isNull() || camPtr.compare(ptr(0x10000)) <= 0) return null;
        var ex = camPtr.add(CAM_EYE_OFFSET).readFloat();
        var ez = camPtr.add(CAM_EYE_OFFSET + 8).readFloat();
        var tx = camPtr.add(CAM_TARGET_OFFSET).readFloat();
        var tz = camPtr.add(CAM_TARGET_OFFSET + 8).readFloat();
        if (isNaN(ex) || isNaN(ez) || isNaN(tx) || isNaN(tz)) return null;
        // Both axes flipped: the direction from target → eye (not eye → target).
        // Confirmed: east/right and south/down were both inverted — flipping both fixes both.
        var fx = ex - tx, fz = ez - tz;
        if (Math.abs(fx) < 0.001 && Math.abs(fz) < 0.001) return null;
        return Math.atan2(fz, fx);
    } catch (e) { return null; }
}

function navRelativeDir(dx, dz) {
    if (Math.abs(dx) < 0.01 && Math.abs(dz) < 0.01) return 'here';
    var northAngle = navGetScreenNorthAngle();
    if (northAngle === null) return 'unknown';
    var worldAngle = Math.atan2(dz, dx);
    var rel = worldAngle - northAngle;
    while (rel > Math.PI) rel -= 2 * Math.PI;
    while (rel < -Math.PI) rel += 2 * Math.PI;
    var deg = rel * 180 / Math.PI;
    if (deg >= -22.5 && deg < 22.5) return 'north';
    if (deg >= 22.5 && deg < 67.5) return 'northwest';
    if (deg >= 67.5 && deg < 112.5) return 'west';
    if (deg >= 112.5 && deg < 157.5) return 'southwest';
    if (deg >= -67.5 && deg < -22.5) return 'northeast';
    if (deg >= -112.5 && deg < -67.5) return 'east';
    if (deg >= -157.5 && deg < -112.5) return 'southeast';
    return 'south';
}

function navScan() {
    var ab = navGetActorsBase();
    if (!ab) { speak('No actors', true); return; }
    var pp = navGetPlayerPos();
    if (!pp || isNaN(pp.x)) return;

    // Load navmesh for reachability filtering
    var nav = navReadNavmesh();
    var playerTri = nav ? navFindTri(nav, pp.x, pp.z) : -1;

    // Iterate the full actor array. The game uses 0xCDCD (MSVC debug fill) in
    // unused slots and chr_enabled=0 for disabled actors — our filter catches
    // both. Don't use current_worker.event_actor_total: that's per-worker script
    // state, not a global actor count, and varies between scans depending on
    // which worker is currently executing.
    var actorLimit = NAV_MAX_ACTORS;

    navEntities = [];
    var filtered = 0;
    for (var i = 1; i < actorLimit; i++) {
        try {
            var actor = ab.add(i * NAV_ACTOR_SIZE);
            var chrId = actor.readU16();
            var en = actor.add(2).readU16();
            if (chrId === 0xCDCD || en === 0xCDCD || en === 0 || chrId === 0) continue;
            var cat = navClassify(chrId);
            if (cat === 'Player') continue;
            var pos = { x: actor.add(0x0C).readFloat(), y: actor.add(0x10).readFloat(), z: actor.add(0x14).readFloat() };
            if (isNaN(pos.x) || isNaN(pos.z)) continue;
            var dist = navDistXZ(pp, pos);
            if (isNaN(dist)) continue;
            // No distance cap — event_actor_total already filters to current-map actors.

            // Navmesh reachability filter: entity must be ON or NEAR a walkable
            // triangle. "On" catches NPCs standing on floor; "near" catches
            // interactable props (dresser, fire pit, chest) that sit just off
            // walkable tiles. Threshold rejects entities through walls / off-map.
            if (nav) {
                var NEAR_THRESHOLD_SQ = 9; // within 3 world units of any walkable tri
                var foundNear = false;
                for (var ti = 0; ti < nav.triCount; ti++) {
                    var t = nav.tris[ti];
                    if (t.coll !== 0 && t.coll !== 2) continue;
                    // Bounding-box pre-filter (with NEAR_THRESHOLD expansion)
                    var minX = Math.min(t.vxs[0], t.vxs[1], t.vxs[2]) - 3;
                    var maxX = Math.max(t.vxs[0], t.vxs[1], t.vxs[2]) + 3;
                    var minZ = Math.min(t.vzs[0], t.vzs[1], t.vzs[2]) - 3;
                    var maxZ = Math.max(t.vzs[0], t.vzs[1], t.vzs[2]) + 3;
                    if (pos.x < minX || pos.x > maxX || pos.z < minZ || pos.z > maxZ) continue;
                    // Strict point-in-triangle (catches entities ON walkable)
                    var v0x = t.vxs[2] - t.vxs[0], v0z = t.vzs[2] - t.vzs[0];
                    var v1x = t.vxs[1] - t.vxs[0], v1z = t.vzs[1] - t.vzs[0];
                    var v2x = pos.x - t.vxs[0], v2z = pos.z - t.vzs[0];
                    var d00 = v0x*v0x + v0z*v0z, d01 = v0x*v1x + v0z*v1z;
                    var d02 = v0x*v2x + v0z*v2z, d11 = v1x*v1x + v1z*v1z;
                    var d12 = v1x*v2x + v1z*v2z;
                    var inv = d00*d11 - d01*d01;
                    if (Math.abs(inv) >= 0.0001) {
                        var invN = 1/inv;
                        var u = (d11*d02 - d01*d12) * invN;
                        var v = (d00*d12 - d01*d02) * invN;
                        if (u >= -0.02 && v >= -0.02 && (u + v) <= 1.02) { foundNear = true; break; }
                    }
                    // Not inside — check distance to each edge
                    for (var ei = 0; ei < 3; ei++) {
                        var ax = t.vxs[ei], az = t.vzs[ei];
                        var bx = t.vxs[(ei+1)%3], bz = t.vzs[(ei+1)%3];
                        var ex = bx - ax, ez = bz - az;
                        var el = ex*ex + ez*ez;
                        if (el < 0.0001) continue;
                        var tt = ((pos.x-ax)*ex + (pos.z-az)*ez) / el;
                        if (tt < 0) tt = 0; else if (tt > 1) tt = 1;
                        var cx = ax + ex*tt, cz = az + ez*tt;
                        var ddx = pos.x-cx, ddz = pos.z-cz;
                        if (ddx*ddx + ddz*ddz <= NEAR_THRESHOLD_SQ) { foundNear = true; break; }
                    }
                    if (foundNear) break;
                }
                if (!foundNear) { filtered++; continue; }
            }

            var name = '?';
            try { var np = actor.add(0x04).readPointer(); if (!np.isNull()) name = np.readCString() || '?'; } catch(e) {}

            // Diagnostic dump — log key actor fields so we can find what
            // distinguishes active entities from stale/placeholder duplicates.
            try {
                logFile('[ACTOR ' + i + '] chrId=0x' + chrId.toString(16) + ' en=' + en +
                    ' name=' + name +
                    ' pos=(' + pos.x.toFixed(2) + ',' + pos.y.toFixed(2) + ',' + pos.z.toFixed(2) + ')' +
                    ' rot=' + actor.add(0x158).readFloat().toFixed(3) +
                    ' speed=' + actor.add(0x154).readFloat().toFixed(3) +
                    ' dir=' + actor.add(0x168).readFloat().toFixed(3) +
                    ' var1=0x' + actor.add(0x824).readU32().toString(16));
            } catch(e) {}

            navEntities.push({ name: name, category: cat, chrId: chrId, pos: pos, dist: dist, actorIdx: i });
        } catch(e) {}
    }

    // Exit triggers (pass nav for reachability filter)
    navScanExits(pp, nav);

    navEntities.sort(function(a, b) { return a.dist - b.dist; });
    navSelectedIdx = 0;
    logFile('[NAV] Scan: ' + navEntities.length + ' entities (' + filtered + ' filtered by navmesh)');
}

// ── Atel bytecode scanner: identifies real map-transition exits ──
// Finds Atel function IDs that map to native exit functions, then scans each
// worker's bytecode for CALL opcodes (0xB5) targeting those IDs.
var exitAtelIds = null; // lazily resolved on first scan

function findExitAtelIds() {
    var ids = {};
    try {
        for (var n = 0; n < ATEL_NAMESPACES.length; n++) {
            var ns = ATEL_NAMESPACES[n];
            var tableBase = rva(ns.rva);
            // Diagnostic: dump the first entry of each namespace so we can see
            // what's actually at these RVAs
            try {
                var p0 = tableBase.readPointer();
                var p1 = tableBase.add(4).readPointer();
                var p2 = tableBase.add(8).readPointer();
                var p3 = tableBase.add(0xC).readPointer();
                logFile('[ATEL-DBG] ns=0x' + ns.id.toString(16) + ' rva=0x' + ns.rva.toString(16) +
                    ' entry0: 0x' + p0.toString(16) + ' 0x' + p1.toString(16) +
                    ' 0x' + p2.toString(16) + ' 0x' + p3.toString(16));
            } catch (e) { }

            for (var i = 0; i < ns.count; i++) {
                var entry = tableBase.add(i * 0x10);
                // Check all 4 function pointers in the AtelCallTarget struct
                for (var fo = 0; fo < 4; fo++) {
                    try {
                        var fnAddr = entry.add(fo * 4).readPointer();
                        if (fnAddr.isNull()) continue;
                        var relRva = fnAddr.sub(base).toInt32();
                        if (relRva < 0 || relRva > 0x3000000) continue;
                        if (EXIT_FN_RVAS.indexOf(relRva) >= 0) {
                            var atelId = (ns.id << 12) | i;
                            ids[atelId] = true;
                            logFile('[ATEL] Exit fn: id=0x' + atelId.toString(16) +
                                ' (ns=' + ns.id.toString(16) + ' idx=' + i +
                                ' field=' + fo + ') -> RVA 0x' + relRva.toString(16));
                            break;
                        }
                    } catch (e) { }
                }
            }
        }
    } catch (e) { logFile('[ATEL] ID discovery error: ' + e.message); }
    return ids;
}

// REQ family opcodes — 1-byte. Workers that use these coordinate with other
// workers (player, dialog, camera). Ambient effects don't; they just CALL and RET.
var REQ_OPCODES = {
    0x36: true, 0x37: true, 0x38: true,            // REQ, REQSW, REQEW
    0x39: true, 0x3A: true, 0x3B: true,            // PREQ, PREQSW, PREQEW
    0x45: true, 0x46: true, 0x47: true, 0x48: true, 0x49: true, // FREQ, TREQ, BREQ, BFREQ, BTREQ
    0x4A: true, 0x4B: true, 0x4C: true, 0x4D: true, 0x4E: true, // *SW variants
    0x4F: true, 0x50: true, 0x51: true, 0x52: true, 0x53: true, // *EW variants
    0x77: true, 0x78: true, 0x79: true             // REQWAIT, PREQWAIT, REQCHG
};

// Scan a worker's bytecode following branch flow (JMP/CJMP/NCJMP/JSR targets).
// Returns { isExit, hasReq, calls, reqs, bytes } — isExit if any CALL targets
// an exit function, hasReq if any REQ-family opcode is used, plus full list of
// CALL operands and REQ opcodes for diagnostic logging.
function workerScanBytecode(worker, exitIds) {
    var result = { isExit: false, hasReq: false, calls: [], reqs: [], reqTargets: [], bytes: 0, entries: 0, steps: 0, truncated: false };
    try {
        var headerPtr = worker.readPointer();
        var chunkPtr  = worker.add(0x04).readPointer();
        if (headerPtr.isNull() || chunkPtr.isNull()) return result;

        var codeOffset = chunkPtr.add(0x30).readU32();
        var codeLength = chunkPtr.readU32();
        if (codeLength === 0 || codeLength > 0x100000) return result;
        var codeBase = chunkPtr.add(codeOffset);

        var entryCount = headerPtr.add(0x08).readU16();
        if (entryCount === 0 || entryCount > 256) return result;
        var entryTableOffset = headerPtr.add(0x20).readU32();
        var entryTable = chunkPtr.add(entryTableOffset);
        result.entries = entryCount;
        result.bytes = codeLength;

        // Flow-following scan: depth-first walk from each entry point,
        // following branch targets. Visited set prevents loops.
        var visited = {};
        var stack = [];
        for (var e = 0; e < entryCount; e++) {
            var ep = entryTable.add(e * 4).readU32();
            if (ep < codeLength) stack.push(ep);
        }

        // Track the last two PUSHI immediates — REQ pops them as (worker_idx, function_id).
        var lastPushi1 = null; // most recent PUSHI operand
        var lastPushi2 = null; // the one before that

        var totalSteps = 0;
        var STEP_CAP = 200000;
        while (stack.length > 0 && totalSteps < STEP_CAP) {
            var pc = stack.pop();
            while (pc < codeLength && totalSteps < STEP_CAP) {
                totalSteps++;
                if (visited[pc]) break; // already explored
                visited[pc] = true;

                var opcode = codeBase.add(pc).readU8();

                // Detect REQ-family (1-byte opcodes)
                if (REQ_OPCODES[opcode]) {
                    result.hasReq = true;
                    if (result.reqs.indexOf(opcode) < 0) result.reqs.push(opcode);
                    // Capture the (worker_idx, function_id) pair from last two PUSHIs.
                    // Per REQ stack order: function_id was on top (pushed last = lastPushi1),
                    // worker_idx was below (pushed first = lastPushi2).
                    if (lastPushi1 !== null && lastPushi2 !== null) {
                        var target = { worker: lastPushi2, fn: lastPushi1 };
                        result.reqTargets.push(target);
                        // If the REQ target function is a known exit function, mark as Exit
                        if (exitIds[lastPushi1]) result.isExit = true;
                    }
                }

                if ((opcode & 0x80) !== 0) {
                    // 3-byte instruction
                    if (pc + 2 >= codeLength) break;
                    var operand = codeBase.add(pc + 1).readU16();
                    // CALL (0xB5) and CALLPOPA (0xD8) both invoke Atel functions
                    if (opcode === 0xB5 || opcode === 0xD8) {
                        if (exitIds[operand]) result.isExit = true;
                        if (result.calls.indexOf(operand) < 0) result.calls.push(operand);
                    }
                    // Track PUSHI immediates to reconstruct REQ targets
                    if (opcode === 0xAD) {
                        lastPushi2 = lastPushi1;
                        lastPushi1 = operand;
                    }
                    // Follow branches — normal + POPX variants
                    if (opcode === 0xB0 || opcode === 0xD5) { pc = operand; continue; } // JMP / POPXJMP
                    if (opcode === 0xB1 || opcode === 0xB2 ||
                        opcode === 0xD6 || opcode === 0xD7) { stack.push(operand); }    // CJMP/NCJMP/POPXCJMP/POPXNCJMP
                    if (opcode === 0xB3) { stack.push(operand); }                        // JSR
                    pc += 3;
                } else {
                    // Return/halt opcodes — break the flow
                    if (opcode === 0x34 || opcode === 0x3C || opcode === 0x3D ||
                        opcode === 0x3E || opcode === 0x3F || opcode === 0x40 ||
                        opcode === 0x54) break; // RTS/RET/RETN/RETT/RETTN/HALT/DRET
                    pc += 1;
                }
            }
            // Reset PUSHI tracking between basic blocks — stack state doesn't survive branches
            lastPushi1 = null;
            lastPushi2 = null;
        }
        result.steps = totalSteps;
        result.truncated = totalSteps >= STEP_CAP;
    } catch (e) {
        result.error = e.message;
    }
    return result;
}

function navScanExits(playerPos, nav) {
    try {
        var fnGetWorker = new NativeFunction(rva(0x46A7C0), 'pointer', ['int']);
        var controllerPtr = rva(RVA_ATEL_CONTROLLER).readPointer();
        if (controllerPtr.isNull()) return;
        var actorCount = controllerPtr.add(0x0C).readU16();
        if (actorCount === 0 || actorCount > 500) return;
        // Lazily discover exit function IDs on first scan
        if (exitAtelIds === null) {
            exitAtelIds = findExitAtelIds();
            var idList = Object.keys(exitAtelIds);
            logFile('[ATEL] Found ' + idList.length + ' exit function IDs: ' + idList.join(','));
        }

        var scanned = 0, exitCount = 0, eventCount = 0;
        // Log type distribution across ALL workers — diagnostic for finding
        // the real transition trigger (which might not be type 2/3/5/6).
        var typeCount = {};
        for (var i = 0; i < actorCount; i++) {
            try {
                var worker = fnGetWorker(i);
                if (worker.isNull()) continue;
                var firstDword = worker.readPointer();
                if (firstDword.isNull()) continue;
                var actorType = firstDword.readU8();
                typeCount[actorType] = (typeCount[actorType] || 0) + 1;

                // Try both geometry offsets since we don't know what types !=2/3/5/6 use
                var geomOffset = (actorType === 5 || actorType === 6) ? 0x284 : 0x558;
                var x = worker.add(geomOffset).readFloat();
                var y = worker.add(geomOffset + 4).readFloat();
                var z = worker.add(geomOffset + 8).readFloat();
                var hasValidPos = !isNaN(x) && !isNaN(z) &&
                    Math.abs(x) <= 50000 && Math.abs(z) <= 50000 &&
                    !(x === 0 && y === 0 && z === 0);

                // For types outside 2/3/5/6, just log type + quick bytecode scan,
                // skip the position/classify/navmesh logic
                if (actorType !== 2 && actorType !== 3 && actorType !== 5 && actorType !== 6) {
                    var altScan = workerScanBytecode(worker, exitAtelIds);
                    var dxFromPlayer = hasValidPos ? (x - playerPos.x) : 0;
                    var dzFromPlayer = hasValidPos ? (z - playerPos.z) : 0;
                    var distFromPlayer = hasValidPos ? Math.sqrt(dxFromPlayer*dxFromPlayer + dzFromPlayer*dzFromPlayer) : -1;
                    logFile('[WORKER-ALT ' + i + ' type=' + actorType +
                        ' pos=' + (hasValidPos ? '(' + x.toFixed(1) + ',' + z.toFixed(1) + ')' : 'invalid') +
                        ' distFromPlayer=' + (distFromPlayer >= 0 ? distFromPlayer.toFixed(1) : '?') +
                        ' isExit=' + altScan.isExit + ' hasReq=' + altScan.hasReq +
                        ' steps=' + altScan.steps + (altScan.truncated ? '(TRUNC)' : '') +
                        ' reqs=[' + altScan.reqs.map(function(o){return '0x'+o.toString(16);}).join(',') + ']' +
                        ' reqTargets=[' + altScan.reqTargets.slice(0, 20).map(function(t){return 'w'+t.worker+':fn0x'+t.fn.toString(16);}).join(',') +
                        (altScan.reqTargets.length > 20 ? ',...+' + (altScan.reqTargets.length-20) : '') + ']' +
                        ' calls=[' + altScan.calls.slice(0, 20).map(function(o){return '0x'+o.toString(16);}).join(',') + ']');
                    continue;
                }

                scanned++;

                if (!hasValidPos) continue;

                // Classify via bytecode scan:
                //   Exit   — script CALLs a map-transition function
                //   Event  — script has REQ-family opcodes (cross-worker coordination)
                //   Ambient — neither, filtered out (water loops, background FX)
                var scan = workerScanBytecode(worker, exitAtelIds);
                var cls = scan.isExit ? 'EXIT' : (scan.hasReq ? 'EVENT' : 'AMBIENT');
                logFile('[WORKER ' + i + ' type=' + actorType + ' class=' + cls +
                    ' bytes=' + scan.bytes + ' entries=' + scan.entries +
                    ' steps=' + scan.steps + (scan.truncated ? '(TRUNC)' : '') +
                    ' pos=(' + x.toFixed(1) + ',' + z.toFixed(1) + ')' +
                    (scan.error ? ' err=' + scan.error : '') +
                    ' reqs=[' + scan.reqs.map(function(o){return '0x'+o.toString(16);}).join(',') + ']' +
                    ' reqTargets=[' + scan.reqTargets.slice(0, 20).map(function(t){return 'w'+t.worker+':fn0x'+t.fn.toString(16);}).join(',') +
                    (scan.reqTargets.length > 20 ? ',...+' + (scan.reqTargets.length-20) : '') + ']' +
                    ' calls=[' + scan.calls.slice(0, 40).map(function(o){return '0x'+o.toString(16);}).join(',') +
                    (scan.calls.length > 40 ? ',...+' + (scan.calls.length-40) : '') + ']');
                var pos = { x: x, y: y, z: z };
                if (scan.isExit) {
                    exitCount++;
                    navEntities.push({ name: 'Exit ' + exitCount, category: 'Exit',
                        chrId: 0xFFFF, pos: pos, dist: navDistXZ(playerPos, pos) });
                } else if (scan.hasReq) {
                    eventCount++;
                    navEntities.push({ name: 'Event ' + eventCount, category: 'Event',
                        chrId: 0xFFFE, pos: pos, dist: navDistXZ(playerPos, pos) });
                }
            } catch(e) {}
        }
        var typeSummary = Object.keys(typeCount).map(function(k){return 'type'+k+'='+typeCount[k];}).join(',');
        logFile('[NAV] Worker types in controller: ' + typeSummary);
        logFile('[NAV] Player pos at scan: (' + playerPos.x.toFixed(1) + ',' + playerPos.z.toFixed(1) + ')');
        logFile('[NAV] Triggers: scanned ' + scanned + ', classified ' + exitCount +
            ' exits, ' + eventCount + ' events');
    } catch(e) { logFile('[NAV] Exit scan error: ' + e.message); }
}

function navGetFiltered() {
    if (navCategoryIdx === 0) return navEntities; // All
    var cat = NAV_CATEGORIES[navCategoryIdx];
    return navEntities.filter(function(e) { return e.category === cat; });
}

function navAnnounceSelected(interrupt) {
    if (interrupt === undefined) interrupt = true;
    var list = navGetFiltered();
    if (list.length === 0) return;
    if (navSelectedIdx >= list.length) navSelectedIdx = 0;
    var e = list[navSelectedIdx];
    var pp = navGetPlayerPos();
    if (!pp) return;
    var dist = navDistXZ(pp, e.pos);
    var steps = (dist / NAV_UNITS_PER_STEP).toFixed(1);
    var dir = navRelativeDir(e.pos.x - pp.x, e.pos.z - pp.z);
    speak((navSelectedIdx + 1) + '/' + list.length + ': ' + navLabel(e) + ', ' + steps + ' steps ' + dir, interrupt);
}

// Navmesh + A* (from nav_integrated.js)
function navReadNavmesh() {
    var triCount = rva(RVA_TRICOUNT).readS32();
    var trisRaw = rva(RVA_TRIS).readU32();
    var vertsRaw = rva(RVA_VERTICES).readU32();
    var scale = rva(RVA_SCALE).readFloat();
    if (triCount <= 0 || trisRaw < 0x10000 || vertsRaw < 0x10000 || scale <= 0) return null;
    var tB = ptr(trisRaw), vB = ptr(vertsRaw);
    var tris = [];
    for (var i = 0; i < triCount; i++) {
        var t = tB.add(i * 0x10);
        var vis = [t.readS16(), t.add(2).readS16(), t.add(4).readS16()];
        var nbs = [t.add(6).readS16(), t.add(8).readS16(), t.add(10).readS16()];
        var coll = (t.add(12).readS32() >>> 0) & 0x7F;
        var vxs = [], vzs = [], cx = 0, cz = 0;
        for (var j = 0; j < 3; j++) {
            var va = vB.add(vis[j] * 8);
            var wx = va.readS16() / scale, wz = va.add(4).readS16() / scale;
            vxs.push(wx); vzs.push(wz); cx += wx; cz += wz;
        }
        tris.push({ verts: vis, neighbours: nbs, coll: coll, cx: cx/3, cz: cz/3, vxs: vxs, vzs: vzs });
    }
    return { tris: tris, triCount: triCount };
}

// Strict point-in-walkable-triangle using precomputed walkable list + bboxes.
function navFindTriStrictFast(walkable, px, pz) {
    for (var wi = 0; wi < walkable.length; wi++) {
        var w = walkable[wi];
        // Bounding box reject (O(1) per triangle)
        if (px < w.minX || px > w.maxX || pz < w.minZ || pz > w.maxZ) continue;
        var t = w.tri;
        var v0x=t.vxs[2]-t.vxs[0],v0z=t.vzs[2]-t.vzs[0],v1x=t.vxs[1]-t.vxs[0],v1z=t.vzs[1]-t.vzs[0];
        var v2x=px-t.vxs[0],v2z=pz-t.vzs[0];
        var d00=v0x*v0x+v0z*v0z,d01=v0x*v1x+v0z*v1z,d02=v0x*v2x+v0z*v2z;
        var d11=v1x*v1x+v1z*v1z,d12=v1x*v2x+v1z*v2z;
        var inv=d00*d11-d01*d01; if (Math.abs(inv)<0.0001) continue; inv=1/inv;
        var u=(d11*d02-d01*d12)*inv,v=(d00*d12-d01*d02)*inv;
        if (u>=-0.02&&v>=-0.02&&(u+v)<=1.02) return true;
    }
    return false;
}

function navBuildWalkable(nav) {
    var walkable = [];
    for (var i = 0; i < nav.triCount; i++) {
        var t = nav.tris[i];
        if (t.coll !== 0 && t.coll !== 2) continue;
        walkable.push({
            tri: t,
            minX: Math.min(t.vxs[0], t.vxs[1], t.vxs[2]),
            maxX: Math.max(t.vxs[0], t.vxs[1], t.vxs[2]),
            minZ: Math.min(t.vzs[0], t.vzs[1], t.vzs[2]),
            maxZ: Math.max(t.vzs[0], t.vzs[1], t.vzs[2])
        });
    }
    return walkable;
}

function navHasLineOfSight(walkable, x1, z1, x2, z2) {
    var dist = Math.sqrt((x2-x1)*(x2-x1) + (z2-z1)*(z2-z1));
    var steps = Math.max(3, Math.ceil(dist / 5));  // coarser sampling
    for (var i = 0; i <= steps; i++) {
        var t = i / steps;
        if (!navFindTriStrictFast(walkable, x1 + (x2-x1) * t, z1 + (z2-z1) * t)) return false;
    }
    return true;
}

// Greedy smoothing: jump to furthest waypoint reachable via straight line.
function navSmoothWaypoints(nav, wps) {
    if (wps.length <= 2) return wps;
    var walkable = navBuildWalkable(nav);
    var out = [wps[0]];
    var i = 0;
    while (i < wps.length - 1) {
        var next = i + 1;
        for (var j = wps.length - 1; j > i + 1; j--) {
            if (navHasLineOfSight(walkable, wps[i].x, wps[i].z, wps[j].x, wps[j].z)) {
                next = j;
                break;
            }
        }
        out.push(wps[next]);
        i = next;
    }
    return out;
}

function navFindTri(nav, px, pz) {
    for (var i = 0; i < nav.triCount; i++) {
        var t = nav.tris[i];
        var v0x=t.vxs[2]-t.vxs[0],v0z=t.vzs[2]-t.vzs[0],v1x=t.vxs[1]-t.vxs[0],v1z=t.vzs[1]-t.vzs[0];
        var v2x=px-t.vxs[0],v2z=pz-t.vzs[0];
        var d00=v0x*v0x+v0z*v0z,d01=v0x*v1x+v0z*v1z,d02=v0x*v2x+v0z*v2z;
        var d11=v1x*v1x+v1z*v1z,d12=v1x*v2x+v1z*v2z;
        var inv=d00*d11-d01*d01; if (Math.abs(inv)<0.0001) continue; inv=1/inv;
        var u=(d11*d02-d01*d12)*inv,v=(d00*d12-d01*d02)*inv;
        if (u>=-0.05&&v>=-0.05&&(u+v)<=1.05) return i;
    }
    var best=999999,bi=-1;
    for (var i=0;i<nav.triCount;i++) {
        var t=nav.tris[i]; var d=(px-t.cx)*(px-t.cx)+(pz-t.cz)*(pz-t.cz);
        if (d<best){best=d;bi=i;}
    }
    return bi;
}

function navAstar(nav, start, end) {
    if (start===end) return [start]; if (start<0||end<0) return null;
    var ex=nav.tris[end].cx,ez=nav.tris[end].cz;
    var open={},closed={},gS={},fS={},from={};
    gS[start]=0; fS[start]=Math.sqrt((nav.tris[start].cx-ex)*(nav.tris[start].cx-ex)+(nav.tris[start].cz-ez)*(nav.tris[start].cz-ez));
    open[start]=true;
    for (var iter=0;iter<10000;iter++) {
        var cur=-1,bestF=1e18;
        for (var k in open){var ki=parseInt(k);if(fS[ki]<bestF){bestF=fS[ki];cur=ki;}}
        if (cur===-1) return null;
        if (cur===end) { var path=[cur]; while(from[cur]!==undefined){cur=from[cur];path.unshift(cur);} return path; }
        delete open[cur]; closed[cur]=true;
        var tri=nav.tris[cur];
        for (var ni=0;ni<3;ni++) {
            var nb=tri.neighbours[ni];
            if (nb<0||nb>=nav.triCount||closed[nb]) continue;
            if (nav.tris[nb].coll!==0&&nav.tris[nb].coll!==2) continue;
            var tg=gS[cur]+Math.sqrt((tri.cx-nav.tris[nb].cx)*(tri.cx-nav.tris[nb].cx)+(tri.cz-nav.tris[nb].cz)*(tri.cz-nav.tris[nb].cz));
            if (gS[nb]===undefined||tg<gS[nb]) { from[nb]=cur;gS[nb]=tg;
                fS[nb]=tg+Math.sqrt((nav.tris[nb].cx-ex)*(nav.tris[nb].cx-ex)+(nav.tris[nb].cz-ez)*(nav.tris[nb].cz-ez));
                open[nb]=true; }
        }
    }
    return null;
}

function navPathfind() {
    var list = navGetFiltered();
    if (list.length === 0 || navSelectedIdx >= list.length) { speak('No entity selected', true); return; }
    var target = list[navSelectedIdx];
    var pp = navGetPlayerPos(); if (!pp) return;
    var nav = navReadNavmesh();
    if (!nav) { speak(navLabel(target) + ': ' + (navDistXZ(pp,target.pos)/NAV_UNITS_PER_STEP).toFixed(1) + ' steps ' + navRelativeDir(target.pos.x-pp.x,target.pos.z-pp.z), true); return; }
    var st = navFindTri(nav,pp.x,pp.z), en = navFindTri(nav,target.pos.x,target.pos.z);
    if (st<0||en<0) { speak(navLabel(target)+': crow-flies '+(navDistXZ(pp,target.pos)/NAV_UNITS_PER_STEP).toFixed(1)+' steps',true); return; }
    var path = navAstar(nav,st,en);
    if (!path) { speak(navLabel(target)+': no path',true); return; }
    // Build raw waypoints from portal midpoints
    var wps = [{x:pp.x,z:pp.z}];
    for (var i=0;i<path.length-1;i++) {
        var a=nav.tris[path[i]],b=nav.tris[path[i+1]]; var sh=[];
        for(var vi=0;vi<3;vi++)for(var vj=0;vj<3;vj++)if(a.verts[vi]===b.verts[vj])sh.push(vi);
        if(sh.length===2)wps.push({x:(a.vxs[sh[0]]+a.vxs[sh[1]])/2,z:(a.vzs[sh[0]]+a.vzs[sh[1]])/2});
    }
    wps.push({x:target.pos.x,z:target.pos.z});

    // Smooth: greedy line-of-sight skip to eliminate zigzag. This keeps only
    // waypoints where actual direction change is forced by geometry.
    var wpsBefore = wps.length;
    wps = navSmoothWaypoints(nav, wps);
    logFile('[NAV] Path: tris=' + path.length + ' wps ' + wpsBefore + '->' + wps.length +
        ' crow=' + navRelativeDir(target.pos.x - pp.x, target.pos.z - pp.z));
    var dirs=[];
    for(var i=0;i<wps.length-1;i++){
        var dx=wps[i+1].x-wps[i].x,dz=wps[i+1].z-wps[i].z;
        var steps=Math.sqrt(dx*dx+dz*dz)/NAV_UNITS_PER_STEP;
        var dir = navRelativeDir(dx,dz);
        logFile('[NAV]   seg ' + i + ': dx=' + dx.toFixed(1) + ' dz=' + dz.toFixed(1) +
            ' steps=' + steps.toFixed(1) + ' dir=' + dir);
        if(steps>0.5) dirs.push(Math.round(steps)+' '+dir);
    }
    var merged=[];
    for(var i=0;i<dirs.length;i++){
        var p=dirs[i].split(' ');  // "3 south" -> ["3", "south"]
        if(merged.length>0){
            var prev=merged[merged.length-1].split(' ');
            if(prev.length>=2&&p.length>=2&&prev[prev.length-1]===p[p.length-1]){
                merged[merged.length-1]=(parseInt(prev[0])+parseInt(p[0]))+' '+p[p.length-1];continue;}}
        merged.push(dirs[i]);
    }
    speak(merged.length > 0 ? merged.join(', ') : 'you are here', true);
}

logFile('Navigation loaded');

// ══════════════════════════════════════════
// SECTION 8: Save/Load Screen
// ══════════════════════════════════════════
// From decompile (layer2_game_logic.txt, lines 183700-183826, FUN_006f0da0):
// FUN_00702e70 is called per slot during population with all formatted strings:
//   param_1 = slot index
//   param_3 = area name ptr
//   param_4 = date string ptr
//   param_5 = character name ptr
//   param_6 = playtime with area ptr
// Cursor globals: DAT_00ce72dc (col) + DAT_00ce72e0 (row) = current slot.
// FUN_00702db0 is called on cursor move with (col + row) = new slot index.

var saveSlots = {};  // slotIdx -> {area, playtime}
var prevSaveSlot = -1;

function readCString(addr) {
    try {
        if (addr.isNull() || addr.compare(ptr(0x10000)) <= 0) return null;
        var s = addr.readCString(64);
        return (s && s.length > 0) ? s : null;
    } catch (e) { return null; }
}

try {
    // Hook slot population: capture area name and playtime per slot
    Interceptor.attach(rva(0x302E70), {
        onEnter: function (args) {
            try {
                var slotIdx = args[0].toInt32();
                var area = readCString(args[2]);
                var playtime = readCString(args[5]);
                if (area || playtime) {
                    saveSlots[slotIdx] = { area: area || '', playtime: playtime || '' };
                    logFile('[SAVE] slot ' + slotIdx + ' populated: area=' + area +
                        ' playtime=' + playtime);
                }
            } catch (e) { }
        }
    });
    logFile('FUN_00702e70 (slot populate) hooked');
} catch (e) { logFile('slot populate hook failed: ' + e.message); }

try {
    // Hook cursor focus change: speak selected slot
    Interceptor.attach(rva(0x302DB0), {
        onEnter: function (args) {
            try {
                var slotIdx = args[0].toInt32();
                if (slotIdx === prevSaveSlot) return;
                prevSaveSlot = slotIdx;
                var info = saveSlots[slotIdx];
                if (info) {
                    var label = 'Slot ' + slotIdx;
                    if (info.area) label += ': ' + info.area;
                    if (info.playtime) label += ', ' + info.playtime;
                    speak(label, true);
                } else {
                    speak('Slot ' + slotIdx + ' empty', true);
                }
            } catch (e) { }
        }
    });
    logFile('FUN_00702db0 (save focus) hooked');
} catch (e) { logFile('save focus hook failed: ' + e.message); }

// AtelSetEventJump2(int room, int entrance, int do_fade) — the actual map-transition
// function. Hooking it tells us the caller (retAddr), destination room/entrance, and
// the current player position + active worker when a transition fires. From the caller
// RVA we can identify the engine-side zone-check code.
try {
    Interceptor.attach(rva(0x46FED0), {
        onEnter: function (args) {
            try {
                var room = args[0].toInt32();
                var entrance = args[1].toInt32();
                var fade = args[2].toInt32();
                var retAddr = this.returnAddress;
                var retRva = retAddr.sub(base).toInt32();
                var pp = navGetPlayerPos();
                var posStr = pp ? ('(' + pp.x.toFixed(1) + ',' + pp.y.toFixed(1) + ',' + pp.z.toFixed(1) + ')') : 'none';
                // Grab current active worker idx if available
                var workerInfo = '';
                try {
                    var worker = rva(RVA_CURRENT_WORKER).readPointer();
                    if (!worker.isNull() && worker.compare(ptr(0x10000)) > 0) {
                        workerInfo = ' w_idx=' + worker.add(0x2E).readU16() + ' w_entry=' + worker.add(0x2C).readU16();
                    }
                } catch (e) { }
                logFile('[JUMP2] room=0x' + room.toString(16) + ' entrance=' + entrance +
                    ' fade=' + fade + ' playerPos=' + posStr +
                    ' retRVA=0x' + retRva.toString(16) + workerInfo);
                // Backtracer — get a few call frames
                try {
                    var bt = Thread.backtrace(this.context, Backtracer.ACCURATE).slice(0, 6);
                    var btStr = bt.map(function (p) {
                        var r = p.sub(base).toInt32();
                        return '0x' + r.toString(16);
                    }).join(' <- ');
                    logFile('[JUMP2-BT] ' + btStr);
                } catch (e) { }
            } catch (e) { }
        }
    });
    logFile('AtelSetEventJump2 (0x46FED0) hooked');
} catch (e) { logFile('AtelSetEventJump2 hook failed: ' + e.message); }

// ══════════════════════════════════════════
// SECTION 8: Per-Frame Hook + Hotkey Handling
// ══════════════════════════════════════════

// Hotkey handler (replaces Python host)
function handleHotkeys() {
    // Navigation
    var shiftDown = fnGetAsyncKeyState ? (fnGetAsyncKeyState(VK_SHIFT) & 0x8000) !== 0 : false;

    if (keyPressed(VK_OEM_3)) { navScan(); var list = navGetFiltered(); speak(list.length + ' ' + NAV_CATEGORIES[navCategoryIdx] + (navCategoryIdx === 0 ? '' : 's'), true); if (list.length > 0) navAnnounceSelected(); }

    if (keyPressed(VK_OEM_6)) {
        if (shiftDown) {
            // Shift+] = next category — speak category first (interrupt), then
            // queue selected entity behind it (non-interrupt).
            navCategoryIdx = (navCategoryIdx + 1) % NAV_CATEGORIES.length;
            navSelectedIdx = 0;
            var list = navGetFiltered();
            speak('category: ' + NAV_CATEGORIES[navCategoryIdx] + ', ' + list.length, true);
            if (list.length > 0) navAnnounceSelected(false);
        } else {
            var list = navGetFiltered();
            if (list.length > 0) { navSelectedIdx = (navSelectedIdx + 1) % list.length; navAnnounceSelected(); }
        }
    }
    if (keyPressed(VK_OEM_4)) {
        if (shiftDown) {
            // Shift+[ = previous category — same queue-behind pattern
            navCategoryIdx = (navCategoryIdx - 1 + NAV_CATEGORIES.length) % NAV_CATEGORIES.length;
            navSelectedIdx = 0;
            var list = navGetFiltered();
            speak('category: ' + NAV_CATEGORIES[navCategoryIdx] + ', ' + list.length, true);
            if (list.length > 0) navAnnounceSelected(false);
        } else {
            var list = navGetFiltered();
            if (list.length > 0) { navSelectedIdx = (navSelectedIdx - 1 + list.length) % list.length; navAnnounceSelected(); }
        }
    }
    if (keyPressed(VK_P)) { var list = navGetFiltered(); if (list.length > 0 && navSelectedIdx < list.length) { var e=list[navSelectedIdx]; var pp=navGetPlayerPos(); if(pp){ var s=(navDistXZ(pp,e.pos)/NAV_UNITS_PER_STEP).toFixed(1); speak(navLabel(e)+': '+s+' steps '+navRelativeDir(e.pos.x-pp.x,e.pos.z-pp.z),true); } } }
    if (keyPressed(VK_OEM_5)) { navPathfind(); }

    // Battle info
    if (keyPressed(VK_T)) {
        try {
            if (btlStateAddr.readU8() !== 0) {
                var curWinIdx = btlCurWinIdxAddr.readU8();
                var win = readBtlWindow(curWinIdx);
                if (win && win.state !== 0) {
                    var chrName = (win.chrId < CHARACTER_NAMES.length) ? CHARACTER_NAMES[win.chrId] : ('chr' + win.chrId);
                    speak(chrName, true);
                }
            }
        } catch(e) {}
    }
    if (keyPressed(VK_H)) {
        try {
            var pArr = playerChrsAddr.readPointer();
            if (!pArr.isNull()) {
                var parts = [];
                for (var i = 0; i < 8; i++) {
                    var chrBase = pArr.add(i * CHR_SIZE);
                    if (chrBase.add(CHR_IN_BATTLE).readU8() === 0) continue;
                    var ram = chrBase.add(CHR_RAM_OFFSET);
                    var name = (i < CHARACTER_NAMES.length) ? CHARACTER_NAMES[i] : 'chr' + i;
                    parts.push(name + ' HP ' + ram.add(0x1A4).readS32() + '/' + ram.add(0x54).readS32() +
                        ' MP ' + ram.add(0x1A8).readS32() + '/' + ram.add(0x58).readS32());
                }
                if (parts.length > 0) speak(parts.join(', '), true);
            }
        } catch(e) {}
    }
    if (keyPressed(VK_N)) {
        try {
            // Try HELP buffer first (battle), then Sphere Grid, then MsMenuGetText desc
            var helpSegs = decodeFFXSegments(helpTextAddr, 128, 0);
            if (helpSegs.length > 0 && helpSegs[0].length > 0) { speak(helpSegs.join(' '), true); return; }
            // Field menu description
            var cursor = fieldMenuCursorAddr.readS32();
            var logical = cursor;
            try { logical = fieldMenuItemTable.add(cursor * 8).readS32(); } catch(e) {}
            if (msMenuDescCache[logical]) { speak(msMenuDescCache[logical], true); }
        } catch(e) {}
    }
}

try {
    Interceptor.attach(rva(0x471D10), {
        onLeave: function(retval) {
            frameCount++;

            // ── Navigation: detect map load/unload via navmesh tri_count ──
            try {
                var triCount = rva(RVA_TRICOUNT).readS32();
                var triPtr = rva(RVA_TRIS).readU32();

                // Helper: log game state at a map transition
                function logTransitionState(tag) {
                    try {
                        var room = saveDataAddr.readU16();
                        var lastRoom = saveDataAddr.add(2).readU16();
                        var worker = rva(RVA_CURRENT_WORKER).readPointer();
                        var workerInfo = 'null';
                        if (!worker.isNull() && worker.compare(ptr(0x10000)) > 0) {
                            var chunk = worker.add(0x04).readPointer();
                            var workerIdx = worker.add(0x2E).readU16();
                            var entryPt = worker.add(0x2C).readU16();
                            workerInfo = 'worker=0x' + worker.toString(16) +
                                ' idx=' + workerIdx + ' entry=' + entryPt +
                                ' chunk=0x' + chunk.toString(16);
                        }
                        logFile('[TRANSITION ' + tag + '] room=0x' + room.toString(16) +
                            ' lastRoom=0x' + lastRoom.toString(16) + ' ' + workerInfo);
                    } catch (e) { }
                }

                // Detect field map active/dormant via tri_count
                if (triCount > 0 && navPrevTriCount <= 0) {
                    navFieldActive = true;
                    logFile('[NAV] Field map active (tris=' + triCount + ')');
                    logTransitionState('FIELD_LOAD');
                    navScanPending = true; navScanRetries = 0;
                }
                if (triCount <= 0 && navPrevTriCount > 0) {
                    navFieldActive = false;
                    navEntities = []; navSelectedIdx = 0;
                    navScanPending = false;
                    logFile('[NAV] Field map dormant');
                    logTransitionState('FIELD_UNLOAD');
                }
                navPrevTriCount = triCount;

                // Detect map-to-map transitions: navmesh pointer changes
                // (tri_count stays positive but the actual mesh data changes)
                if (navFieldActive && triPtr !== navPrevTriPtr && navPrevTriPtr !== 0) {
                    logFile('[NAV] Navmesh changed: 0x' + navPrevTriPtr.toString(16) + ' -> 0x' + triPtr.toString(16));
                    logTransitionState('NAVMESH_CHANGE');
                    // Battle entry/exit swaps the arena navmesh while actors are mid-
                    // teardown; scanning then reads uninitialized slots. Nav block
                    // runs before the battle block this frame, so prevBtlState still
                    // holds last frame's value — a reliable mid-transition signal.
                    var inBtlTransition = btlStateAddr.readU8() !== 0 || prevBtlState !== 0;
                    if (!inBtlTransition) {
                        navScanPending = true; navScanRetries = 0;
                    } else {
                        logFile('[NAV] Skipping autoscan — battle transition in progress');
                    }
                }
                navPrevTriPtr = triPtr;

                // Retry scan until actors are available
                if (navScanPending && navFieldActive) {
                    navScanRetries++;
                    // Wait a few frames for actors to load, then try
                    if (navScanRetries > 30) {
                        var ab = navGetActorsBase();
                        if (ab) {
                            // Check slot 0 (player) has a valid position
                            var px = ab.add(0x0C).readFloat();
                            if (!isNaN(px) && px !== 0) {
                                logFile('[NAV] Actors ready after ' + navScanRetries + ' frames');
                                navScan();
                                navScanPending = false;
                            }
                        }
                        // Give up after ~5 seconds (150 frames at 30fps)
                        if (navScanRetries > 150) {
                            logFile('[NAV] Scan timed out — no actors after 150 frames');
                            navScanPending = false;
                        }
                    }
                }
            } catch(e) {}

            // ── Hotkeys ──
            try { handleHotkeys(); } catch(e) {}

            // ── PART 1: Battle menu monitoring ──
            try {
                var btlState = btlStateAddr.readU8();
                if (btlState !== 0) {
                    if (prevBtlState === 0) logFile('[BTL] Battle started');
                    var curWinIdx = btlCurWinIdxAddr.readU8();
                    var win = readBtlWindow(curWinIdx);
                    if (win) {
                        var typeName = BTL_WIN_TYPES[win.type] || ('0x' + win.type.toString(16));
                        var chrName = (win.chrId < CHARACTER_NAMES.length) ? CHARACTER_NAMES[win.chrId] : ('chr' + win.chrId);
                        var winChanged = (win.type !== prevBtlWinType || curWinIdx !== prevBtlWinIdx);
                        if (winChanged) { btlWinAnnounced = false; prevBtlSelIdx = -1; prevBtlSelCmd = -1; }
                        if (win.state === 4 && !btlWinAnnounced && win.validSelAmt > 0) {
                            btlWinAnnounced = true;
                            var cmdName = readCommandName(win);
                            // Non-interrupt: queues behind damage/status announcements
                            speak(typeName + ', ' + chrName + (cmdName ? ', ' + cmdName : ''), false);
                            prevBtlSelIdx = win.selIdx; prevBtlSelCmd = win.selCmdId;
                            if (win.selCmdId !== 0) lastNonZeroCmdId = win.selCmdId;
                        }
                        // Targeting
                        var winBaseT = btlWinArrayAddr.add(curWinIdx * BTL_WIN_SIZE);
                        var curChrs = winBaseT.add(0xD8).readU16();
                        var curMons = winBaseT.add(0xDA).readU16();
                        var nowTargeting = (win.validSelAmt === 0 && win.state !== 0 && (curChrs !== 0 || curMons !== 0));
                        if (nowTargeting && !inTargeting) {
                            inTargeting = true;
                            targetShowHp = true; targetShowMp = false;
                            if (lastNonZeroCmdId > 0) {
                                var preview = getCommandPartyPreview(lastNonZeroCmdId);
                                if (preview) { targetShowHp = preview.showHp; targetShowMp = preview.showMp; if (!targetShowHp && !targetShowMp) targetShowHp = true; }
                            }
                        }
                        if (inTargeting && nowTargeting && (curChrs !== prevTargetChrs || curMons !== prevTargetMons)) {
                            var names = [];
                            // Monsters: bits 4-15 (bit 4 = monster index 0), 16 bits total
                            for (var bit = 4; bit < 16; bit++) {
                                if (curMons & (1 << bit)) {
                                    var monIdx = bit - 4;
                                    var info = readMonsterInfo(monIdx);
                                    if (info) names.push(info.name + ': ' + info.hpPct + '%');
                                }
                            }
                            // Allies: bits 0-17 (includes all party + Aeons)
                            for (var bit2 = 0; bit2 < 18; bit2++) {
                                if (curChrs & (1 << bit2)) {
                                    var pInfo = readPlayerInfo(bit2);
                                    if (pInfo) {
                                        names.push(pInfo.name + ':' + (targetShowMp ? ' MP ' + pInfo.currentMp + '/' + pInfo.maxMp : ' HP ' + pInfo.currentHp + '/' + pInfo.maxHp));
                                    }
                                }
                            }
                            if (names.length > 0) speak(names.join(', '), false);
                            prevTargetChrs = curChrs; prevTargetMons = curMons;
                        }
                        if (!nowTargeting && inTargeting) { inTargeting = false; prevTargetChrs = 0; prevTargetMons = 0; }
                        // Cursor change
                        if (win.state === 4 && win.validSelAmt > 0 && (win.selIdx !== prevBtlSelIdx || win.selCmdId !== prevBtlSelCmd)) {
                            var cmdName2 = readCommandName(win);
                            if (cmdName2) speak(cmdName2, true);
                            prevBtlSelIdx = win.selIdx; prevBtlSelCmd = win.selCmdId;
                            if (win.selCmdId !== 0) lastNonZeroCmdId = win.selCmdId;
                        }
                        prevBtlWinType = win.type; prevBtlWinIdx = curWinIdx;
                    }
                    // Status effects
                    try {
                        var pArr2 = playerChrsAddr.readPointer();
                        if (!pArr2.isNull()) {
                            for (var si = 0; si < 0x1f; si++) {
                                try {
                                    var sChr = pArr2.add(si * CHR_SIZE);
                                    var sk = '' + si;
                                    var deathNow = sChr.add(0xDCC).readU8();
                                    var deathOld = (prevStatDeath[sk] !== undefined) ? prevStatDeath[sk] : deathNow;
                                    if (deathNow !== 0 && deathOld === 0) {
                                        var dSegs = decodeFFXSegments(sChr.add(CHR_RAM_OFFSET), 40, 0);
                                        var dName = (dSegs.length > 0) ? dSegs[0] : ((si < CHARACTER_NAMES.length) ? CHARACTER_NAMES[si] : 'chr' + si);
                                        speak(dName + ': KO', false);
                                    }
                                    prevStatDeath[sk] = deathNow;
                                    if (sChr.add(CHR_IN_BATTLE).readU8() === 0) continue;
                                    var permNow = sChr.add(0x606).readU16();
                                    var permOld = (prevStatusPerm[sk] !== undefined) ? prevStatusPerm[sk] : permNow;
                                    var permDiff = permNow ^ permOld;
                                    for (var bit3 = 1; bit3 < PERM_STATUS_NAMES.length; bit3++) {
                                        if (permDiff & (1 << bit3)) {
                                            var sName = null; try { var ns5 = decodeFFXSegments(sChr.add(CHR_RAM_OFFSET), 40, 0); sName = ns5.length > 0 ? ns5[0] : null; } catch(e) {}
                                            if (!sName) sName = (si < CHARACTER_NAMES.length) ? CHARACTER_NAMES[si] : 'chr' + si;
                                            speak(sName + ': ' + PERM_STATUS_NAMES[bit3] + ((permNow & (1 << bit3)) ? '' : ' cured'), false);
                                        }
                                    }
                                    prevStatusPerm[sk] = permNow;
                                    var extraNow = sChr.add(0x616).readU16();
                                    var extraOld = (prevStatusExtra[sk] !== undefined) ? prevStatusExtra[sk] : extraNow;
                                    var extraDiff = extraNow ^ extraOld;
                                    for (var bit4 = 0; bit4 < EXTRA_STATUS_NAMES.length; bit4++) {
                                        if ((extraDiff & (1 << bit4)) && EXTRA_STATUS_NAMES[bit4] !== '') {
                                            var sName2 = null; try { var ns6 = decodeFFXSegments(sChr.add(CHR_RAM_OFFSET), 40, 0); sName2 = ns6.length > 0 ? ns6[0] : null; } catch(e) {}
                                            if (!sName2) sName2 = (si < CHARACTER_NAMES.length) ? CHARACTER_NAMES[si] : 'chr' + si;
                                            speak(sName2 + ': ' + EXTRA_STATUS_NAMES[bit4] + ((extraNow & (1 << bit4)) ? '' : ' cured'), false);
                                        }
                                    }
                                    prevStatusExtra[sk] = extraNow;
                                } catch(e) {}
                            }
                        }
                    } catch(e) {}
                    prevBtlState = btlState;
                } else {
                    if (prevBtlState !== 0) {
                        logFile('[BTL] Battle ended');
                        rewardsAnnounced = false; prevBtlState = 0; prevBtlWinType = -1;
                        prevBtlSelIdx = -1; prevBtlSelCmd = -1; prevBtlWinIdx = -1;
                        btlWinAnnounced = false; inTargeting = false; lastNonZeroCmdId = 0;
                        prevStatusPerm = {}; prevStatusTemp = {}; prevStatusExtra = {}; prevStatDeath = {};
                        equipListNames = [];
                    }
                }
            } catch(e) { if (frameCount % 1000 === 0) logFile('[BTL ERROR] ' + e.message); }

            // ── PART 2: Field menu monitoring ──
            try {
                var wasInMainMenu = inMainMenu;
                inMainMenu = (frameCount - mainMenuLastFrame) < 10;
                if (inMainMenu && !wasInMainMenu) { prevFieldCursor = -1; prevFieldPlayer = -1; }
                if (inMainMenu) {
                    var fieldCursor = fieldMenuCursorAddr.readS32();
                    var fieldPlayer = fieldMenuPlayerAddr.readS32();
                    if (fieldCursor !== prevFieldCursor) {
                        var logicalIdx = fieldCursor;
                        try { logicalIdx = fieldMenuItemTable.add(fieldCursor * 8).readS32(); } catch(e) {}
                        var itemName = msMenuTextCache[logicalIdx] || ('item ' + logicalIdx);
                        speak(itemName, true);
                        prevFieldCursor = fieldCursor;
                    }
                    if (fieldPlayer !== prevFieldPlayer && prevFieldPlayer !== -1) {
                        var playerName = (fieldPlayer >= 0 && fieldPlayer < CHARACTER_NAMES.length)
                            ? CHARACTER_NAMES[fieldPlayer] : ('player ' + fieldPlayer);
                        speak(playerName, true);
                        prevFieldPlayer = fieldPlayer;
                    } else { prevFieldPlayer = fieldPlayer; }
                }
            } catch(e) {}

            // ── PART 2b: TkWindow monitoring (title screen, popups) ──
            try {
                var eventId = eventIdAddr.readS32();
                for (var w = 0; w < 8; w++) {
                    try {
                        var ctx = readTkWindowContext(w);
                        if (!ctx || ctx.vis === 0) { prevTkVisible['w' + w] = 0; continue; }
                        var key = 'w' + w;
                        if (prevTkCursors[key] === undefined) prevTkCursors[key] = ctx.idx;
                        var wasVis = prevTkVisible[key] || 0;
                        var oldTp = prevTkTextPtrs[key] || ptr(0);
                        var tpChanged = !ctx.textPtr.equals(oldTp) && !oldTp.isNull();
                        prevTkTextPtrs[key] = ctx.textPtr;
                        if (ctx.vis === 1 && wasVis === 0) {
                            initialTkTextPtrs[key] = ctx.textPtr;
                            if (eventId === 0x17 && w === 2) {
                                speak(ctx.idx < TITLE_ITEMS.length ? TITLE_ITEMS[ctx.idx] : '?', true);
                            } else {
                                var segs = getTkSegments(ctx);
                                if (segs.length > 0) speak(cleanText(segs[0]), true);
                                if (ctx.idx + 1 < segs.length) speak(cleanText(segs[ctx.idx + 1]), false);
                            }
                            prevTkCursors[key] = ctx.idx;
                        } else if (ctx.vis === 1 && ctx.idx !== prevTkCursors[key]) {
                            if (eventId === 0x17 && w === 2) {
                                speak(ctx.idx < TITLE_ITEMS.length ? TITLE_ITEMS[ctx.idx] : '?', true);
                            } else {
                                var segs2 = getTkSegments(ctx);
                                var firstItem = segs2.length - ctx.items;
                                var target2 = firstItem + ctx.idx;
                                if (target2 >= 0 && target2 < segs2.length) speak(cleanText(segs2[target2]), true);
                            }
                            prevTkCursors[key] = ctx.idx;
                        } else if (tpChanged && ctx.vis === 1) {
                            var segs3 = getTkSegments(ctx);
                            if (segs3.length > 0) speak(cleanText(segs3[0]), true);
                            prevTkTextPtrs[key] = ctx.textPtr;
                        }
                        prevTkVisible[key] = ctx.vis;
                    } catch(e) {}
                }
            } catch(e) {}

            // ── PART 3: Sphere Grid monitoring ──
            try {
                var sgOpen = sgridIsOpenAddr.readU8() !== 0;
                if (sgOpen) {
                    var eng = sgridEnginePtrAddr.readPointer();
                    if (!eng.isNull()) {
                        var chrId = eng.add(0x115BC).readU8();
                        var selNode = eng.add(0x112FC).readS32();
                        if (chrId !== prevSgridChrId) {
                            var slv = readSgridSlv(chrId);
                            speak(CHARACTER_NAMES[chrId] + (slv ? ', S.Lv ' + slv.available : ''), true);
                            prevSgridChrId = chrId;
                        }
                        if (selNode !== prevSgridSelectedNode && selNode >= 0) {
                            var node = readSgridNode(eng, selNode);
                            if (node) {
                                var desc = node.typeName || 'ability node';
                                if (!node.typeName) {
                                    var nctPtr = eng.add(0x11654).readPointer();
                                    if (!nctPtr.isNull()) {
                                        var nctSegs = decodeFFXSegments(nctPtr, 128, 0);
                                        if (nctSegs.length > 0) desc = nctSegs[0];
                                    }
                                }
                                speak(desc, true);
                            }
                            prevSgridSelectedNode = selNode;
                        }
                    }
                }
                if (!sgOpen && prevSgridOpen) { prevSgridSelectedNode = -1; prevSgridChrId = -1; }
                prevSgridOpen = sgOpen;
            } catch(e) {}
        }
    });
    logFile('Per-frame hook installed');
} catch(e) { logFile('FAIL Per-frame hook: ' + e.message); }

logFile('FFX Screen Reader Mod fully loaded');
