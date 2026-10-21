// @desc Unified discovery probe: overdrive offsets, sphere grid text slots, dialogue resolver caller RAs.
//
// Three feature tracks in one probe run:
//
// 1. OVERDRIVE (verification) — confirm the decompile-sourced byte offsets
//    actor+0x5BC (max) / actor+0x5BD (current) match what the UI renders.
//    These came from reading FUN_00892880 (TOBtlDrawCrossLimitGauge) and
//    its callees FUN_00795560 / FUN_007955a0 in layer2_game_logic.txt.
//    F10 during battle dumps the raw bytes for all 7 party slots.
//
// 2. SPHERE GRID (discovery) — three real gaps per user clarification:
//    (a) basic stat nodes (Strength +1 etc.) ALREADY read from
//        NODE_TYPE_NAMES — this is fine.
//    (b) ABILITY nodes (Cure, Fire, etc.) do NOT read — their rawType
//        is outside NODE_TYPE_NAMES. Need the game's own lookup.
//    (c) Once the user presses Confirm on a node, a SUB-SCREEN appears
//        to pick which sphere to use. That whole sub-screen is silent.
//    Hooks:
//      - 0x648D70 node cursor move (browse) — dumps 40-byte node struct
//        + per-rawType 0x30-byte metadata @ eng+0xf834+rawType*0x30
//        + all four description-text slots + tries FUN_007AB890
//        against BOTH DAT_01A860E0 and DAT_01A860E4 as range-table base.
//      - 0x648E40 FUN_00A48E40 — fires when Confirm pressed on the
//        already-selected node (double-confirm = enter sphere-select
//        sub-screen). Writes menu-instance trigger at DAT_01A86108.
//      - 0x648D20 FUN_00A48D20 — fires on first Confirm on a node.
//      - 0x656330 FUN_00A56330 — sub-screen state handler (param_3 is
//        state code: 0 = open, 1 = confirmed).
//      - 0x656710 FUN_00A56710 — item→text resolver for sphere names
//        (uses DAT_01A860E4). Likely fires when the sub-screen lists
//        available sphere types.
//      - 0x65A640 activation text writer — same slot dump.
//      - 0x4B84C0 font-draw with args[0]==7 filter.
//    Plus frame-tick snapshot of eng+0x115A8 (the state machine handler
//    pointer) — changes to this signal state transitions.
//
// 3. DIALOGUE (discovery) — baseline/armed diff. Hooks:
//      - MsMenuGetText (0x38FD40), all arg0 values.
//      - getLocByStr (0x2E96D0).
//      - getLocById (0x2E96B0).
//    F8 toggles baseline (record what fires while idle in a field zone).
//    F9 arms capture. Talk to an NPC. F9 again or 30s timeout auto-dumps
//    the diff. Per session 38 pattern: caller RAs present in armed but
//    not baseline, with decoded text that matches the dialogue, identify
//    the production hook site.
//
// NO Stalker — per user direction, Stalker is only used after a specific
// function is identified, not as a discovery net.
// NO deduplication — repeated speech would be a production-hook problem,
// not a probe problem.
// NO polling, NO byte scanning, NO passive memory watchers.
//
// Hotkeys:
//   F7   reset all state.
//   F8   baseline toggle (dialogue mode).
//   F9   arm (dialogue mode) — auto-dumps on F9-again or 30s after first
//        in-armed text event.
//   F10  overdrive dump (works any time; reports "no battle" outside).
//   F11  S.Lv diagnostic — dumps raw bytes at saveData+0x55CC+chr*0x94
//        for all 7 party. Used to validate the production VK_S hotkey.
//
// Test protocol:
//   1. In battle, press F10 once. Expect `[OVR]` line with per-slot bytes.
//   2. Open sphere grid. Press F11 — expect [SLV] lines with per-char
//      numbers that match the in-game "S.Lv N" display top-right.
//      Then move cursor over:
//        - A basic stat node (Strength +1) — [SG CURSOR] + [META] +
//          [TEXT CURSOR] (if the description banner is readable).
//        - An ability-teach node ("Teaches the ability Flee") — same,
//          CRITICAL datapoint for ability name resolution.
//        - A lock node.
//      Then press Confirm on a basic stat node (first Confirm moves
//      cursor; Confirm again to open the Move/Use menu). Expect
//      [SG CONFIRM1] and [SG STATE_FN] transitions.
//      Press Confirm AGAIN to enter the Move/Use sub-screen
//      (screenshot 2). Down-arrow from Move to Use. Confirm on Use.
//      That opens the Use-sphere list (screenshot 3). Scroll through the
//      sphere types with Up/Down. Expect [SG MGT], [SG SPHERE_TX], and
//      [SG DRAW7] lines with decoded text including "Move", "Use",
//      "Power Sphere", sphere counts, etc.
//      Cancel out (don't activate) and close the grid.
//   3. Walk to an NPC in a quiet field. F8 baseline (5s). F8 again.
//      F9 arm. Talk. F9 again or 30s timeout. Expect [DLG NEW] +
//      [DLG CALLERS] lines.
//
// Output: Docs/Scripts/Frida/output/FFX/three_feature_probe.log
// Console: summaries only (state transitions, final dumps).

const LOG_PATH = 'D:/Games/Dev/Custom/FFX/ffx-x2-screen-reader/Docs/Scripts/Frida/output/FFX/three_feature_probe.log';
const logHandle = new File(LOG_PATH, 'w');
function logFile(msg) { try { logHandle.write(msg + '\n'); logHandle.flush(); } catch (e) {} }
function logBoth(msg) { console.log(msg); logFile(msg); }

