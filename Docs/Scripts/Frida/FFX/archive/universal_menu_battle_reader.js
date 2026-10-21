// @desc Universal menu reader — battle, field, config, items, sphere grid, shops
// @python
//
// Unified script covering all menu systems:
//   - Battle menus: BtlWindow state, command IDs, targeting, damage, status effects
//   - Field main menu: TkMenu cursor tracking via MsMenuGetText(9,...)
//   - Config menu: scene 0x0A fn_ctrl hook, NativeFunction text resolution
//   - Items/popups: TkWindow monitoring (8 slots), text pointer shift detection
//   - Sphere Grid: LpAbilityMapEngine struct, node types, character tracking
//   - Shops: TODrawSelectCursor hook
//
// Attach to FFX.exe. Ctrl+C to stop.

var logPath = 'D:/Games/Dev/Custom/FFX/ffx-x2-screen-reader/Docs/Scripts/Frida/output/FFX/universal_menu_battle_reader.log';
var logHandle = new File(logPath, 'w');
function logFile(msg) { logHandle.write(msg + '\n'); logHandle.flush(); }
function logBoth(msg) { console.log(msg); logFile(msg); }

var base = Process.enumerateModules()[0].base;
function rva(offset) { return base.add(offset); }

logBoth('[*] Battle/TkMenu discovery script');

// ── Addresses (all Fahrenheit RVAs — use directly) ──

// Battle system
var btlBaseAddr       = rva(0xD2A8D0);          // Btl struct (direct, not pointer)
var btlStateAddr      = rva(0xD2A8D0 + 0x10);   // Btl.battle_state (byte)
var btlWinArrayAddr   = rva(0xF3C910);           // BtlWindow array (direct)
var btlCurWinIdxAddr  = rva(0x1FCC092);          // Current window index (byte)
var btlCommandBinAddr = rva(0xD2A8D0 + 0x5C);   // ptr_command_bin (pointer to ExcelFile)
var btlItemBinAddr    = rva(0xD2A8D0 + 0x70);    // ptr_item_bin (pointer to ExcelFile)
var saveDataAddr      = rva(0xD2CA90);            // SaveData* (direct)
var INVENTORY_IDS_OFF = 0x3ECC;   // SaveData + 0x3ECC: 256 x ushort item IDs
var INVENTORY_CNT_OFF = 0x40CC;   // SaveData + 0x40CC: 256 x byte counts
var monsterChrsAddr   = rva(0xD34460);            // Chr* monster_characters (get_at — dereference)
var playerChrsAddr    = rva(0xD334CC);            // Chr* player_characters (get_at — dereference)
var CHR_SIZE = 0xF90;
var CHR_RAM_OFFSET = 0x540;  // ChrRam within Chr
var CHR_IN_BATTLE = 0xDC8;   // Chr.in_battle (byte)
var btlRewardAddr = rva(0x1F10EA0); // BtlRewardData (direct, game's calculated rewards)
var helpTextAddr  = rva(0xF3F4F8);  // HELP panel text buffer (128 bytes, FFX charset)

// Field menu cursor (from Ghidra decompile of FUN_008e20f0 — DAT_01871508)
var fieldMenuCursorAddr = rva(0x1471508);  // visual cursor position (int) — DAT_01871508
var fieldMenuPlayerAddr = rva(0x147150C);  // selected player index (int) — DAT_0187150C
var fieldMenuItemTable  = rva(0x1471528);  // visual→logical mapping table — DAT_01871528
// Table layout: [visual_pos * 8] = logical MsMenuGetText index (dword)

// Sphere Grid (LpAbilityMap system)
var sgridIsOpenAddr = rva(0x1685f70);      // bool at this RVA (ptr_at<bool> = direct read)
var sgridEnginePtrAddr = rva(0x1F05834);   // ptr to LpAbilityMapEngine — dereference once

// TkWindow system (Fahrenheit game_ui.cs)
// Pool at RVA 0x14408C0, 32 slots × 0x98 bytes. Allocated by FUN_008aa150.
// Activated by FUN_008aaab0 (RVA 0x4AAAB0): sets is_active=1, calls fn_init.
// Layout: fn_init +0x08, fn_state +0x0C, fn_render +0x10,
//   is_active +0x40, selected_index +0x48, num_items +0x30

// ── Constants ──

var BTL_WIN_SIZE = 0xF0;

var BTL_WIN_TYPES = {};
BTL_WIN_TYPES[0x00] = 'Main';
BTL_WIN_TYPES[0x01] = 'Black Magic';
BTL_WIN_TYPES[0x02] = 'White Magic';
BTL_WIN_TYPES[0x03] = 'Skill';
BTL_WIN_TYPES[0x04] = 'Overdrive';
BTL_WIN_TYPES[0x05] = 'Summon';
BTL_WIN_TYPES[0x06] = 'Item';
BTL_WIN_TYPES[0x07] = 'Weapon';
BTL_WIN_TYPES[0x0A] = 'Change';
BTL_WIN_TYPES[0x0C] = 'Left';
BTL_WIN_TYPES[0x0D] = 'Right';
BTL_WIN_TYPES[0x0E] = 'Special';
BTL_WIN_TYPES[0x0F] = 'Armor';
BTL_WIN_TYPES[0x11] = 'Use';
BTL_WIN_TYPES[0x14] = 'Mix';
BTL_WIN_TYPES[0x15] = 'Spare Change';
BTL_WIN_TYPES[0x16] = 'Yojimbo Pay';

var CHARACTER_NAMES = ['Tidus','Yuna','Auron','Kimahri','Wakka','Lulu','Rikku','Seymour',
    'Valefor','Ifrit','Ixion','Shiva','Bahamut','Anima','Yojimbo',
    'Cindy','Sandy','Mindy','Dummy','Dummy2'];

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
// Skill/ability nodes 0x2A-0x7E: resolved via node_change_text at runtime

// ── FFX charset decoder (reused from tk_menu_probe.js) ──

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

// ── Config menu helpers ──

function readConfigItem(cursorIdx) {
    try {
        var arrayPtr = configArrayPtrAddr.readPointer();
        if (arrayPtr.isNull()) return null;
        var itemPtr = arrayPtr.add(cursorIdx * 4).readPointer();
        if (itemPtr.isNull()) return null;
        return {
            numOptions: itemPtr.add(0x04).readS32(),
            curSelection: itemPtr.add(0x08).readS32(),
            visible: itemPtr.add(0x0C).readS32(),
            textId: itemPtr.add(0x18).readU16(),
            ptr: itemPtr
        };
    } catch(e) { return null; }
}

function resolveLocString(textId) {
    try {
        var buf = Memory.alloc(256);
        locStringCopyFn(textId, buf);
        var segs = decodeFFXSegments(buf, 256, 0);
        if (segs.length > 0) return segs[0];
    } catch(e) {}
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
    return null;
}

function resolveConfigDesc(textId) {
    try {
        var lang = getConfigLangFn();
        var textPtr = msMenuGetTextFn(7, textId, lang);
        if (!textPtr.isNull()) {
            var segs = decodeFFXSegments(textPtr, 256, 0);
            if (segs.length > 2 && segs[1] === '-') return segs[2];
        }
    } catch(e) {}
    return null;
}

// ── TkWindow helpers (Items, popups, title screen) ──

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

// ── Read monster/character name and HP from Chr array ──

function readMonsterInfo(monIdx) {
    try {
        var arrayPtr = monsterChrsAddr.readPointer();
        if (arrayPtr.isNull()) return null;
        var chrBase = arrayPtr.add(monIdx * CHR_SIZE);
        var inBattle = chrBase.add(CHR_IN_BATTLE).readU8();
        if (inBattle === 0) return null;

        var ramBase = chrBase.add(CHR_RAM_OFFSET);
        // ChrRam.name at +0x00 (40 bytes, FFX charset)
        var nameSegs = decodeFFXSegments(ramBase, 40, 0);
        var name = (nameSegs.length > 0) ? nameSegs[0] : 'Monster ' + monIdx;

        // ChrRam.current_hp at +0x1A4, ChrRam.max_hp at +0x54
        var currentHp = ramBase.add(0x1A4).readS32();
        var maxHp = ramBase.add(0x54).readS32();
        var hpPct = (maxHp > 0) ? Math.round(currentHp / maxHp * 100) : 0;

        return { name: name, currentHp: currentHp, maxHp: maxHp, hpPct: hpPct };
    } catch(e) {
        return null;
    }
}

// Build a map of sel_mons bit -> monster index (scan all 8 slots, map active ones to bits)
function buildMonsterBitMap() {
    var map = {};
    try {
        var arrayPtr = monsterChrsAddr.readPointer();
        if (arrayPtr.isNull()) return map;
        for (var i = 0; i < 8; i++) {
            var chrBase = arrayPtr.add(i * CHR_SIZE);
            var inBattle = chrBase.add(CHR_IN_BATTLE).readU8();
            if (inBattle !== 0) {
                map[i] = i;
            }
        }
    } catch(e) {}
    return map;
}

function readPlayerInfo(chrIdx) {
    try {
        var arrayPtr = playerChrsAddr.readPointer();
        if (arrayPtr.isNull()) return null;
        var chrBase = arrayPtr.add(chrIdx * CHR_SIZE);
        var inBattle = chrBase.add(CHR_IN_BATTLE).readU8();
        if (inBattle === 0) return null;

        var ramBase = chrBase.add(CHR_RAM_OFFSET);
        var currentHp = ramBase.add(0x1A4).readS32();
        var maxHp = ramBase.add(0x54).readS32();
        var currentMp = ramBase.add(0x1A8).readS32();
        var maxMp = ramBase.add(0x58).readS32();
        var name = (chrIdx < CHARACTER_NAMES.length) ? CHARACTER_NAMES[chrIdx] : ('Player ' + chrIdx);

        return { name: name, currentHp: currentHp, maxHp: maxHp, currentMp: currentMp, maxMp: maxMp };
    } catch(e) {
        return null;
    }
}

// ── Read command's party_preview flags to determine HP vs MP display ──

function getCommandPartyPreview(cmdId) {
    // Returns {showHp, showMp} from Command.party_preview byte at +0x22
    try {
        var prefix = cmdId & 0xF000;
        var binAddr = (prefix === 0x2000) ? btlItemBinAddr : btlCommandBinAddr;
        var excelPtr = binAddr.readPointer();
        if (excelPtr.isNull()) return null;

        var hdr = excelPtr.add(0x8);
        var firstIdx = hdr.readU16();
        var lastIdx = hdr.add(2).readU16();
        var elementSize = hdr.add(4).readU16();
        var dataStart = hdr.add(8).readU32();

        var excelIdx = cmdId & 0xFFF;
        if (excelIdx < firstIdx || excelIdx > lastIdx) return null;

        var elementAddr = excelPtr.add(dataStart).add((excelIdx - firstIdx) * elementSize);
        var partyPreview = elementAddr.add(0x22).readU8();

        return {
            showHp: (partyPreview & (1 << 6)) !== 0,
            showMp: (partyPreview & (1 << 1)) !== 0
        };
    } catch(e) {
        return null;
    }
}

