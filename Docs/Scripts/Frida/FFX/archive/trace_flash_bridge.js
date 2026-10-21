// @desc Hook Flash<->Game bridge — AS3 callbacks, text lookups, game->Flash calls
//
// Targets (all Ghidra VAs, rebased with offset 0x680000):
//   FUN_00a26100 — AS3 callback dispatcher (ALL Flash->game calls go through here)
//   FUN_006e96d0 — getLocByStr (Flash requests localized text by string name)
//   FUN_006e96b0 — getLocById (Flash requests localized text by integer ID)
//   FUN_00a28ba0 — game->Flash function caller (game tells Flash to update)
//   FUN_00a28b20 — read int from Flash VM (game reads selection index)
//   FUN_00a28b60 — read string from Flash VM (game reads text from Flash)
//
// Navigate title screen (up/down/confirm). Ctrl+C to stop.

var logPath = 'D:/Games/Dev/Custom/FFX/ffx-x2-screen-reader/Docs/Scripts/Frida/output/FFX/trace_flash_bridge.log';
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

// ── Ghidra rebase (proven: ghidra base 0x400000, offset 0x680000) ──
var ffxMod = Process.enumerateModules()[0];
var runtimeBase = ffxMod.base;
var ghidraImageBase = 0x00400000;
var rebaseOffset = runtimeBase.sub(ghidraImageBase);

function g(ghidraAddr) {
    return ptr(ghidraAddr).add(rebaseOffset);
}

logBoth('[*] trace_flash_bridge (rebase 0x' + rebaseOffset.toString(16) + ')');

var counts = {};
var seen = {};

// ── Hook 1: FUN_00a26100 — AS3 callback dispatcher ──
// This is the UNIVERSAL Flash->game bridge. Every ExternalInterface.call() enters here.
// Uses hash-based lookup (0x1505 polynomial) on function name.
// Args: this=context with callback registry at +0x10/+0x14
try {
    Interceptor.attach(g(0x00a26100), {
        onEnter: function(args) {
            if (!counts['AS3Dispatch']) counts['AS3Dispatch'] = 0;
            counts['AS3Dispatch']++;

            // ECX (this pointer) has the callback registry
            var ctx = this.context.ecx;
            // Try to read args — the dispatcher receives the function name as a string
            // from Flash. Read stack/args for the name.
            var s0 = tryReadCString(args[0]);
            var s1 = tryReadCString(args[1]);
            var s2 = tryReadCString(args[2]);

            var name = s0 || s1 || s2 || '(hash-only)';

            logFile('[AS3Dispatch #' + counts['AS3Dispatch'] + '] ctx=0x' +
                ctx.toString(16) +
                ' a0=0x' + args[0].toString(16) + (s0 ? '("' + s0 + '")' : '') +
                ' a1=0x' + args[1].toString(16) + (s1 ? '("' + s1 + '")' : '') +
                ' a2=0x' + args[2].toString(16) + (s2 ? '("' + s2 + '")' : ''));

            if (!seen['AS3:' + name]) {
                seen['AS3:' + name] = true;
                logBoth('[AS3] ' + name);
            }
        }
    });
    logBoth('[OK] AS3 callback dispatcher (FUN_00a26100)');
} catch (e) {
    logBoth('[!] AS3 dispatcher: ' + e.message);
}

// ── Hook 2: FUN_006e96d0 — getLocByStr ──
// Flash calls this to get localized text by string name.
// Returns UTF-8 string via IggyValueSetStringUTF8RS.
try {
    Interceptor.attach(g(0x006e96d0), {
        onEnter: function(args) {
            if (!counts['getLocByStr']) counts['getLocByStr'] = 0;
            counts['getLocByStr']++;

            var s0 = tryReadCString(args[0]);
            var s1 = tryReadCString(args[1]);
            var s2 = tryReadCString(args[2]);
            var s3 = tryReadCString(args[3]);

            logFile('[getLocByStr #' + counts['getLocByStr'] + ']' +
                ' a0=0x' + args[0].toString(16) + (s0 ? '("' + s0 + '")' : '') +
                ' a1=0x' + args[1].toString(16) + (s1 ? '("' + s1 + '")' : '') +
                ' a2=0x' + args[2].toString(16) + (s2 ? '("' + s2 + '")' : '') +
                ' a3=0x' + args[3].toString(16) + (s3 ? '("' + s3 + '")' : ''));

            var textName = s0 || s1 || s2 || s3;
            if (textName && !seen['LocStr:' + textName]) {
                seen['LocStr:' + textName] = true;
                logBoth('[LocStr] ' + textName);
            }
        }
    });
    logBoth('[OK] getLocByStr (FUN_006e96d0)');
} catch (e) {
    logBoth('[!] getLocByStr: ' + e.message);
}

