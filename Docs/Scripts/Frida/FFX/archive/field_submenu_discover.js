// @desc Field sub-menu discovery — maps scene IDs, TkWindow slots, and text sources for all 8 remaining menus
// @python
//
// Items already reads via tk_menu_probe.js (monitors 8 TkWindow pointer slots at 0x14676B0).
// The other 8 sub-menus (Abilities, Overdrive, Equip, Aeons, Customize, Config, Help, Formation)
// produce silence. This script discovers WHY by:
//   1. Hooking scene registration/activation to detect which scene ID maps to which menu
//   2. Scanning ALL 32 TkWindow pool slots when scenes activate
//   3. Hooking MsMenuGetText for text that bypasses TkWindow
//   4. Tracking the HELP text buffer for description text
//
// Run via run_frida.bat -> 1 (FFX) -> field_submenu_discover
// Open main menu (Triangle), select each sub-menu. Check log for results.

var logPath = 'D:/Games/Dev/Custom/FFX/ffx-x2-screen-reader/Docs/Scripts/Frida/output/FFX/field_submenu_discover.log';
var logHandle = new File(logPath, 'w');
function logFile(msg) { logHandle.write(msg + '\n'); logHandle.flush(); }
function logBoth(msg) { console.log(msg); logFile(msg); }

var base = Process.enumerateModules()[0].base;
function rva(offset) { return base.add(offset); }

logBoth('[*] Field sub-menu discovery script');

// ── Addresses (all Fahrenheit RVAs — use directly) ──

// Scene dispatch system (from Ghidra decompile of layer2_game_logic.txt)
var sceneTableAddr     = rva(0x1440848);  // DAT_01840848: 25 scene slots, 4 bytes each (ptrs to TkMenu structs)
var sceneRegisterRVA   = 0x4AAA60;        // FUN_008aaa60: registers a TkMenu struct at scene_id
var sceneActivateRVA   = 0x4AA0B0;        // FUN_008aa0b0: activates a scene (calls fn_0x0C + fn_ctrl)
var eventDispatchRVA   = 0x4AA730;        // FUN_008aa730: event mask -> scene dispatch
var sceneFocusRVA      = 0x4E1C60;        // FUN_008e1c60: sets scene visual focus

// TkWindow pool (Fahrenheit game_ui.cs)
var tkWindowPoolAddr   = rva(0x14408C0);  // 32 slots x 0x98 bytes each
var TKWIN_SIZE         = 0x98;
var TKWIN_SLOTS        = 32;

// Existing probe's pointer arrays (for comparison)
var windowArrayAddr    = rva(0x14676B0);  // 8 window pointers (what tk_menu_probe monitors)
var cursorArrayAddr    = rva(0x1468A50);  // 8 cursor pointers

// Menu text
var msMenuGetTextRVA   = 0x38FD40;        // MsMenuGetText(arg0, arg1, arg2)
var helpTextAddr       = rva(0xF3F4F8);   // HELP panel text buffer (128 bytes, FFX charset)

// Field menu cursor
var fieldMenuCursorAddr = rva(0x1471508); // visual cursor position
var fieldMenuPlayerAddr = rva(0x147150C); // selected player index
var fieldMenuItemTable  = rva(0x1471528); // visual->logical mapping table

// State
var isInTkMenuAddr     = rva(0xF407EC);   // IsInTkMenuFakeLoop
var eventIdAddr        = rva(0xEFBBF8);   // event_id
var inputAddr          = rva(0xF27080);   // input bitmask

// ── FFX charset decoder (from tk_menu_probe.js / btl_tkmenu_discover.js) ──

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

// ── TkWindow pool scanner ──

function scanTkWindowPool(label) {
    logFile('');
    logFile('=== TkWindow Pool Scan: ' + label + ' ===');
    var activeCount = 0;
    for (var i = 0; i < TKWIN_SLOTS; i++) {
        try {
            var winBase = tkWindowPoolAddr.add(i * TKWIN_SIZE);
            var isActive = winBase.add(0x40).readU8();
            if (isActive === 0) continue;

            activeCount++;
            var numItems = winBase.add(0x30).readS16();
            var selIdx = winBase.add(0x48).readS16();
            var menuGroup = winBase.add(0x36).readU8();
            var fnInit = winBase.add(0x08).readPointer();
            var fnState = winBase.add(0x0C).readPointer();
            var fnRender = winBase.add(0x10).readPointer();
            var textPtr = ptr(0);
            try { textPtr = winBase.add(0x5C).readPointer(); } catch(e) {}

            var textPreview = '';
            if (!textPtr.isNull() && textPtr.compare(ptr(0x10000)) > 0) {
                try {
                    var segs = decodeFFXSegments(textPtr, 256, 0);
                    if (segs.length > 0) textPreview = segs.slice(0, 4).join(' | ');
                } catch(e) {}
            }

            var line = '[POOL slot=' + i +
                ' active=1 items=' + numItems +
                ' sel=' + selIdx +
                ' group=' + menuGroup +
                ' fnInit=RVA 0x' + fnInit.sub(base).toString(16) +
                ' fnState=RVA 0x' + fnState.sub(base).toString(16) +
                ' fnRender=RVA 0x' + fnRender.sub(base).toString(16) +
                ' text=0x' + textPtr.toString(16) +
                ']';
            logFile(line);
            if (textPreview.length > 0) {
                logFile('  TEXT: ' + textPreview);
            }
        } catch(e) {}
    }
    logFile('[POOL] ' + activeCount + ' active TkWindow slots (' + label + ')');
    logFile('=== End Pool Scan ===');
    logFile('');
}

