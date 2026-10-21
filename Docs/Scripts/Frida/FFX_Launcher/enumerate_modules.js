// enumerate_modules.js — List all DLLs loaded by the launcher process
// Purpose: Find proxy DLL candidates for the launcher
// Output: Console summary + full list to file

var OUTPUT_DIR = 'D:/Games/Dev/Custom/FFX/ffx-x2-screen-reader/Docs/Scripts/Frida/output/FFX_Launcher/';
var outFile = new File(OUTPUT_DIR + 'loaded_modules.txt', 'w');

function logFile(msg) { outFile.write(msg + '\n'); }
function logBoth(msg) { console.log(msg); logFile(msg); }

logBoth('=== Launcher Module Enumeration ===');

var modules = Process.enumerateModules();
logBoth('Total modules loaded: ' + modules.length);
logBoth('');

// Categorize modules
var systemDlls = [];
var appDlls = [];

for (var i = 0; i < modules.length; i++) {
    var m = modules[i];
    var path = m.path.toLowerCase();
    var entry = m.name + '  (' + m.path + ')  size=' + m.size;

    logFile(entry);

    // Check if it's from the game directory (potential proxy target)
    if (path.indexOf('windows') === -1 &&
        path.indexOf('microsoft') === -1 &&
        path.indexOf('winsxs') === -1 &&
        path.indexOf('assembly') === -1) {
        appDlls.push(m.name);
    } else {
        systemDlls.push(m.name);
    }
}

logBoth('');
logBoth('--- App-directory DLLs ---');
for (var j = 0; j < appDlls.length; j++) {
    logBoth('  ' + appDlls[j]);
}

logBoth('');
logBoth('--- System DLLs (potential proxy candidates) ---');

// Filter to DLLs commonly used as proxy targets
var proxyCandidates = [
    'version.dll', 'winmm.dll', 'winhttp.dll', 'uxtheme.dll',
    'dwmapi.dll', 'iphlpapi.dll', 'dinput8.dll', 'dxgi.dll',
    'd3d9.dll', 'd3d11.dll', 'dbghelp.dll', 'wintrust.dll',
    'profapi.dll', 'propsys.dll', 'shcore.dll'
];

for (var k = 0; k < systemDlls.length; k++) {
    var name = systemDlls[k].toLowerCase();
    for (var c = 0; c < proxyCandidates.length; c++) {
        if (name === proxyCandidates[c]) {
            logBoth('  CANDIDATE: ' + systemDlls[k]);
        }
    }
}

logBoth('');
logBoth('Full module list written to: ' + OUTPUT_DIR + 'loaded_modules.txt');

outFile.flush();
outFile.close();
