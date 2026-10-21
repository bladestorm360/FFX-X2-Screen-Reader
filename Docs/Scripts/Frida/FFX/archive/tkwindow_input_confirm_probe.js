// @desc Confirm FUN_008B6260 is the TkWindow input handler.
//
// Decompile (layer2_game_logic.txt lines 552831-552896) shows this fn
// at RVA 0x4B6260 handles D-pad input for the TkWindow at slot
// param_1, incrementing/decrementing tkCurArrayAddr[slot]+2 based on
// input bitmasks. Before migrating production code, confirm by hooking
// it, logging param_1 + cursor-before / cursor-after on each call.
//
// Also includes a sanity hook on the per-frame fn (0x471D10) so we can
// see the probe is live even if the TkWindow handler never fires.
//
// Usage: run via run_frida.bat -> FFX -> tkwindow_input_confirm_probe.
// Expected log on title screen with one Down press:
//   [SANITY] per-frame=... lines every ~5s
//   [TK] slot=2 cursor 1->0 (or 0->1) on the Down press
// Ctrl+C to stop.
//
// Output: Docs/Scripts/Frida/output/FFX/tkwindow_input_confirm_probe.log

const LOG_PATH = 'D:/Games/Dev/Custom/FFX/ffx-x2-screen-reader/Docs/Scripts/Frida/output/FFX/tkwindow_input_confirm_probe.log';
const logHandle = new File(LOG_PATH, 'w');
function logFile(msg) { try { logHandle.write(msg + '\n'); logHandle.flush(); } catch (e) {} }
function logBoth(msg) { console.log(msg); logFile(msg); }

// Explicit module lookup by name (not enumeration order).
let gameModule = null;
Process.enumerateModules().forEach(function(m) {
    if (m.name.toLowerCase() === 'ffx.exe') gameModule = m;
});
if (!gameModule) gameModule = Process.enumerateModules()[0];
const base = gameModule.base;
function rva(offset) { return base.add(offset); }

logBoth('[*] tkwindow_input_confirm_probe');
logBoth('[*] Module: ' + gameModule.name + ' base=0x' + base.toString(16));

const TK_CUR_ARRAY_RVA = 0x1468A50;  // tkCurArrayAddr

function readCursor(slot) {
    try {
        const slotPtr = rva(TK_CUR_ARRAY_RVA).add(slot * 4).readPointer();
        if (slotPtr.isNull() || slotPtr.compare(ptr(0x10000)) <= 0) return null;
        return slotPtr.add(2).readU8();
    } catch (e) { return null; }
}

let dispEntries = 0, dispChanges = 0;
let handlerEntries = 0, handlerChanges = 0;

// Hook the per-slot dispatcher: iterates active slots, invokes each
// slot's +0xFC vtable method. Fires for ANY active TkWindow slot.
try {
    Interceptor.attach(rva(0x4B6B20), {
        onEnter: function(args) {
            this.slot = args[0].toInt32();
            this.cursorBefore = readCursor(this.slot);
        },
        onLeave: function() {
            dispEntries++;
            const cursorAfter = readCursor(this.slot);
            if (this.cursorBefore !== null && cursorAfter !== null &&
                this.cursorBefore !== cursorAfter) {
                dispChanges++;
                logFile('[DISP CHANGE #' + dispChanges + '] slot=' + this.slot +
                        ' cursor ' + this.cursorBefore + ' -> ' + cursorAfter);
            }
        }
    });
    logBoth('[*] hooked FUN_008B6B20 @ RVA 0x4B6B20 (per-slot dispatcher)');
} catch (e) {
    logBoth('[!] Dispatcher hook FAILED: ' + e.message);
}

// Hook the registered +0xFC handler (normal TkWindows).
try {
    Interceptor.attach(rva(0x4B6260), {
        onEnter: function(args) {
            this.slot = args[0].toInt32();
            this.cursorBefore = readCursor(this.slot);
        },
        onLeave: function() {
            handlerEntries++;
            const cursorAfter = readCursor(this.slot);
            if (this.cursorBefore !== null && cursorAfter !== null &&
                this.cursorBefore !== cursorAfter) {
                handlerChanges++;
                logFile('[HANDLER CHANGE #' + handlerChanges + '] slot=' + this.slot +
                        ' cursor ' + this.cursorBefore + ' -> ' + cursorAfter);
            }
        }
    });
    logBoth('[*] hooked FUN_008B6260 @ RVA 0x4B6260 (normal TkWindow handler)');
} catch (e) {
    logBoth('[!] Handler hook FAILED: ' + e.message);
}

// Sanity — per-frame counter + periodic summary.
let sanityFrames = 0;
let lastDump = 0;
try {
    Interceptor.attach(rva(0x471D10), {
        onLeave: function() {
            sanityFrames++;
            if (sanityFrames - lastDump < 300) return;
            lastDump = sanityFrames;
            logFile('[SANITY] per-frame=' + sanityFrames +
                    '  disp.entries=' + dispEntries + '  disp.changes=' + dispChanges +
                    '  handler.entries=' + handlerEntries + '  handler.changes=' + handlerChanges);
        }
    });
    logBoth('[*] hooked per-frame sanity @ RVA 0x471D10');
} catch (e) {
    logBoth('[!] Sanity hook FAILED: ' + e.message);
}

logBoth('[*] Ready. Title screen, press Down ONCE, Ctrl+C when done.');