// ── Module + base ──
let gameModule = null;
Process.enumerateModules().forEach(function (m) {
    if (m.name.toLowerCase() === 'ffx.exe') gameModule = m;
});
if (!gameModule) gameModule = Process.enumerateModules()[0];
const base = gameModule.base;
const moduleEnd = base.add(gameModule.size);
function rva(off) { return base.add(off); }
function rvaHex(p) {
    try { return '0x' + p.sub(base).toInt32().toString(16); }
    catch (e) { return '?'; }
}
logBoth('[*] three_feature_probe  module=' + gameModule.name +
        '  base=0x' + base.toString(16));

// ── RVAs ──
const RVA_BTL_STATE         = 0xD2A8E0;   // battle-active byte (Btl+0x10)
const RVA_PLAYER_CHRS       = 0xD334CC;   // playerChrsAddr — ptr to 0xF90 structs
const CHR_STRIDE            = 0xF90;
const CHR_IN_BATTLE         = 0xDC8;
const NUM_PARTY             = 7;
const CHAR_NAMES            = ['Tidus','Yuna','Auron','Kimahri','Wakka','Lulu','Rikku'];

// Overdrive offsets — from FUN_00795560 / FUN_007955a0 decompile bodies.
const OVR_MAX_OFF           = 0x5BC;
const OVR_CUR_OFF           = 0x5BD;

// Sphere grid
const RVA_SGRID_IS_OPEN     = 0x1685F70;
const RVA_SGRID_ENG_PTR     = 0x1F05834;
const RVA_SGRID_NODE_CURSOR = 0x648D70;   // FUN_00A48D70 (browse; already hooked in prod)
const RVA_SGRID_CONFIRM_1   = 0x648D20;   // FUN_00A48D20 (first Confirm on a node)
const RVA_SGRID_CONFIRM_2   = 0x648E40;   // FUN_00A48E40 (2nd Confirm = enter sub-screen)
const RVA_SGRID_SUBSCENE    = 0x656330;   // FUN_00A56330 (sub-screen state handler)
const RVA_SGRID_SPHERE_TEXT = 0x656710;   // FUN_00A56710 (sphere item→text resolver)
const RVA_SGRID_ACTIVATE    = 0x65A640;   // FUN_00A5A640 (activation text writer)
const RVA_SGRID_FONT_DRAW   = 0x4B84C0;   // FUN_008B84C0 (font-draw primitive; slot 7)
const RVA_ABMAP_RANGE_FN    = 0x3AB890;   // FUN_007AB890 range-lookup helper
// DAT_01A860E0 / E4 (Ghidra VAs) → RVAs = Ghidra - 0x400000.
const RVA_ABMAP_RANGE_BASE_A = 0x16860E0; // DAT_01A860E0: node-type range-table base
const RVA_ABMAP_RANGE_BASE_B = 0x16860E4; // DAT_01A860E4: sphere-item range-table base
const NODE_BASE_OFF         = 0x808;      // first node at eng+0x808
const NODE_STRIDE           = 0x28;
const NODE_RAWTYPE_OFF      = 0x6;
const NODE_ACTIVATED_OFF    = 0x21;
const NODE_PROPS_OFF        = 0x22;
const SGRID_SEL_NODE_OFF    = 0x112FC;
const SGRID_CHR_ID_OFF      = 0x115BC;
const SGRID_STATE_HANDLER   = 0x115A8;    // eng+0x115A8 = current state fn ptr
const SGRID_METATABLE_OFF   = 0xF834;     // eng+0xf834 + rawType*0x30 = metadata entry
const SGRID_METATABLE_STRIDE = 0x30;
const SGRID_TEXT_SLOTS      = [
    { off: 0x11654, name: 'node_change_text' },
    { off: 0x11658, name: 'text_slot_b'      },
    { off: 0x1165C, name: 'text_slot_c'      },
    { off: 0x11660, name: 'text_slot_d'      },
];

// Menu instance base (DAT_01A86108 @ Ghidra → RVA 0x1686108) — used by
// sphere-select sub-screen to install the scene-7 handler + config.
const RVA_MENU_INSTANCE     = 0x1686108;

// Save data (PlySave S.Lv diagnostic for production VK_S bug)
// RVA 0xD2CA90 = saveData base (per gameArchitecture + src/core/addresses.js).
// Production code assumes PlySave stride 0x94 starting at +0x55CC with
// available @+0x3B, spent @+0x3C. F11 dumps those raw bytes so we can
// validate the offsets against in-game S.Lv values.
const RVA_SAVE_DATA         = 0xD2CA90;
const PLY_SAVE_OFF          = 0x55CC;
const PLY_SAVE_STRIDE       = 0x94;
const PLY_SAVE_AVAIL_OFF    = 0x3B;
const PLY_SAVE_SPENT_OFF    = 0x3C;

// Dialogue
const RVA_MS_MENU_GET_TEXT  = 0x38FD40;
const RVA_GET_LOC_BY_STR    = 0x2E96D0;
const RVA_GET_LOC_BY_ID     = 0x2E96B0;

// Per-frame hook for hotkey dispatch (same RVA the production mod uses).
const RVA_FRAME_TICK        = 0x471D10;

