// @desc Hook AS3 external function callback — the Flash-to-game bridge for text/data
//
// Hooks:
//   FUN_00a262e0 — registers AS3 external callback (IggySetAS3ExternalFunctionCallbackUTF8)
//   The registered callback itself — captures every Flash ExternalInterface.call()
//   4 functions that call IggyValueSetStringUTF8RS — game pushing text to iggy
//
// Navigate title screen to capture text flow. Ctrl+C to stop.

var logPath = 'D:/Games/Dev/Custom/FFX/ffx-x2-screen-reader/Docs/Scripts/Frida/output/FFX/trace_as3_callback.log';
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

// ── Resolve Ghidra VA to runtime address ──
// Ghidra uses the PE preferred image base. Runtime base may differ.
// Read PE image base from the PE header in memory.

var ffxMod = Process.enumerateModules()[0];
var runtimeBase = ffxMod.base;

// Windows patches the in-memory PE ImageBase to match the actual load address.
// Ghidra analyzed the FILE from disk where the original preferred base is different.
// For 32-bit PEs the standard base is 0x00400000. We verify by checking if a known
// Ghidra address rebased with this base falls within the module range.
var moduleEnd = runtimeBase.add(ffxMod.size);
var ghidraImageBase = 0x00400000;

// Verify: FUN_00a262e0 rebased should be within [runtimeBase, moduleEnd)
var testAddr = runtimeBase.add(0x00a262e0 - ghidraImageBase);
if (testAddr.compare(runtimeBase) < 0 || testAddr.compare(moduleEnd) >= 0) {
    // Fallback: try reading from PE header (won't work if Windows patched it)
    var e_lfanew = runtimeBase.add(0x3C).readU32();
    var optionalHeader = runtimeBase.add(e_lfanew + 4 + 20);
    ghidraImageBase = optionalHeader.add(28).readU32();
}

logBoth('[*] trace_as3_callback starting');
logBoth('[*] Runtime base: 0x' + runtimeBase.toString(16));
logBoth('[*] Ghidra image base: 0x' + ghidraImageBase.toString(16));

var rebaseOffset = runtimeBase.sub(ghidraImageBase);
logBoth('[*] Rebase offset: 0x' + rebaseOffset.toString(16));

// Convert Ghidra VA to runtime address
function g(ghidraAddr) {
    return ptr(ghidraAddr).add(rebaseOffset);
}

// Test: FUN_00a262e0 should now land inside FFX.exe
var testAddr = g(0x00a262e0);
var moduleEnd = runtimeBase.add(ffxMod.size);
var inRange = testAddr.compare(runtimeBase) >= 0 && testAddr.compare(moduleEnd) < 0;
logBoth('[*] FUN_00a262e0 -> runtime 0x' + testAddr.toString(16) + ' (in range: ' + inRange + ')');

