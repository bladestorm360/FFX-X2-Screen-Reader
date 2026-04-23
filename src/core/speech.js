// File logging + Tolk speech output. Initializes on first import.

import { base } from './memory.js';

let logHandle = null;
let LOG_PATH = '';

(function initLog() {
    try {
        const exePath = Process.enumerateModules()[0].path;
        const gameDir = exePath.substring(0, exePath.lastIndexOf('\\') + 1);
        LOG_PATH = gameDir + 'FFX_SR_Latest.log';
        logHandle = new File(LOG_PATH, 'w');
        logFile('FFX Screen Reader Mod — Frida Gadget mode');
        logFile('Game: ' + exePath);
        logFile('Base: 0x' + base.toString(16));
    } catch (e) {}
})();

export function logFile(msg) {
    if (!logHandle) return;
    try {
        const d = new Date();
        const ts = ('0'+d.getHours()).slice(-2) + ':' + ('0'+d.getMinutes()).slice(-2) + ':' +
                   ('0'+d.getSeconds()).slice(-2) + '.' + ('00'+d.getMilliseconds()).slice(-3);
        logHandle.write('[' + ts + '] ' + msg + '\n');
        logHandle.flush();
    } catch (e) {}
}

let tolkLoaded = false;
let fnTolkOutput = null;

(function initTolk() {
    try {
        Module.load('Tolk.dll');
        let tolkModule = null;
        Process.enumerateModules().forEach(function(m) {
            if (m.name.toLowerCase() === 'tolk.dll') tolkModule = m;
        });
        if (!tolkModule) { logFile('Tolk.dll not available'); return; }

        const addrs = {};
        tolkModule.enumerateExports().forEach(function(exp) { addrs[exp.name] = exp.address; });

        const fnLoad = new NativeFunction(addrs['Tolk_Load'], 'void', []);
        const fnTrySAPI = new NativeFunction(addrs['Tolk_TrySAPI'], 'void', ['int']);
        const fnIsLoaded = new NativeFunction(addrs['Tolk_IsLoaded'], 'int', []);
        fnTolkOutput = new NativeFunction(addrs['Tolk_Output'], 'int', ['pointer', 'int']);
        const fnDetect = new NativeFunction(addrs['Tolk_DetectScreenReader'], 'pointer', []);

        fnTrySAPI(1);
        fnLoad();
        if (fnIsLoaded()) {
            tolkLoaded = true;
            const srPtr = fnDetect();
            logFile('Tolk OK, screen reader: ' + (srPtr.isNull() ? 'none' : srPtr.readUtf16String()));
        } else {
            logFile('Tolk_IsLoaded returned false');
        }
    } catch (e) { logFile('Tolk init error: ' + e.message); }
})();

export function speak(text, interrupt) {
    logFile('[SPEECH' + (interrupt ? ' INT' : '') + '] ' + text);
    if (!tolkLoaded) return;
    try {
        const buf = Memory.allocUtf16String(text);
        fnTolkOutput(buf, interrupt ? 1 : 0);
    } catch (e) {}
}

logFile('Speech layer ready');