// ── AOB sanity (prologue bytes) ──
function logPrologue(label, rvaOff, n) {
    try {
        const bytes = [];
        for (let i = 0; i < n; i++) {
            bytes.push(('0' + rva(rvaOff).add(i).readU8().toString(16)).slice(-2));
        }
        logFile('[AOB] ' + label + ' @0x' + rvaOff.toString(16) + ' bytes: ' + bytes.join(' '));
    } catch (e) {
        logBoth('[!] AOB read failed for ' + label + ': ' + e.message);
    }
}
logPrologue('MsMenuGetText ', RVA_MS_MENU_GET_TEXT, 16);
logPrologue('getLocByStr   ', RVA_GET_LOC_BY_STR,   16);
logPrologue('getLocById    ', RVA_GET_LOC_BY_ID,    16);
logPrologue('SgridCursor   ', RVA_SGRID_NODE_CURSOR,16);
logPrologue('SgridConfirm1 ', RVA_SGRID_CONFIRM_1,  16);
logPrologue('SgridConfirm2 ', RVA_SGRID_CONFIRM_2,  16);
logPrologue('SgridSubscene ', RVA_SGRID_SUBSCENE,   16);
logPrologue('SgridSphereTx ', RVA_SGRID_SPHERE_TEXT,16);
logPrologue('SgridActivate ', RVA_SGRID_ACTIVATE,   16);
logPrologue('SgridFontDraw ', RVA_SGRID_FONT_DRAW,  16);
logPrologue('AbmapRangeFn  ', RVA_ABMAP_RANGE_FN,   16);
logPrologue('FrameTick     ', RVA_FRAME_TICK,       16);

// ── FFX charset decoder (simplified for probe; mirrors src/core/charset.js) ──
const CHARSET = '0123456789 !"#$%&\'()*+,-./:;<=>?ABCDEFGHIJKLMNOPQRSTUVWXYZ[\\]^_`abcdefghijklmnopqrstuvwxyz{|}~';
function decodeFFX(addr, maxLen) {
    if (!addr || addr.isNull()) return '';
    if (maxLen <= 0 || maxLen > 512) maxLen = 128;
    let raw;
    try { raw = addr.readByteArray(maxLen); }
    catch (e) { return '<unreadable>'; }
    if (!raw) return '<null>';
    const bytes = new Uint8Array(raw);
    let result = '';
    let i = 0;
    while (i < maxLen) {
        let b = bytes[i];
        if (b === 0x00) break;
        if (b === 0x03 || b === 0x0A) { result += ' '; i++; continue; }
        if (b === 0x09 || b === 0x10 || b === 0x13) { i += 2; continue; }
        if (b < 0x2B) { i++; continue; }
        let idx = 0;
        if (b < 0x30) {
            idx = (b - 0x2B) * 0xD0;
            i++;
            if (i >= maxLen) break;
            b = bytes[i];
        }
        idx += (b - 0x30);
        if (idx >= 0 && idx < CHARSET.length) result += CHARSET[idx];
        i++;
    }
    return result.trim();
}

function isValidPtr(p) {
    if (!p || p.isNull()) return false;
    try { return p.compare(ptr(0x10000)) > 0 && p.compare(ptr(0x7FFFFFFF)) < 0; }
    catch (e) { return false; }
}
function inBattle() {
    try { return rva(RVA_BTL_STATE).readU8() !== 0; } catch (e) { return false; }
}
function sgridOpen() {
    try { return rva(RVA_SGRID_IS_OPEN).readU8() !== 0; } catch (e) { return false; }
}

// ── Keyboard (proven pattern) ──
let fnGetAsyncKeyState = null;
let fnGetForegroundWindow = null;
let fnGetWindowThreadProcessId = null;
const myPid = Process.id;
const pidBuf = Memory.alloc(4);
const keyWasDown = {};
Process.enumerateModules().forEach(function (m) {
    if (m.name.toLowerCase() === 'user32.dll') {
        m.enumerateExports().forEach(function (exp) {
            if (exp.name === 'GetAsyncKeyState') {
                fnGetAsyncKeyState = new NativeFunction(exp.address, 'int16', ['int']);
            }
            if (exp.name === 'GetForegroundWindow') {
                fnGetForegroundWindow = new NativeFunction(exp.address, 'pointer', []);
            }
            if (exp.name === 'GetWindowThreadProcessId') {
                fnGetWindowThreadProcessId = new NativeFunction(
                    exp.address, 'uint32', ['pointer', 'pointer']);
            }
        });
    }
});
function isGameFocused() {
    if (!fnGetForegroundWindow || !fnGetWindowThreadProcessId) return true;
    const hwnd = fnGetForegroundWindow();
    if (hwnd.isNull()) return false;
    fnGetWindowThreadProcessId(hwnd, pidBuf);
    return pidBuf.readU32() === myPid;
}
function keyPressed(vk) {
    if (!fnGetAsyncKeyState) return false;
    const down = (fnGetAsyncKeyState(vk) & 0x8000) !== 0;
    const was = keyWasDown[vk] || false;
    keyWasDown[vk] = down;
    if (!isGameFocused()) return false;
    return down && !was;
}
const VK_F7 = 0x76, VK_F8 = 0x77, VK_F9 = 0x78, VK_F10 = 0x79, VK_F11 = 0x7A;