// Also compare what the existing probe monitors
function scanProbeArrays(label) {
    logFile('');
    logFile('=== Probe Array Scan (8 monitored slots): ' + label + ' ===');
    for (var w = 0; w < 8; w++) {
        try {
            var winPtr = windowArrayAddr.add(w * 4).readPointer();
            var curPtr = cursorArrayAddr.add(w * 4).readPointer();
            if (winPtr.isNull()) { logFile('[PROBE w=' + w + '] null'); continue; }

            var vis = 0;
            try { vis = curPtr.add(1).readU8(); } catch(e) {}
            var idx = 0;
            try { idx = curPtr.add(2).readU8(); } catch(e) {}
            var items = 0;
            try { items = curPtr.add(5).readU8(); } catch(e) {}
            var textPtr = ptr(0);
            try { textPtr = winPtr.add(0x5C).readPointer(); } catch(e) {}

            logFile('[PROBE w=' + w + ' vis=' + vis + ' idx=' + idx + ' items=' + items +
                ' winPtr=0x' + winPtr.toString(16) + ' textPtr=0x' + textPtr.toString(16) + ']');
        } catch(e) { logFile('[PROBE w=' + w + '] error: ' + e.message); }
    }
    logFile('=== End Probe Array Scan ===');
}

// ── Scene table dump ──

function dumpSceneTable(label) {
    logFile('');
    logFile('=== Scene Table: ' + label + ' ===');
    var activeScenes = 0;
    for (var sid = 0; sid <= 0x18; sid++) {
        try {
            var tkMenuPtr = sceneTableAddr.add(sid * 4).readPointer();
            if (tkMenuPtr.isNull()) continue;
            activeScenes++;

            var fnCtrl = tkMenuPtr.readPointer();
            var fnDraw = tkMenuPtr.add(4).readPointer();
            var fnInit = tkMenuPtr.add(8).readPointer();
            var fn0C   = tkMenuPtr.add(0x0C).readPointer();
            var fnDeact = tkMenuPtr.add(0x14).readPointer();
            var state  = tkMenuPtr.add(0x1C).readS32();

            logFile('[SCENE 0x' + sid.toString(16) +
                '] fnCtrl=RVA 0x' + fnCtrl.sub(base).toString(16) +
                ' fnDraw=RVA 0x' + fnDraw.sub(base).toString(16) +
                ' fnInit=RVA 0x' + fnInit.sub(base).toString(16) +
                ' fn0C=RVA 0x' + fn0C.sub(base).toString(16) +
                ' fnDeact=RVA 0x' + fnDeact.sub(base).toString(16) +
                ' state=' + state);
        } catch(e) {}
    }
    logFile('[SCENES] ' + activeScenes + ' active scenes (' + label + ')');
    logFile('=== End Scene Table ===');
}

// ── HELP text buffer reader ──

var prevHelpText = '';

function readHelpText() {
    try {
        var segs = decodeFFXSegments(helpTextAddr, 128, 0);
        return segs.join(' ');
    } catch(e) { return ''; }
}

function checkHelpTextChange() {
    var ht = readHelpText();
    if (ht !== prevHelpText && ht.length > 0) {
        logFile('[HELP] ' + ht);
        prevHelpText = ht;
    }
}

// ── Hook 1: Scene registration — FUN_008aaa60 (RVA 0x4AAA60) ──
// Can fire 16+ times on menu open. ALL output to file only.

var sceneRegCount = 0;
try {
    Interceptor.attach(rva(sceneRegisterRVA), {
        onEnter: function(args) {
            var sceneId = args[0].toInt32();
            var tkMenuPtr = args[1];
            var cursor = -1;
            try { cursor = fieldMenuCursorAddr.readS32(); } catch(e) {}

            sceneRegCount++;
            logFile('[SCENE REGISTER] scene=0x' + sceneId.toString(16) + ' cursor=' + cursor);

            try {
                var fnCtrl = tkMenuPtr.readPointer();
                var fnDraw = tkMenuPtr.add(4).readPointer();
                var fnInit = tkMenuPtr.add(8).readPointer();
                var fn0C   = tkMenuPtr.add(0x0C).readPointer();
                logFile('  fnCtrl=RVA 0x' + fnCtrl.sub(base).toString(16) +
                    ' fnDraw=RVA 0x' + fnDraw.sub(base).toString(16) +
                    ' fnInit=RVA 0x' + fnInit.sub(base).toString(16) +
                    ' fn0C=RVA 0x' + fn0C.sub(base).toString(16));
            } catch(e) { logFile('  (struct read error: ' + e.message + ')'); }
        }
    });
    logBoth('[OK] Scene register hooked (0x4AAA60)');
} catch(e) { logBoth('[FAIL] Scene register: ' + e.message); }

