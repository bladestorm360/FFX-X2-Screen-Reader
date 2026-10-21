// @desc Track native cursor — READ FROM ECX (this pointer), NOT global pointers
//
// CRITICAL FIX: All previous probes read from global DAT_ pointers which point to
// DIFFERENT objects than what the hooked functions operate on. This probe reads from
// this.context.ecx — the actual 'this' pointer in __thiscall convention.
//
// Hooks:
//   FUN_0062dd50 (RVA 0x22dd50) — XInput decoder (688 bytes, fires every frame)
//   FUN_006f22f0 (RVA 0x2f22f0) — Input dispatch (119 bytes)
//   FUN_00647ea0 (RVA 0x247ea0) — Cursor handler (23 bytes)
//
// Console: ECX addresses on first call, then ONLY non-zero buttons or cursor changes.
// File: full state dumps.
//
// Use Xbox controller on title screen. Ctrl+C to stop.

var logPath = 'D:/Games/Dev/Custom/FFX/ffx-x2-screen-reader/Docs/Scripts/Frida/output/FFX/track_cursor_native.log';
var logHandle = new File(logPath, 'w');

function logFile(msg) {
    logHandle.write(msg + '\n');
    logHandle.flush();
}

function logBoth(msg) {
    console.log(msg);
    logFile(msg);
}

function hexDump(addr, len) {
    try {
        var buf = addr.readByteArray(len);
        var bytes = new Uint8Array(buf);
        var hex = '';
        for (var i = 0; i < bytes.length; i++) {
            var h = bytes[i].toString(16);
            if (h.length < 2) h = '0' + h;
            hex += h;
            if ((i + 1) % 16 === 0) hex += '\n    ';
            else if ((i + 1) % 4 === 0) hex += ' ';
        }
        return hex.trim();
    } catch(e) { return '(unreadable: ' + e.message + ')'; }
}

function safeReadU8(addr) { try { return addr.readU8(); } catch(e) { return -1; } }
function safeReadS32(addr) { try { return addr.readS32(); } catch(e) { return -99999; } }
function safeReadU32(addr) { try { return addr.readU32(); } catch(e) { return 0; } }

// ── RVA addressing ──
var base = Process.enumerateModules()[0].base;
function rva(offset) { return base.add(offset); }
function toGhidra(addr) { return ptr(addr).sub(base).add(0x00400000); }

logBoth('[*] track_cursor_native v2 (ECX fix)');
logBoth('[*] base=0x' + base.toString(16));

// ── Hook 1: FUN_0062dd50 (RVA 0x22dd50) — XInput decoder ──
var xinputCount = 0;
var xinputEcxDumped = false;
var prevButtons = '';

try {
    Interceptor.attach(rva(0x22dd50), {
        onEnter: function(args) {
            this.ecx = ptr(this.context.ecx);
        },
        onLeave: function(retval) {
            xinputCount++;
            var ecx = this.ecx;

            // First call: dump ECX address and object layout
            if (!xinputEcxDumped) {
                xinputEcxDumped = true;
                logBoth('[XINPUT] ECX=0x' + ecx.toString(16) +
                    ' (Ghidra obj at 0x' + toGhidra(ecx).toString(16) + ')');
                logFile('[XINPUT] Dump ECX+0x390 to ECX+0x3C0:');
                logFile('    ' + hexDump(ecx.add(0x390), 0x30));
                logFile('[XINPUT] Dump ECX+0x0 to ECX+0x40:');
                logFile('    ' + hexDump(ecx, 0x40));
            }

            // Read buttons FROM ECX
            var btnA = safeReadU8(ecx.add(0x398));
            var btnB = safeReadU8(ecx.add(0x399));
            var btnC = safeReadU8(ecx.add(0x39a));
            var btnD = safeReadU8(ecx.add(0x39b));
            var cursor = safeReadS32(ecx.add(0x39c));

            var buttonStr = btnA + ',' + btnB + ',' + btnC + ',' + btnD;

            // File: periodic logging
            if (xinputCount <= 5 || xinputCount % 100 === 0) {
                logFile('[XINPUT #' + xinputCount + '] ECX=0x' + ecx.toString(16) +
                    ' A=' + btnA + ' B=' + btnB + ' C=' + btnC + ' D=' + btnD +
                    ' cursor=' + cursor);
            }

            // Console: ONLY when buttons change from previous (actual input detected)
            if (buttonStr !== prevButtons) {
                if (prevButtons !== '' && buttonStr !== '0,0,0,0') {
                    logBoth('[BUTTON] A=' + btnA + ' B=' + btnB +
                        ' C=' + btnC + ' D=' + btnD + ' cursor=' + cursor);
                }
                prevButtons = buttonStr;
            }
        }
    });
    logBoth('[OK] XInput decoder (RVA 0x22dd50)');
} catch(e) {
    logBoth('[!] XInput decoder: ' + e.message);
}