// ── S.Lv verification (diagnosing the production VK_S hotkey bug) ──
// Dumps raw bytes at saveData + 0x55CC + chr*0x94 for each of the 7 party
// members. If the numbers match the in-game "S.Lv N" display (screenshot
// 1 top-right shows Tidus S.Lv 29 with 4 available and some spent), the
// offsets are correct and the production hotkey has a different bug. If
// garbage, the offsets need to change.
function dumpSlv() {
    try {
        const saveBase = rva(RVA_SAVE_DATA);
        const lines = [];
        for (let i = 0; i < NUM_PARTY; i++) {
            const ply = saveBase.add(PLY_SAVE_OFF + i * PLY_SAVE_STRIDE);
            let a = 0, s = 0;
            try { a = ply.add(PLY_SAVE_AVAIL_OFF).readU8(); } catch (e) {}
            try { s = ply.add(PLY_SAVE_SPENT_OFF).readU8(); } catch (e) {}
            // Also dump 8 bytes around the expected offsets for context.
            const ctx = [];
            for (let o = 0x38; o < 0x48; o++) {
                try { ctx.push(('0' + ply.add(o).readU8().toString(16)).slice(-2)); }
                catch (e) { ctx.push('??'); }
            }
            lines.push(CHAR_NAMES[i] + ' avail=' + a + ' spent=' + s +
                       ' (+0x38..+0x47: ' + ctx.join(' ') + ')');
        }
        logBoth('[SLV] ' + (sgridOpen() ? 'grid-open' : 'grid-closed'));
        for (const l of lines) logFile('[SLV] ' + l);
        // Console: just the visible ones.
        console.log('[SLV]   Tidus: ' + lines[0]);
    } catch (e) {
        logBoth('[SLV] err: ' + e.message);
    }
}

// ── Feature 1: OVERDRIVE verification ──
// Dump all in-battle party members' OVR bytes on F10 (and on startup if
// we happen to be in battle).
function dumpOverdrive() {
    if (!inBattle()) { logBoth('[OVR] no battle — skip'); return; }
    let pArr;
    try { pArr = rva(RVA_PLAYER_CHRS).readPointer(); } catch (e) { pArr = ptr(0); }
    if (!isValidPtr(pArr)) { logBoth('[OVR] playerChrsAddr invalid'); return; }
    const lines = [];
    for (let i = 0; i < NUM_PARTY; i++) {
        const chr = pArr.add(i * CHR_STRIDE);
        let inB = 0;
        try { inB = chr.add(CHR_IN_BATTLE).readU8(); } catch (e) { continue; }
        if (inB === 0) continue;
        let max = 0, cur = 0;
        try { max = chr.add(OVR_MAX_OFF).readU8(); } catch (e) {}
        try { cur = chr.add(OVR_CUR_OFF).readU8(); } catch (e) {}
        const pct = (max > 0) ? Math.round(cur * 100 / max) : 0;
        lines.push(CHAR_NAMES[i] + ' max=0x' + max.toString(16) +
                   ' cur=0x' + cur.toString(16) + ' (' + pct + '%)');
    }
    if (lines.length === 0) { logBoth('[OVR] no in-battle party slots'); return; }
    logBoth('[OVR] ' + lines.join(' | '));
}

// ── Feature 2: SPHERE GRID ──
// NativeFunction wrapper for the ExcelFile range-lookup helper. Signature
// inferred from the two call sites in layer4 (lines 6242, 6244):
//   FUN_007AB890(key: uint, excelBase: ptr, &outOffset: ptr) -> entryPtr
const fnAbmapRange = new NativeFunction(rva(RVA_ABMAP_RANGE_FN),
    'pointer', ['uint32', 'pointer', 'pointer']);
const abmapOutBuf = Memory.alloc(4);

function readSgridEng() {
    try {
        const p = rva(RVA_SGRID_ENG_PTR).readPointer();
        return isValidPtr(p) ? p : null;
    } catch (e) { return null; }
}

function readSelNodeIdx(eng) {
    try {
        const idx = eng.add(SGRID_SEL_NODE_OFF).readS32();
        if (idx < 0 || idx > 0x2000) return -1;
        return idx;
    } catch (e) { return -1; }
}

function dumpNodeStruct(eng, nodeIdx, label) {
    try {
        const nodeBase = eng.add(NODE_BASE_OFF + nodeIdx * NODE_STRIDE);
        const hexParts = [];
        for (let i = 0; i < NODE_STRIDE; i++) {
            hexParts.push(('0' + nodeBase.add(i).readU8().toString(16)).slice(-2));
        }
        const rawType = nodeBase.add(NODE_RAWTYPE_OFF).readU16();
        const activatedBy = nodeBase.add(NODE_ACTIVATED_OFF).readU8();
        const props = nodeBase.add(NODE_PROPS_OFF).readU8();
        logFile('[NODE ' + label + '] idx=' + nodeIdx +
                ' rawType=0x' + rawType.toString(16) +
                ' act=0x' + activatedBy.toString(16) +
                ' props=0x' + props.toString(16));
        logFile('         bytes: ' + hexParts.join(' '));
    } catch (e) { logFile('[NODE ' + label + '] dump err: ' + e.message); }
}

function dumpTextSlots(eng, label) {
    for (let i = 0; i < SGRID_TEXT_SLOTS.length; i++) {
        const slot = SGRID_TEXT_SLOTS[i];
        try {
            const raw = eng.add(slot.off).readU32();
            if (raw === 0) { continue; }
            const p = ptr(raw);
            const valid = isValidPtr(p);
            const decoded = valid ? decodeFFX(p, 128) : '';
            logFile('[TEXT ' + label + '] +0x' + slot.off.toString(16) + ' (' +
                    slot.name + ') = 0x' + raw.toString(16) +
                    (decoded ? ' "' + decoded + '"' : ''));
        } catch (e) {}
    }
}