// ── Hook 3: FUN_006e96b0 — getLocById ──
// Flash calls this to get localized text by integer ID.
try {
    Interceptor.attach(g(0x006e96b0), {
        onEnter: function(args) {
            if (!counts['getLocById']) counts['getLocById'] = 0;
            counts['getLocById']++;

            var id = args[1].toInt32();

            logFile('[getLocById #' + counts['getLocById'] + ']' +
                ' a0=0x' + args[0].toString(16) +
                ' id=' + id +
                ' a2=0x' + args[2].toString(16) +
                ' a3=0x' + args[3].toString(16));

            if (!seen['LocId:' + id]) {
                seen['LocId:' + id] = true;
                logBoth('[LocId] id=' + id);
            }
        }
    });
    logBoth('[OK] getLocById (FUN_006e96b0)');
} catch (e) {
    logBoth('[!] getLocById: ' + e.message);
}

// ── Hook 4: FUN_00a28ba0 — game->Flash function caller ──
// Game calls Flash functions: setSysSettingPage, setAudioPage, showPopUp, etc.
// param_2 is the function name string.
try {
    Interceptor.attach(g(0x00a28ba0), {
        onEnter: function(args) {
            if (!counts['CallFlash']) counts['CallFlash'] = 0;
            counts['CallFlash']++;

            var s1 = tryReadCString(args[1]);

            logFile('[CallFlash #' + counts['CallFlash'] + ']' +
                ' a0=0x' + args[0].toString(16) +
                ' name=' + (s1 || '0x' + args[1].toString(16)) +
                ' a2=0x' + args[2].toString(16) +
                ' a3=0x' + args[3].toString(16) +
                ' a4=0x' + args[4].toString(16));

            if (s1 && !seen['Flash:' + s1]) {
                seen['Flash:' + s1] = true;
                logBoth('[CallFlash] ' + s1);
            }
        }
    });
    logBoth('[OK] game->Flash caller (FUN_00a28ba0)');
} catch (e) {
    logBoth('[!] CallFlash: ' + e.message);
}

// ── Hook 5: FUN_00a28b20 — read int from Flash ──
// Game reads selected value (int32) from Flash VM.
try {
    Interceptor.attach(g(0x00a28b20), {
        onEnter: function(args) {
            if (!counts['ReadInt']) counts['ReadInt'] = 0;
            counts['ReadInt']++;
            this.index = args[0].toInt32();
        },
        onLeave: function(retval) {
            var val = retval.toInt32();
            logFile('[ReadInt #' + counts['ReadInt'] + '] index=' +
                this.index + ' result=' + val);

            if (!seen['ReadInt:' + this.index + ':' + val]) {
                seen['ReadInt:' + this.index + ':' + val] = true;
                logBoth('[ReadInt] index=' + this.index + ' val=' + val);
            }
        }
    });
    logBoth('[OK] read int from Flash (FUN_00a28b20)');
} catch (e) {
    logBoth('[!] ReadInt: ' + e.message);
}

// ── Hook 6: FUN_00a28b60 — read string from Flash ──
// Game reads text from Flash VM.
try {
    Interceptor.attach(g(0x00a28b60), {
        onEnter: function(args) {
            if (!counts['ReadStr']) counts['ReadStr'] = 0;
            counts['ReadStr']++;

            logFile('[ReadStr #' + counts['ReadStr'] + '] a0=0x' +
                args[0].toString(16) + ' a1=0x' + args[1].toString(16) +
                ' a2=0x' + args[2].toString(16));
        },
        onLeave: function(retval) {
            var s = tryReadCString(retval);
            logFile('[ReadStr result] ' + (s || '0x' + retval.toString(16)));
            if (s && !seen['ReadStr:' + s]) {
                seen['ReadStr:' + s] = true;
                logBoth('[ReadStr] "' + s + '"');
            }
        }
    });
    logBoth('[OK] read string from Flash (FUN_00a28b60)');
} catch (e) {
    logBoth('[!] ReadStr: ' + e.message);
}

// ── Hook 7: FUN_00a28e90 — register named callback ──
// Catches callback registrations like "Finish" for title screen.
try {
    Interceptor.attach(g(0x00a28e90), {
        onEnter: function(args) {
            if (!counts['RegCB']) counts['RegCB'] = 0;
            counts['RegCB']++;

            var name = tryReadCString(args[0]);
            var cbAddr = args[1];

            logFile('[RegCB #' + counts['RegCB'] + '] name=' +
                (name || '0x' + args[0].toString(16)) +
                ' callback=0x' + cbAddr.toString(16));

            if (name) {
                logBoth('[RegCB] "' + name + '" -> 0x' + cbAddr.toString(16));
            }
        }
    });
    logBoth('[OK] register callback (FUN_00a28e90)');
} catch (e) {
    logBoth('[!] RegCB: ' + e.message);
}

logBoth('[*] All hooks set. Navigate title screen (up/down/confirm). Ctrl+C to stop.');