if (!inRange) {
    logBoth('[!] Address still out of range. Cannot proceed.');
} else {
    var seenCallbacks = {};
    var seenTexts = {};
    var callbackCount = 0;
    var textSetCount = 0;
    var registeredCallback = null;

    // ── Hook FUN_00a262e0 — AS3 callback registration ──
    try {
        Interceptor.attach(g(0x00a262e0), {
            onEnter: function(args) {
                logFile('[REG] FUN_00a262e0 called');
                logFile('  a0=0x' + args[0].toString(16) +
                    ' a1=0x' + args[1].toString(16) +
                    ' a2=0x' + args[2].toString(16));
                var s0 = tryReadCString(args[0]);
                var s1 = tryReadCString(args[1]);
                if (s0) logFile('  a0 str: "' + s0 + '"');
                if (s1) logFile('  a1 str: "' + s1 + '"');
                logBoth('[REG] AS3 callback registration called');
            }
        });
        logBoth('[OK] FUN_00a262e0 hooked');
    } catch (e) {
        logBoth('[!] FUN_00a262e0 hook failed: ' + e.message);
    }

    // ── Hook 4 IggyValueSetStringUTF8RS callers ──
    var setStringCallers = [
        {ghidra: 0x006ff110, name: 'FUN_006ff110'},
        {ghidra: 0x006fcca0, name: 'FUN_006fcca0'},
        {ghidra: 0x006e99b0, name: 'FUN_006e99b0'},
        {ghidra: 0x006e9650, name: 'FUN_006e9650'}
    ];

    for (var i = 0; i < setStringCallers.length; i++) {
        (function(info) {
            var addr = g(info.ghidra);
            try {
                Interceptor.attach(addr, {
                    onEnter: function(args) {
                        textSetCount++;
                        var s0 = tryReadCString(args[0]);
                        var s1 = tryReadCString(args[1]);
                        var s2 = tryReadCString(args[2]);
                        var s3 = tryReadCString(args[3]);
                        var text = s0 || s1 || s2 || s3 || null;

                        logFile('[SETSTR #' + textSetCount + '] ' + info.name +
                            ' a0=0x' + args[0].toString(16) + (s0 ? ' "' + s0 + '"' : '') +
                            ' a1=0x' + args[1].toString(16) + (s1 ? ' "' + s1 + '"' : '') +
                            ' a2=0x' + args[2].toString(16) + (s2 ? ' "' + s2 + '"' : '') +
                            ' a3=0x' + args[3].toString(16) + (s3 ? ' "' + s3 + '"' : ''));

                        if (textSetCount <= 20) {
                            var bt = Thread.backtrace(this.context, Backtracer.ACCURATE)
                                .map(function(a) { return a.toString(); })
                                .join('\n    ');
                            logFile('  backtrace:\n    ' + bt);
                        }

                        if (text && !seenTexts[text]) {
                            seenTexts[text] = true;
                            logBoth('[TEXT] ' + info.name + ': ' + text);
                        }
                    }
                });
                logBoth('[OK] ' + info.name + ' hooked at ' + addr);
            } catch (e) {
                logBoth('[!] ' + info.name + ' hook failed: ' + e.message);
            }
        })(setStringCallers[i]);
    }

    // ── Hook IggySetAS3ExternalFunctionCallbackUTF8 export ──
    var iggyMod = null;
    var modules = Process.enumerateModules();
    for (var j = 0; j < modules.length; j++) {
        if (modules[j].name.toLowerCase() === 'iggy_w32.dll') {
            iggyMod = modules[j];
            break;
        }
    }

    if (iggyMod) {
        var exports = iggyMod.enumerateExports();
        var setCallbackAddr = null;
        for (var k = 0; k < exports.length; k++) {
            if (exports[k].name === '_IggySetAS3ExternalFunctionCallbackUTF8@8') {
                setCallbackAddr = exports[k].address;
                break;
            }
        }

        if (setCallbackAddr) {
            try {
                Interceptor.attach(setCallbackAddr, {
                    onEnter: function(args) {
                        var player = args[0];
                        var cbPtr = args[1];
                        logBoth('[CB-REG] Callback registered at 0x' + cbPtr.toString(16));

                        if (!registeredCallback) {
                            registeredCallback = cbPtr;
                            try {
                                Interceptor.attach(cbPtr, {
                                    onEnter: function(args) {
                                        callbackCount++;
                                        var s0 = tryReadCString(args[0]);
                                        var s1 = tryReadCString(args[1]);
                                        var s2 = tryReadCString(args[2]);
                                        var s3 = tryReadCString(args[3]);
                                        var funcName = s0 || s1 || s2 || '(unknown)';

                                        logFile('[AS3 #' + callbackCount + '] ' +
                                            'a0=0x' + args[0].toString(16) +
                                            (s0 ? ' "' + s0 + '"' : '') +
                                            ' a1=0x' + args[1].toString(16) +
                                            (s1 ? ' "' + s1 + '"' : '') +
                                            ' a2=0x' + args[2].toString(16) +
                                            (s2 ? ' "' + s2 + '"' : '') +
                                            ' a3=0x' + args[3].toString(16) +
                                            (s3 ? ' "' + s3 + '"' : ''));

                                        if (funcName !== '(unknown)' &&
                                            !seenCallbacks[funcName]) {
                                            seenCallbacks[funcName] = true;
                                            logBoth('[AS3] ' + funcName);
                                        }
                                    }
                                });
                                logBoth('[OK] AS3 callback hooked at ' + cbPtr);
                            } catch (e) {
                                logBoth('[!] AS3 callback hook failed: ' + e.message);
                            }
                        }
                    }
                });
                logBoth('[OK] IggySetAS3ExternalFunctionCallbackUTF8 hooked');
            } catch (e) {
                logBoth('[!] Iggy callback export hook failed: ' + e.message);
            }
        } else {
            logBoth('[!] IggySetAS3ExternalFunctionCallbackUTF8 not found in exports');
        }
    } else {
        logBoth('[!] iggy_w32.dll not found');
    }

    logBoth('[*] All hooks set. Navigate title screen. Ctrl+C to stop.');
}