function tryAbmapRangeLookupOne(key, baseRva, baseName, label) {
    try {
        const excelBase = rva(baseRva).readPointer();
        if (!isValidPtr(excelBase)) {
            logFile('[RANGE ' + label + '] ' + baseName + ' invalid');
            return;
        }
        abmapOutBuf.writeU32(0);
        const entry = fnAbmapRange(key, excelBase, abmapOutBuf);
        const outIdx = abmapOutBuf.readU32();
        let preview = '';
        if (isValidPtr(entry)) {
            // Entry layout uncertain; probe offsets 0 and 4 as ushort text
            // offsets relative to excelBase.
            try {
                const off0 = entry.readU16();
                const off4 = entry.add(4).readU16();
                const t0 = decodeFFX(excelBase.add(off0), 64);
                const t4 = decodeFFX(excelBase.add(off4), 64);
                preview = ' t0="' + t0 + '" t4="' + t4 + '"';
            } catch (e) {}
        }
        logFile('[RANGE ' + label + '/' + baseName + '] key=0x' + key.toString(16) +
                ' entry=' + (isValidPtr(entry) ? entry.toString() : 'null') +
                ' outIdx=' + outIdx + preview);
    } catch (e) {
        logFile('[RANGE ' + label + '/' + baseName + '] err: ' + e.message);
    }
}
function tryAbmapRangeLookup(rawType, label) {
    tryAbmapRangeLookupOne(rawType, RVA_ABMAP_RANGE_BASE_A, 'DAT_01A860E0', label);
    tryAbmapRangeLookupOne(rawType, RVA_ABMAP_RANGE_BASE_B, 'DAT_01A860E4', label);
}

// Dump the 0x30-byte metadata entry for a node's rawType @ eng+0xf834.
function dumpMetaEntry(eng, rawType, label) {
    try {
        const entry = eng.add(SGRID_METATABLE_OFF + rawType * SGRID_METATABLE_STRIDE);
        const bytes = [];
        for (let i = 0; i < SGRID_METATABLE_STRIDE; i++) {
            bytes.push(('0' + entry.add(i).readU8().toString(16)).slice(-2));
        }
        logFile('[META ' + label + '] rawType=0x' + rawType.toString(16) +
                ' +0xf834 entry: ' + bytes.join(' '));
        // Also try decoding any ushort fields as text offsets relative to
        // the two candidate excel bases.
        const candidates = [0, 2, 4, 6, 8, 0xA, 0xC, 0x10];
        for (const eBaseRva of [RVA_ABMAP_RANGE_BASE_A, RVA_ABMAP_RANGE_BASE_B]) {
            const eBase = rva(eBaseRva).readPointer();
            if (!isValidPtr(eBase)) continue;
            for (const off of candidates) {
                try {
                    const textOff = entry.add(off).readU16();
                    if (textOff < 8 || textOff > 0x8000) continue;
                    const t = decodeFFX(eBase.add(textOff), 64);
                    if (t && t.length > 1) {
                        const baseName = (eBaseRva === RVA_ABMAP_RANGE_BASE_A) ? 'E0' : 'E4';
                        logFile('[META ' + label + '] entry+0x' + off.toString(16) +
                                '=0x' + textOff.toString(16) + ' @' + baseName +
                                ' -> "' + t + '"');
                    }
                } catch (e) {}
            }
        }
    } catch (e) {}
}

// Dump the first few bytes of the sphere-select TkMenu scene instance at
// DAT_01A86108 — specifically the offsets that FUN_00A56330 writes
// (line 3146-3150 in layer4): +0x16E3 (byte), +0x16EA (ptr), +0x16EE (u32),
// +0x16F2 (byte), +0x16F4 (code ptr). Plus the confirm-trigger block
// FUN_00A48E40 touches (+0x16E3..+0x16E5, +0x2700..+0x2704).
function dumpMenuInstance(label) {
    try {
        const m = rva(RVA_MENU_INSTANCE);
        const hotOffsets = [0x16E3, 0x16E4, 0x16E5, 0x16EA, 0x16EE, 0x16F2, 0x16F4,
                            0x2700, 0x2702, 0x2704];
        const lines = [];
        for (const off of hotOffsets) {
            try {
                const byte = m.add(off).readU8();
                lines.push('+0x' + off.toString(16) + '=0x' + byte.toString(16));
            } catch (e) {}
        }
        logFile('[MENU ' + label + '] @DAT_01A86108 ' + lines.join(' '));
    } catch (e) {}
}

function sgridSnapshot(label) {
    const eng = readSgridEng();
    if (!eng) { logFile('[SG ' + label + '] no engine ptr'); return; }
    let selIdx = readSelNodeIdx(eng);
    let chrId = -1;
    try { chrId = eng.add(SGRID_CHR_ID_OFF).readU8(); } catch (e) {}
    let handler = 'null';
    try { handler = eng.add(SGRID_STATE_HANDLER).readPointer().toString(); } catch (e) {}
    logFile('[SG ' + label + '] selNode=' + selIdx + ' chrId=' + chrId +
            ' stateFn=' + handler);
    if (selIdx >= 0) {
        dumpNodeStruct(eng, selIdx, label);
        try {
            const rawType = eng.add(NODE_BASE_OFF + selIdx * NODE_STRIDE +
                                   NODE_RAWTYPE_OFF).readU16();
            tryAbmapRangeLookup(rawType, label);
            dumpMetaEntry(eng, rawType, label);
        } catch (e) {}
    }
    dumpTextSlots(eng, label);
    dumpMenuInstance(label);
}

