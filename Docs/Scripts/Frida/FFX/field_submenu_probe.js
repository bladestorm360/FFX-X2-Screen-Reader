// @desc Field submenu fn_ctrl probe (per-submenu cursor reads + Customize discovery)
// @auto
//
// v2: previous version snapshotted the 8-slot tkCurArrayAddr pool, but per
// gameArchitecture.md:1086-1094 these submenus do NOT use that pool — each
// has its own active-TkWindow global (e.g. DAT_0186a354 for Abilities) and
// reads cursor at TkWindow+0x48. v1 also missed Customize (RVA unknown).
//
// v2 hooks each known fn_ctrl (read decompile-confirmed cursor location)
// plus FUN_008aa0b0 (scene activate) which reads the scene table at
// DAT_01840848 and looks up scene_id->fn_ctrl_ptr at +0x04 of the scene
// struct (gameArchitecture.md:1054). When user opens Customize, that hook
// logs Customize's scene id AND its fn_ctrl pointer — both unknowns at
// once. After this run we'll know Customize's RVA.
//
// All RVAs from gameArchitecture.md and decompile (already RVAs — do NOT
// subtract 0x400000).

var base = Process.enumerateModules()[0].base;
function rva(off) { return base.add(off); }

var logPath = 'D:/Games/Dev/Custom/FFX/ffx-x2-screen-reader/Docs/Scripts/Frida/output/FFX/field_submenu_probe.log';
var logH = new File(logPath, 'w');
function logFile(m) { logH.write(m + '\n'); logH.flush(); }
function logBoth(m) { console.log(m); logFile(m); }

var SCENE_TABLE = rva(0x1440848); // DAT_01840848

// Per-submenu cursor read functions. Each returns a number (or NaN on
// invalid pointer / failed read). Reads at fn_ctrl onLeave (post-input
// dispatch) so the cursor is the value AFTER the user's button input
// has been processed for this frame.
function readU16Safe(addr) {
    try { return addr.readU16(); } catch (e) { return NaN; }
}
function readDerefU16At48(globalRva) {
    try {
        var p = rva(globalRva).readPointer();
        if (p.isNull()) return NaN;
        return p.add(0x48).readU16();
    } catch (e) { return NaN; }
}

var SUBMENUS = [
    {
        name: 'Help', rva: 0x4BEF20,
        readCursor: function() { return readU16Safe(rva(0x146A1F4)); }, // DAT_0186a1f4
        cursorDesc: 'DAT_0186a1f4'
    },
    {
        name: 'Abilities', rva: 0x4C6080,
        readCursor: function() { return readDerefU16At48(0x146A354); }, // DAT_0186a354 -> +0x48
        cursorDesc: '*DAT_0186a354+0x48'
    },
    {
        name: 'Aeons', rva: 0x4CC300,
        readCursor: function() {
            var tab = readDerefU16At48(0x1FCC128);   // DAT_023cc128
            var item = readDerefU16At48(0x146A560);  // DAT_0186a560
            return 'tab=' + tab + ' item=' + item;
        },
        cursorDesc: 'tab=*DAT_023cc128+0x48 item=*DAT_0186a560+0x48'
    },
    {
        name: 'Equip', rva: 0x4CEFF0,
        readCursor: function() {
            var c = readDerefU16At48(0x146A5DC);     // DAT_0186a5dc
            var tab = readU16Safe(rva(0x146A5E4));   // DAT_0186a5e4
            return 'cursor=' + c + ' tab=' + tab;
        },
        cursorDesc: '*DAT_0186a5dc+0x48 + DAT_0186a5e4 (tab)'
    },
    {
        name: 'Overdrive', rva: 0x4D0430,
        readCursor: function() {
            var s = readDerefU16At48(0x146A6D4);     // DAT_0186a6d4 (Set)
            var v = readDerefU16At48(0x146A6D8);     // DAT_0186a6d8 (View)
            return 'set=' + s + ' view=' + v;
        },
        cursorDesc: 'Set+View TkWindow cursors'
    },
    {
        name: 'Status', rva: 0x4D1DD0,
        readCursor: function() { return 'n/a (display-only)'; },
        cursorDesc: 'no cursor'
    },
    {
        name: 'Formation', rva: 0x4D5510,
        readCursor: function() {
            var a = readU16Safe(rva(0x147151C)); // DAT_0187151c
            var b = readU16Safe(rva(0x1471520)); // DAT_01871520
            return 'a=' + a + ' b=' + b;
        },
        cursorDesc: 'DAT_0187151c + DAT_01871520'
    }
];