// ── Hook 2: Scene activation — FUN_008aa0b0 (RVA 0x4AA0B0) ──
// Can fire multiple times. File only; pool scan also file only.

var sceneActCount = 0;
try {
    Interceptor.attach(rva(sceneActivateRVA), {
        onEnter: function(args) {
            var sceneId = args[0].toInt32();
            var param2  = args[1].toInt32();
            var cursor = -1;
            try { cursor = fieldMenuCursorAddr.readS32(); } catch(e) {}

            sceneActCount++;
            logFile('[SCENE ACTIVATE] scene=0x' + sceneId.toString(16) +
                ' param2=0x' + param2.toString(16) + ' cursor=' + cursor);

            scanTkWindowPool('after scene 0x' + sceneId.toString(16) + ' activate');
            scanProbeArrays('after scene 0x' + sceneId.toString(16) + ' activate');
            checkHelpTextChange();
        }
    });
    logBoth('[OK] Scene activate hooked (0x4AA0B0)');
} catch(e) { logBoth('[FAIL] Scene activate: ' + e.message); }

// ── Hook 3: Event mask dispatcher — FUN_008aa730 (RVA 0x4AA730) ──
// Fires once per menu open. File only.

try {
    Interceptor.attach(rva(eventDispatchRVA), {
        onEnter: function(args) {
            var eventMask = args[0].toUInt32();
            var param2    = args[1].toUInt32();
            var cursor = -1;
            try { cursor = fieldMenuCursorAddr.readS32(); } catch(e) {}

            logFile('[EVENT DISPATCH] mask=0x' + eventMask.toString(16) +
                ' param2=0x' + param2.toString(16) + ' cursor=' + cursor);
        }
    });
    logBoth('[OK] Event dispatch hooked (0x4AA730)');
} catch(e) { logBoth('[FAIL] Event dispatch: ' + e.message); }

// ── Hook 4: Scene visual focus — FUN_008e1c60 (RVA 0x4E1C60) ──
// Can fire multiple times. File only.

try {
    Interceptor.attach(rva(sceneFocusRVA), {
        onEnter: function(args) {
            var arg0 = args[0].toInt32();
            var sceneId = args[1].toInt32();
            logFile('[SCENE FOCUS] arg0=' + arg0 + ' scene=0x' + sceneId.toString(16));
        }
    });
    logBoth('[OK] Scene focus hooked (0x4E1C60)');
} catch(e) { logBoth('[FAIL] Scene focus: ' + e.message); }

// ── Hook 5: MsMenuGetText (RVA 0x38FD40) — catch text by category ──
// Can fire many times (especially Overdrive). ALL output to file only.

var msMenuTextCount = 0;
try {
    Interceptor.attach(rva(msMenuGetTextRVA), {
        onEnter: function(args) {
            this.arg0 = args[0].toInt32();
            this.arg1 = args[1].toInt32();
            this.arg2 = args[2].toInt32();
        },
        onLeave: function(retval) {
            if (retval.isNull()) return;
            msMenuTextCount++;
            var segs = null;
            try { segs = decodeFFXSegments(retval, 256, 0); } catch(e) {}
            var preview = (segs && segs.length > 0) ? segs.slice(0, 3).join(' | ') : '(empty)';
            logFile('[MsMenuGetText] arg0=' + this.arg0 + ' arg1=' + this.arg1 +
                ' arg2=' + this.arg2 + ' -> ' + preview);
        }
    });
    logBoth('[OK] MsMenuGetText hooked (0x38FD40)');
} catch(e) { logBoth('[FAIL] MsMenuGetText: ' + e.message); }

// No per-frame hooks — all discovery is event-driven via scene/text hooks above.

// ── Hotkey handler ──

recv('hotkey', function onHotkey(msg) {
    var key = msg.key;
    if (key === 's') {
        scanTkWindowPool('manual scan');
        scanProbeArrays('manual scan');
        logBoth('[s] Pool scan written to log file.');
    } else if (key === 't') {
        dumpSceneTable('manual dump');
        logBoth('[t] Scene table written to log file.');
    } else if (key === 'h') {
        var ht = readHelpText();
        logBoth('[HELP] ' + (ht.length > 0 ? ht : '(empty)'));
    } else if (key === 'c') {
        logBoth('[COUNTS] registers=' + sceneRegCount +
            ' activations=' + sceneActCount +
            ' textCalls=' + msMenuTextCount);
    }
    recv('hotkey', onHotkey);
});

// ── Initial state dump ──

logBoth('');
logBoth('[*] Scanning initial state (results in log file)...');
dumpSceneTable('initial');
scanTkWindowPool('initial');
scanProbeArrays('initial');
logBoth('[*] Ready. ALL hook output goes to log file only.');
logBoth('[*] Open main menu, select each sub-menu, then check log.');
logBoth('[*] Hotkeys: s=scan pools, t=scenes, h=help text, c=counts, q=quit');
logBoth('[*] Log: ' + logPath);