// ── ExcelFile command text resolver ──

var excelHeaderDumped = false;

function resolveCommandText(commandId, excelBinAddr) {
    try {
        var excelPtr = excelBinAddr.readPointer();
        if (excelPtr.isNull()) return null;

        // ExcelFile: header_count at +0x0 (u16), first header at +0x8
        var headerCount = excelPtr.readU16();
        if (headerCount === 0 || headerCount > 10) return null; // sanity check

        // ExcelHeader at +0x8: first_idx(u16), last_idx(u16), element_size(u16), data_length(u16), data_start(i32)
        var hdr = excelPtr.add(0x8);
        var firstIdx   = hdr.readU16();
        var lastIdx    = hdr.add(2).readU16();
        var elementSize = hdr.add(4).readU16();
        var dataLength = hdr.add(6).readU16();
        var dataStart  = hdr.add(8).readU32(); // nint on 32-bit = 4 bytes, read as uint

        // Dump header once for verification
        if (!excelHeaderDumped) {
            excelHeaderDumped = true;
            logFile('[EXCEL HEADER] ptr=0x' + excelPtr.toString(16));
            logFile('  headerCount=' + headerCount);
            logFile('  firstIdx=0x' + firstIdx.toString(16) + ' lastIdx=0x' + lastIdx.toString(16));
            logFile('  elementSize=0x' + elementSize.toString(16) + ' dataLength=0x' + dataLength.toString(16));
            logFile('  dataStart=0x' + dataStart.toString(16));
            // Dump raw 20 bytes of ExcelFile header
            try {
                var rawBytes = new Uint8Array(excelPtr.readByteArray(20));
                var hex = '';
                for (var h = 0; h < rawBytes.length; h++) {
                    hex += ('0' + rawBytes[h].toString(16)).slice(-2) + ' ';
                }
                logFile('  raw hex: ' + hex);
            } catch(e) {}
        }

        // Strip category prefix from command IDs (0x3000+ = player commands, etc.)
        // ExcelFile indexes from 0, but game command IDs have high-byte prefix
        var excelIdx = commandId & 0xFFF;

        // Check if index is in range
        if (excelIdx < firstIdx || excelIdx > lastIdx) return null;

        // PCommand at: excelPtr + dataStart + (excelIdx - firstIdx) * elementSize
        var elementOffset = (excelIdx - firstIdx) * elementSize;
        var elementAddr = excelPtr.add(dataStart).add(elementOffset);

        // Command.name_offset at PCommand+0x00 (ushort)
        var nameOffset = elementAddr.readU16();

        // Text at: excelPtr + dataStart + dataLength + nameOffset
        var textAddr = excelPtr.add(dataStart + dataLength + nameOffset);

        var segments = decodeFFXSegments(textAddr, 64, 0);
        if (segments.length > 0) return segments[0];
        return null;
    } catch(e) {
        logFile('[resolveCommandText ERROR] ' + e.message);
        return null;
    }
}

// ── State tracking ──

// Battle
var prevBtlState = 0;
var prevBtlWinIdx = -1;
var prevBtlWinState = 0;
var prevBtlWinType = -1;
var prevBtlSelIdx = -1;
var prevBtlSelCmd = -1;
var btlWinAnnounced = false; // only announce once per window open (on state=4)

// Targeting
var inTargeting = false;
var prevTargetChrs = 0;
var prevTargetMons = 0;
var targetTextCache = {}; // MsMenuGetText(1, id, 0) -> name
var targetShowHp = true;  // default: show HP
var targetShowMp = false; // determined by command's party_preview flags
var lastNonZeroCmdId = 0; // persists through window transitions

// TkMenu — track via MsMenuGetText calls (IsInTkMenuFakeLoop was unreliable)
var inMainMenu = false;
var mainMenuLastFrame = 0; // last frame MsMenuGetText(9,...) fired
var tkMenuFrameCount = 0;

// MsMenuGetText text cache — keyed by arg0_arg1, stores {name, desc}
var msMenuTextCache = {};    // legacy: arg1 -> name (main menu, arg0=9)
var msMenuDescCache = {};    // arg1 -> description (main menu, arg0=9)

// Sphere Grid state tracking
var prevSgridOpen = false;
var prevSgridSelectedNode = -1;
var prevSgridChrId = -1;
var prevSgridMoving = false;
var prevSgridNodeChangeText = '';
var prevSgridActivatedText = '';
var prevSgridChrPositions = {};

// Status effect tracking
var PERM_STATUS_NAMES = ['KO','zombie','petrify','poison','power break','magic break',
    'armor break','mental break','confuse','berserk','provoke','threaten'];
var TEMP_STATUS_NAMES = ['sleep','silence','darkness','shell','protect','reflect',
    'nul-water','nul-fire','nul-thunder','nul-ice','regen','haste','slow'];
var EXTRA_STATUS_NAMES = ['scan','','','','','','shield','boost','eject','auto-life',
    'curse','defend','guard','sentinel','doom'];
var prevStatusPerm = {};   // key = chrIdx, value = ushort
var prevStatusTemp = {};   // key = chrIdx, value = array of 13 bytes
var prevStatusExtra = {};  // key = chrIdx, value = ushort
var prevStatDeath = {};    // key = chrIdx, value = byte (Chr+0xDCC)

// Field menu cursor tracking
var prevFieldCursor = -1;
var prevFieldPlayer = -1;

// ── Field sub-menu tracking (scene-based) ──
// Scene table at RVA 0x1440848 — but we track via hook instead of polling
var sceneActivateRVA = 0x4AA0B0;   // FUN_008aa0b0: activates a scene
var activeSubScene = -1;           // currently active sub-menu scene ID (-1 = none/main)
var prevSubScene = -1;

// Scene ID → menu name mapping (from field_submenu_discover.js log data)
var SCENE_NAMES = {};
SCENE_NAMES[0x01] = 'Main Menu';
SCENE_NAMES[0x02] = 'Abilities';
SCENE_NAMES[0x15] = 'Overdrive';
SCENE_NAMES[0x04] = 'Equip';
SCENE_NAMES[0x06] = 'Status';
SCENE_NAMES[0x07] = 'Aeons';
SCENE_NAMES[0x09] = 'Formation';
SCENE_NAMES[0x0A] = 'Config';
SCENE_NAMES[0x0B] = 'Help';

// ── Config menu (scene 0x0A) — follows game's fn_ctrl logic ──
var configCursorAddr   = rva(0x146A404);  // cursor index (byte)
var configCountAddr    = rva(0x146A408);  // total item count (int)
var configArrayPtrAddr = rva(0x146A40C);  // pointer to item struct array
var configSpecialIdx   = rva(0x146A3F8);  // Music Type item index (special rendering)
var configFnCtrlRVA    = 0x4CB0B0;        // Config scene fn_ctrl
var prevConfigCursor = -1;
var prevConfigSelection = -1;
var configActive = false;

// NativeFunction: call game's text functions directly
var msMenuGetTextFn = new NativeFunction(rva(0x38FD40), 'pointer', ['int', 'int', 'int']);
var getConfigLangFn = new NativeFunction(rva(0x3852B0), 'int', []);
var locStringCopyFn = new NativeFunction(rva(0x241480), 'void', ['int', 'pointer']);

// ── TkWindow monitoring (Items, popups, title screen) ──
var tkWinArrayAddr = rva(0x14676B0);   // 8 window pointers
var tkCurArrayAddr = rva(0x1468A50);   // 8 cursor pointers
var prevTkCursors = {};
var prevTkVisible = {};
var prevTkTextPtrs = {};
var initialTkTextPtrs = {};
var isTkPopup = {};
var TITLE_ITEMS = ['New Game', 'Load'];

// Battle results text tracking (MsMenuGetText arg0=4)
var battleResultsAnnounced = {};

// Status diagnostics — dump once per battle
var statusDiagDone = false;

// TODrawSelectCursor tracking — for shop/inn menus
var drawCursorLogCount = 0;
var prevDrawCursorArgs = '';

// ── Read BtlWindow fields ──

function readBtlWindow(idx) {
    var winBase = btlWinArrayAddr.add(idx * BTL_WIN_SIZE);
    try {
        return {
            state:       winBase.add(0x01).readU8(),
            type:        winBase.add(0x06).readU16(),
            chrId:       winBase.add(0x08).readU8(),
            submenuDepth: winBase.add(0x0B).readU8(),
            winCmdId:    winBase.add(0x0C).readU16(),
            selIdxRender: winBase.add(0x0E).readU8(),
            selCmdIdRender: winBase.add(0x16).readU16(),
            selCmdId:    winBase.add(0x1E).readU16(),
            cmdListPtr:  winBase.add(0x20).readPointer(),
            cmdListLen:  winBase.add(0x24).readU32(),
            scrollPos:   winBase.add(0x40).readU8(),
            selIdx:      winBase.add(0x42).readU8(),
            validSelAmt: winBase.add(0x44).readU32(),
            secInMenu:   winBase.add(0xE0).readFloat()
        };
    } catch(e) {
        return null;
    }
}

// ── Decode one null-terminated FFX text entry (0x03/0x0A = space, 0x00 = stop) ──