var perMenuState = {};
SUBMENUS.forEach(function(m) {
    perMenuState[m.name] = { lastState: -1, lastCursor: '<init>', fired: false };
});

logBoth('[FIELD_SUBMENU_PROBE v2] hooks installing...');
logFile('--- field_submenu_probe v2 START base=' + base + ' ---');

SUBMENUS.forEach(function(m) {
    try {
        Interceptor.attach(rva(m.rva), {
            onEnter: function(args) {
                this.menu = m.name;
                this.param1 = args[0];
                try { this.stateBefore = this.param1.add(0x1C).readS32(); } catch (e) { this.stateBefore = -999; }
                this.cursorBefore = m.readCursor();
            },
            onLeave: function() {
                try {
                    var st = perMenuState[this.menu];
                    var stateAfter = -999;
                    try { stateAfter = this.param1.add(0x1C).readS32(); } catch (e) {}
                    var cursorAfter = m.readCursor();
                    var cursorStr = String(cursorAfter);

                    if (!st.fired) {
                        st.fired = true;
                        st.lastState = stateAfter;
                        st.lastCursor = cursorStr;
                        logBoth('[FIRST_FIRE] ' + this.menu +
                            ' rva=0x' + m.rva.toString(16) +
                            ' param1=' + this.param1 +
                            ' state=' + stateAfter +
                            ' cursor=' + cursorStr +
                            ' (' + m.cursorDesc + ')');
                        return;
                    }
                    if (stateAfter !== st.lastState) {
                        logFile('[STATE] ' + this.menu +
                            ' state=' + st.lastState + '->' + stateAfter +
                            ' cursor=' + cursorStr);
                        st.lastState = stateAfter;
                    }
                    if (cursorStr !== st.lastCursor) {
                        logBoth('[CURSOR] ' + this.menu +
                            ' state=' + stateAfter +
                            ' cursor=' + st.lastCursor + ' -> ' + cursorStr);
                        st.lastCursor = cursorStr;
                    }
                } catch (e) { logFile('[ERR] ' + this.menu + ': ' + e.message); }
            }
        });
        logFile('  hooked ' + m.name + ' @ rva=0x' + m.rva.toString(16) +
            ' (cursor: ' + m.cursorDesc + ')');
    } catch (e) {
        logBoth('[FAIL] ' + m.name + ': ' + e.message);
    }
});

