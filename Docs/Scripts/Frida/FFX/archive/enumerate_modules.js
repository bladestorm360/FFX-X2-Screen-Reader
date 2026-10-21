// @auto
// @desc Enumerate modules, iggy exports, and FFX.exe imports for recon

var logPath = 'D:/Games/Dev/Custom/FFX/ffx-x2-screen-reader/Docs/Scripts/Frida/output/FFX/enumerate_modules.log';
var logHandle = new File(logPath, 'w');

function logFile(msg) {
    logHandle.write(msg + '\n');
    logHandle.flush();
}

function logBoth(msg) {
    console.log(msg);
    logFile(msg);
}

// ── 1. Enumerate all loaded modules ──

var modules = Process.enumerateModules();
logBoth('[*] Loaded modules: ' + modules.length);
logFile('');
logFile('=== ALL LOADED MODULES ===');
logFile('');

for (var i = 0; i < modules.length; i++) {
    var m = modules[i];
    logFile('  ' + m.name + '  base=0x' + m.base.toString(16) + '  size=0x' + m.size.toString(16));
}

// ── 2. Iggy exports ──

logFile('');
logFile('=== IGGY_W32.DLL EXPORTS ===');
logFile('');

var iggyMod = null;
for (var i = 0; i < modules.length; i++) {
    if (modules[i].name.toLowerCase() === 'iggy_w32.dll') {
        iggyMod = modules[i];
        break;
    }
}

var iggyExportCount = 0;
var iggyInteresting = [];
var interestingPatterns = /text|string|invoke|movie|value|variable|gfx/i;

if (iggyMod) {
    var exports = iggyMod.enumerateExports();
    iggyExportCount = exports.length;
    logFile('Total exports: ' + exports.length);
    logFile('');

    for (var i = 0; i < exports.length; i++) {
        var e = exports[i];
        var line = '  [' + e.type + '] ' + e.name + '  addr=0x' + e.address.toString(16);
        logFile(line);
        if (interestingPatterns.test(e.name)) {
            iggyInteresting.push(e.name);
        }
    }

    logFile('');
    logFile('--- Interesting iggy exports (text/string/invoke/movie/value/variable/gfx) ---');
    logFile('');
    for (var i = 0; i < iggyInteresting.length; i++) {
        logFile('  ' + iggyInteresting[i]);
    }
    logBoth('[*] Iggy exports: ' + iggyExportCount + ' total, ' + iggyInteresting.length + ' interesting');
} else {
    logBoth('[!] iggy_w32.dll not found');
}

// ── 3. FFX.exe imports ──

logFile('');
logFile('=== FFX.EXE IMPORTS ===');
logFile('');

var ffxMod = modules[0]; // main exe is always first
var imports = ffxMod.enumerateImports();

// Group imports by module
var importsByDll = {};
for (var i = 0; i < imports.length; i++) {
    var imp = imports[i];
    var dll = imp.module ? imp.module.toLowerCase() : '(unknown)';
    if (!importsByDll[dll]) {
        importsByDll[dll] = [];
    }
    importsByDll[dll].push(imp);
}

var dllNames = Object.keys(importsByDll).sort();
logFile('FFX.exe imports from ' + dllNames.length + ' DLLs, ' + imports.length + ' total functions');
logFile('');

// Proxy DLL candidates — these are commonly used for DLL proxying
var proxyTargets = ['d3d9.dll', 'dinput8.dll', 'dinput.dll', 'dsound.dll',
                    'xinput1_3.dll', 'xinput1_4.dll', 'xinput9_1_0.dll',
                    'dxgi.dll', 'winmm.dll', 'version.dll'];
var proxyFindings = [];

for (var d = 0; d < dllNames.length; d++) {
    var dll = dllNames[d];
    var funcs = importsByDll[dll];
    logFile('── ' + dll + ' (' + funcs.length + ' imports) ──');
    for (var f = 0; f < funcs.length; f++) {
        var imp = funcs[f];
        var name = imp.name || ('ordinal#' + imp.ordinal);
        logFile('    ' + name + '  addr=0x' + (imp.address ? imp.address.toString(16) : '?'));
    }
    logFile('');

    // Check if this is a proxy candidate
    for (var p = 0; p < proxyTargets.length; p++) {
        if (dll === proxyTargets[p]) {
            proxyFindings.push(dll + ' (' + funcs.length + ' imports)');
            break;
        }
    }
}

// ── 4. Console summary ──

logBoth('[*] FFX.exe imports: ' + imports.length + ' functions from ' + dllNames.length + ' DLLs');

if (proxyFindings.length > 0) {
    logBoth('[*] Proxy DLL candidates:');
    for (var i = 0; i < proxyFindings.length; i++) {
        logBoth('    ' + proxyFindings[i]);
    }
} else {
    logBoth('[!] No common proxy DLL candidates found in imports');
}

logFile('');
logFile('=== ALL IMPORTED DLLs (summary) ===');
logFile('');
for (var d = 0; d < dllNames.length; d++) {
    logFile('  ' + dllNames[d] + ' (' + importsByDll[dllNames[d]].length + ')');
}

logBoth('[*] Log written to: ' + logPath);
logHandle.close();