function decodeFFXString(addr, maxLen) {
    var result = '';
    try {
        var bytes = new Uint8Array(addr.readByteArray(maxLen));
        var i = 0;
        while (i < maxLen) {
            var b = bytes[i];
            if (b === 0x00) break; // end of this text entry
            if (b === 0x03 || b === 0x0A) { result += ' '; i++; continue; } // line break = space
            if (b === 0x13 && i + 1 < maxLen) {
                // Character name substitution: next byte - 0x30 = character index
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

// ── Sphere Grid helpers ──

function readSgridNode(engineBase, nodeIdx) {
    try {
        var nodeBase = engineBase.add(0x808 + nodeIdx * 0x28);
        var rawType = nodeBase.add(0x6).readS16();
        var nodeType = (rawType === -1) ? 0xFF : (rawType & 0xFF);
        var typeName = NODE_TYPE_NAMES[nodeType] || null;
        var activatedBy = nodeBase.add(0x21).readU8();
        var props = nodeBase.add(0x22).readU8();
        var linkCount = 0;
        for (var li = 0; li < 5; li++) {
            if (nodeBase.add(0xC + li * 4).readU32() !== 0) linkCount++;
        }
        return {
            type: nodeType, typeName: typeName,
            activatedBy: activatedBy, props: props,
            x: nodeBase.add(0x0).readS16(), y: nodeBase.add(0x2).readS16(),
            linkCount: linkCount,
            canTarget: (props & 1) !== 0, highlighted: (props & 2) !== 0
        };
    } catch(e) { return null; }
}

function readSgridSlv(chrIdx) {
    try {
        var plySave = saveDataAddr.add(0x55CC + chrIdx * 0x94);
        return { available: plySave.add(0x3B).readU8(), spent: plySave.add(0x3C).readU8() };
    } catch(e) { return null; }
}

// ── Read description text from ExcelFile (Command.desc_offset at +0x08) ──

function resolveCommandDesc(cmdId) {
    try {
        var prefix = cmdId & 0xF000;
        if (prefix === 0x5000) return null;
        var binAddr = (prefix === 0x2000) ? btlItemBinAddr : btlCommandBinAddr;
        var excelPtr = binAddr.readPointer();
        if (excelPtr.isNull()) return null;

        var hdr = excelPtr.add(0x8);
        var firstIdx = hdr.readU16();
        var lastIdx = hdr.add(2).readU16();
        var elementSize = hdr.add(4).readU16();
        var dataLength = hdr.add(6).readU16();
        var dataStart = hdr.add(8).readU32();

        var excelIdx = cmdId & 0xFFF;
        if (excelIdx < firstIdx || excelIdx > lastIdx) return null;

        var elementAddr = excelPtr.add(dataStart).add((excelIdx - firstIdx) * elementSize);
        var descOffset = elementAddr.add(0x08).readU16();
        var textAddr = excelPtr.add(dataStart + dataLength + descOffset);
        var text = decodeFFXString(textAddr, 256);
        return (text.length > 0) ? text : null;
    } catch(e) { return null; }
}

// ── Look up item quantity from SaveData inventory ──

function getItemQuantity(itemId) {
    try {
        var invIds = saveDataAddr.add(INVENTORY_IDS_OFF);
        for (var i = 0; i < 256; i++) {
            var id = invIds.add(i * 2).readU16();
            if (id === itemId) {
                return saveDataAddr.add(INVENTORY_CNT_OFF + i).readU8();
            }
            if (id === 0) break; // end of inventory
        }
    } catch(e) {}
    return 0;
}

// ── Read command/item name from command_list[selIdx] ──

// Call game's MsWeaponName directly to resolve equipment names
var msWeaponNameFn = new NativeFunction(rva(0x3A0C70), 'pointer', ['uint', 'uint', 'int', 'pointer']);

// Read auto-ability names from runtime equipment inventory
// Runtime inventory at RVA 0xD30F2C (NOT SaveData), Equipment is 0x16 bytes
// Abilities at Equipment+0x0E (4 shorts), resolved via a_ability ExcelFile at RVA 0xD2A944
var runtimeEquipAddr = rva(0xD30F2C);
var aAbilityBinAddr = rva(0xD2A944);  // auto-ability ExcelFile pointer

function readEquipAbilities(equipId) {
    try {
        var invIdx = equipId & 0xFFF;
        if (invIdx > 199) return null;
        var equipBase = runtimeEquipAddr.add(invIdx * 0x16);
        var slotCount = equipBase.add(0x0B).readU8();
        if (slotCount === 0) return null;

        var abilNames = [];
        var aExcelPtr = aAbilityBinAddr.readPointer();
        if (aExcelPtr.isNull()) return null;

        for (var ai = 0; ai < 4 && ai < slotCount; ai++) {
            var abilId = equipBase.add(0x0E + ai * 2).readS16();
            if (abilId === -1 || abilId === 0) continue; // 0xFF = empty slot
            // Resolve via auto-ability ExcelFile (same lookup as FUN_007909c0)
            var abilName = resolveCommandText(abilId & 0xFFFF, aAbilityBinAddr);
            if (abilName) abilNames.push(abilName);
        }
        return (abilNames.length > 0) ? abilNames.join(', ') : null;
    } catch(e) { return null; }
}

function readEquipmentName(equipId, chrId) {
    try {
        inOurWeaponCall = true;
        var namePtr = msWeaponNameFn(equipId, chrId & 0xFF, 0, ptr(0));
        inOurWeaponCall = false;
        if (namePtr.isNull()) return null;
        var name = decodeFFXString(namePtr, 64);
        return (name.length > 0) ? name : null;
    } catch(e) { inOurWeaponCall = false; return null; }
}

function readCommandName(win) {
    if (win.cmdListPtr.isNull() || win.cmdListLen === 0) return null;
    if (win.selIdx >= win.cmdListLen) return null;

    try {
        var cmdId = win.cmdListPtr.add(win.selIdx * 2).readU16();
        var prefix = cmdId & 0xF000;
        var name = null;

        // Special handling for Weapon/Armor/Change/Summon window types
        if (win.type === 0x07 || win.type === 0x0F) {
            // Weapon or Armor — use frame-ordered names from game's MsWeaponName rendering
            if (win.selIdx < equipListNames.length) {
                return equipListNames[win.selIdx];
            }
            return 'equipment ' + win.selIdx;
        }

        if (win.type === 0x0A || win.type === 0x05) {
            // Change/Switch (0x0A) or Summon (0x05) — command_list has character indices
            // Summon: 0x8=Valefor, 0x9=Ifrit, 0xa=Ixion, etc.
            var chrIdx = cmdId & 0xFF;
            var pArr = playerChrsAddr.readPointer();
            if (!pArr.isNull() && chrIdx < 0x1f) {
                var ns = decodeFFXSegments(pArr.add(chrIdx * CHR_SIZE + CHR_RAM_OFFSET), 40, 0);
                if (ns.length > 0) return ns[0];
            }
            return (chrIdx < CHARACTER_NAMES.length) ? CHARACTER_NAMES[chrIdx] : 'chr' + chrIdx;
        }

        if (prefix === 0x2000) {
            // Item IDs — use item_bin
            name = resolveCommandText(cmdId, btlItemBinAddr);
            if (name) {
                var qty = getItemQuantity(cmdId);
                return name + ' (' + qty + ')';
            }
        } else if (prefix === 0x3000) {
            // Player command IDs — use command_bin
            name = resolveCommandText(cmdId, btlCommandBinAddr);
        } else {
            // Unknown prefix — try both
            name = resolveCommandText(cmdId, btlCommandBinAddr);
            if (!name) name = resolveCommandText(cmdId, btlItemBinAddr);
        }

        return name || ('cmd:0x' + cmdId.toString(16));
    } catch(e) {
        return null;
    }
}

// ── Hook: getText (0x2DB320) — intercept text API calls ──

try {
    Interceptor.attach(rva(0x2DB320), {
        onEnter: function(args) { this.id = args[0].toInt32(); },
        onLeave: function(retval) {
            if (!retval.isNull()) {
                try {
                    var str = retval.readUtf8String();
                    if (str && str.length > 0) {
                        logBoth('[getText] id=' + this.id + ' "' + str + '"');
                    }
                } catch(e) {}
            }
        }
    });
    logBoth('[OK] getText hooked');
} catch(e) { logBoth('[FAIL] getText: ' + e.message); }

// ── Hook: MsMenuGetText (0x38FD40) — menu text by ID ──

var msMenuBacktraceDone = false;
try {
    Interceptor.attach(rva(0x38FD40), {
        onEnter: function(args) {
            this.ecx = ptr(this.context.ecx);
            this.arg0 = args[0].toInt32();
            this.arg1 = args[1].toInt32();
            this.arg2 = args[2].toInt32();

            // Backtrace once to find caller (leads to cursor variable)
            if (!msMenuBacktraceDone && this.arg0 === 9) {
                msMenuBacktraceDone = true;
                try {
                    var bt = Thread.backtrace(this.context, Backtracer.FUZZY);
                    logFile('[MsMenuGetText BACKTRACE] for arg1=' + this.arg1 + ':');
                    for (var i = 0; i < bt.length; i++) {
                        var rvaAddr = bt[i].sub(base);
                        logFile('  [' + i + '] 0x' + bt[i].toString(16) + ' (RVA 0x' + rvaAddr.toString(16) + ')');
                    }
                } catch(e) {
                    logFile('[MsMenuGetText BACKTRACE ERROR] ' + e.message);
                }
            }
        },
        onLeave: function(retval) {
            if (retval.isNull()) return;
            // Decode text segments — pattern: name | - | description | - | next name | ...
            var segs = null;
            try { segs = decodeFFXSegments(retval, 256, 0); } catch(e) {}

            // Main menu (arg0=9) — cache names and descriptions
            if (this.arg0 === 9) {
                mainMenuLastFrame = frameCount;
                if (segs && segs.length > 0) {
                    msMenuTextCache[this.arg1] = segs[0];
                    if (segs.length > 2 && segs[1] === '-') {
                        msMenuDescCache[this.arg1] = segs[2];
                    }
                }
            }
            // Targeting (arg0=1)
            if (this.arg0 === 1 && segs && segs.length > 0) {
                targetTextCache[this.arg1] = segs[0];
            }
            // Battle results (arg0=4)
            if (this.arg0 === 4) {
                logFile('[MsMenuGetText] args=(4,' + this.arg1 + ',' + this.arg2 + ')');
                if (!rewardsAnnounced) {
                    rewardsAnnounced = true;
                    try {
                        announceRewardsPhase1();
                        announceRewardsPhase2();
                    } catch(e) { logFile('[REWARDS ERR] ' + e.message); }
                }
            }
            // File-only logging
            logFile('[MsMenuGetText] args=(' + this.arg0 + ',' + this.arg1 + ',' + this.arg2 + ')');
        }
    });
    logBoth('[OK] MsMenuGetText hooked');
} catch(e) { logBoth('[FAIL] MsMenuGetText: ' + e.message); }

// ── Hook: Scene activation (0x4AA0B0) — tracks which sub-menu is active ──

try {
    Interceptor.attach(rva(sceneActivateRVA), {
        onEnter: function(args) {
            var sceneId = args[0].toInt32();
            activeSubScene = sceneId;
            var sceneName = SCENE_NAMES[sceneId] || ('scene 0x' + sceneId.toString(16));
            // Scene 0x1 = returning to main cursor, not a sub-menu
            if (sceneId !== 0x01) {
                logBoth('[SUBMENU] ' + sceneName);
            }
            logFile('[SCENE ACTIVATE] scene=0x' + sceneId.toString(16) + ' (' + sceneName + ')');
        }
    });
    logBoth('[OK] Scene activate hooked');
} catch(e) { logBoth('[FAIL] Scene activate: ' + e.message); }

// ── Hook: Config fn_ctrl (0x4CB0B0) — reads cursor and announces settings ──

try {
    Interceptor.attach(rva(configFnCtrlRVA), {
        onEnter: function(args) { this.param1 = args[0]; },
        onLeave: function() {
            var state = this.param1.add(0x1C).readS32();
            if (state < 3) {
                if (!configActive) { configActive = true; logBoth('[CONFIG] Opened'); }
                return;
            }
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
            var settingName = '';
            var optionName = '';

            if (isSpecial) {
                settingName = resolveLocString(0x1CD) || 'Music';
                var musicOptIds = [0x1CE, 0x1D0];
                if (item.curSelection >= 0 && item.curSelection < musicOptIds.length) {
                    optionName = resolveLocString(musicOptIds[item.curSelection]) || ('option ' + item.curSelection);
                } else { optionName = 'option ' + item.curSelection; }
            } else {
                settingName = resolveConfigText(item.textId) || ('setting ' + item.textId);
                try {
                    var optTextId = item.ptr.add(0x1C + item.curSelection * 4).readU32();
                    optionName = resolveConfigText(optTextId) || ('option ' + item.curSelection);
                } catch(e) { optionName = 'option ' + item.curSelection; }
            }

            if (cursorChanged) {
                logBoth('[CONFIG] ' + settingName + ': ' + optionName);
            } else {
                logBoth('[CONFIG] ' + optionName);
            }
            prevConfigCursor = cursor;
            prevConfigSelection = item.curSelection;
        }
    });
    logBoth('[OK] Config fn_ctrl hooked');
} catch(e) { logBoth('[FAIL] Config fn_ctrl: ' + e.message); }

// ── Hook: TkMenuStartScene (0x67C880) ──

try {
    Interceptor.attach(rva(0x67C880), {
        onEnter: function(args) {
            var ecx = ptr(this.context.ecx);
            logFile('[TkMenuStartScene] ecx=0x' + ecx.toString(16) +
                ' arg0=0x' + args[0].toString(16));
        }
    });
    logBoth('[OK] TkMenuStartScene hooked');
} catch(e) { logBoth('[FAIL] TkMenuStartScene: ' + e.message); }

// ── Hook: TkMenuStopScene (0x67C8A0) ──

try {
    Interceptor.attach(rva(0x67C8A0), {
        onEnter: function(args) {
            var ecx = ptr(this.context.ecx);
            logBoth('[TkMenuStopScene] ecx=0x' + ecx.toString(16));
        }
    });
    logBoth('[OK] TkMenuStopScene hooked');
} catch(e) { logBoth('[FAIL] TkMenuStopScene: ' + e.message); }

// ── Hook: TkMenuExecScene (0x67B460) — per-frame main menu execution ──
// Discovery: dump ECX object data to find cursor index for main menu

var tkExecSceneDumpCount = 0;
try {
    Interceptor.attach(rva(0x67B460), {
        onEnter: function(args) {
            var ecx = ptr(this.context.ecx);
            // Dump ECX object on first 3 calls (always fires during menus)
            if (tkExecSceneDumpCount < 3) {
                tkExecSceneDumpCount++;
                logFile('[TkMenuExecScene #' + tkExecSceneDumpCount + '] ecx=0x' + ecx.toString(16) +
                    ' arg0=0x' + args[0].toString(16));
                try {
                    var bytes = new Uint8Array(ecx.readByteArray(256));
                    for (var row = 0; row < 16; row++) {
                        var hex = '';
                        var off = row * 16;
                        for (var col = 0; col < 16; col++) {
                            hex += ('0' + bytes[off + col].toString(16)).slice(-2) + ' ';
                        }
                        logFile('  +0x' + ('0' + off.toString(16)).slice(-2) + ': ' + hex);
                    }
                } catch(e) { logFile('  read error: ' + e.message); }
            }
        }
    });
    logBoth('[OK] TkMenuExecScene hooked');
} catch(e) { logBoth('[FAIL] TkMenuExecScene: ' + e.message); }

// ── Hook: TODrawSelectCursor (0x4B5F60) — cursor/selector renderer ──
// This draws the menu cursor and must receive the cursor position

try {
    Interceptor.attach(rva(0x4B5F60), {
        onEnter: function(args) {
            if (!inMainMenu) return;
            var ecx = ptr(this.context.ecx);
            // Log args — one of them should be cursor index or position
            var sig = 'ecx=0x' + ecx.toString(16);
            for (var a = 0; a < 6; a++) {
                try { sig += ' a' + a + '=' + args[a].toInt32(); } catch(e) { break; }
            }
            // Only log when args change (avoid per-frame spam)
            if (sig !== prevDrawCursorArgs) {
                prevDrawCursorArgs = sig;
                drawCursorLogCount++;
                if (drawCursorLogCount <= 30) {
                    logBoth('[DRAW CURSOR] ' + sig);
                }
                logFile('[TODrawSelectCursor] ' + sig);
            }
        }
    });
    logBoth('[OK] TODrawSelectCursor hooked');
} catch(e) { logBoth('[FAIL] TODrawSelectCursor: ' + e.message); }

// ── Hook: Damage number display (FUN_0079fa20, RVA 0x39FA20) ──
// Called when the game displays a damage/heal number on screen.
// param_1=charId, param_2=type(1=HP,3=MP,4=CTB), param_3=value (negative=heal)

try {
    Interceptor.attach(rva(0x39FA20), {
        onEnter: function(args) {
            var chrId = args[0].toInt32() & 0xFF;
            var dmgType = args[1].toInt32();
            var value = args[2].toInt32();

            if (value === 0) return;

            // Resolve name using the game's own lookup logic (FUN_00794030):
            // chrId < 0x1f: playerArray + chrId * 0xF90 (players + aeons + monsters in shared array)
            // chrId >= 0x1f: monsterArray + (chrId - 0x1f) * 0x390
            var name = '';
            try {
                if (chrId < 0x1f) {
                    var pArr = playerChrsAddr.readPointer();
                    var chrPtr = pArr.add(chrId * CHR_SIZE);
                    // Read name from ChrRam at +0x540
                    var nameSegs = decodeFFXSegments(chrPtr.add(CHR_RAM_OFFSET), 40, 0);
                    name = (nameSegs.length > 0) ? nameSegs[0] : '';
                    // Fallback to CHARACTER_NAMES for known player indices
                    if (name === '' && chrId < CHARACTER_NAMES.length) {
                        name = CHARACTER_NAMES[chrId];
                    }
                }
                if (name === '') name = 'chr' + chrId;
            } catch(e) { name = 'chr' + chrId; }

            // Terse damage/heal format for screen reader speed
            var absVal = (value < 0) ? -value : value;
            var isHeal = (value < 0);
            var msg = '';
            if (dmgType === 1 || dmgType === 0) {
                msg = isHeal ? (name + ': healed for ' + absVal) : (name + ': ' + absVal + ' damage');
            } else if (dmgType === 3) {
                msg = isHeal ? (name + ': restored ' + absVal + ' MP') : (name + ': ' + absVal + ' MP damage');
            } else {
                msg = isHeal ? (name + ': healed for ' + absVal) : (name + ': ' + absVal + ' damage');
            }
            logBoth(msg);
            logFile('  chrId=' + chrId + ' type=' + dmgType + ' raw=' + value);
        }
    });
    logBoth('[OK] Damage display hooked');
} catch(e) { logBoth('[FAIL] Damage display: ' + e.message); }

// ── Hook: MsWeaponName (0x3A0C70) — cache equipment names by equipId ──
// The game calls this with the REAL equipment IDs (different from command_list IDs).
// Cache: equipId → name, so we can look up by ID when focusing.

var equipListNames = [];     // frame-ordered weapon/armor names from game's own rendering
var equipListIds = [];       // parallel array of equipment IDs (0x5xxx inventory indices)
var equipListLastFrame = 0;
var inOurWeaponCall = false;  // flag to prevent our calls from polluting the cache
try {
    Interceptor.attach(rva(0x3A0C70), {
        onEnter: function(args) {
            this.equipId = args[0].toInt32();
            this.skip = inOurWeaponCall;
        },
        onLeave: function(retval) {
            if (this.skip || retval.isNull()) return;
            try {
                var name = decodeFFXString(retval, 64);
                if (name.length > 0) {
                    if (frameCount !== equipListLastFrame) {
                        equipListNames = [];
                        equipListIds = [];
                        equipListLastFrame = frameCount;
                    }
                    if (equipListIds.indexOf(this.equipId) === -1) {
                        equipListNames.push(name);
                        equipListIds.push(this.equipId);
                    }
                }
            } catch(e) {}
        }
    });
    logBoth('[OK] MsWeaponName hooked');
} catch(e) { logBoth('[FAIL] MsWeaponName: ' + e.message); }

// ── Hook: TOBtlDrawWeaponAbilityWindow (0x4F34C0) — capture ability equipment IDs ──
var abilEquipIds = [];       // frame-ordered equipment IDs for ability display
var abilEquipLastFrame = 0;
try {
    Interceptor.attach(rva(0x4F34C0), {
        onEnter: function(args) {
            var equipId = args[0].toInt32();
            if (frameCount !== abilEquipLastFrame) {
                abilEquipIds = [];
                abilEquipLastFrame = frameCount;
            }
            if (abilEquipIds.indexOf(equipId) === -1) {
                abilEquipIds.push(equipId);
                logFile('[ABIL EQUIP] id=0x' + equipId.toString(16));
            }
        }
    });
    logBoth('[OK] WeaponAbilityWindow hooked');
} catch(e) { logBoth('[FAIL] WeaponAbilityWindow: ' + e.message); }

// ── Hook: Battle results renderers — discovery ──
// Previous hook on 0x4971B0 was WRONG — that's TOBtlDrawTidusLimitWindow (Overdrive UI).
// Correct results functions from Fahrenheit: MsBtlStartAP, TOBtlDrawGet*MessageWindow.

var rewardsAnnounced = false;

// Helper: read battle state info for logging context
function readBtlContext() {
    try {
        var bs = btlStateAddr.readU8();
        var bet = btlBaseAddr.add(0x2121).readU8();
        return 'btlState=' + bs + ' endType=' + bet;
    } catch(e) { return 'btlState=ERR'; }
}

// Helper: dump full BtlRewardData to file
function dumpBtlRewardData(tag) {
    try {
        var ctx = readBtlContext();
        logFile('[' + tag + '] ' + ctx);
        // in_battle + shared_ap (18 bools each)
        var inBtl = [], shAp = [];
        for (var i = 0; i < 18; i++) {
            inBtl.push(btlRewardAddr.add(i).readU8());
            shAp.push(btlRewardAddr.add(0x24 + i).readU8());
        }
        logFile('  in_battle: [' + inBtl.join(',') + ']');
        logFile('  shared_ap: [' + shAp.join(',') + ']');
        // get_ap and get_ap_temp (18 uint32 each)
        var apParts = [], apTmpParts = [];
        for (var j = 0; j < 18; j++) {
            var ap = btlRewardAddr.add(0x38 + j * 4).readU32();
            var apT = btlRewardAddr.add(0x80 + j * 4).readU32();
            if (ap > 0 || inBtl[j]) apParts.push(CHARACTER_NAMES[j] + ':' + ap);
            if (apT > 0 || inBtl[j]) apTmpParts.push(CHARACTER_NAMES[j] + ':' + apT);
        }
        logFile('  get_ap: [' + apParts.join(', ') + ']');
        logFile('  get_ap_temp: [' + apTmpParts.join(', ') + ']');
        // gil, items, gear
        var gil = btlRewardAddr.add(0xCC).readU32();
        var itemCnt = btlRewardAddr.add(0xD0).readU8();
        var keyCnt = btlRewardAddr.add(0xD1).readU8();
        var gearCnt = btlRewardAddr.add(0xD2).readU8();
        logFile('  gil=' + gil + ' items=' + itemCnt + ' keyItems=' + keyCnt + ' gear=' + gearCnt);
        for (var k = 0; k < itemCnt && k < 8; k++) {
            var iid = btlRewardAddr.add(0xD4 + k * 2).readU16();
            var iamt = btlRewardAddr.add(0xE4 + k).readU8();
            logFile('  item[' + k + ']: id=0x' + iid.toString(16) + ' amt=' + iamt);
        }
        // Console summary
        var apSum = apParts.length > 0 ? apParts.join(', ') : 'none';
        return 'AP=[' + apSum + '] gil=' + gil + ' items=' + itemCnt + ' gear=' + gearCnt;
    } catch(e) { logFile('[' + tag + ' DUMP ERR] ' + e.message); return 'ERR'; }
}

// Phase 1: AP per character (triggered when HELP says "Ability Points earned.")
// AP is in get_ap_temp (offset 0x80), NOT get_ap (offset 0x38, already consumed)
function announceRewardsPhase1() {
    dumpBtlRewardData('RESULTS_PHASE1');
    var apParts = [];
    for (var i = 0; i < 18; i++) {
        var inBtl = btlRewardAddr.add(i).readU8();
        if (!inBtl) continue;
        var ap = btlRewardAddr.add(0x80 + i * 4).readU32(); // get_ap_temp
        var name = (i < CHARACTER_NAMES.length) ? CHARACTER_NAMES[i] : ('chr' + i);
        apParts.push(name + ' ' + ap + ' AP');
    }
    if (apParts.length > 0) logBoth(apParts.join(', '));
}

// Phase 2: Items, equipment, gil (triggered when HELP says "Items, equipment...")
function announceRewardsPhase2() {
    logFile('[RESULTS_PHASE2]');

    // Items
    var itemCnt = btlRewardAddr.add(0xD0).readU8();
    if (itemCnt > 0) {
        var itemParts = [];
        for (var ii = 0; ii < itemCnt && ii < 8; ii++) {
            var itemId = btlRewardAddr.add(0xD4 + ii * 2).readU16();
            var itemAmt = btlRewardAddr.add(0xE4 + ii).readU8();
            var itemName = resolveCommandText(itemId, btlItemBinAddr);
            itemParts.push((itemName || ('item 0x' + itemId.toString(16))) + ' x' + itemAmt);
        }
        logBoth(itemParts.join(', '));
    }

    // Equipment drops
    var gearCnt = btlRewardAddr.add(0xD2).readU8();
    if (gearCnt > 0) {
        for (var gi = 0; gi < gearCnt && gi < 8; gi++) {
            var gearBase = btlRewardAddr.add(0xFE + gi * 0x16);
            var nameId = gearBase.readU16();
            logFile('[GEAR] idx=' + gi + ' nameId=0x' + nameId.toString(16));
            logBoth('Equipment obtained (id 0x' + nameId.toString(16) + ')');
        }
    }

    // Key items
    var keyCnt = btlRewardAddr.add(0xD1).readU8();
    if (keyCnt > 0) {
        var keyId = btlRewardAddr.add(0xEC).readU16();
        logBoth('Key item obtained (id 0x' + keyId.toString(16) + ')');
    }

    // Gil
    var gil = btlRewardAddr.add(0xCC).readU32();
    if (gil > 0) logBoth(gil + ' Gil');
}

// 1. MsBtlStartAP (0x398EA0) — fires once when AP phase begins
try {
    Interceptor.attach(rva(0x398EA0), {
        onEnter: function(args) {
            var summary = dumpBtlRewardData('MsBtlStartAP');
            logBoth('[MsBtlStartAP] ' + summary);
        }
    });
    logBoth('[OK] MsBtlStartAP hooked');
} catch(e) { logBoth('[FAIL] MsBtlStartAP: ' + e.message); }

// 2. TOBtlDrawGetMoneyMessageWindow (0x4935D0) — gil display
var moneyMsgCount = 0;
try {
    Interceptor.attach(rva(0x4935D0), {
        onEnter: function(args) {
            moneyMsgCount++;
            if (moneyMsgCount <= 3) {
                try {
                    var gil = btlRewardAddr.add(0xCC).readU32();
                    var ctx = readBtlContext();
                    logBoth('[DrawMoneyMsg] gil=' + gil + ' ' + ctx);
                    logFile('[DrawMoneyMsg] call#' + moneyMsgCount);
                } catch(e) { logFile('[DrawMoneyMsg ERR] ' + e.message); }
            }
        }
    });
    logBoth('[OK] DrawMoneyMsg hooked');
} catch(e) { logBoth('[FAIL] DrawMoneyMsg: ' + e.message); }

// 3. TOBtlDrawGetItemMessageWindow (0x493480) — item display
var itemMsgCount = 0;
try {
    Interceptor.attach(rva(0x493480), {
        onEnter: function(args) {
            itemMsgCount++;
            if (itemMsgCount <= 5) {
                try {
                    var itemCnt = btlRewardAddr.add(0xD0).readU8();
                    var gearCnt = btlRewardAddr.add(0xD2).readU8();
                    var ctx = readBtlContext();
                    logBoth('[DrawItemMsg] items=' + itemCnt + ' gear=' + gearCnt + ' ' + ctx);
                    logFile('[DrawItemMsg] call#' + itemMsgCount);
                } catch(e) { logFile('[DrawItemMsg ERR] ' + e.message); }
            }
        }
    });
    logBoth('[OK] DrawItemMsg hooked');
} catch(e) { logBoth('[FAIL] DrawItemMsg: ' + e.message); }

// 4. TOBtlDrawLearningMessageWindow (0x495290) — sphere level-up
var learnMsgCount = 0;
try {
    Interceptor.attach(rva(0x495290), {
        onEnter: function(args) {
            learnMsgCount++;
            if (learnMsgCount <= 10) {
                var ctx = readBtlContext();
                logBoth('[DrawLearningMsg] ' + ctx + ' call#' + learnMsgCount);
            }
        }
    });
    logBoth('[OK] DrawLearningMsg hooked');
} catch(e) { logBoth('[FAIL] DrawLearningMsg: ' + e.message); }

// 5. TOBtlDrawStdChrNameMessageWindow (0x497170) — character name in results
var chrNameMsgCount = 0;
try {
    Interceptor.attach(rva(0x497170), {
        onEnter: function(args) {
            chrNameMsgCount++;
            if (chrNameMsgCount <= 10) {
                var ctx = readBtlContext();
                logBoth('[DrawChrNameMsg] ' + ctx + ' call#' + chrNameMsgCount);
            }
        }
    });
    logBoth('[OK] DrawChrNameMsg hooked');
} catch(e) { logBoth('[FAIL] DrawChrNameMsg: ' + e.message); }

// 6. TOBtlDrawCaptureMonsterMessageWindow (0x4927E0) — monster capture
var captureMsgCount = 0;
try {
    Interceptor.attach(rva(0x4927E0), {
        onEnter: function(args) {
            captureMsgCount++;
            if (captureMsgCount <= 5) {
                var ctx = readBtlContext();
                logBoth('[DrawCaptureMsg] ' + ctx + ' call#' + captureMsgCount);
            }
        }
    });
    logBoth('[OK] DrawCaptureMsg hooked');
} catch(e) { logBoth('[FAIL] DrawCaptureMsg: ' + e.message); }

// ── Hook: TODrawBtlWindow (0x49B2D0) — battle window renderer ──

var toDrawBtlWinCount = 0;
try {
    Interceptor.attach(rva(0x49B2D0), {
        onEnter: function(args) {
            toDrawBtlWinCount++;
            if (toDrawBtlWinCount <= 5) {
                var ecx = ptr(this.context.ecx);
                logFile('[TODrawBtlWindow] call #' + toDrawBtlWinCount +
                    ' ecx=0x' + ecx.toString(16) +
                    ' arg0=0x' + args[0].toString(16) +
                    ' arg1=0x' + args[1].toString(16));
            }
        }
    });
    logBoth('[OK] TODrawBtlWindow hooked');
} catch(e) { logBoth('[FAIL] TODrawBtlWindow: ' + e.message); }

// ── Main per-frame hook ──

var frameCount = 0;

try {
    Interceptor.attach(rva(0x471D10), {
        onLeave: function(retval) {
            frameCount++;

            // ════════════════════════════════════════
            // PART 1: Battle menu monitoring
            // ════════════════════════════════════════

            try {
                var btlState = btlStateAddr.readU8();

                if (btlState !== 0) {
                    // We're in battle
                    if (prevBtlState === 0) {
                        logBoth('[BTL] Battle started (state=' + btlState + ')');
                    }

                    var curWinIdx = btlCurWinIdxAddr.readU8();
                    var win = readBtlWindow(curWinIdx);

                    if (win) {
                        var typeName = BTL_WIN_TYPES[win.type] || ('0x' + win.type.toString(16));
                        var chrName = (win.chrId < CHARACTER_NAMES.length) ? CHARACTER_NAMES[win.chrId] : ('chr' + win.chrId);

                        // Detect window type/index change — new menu opened
                        var winChanged = (win.type !== prevBtlWinType || curWinIdx !== prevBtlWinIdx);
                        if (winChanged) {
                            btlWinAnnounced = false;
                            prevBtlSelIdx = -1; // reset cursor for new window
                            prevBtlSelCmd = -1;
                        }

                        // Only announce window ONCE when it reaches state 4 (fully open)
                        if (winChanged || (win.state === 4 && !btlWinAnnounced)) {
                            if (win.state === 4 && !btlWinAnnounced) {
                                btlWinAnnounced = true;
                                if (win.validSelAmt > 0) {
                                    logBoth('[BTL] ' + typeName + ', ' + chrName);

                                    // Dump command list to FILE only (no console spam)
                                    if (!win.cmdListPtr.isNull() && win.cmdListLen > 0) {
                                        logFile('[BTL CMD LIST] ' + win.cmdListLen + ' commands:');
                                        var max = Math.min(win.cmdListLen, 20);
                                        for (var c = 0; c < max; c++) {
                                            try {
                                                var cid = win.cmdListPtr.add(c * 2).readU16();
                                                var cname = resolveCommandText(cid, btlCommandBinAddr);
                                                if (!cname) cname = resolveCommandText(cid, btlItemBinAddr);
                                                logFile('  [' + c + '] id=0x' + cid.toString(16) + ' ' + (cname || '?'));
                                            } catch(e) {}
                                        }
                                    }

                                    // Announce initially focused item
                                    var cmdName = readCommandName(win);
                                    if (cmdName) logBoth('[BTL FOCUS] ' + cmdName);
                                }
                            }
                        }
                        prevBtlWinState = win.state;
                        prevBtlWinType = win.type;
                        prevBtlWinIdx = curWinIdx;

                        // ── Targeting detection ──
                        // When a command is selected, window goes to items=0, sel_chrs/sel_mons set
                        var winBaseT = btlWinArrayAddr.add(curWinIdx * BTL_WIN_SIZE);
                        var curChrs = winBaseT.add(0xD8).readU16();
                        var curMons = winBaseT.add(0xDA).readU16();
                        var nowTargeting = (win.validSelAmt === 0 && win.state !== 0 &&
                            (curChrs !== 0 || curMons !== 0));

                        if (nowTargeting && !inTargeting) {
                            inTargeting = true;
                            // Determine HP vs MP from the command that was just selected
                            targetShowHp = true;
                            targetShowMp = false;
                            if (lastNonZeroCmdId > 0) {
                                var preview = getCommandPartyPreview(lastNonZeroCmdId);
                                if (preview) {
                                    targetShowHp = preview.showHp;
                                    targetShowMp = preview.showMp;
                                    if (!targetShowHp && !targetShowMp) targetShowHp = true; // fallback
                                }
                                logFile('[TARGET] command=0x' + lastNonZeroCmdId.toString(16) +
                                    ' showHp=' + targetShowHp + ' showMp=' + targetShowMp);
                            }
                            logFile('[TARGET] Targeting mode entered. sel_chrs=0x' +
                                curChrs.toString(16) + ' sel_mons=0x' + curMons.toString(16));
                            // Dump all 8 monster slots to find which are active
                            try {
                                var mArrPtr = monsterChrsAddr.readPointer();
                                logFile('[TARGET MON SCAN] arrayPtr=0x' + mArrPtr.toString(16));
                                for (var mi = 0; mi < 8; mi++) {
                                    var mBase = mArrPtr.add(mi * CHR_SIZE);
                                    var mInBattle = mBase.add(CHR_IN_BATTLE).readU8();
                                    var mRam = mBase.add(CHR_RAM_OFFSET);
                                    var mName = decodeFFXSegments(mRam, 40, 0);
                                    var mHp = mRam.add(0x1A4).readS32();
                                    var mMaxHp = mRam.add(0x54).readS32();
                                    logFile('  [' + mi + '] in_battle=' + mInBattle +
                                        ' name=' + (mName.length > 0 ? mName[0] : '?') +
                                        ' hp=' + mHp + '/' + mMaxHp);
                                }
                            } catch(e) {
                                logFile('[TARGET MON SCAN ERROR] ' + e.message);
                            }
                        }

                        // Track target selection changes
                        if (inTargeting && nowTargeting) {
                            if (curChrs !== prevTargetChrs || curMons !== prevTargetMons) {
                                var selectedNames = [];

                                // sel_mons bitmask — bits 4+ map to monster array (bit 4 = index 0)
                                for (var bit = 0; bit < 8; bit++) {
                                    if (curMons & (1 << bit)) {
                                        var monIdx = (bit >= 4) ? bit - 4 : bit;
                                        var info = readMonsterInfo(monIdx);
                                        if (info) {
                                            selectedNames.push(info.name + ': ' + info.hpPct + '%');
                                            logFile('[TARGET MON] bit=' + bit + ' ' + info.name +
                                                ' HP=' + info.currentHp + '/' + info.maxHp);
                                        } else {
                                            selectedNames.push('Monster ' + bit);
                                        }
                                    }
                                }

                                // sel_chrs bitmask — party member names + HP/MP
                                for (var bit = 0; bit < 8; bit++) {
                                    if (curChrs & (1 << bit)) {
                                        var pInfo = readPlayerInfo(bit);
                                        if (pInfo) {
                                            var statStr = pInfo.name + ':';
                                            if (targetShowMp) {
                                                statStr += ' MP ' + pInfo.currentMp + '/' + pInfo.maxMp;
                                            } else {
                                                statStr += ' HP ' + pInfo.currentHp + '/' + pInfo.maxHp;
                                            }
                                            selectedNames.push(statStr);
                                        } else if (bit < CHARACTER_NAMES.length) {
                                            selectedNames.push(CHARACTER_NAMES[bit]);
                                        }
                                    }
                                }

                                if (selectedNames.length > 0) {
                                    logBoth('[TARGET] ' + selectedNames.join(', '));
                                }
                                logFile('[TARGET] sel_chrs=0x' + curChrs.toString(16) +
                                    ' sel_mons=0x' + curMons.toString(16));
                                prevTargetChrs = curChrs;
                                prevTargetMons = curMons;
                            }
                        }

                        if (!nowTargeting && inTargeting) {
                            inTargeting = false;
                            prevTargetChrs = 0;
                            prevTargetMons = 0;
                        }

                        // Detect cursor change (only when window has items)
                        if (win.state === 4 && win.validSelAmt > 0 &&
                            (win.selIdx !== prevBtlSelIdx || win.selCmdId !== prevBtlSelCmd)) {
                            var cmdName = readCommandName(win);
                            if (cmdName) {
                                logBoth('[BTL FOCUS] ' + cmdName);
                            } else if (win.selCmdId !== 0) {
                                logBoth('[BTL FOCUS] cmd:0x' + win.selCmdId.toString(16));
                            }
                            logFile('  selIdx=' + win.selIdx + ' selCmdId=0x' + win.selCmdId.toString(16) +
                                ' scroll=' + win.scrollPos + ' validAmt=' + win.validSelAmt);
                            prevBtlSelIdx = win.selIdx;
                            prevBtlSelCmd = win.selCmdId;
                            if (win.selCmdId !== 0) lastNonZeroCmdId = win.selCmdId;
                        }

                        // Scan all 8 slots once at battle start (file only)
                        if (prevBtlState === 0 && btlState !== 0) {
                            logFile('[BTL WIN SCAN] All window slots on battle start:');
                            for (var wi = 0; wi < 8; wi++) {
                                var w = readBtlWindow(wi);
                                if (w) {
                                    var tn = BTL_WIN_TYPES[w.type] || ('0x' + w.type.toString(16));
                                    logFile('  [' + wi + '] state=' + w.state + ' type=' + tn +
                                        ' chr=' + w.chrId + ' selIdx=' + w.selIdx +
                                        ' cmdLen=' + w.cmdListLen + ' validAmt=' + w.validSelAmt);
                                }
                            }
                        }
                    }

                    // ── Status effect tracking ──
                    var pArr2 = playerChrsAddr.readPointer();
                    if (!pArr2.isNull()) {
                        // One-time diagnostic dump — no in_battle filter, check key indices
                        if (!statusDiagDone) {
                            statusDiagDone = true;
                            logFile('[STATUS DIAG] pArr2=0x' + pArr2.toString(16));
                            // Check party (0-7) and likely monster slots (18-25)
                            var diagIndices = [0,1,2,3,4,5,6,7, 18,19,20,21,22,23,24,25];
                            for (var dii = 0; dii < diagIndices.length; dii++) {
                                var di = diagIndices[dii];
                                try {
                                    var dChr = pArr2.add(di * CHR_SIZE);
                                    var dInBattle = dChr.add(CHR_IN_BATTLE).readU8();
                                    var dPerm = dChr.add(0x606).readU16();
                                    var dExtra = dChr.add(0x616).readU16();
                                    var dDeath = dChr.add(0xDCC).readU8();
                                    var dNameSegs = decodeFFXSegments(dChr.add(CHR_RAM_OFFSET), 40, 0);
                                    var dName = (dNameSegs.length > 0) ? dNameSegs[0] : '(empty)';
                                    // Also check stat_visible at Chr+0x16 and chr_id at Chr+0x0E
                                    var dVisible = dChr.add(0x16).readU8();
                                    var dChrId = dChr.add(0x0E).readU16();
                                    logFile('  [' + di + '] name=' + dName + ' chrId=' + dChrId +
                                        ' inBattle=' + dInBattle + ' visible=' + dVisible +
                                        ' perm=0x' + dPerm.toString(16) + ' death=' + dDeath +
                                        ' extra=0x' + dExtra.toString(16));
                                } catch(e) {
                                    logFile('  [' + di + '] ERROR: ' + e.message);
                                }
                            }
                        }

                        for (var si = 0; si < 0x1f; si++) {
                            try {
                                var sChr = pArr2.add(si * CHR_SIZE);
                                var sInBattle = sChr.add(CHR_IN_BATTLE).readU8();

                                // Check death BEFORE skipping non-battle chars
                                // (game may clear inBattle when a character dies)
                                var sk = '' + si;
                                var deathNow = sChr.add(0xDCC).readU8();
                                var deathOld = (prevStatDeath[sk] !== undefined) ? prevStatDeath[sk] : deathNow;
                                if (deathNow !== 0 && deathOld === 0) {
                                    var dSegs = decodeFFXSegments(sChr.add(CHR_RAM_OFFSET), 40, 0);
                                    var dName = (dSegs.length > 0) ? dSegs[0] : ((si < CHARACTER_NAMES.length) ? CHARACTER_NAMES[si] : 'chr' + si);
                                    logBoth(dName + ': KO');
                                }
                                prevStatDeath[sk] = deathNow;

                                if (sInBattle === 0) continue;

                                var permNow = sChr.add(0x606).readU16();
                                var extraNow = sChr.add(0x616).readU16();
                                var sk = '' + si;

                                // Resolve name inline (no closure)
                                var sName = null;

                                // Check permanent status changes (skip bit 0 = KO, handled by stat_death above)
                                var permOld = (prevStatusPerm[sk] !== undefined) ? prevStatusPerm[sk] : permNow;
                                var permDiff = permNow ^ permOld;
                                for (var bit = 1; bit < PERM_STATUS_NAMES.length; bit++) {
                                    if (permDiff & (1 << bit)) {
                                        if (!sName) { var ns = decodeFFXSegments(sChr.add(CHR_RAM_OFFSET), 40, 0); sName = (ns.length > 0) ? ns[0] : ((si < CHARACTER_NAMES.length) ? CHARACTER_NAMES[si] : 'chr' + si); }
                                        var on = (permNow & (1 << bit)) !== 0;
                                        logBoth(sName + ': ' + PERM_STATUS_NAMES[bit] + (on ? '' : ' cured'));
                                    }
                                }
                                prevStatusPerm[sk] = permNow;

                                // Check temporal statuses (13 bytes at Chr+0x608)
                                var tempOld = prevStatusTemp[sk] || null;
                                var tempArr = [];
                                for (var ti = 0; ti < TEMP_STATUS_NAMES.length; ti++) {
                                    var tNow = sChr.add(0x608 + ti).readU8();
                                    tempArr.push(tNow);
                                    var tOld = tempOld ? tempOld[ti] : tNow;
                                    if (tNow > 0 && tOld === 0) {
                                        if (!sName) { var ns2 = decodeFFXSegments(sChr.add(CHR_RAM_OFFSET), 40, 0); sName = (ns2.length > 0) ? ns2[0] : ((si < CHARACTER_NAMES.length) ? CHARACTER_NAMES[si] : 'chr' + si); }
                                        logBoth(sName + ': ' + TEMP_STATUS_NAMES[ti]);
                                    } else if (tNow === 0 && tOld > 0) {
                                        if (!sName) { var ns3 = decodeFFXSegments(sChr.add(CHR_RAM_OFFSET), 40, 0); sName = (ns3.length > 0) ? ns3[0] : ((si < CHARACTER_NAMES.length) ? CHARACTER_NAMES[si] : 'chr' + si); }
                                        logBoth(sName + ': ' + TEMP_STATUS_NAMES[ti] + ' cured');
                                    }
                                }
                                prevStatusTemp[sk] = tempArr;

                                // Check extra status changes
                                var extraOld = (prevStatusExtra[sk] !== undefined) ? prevStatusExtra[sk] : extraNow;
                                var extraDiff = extraNow ^ extraOld;
                                for (var bit2 = 0; bit2 < EXTRA_STATUS_NAMES.length; bit2++) {
                                    if ((extraDiff & (1 << bit2)) && EXTRA_STATUS_NAMES[bit2] !== '') {
                                        if (!sName) { var ns4 = decodeFFXSegments(sChr.add(CHR_RAM_OFFSET), 40, 0); sName = (ns4.length > 0) ? ns4[0] : ((si < CHARACTER_NAMES.length) ? CHARACTER_NAMES[si] : 'chr' + si); }
                                        var on2 = (extraNow & (1 << bit2)) !== 0;
                                        logBoth(sName + ': ' + EXTRA_STATUS_NAMES[bit2] + (on2 ? '' : ' cured'));
                                    }
                                }
                                prevStatusExtra[sk] = extraNow;
                            } catch(e) {
                                logFile('[STATUS ERR] si=' + si + ' ' + e.message);
                            }
                        }
                    }

                    prevBtlState = btlState;
                } else {
                    if (prevBtlState !== 0) {
                        logBoth('[BTL] Battle ended');
                        rewardsAnnounced = false; // reset for next battle
                        moneyMsgCount = 0;
                        itemMsgCount = 0;
                        learnMsgCount = 0;
                        chrNameMsgCount = 0;
                        captureMsgCount = 0;
                        prevBtlState = 0;
                        prevBtlWinState = 0;
                        prevBtlWinType = -1;
                        prevBtlSelIdx = -1;
                        prevBtlSelCmd = -1;
                        prevBtlWinIdx = -1;
                        btlWinAnnounced = false;
                        inTargeting = false;
                        lastNonZeroCmdId = 0;
                        prevStatusPerm = {};
                        prevStatusTemp = {};
                        prevStatusExtra = {};
                        prevStatDeath = {};
                        battleResultsAnnounced = {};
                        statusDiagDone = false;
                        excelHeaderDumped = false;
                    }
                }
            } catch(e) {
                // Battle monitoring error — log once per 1000 frames
                if (frameCount % 1000 === 0) logFile('[BTL ERROR] ' + e.message);
            }

            // ════════════════════════════════════════
            // PART 2: Field menu monitoring (all menus)
            // ════════════════════════════════════════
            // Main menu: detected via MsMenuGetText(9,...), cursor at fieldMenuCursorAddr.
            // MsMenuGetText text format: name | - | description | - | next name | ...

            try {
                // Detect main menu open/close via MsMenuGetText(9,...) activity
                var wasInMainMenu = inMainMenu;
                inMainMenu = (frameCount - mainMenuLastFrame) < 10;

                if (inMainMenu && !wasInMainMenu) {
                    logBoth('[MENU] Main menu opened');
                    prevFieldCursor = -1;
                    prevFieldPlayer = -1;
                }
                if (!inMainMenu && wasInMainMenu) {
                    logBoth('[MENU] Main menu closed');
                    activeSubScene = -1;
                    prevSubScene = -1;
                }

                // ── Main menu cursor tracking (DAT_01871508) ──
                if (inMainMenu) {
                    var fieldCursor = fieldMenuCursorAddr.readS32();
                    var fieldPlayer = fieldMenuPlayerAddr.readS32();

                    if (fieldCursor !== prevFieldCursor) {
                        var logicalIdx = fieldCursor;
                        try {
                            logicalIdx = fieldMenuItemTable.add(fieldCursor * 8).readS32();
                        } catch(e) {}
                        var itemName = msMenuTextCache[logicalIdx] || ('item ' + logicalIdx);
                        logBoth('[FIELD MENU] ' + itemName);
                        logFile('[FIELD CURSOR] visual=' + fieldCursor + ' logical=' + logicalIdx + ' player=' + fieldPlayer);
                        prevFieldCursor = fieldCursor;
                    }
                    if (fieldPlayer !== prevFieldPlayer && prevFieldPlayer !== -1) {
                        var playerName = (fieldPlayer >= 0 && fieldPlayer < CHARACTER_NAMES.length)
                            ? CHARACTER_NAMES[fieldPlayer] : ('player ' + fieldPlayer);
                        logBoth('[FIELD PLAYER] ' + playerName);
                        logFile('[FIELD PLAYER] idx=' + fieldPlayer);
                        prevFieldPlayer = fieldPlayer;
                    } else {
                        prevFieldPlayer = fieldPlayer;
                    }
                }

            } catch(e) {
                if (frameCount % 1000 === 0) logFile('[MENU ERROR] ' + e.message);
            }

            // ════════════════════════════════════════
            // PART 2b: TkWindow monitoring (Items, popups, title screen)
            // ════════════════════════════════════════

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
                                logBoth('[MENU OPEN] Title Screen');
                                logBoth('[FOCUS] ' + (ctx.idx < TITLE_ITEMS.length ? TITLE_ITEMS[ctx.idx] : '?'));
                            } else {
                                var segs = getTkSegments(ctx);
                                logFile('[WIN' + w + ' OPEN] ' + JSON.stringify(segs));
                                if (segs.length > 0) logBoth('[MENU OPEN] ' + cleanText(segs[0]));
                                if (ctx.idx + 1 < segs.length) logBoth('[FOCUS] ' + cleanText(segs[ctx.idx + 1]));
                            }
                            prevTkCursors[key] = ctx.idx;
                        }
                        // Popup open (text pointer shifted)
                        else if (tpChanged) {
                            var segs = getTkSegments(ctx);
                            logFile('[WIN' + w + ' POPUP] ' + JSON.stringify(segs));
                            if (segs.length > 0) {
                                var numInstr = segs.length - ctx.items;
                                if (numInstr < 0) numInstr = 0;
                                var instr = '';
                                for (var si = 0; si < numInstr; si++) {
                                    var cl = cleanText(segs[si]);
                                    if (cl.length > 1) instr += (instr.length > 0 ? ' ' : '') + cl;
                                }
                                if (instr.length > 0) logBoth('[MENU OPEN] ' + instr);
                                logBoth('[FOCUS] ' + getTkFocusedItem(ctx, segs));
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
                                var segs = getTkSegments(ctx);
                                logFile('[WIN' + w + ' cursor] ' + JSON.stringify(segs));
                                if (isTkPopup[key]) {
                                    focusedText = getTkFocusedItem(ctx, segs);
                                } else {
                                    focusedText = (ctx.idx + 1 < segs.length) ? cleanText(segs[ctx.idx + 1]) : '?';
                                }
                            }
                            logBoth('[FOCUS] ' + focusedText);
                            prevTkCursors[key] = ctx.idx;
                        }
                    } catch(e) {}
                }
            } catch(e) {
                if (frameCount % 1000 === 0) logFile('[TKWIN ERROR] ' + e.message);
            }

            // ════════════════════════════════════════
            // PART 3: Sphere Grid monitoring
            // ════════════════════════════════════════

            try {
                var sgridOpen = false;
                try { sgridOpen = (sgridIsOpenAddr.readU8() !== 0); } catch(e) {}

                if (sgridOpen && !prevSgridOpen) {
                    logBoth('[SGRID] Sphere Grid opened');
                    prevSgridSelectedNode = -1;
                    prevSgridChrId = -1;
                    prevSgridMoving = false;
                    prevSgridNodeChangeText = '';
                    prevSgridActivatedText = '';
                    prevSgridChrPositions = {};
                }
                if (!sgridOpen && prevSgridOpen) {
                    logBoth('[SGRID] Sphere Grid closed');
                }
                prevSgridOpen = sgridOpen;

                if (sgridOpen) {
                    var eng = sgridEnginePtrAddr.readPointer();
                    if (!eng.isNull()) {
                        // ─── Character switch ───
                        var curChr = eng.add(0x115BC).readU8();
                        if (curChr !== prevSgridChrId) {
                            var chrName = (curChr < CHARACTER_NAMES.length) ? CHARACTER_NAMES[curChr] : ('chr' + curChr);
                            var slv = readSgridSlv(curChr);
                            var slvStr = slv ? (', S.Lv ' + slv.available + ' available, ' + slv.spent + ' spent') : '';
                            logBoth('[SGRID] ' + chrName + slvStr);
                            prevSgridChrId = curChr;
                        }

                        // ─── Selected node change ───
                        var selNode = eng.add(0x112FC).readS16();
                        if (selNode !== prevSgridSelectedNode && selNode >= 0) {
                            var nodeInfo = readSgridNode(eng, selNode);
                            if (nodeInfo) {
                                var activStr = '';
                                if (nodeInfo.activatedBy < 7) {
                                    activStr = ', ' + CHARACTER_NAMES[nodeInfo.activatedBy];
                                } else if (nodeInfo.type !== 0xFF && nodeInfo.type !== 0x01) {
                                    activStr = ', not activated';
                                }
                                // Use table name for stat/lock nodes, fall through to game text for abilities
                                var displayName = nodeInfo.typeName || ('node 0x' + nodeInfo.type.toString(16));
                                logBoth('[SGRID] ' + displayName + activStr);
                                logFile('[SGRID NODE] idx=' + selNode + ' type=0x' + nodeInfo.type.toString(16) +
                                    ' pos=(' + nodeInfo.x + ',' + nodeInfo.y + ')' +
                                    ' links=' + nodeInfo.linkCount +
                                    ' props=0x' + nodeInfo.props.toString(16) +
                                    ' activBy=' + nodeInfo.activatedBy);
                            }
                            prevSgridSelectedNode = selNode;
                        }

                        // ─── Node change text (game's description) ───
                        try {
                            var nctPtr = eng.add(0x11654).readPointer();
                            if (!nctPtr.isNull()) {
                                var nctText = decodeFFXString(nctPtr, 128);
                                if (nctText.length > 0 && nctText !== prevSgridNodeChangeText) {
                                    logBoth('[SGRID INFO] ' + nctText);
                                    prevSgridNodeChangeText = nctText;
                                }
                            }
                        } catch(e) {}

                        // ─── Activated node name ───
                        try {
                            var actPtr = eng.add(0x1165C).readPointer();
                            if (!actPtr.isNull()) {
                                var actText = decodeFFXString(actPtr, 64);
                                if (actText.length > 0 && actText !== prevSgridActivatedText) {
                                    logBoth('[SGRID ACTIVATED] ' + actText);
                                    prevSgridActivatedText = actText;
                                }
                            }
                        } catch(e) {}

                        // ─── Movement tracking ───
                        var moveProg = eng.add(0x11620).readFloat();
                        var nowMoving = (moveProg > 0.001);
                        if (nowMoving && !prevSgridMoving) {
                            var moveFrom = eng.add(0x11630).readS16();
                            var moveTo = eng.add(0x11634).readS16();
                            logBoth('[SGRID] Moving');
                            logFile('[SGRID MOVE] from=' + moveFrom + ' to=' + moveTo +
                                ' progress=' + moveProg.toFixed(2));
                        }
                        if (prevSgridMoving && nowMoving) {
                            logFile('[SGRID MOVE] progress=' + moveProg.toFixed(2));
                        }
                        prevSgridMoving = nowMoving;

                        // ─── Character positions (file-only, every 60 frames) ───
                        if (frameCount % 60 === 0) {
                            for (var ci = 0; ci < 7; ci++) {
                                try {
                                    var chrNodeIdx = eng.add(0x11088 + ci * 0x50 + 0x44).readS16();
                                    var pk = '' + ci;
                                    if (chrNodeIdx !== (prevSgridChrPositions[pk] || -1)) {
                                        prevSgridChrPositions[pk] = chrNodeIdx;
                                        logFile('[SGRID POS] ' + CHARACTER_NAMES[ci] +
                                            ' at node ' + chrNodeIdx);
                                    }
                                } catch(e) {}
                            }
                        }
                    }
                }
            } catch(e) {
                if (frameCount % 1000 === 0) logFile('[SGRID ERROR] ' + e.message);
            }
        }
    });
    logBoth('[OK] Per-frame monitor attached');
} catch(e) { logBoth('[FAIL] Per-frame hook: ' + e.message); }