// Hook: sphere-grid node cursor move. Fires on any cursor update; we
// snapshot everything.
Interceptor.attach(rva(RVA_SGRID_NODE_CURSOR), {
    onLeave: function () {
        if (!sgridOpen()) return;
        sgridSnapshot('CURSOR');
    }
});

// Hook: sphere-grid activation text writer.
Interceptor.attach(rva(RVA_SGRID_ACTIVATE), {
    onEnter: function (args) {
        this.param1 = args[0].toInt32();
        this.param2 = args[1].toInt32();
    },
    onLeave: function () {
        logFile('[SG ACTIVATE] param1=0x' + this.param1.toString(16) +
                ' param2=' + this.param2);
        sgridSnapshot('ACTIVATE');
    }
});

// Hook: font-draw primitive (FUN_008B84C0). Narrow filter on args[0]==7
// only — sphere grid's own draw slot per layer4:5309. Keeps log volume
// bounded. If the Move/Use or Use-sphere menus turn out to render through
// a different slot, we'll see that in the [SG MGT] log (MsMenuGetText
// captures all arg0 values during sphere grid) and extend in a follow-up.
Interceptor.attach(rva(RVA_SGRID_FONT_DRAW), {
    onEnter: function (args) {
        if (!sgridOpen()) return;
        const slot = args[0].toInt32();
        if (slot !== 7) return;
        const sub = args[1].toInt32();
        const tp = args[2];
        if (!isValidPtr(tp)) return;
        const decoded = decodeFFX(tp, 128);
        if (!decoded) return;
        logFile('[SG DRAW7] sub=' + sub + ' tp=' + tp + ' "' + decoded + '"');
    }
});

// Hook: first-Confirm on a node (FUN_00A48D20). 65-byte wrapper —
// calls FUN_00A48D70 then installs FUN_00A59E80 as state handler.
Interceptor.attach(rva(RVA_SGRID_CONFIRM_1), {
    onEnter: function (args) {
        this.nodeIdx = args[0].toInt32();
    },
    onLeave: function () {
        if (!sgridOpen()) return;
        logFile('[SG CONFIRM1] nodeIdx=' + this.nodeIdx);
        sgridSnapshot('CONFIRM1');
    }
});

// Hook: second-Confirm on already-selected node (FUN_00A48E40). This is
// the entry into the sphere-select sub-screen. Writes 0x301 to a menu
// instance + installs FUN_00A56330 as the scene handler.
Interceptor.attach(rva(RVA_SGRID_CONFIRM_2), {
    onEnter: function (args) {
        this.arg0 = args[0].toInt32();
    },
    onLeave: function () {
        if (!sgridOpen()) return;
        logFile('[SG CONFIRM2/SUBSCENE_ENTER] arg0=0x' + this.arg0.toString(16));
        sgridSnapshot('CONFIRM2');
    }
});

// Hook: sphere-select sub-screen state dispatcher (FUN_00A56330). Called
// with (param1, param2, state_code, flag). state_code: 0 = open,
// 1 = confirmed, other = pass-through. See layer4:3133.
Interceptor.attach(rva(RVA_SGRID_SUBSCENE), {
    onEnter: function (args) {
        this.p1 = args[0].toInt32();
        this.p2 = args[1].toInt32();
        this.p3 = args[2].toInt32();
        this.p4 = args[3].toInt32();
    },
    onLeave: function () {
        logFile('[SG SUBSCENE] p3=' + this.p3 + ' p4=' + this.p4 +
                ' p1=0x' + this.p1.toString(16) + ' p2=0x' + this.p2.toString(16));
        if (sgridOpen()) sgridSnapshot('SUBSCENE_' + this.p3);
    }
});

// Hook: sphere item→text resolver (FUN_00A56710). 51 bytes — wraps
// FUN_00790A40 (item entry getter, MsGetComData prefix 2) + FUN_007AB890
// against DAT_01A860E4. Fires when the sub-screen needs a sphere name.
Interceptor.attach(rva(RVA_SGRID_SPHERE_TEXT), {
    onEnter: function (args) {
        this.p1 = args[0].toInt32();
        this.p2 = args[1].toInt32();
    },
    onLeave: function (retval) {
        let decoded = '';
        try {
            if (isValidPtr(retval)) decoded = decodeFFX(retval, 128);
        } catch (e) {}
        // Also decode at retval + excelBase combinations (the actual text
        // may live at base + retval-offset like MsGetComData).
        let alt0 = '', alt4 = '';
        try {
            const e4Base = rva(RVA_ABMAP_RANGE_BASE_B).readPointer();
            if (isValidPtr(e4Base) && isValidPtr(retval)) {
                // retval may itself be a pointer into data already
                const maybeOff = retval.sub(e4Base).toInt32();
                if (maybeOff >= 0 && maybeOff < 0x80000) {
                    alt0 = '(offset ' + maybeOff + ' into E4)';
                }
            }
        } catch (e) {}
        logFile('[SG SPHERE_TX] p1=0x' + this.p1.toString(16) +
                ' p2=0x' + this.p2.toString(16) +
                ' ret=' + retval + ' "' + decoded + '" ' + alt0 + alt4);
    }
});

