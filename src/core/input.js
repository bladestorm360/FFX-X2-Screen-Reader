// Keyboard input via GetAsyncKeyState, with foreground-window gating.

import { logFile } from './speech.js';

export const VK_OEM_4 = 0xDB; // [
export const VK_OEM_6 = 0xDD; // ]
export const VK_OEM_3 = 0xC0; // `
export const VK_OEM_5 = 0xDC; // backslash
export const VK_P = 0x50;
export const VK_T = 0x54;
export const VK_H = 0x48;
export const VK_N = 0x4E;
export const VK_S = 0x53;
export const VK_O = 0x4F;
export const VK_SHIFT = 0x10;

let fnGetAsyncKeyStateRaw = null;
let fnGetForegroundWindow = null;
let fnGetWindowThreadProcessId = null;
const myPid = Process.id;
const keyWasDown = {};
const pidBuf = Memory.alloc(4);

try {
    let user32 = null;
    Process.enumerateModules().forEach(function(m) {
        if (m.name.toLowerCase() === 'user32.dll') user32 = m;
    });
    if (user32) {
        user32.enumerateExports().forEach(function(exp) {
            if (exp.name === 'GetAsyncKeyState')
                fnGetAsyncKeyStateRaw = new NativeFunction(exp.address, 'int16', ['int']);
            if (exp.name === 'GetForegroundWindow')
                fnGetForegroundWindow = new NativeFunction(exp.address, 'pointer', []);
            if (exp.name === 'GetWindowThreadProcessId')
                fnGetWindowThreadProcessId = new NativeFunction(exp.address, 'uint32', ['pointer', 'pointer']);
        });
    }
    logFile('Keyboard: ' + (fnGetAsyncKeyStateRaw ? 'OK' : 'FAIL') +
        ', ForegroundCheck: ' + (fnGetForegroundWindow ? 'OK' : 'FAIL'));
} catch (e) { logFile('Keyboard init error: ' + e.message); }

export function isGameFocused() {
    if (!fnGetForegroundWindow || !fnGetWindowThreadProcessId) return true;
    const hwnd = fnGetForegroundWindow();
    if (hwnd.isNull()) return false;
    fnGetWindowThreadProcessId(hwnd, pidBuf);
    return pidBuf.readU32() === myPid;
}

export function keyPressed(vk) {
    if (!fnGetAsyncKeyStateRaw) return false;
    const down = (fnGetAsyncKeyStateRaw(vk) & 0x8000) !== 0;
    const was = keyWasDown[vk] || false;
    keyWasDown[vk] = down;
    if (!isGameFocused()) return false;
    return down && !was;
}

// Raw query — used for modifier checks (e.g. Shift state while another key is pressed).
export function keyDown(vk) {
    if (!fnGetAsyncKeyStateRaw) return false;
    return (fnGetAsyncKeyStateRaw(vk) & 0x8000) !== 0;
}