// ── Hotkey handler (messages from Python host) ──
// T = whose turn, H = health check, N = item/command description

function handleHotkey(key) {
    var btlState = btlStateAddr.readU8();

    if (key === 't') {
        if (btlState === 0) { logBoth('[KEY t] Not in battle'); return; }
        // Whose turn — read current BtlWindow's character
        var curWinIdx = btlCurWinIdxAddr.readU8();
        var win = readBtlWindow(curWinIdx);
        if (win && win.state !== 0) {
            var chrName = (win.chrId < CHARACTER_NAMES.length) ? CHARACTER_NAMES[win.chrId] : ('chr' + win.chrId);
            logBoth('[TURN] ' + chrName);
        } else {
            logBoth('[TURN] No active turn');
        }
    }
    else if (key === 'h') {
        if (btlState === 0) { logBoth('[KEY h] Not in battle'); return; }
        // Health check — read active character's HP and MP
        var curWinIdx = btlCurWinIdxAddr.readU8();
        var win = readBtlWindow(curWinIdx);
        if (win && win.state !== 0) {
            var pInfo = readPlayerInfo(win.chrId);
            if (pInfo) {
                logBoth('[HEALTH] ' + pInfo.name + ': HP ' + pInfo.currentHp + '/' + pInfo.maxHp +
                    ' MP ' + pInfo.currentMp + '/' + pInfo.maxMp);
            }
        }
    }
    else if (key === 'n') {
        var foundDesc = false;

        // Source 0: Config menu description (when in Config sub-menu)
        if (activeSubScene === 0x0A) {
            try {
                var cfgCursor = configCursorAddr.readU8();
                var cfgItem = readConfigItem(cfgCursor);
                if (cfgItem) {
                    var cfgSpecial = configSpecialIdx.readS32();
                    var cfgDesc = null;
                    if (cfgCursor === cfgSpecial) {
                        var musicDescIds = [0x1CF, 0x1D1];
                        if (cfgItem.curSelection >= 0 && cfgItem.curSelection < musicDescIds.length) {
                            cfgDesc = resolveLocString(musicDescIds[cfgItem.curSelection]);
                        }
                    } else {
                        var cfgOptId = cfgItem.ptr.add(0x1C + cfgItem.curSelection * 4).readU32();
                        cfgDesc = resolveConfigDesc(cfgOptId);
                        if (!cfgDesc) cfgDesc = resolveConfigDesc(cfgItem.textId);
                    }
                    if (cfgDesc) {
                        logBoth('[DESC] ' + cfgDesc);
                        foundDesc = true;
                    }
                }
            } catch(e) {}
        }

        // Source 1: MsMenuGetText cached description — field menus
        if (!foundDesc) {
            try {
                var fieldCursor = fieldMenuCursorAddr.readS32();
                var logicalIdx = fieldCursor;
                try { logicalIdx = fieldMenuItemTable.add(fieldCursor * 8).readS32(); } catch(e) {}
                if (msMenuDescCache[logicalIdx]) {
                    logBoth('[DESC] ' + msMenuDescCache[logicalIdx]);
                    foundDesc = true;
                }
            } catch(e) {}
        }

        // Source 2: Battle HELP buffer (0xF3F4F8) — only populated during battle
        if (!foundDesc) {
            try {
                var helpText = decodeFFXString(helpTextAddr, 128);
                if (helpText.length > 0) {
                    logBoth('[DESC] ' + helpText);
                    foundDesc = true;
                }
            } catch(e) {}
        }

        // Source 3: Sphere Grid node_change_text (known issue: not populated during browsing)
        if (!foundDesc) {
            try {
                if (sgridIsOpenAddr.readU8() !== 0) {
                    var eng = sgridEnginePtrAddr.readPointer();
                    if (!eng.isNull()) {
                        var nctPtr = eng.add(0x11654).readPointer();
                        if (!nctPtr.isNull()) {
                            var nctText = decodeFFXString(nctPtr, 128);
                            if (nctText.length > 0) {
                                logBoth('[DESC] ' + nctText);
                                foundDesc = true;
                            }
                        }
                    }
                }
            } catch(e) {}
        }

        if (!foundDesc) {
            logBoth('[DESC] No description available');
        }
    }
    else if (key === 's') {
        // Sphere Grid status dump
        var sgOpen = false;
        try { sgOpen = (sgridIsOpenAddr.readU8() !== 0); } catch(e) {}
        if (!sgOpen) { logBoth('[KEY s] Sphere Grid not open'); return; }

        try {
            var eng = sgridEnginePtrAddr.readPointer();
            if (eng.isNull()) { logBoth('[KEY s] Engine not loaded'); return; }

            var curChr = eng.add(0x115BC).readU8();
            var chrName = (curChr < CHARACTER_NAMES.length) ? CHARACTER_NAMES[curChr] : ('chr' + curChr);
            var slv = readSgridSlv(curChr);
            logBoth('[SGRID STATUS] ' + chrName + ', S.Lv ' + (slv ? slv.available : '?') +
                ' available, ' + (slv ? slv.spent : '?') + ' spent');

            var selIdx = eng.add(0x112FC).readS16();
            if (selIdx >= 0) {
                var node = readSgridNode(eng, selIdx);
                if (node) {
                    var actStr = (node.activatedBy < 7) ? CHARACTER_NAMES[node.activatedBy] : 'none';
                    logBoth('[SGRID NODE] ' + (node.typeName || ('0x' + node.type.toString(16))) +
                        ', activated: ' + actStr +
                        ', pos (' + node.x + ',' + node.y + '), ' + node.linkCount + ' links');
                }
            }

            var tilt = eng.add(0x115CB).readU8();
            var zoom = eng.add(0x115CC).readU8();
            var tiltNames = ['flat','slight','far'];
            var zoomNames = ['close','medium','far','very far'];
            logFile('[SGRID VIEW] tilt=' + (tiltNames[tilt] || tilt) +
                ' zoom=' + (zoomNames[zoom] || zoom));

            for (var pi = 0; pi < 7; pi++) {
                var pNodeIdx = eng.add(0x11088 + pi * 0x50 + 0x44).readS16();
                var pSlv = readSgridSlv(pi);
                logBoth('[SGRID] ' + CHARACTER_NAMES[pi] + ' at node ' + pNodeIdx +
                    (pSlv ? (', S.Lv ' + pSlv.available) : ''));
            }
        } catch(e) {
            logBoth('[KEY s] Error: ' + e.message);
        }
    }
}

recv('hotkey', function onHotkey(msg) {
    try {
        handleHotkey(msg.key);
    } catch(e) {
        logFile('[HOTKEY ERROR] ' + e.message);
    }
    recv('hotkey', onHotkey);
});

logBoth('[*] Enter battle or press Triangle for field menu. Ctrl+C to stop.');
logBoth('[*] Hotkeys: T=turn, H=health, N=description, S=sphere grid');
