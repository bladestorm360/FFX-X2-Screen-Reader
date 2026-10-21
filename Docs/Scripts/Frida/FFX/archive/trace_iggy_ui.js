// @desc Trace Iggy UI text sets and ActionScript calls — navigate menus to capture
//
// Hooks:
//   IggyValueSetStringUTF8RS   — every text string the game writes to the UI
//   IggyPlayerCallFunctionRS   — every ActionScript function the game calls
//   IggyPlayerDispatchEventRS  — UI events (navigation, button presses)
//
// Console: first occurrence of each unique text/function (keeps output manageable)
// File: full trace with all args and timestamps

var logPath = 'D:/Games/Dev/Custom/FFX/ffx-x2-screen-reader/Docs/Scripts/Frida/output/FFX/trace_iggy_ui.log';
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

// Track unique strings/functions for console output management
var seenTexts = {};
var seenFuncs = {};
var seenEvents = {};
var textCount = 0;
var funcCount = 0;
var eventCount = 0;

// ── Resolve exports via proven pattern (enumerateExports on module instance) ──

var iggyMod = null;
var modules = Process.enumerateModules();
for (var i = 0; i < modules.length; i++) {
    if (modules[i].name.toLowerCase() === 'iggy_w32.dll') {
        iggyMod = modules[i];
        break;
    }
}

if (!iggyMod) {
    logBoth('[!] iggy_w32.dll not found — is the game fully loaded?');
} else {
    var exports = iggyMod.enumerateExports();
    var setStrAddr = null;
    var callFuncAddr = null;
    var dispatchAddr = null;

    for (var i = 0; i < exports.length; i++) {
        var name = exports[i].name;
        if (name === '_IggyValueSetStringUTF8RS@20') setStrAddr = exports[i].address;
        else if (name === '_IggyPlayerCallFunctionRS@20') callFuncAddr = exports[i].address;
        else if (name === '_IggyPlayerDispatchEventRS@12') dispatchAddr = exports[i].address;
    }

    logBoth('[*] trace_iggy_ui starting');
    logBoth('[*] SetStringUTF8RS: ' + setStrAddr);
    logBoth('[*] CallFunctionRS:  ' + callFuncAddr);
    logBoth('[*] DispatchEventRS: ' + dispatchAddr);
    logBoth('[*] Navigate menus to see text flow. Ctrl+C to stop.');

    // ── Hook: IggyValueSetStringUTF8RS@20 ──
    // __stdcall, 5 params (20 bytes). Signature unknown — probe all args for strings.

    if (setStrAddr) {
        Interceptor.attach(setStrAddr, {
            onEnter: function(args) {
                textCount++;
                var a0 = args[0];
                var a1 = args[1];
                var a2 = args[2];
                var a3 = args[3];
                var a4 = args[4];

                var s1 = tryReadCString(a1);
                var s2 = tryReadCString(a2);
                var s3 = tryReadCString(a3);

                var textVal = s1 || s2 || s3 || '(no string found)';

                logFile('[TEXT #' + textCount + '] args: ' +
                    'a0=0x' + a0.toString(16) +
                    ' a1=0x' + a1.toString(16) + (s1 ? ' "' + s1 + '"' : '') +
                    ' a2=0x' + a2.toString(16) + (s2 ? ' "' + s2 + '"' : '') +
                    ' a3=0x' + a3.toString(16) + (s3 ? ' "' + s3 + '"' : '') +
                    ' a4=0x' + a4.toString(16));

                if (textVal !== '(no string found)' && !seenTexts[textVal]) {
                    seenTexts[textVal] = true;
                    logBoth('[TEXT] ' + textVal);
                }
            }
        });
        logFile('[*] SetStringUTF8RS hooked');
    } else {
        logBoth('[!] SetStringUTF8RS export not found');
    }

    // ── Hook: IggyPlayerCallFunctionRS@20 ──
    // __stdcall, 5 params. Likely: (player, funcName, argsArray, numArgs, resultPtr)

    if (callFuncAddr) {
        Interceptor.attach(callFuncAddr, {
            onEnter: function(args) {
                funcCount++;
                var a0 = args[0];
                var a1 = args[1];
                var a2 = args[2];
                var a3 = args[3];
                var a4 = args[4];

                var s1 = tryReadCString(a1);
                var s2 = tryReadCString(a2);

                var funcName = s1 || s2 || '(unknown)';

                logFile('[FUNC #' + funcCount + '] args: ' +
                    'a0=0x' + a0.toString(16) +
                    ' a1=0x' + a1.toString(16) + (s1 ? ' "' + s1 + '"' : '') +
                    ' a2=0x' + a2.toString(16) + (s2 ? ' "' + s2 + '"' : '') +
                    ' a3=0x' + a3.toString(16) +
                    ' a4=0x' + a4.toString(16));

                if (funcName !== '(unknown)' && !seenFuncs[funcName]) {
                    seenFuncs[funcName] = true;
                    logBoth('[FUNC] ' + funcName);
                }
            }
        });
        logFile('[*] CallFunctionRS hooked');
    } else {
        logBoth('[!] CallFunctionRS export not found');
    }

    // ── Hook: IggyPlayerDispatchEventRS@12 ──
    // __stdcall, 3 params. Likely: (player, eventName, eventData)

    if (dispatchAddr) {
        Interceptor.attach(dispatchAddr, {
            onEnter: function(args) {
                eventCount++;
                var a0 = args[0];
                var a1 = args[1];
                var a2 = args[2];

                var s1 = tryReadCString(a1);
                var s2 = tryReadCString(a2);

                var eventName = s1 || s2 || '(unknown)';

                logFile('[EVENT #' + eventCount + '] args: ' +
                    'a0=0x' + a0.toString(16) +
                    ' a1=0x' + a1.toString(16) + (s1 ? ' "' + s1 + '"' : '') +
                    ' a2=0x' + a2.toString(16) + (s2 ? ' "' + s2 + '"' : ''));

                if (eventName !== '(unknown)' && !seenEvents[eventName]) {
                    seenEvents[eventName] = true;
                    logBoth('[EVENT] ' + eventName);
                }
            }
        });
        logFile('[*] DispatchEventRS hooked');
    } else {
        logBoth('[!] DispatchEventRS export not found');
    }
}
