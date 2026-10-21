// @desc Launcher native window/input events; F9 dumps real child controls (no injection proxy)
// @auto
// Run from run_frida.bat, game 4. Diagnostic only; never launches a game,
// sends input, loads speech DLLs, or changes launcher controls/configuration.
// MSG layout is x86. All snapshots happen on explicit UI events.
if (Process.pointerSize !== 4) throw new Error('Expected the x86 launcher');
var user32 = Process.enumerateModules().find(function(m) { return m.name.toLowerCase() === 'user32.dll'; });
if (!user32) throw new Error('USER32 is not loaded');
var exports = user32.enumerateExports();
function address(name) {
    var entry = exports.find(function(e) { return e.name === name; });
    if (!entry) throw new Error('Missing USER32 export: ' + name);
    return entry.address;
}
function native(name, result, args) { return new NativeFunction(address(name), result, args); }
var getTitle = native('GetWindowTextW', 'int', ['pointer', 'pointer', 'int']);
var getClass = native('GetClassNameW', 'int', ['pointer', 'pointer', 'int']);
var ancestor = native('GetAncestor', 'pointer', ['pointer', 'uint']);
var enumChildren = native('EnumChildWindows', 'int', ['pointer', 'pointer', 'pointer']);
var enumWindows = native('EnumWindows', 'int', ['pointer', 'pointer']);
var getPid = native('GetWindowThreadProcessId', 'uint', ['pointer', 'pointer']);
var outputDir = 'D:/Games/Dev/Custom/FFX/ffx-x2-screen-reader/Docs/Scripts/Frida/output/FFX_Launcher/';
var stamp = new Date().toISOString().replace(/[:.]/g, '-');
var file = new File(outputDir + 'native_ui_events_' + stamp + '.log', 'w');
function log(message) { console.log(message); file.write(message + '\n'); file.flush(); }
function text(hwnd, getter) {
    var buffer = Memory.alloc(1024);
    var count = getter(hwnd, buffer, 512);
    return count > 0 ? buffer.readUtf16String(count) : '';
}
function ours(hwnd) {
    var pid = Memory.alloc(4);
    getPid(hwnd, pid);
    return pid.readU32() === Process.id;
}
function describe(hwnd) {
    return 'hwnd=' + hwnd + ' class=' + JSON.stringify(text(hwnd, getClass)) +
        ' text=' + JSON.stringify(text(hwnd, getTitle));
}
function snapshot(hwnd) {
    log('[ROOT] ' + describe(hwnd));
    var count = 0;
    var callback = new NativeCallback(function(child) {
        count++;
        log('[CONTROL] ' + describe(child));
        return 1;
    }, 'int', ['pointer', 'pointer']);
    enumChildren(hwnd, callback, ptr(0));
    log('[CHILD_COUNT] ' + count);
}
var initial = new NativeCallback(function(hwnd) {
    if (ours(hwnd)) snapshot(hwnd);
    return 1;
}, 'int', ['pointer', 'pointer']);
enumWindows(initial, ptr(0));
var events = { 0x6: 'ACTIVATE', 0x7: 'FOCUS', 0x18: 'SHOW', 0x100: 'KEYDOWN', 0x111: 'COMMAND', 0x202: 'CLICK' };
Interceptor.attach(address('DispatchMessageW'), {
    onEnter: function(args) {
        try {
            var msg = args[0];
            var hwnd = msg.readPointer();
            if (hwnd.isNull() || !ours(hwnd)) return;
            var id = msg.add(4).readU32();
            var wParam = msg.add(8).readU32(), lParam = msg.add(12).readU32();
            if (id === 0x100 && wParam === 0x78) snapshot(ancestor(hwnd, 2)); // F9
            if (events[id]) log('[EVENT ' + events[id] + '] ' + describe(hwnd) +
                ' wParam=0x' + wParam.toString(16) + ' lParam=0x' + lParam.toString(16));
        } catch (e) { log('[ERROR] ' + e.message); }
    }
});
log('[READY] Use the selector and Settings; F9 snapshots the current window. No speech producer installed.');