// Scene activate (FUN_008aa0b0). Logs every scene activation with its
// fn_ctrl pointer pulled from the scene table. When user opens Items,
// Customize, or any other unhooked submenu, this captures its scene id
// AND fn_ctrl RVA in one event.
//
// Scene table layout (gameArchitecture.md:1045-1062):
//   DAT_01840848 = scene_table; each entry is a pointer to a scene
//   struct; struct+0x04 = fn_ctrl. The activate function is __cdecl with
//   scene_id as args[0].
//
// Known scene_id->name mapping (from agent research + production code).
// Anything NOT in this map is an unknown submenu we need to discover —
// notably Items (sceneId 0x00, claimed "covered by iterator path" in the
// arch doc but the user reports silent — that claim is wrong) and
// Customize (sceneId not yet identified).
// Updated per session 60 user evidence:
//   - sceneId 0x3 = Items (was UNKNOWN; user observed it as first event when
//     opening Items menu)
//   - sceneId 0x9 was labeled Formation but user reports Customize triggers
//     it — could be misidentified or shared. Marking as ambiguous until
//     re-verified.
var SCENE_NAMES = {
    0x01: 'FieldMenu',
    0x02: 'Abilities',
    0x03: 'Items',
    0x04: 'Equip',
    0x06: 'Status',
    0x07: 'Aeons',
    0x09: 'Formation/Customize?',
    0x0A: 'Config',
    0x0B: 'Help',
    0x15: 'Overdrive'
};
var KNOWN_FN_CTRL_RVAS = {
    0x4BEF20: 'Help',
    0x4C6080: 'Abilities',
    0x4CC300: 'Aeons',
    0x4CEFF0: 'Equip',
    0x4D0430: 'Overdrive',
    0x4D1DD0: 'Status',
    0x4D5510: 'Formation',
    0x4CB0B0: 'Config'
};
// v3: dump ALL 6 scene-struct offsets (gameArchitecture.md claims +0x00=
// fn_render and +0x04=fn_ctrl, but evidence shows the layout is reversed
// — Abilities fn_ctrl is 0x4C6080 but entry+0x04 yields 0x4C6470 which
// the cursor table identifies as fn_draw/fn_render). Logging all 6
// offsets so we can definitively correlate offset→role.
function readPtrAt(entry, off) {
    try {
        var p = entry.add(off).readPointer();
        if (p.isNull()) return -1;
        return p.sub(base).toInt32();
    } catch (e) { return -2; }
}
function fmtRva(r) {
    if (r === -1) return 'null';
    if (r === -2) return 'err';
    return '0x' + r.toString(16);
}
Interceptor.attach(rva(0x4AA0B0), {
    onEnter: function(args) {
        try {
            var sceneId = args[0].toInt32();
            var entryPtr = SCENE_TABLE.add(sceneId * 4).readPointer();
            var sceneName = SCENE_NAMES[sceneId] || 'UNKNOWN';
            if (entryPtr.isNull()) {
                logBoth('[ACTIVATE] sceneId=0x' + sceneId.toString(16) +
                    ' (' + sceneName + ') entry=NULL');
                return;
            }
            var off00 = readPtrAt(entryPtr, 0x00);
            var off04 = readPtrAt(entryPtr, 0x04);
            var off08 = readPtrAt(entryPtr, 0x08);
            var off0C = readPtrAt(entryPtr, 0x0C);
            var off10 = readPtrAt(entryPtr, 0x10);
            var off14 = readPtrAt(entryPtr, 0x14);
            // Mark known functions for cross-referencing with arch-doc cursor
            // table — when we see a known fn_ctrl RVA appear at e.g. +0x00,
            // we know +0x00 is fn_ctrl.
            var marker = function(rva) {
                var name = KNOWN_FN_CTRL_RVAS[rva];
                return name ? ' (=' + name + ' fn_ctrl)' : '';
            };
            var msg = '[ACTIVATE] sceneId=0x' + sceneId.toString(16) +
                ' (' + sceneName + ') entry=' + entryPtr +
                '\n    +0x00=' + fmtRva(off00) + marker(off00) +
                '\n    +0x04=' + fmtRva(off04) + marker(off04) +
                '\n    +0x08=' + fmtRva(off08) + marker(off08) +
                '\n    +0x0C=' + fmtRva(off0C) + marker(off0C) +
                '\n    +0x10=' + fmtRva(off10) + marker(off10) +
                '\n    +0x14=' + fmtRva(off14) + marker(off14);
            logBoth(msg);
        } catch (e) { logFile('[ACTIVATE_ERR] ' + e.message); }
    }
});

// Heartbeat on FUN_008ABCE0 (TODrawMessageWindow) — the production menus
// iterator anchor. Mod log shows zero [WIN N OPEN] entries since boot,
// suggesting either this never fires or it never sees an active slot.
// Counter confirms which.
var abce0Counter = 0;
var abce0LastReportedFrame = 0;
Interceptor.attach(rva(0x4ABCE0), {
    onLeave: function() {
        abce0Counter++;
        // Emit a heartbeat every 600 calls (~10s at 60fps if it fires per-frame)
        if (abce0Counter - abce0LastReportedFrame >= 600) {
            abce0LastReportedFrame = abce0Counter;
            logFile('[ABCE0_HEARTBEAT] count=' + abce0Counter);
        }
    }
});

logBoth('[FIELD_SUBMENU_PROBE v2] hooks installed. Open each submenu (incl. Customize) and arrow.');
logFile('--- hooks installed ---');