// ── Feature 3: DIALOGUE baseline/armed diff ──
let dlgState = 'idle';  // idle | baseline | armed
let dlgBaselineFrames = 0;
let dlgArmedStartFrame = 0;
let dlgArmedFirstEventFrame = -1;
const DLG_ARMED_TIMEOUT_FRAMES = 30 * 60;  // 30 seconds at 60fps

// Keys: source|callerRva|arg1hex  →  { count, sampleText }
const dlgBaseline = {};
const dlgArmed = {};

function dlgKey(source, callerRva, arg1) {
    return source + '|' + callerRva.toString(16) + '|' + arg1.toString(16);
}

function dlgRecord(source, callerRva, arg0, arg1, decoded) {
    const k = dlgKey(source, callerRva, arg1);
    const bucket = (dlgState === 'baseline') ? dlgBaseline
                 : (dlgState === 'armed')    ? dlgArmed
                 : null;
    if (!bucket) return;
    if (!bucket[k]) {
        bucket[k] = { count: 0, arg0: arg0, arg1: arg1, sampleText: decoded, callerRva: callerRva, source: source };
    }
    bucket[k].count++;
    // Update sample if the previous was empty
    if (!bucket[k].sampleText && decoded) bucket[k].sampleText = decoded;
    if (dlgState === 'armed') {
        if (dlgArmedFirstEventFrame < 0) dlgArmedFirstEventFrame = frameCount;
        // File-only verbose log during armed (every call).
        logFile('[DLG ARMED] src=' + source + ' caller=' + callerRva.toString(16) +
                ' arg0=' + arg0 + ' arg1=0x' + arg1.toString(16) +
                (decoded ? ' "' + decoded + '"' : ''));
    }
}

// MsMenuGetText hook — all arg0 values. Records into dialogue
// baseline/armed buckets AND, while the sphere grid is open, logs every
// call to file so we see the Move/Use and Use-sphere menus resolving
// their text.
Interceptor.attach(rva(RVA_MS_MENU_GET_TEXT), {
    onEnter: function (args) {
        this.arg0 = args[0].toInt32();
        this.arg1 = args[1].toInt32();
        this.callerRva = this.returnAddress.sub(base).toInt32();
    },
    onLeave: function (retval) {
        let decoded = '';
        if (isValidPtr(retval)) decoded = decodeFFX(retval, 128);
        if (sgridOpen()) {
            logFile('[SG MGT] caller=0x' + this.callerRva.toString(16) +
                    ' arg0=' + this.arg0 + ' arg1=0x' + this.arg1.toString(16) +
                    (decoded ? ' "' + decoded + '"' : ''));
        }
        if (dlgState === 'idle') return;
        dlgRecord('MGT', this.callerRva, this.arg0, this.arg1, decoded);
    }
});

// getLocByStr hook — arg[1] is struct ptr, string key at +8.
Interceptor.attach(rva(RVA_GET_LOC_BY_STR), {
    onEnter: function (args) {
        this.arg0 = args[0].toInt32();
        this.namePtr = ptr(0);
        this.nameSig = 0;
        try {
            if (isValidPtr(args[1])) {
                const p = args[1].add(8).readPointer();
                if (isValidPtr(p)) {
                    this.namePtr = p;
                    const s = p.readCString(32) || '';
                    // Hash a short name to arg1-like u32 so bucket key is stable.
                    let h = 0;
                    for (let i = 0; i < s.length; i++) h = ((h * 31) + s.charCodeAt(i)) & 0xFFFFFFFF;
                    this.nameSig = h >>> 0;
                    this.nameStr = s;
                }
            }
        } catch (e) {}
        this.callerRva = this.returnAddress.sub(base).toInt32();
    },
    onLeave: function () {
        if (dlgState === 'idle') return;
        dlgRecord('LBS', this.callerRva, 0, this.nameSig, this.nameStr || '');
    }
});

// getLocById hook — arg[1] is integer id in some variant. Capture both
// possibilities (arg0 or arg1).
Interceptor.attach(rva(RVA_GET_LOC_BY_ID), {
    onEnter: function (args) {
        this.arg0 = args[0].toInt32();
        this.arg1 = args[1].toInt32();
        this.callerRva = this.returnAddress.sub(base).toInt32();
    },
    onLeave: function (retval) {
        if (dlgState === 'idle') return;
        let decoded = '';
        if (isValidPtr(retval)) decoded = decodeFFX(retval, 128);
        dlgRecord('LBI', this.callerRva, this.arg0, this.arg1, decoded);
    }
});

// ── Commands ──
function cmdBaseline() {
    if (dlgState === 'armed') {
        logBoth('[!] armed — press F9 to dump first, or F7 to reset');
        return;
    }
    if (dlgState === 'idle') {
        for (const k in dlgBaseline) delete dlgBaseline[k];
        dlgState = 'baseline';
        dlgBaselineFrames = frameCount;
        logBoth('[DLG BASELINE STARTED] stay idle — F8 again to stop');
    } else { // dlgState === 'baseline'
        dlgState = 'idle';
        const frames = frameCount - dlgBaselineFrames;
        const keys = Object.keys(dlgBaseline).length;
        logBoth('[DLG BASELINE STOPPED] ' + frames + ' frames, ' + keys + ' unique triples');
    }
}

