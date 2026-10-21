// Native migration ownership is advertised only after a successful hook batch.
// Diagnostics and speech exports do not imply parity for another feature.
let features = 0;
let output = null;
try {
    const proxy = Process.enumerateModules().find(m => m.name.toLowerCase() === 'dinput8.dll');
    if (proxy) {
        const mask = proxy.findExportByName('FfxNative_Features');
        const speak = proxy.findExportByName('FfxNative_Output');
        const ready = proxy.findExportByName('FfxNative_SpeechReady');
        if (mask) features = new NativeFunction(mask, 'uint', [], 'mscdecl')();
        if (speak && ready && new NativeFunction(ready, 'int', [], 'mscdecl')())
            output = new NativeFunction(speak, 'int', ['pointer', 'int'], 'mscdecl');
    }
} catch (_) {}

export const NATIVE_MAIN_MENU = (features & 1) !== 0;
export const NATIVE_ALL_READERS = (features & 0x7f) === 0x7f;
export function nativeOutput(text, interrupt) {
    return output ? output(Memory.allocUtf16String(text), interrupt ? 1 : 0) !== 0 : false;
}
export const NATIVE_SPEECH = output !== null;