// ── Hook 2: FUN_006f22f0 (RVA 0x2f22f0) — Input dispatch ──
var dispatchCount = 0;
var dispatchEcxDumped = false;

try {
    Interceptor.attach(rva(0x2f22f0), {
        onEnter: function(args) {
            this.ecx = ptr(this.context.ecx);
            dispatchCount++;

            var ecx = this.ecx;

            // First call: dump ECX
            if (!dispatchEcxDumped) {
                dispatchEcxDumped = true;
                logBoth('[DISPATCH] ECX=0x' + ecx.toString(16));
                logFile('[DISPATCH] Dump ECX+0x0 to ECX+0x40:');
                logFile('    ' + hexDump(ecx, 0x40));
                logFile('[DISPATCH] Dump ECX+0x390 to ECX+0x3C0:');
                logFile('    ' + hexDump(ecx.add(0x390), 0x30));
            }

            // Read state FROM ECX
            var cursor = safeReadS32(ecx.add(0x39c));
            var state = safeReadS32(ecx.add(0x08));
            var pressed = safeReadS32(ecx.add(0x3ac));

            // File: periodic
            if (dispatchCount <= 5 || dispatchCount % 100 === 0) {
                logFile('[DISPATCH #' + dispatchCount + '] ECX=0x' + ecx.toString(16) +
                    ' cursor=' + cursor + ' state=' + state + ' pressed=' + pressed);
            }
        }
    });
    logBoth('[OK] Input dispatch (RVA 0x2f22f0)');
} catch(e) {
    logBoth('[!] Input dispatch: ' + e.message);
}

// ── Hook 3: FUN_00647ea0 (RVA 0x247ea0) — Cursor handler ──
var cursorCount = 0;
var cursorEcxDumped = false;

try {
    Interceptor.attach(rva(0x247ea0), {
        onEnter: function(args) {
            this.ecx = ptr(this.context.ecx);

            if (!cursorEcxDumped) {
                cursorEcxDumped = true;
                logBoth('[CURSOR_HANDLER] ECX=0x' + this.ecx.toString(16));
                logFile('[CURSOR_HANDLER] Dump ECX+0x390 to ECX+0x3C0:');
                logFile('    ' + hexDump(this.ecx.add(0x390), 0x30));
            }

            // Read cursor BEFORE from ECX
            this.beforeVal = safeReadS32(this.ecx.add(0x39c));
        },
        onLeave: function(retval) {
            cursorCount++;
            var afterVal = safeReadS32(this.ecx.add(0x39c));
            logBoth('[CURSOR] ' + this.beforeVal + ' -> ' + afterVal +
                ' (ECX=0x' + this.ecx.toString(16) + ')');
        }
    });
    logBoth('[OK] Cursor handler (RVA 0x247ea0)');
} catch(e) {
    logBoth('[!] Cursor handler: ' + e.message);
}

// ── Also hook PInput::Update to see its ECX ──
var pInputCount = 0;

try {
    Interceptor.attach(rva(0x225930), {
        onEnter: function(args) {
            pInputCount++;
            if (pInputCount === 1) {
                var ecx = ptr(this.context.ecx);
                logBoth('[PINPUT] ECX=0x' + ecx.toString(16));
                logFile('[PINPUT] Dump ECX+0x0 to ECX+0x40:');
                logFile('    ' + hexDump(ecx, 0x40));
            }
        }
    });
    logBoth('[OK] PInput::Update (RVA 0x225930)');
} catch(e) {
    logBoth('[!] PInput::Update: ' + e.message);
}

logBoth('[*] All hooks set. Press d-pad on Xbox controller.');
logBoth('[*] Button presses and cursor changes will appear on console.');