function cmdArm() {
    if (dlgState === 'baseline') { logBoth('[!] stop baseline first (F8)'); return; }
    if (dlgState === 'armed') { cmdDump(); return; }
    for (const k in dlgArmed) delete dlgArmed[k];
    dlgArmedStartFrame = frameCount;
    dlgArmedFirstEventFrame = -1;
    dlgState = 'armed';
    logBoth('[DLG ARMED] trigger dialogue — F9 again or 30s timeout to dump');
}

function cmdDump() {
    if (dlgState !== 'armed') { logBoth('[!] not armed'); return; }
    dlgState = 'idle';
    const frames = frameCount - dlgArmedStartFrame;
    const keys = Object.keys(dlgArmed);
    logBoth('[DLG DUMP] armed=' + frames + ' frames  entries=' + keys.length);
    // Diff: entries in armed not in baseline.
    const newEntries = [];
    for (const k of keys) {
        if (!dlgBaseline[k]) newEntries.push(k);
    }
    logBoth('[DLG DIFF] ' + newEntries.length + ' new (caller,arg1) triples');
    // Print top 10 on console; all on file.
    let consoleShown = 0;
    for (const k of newEntries) {
        const e = dlgArmed[k];
        const line = '[DLG NEW] ' + e.source + ' caller=0x' + e.callerRva.toString(16) +
                     ' arg0=' + e.arg0 + ' arg1=0x' + e.arg1.toString(16) +
                     ' count=' + e.count +
                     (e.sampleText ? ' sample="' + e.sampleText + '"' : '');
        logFile(line);
        if (consoleShown < 10) { console.log(line); consoleShown++; }
    }
    if (newEntries.length > consoleShown) {
        console.log('[DLG DIFF] ... (' + (newEntries.length - consoleShown) +
                    ' more in log file)');
    }
    // Also per-caller aggregation to console.
    const callerCounts = {};
    for (const k of newEntries) {
        const e = dlgArmed[k];
        const ck = e.source + '|0x' + e.callerRva.toString(16);
        callerCounts[ck] = (callerCounts[ck] || 0) + e.count;
    }
    const sorted = Object.keys(callerCounts).sort(function (a, b) {
        return callerCounts[b] - callerCounts[a];
    });
    logBoth('[DLG CALLERS] top callers by volume:');
    for (let i = 0; i < Math.min(sorted.length, 10); i++) {
        logBoth('  ' + sorted[i] + ' → ' + callerCounts[sorted[i]]);
    }
}

function cmdReset() {
    dlgState = 'idle';
    for (const k in dlgBaseline) delete dlgBaseline[k];
    for (const k in dlgArmed) delete dlgArmed[k];
    logBoth('[RESET]');
}

// ── Frame tick for hotkey dispatch + state monitor + auto-timeout ──
let frameCount = 0;
// State-handler change monitor. When the sphere grid is open, the handler
// pointer at eng+0x115A8 changes between states (browse → confirm → sub-
// screen → activate). Logging changes identifies state transitions
// without any extra hook.
let prevStateHandler = 0;
let prevSgridOpen = 0;
Interceptor.attach(rva(RVA_FRAME_TICK), {
    onLeave: function () {
        frameCount++;
        if (keyPressed(VK_F7))  cmdReset();
        if (keyPressed(VK_F8))  cmdBaseline();
        if (keyPressed(VK_F9))  cmdArm();
        if (keyPressed(VK_F10)) dumpOverdrive();
        if (keyPressed(VK_F11)) dumpSlv();
        // Auto-dump on 30s after first in-armed event (gives time for
        // dialogue to play out without forcing a second F9).
        if (dlgState === 'armed' && dlgArmedFirstEventFrame > 0 &&
            frameCount - dlgArmedFirstEventFrame >= DLG_ARMED_TIMEOUT_FRAMES) {
            logBoth('[DLG AUTO-DUMP] 30s since first event');
            cmdDump();
        }
        // Sphere grid state-handler change monitor.
        const openNow = sgridOpen() ? 1 : 0;
        if (openNow !== prevSgridOpen) {
            prevSgridOpen = openNow;
            prevStateHandler = 0;
            if (openNow) logFile('[SG OPENED] frame=' + frameCount);
            else         logFile('[SG CLOSED] frame=' + frameCount);
        }
        if (openNow) {
            try {
                const eng = rva(RVA_SGRID_ENG_PTR).readPointer();
                if (isValidPtr(eng)) {
                    const h = eng.add(SGRID_STATE_HANDLER).readU32();
                    if (h !== prevStateHandler) {
                        logFile('[SG STATE_FN] frame=' + frameCount +
                                ' 0x' + prevStateHandler.toString(16) +
                                ' -> 0x' + h.toString(16) +
                                ' (ghidra=0x' + (h + 0x400000 - base.toInt32()).toString(16) + ')');
                        prevStateHandler = h;
                    }
                }
            } catch (e) {}
        }
    }
});

logBoth('[*] probe armed. Hotkeys: F7 reset, F8 baseline toggle, F9 arm/dump, F10 overdrive, F11 S.Lv.');
logBoth('[*] overdrive offsets being tested: actor+0x' + OVR_MAX_OFF.toString(16) +
        ' (max), +0x' + OVR_CUR_OFF.toString(16) + ' (cur)');
logBoth('[*] sphere grid: cursor / confirm1 / confirm2 / subscene / sphere-text / activate / font-draw7 hooks active');
logBoth('[*] sphere grid: per-frame state-handler-ptr monitor active while grid open');
logBoth('[*] dialogue: F8 in a quiet field, then F9 and talk to an NPC');
